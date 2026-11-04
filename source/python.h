#pragma once
// ---------------------------------------------------------------------------
// python.h — Python embedding interface.
//
// This header VERIFIES that <Python.h> actually defined the API before we
// declare anything that uses it. If Python.h is missing or half-included,
// the static_assert at the bottom of the includes produces a readable
// diagnostic instead of a cascade of "'PyObject' has not been declared".
// ---------------------------------------------------------------------------

#include <string>

// Pull in the full Python C API. No extern "C" here: CPython's own headers
// already carry the correct linkage macros (PyAPI_FUNC etc.), and wrapping
// them again in extern "C" is what broke the previous attempt — some of
// Python.h's own nested extern "C" blocks don't survive a second wrap.
#include <Python.h>

// If the include above didn't define PyObject, this static_assert fires with
// a single clear message. C++11 static_assert is constexpr, so it can't be
// built from a runtime expression — we do it with a template trick.
namespace PyEngine_detail {
    template <typename T> struct dependent_false { static const bool value = false; };
}

// ---- Sanity: if any of these are undefined we stop here with one message ----
#ifndef Py_PYTHON_H
#  error "Python.h did not define Py_PYTHON_H — the header guard was not entered. Check that the include path resolves to a real CPython header."
#endif

// PyCFunction and PyObject are both typedef'd by Python.h:
//   typedef struct _object PyObject;
//   typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
// If they aren't defined, the include didn't take.
#if !defined(METH_VARARGS)
#  error "Python.h did not define METH_VARARGS — the method-def table section of the header did not compile. Check for a conflicting pyconfig.h."
#endif

namespace PyEngine {

bool Init();
void Shutdown();

bool RunFile(const std::string& path);
bool RunString(const std::string& code);

std::string CallStringFunc(const std::string& module,
                           const std::string& name,
                           const std::string& arg);

void RegisterCallback(const std::string& name, PyCFunction func);

std::string LastError();
bool IsInitialized();

} // namespace PyEngine