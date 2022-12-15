#include "SafeGl.h"
#include <iostream>
#include <fstream>

namespace
{
    using namespace SafeGl;

    std::string loadTextFile(std::filesystem::path path)
    {
        std::ifstream file(path);
        if (!file) throw std::invalid_argument("Unable to open file");

        std::ostringstream sstream;
        sstream << file.rdbuf();
        return std::move(sstream).str();
    }

    Shader compileShader(GLenum shader_type, std::string_view src)
    {
        auto& gl = QGl();
        Shader shader(gl.glCreateShader(shader_type));
        const char* src_pointer = src.data();
        gl.glShaderSource(shader, 1, &src_pointer, NULL);
        gl.glCompileShader(shader);

        GLint result;
        GLint log_length;
        gl.glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
        gl.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            std::string log_message(log_length + 1, '\0');
            gl.glGetShaderInfoLog(shader, log_length, NULL, log_message.data());
            std::clog << "[Shader compilation] : " << log_message << std::endl;
        }
        if (result != GL_TRUE) throw std::runtime_error("Shader compilation failed");

        return shader;
    }

    Program compileShaderProgram(std::string_view vertex_src, std::string_view fragment_src)
    {
        auto& gl = QGl();
        Shader vertex_shader(compileShader(GL_VERTEX_SHADER, vertex_src));
        Shader fragment_shader(compileShader(GL_FRAGMENT_SHADER, fragment_src));
        Program program(gl.glCreateProgram());
        gl.glAttachShader(program, vertex_shader);
        gl.glAttachShader(program, fragment_shader);
        gl.glLinkProgram(program);

        GLint result;
        GLint log_length;
        gl.glGetProgramiv(program, GL_LINK_STATUS, &result);
        gl.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            std::string log_message(log_length + 1, '\0');
            gl.glGetProgramInfoLog(program, log_length, NULL, log_message.data());
            std::clog << "[Shader linking] : " << log_message << std::endl;
        }
        if (result != GL_TRUE) throw std::runtime_error("Shader linking failed");

        gl.glDetachShader(program, vertex_shader);
        gl.glDetachShader(program, fragment_shader);

        return program;
    }
}


namespace SafeGl
{
    Program loadAndCompileProgram(std::filesystem::path v, std::filesystem::path f)
    {
        return compileShaderProgram(loadTextFile(v), loadTextFile(f));
    }
}
