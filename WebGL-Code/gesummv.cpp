#include <iostream>
#include <vector>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>

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
    "uniform sampler2D samplerA;                  \n"
    "uniform sampler2D samplerB;                  \n"
    "uniform sampler2D samplerX;                  \n"
    "uniform float alpha;                         \n"
    "uniform float beta;                          \n"
    "in vec2 uv;                                  \n"
    "out vec4 glColor;                            \n"
    "void main() {                                \n"
    "    int row = int(uv.x * float(textureSize(samplerA, 0).y));    \n"
    "    float tmp = 0.0;                         \n"
    "    float y = 0.0;                           \n"
    "    int size = textureSize(samplerA, 0).x;   \n"
    "    for (int k = 0; k < size; ++k) {         \n"
    "        vec2 uvA = vec2(float(k) / float(size), float(row) / float(size)); \n"
    "        vec2 uvB = vec2(float(k) / float(size), float(row) / float(size)); \n"
    "        vec2 uvX = vec2(float(k) / float(size), 1.0);                      \n"
    "        float a = texture(samplerA, uvA).r;  \n"
    "        float b = texture(samplerB, uvB).r;  \n"
    "        float x = texture(samplerX, uvX).r;  \n"
    "        tmp += a * x;                        \n"
    "        y += b * x;                          \n"
    "    }                                        \n"
    "    y = alpha * tmp + beta * y;              \n"
    "    glColor = vec4(y, 0.0, 0.0, 1.0);        \n"
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

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
GLuint program, framebuffer;
GLuint textureA, textureB, textureX;
int n;
float alpha, beta;
std::chrono::high_resolution_clock::time_point start;

void render() {
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, n, 1);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    float result[n];
    glReadPixels(0, 0, n, 1, GL_RED, GL_FLOAT, result);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Total execution time: " << elapsed.count() << " seconds" << std::endl;


    emscripten_cancel_main_loop();
}

void init_array(int n, float& alpha, float& beta, float* A, float* B, float* x) {
    alpha = 1.5f;
    beta = 1.2f;
    for (int i = 0; i < n; i++) {
        x[i] = static_cast<float>(i % n) / n;
        for (int j = 0; j < n; j++) {
            A[i * n + j] = static_cast<float>((i * j + 1) % n) / n;
            B[i * n + j] = static_cast<float>((i * j + 2) % n) / n;
        }
    }
}

int main() {

    start = std::chrono::high_resolution_clock::now();

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

    n = 2800;

    float A[n * n];

    float B[n * n];

    float x[n];

    init_array(n, alpha, beta, A, B, x);

    textureA = createDataTexture(A, n, n, 0);
    textureB = createDataTexture(B, n, n, 1);
    textureX = createDataTexture(x, n, 1, 2);

    framebuffer = createFramebuffer(n, 1);

    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glUniform1i(glGetUniformLocation(program, "samplerA"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glUniform1i(glGetUniformLocation(program, "samplerB"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureX);
    glUniform1i(glGetUniformLocation(program, "samplerX"), 2);

    glUniform1f(glGetUniformLocation(program, "alpha"), alpha);
    glUniform1f(glGetUniformLocation(program, "beta"), beta);

    emscripten_set_main_loop(render, 0, 1);

    return 0;
}
