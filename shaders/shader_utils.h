// Shader utilities for the Abyss Engine terrain rendering system.
// Provides shader loading, uniform setup, and shader program management.

#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Shaders {

// Shader program holder
struct ShaderProgram {
    unsigned int id;
    unsigned int vertexShader;
    unsigned int fragmentShader;
};

// Load and compile shaders from GLSL source files
ShaderProgram loadShaders(
    const std::string& vertPath,
    const std::string& fragPath,
    const std::string& geomPath = ""
);

// Check shader compilation status
bool checkCompileStatus(unsigned int shader, const std::string& type);

// Link shader program with error checking
unsigned int linkProgram(unsigned int vertexShader, unsigned int fragmentShader,
                         unsigned int geometryShader = 0);

// Delete shaders
void deleteShaders(ShaderProgram& program);

// Uniform setters
void setMat4(unsigned int location, const glm::mat4& matrix);
void setVec3(unsigned int location, const glm::vec3& vector);
void setFloat(unsigned int location, float value);
void setInt(unsigned int location, int value);

} // namespace Shaders
