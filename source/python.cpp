#include "python.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <mutex>
#include <vector>

namespace PyEngine {

static bool g_initialized = false;
static std::string g_lastError;

// Keeps PyMethodDef objects alive for the lifetime of the interpreter.
// PyCFunction_New stores a raw pointer to the PyMethodDef, so it must outlive
// the PyCFunction — never pass a stack-local struct.
static std::vector<PyMethodDef>* g_methodDefs = nullptr;

bool Init() {
    if (g_initialized) {
        return true;
    }

    // Python 3.12 removed the wchar_t* overload of Py_SetProgramName and
    // deprecated the whole family in favor of PyConfig. We don't strictly
    // need to set a program name — Python will resolve sys.executable from
    // the process image. Leaving it unset is valid.
    //
    // If you later want explicit control, use PyConfig:
    //   PyConfig cfg;
    //   PyConfig_InitPythonConfig(&cfg);
    //   PyConfig_SetBytesString(&cfg, &cfg.program_name, "DATS");
    //   Py_InitializeFromConfig(&cfg);
    //   PyConfig_Clear(&cfg);

    Py_Initialize();

    if (!Py_IsInitialized()) {
        g_lastError = "Failed to initialize Python interpreter.";
        std::cerr << "[PY] FATAL: " << g_lastError << std::endl;
        return false;
    }

    // Allocate the method-definition table on the heap so registered
    // callbacks survive past this function.
    g_methodDefs = new std::vector<PyMethodDef>();

    g_initialized = true;
    g_lastError.clear();
    std::cout << "[PY] Embedded Python runtime initialized." << std::endl;
    return true;
}

void Shutdown() {
    if (!g_initialized) {
        return;
    }

    // Py_FinalizeEx returns -1 on error (e.g. a module refused to finalize).
    // We don't abort, but we do surface it.
    int rc = Py_FinalizeEx();
    if (rc < 0) {
        std::cerr << "[PY] WARNING: Py_FinalizeEx reported an error." << std::endl;
    }

    // Safe now — the interpreter no longer holds pointers into these.
    delete g_methodDefs;
    g_methodDefs = nullptr;

    g_initialized = false;
    g_lastError.clear();
    std::cout << "[PY] Embedded Python runtime shutdown." << std::endl;
}

bool RunFile(const std::string& path) {
    if (!g_initialized) {
        g_lastError = "Python interpreter not initialized.";
        return false;
    }

    FILE* fp = nullptr;
#ifdef _WIN32
    if (fopen_s(&fp, path.c_str(), "r") != 0 || !fp) {
#else
    fp = fopen(path.c_str(), "r");
    if (!fp) {
#endif
        g_lastError = "Failed to open Python script: " + path;
        std::cerr << "[PY] " << g_lastError << std::endl;
        return false;
    }

    // PyRun_SimpleFile takes ownership of the FILE only on success.
    // On failure we close it ourselves.
    int result = PyRun_SimpleFile(fp, path.c_str());

    // PyRun_SimpleFileEx (the Ex variant) closes the file; the non-Ex
    // variant does not. Always close on our side.
    if (fp) {
        fclose(fp);
    }

    if (result != 0) {
        g_lastError = "Python script execution failed: " + path;
        std::cerr << "[PY] " << g_lastError << std::endl;
        return false;
    }

    g_lastError.clear();
    return true;
}

bool RunString(const std::string& code) {
    if (!g_initialized) {
        g_lastError = "Python interpreter not initialized.";
        return false;
    }

    int result = PyRun_SimpleString(code.c_str());
    if (result != 0) {
        g_lastError = "Python string execution failed.";
        std::cerr << "[PY] " << g_lastError << std::endl;
        return false;
    }

    g_lastError.clear();
    return true;
}

std::string CallStringFunc(const std::string& module,
                           const std::string& name,
                           const std::string& arg) {
    if (!g_initialized) {
        g_lastError = "Python interpreter not initialized.";
        return "";
    }

    PyObject* pModule = PyImport_ImportModule(module.c_str());
    if (!pModule) {
        PyErr_Print();
        g_lastError = "Failed to import Python module: " + module;
        return "";
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, name.c_str());
    Py_DECREF(pModule);
    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_XDECREF(pFunc);
        g_lastError = "Python function not found: " + module + "." + name;
        return "";
    }

    PyObject* pArgs = Py_BuildValue("(s)", arg.c_str());
    if (!pArgs) {
        Py_DECREF(pFunc);
        g_lastError = "Failed to build Python arguments.";
        return "";
    }

    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
    Py_DECREF(pFunc);

    if (!pResult) {
        PyErr_Print();
        g_lastError = "Python call failed: " + module + "." + name;
        return "";
    }

    std::string out;
    if (PyUnicode_Check(pResult)) {
        PyObject* pyStr = PyUnicode_AsUTF8String(pResult);
        if (pyStr) {
            const char* cstr = PyBytes_AsString(pyStr);
            if (cstr) {
                out = cstr;
            }
            Py_DECREF(pyStr);
        }
    } else {
        // Fall back to string representation.
        PyObject* pyStr = PyObject_Str(pResult);
        if (pyStr) {
            PyObject* utf8 = PyUnicode_AsUTF8String(pyStr);
            if (utf8) {
                const char* cstr = PyBytes_AsString(utf8);
                if (cstr) {
                    out = cstr;
                }
                Py_DECREF(utf8);
            }
            Py_DECREF(pyStr);
        }
    }

    Py_DECREF(pResult);
    g_lastError.clear();
    return out;
}

void RegisterCallback(const std::string& name, PyCFunction func) {
    if (!g_initialized || g_methodDefs == nullptr) {
        return;
    }

    PyObject* builtins = PyEval_GetBuiltins();
    if (!builtins) {
        return;
    }

    // PyCFunction_New stores a raw pointer to the PyMethodDef — it does NOT
    // copy the struct. Keep a permanent heap copy whose lifetime matches the
    // interpreter. Also intern the name string so its c_str() outlives the
    // call to this function.
    PyMethodDef def{};
    // ml_name must remain valid — Python may dereference it later.
    // Intern it with PyUnicode_InternFromString and keep the PyObject alive.
    // A simpler approach: push_back into the persistent vector and use that
    // element's storage; the std::string inside the vector is stable because
    // vector reallocation moves the struct, not the string payload's heap
    // allocation the c_str() points to... except that's UB-adjacent.
    //
    // The cleanest fix: hold the name in a static interned set on the Python
    // side, and store the PyMethodDef in a vector. Because C++ std::string
    // small-string optimization can inline short strings, a vector realloc
    // would move the buffer. To be safe we allocate names individually.
    //
    // We use PyUnicode_InternFromString — Python keeps it alive itself.
    PyObject* pName = PyUnicode_InternFromString(name.c_str());
    if (!pName) {
        return;
    }
    // The interned name lives for the interpreter's lifetime.
    // (We deliberately leak one reference — this is a global registry.)

    def.ml_name = PyUnicode_AsUTF8(pName);
    def.ml_meth = func;
    def.ml_flags = METH_VARARGS;
    def.ml_doc = nullptr;

    // Reserve first so the push_back never reallocates the vector after
    // we've captured &g_methodDefs->back().
    g_methodDefs->reserve(g_methodDefs->size() + 1);
    g_methodDefs->push_back(def);

    PyMethodDef* stored = &g_methodDefs->back();
    PyObject* pFunc = PyCFunction_New(stored, nullptr);
    if (!pFunc) {
        Py_DECREF(pName);
        return;
    }

    PyDict_SetItemString(builtins, name.c_str(), pFunc);
    Py_DECREF(pFunc);
    // Do NOT Py_DECREF(pName) — the interpreter may deref ml_name later.
    // (One leaked reference per registered callback; register at startup.)
}

std::string LastError() {
    return g_lastError;
}

bool IsInitialized() {
    return g_initialized;
}

} // namespace PyEngine