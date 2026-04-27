#include <iostream>
#include <vector>
#include <iomanip>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

constexpr int TMAX = 1000;
constexpr int NX = 2000;
constexpr int NY = 2600;
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
"uniform sampler2D samplerHz;\n"
"uniform sampler2D samplerEy;\n"
"uniform sampler2D samplerEx;\n"
"uniform int passNumber;\n"
"uniform int nx;\n"
"uniform int ny;\n"
"in vec2 uv;\n"
"out vec4 glColor;\n"
"void main() {\n"
"    int i = int(uv.y * float(nx));\n"
"    int j = int(uv.x * float(ny));\n"
"    float result = 0.0;\n"
"    if (passNumber == 0) {\n"
"        vec2 uvEy = vec2(float(j) / float(ny), float(i) / float(nx));\n"
"        float EyIJ = texture(samplerEy, uvEy).r;\n"
"        if (i >= 1 && j>=0) {\n"
"            vec2 uvHz = vec2(float(j) / float(ny), float(i) / float(nx));\n"
"            vec2 uvHzi1 = vec2(float(j) / float(ny), uv.y - 1.0/ float(nx));\n"
"            float HzIJ = texture(samplerHz, uvHz).r;\n"
"            float HzI1 = texture(samplerHz, uvHzi1).r;\n"
"            EyIJ -= 0.5 * (HzIJ - HzI1);\n"
"        }\n"
"        result = EyIJ;\n"
"    }\n"
"    if (passNumber == 1) {\n"
"        vec2 uvEx = vec2(float(j) / float(ny), float(i) / float(nx));\n"
"        float ExIJ = texture(samplerEx, uvEx).r;\n"
"        if (j >= 1 && i>=0) {\n"
"            vec2 uvHz = vec2(float(j) / float(ny), float(i) / float(nx));\n"
"            vec2 uvHzj1 = vec2(uv.x - 1.0/ float(ny), float(i) / float(nx));\n"
"            float HzIJ = texture(samplerHz, uvHz).r;\n"
"            float HzJ1 = texture(samplerHz, uvHzj1).r;\n"
"            ExIJ -= 0.5 * (HzIJ - HzJ1);\n"
"        }\n"
"        result = ExIJ;\n"
"    }\n"
"    if (passNumber == 2) {\n"
"        vec2 uvHz = vec2(float(j) / float(ny), float(i) / float(nx));\n"
"        float HzIJ = texture(samplerHz, uvHz).r;\n"
"        if (i < nx - 1 && j < ny - 1 && i>=0 && j>=0) {\n"
"            vec2 uvEx = vec2(float(j) / float(ny), float(i) / float(nx));\n"
"            vec2 uvExj1 = vec2(uv.x + 1.0/ float(ny), float(i) / float(nx));\n"
"            vec2 uvEy = vec2(float(j) / float(ny), float(i) / float(nx));\n"
"            vec2 uvEyi1 = vec2(float(j) / float(ny), uv.y + 1.0/ float(nx));\n"
"            float ExIJ = texture(samplerEx, uvEx).r;\n"
"            float ExJ1 = texture(samplerEx, uvExj1).r;\n"
"            float EyIJ = texture(samplerEy, uvEy).r;\n"
"            float EyI1 = texture(samplerEy, uvEyi1).r;\n"
"            HzIJ -= 0.7 * (ExJ1 - ExIJ + EyI1 - EyIJ);\n"
"        }\n"
"        result = HzIJ;\n"
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
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind the texture
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
GLuint program, framebuffer1, framebuffer2, framebuffer3, textureHz, textureEy, textureEx, textureEx2,  textureEy2, textureFict;

std::chrono::high_resolution_clock::time_point start_time;

void render() {


    glUseProgram(program);

    int tmax = TMAX;
    int nx = NX;
    int ny = NY;

    DATA_TYPE ex[nx * ny];
    DATA_TYPE ey[nx * ny];
    DATA_TYPE hz[nx * ny];
    DATA_TYPE _fict_[tmax];

    for (int i = 0; i < tmax; i++) {
        _fict_[i] = static_cast<DATA_TYPE>(i);
    }

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int index = i * NY + j;  
            ex[index] = static_cast<DATA_TYPE>(i * (j + 1)) / nx;
            ey[index] = static_cast<DATA_TYPE>(i * (j + 2)) / ny;
            hz[index] = static_cast<DATA_TYPE>(i * (j + 3)) / nx;
        }
    }

    glUniform1i(glGetUniformLocation(program, "nx"), nx);

    glUniform1i(glGetUniformLocation(program, "ny"), ny);


    for (int t = 0; t < tmax; t++) {

        for (int j = 0; j < ny; j++) {
            ey[j] = _fict_[t];
        }
  
        textureHz = createDataTexture(hz, NY, NX, 0);
        textureEy = createDataTexture(ey, NY, NX, 1);

        framebuffer1 = createFramebuffer(NY, NX);
        framebuffer2 = createFramebuffer(NY, NX);
        framebuffer3 = createFramebuffer(NY, NX);


        glUniform1i(glGetUniformLocation(program, "passNumber"), 0);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1);
        glViewport(0, 0, NY, NX);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureHz);
        glUniform1i(glGetUniformLocation(program, "samplerHz"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureEy);
        glUniform1i(glGetUniformLocation(program, "samplerEy"), 1);


        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glReadPixels(0, 0, NY, NX, GL_RED, GL_FLOAT, ey);



        textureEx = createDataTexture(ex, NY, NX, 2);


        glUniform1i(glGetUniformLocation(program, "passNumber"), 1);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2);
        glViewport(0, 0, NY, NX);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureHz);
        glUniform1i(glGetUniformLocation(program, "samplerHz"), 0);


        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureEy);
        glUniform1i(glGetUniformLocation(program, "samplerEy"), 1);


        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textureEx);
        glUniform1i(glGetUniformLocation(program, "samplerEx"), 2);


        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glReadPixels(0, 0, NY, NX, GL_RED, GL_FLOAT, ex);

  
        textureEy = createDataTexture(ey, NY, NX, 1);
        textureEx = createDataTexture(ex, NY, NX, 2);  

        glUniform1i(glGetUniformLocation(program, "passNumber"), 2);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer3);
        glViewport(0, 0, NY, NX);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureHz);
        glUniform1i(glGetUniformLocation(program, "samplerHz"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureEy);
        glUniform1i(glGetUniformLocation(program, "samplerEy"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textureEx);
        glUniform1i(glGetUniformLocation(program, "samplerEx"), 2);


        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glReadPixels(0, 0, NY, NX, GL_RED, GL_FLOAT, hz);


        glDeleteTextures(1, &textureHz);
        glDeleteTextures(1, &textureEx);
        glDeleteTextures(1, &textureEy);
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
