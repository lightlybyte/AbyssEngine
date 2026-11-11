#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Shaders {

ShaderProgram loadShaders(const std::string& vertPath, const std::string& fragPath,
                          const std::string& geomPath) {
    ShaderProgram program;

    // Read vertex shader
    std::ifstream vertFile(vertPath);
    if (!vertFile.is_open()) {
        std::cerr << "ERROR: Cannot open vertex shader: " << vertPath << std::endl;
        return program;
    }
    std::stringstream vertStream;
    vertStream << vertFile.rdbuf();
    std::string vertCode = vertStream.str();
    vertFile.close();

    // Read fragment shader
    std::ifstream fragFile(fragPath);
    if (!fragFile.is_open()) {
        std::cerr << "ERROR: Cannot open fragment shader: " << fragPath << std::endl;
        return program;
    }
    std::stringstream fragStream;
    fragStream << fragFile.rdbuf();
    std::string fragCode = fragStream.str();
    fragFile.close();

    // Vertex shader
    unsigned int vertShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertSource = vertCode.c_str();
    glShaderSource(vertShader, 1, &vertSource, NULL);
    glCompileShader(vertShader);

    if (!checkCompileStatus(vertShader, "VERTEX")) {
        return program;
    }

    // Fragment shader
    unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragSource = fragCode.c_str();
    glShaderSource(fragShader, 1, &fragSource, NULL);
    glCompileShader(fragShader);

    if (!checkCompileStatus(fragShader, "FRAGMENT")) {
        glDeleteShader(vertShader);
        return program;
    }

    // Geometry shader (optional)
    unsigned int geomShader = 0;
    if (!geomPath.empty()) {
        std::ifstream geomFile(geomPath);
        if (geomFile.is_open()) {
            std::stringstream geomStream;
            geomStream << geomFile.rdbuf();
            std::string geomCode = geomStream.str();
            geomFile.close();

            geomShader = glCreateShader(GL_GEOMETRY_SHADER);
            const char* geomSource = geomCode.c_str();
            glShaderSource(geomShader, 1, &geomSource, NULL);
            glCompileShader(geomShader);

            if (!checkCompileStatus(geomShader, "GEOMETRY")) {
                glDeleteShader(vertShader);
                glDeleteShader(fragShader);
                return program;
            }
        }
    }

    // Link program
    program.id = linkProgram(vertShader, fragShader, geomShader);
    program.vertexShader = vertShader;
    program.fragmentShader = fragShader;

    // Cleanup shaders after linking
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    if (geomShader) glDeleteShader(geomShader);

    return program;
}

bool checkCompileStatus(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                  << infoLog << std::endl;
        return false;
    }
    return true;
}

unsigned int linkProgram(unsigned int vertexShader, unsigned int fragmentShader,
                         unsigned int geometryShader) {
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    if (geometryShader != 0) {
        glAttachShader(program, geometryShader);
    }
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER_PROGRAM_LINKING_ERROR:\n" << infoLog << std::endl;
        return 0;
    }

    return program;
}

void deleteShaders(ShaderProgram& program) {
    glDeleteProgram(program.id);
    program.id = 0;
    program.vertexShader = 0;
    program.fragmentShader = 0;
}

void setMat4(unsigned int location, const glm::mat4& matrix) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

void setVec3(unsigned int location, const glm::vec3& vector) {
    glUniform3fv(location, 1, glm::value_ptr(vector));
}

void setFloat(unsigned int location, float value) {
    glUniform1f(location, value);
}

void setInt(unsigned int location, int value) {
    glUniform1i(location, value);
}

} // namespace Shaders
