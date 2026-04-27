#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>

#include <stdlib.h>
#include <time.h>

#define DATA_TYPE float


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
    "uniform sampler2D B;               \n"
    "uniform sampler2D D;               \n"
    "uniform sampler2D C;               \n"
    "uniform sampler2D TempArr;               \n"
    "uniform int ni;                               \n"
    "uniform int nj;                               \n"
    "uniform int nk;                               \n"
    "uniform int nl;                               \n"
    "uniform float alpha;                               \n"
    "uniform float beta;                               \n"
    "uniform int m;                               \n"
    "uniform int n;                               \n"
    "uniform int callNumber;                        \n"
    "in vec2 uv;                                  \n"
    "out vec4 glColor;                            \n"
    "void main() {                                \n"
    "    float result = 0.0;                      \n"
    "    float outVar = 0.0;                      \n"
    "                                                          \n"
    "    if(callNumber == 1){                        \n"
    "       int i = int(uv.y * float(ni));            \n"
    "       int j = int(uv.x * float(nj));            \n"
    "       for (int k = 0; k < nk; ++k) {            \n"
    "           float a_ik = texture(A, vec2(float(k) / float(nk), float(i) / float(ni))).r; \n"
    "           float b_kj = texture(B, vec2(float(j) / float(nj), float(k) / float(nk))).r; \n"
    "           result += alpha * a_ik * b_kj;                \n"
    "       }                                        \n"
    "       outVar=result;                                           \n"
    "     }                                            \n"
    "                                              \n"
    "    if(callNumber == 2){                        \n"
    "       int i = int(uv.y * float(ni));            \n"
    "       int j = int(uv.x * float(nl));            \n"
    "       result = beta * texture(D, vec2(float(j) / float(nl), float(i) / float(ni))).r; \n"
    "       for (int k = 0; k < nj; ++k) {            \n"
    "           float tmpval = texture(TempArr, vec2(float(k) / float(nj), float(i) / float(ni))).r; \n"
    "           float c_kj = texture(C, vec2(float(j) / float(nl), float(k) / float(nj))).r; \n"
    "           result += tmpval * c_kj;             \n"
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
GLuint program, framebuffer1, framebuffer2;
GLuint textureA, textureB, textureC, textureD, textureTmp;

std::chrono::high_resolution_clock::time_point start_time, start_time2, start_time3, start_time4;


int main() {



    start_time= std::chrono::high_resolution_clock::now();


    int ni = 180, nj = 190, nk = 210, nl = 220;

    DATA_TYPE A[ni * nk];
    DATA_TYPE B[nk * nj];
    DATA_TYPE C[nj * nl];
    DATA_TYPE D[ni * nl];

    DATA_TYPE tmp[ni * nj];

    DATA_TYPE alpha = 1.5;
    DATA_TYPE beta = 1.2;
    
    for (int i = 0; i < ni; i++)
        for (int j = 0; j < nk; j++)
            A[i * nk + j] = (float)((i * j + 1) % ni) / ni;

    for (int i = 0; i < nk; i++)
        for (int j = 0; j < nj; j++)
            B[i * nj + j] = (float)(i * (j + 1) % nj) / nj;

    for (int i = 0; i < nj; i++)
        for (int j = 0; j < nl; j++)
            C[i * nl + j] = (float)((i * (j + 3) + 1) % nl) / nl;

    for (int i = 0; i < ni; i++)
        for (int j = 0; j < nl; j++)
            D[i * nl + j] = (float)(i * (j + 2) % nk) / nk;


    auto start_init1 = std::chrono::high_resolution_clock::now();

    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.majorVersion = 2;
    context = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(context);

   auto end_init1 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_init1 = end_init1 - start_init1;
    std::cout << "WebGL context setup overhead: " << elapsed_init1.count() << " seconds" << std::endl;


    auto start_init2 = std::chrono::high_resolution_clock::now();

    if (!emscripten_webgl_enable_extension(context, "EXT_color_buffer_float")) {
        std::cerr << "EXT_color_buffer_float extension not supported!" << std::endl;
        return 1;
    }

   auto end_init2 = std::chrono::high_resolution_clock::now();

   std::chrono::duration<double> elapsed_init2 = end_init2 - start_init2;
   std::cout << "Enabling WebGL extension color_buffer_float overhead: " << elapsed_init2.count() << " seconds" << std::endl;

    program = createProgram(vertexShaderSource, fragmentShaderSource);
    initScene(program);
    

    glUseProgram(program);


    textureA = createDataTexture(A, nk, ni, 0);
    textureB = createDataTexture(B, nj, nk, 1);
    textureC = createDataTexture(C, nl, nj, 2);
    textureD = createDataTexture(D, nl, ni, 3); 
 

    framebuffer1 = createFramebuffer(nj, ni);
    framebuffer2 = createFramebuffer(nl, ni);


    glUniform1i(glGetUniformLocation(program, "callNumber"), 1);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1);
    glViewport(0, 0, nj, ni);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glUniform1i(glGetUniformLocation(program, "A"), 0);


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glUniform1i(glGetUniformLocation(program, "B"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureC);
    glUniform1i(glGetUniformLocation(program, "C"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textureD);
    glUniform1i(glGetUniformLocation(program, "D"), 3);


    glUniform1i(glGetUniformLocation(program, "ni"), ni);
    glUniform1i(glGetUniformLocation(program, "nj"), nj);
    glUniform1i(glGetUniformLocation(program, "nk"), nk);
    glUniform1i(glGetUniformLocation(program, "nl"), nl);

    glUniform1f(glGetUniformLocation(program, "alpha"), alpha);
    glUniform1f(glGetUniformLocation(program, "beta"), beta);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);



    glReadPixels(0, 0, nj, ni, GL_RED, GL_FLOAT, tmp);



    textureTmp = createDataTexture(tmp, nj, ni, 4);
    


    glUniform1i(glGetUniformLocation(program, "callNumber"), 2);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2);
    glViewport(0, 0, nl, ni);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glUniform1i(glGetUniformLocation(program, "A"), 0);


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glUniform1i(glGetUniformLocation(program, "B"), 1);


    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureC);
    glUniform1i(glGetUniformLocation(program, "C"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textureD);
    glUniform1i(glGetUniformLocation(program, "D"), 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, textureTmp);
    glUniform1i(glGetUniformLocation(program, "TempArr"), 4);

    glUniform1i(glGetUniformLocation(program, "ni"), ni);
    glUniform1i(glGetUniformLocation(program, "nj"), nj);
    glUniform1i(glGetUniformLocation(program, "nk"), nk);
    glUniform1i(glGetUniformLocation(program, "nl"), nl);

    glUniform1f(glGetUniformLocation(program, "alpha"), alpha);
    glUniform1f(glGetUniformLocation(program, "beta"), beta);

    glClear(GL_COLOR_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    glReadPixels(0, 0, nl, ni, GL_RED, GL_FLOAT, D);


    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_time = end_time - start_time;
    std::cout << "Time taken: " << elapsed_time.count() << " seconds" << std::endl; 






    return 0;
}
