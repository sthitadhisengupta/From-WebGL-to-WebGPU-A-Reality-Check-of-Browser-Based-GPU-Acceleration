#include <iostream>
#include <vector>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>

#include <cassert>
#include <sstream>
#include <string>
#include <array>
#include <cstdlib>

#include <iomanip>
#include <cmath>

#include <ctime>

const char* vertexShaderSource =
    "#version 300 es                            \n"
    "layout(location = 0) in vec2 aPosition;    \n"
    "layout(location = 1) in vec2 aUV;          \n"
    "out vec2 uv;                               \n"
    "void main() {                              \n"
    "    gl_Position = vec4(aPosition, 0.0, 1.0);    \n"
    "    uv = aUV;                              \n"
    "}                                          \n";

const char* fragmentShaderSource =
    "#version 300 es                              \n"
    "precision highp float;                       \n"
    "uniform sampler2D samplerData;               \n"
    "uniform sampler2D samplerMean;               \n"
    "uniform int mode;                            \n"
    "uniform float float_n;                       \n"
    "in vec2 uv;                                  \n"
    "out vec4 glColor;                            \n"
    "void main() {                                \n"
    "    int m = 2600; \n"
    "    int n = 3000; \n"
    "    int im = int(uv.y * float(m));            \n"
    "    int jm = int(uv.x * float(m));            \n"
    "    float outputVal = 0.0;                                           \n"
    "    if (mode == 0) {                         \n" 
    "        float sum = 0.0;                     \n"
    "        for (int i = 0; i < n; ++i) {   \n"
    "            sum += texture(samplerData, vec2(uv.x, float(i) / float(n))).r; \n"
    "        }                                    \n"
    "        float mean = sum / float(n);    \n"
    "        outputVal = mean;                   \n"
    "    } else if (mode == 1) {                  \n" 
    "        float mean = texture(samplerMean, vec2(uv.x, 1)).r; \n"
    "        float value = texture(samplerData, uv).r; \n"
    "        value -= mean;                            \n"
    "        outputVal = value;                     \n"
    "    } else if (mode == 2) {                  \n"
    "       float covVal = 0.0;                      \n"
    "       if (im <= jm) {                            \n"
    "           for (int k = 0; k < n; ++k) {        \n"
    "               float dataki = texture(samplerData, vec2(uv.y, float(k) / float(n))).r;   \n"
    "               float datakj = texture(samplerData, vec2(uv.x, float(k) / float(n))).r;   \n"
    "               covVal += dataki * datakj;                   \n"
    "           }                                    \n"
    "           covVal /= (float_n - 1.0);           \n"
    "       }                                        \n"
    "        outputVal = covVal;                     \n"
    "    }                                        \n"
    "    glColor = vec4(outputVal, 0.0, 0.0, 1.0);                        \n"
    "}                                            \n";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
    }
    return shader;
}

GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program Linking Error: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void initScene(GLuint program) {
    float positions[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    float uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    GLuint VAO, VBO, EBO, UVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &UVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, UVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glUseProgram(program);
}

GLuint createDataTexture(const float* data, int width, int height, int textureUnit) {
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);
    glBindTexture(GL_TEXTURE_2D, 0); 
    return texture;
}

GLuint createFramebuffer(int width, int height) {
    GLuint framebuffer, texture;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete" << std::endl;
    }
    return framebuffer;
}

std::string to_iso_string_with_ms(std::chrono::high_resolution_clock::time_point tp) {
    using namespace std::chrono;

    auto now_sys = system_clock::now();
    auto now_high = high_resolution_clock::now();
    auto tp_sys = now_sys + duration_cast<system_clock::duration>(tp - now_high);

    std::time_t time = system_clock::to_time_t(tp_sys);
    auto ms = duration_cast<milliseconds>(tp_sys.time_since_epoch()).count() % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time), "%FT%T")
        << '.' << std::setw(3) << std::setfill('0') << ms << 'Z';

    return oss.str();
}


EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
GLuint program, program2, program3;
GLuint framebufferMean, framebufferNormalize, framebufferCovariance;
GLuint textureData, textureMean, textureNormalizedData, textureCovariance;

std::chrono::high_resolution_clock::time_point start_time;

int m = 2600, n = 3000;

void render() {
    glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "mode"), 0);

    glUniform1f(glGetUniformLocation(program, "float_n"), static_cast<float>(n));

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferMean);
    glViewport(0, 0, m, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureData);
    glUniform1i(glGetUniformLocation(program, "samplerData"), 0);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    float resultMean[m];


    glReadPixels(0, 0, m, 1, GL_RED, GL_FLOAT, resultMean);


    textureMean = createDataTexture(resultMean, m, 1, 0);


    glUniform1i(glGetUniformLocation(program, "mode"), 1);

    glUniform1f(glGetUniformLocation(program, "float_n"), static_cast<float>(n));

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferNormalize);
    glViewport(0, 0, m, n);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureData);
    glUniform1i(glGetUniformLocation(program, "samplerData"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureMean);
    glUniform1i(glGetUniformLocation(program, "samplerMean"), 1);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    float resultData[n*m];


    glReadPixels(0, 0, m, n, GL_RED, GL_FLOAT, resultData);


    textureData = createDataTexture(resultData, m, n, 0);

    glUniform1i(glGetUniformLocation(program, "mode"), 2);

    glUniform1f(glGetUniformLocation(program, "float_n"), static_cast<float>(n));

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferCovariance);
    glViewport(0, 0, m, m);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureData);
    glUniform1i(glGetUniformLocation(program, "samplerData"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureMean);
    glUniform1i(glGetUniformLocation(program, "samplerMean"), 1);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    float resultCovariance[m * m];


    glReadPixels(0, 0, m, m, GL_RED, GL_FLOAT, resultCovariance);


    for (int i = 0; i < m; ++i) {
        for (int j = i; j < m; ++j) {
            resultCovariance[j * m + i] = resultCovariance[i * m + j];
        }
    }


    auto end_time = std::chrono::high_resolution_clock::now();


    std::chrono::duration<double> elapsed_time = end_time - start_time;
    std::cout << "Time taken: " << elapsed_time.count() << " seconds" << std::endl;

    emscripten_cancel_main_loop();
}

void init_array(int n, int m, float* data) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            data[i * m + j] = static_cast<float>(i * j) / m;
        }
    }
}

int main() {

    start_time= std::chrono::high_resolution_clock::now();

    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.majorVersion = 2;
    context = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(context);

    if (!emscripten_webgl_enable_extension(context, "EXT_color_buffer_float")) {
        std::cerr << "EXT_color_buffer_float extension not supported!" << std::endl;
        return 1;
    }

    program = createProgram(vertexShaderSource, fragmentShaderSource);
    initScene(program);


    float data[n * m];
    float mean[m];
    init_array(n, m, data);


    textureData = createDataTexture(data, m, n, 0);

    textureMean = createDataTexture(mean, m, 1, 0);
    
    framebufferMean = createFramebuffer(m, 1);
    framebufferNormalize = createFramebuffer(m, n);
    framebufferCovariance = createFramebuffer(m, m);

    emscripten_set_main_loop(render, 0, 1);

    return 0;
}
