// ---------------------------------------------------------------------------
// python.cpp — Python embedding implementation.
//
// Includes `pyengine.h` (renamed from python.h — see that header's comment
// about case-insensitive filesystem collision with CPython's Python.h).
// ---------------------------------------------------------------------------

#include "pyengine.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <mutex>
#include <vector>

namespace PyEngine {

static bool g_initialized = false;
static std::string g_lastError;
static std::vector<PyMethodDef>* g_methodDefs = nullptr;

bool Init() {
    if (g_initialized) return true;

    Py_Initialize();
    if (!Py_IsInitialized()) {
        g_lastError = "Failed to initialize Python interpreter.";
        std::cerr << "[PY] FATAL: " << g_lastError << std::endl;
        return false;
    }

    g_methodDefs = new std::vector<PyMethodDef>();
    g_initialized = true;
    g_lastError.clear();
    std::cout << "[PY] Embedded Python runtime initialized." << std::endl;
    return true;
}

void Shutdown() {
    if (!g_initialized) return;
    int rc = Py_FinalizeEx();
    if (rc < 0) {
        std::cerr << "[PY] WARNING: Py_FinalizeEx reported an error." << std::endl;
    }
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
    int result = PyRun_SimpleFile(fp, path.c_str());
    if (fp) fclose(fp);
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
            if (cstr) out = cstr;
            Py_DECREF(pyStr);
        }
    } else {
        PyObject* pyStr = PyObject_Str(pResult);
        if (pyStr) {
            PyObject* utf8 = PyUnicode_AsUTF8String(pyStr);
            if (utf8) {
                const char* cstr = PyBytes_AsString(utf8);
                if (cstr) out = cstr;
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
    if (!g_initialized || g_methodDefs == nullptr) return;
    PyObject* builtins = PyEval_GetBuiltins();
    if (!builtins) return;

    PyObject* pName = PyUnicode_InternFromString(name.c_str());
    if (!pName) return;

    PyMethodDef def{};
    def.ml_name  = PyUnicode_AsUTF8(pName);
    def.ml_meth  = func;
    def.ml_flags = METH_VARARGS;
    def.ml_doc   = nullptr;

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
}

std::string LastError() { return g_lastError; }
bool IsInitialized()     { return g_initialized; }

} // namespace PyEngine