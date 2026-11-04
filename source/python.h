#pragma once
// ---------------------------------------------------------------------------
// python.h — Python embedding interface.
//
// <Python.h> MUST be the first include in any TU that touches the Python C
// API. The Python docs are explicit: pyconfig.h overrides feature-test
// macros, and headers that reference PyObject* before Python.h is seen
// produce the exact cascade the build log shows ('PyObject has not been
// declared', 'PyCFunction has not been declared').
//
// Including <Python.h> here means every TU that includes <python.h> gets the
// full Python API in scope automatically, in the right order.
// ---------------------------------------------------------------------------

// CPython's headers are C. On MSVC and on Linux's CPython ABI, the headers
// already carry their own extern "C" wrappers, but we open ours explicitly
// so the C++ lambdas we pass as PyCFunction match the C calling convention.
extern "C" {
#include <Python.h>
}

#include <string>

namespace PyEngine {

// Lifecycle
bool Init();
void Shutdown();

// Execution
bool RunFile(const std::string& path);
bool RunString(const std::string& code);

// Module / function calls
std::string CallStringFunc(const std::string& module,
                           const std::string& name,
                           const std::string& arg);

// Callback registration — PyCFunction is a C function pointer type provided
// by <Python.h>. Any TU including this header will see it.
void RegisterCallback(const std::string& name, PyCFunction func);

// Diagnostics
std::string LastError();
bool IsInitialized();

} // namespace PyEngine