#include <iostream>
#include <vector>
#include <iomanip>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>

constexpr int TSTEPS = 500;
constexpr int N = 1300;
using DATA_TYPE = float; 

const char* vertexShaderSource =
    "#version 300 es                            \n"
    "layout(location = 0) in vec2 aPosition;    \n"
    "layout(location = 1) in vec2 aUV;          \n"
    "out vec2 uv;                               \n"
    "void main() {                              \n"
    "    gl_Position = vec4(aPosition, 0.0, 1.0); \n"
    "    uv = aUV;                              \n"
    "}                                          \n";


const char* fragmentShaderSource = 
"#version 300 es\n"
"precision highp float;\n"
"uniform sampler2D A;\n"
"uniform sampler2D B;\n"
"uniform float scalarVal;\n"
"uniform int n;\n"
"uniform int mode;\n"
"in vec2 uv;\n"
"out float fragColor;\n"
"void main() {\n"
"    int i = int(uv.x * float(n));\n"
"    int j = int(uv.y * float(n));\n"
"    float result = 0.0;\n"
"    if (mode == 0) {\n"
"        if (i >= 1 && i < n - 1 && j >= 1 && j < n - 1) {\n"
"            float A_center = texture(A, uv).r;\n"
"            float A_left = texture(A, vec2(uv.x, uv.y - 1.0 / float(n))).r;\n"
"            float A_right = texture(A, vec2(uv.x, uv.y + 1.0 / float(n))).r;\n"
"            float A_top = texture(A, vec2(uv.x - 1.0 / float(n), uv.y)).r;\n"
"            float A_bottom = texture(A, vec2(uv.x + 1.0 / float(n), uv.y)).r;\n"
"            result = 0.2 * (A_center + A_left + A_right + A_top + A_bottom);\n"
"        } else {\n"
"            float B_center_x = float(i) / float(n);\n"
"            float B_center_y = float(j) / float(n);\n"
"            float B_center = texture(B, vec2(B_center_x, B_center_y)).r;\n"
"            result = B_center;\n"
"        }\n"
"    }\n"
"    if (mode == 1) {\n"
"        if (i >= 1 && i < n - 1 && j >= 1 && j < n - 1) {\n"
"            float B_center = texture(B, uv).r;\n"
"            float B_left = texture(B, vec2(uv.x, uv.y - 1.0 / float(n))).r;\n"
"            float B_right = texture(B, vec2(uv.x, uv.y + 1.0 / float(n))).r;\n"
"            float B_top = texture(B, vec2(uv.x - 1.0 / float(n), uv.y)).r;\n"
"            float B_bottom = texture(B, vec2(uv.x + 1.0 / float(n), uv.y)).r;\n"
"            result = 0.2 * (B_center + B_left + B_right + B_top + B_bottom);\n"
"        } else {\n"
"            float A_center_x = float(i) / float(n);\n"
"            float A_center_y = float(j) / float(n);\n"
"            float A_center = texture(A, vec2(A_center_x, A_center_y)).r;\n"
"            result = A_center;\n"
"        }\n"
"    }\n"
"    fragColor = result;\n"
"}\n";


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

GLuint createDataTexture(const DATA_TYPE* data, int width, int height, int textureUnit) {
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
GLuint program;
GLuint framebufferA, framebufferB;
GLuint textureA, textureB, textureB2;

std::chrono::high_resolution_clock::time_point start_time;

void render() {
    glUseProgram(program);

    int n = N;
    int tsteps = TSTEPS;


    DATA_TYPE A[n * n];
    DATA_TYPE B[n * n];

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int index = i * N + j;  
            A[index] = ((DATA_TYPE)i * (j + 2) + 2) / n;
            B[index] = ((DATA_TYPE)i * (j + 3) + 3) / n;
        }
    }


    glUniform1i(glGetUniformLocation(program, "n"), n);


    for (int t = 0; t < tsteps; t++) {

        textureA = createDataTexture(A, n, n, 0);
        textureB = createDataTexture(B, n, n, 1);

        framebufferA = createFramebuffer(n, n);
        framebufferB = createFramebuffer(n, n);



        glUniform1i(glGetUniformLocation(program, "mode"), 0);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferB);
        glViewport(0, 0, n, n);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureA);
        glUniform1i(glGetUniformLocation(program, "A"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureB);
        glUniform1i(glGetUniformLocation(program, "B"), 1);


        glClear(GL_COLOR_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glReadPixels(0, 0, n, n, GL_RED, GL_FLOAT, B);

        textureB2 = createDataTexture(B, n, n, 1);


        glUniform1i(glGetUniformLocation(program, "mode"), 1);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferA);
        glViewport(0, 0, n, n);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureA);
        glUniform1i(glGetUniformLocation(program, "A"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureB2);
        glUniform1i(glGetUniformLocation(program, "B"), 1);

        glClear(GL_COLOR_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        glReadPixels(0, 0, n, n, GL_RED, GL_FLOAT, A);

        glDeleteTextures(1, &textureA);
        glDeleteTextures(1, &textureB);
        glDeleteTextures(1, &textureB2);
        glDeleteFramebuffers(1, &framebufferA);
        glDeleteFramebuffers(1, &framebufferB);

    }



    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_time = end_time - start_time;
    std::cout << "Time taken: " << elapsed_time.count() << " seconds" << std::endl;


    emscripten_cancel_main_loop();
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

    emscripten_set_main_loop(render, 0, 1);

    return 0;
}
