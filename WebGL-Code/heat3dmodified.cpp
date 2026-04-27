#include <iostream>
#include <vector>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>

#define TSTEPS 100  
#define N 40    

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
"uniform sampler2D samplerA;\n"
"uniform sampler2D samplerB;\n"
"\n"
"uniform int passNumber;\n"
"uniform int n;\n"
"// uniform int i;\n"
"\n"
"in vec2 uv;\n"
"out vec4 glColor;\n"
"\n"
"void main() {\n"
"    int i = int(uv.y * float(n));\n"
"    float ycoord = uv.x * float(n) * float(n);\n"
"    int j = int(ycoord / float(n));\n"
"    int k = int(mod(ycoord, float(n)));\n"
"\n"
"    float result = 0.0;\n"
"\n"
"    if (passNumber == 0) {\n"
"        if (i >= 1 && i < n - 1 && j >= 1 && j < n - 1 && k >= 1 && k < n - 1) {\n"
"            vec2 centerUV = vec2(float(j * n + k) / float(n * n), float(i) / float(n));\n"
"\n"
"            // Sample A at the center position\n"
"            float A_center = texture(samplerA, centerUV).r;\n"
"\n"
"            // Neighbors in the i-direction\n"
"            float A_ip1 = texture(samplerA, vec2(uv.x, float(i + 1) / float(n))).r;\n"
"            float A_im1 = texture(samplerA, vec2(uv.x, float(i - 1) / float(n))).r;\n"
"\n"
"            // Neighbors in the j-direction\n"
"            float A_jp1 = texture(samplerA, vec2(float((j + 1) * n + k) / float(n * n), uv.y)).r;\n"
"            float A_jm1 = texture(samplerA, vec2(float((j - 1) * n + k) / float(n * n), uv.y)).r;\n"
"\n"
"            // Neighbors in the k-direction\n"
"            float A_kp1 = texture(samplerA, vec2(float(j * n + (k + 1)) / float(n * n), uv.y)).r;\n"
"            float A_km1 = texture(samplerA, vec2(float(j * n + (k - 1)) / float(n * n), uv.y)).r;\n"
"\n"
"            // Apply the formula for B[i][j][k]\n"
"            result = 0.125 * (A_ip1 - 2.0 * A_center + A_im1)\n"
"                   + 0.125 * (A_jp1 - 2.0 * A_center + A_jm1)\n"
"                   + 0.125 * (A_kp1 - 2.0 * A_center + A_km1)\n"
"                   + A_center;\n"
"        } else {\n"
"            // Handle boundary conditions\n"
"            vec2 centerUV = vec2(float(j * n + k) / float(n * n), float(i) / float(n));\n"
"            result = texture(samplerB, centerUV).r;\n"
"        }\n"
"    }\n"
"\n"
"    if (passNumber == 1) {\n"
"        if (i >= 1 && i < n - 1 && j >= 1 && j < n - 1 && k >= 1 && k < n - 1) {\n"
"            vec2 centerUV = vec2(float(j * n + k) / float(n * n), float(i) / float(n));\n"
"\n"
"            // Sample B at the center position\n"
"            float B_center = texture(samplerB, centerUV).r;\n"
"\n"
"            // Neighbors in the i-direction\n"
"            float B_ip1 = texture(samplerB, vec2(uv.x, float(i + 1) / float(n))).r;\n"
"            float B_im1 = texture(samplerB, vec2(uv.x, float(i - 1) / float(n))).r;\n"
"\n"
"            // Neighbors in the j-direction\n"
"            float B_jp1 = texture(samplerB, vec2(float((j + 1) * n + k) / float(n * n), uv.y)).r;\n"
"            float B_jm1 = texture(samplerB, vec2(float((j - 1) * n + k) / float(n * n), uv.y)).r;\n"
"\n"
"            // Neighbors in the k-direction\n"
"            float B_kp1 = texture(samplerB, vec2(float(j * n + (k + 1)) / float(n * n), uv.y)).r;\n"
"            float B_km1 = texture(samplerB, vec2(float(j * n + (k - 1)) / float(n * n), uv.y)).r;\n"
"\n"
"            // Apply the formula for B[i][j][k]\n"
"            result = 0.125 * (B_ip1 - 2.0 * B_center + B_im1)\n"
"                   + 0.125 * (B_jp1 - 2.0 * B_center + B_jm1)\n"
"                   + 0.125 * (B_kp1 - 2.0 * B_center + B_km1)\n"
"                   + B_center;\n"
"        } else {\n"
"            // Handle boundary conditions\n"
"            vec2 centerUV = vec2(float(j * n + k) / float(n * n), float(i) / float(n));\n"
"            result = texture(samplerA, centerUV).r;\n"
"        }\n"
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
GLuint program;
GLuint framebuffer1, framebuffer2;
GLuint textureA, textureB;

std::chrono::high_resolution_clock::time_point start_time;

void render() {
    glUseProgram(program);

    int n = N;
    int tsteps = TSTEPS;

    DATA_TYPE A[N * N * N];
    DATA_TYPE B[N * N * N];


    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                int index = i * N * N + j * N + k; 
                A[index] = B[index] = (DATA_TYPE)(i + j + (n - k)) * 10 / n;
            }
        }
    }

    glUniform1i(glGetUniformLocation(program, "n"), n);

    for (int t = 1; t <= tsteps; t++) {
            
            textureA = createDataTexture(A, n*n, n, 0);
            textureB = createDataTexture(B, n*n, n, 1);

            framebuffer1 = createFramebuffer(n*n, n);


            glUniform1i(glGetUniformLocation(program, "passNumber"), 0);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1);
            glViewport(0, 0, n*n, n);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureA);
            glUniform1i(glGetUniformLocation(program, "samplerA"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textureB);
            glUniform1i(glGetUniformLocation(program, "samplerB"), 1);


            glClear(GL_COLOR_BUFFER_BIT);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glReadPixels(0, 0, n*n, n, GL_RED, GL_FLOAT, B);
            


            textureA = createDataTexture(A, n*n, n, 0);
            textureB = createDataTexture(B, n*n, n, 1);

            framebuffer2 = createFramebuffer(n*n, n);


            glUniform1i(glGetUniformLocation(program, "passNumber"), 1);
          
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2);
            glViewport(0, 0, n*n, n);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureA);
            glUniform1i(glGetUniformLocation(program, "samplerA"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textureB);
            glUniform1i(glGetUniformLocation(program, "samplerB"), 1);


            glClear(GL_COLOR_BUFFER_BIT);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


            glReadPixels(0, 0, n*n, n, GL_RED, GL_FLOAT, A);


            glDeleteTextures(1, &textureA);
            glDeleteTextures(1, &textureB);
            glDeleteFramebuffers(1, &framebuffer1);
            glDeleteFramebuffers(1, &framebuffer2);


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
