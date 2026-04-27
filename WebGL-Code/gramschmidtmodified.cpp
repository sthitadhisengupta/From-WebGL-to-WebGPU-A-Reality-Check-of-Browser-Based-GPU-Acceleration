#include <iostream>
#include <vector>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>
#include <cmath>

const int M = 2000;
const int N = 2600;
typedef float DATA_TYPE;


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
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform sampler2D samplerA;\n"
    "uniform sampler2D samplerR;\n"
    "uniform sampler2D samplerQ;\n"
    "uniform sampler2D samplerResultR;\n"
    "uniform int passNumber;\n"
    "uniform int m;\n"
    "uniform int n;\n"
    "uniform int k;\n"
    "uniform float Rkk;\n"
    "in vec2 uv;\n"
    "out vec4 glColor;\n"
    "\n"
    "void main() {\n"
    "\n"
    "    float result = 0.0;\n"
    "\n"
    "    if (passNumber == 0) {\n"
    "        int i = int(uv.x * float(m));\n"
    "        vec2 uvA = vec2(float(k) / float(n), float(i) / float(m));\n"

    "        float Aik = texture(samplerA, uvA).r;\n"

    "        result = float(Aik) / float(Rkk);\n"
    "    }\n"
    "\n"
    "    if (passNumber == 1) {\n"
    "        int j = int(uv.x * float(n));\n"
    "        if (j >= k + 1) {\n"
    "           result = 0.0;\n"
    "           for (int a = 0; a < m; a++) {\n"
    "               vec2 uvA = vec2(float(j) / float(n), float(a) / float(m));\n"
    "               vec2 uvQ = vec2(float(a) / float(m), 1.0);\n"
    "               float Qik = texture(samplerQ, uvQ).r;\n"
    "               float Aij = texture(samplerA, uvA).r;\n"
    "               result += Qik * Aij;\n"
    "           }\n"
    "         }                        \n"
    "    }\n"
    "\n"
    "    if (passNumber == 2) {\n"
    "        int i = int(uv.y * float(m));\n"
    "        int j = int(uv.x * float(n));\n"
    "        result = 0.0;\n"
    "        vec2 uvA = vec2(float(j) / float(n), float(i) / float(m));\n"
    "        float Aij = texture(samplerA, uvA).r;\n"
    "        if (j >= k + 1) {\n"
    "            vec2 uvQ = vec2(float(i) / float(m), 1.0);\n"
    "            vec2 uvR = vec2(float(j) / float(n), float(k) / float(n));\n"
    "            float Qik = texture(samplerQ, uvQ).r;\n"
    "            float Rkj = texture(samplerResultR, uvR).r;\n"
    "            Aij -= Qik * Rkj;\n"
    "        }\n"
    "        result = Aij;\n"
    "    }\n"
    "\n"
    "    glColor = vec4(result, 0.0, 0.0, 1.0);\n"
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
GLuint program, framebuffer1, framebuffer2, framebuffer3;
GLuint textureA, textureQ, textureR, textureResultQ, textureResultR;
constexpr int m = M, n = N;
float A[m * n];
float Q[m * n];
float R[n * n];

std::chrono::high_resolution_clock::time_point start_time;

void render() {
    glUseProgram(program);

    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++) {
            A[i * n + j] = (((float)((i * j) % m) / m) * 100) + 10;
            Q[i * n + j] = 0.0;
        }

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            R[i * n + j] = 0.0;


    glUniform1i(glGetUniformLocation(program, "n"), n);

    glUniform1i(glGetUniformLocation(program, "m"), m);


    for (int k = 0; k < n; ++k) {

        framebuffer1 = createFramebuffer(m, 1);
        framebuffer2 = createFramebuffer(n, 1);
        framebuffer3 = createFramebuffer(n, m);


        float nrm = 0.0;

        for (int i = 0; i < m; i++)
            nrm += A[i * n + k] * A[i * n + k];
        
        R[k * n + k] = sqrtf(nrm);

        float Rkk = R[k * n + k];

        glUniform1f(glGetUniformLocation(program, "Rkk"), Rkk);

        textureA = createDataTexture(A, n, m, 0); 
        textureR = createDataTexture(R, n, n, 1);    


        glUniform1i(glGetUniformLocation(program, "passNumber"), 0);
        glUniform1i(glGetUniformLocation(program, "k"), k);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1);
        glViewport(0, 0, m, 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureA);
        glUniform1i(glGetUniformLocation(program, "samplerA"), 0);


        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureR);
        glUniform1i(glGetUniformLocation(program, "samplerR"), 1);

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        float result_q[m];
        glReadPixels(0, 0, m, 1, GL_RED, GL_FLOAT, result_q);



        for (int i = 0; i < m; ++i) {
            Q[i * n + k] = result_q[i];
        }


        glUniform1i(glGetUniformLocation(program, "passNumber"), 1);
        glUniform1i(glGetUniformLocation(program, "k"), k);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2);
        glViewport(0, 0, n, 1);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureA);
        glUniform1i(glGetUniformLocation(program, "samplerA"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureR);
        glUniform1i(glGetUniformLocation(program, "samplerR"), 1);


        textureResultQ = createDataTexture(result_q, m, 1, 2);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textureResultQ);
        glUniform1i(glGetUniformLocation(program, "samplerQ"), 2);


        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        float result_r[n];
        glReadPixels(0, 0, n, 1, GL_RED, GL_FLOAT, result_r);


        for (int j = k+1; j < n; ++j) {
            R[k * n + j] = result_r[j];
        }


        glUniform1i(glGetUniformLocation(program, "passNumber"), 2);
        glUniform1i(glGetUniformLocation(program, "k"), k);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer3);
        glViewport(0, 0, n, m);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureA);
        glUniform1i(glGetUniformLocation(program, "samplerA"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureR);
        glUniform1i(glGetUniformLocation(program, "samplerR"), 1);


        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textureResultQ);
        glUniform1i(glGetUniformLocation(program, "samplerQ"), 2);

        textureResultR = createDataTexture(R, n, n, 3);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, textureResultR);
        glUniform1i(glGetUniformLocation(program, "samplerResultR"), 3);


        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        float result_a[m * n];
        glReadPixels(0, 0, n, m, GL_RED, GL_FLOAT, result_a);

        for (int j = k+1; j < n; ++j) {
            for(int i=0; i<m; i++){

                A[i * n + j] = result_a[i * n + j];

            }
        }

        glDeleteTextures(1, &textureA);
        glDeleteTextures(1, &textureR);
        glDeleteTextures(1, &textureResultQ);
        glDeleteTextures(1, &textureResultR);
        glDeleteFramebuffers(1, &framebuffer1);
        glDeleteFramebuffers(1, &framebuffer2);
        glDeleteFramebuffers(1, &framebuffer3);

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

    glUseProgram(program);

    emscripten_set_main_loop(render, 0, 1);

    return 0;
}
