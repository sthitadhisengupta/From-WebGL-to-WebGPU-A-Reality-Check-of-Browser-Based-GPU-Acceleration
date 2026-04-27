#include <iostream>
#include <vector>
#include <iomanip>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

constexpr int NQ = 40;
constexpr int NR = 50;
constexpr int NP = 60;
using DATA_TYPE = float;

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
    "uniform sampler2D samplerC4;\n"
    "uniform sampler2D samplerA;\n"
    "uniform sampler2D samplerSum;\n"
    "uniform int np;\n"
    "uniform int nq;\n"
    "uniform int nr;\n"
    "uniform int passNumber;\n"
    "uniform int r;\n"
    "uniform int q;\n"
    "in vec2 uv;\n"
    "out vec4 glColor;\n"
    "void main() {\n"
    "    int p = int(uv.x * float(np));\n"
    "    float result = 0.0;\n"
    "    if (passNumber == 0) {\n"
    "       float sum = 0.0;\n"
    "       float A_y = float(r) / float(nr);\n"
    "       for (int s = 0; s < np; s++) {\n"
    "           float A_x = float(q * np + s) / float(nq * np);\n"
    "           float C4_x = float(p) / float(np);\n"
    "           float C4_y = float(s) / float(np);\n"
    "           float A_rqs = texture(samplerA, vec2(A_x, A_y)).r;\n"
    "           float C4_sp = texture(samplerC4, vec2(C4_x, C4_y)).r;\n"
    "           sum += A_rqs * C4_sp;\n"
    "       }\n"
    "       result = sum; \n"
    "    }\n"
    "    if (passNumber == 1) {\n"
    "       result = texture(samplerSum, vec2(float(p) / float(np), 1.0)).r;    \n"
    "    }\n"
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
GLuint program, framebuffer1, framebuffer2;
GLuint A_tex, C4_tex, Sum_tex;

std::chrono::high_resolution_clock::time_point start_time;

void render(){

    glUseProgram(program);

    DATA_TYPE A[NR * NQ * NP], C4[NP * NP];
    
    DATA_TYPE sum[NP];

    for (int i = 0; i < NR; i++)
        for (int j = 0; j < NQ; j++)
            for (int k = 0; k < NP; k++)
                A[i * NQ * NP + j * NP + k] = static_cast<DATA_TYPE>((i * j + k) % NP) / NP;

    for (int i = 0; i < NP; i++)
        for (int j = 0; j < NP; j++)
            C4[i * NP + j] = static_cast<DATA_TYPE>((i * j) % NP) / NP;


    glUniform1i(glGetUniformLocation(program, "nr"), NR);
    glUniform1i(glGetUniformLocation(program, "nq"), NQ);
    glUniform1i(glGetUniformLocation(program, "np"), NP);

    for (int r = 0; r < NR; r++) {
        for (int q = 0; q < NQ; q++) {


            C4_tex = createDataTexture(C4, NP, NP, 0);   

            A_tex = createDataTexture(A, NQ * NP, NR, 1); 

            framebuffer1 = createFramebuffer(NP, 1);

            framebuffer2 = createFramebuffer(NP, 1);

            glUniform1i(glGetUniformLocation(program, "r"), r);
            glUniform1i(glGetUniformLocation(program, "q"), q);

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1);
            glViewport(0, 0, NP, 1);

            glUniform1i(glGetUniformLocation(program, "passNumber"), 0);


            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, C4_tex);
            glUniform1i(glGetUniformLocation(program, "samplerC4"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, A_tex);
            glUniform1i(glGetUniformLocation(program, "samplerA"), 1);

            glClear(GL_COLOR_BUFFER_BIT);
           
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glReadPixels(0, 0, NP, 1, GL_RED, GL_FLOAT, sum);



            Sum_tex = createDataTexture(sum, NP, 1, 2);

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2);
            glViewport(0, 0, NP, 1);

            glUniform1i(glGetUniformLocation(program, "passNumber"), 1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, C4_tex);
            glUniform1i(glGetUniformLocation(program, "samplerC4"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, A_tex);
            glUniform1i(glGetUniformLocation(program, "samplerA"), 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, Sum_tex);
            glUniform1i(glGetUniformLocation(program, "samplerSum"), 2);

            glClear(GL_COLOR_BUFFER_BIT);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            DATA_TYPE A_P[NP];

            glReadPixels(0, 0, NP, 1, GL_RED, GL_FLOAT, A_P);

            for(int p=0; p<NP; p++)
            {
                A[r * NQ * NP + q * NP + p] = A_P[p];
            }

            glDeleteTextures(1, &C4_tex);
            glDeleteTextures(1, &A_tex);
            glDeleteTextures(1, &Sum_tex);
            glDeleteFramebuffers(1, &framebuffer1);
            glDeleteFramebuffers(1, &framebuffer2);

        }
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

