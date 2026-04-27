#include <iostream>
#include <vector>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <chrono>

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
"uniform sampler2D samplerY;\n"
"uniform sampler2D samplerZ;\n"
"uniform int passNumber;\n"
"uniform int k;\n"
"uniform int n;\n"
"uniform float alpha;\n" 
"in vec2 uv;\n"
"out vec4 glColor;\n"
"\n"
"void main() {\n"
"\n"
"    float result=0.0;\n"
"\n"
"    if(passNumber==0){\n"
"\n"
"        int i = int(uv.x * float(n));\n"
"\n"
"        vec2 uvZ = vec2(float(i) / float(n), 1.0);\n"
"\n"
"        float zi = 0.0;\n"
"\n"
"        if(i<k){\n"
"\n"
"            vec2 uvY = vec2(float(i) / float(n), 1.0);\n"
"            vec2 uvYK1 = vec2(float(k-i-1) / float(n), 1.0);\n"
"\n"
"            float yi = texture(samplerY, uvY).r;\n"
"            float yk1 = texture(samplerY, uvYK1).r;\n"
"\n"
"            zi = yi + alpha * yk1;\n"
"        }\n"
"\n"
"        result = zi;\n"
"    }\n"
"\n"
"    if(passNumber==1){\n"
"\n"
"        int i = int(uv.x * float(n));\n"
"\n"
"        vec2 uvY = vec2(float(i) / float(n), 1.0);\n"
"\n"
"        float yi = 0.0;\n"
"\n"
"        if(i<k){\n"
"\n"
"            vec2 uvZ = vec2(float(i) / float(n), 1.0);\n"
"\n"
"            float zi = texture(samplerZ, uvZ).r;\n"
"\n"
"            yi = zi;\n"
"        }\n"
"\n"
"        result = yi;\n"
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
constexpr int N = 4000; 
GLuint textureY, textureZ;
GLuint program, framebuffer1, framebuffer2;

DATA_TYPE r[N];

std::chrono::high_resolution_clock::time_point start_time;

void render() {
    
    glUseProgram(program);

    DATA_TYPE y[N] = {0.0f};
    DATA_TYPE z[N]= {0.0f};
    DATA_TYPE alpha;
    DATA_TYPE beta;
    DATA_TYPE sum;

    y[0] = -r[0];
    beta = 1.0f;
    alpha = -r[0];

    glUniform1i(glGetUniformLocation(program, "n"), N);

    for (int k = 1; k < N; ++k) {
        beta = (1.0f - alpha * alpha) * beta;
        sum = 0.0f;
        for (int i = 0; i < k; ++i) {
            sum += r[k - i - 1] * y[i];
        }
        alpha = -(r[k] + sum) / beta;


        textureY = createDataTexture(y, N, 1, 0);  

        glUniform1i(glGetUniformLocation(program, "passNumber"), 0);
        glUniform1f(glGetUniformLocation(program, "alpha"), alpha);
        glUniform1i(glGetUniformLocation(program, "k"), k);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1);
        glViewport(0, 0, N, 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureY);
        glUniform1i(glGetUniformLocation(program, "samplerY"), 0);

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        float result_z[N];
        glReadPixels(0, 0, N, 1, GL_RED, GL_FLOAT, result_z);


        for (int i = 0; i < k; ++i) {
            z[i] = result_z[i];
        }


        glUniform1i(glGetUniformLocation(program, "passNumber"), 1);
        glUniform1i(glGetUniformLocation(program, "k"), k);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2);
        glViewport(0, 0, N, 1);

        textureZ = createDataTexture(z, N, 1, 1);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureZ);
        glUniform1i(glGetUniformLocation(program, "samplerZ"), 1);

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        float result_y[N];
        glReadPixels(0, 0, N, 1, GL_RED, GL_FLOAT, result_y);

        for (int i = 0; i < k; ++i) {
            y[i] = result_y[i];
        }

        y[k] = alpha;

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

    for (int i = 0; i < N; i++) {
        r[i] = static_cast<DATA_TYPE>(N + 1 - i);
    }

    framebuffer1 = createFramebuffer(N, 1);

    framebuffer2 = createFramebuffer(N, 1);

    glUseProgram(program);

    emscripten_set_main_loop(render, 0, 1);

    return 0;
}


