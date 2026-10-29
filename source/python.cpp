#include "python.h"

#include <iostream>
#include <sstream>
#include <cstring>

namespace PyEngine {

static bool g_initialized = false;
static std::string g_lastError;

bool Init() {
    if (g_initialized) {
        return true;
    }

    // Set program name so Python can locate its standard library.
    Py_SetProgramName(L"DATS");

    // Initialize the interpreter.
    Py_Initialize();

    if (!Py_IsInitialized()) {
        g_lastError = "Failed to initialize Python interpreter.";
        std::cerr << "[PY] FATAL: " << g_lastError << std::endl;
        return false;
    }

    g_initialized = true;
    std::cout << "[PY] Embedded Python runtime initialized." << std::endl;
    return true;
}

void Shutdown() {
    if (!g_initialized) {
        return;
    }

    Py_FinalizeEx();
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
    fclose(fp);

    if (result != 0) {
        g_lastError = "Python script execution failed: " + path;
        std::cerr << "[PY] " << g_lastError << std::endl;
        return false;
    }

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
    return out;
}

void RegisterCallback(const std::string& name, PyCFunction func) {
    if (!g_initialized) {
        return;
    }

    PyObject* builtins = PyEval_GetBuiltins();
    if (!builtins) {
        return;
    }

    PyMethodDef methodDef = {name.c_str(), func, METH_VARARGS, nullptr};
    PyObject* pFunc = PyCFunction_New(&methodDef, nullptr);
    if (!pFunc) {
        return;
    }

    PyDict_SetItemString(builtins, name.c_str(), pFunc);
    Py_DECREF(pFunc);
}

std::string LastError() {
    return g_lastError;
}

bool IsInitialized() {
    return g_initialized;
}

} // namespace PyEngine
