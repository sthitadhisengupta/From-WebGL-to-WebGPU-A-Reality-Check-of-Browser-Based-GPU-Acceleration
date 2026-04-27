#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>

#define DATA_TYPE float

constexpr int M = 1800;
constexpr int N = 2200;

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
    "#version 300 es                              \n"
    "precision highp float;                       \n"
    "uniform sampler2D A;               \n"
    "uniform sampler2D X;               \n"
    "uniform sampler2D TempArr;               \n"
    "uniform int m;                               \n"
    "uniform int n;                               \n"
    "uniform int callNumber;                        \n"
    "in vec2 uv;                                  \n"
    "out vec4 glColor;                            \n"
    "void main() {                                \n"
    "    float tmpVal = 0.0;                      \n"
    "    float result = 0.0;                      \n"
    "    float outVar = 0.0;                      \n"
    "                                                          \n"
    "    if(callNumber == 1){                        \n"
    "       int i = int(uv.x * float(m));            \n"
    "       // First loop: parallelize m times       \n"
    "       for (int k = 0; k < n; ++k) {            \n"
    "           float a_ik = texture(A, vec2(float(k) / float(n), float(i) / float(m))).r; \n"
    "           float x_k = texture(X, vec2(float(k) / float(n), 1.0)).r; \n"
    "           tmpVal += a_ik * x_k;                \n"
    "       }                                        \n"
    "       outVar=tmpVal;                                           \n"
    "     }                                            \n"
    "                                              \n"
    "    if(callNumber == 2){                        \n"
    "       int j = int(uv.x * float(n));            \n"
    "       // Second loop: parallelize n times      \n"
    "       for (int k = 0; k < m; ++k) {            \n"
    "           float a_kj = texture(A, vec2(float(j) / float(n), float(k) / float(m))).r; \n"
    "           tmpVal = texture(TempArr, vec2(float(k) / float(m), 1.0)).r;        \n"
    "           result += a_kj * tmpVal;             \n"
    "       }                                        \n"
    "       outVar=result;                                          \n"
    "     }                                            \n"
    "                                              \n"
    "    // Final result output                   \n"
    "    glColor = vec4(outVar, 0.0, 0.0, 1.0);   \n"
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
GLuint program, framebuffer1, framebuffer2, framebuffer3;
GLuint textureA, textureX, textureTmp;

std::chrono::high_resolution_clock::time_point start_time;


void render() {

    glUseProgram(program);

    DATA_TYPE y[N];
    DATA_TYPE tmp[M];
    DATA_TYPE A[M * N];
    DATA_TYPE x[N];

    DATA_TYPE fn = (DATA_TYPE)N;
    
    for (int i = 0; i < N; i++)
        x[i] = 1 + (i / fn);
        
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A[i * N + j] = (DATA_TYPE)((i + j) % N) / (5 * M);
        }
    }

    textureA = createDataTexture(A, N, M, 0);
    textureX = createDataTexture(x, N, 1, 1);

    framebuffer1 = createFramebuffer(N, 1);
    framebuffer2 = createFramebuffer(M, 1);
    framebuffer3 = createFramebuffer(N, 1);


    glUniform1i(glGetUniformLocation(program, "callNumber"), 1);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2);
    glViewport(0, 0, M, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glUniform1i(glGetUniformLocation(program, "A"), 0);


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureX);
    glUniform1i(glGetUniformLocation(program, "X"), 1);

    // Set uniform values for m and n
    glUniform1i(glGetUniformLocation(program, "m"), M);
    glUniform1i(glGetUniformLocation(program, "n"), N);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    glReadPixels(0, 0, M, 1, GL_RED, GL_FLOAT, tmp);



    textureTmp = createDataTexture(tmp, M, 1, 2);
    

    glUniform1i(glGetUniformLocation(program, "callNumber"), 2);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer3);
    glViewport(0, 0, N, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glUniform1i(glGetUniformLocation(program, "A"), 0);


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureX);
    glUniform1i(glGetUniformLocation(program, "X"), 1);


    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureTmp);
    glUniform1i(glGetUniformLocation(program, "TempArr"), 2);

    glUniform1i(glGetUniformLocation(program, "m"), M);
    glUniform1i(glGetUniformLocation(program, "n"), N);

    glClear(GL_COLOR_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    glReadPixels(0, 0, N, 1, GL_RED, GL_FLOAT, y);


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
    

    glUseProgram(program);


    emscripten_set_main_loop(render, 0, 1);

    return 0;
}
