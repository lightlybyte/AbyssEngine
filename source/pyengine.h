#pragma once
// ---------------------------------------------------------------------------
// pyengine.h — Python embedding wrapper.
//
// RENAMED from `python.h`. The old name collided with CPython's `Python.h`
// on Windows' case-insensitive filesystem: `#include <python.h>` would
// resolve to CPython's header via the -I path before hitting our local
// wrapper, so `namespace PyEngine` was never declared in that TU.
// Renaming to `pyengine.h` removes the ambiguity entirely.
// ---------------------------------------------------------------------------

#include <Python.h>
#include <string>

// Verify the CPython header really loaded. If Python.h resolved to a stub
// or was skipped, one of these fires with a single clear message.
#ifndef Py_PYTHON_H
#  error "pyengine.h: <Python.h> did not define Py_PYTHON_H."
#endif
#if !defined(METH_VARARGS)
#  error "pyengine.h: <Python.h> did not define METH_VARARGS."
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