#include <iostream>
#include <cmath>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>

#define W 7680
#define H 4320

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
"\n"
"uniform sampler2D u_y1;\n"
"uniform sampler2D u_y2;\n"
"uniform float u_c;  // Coefficient c1 or c2\n"
"\n"
"in vec2 uv;\n"
"out vec4 glColor;\n"
"\n"
"void main() {\n"
"    float y1_val = texture(u_y1, uv).r;\n"
"    float y2_val = texture(u_y2, uv).r;\n"
"    float result = u_c * (y1_val + y2_val);\n"
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

GLuint textureY1, textureY2;
GLuint program, framebuffer1, framebuffer2;

DATA_TYPE alpha;
DATA_TYPE imgIn[W * H];
DATA_TYPE imgOut[W * H];
DATA_TYPE y1_data[W * H];
DATA_TYPE y2_data[W * H];

std::chrono::high_resolution_clock::time_point start_time;


void render() {
   
    glUseProgram(program);
    
    int w = W;
    int h = H;

    alpha = 0.25;  


    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            imgIn[i * H + j] = static_cast<DATA_TYPE>((313 * i + 991 * j) % 65536) / 65535.0f;
        }
    }

    DATA_TYPE xm1, tm1, ym1, ym2;
    DATA_TYPE xp1, xp2;
    DATA_TYPE tp1, tp2;
    DATA_TYPE yp1, yp2;

    DATA_TYPE k, a1, a2, a3, a4, a5, a6, a7, a8, b1, b2, c1, c2;

    k = (1.0 - exp(-alpha)) * (1.0 - exp(-alpha)) / (1.0 + 2.0 * alpha * exp(-alpha) - exp(2.0 * alpha));
    a1 = a5 = k;
    a2 = a6 = k * exp(-alpha) * (alpha - 1.0);
    a3 = a7 = k * exp(-alpha) * (alpha + 1.0);
    a4 = a8 = -k * exp(-2.0 * alpha);
    b1 = pow(2.0, -alpha);
    b2 = -exp(-2.0 * alpha);
    c1 = c2 = 1.0;

    for (int i = 0; i < w; i++) {
        ym1 = ym2 = xm1 = 0.0;
        for (int j = 0; j < h; j++) {
            int index = i * H + j;  
            y1_data[index] = a1 * imgIn[index] + a2 * xm1 + b1 * ym1 + b2 * ym2;
            xm1 = imgIn[index];
            ym2 = ym1;
            ym1 = y1_data[index];
        }
    }


    for (int i = 0; i < w; i++) {
        yp1 = yp2 = xp1 = xp2 = 0.0;
        for (int j = h - 1; j >= 0; j--) {
            int index = i * H + j; 
            y2_data[index] = a3 * xp1 + a4 * xp2 + b1 * yp1 + b2 * yp2;
            xp2 = xp1;
            xp1 = imgIn[index];
            yp2 = yp1;
            yp1 = y2_data[index];
        }
    }

    
    textureY1 = createDataTexture(y1_data, h, w, 0);    
    textureY2 = createDataTexture(y2_data, h, w, 1);  

    framebuffer1 = createFramebuffer(h, w);

    framebuffer2 = createFramebuffer(h, w);


    glUniform1f(glGetUniformLocation(program, "u_c"), 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1); 

    glViewport(0, 0, h, w);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY1);  
    glUniform1i(glGetUniformLocation(program, "u_y1"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureY2); 
    glUniform1i(glGetUniformLocation(program, "u_y2"), 1);

    glClear(GL_COLOR_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glReadPixels(0, 0, h, w, GL_RED, GL_FLOAT, imgOut);


    for (int j = 0; j < h; j++) {
        tm1 = ym1 = ym2 = 0.0;
        for (int i = 0; i < w; i++) {
            int index = i * H + j;  
            y1_data[index] = a5 * imgOut[index] + a6 * tm1 + b1 * ym1 + b2 * ym2;
            tm1 = imgOut[index];
            ym2 = ym1;
            ym1 = y1_data[index];
        }
    }

    for (int j = 0; j < h; j++) {
        tp1 = tp2 = yp1 = yp2 = 0.0;
        for (int i = w - 1; i >= 0; i--) {
            int index = i * H + j; 
            y2_data[index] = a7 * tp1 + a8 * tp2 + b1 * yp1 + b2 * yp2;
            tp2 = tp1;
            tp1 = imgOut[index];
            yp2 = yp1;
            yp1 = y2_data[index];
        }
    }

    textureY1 = createDataTexture(y1_data, h, w, 0);    
    textureY2 = createDataTexture(y2_data, h, w, 1);

    glUniform1f(glGetUniformLocation(program, "u_c"), 1.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY1); 
    glUniform1i(glGetUniformLocation(program, "u_y1"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureY2); 
    glUniform1i(glGetUniformLocation(program, "u_y2"), 1);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2); 
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glReadPixels(0, 0, h, w, GL_RED, GL_FLOAT, imgOut);


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
