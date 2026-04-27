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
    "uniform sampler2D samplerP;                  \n"
    "uniform sampler2D samplerR;                  \n"
    "uniform int computeS;                        \n"
    "in vec2 uv;                                  \n"
    "out vec4 glColor;                            \n"
    "void main() {                                \n"
    "    int rowOrCol = 0;    \n"
    "    float result = 0.0;                      \n"
    "    int size = 0;   \n"
    "  if(computeS == 1){ \n"
    "     //size=  textureSize(samplerA, 0).y;                                        \n"
    "     size = 2200;                                        \n"
    "    rowOrCol = int(uv.x * float(textureSize(samplerA, 0).x));    \n"
    "     for (int k = 0; k < size; ++k) {         \n"
    "        vec2 uvA = vec2(float(rowOrCol) / float(1800), float(k) / float(size)); \n"
    "        float a = texture(samplerA, uvA).r;  \n"
    "            float r = texture(samplerR, vec2(float(k) / float(size), 1.0)).r; \n"
    "            result += r * a;                 \n"
    "        } \n"
    "    }                                        \n"
    "  else{                                                   \n"
    "      //size= textureSize(samplerA, 0).x;                                               \n"
    "      size = 1800;                                               \n"
    "    rowOrCol = int(uv.x * float(textureSize(samplerA, 0).y));    \n"
    "      for (int l = 0; l < size; ++l) {                                          \n"
    "           vec2 uvA = vec2(float(l) / float(size), float(rowOrCol) / float(2200));                  \n"
    "           float a = texture(samplerA, uvA).r;  \n"
    "           float p = texture(samplerP, vec2(float(l) / float(size), 1.0)).r; \n"
     "          result += p * a;                 \n"
    "        }                                     \n"
    "    }\n"
    "    glColor = vec4(result, 0.0, 0.0, 1.0);   \n"
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
GLuint program, framebuffer_s, framebuffer_q;
GLuint textureA, textureP, textureR;
int m, n;
std::chrono::high_resolution_clock::time_point start_time;

void render() {
    glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "computeS"), 1);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_s);
    glViewport(0, 0, m, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glUniform1i(glGetUniformLocation(program, "samplerA"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureR);
    glUniform1i(glGetUniformLocation(program, "samplerR"), 1);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    float result_s[m];

    glReadPixels(0, 0, n, 1, GL_RED, GL_FLOAT, result_s);


    glUniform1i(glGetUniformLocation(program, "computeS"), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_q);
    glViewport(0, 0, n, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glUniform1i(glGetUniformLocation(program, "samplerA"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureP);
    glUniform1i(glGetUniformLocation(program, "samplerP"), 1);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    float result_q[n];

    glReadPixels(0, 0, n, 1, GL_RED, GL_FLOAT, result_q);


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

    m = 1800;
    n = 2200;

    float A[n*m], p[m], r[n];

    for (int i = 0; i < m; i++) {
        p[i] = static_cast<float>(i % m) / m;
    }
    for (int i = 0; i < n; i++) {
        r[i] = static_cast<float>(i % n) / n;
        for (int j = 0; j < m; j++) {
            A[i * m + j] = static_cast<float>(i * (j + 1) % n) / n;
        }
    }


    textureA = createDataTexture(A, m, n, 0);
    textureP = createDataTexture(p, m, 1, 1);
    textureR = createDataTexture(r, n, 1, 2);

    framebuffer_s = createFramebuffer(m, 1);
    framebuffer_q = createFramebuffer(n, 1);

    glUseProgram(program);

    emscripten_set_main_loop(render, 0, 1);

    return 0;
}
