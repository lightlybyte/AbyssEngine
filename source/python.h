#pragma once

#include <Python.h>
#include <string>
#include <vector>
#include <functional>

namespace PyEngine {

// Initialize the embedded Python interpreter. Returns false on fatal failure.
bool Init();

// Shutdown the embedded Python interpreter.
void Shutdown();

// Run a Python script file. Returns true on success.
bool RunFile(const std::string& path);

// Run a Python string. Returns true on success.
bool RunString(const std::string& code);

// Call a Python function by module and name, passing a single string argument.
// Returns the string result (or empty string on failure / non-string return).
std::string CallStringFunc(const std::string& module,
                           const std::string& name,
                           const std::string& arg);

// Push a C++ function into Python's builtins so scripts can call it.
void RegisterCallback(const std::string& name,
                      PyCFunction func);

// Last error message from a Python operation.
std::string LastError();

// Whether the interpreter is currently initialized.
bool IsInitialized();

} // namespace PyEngine
