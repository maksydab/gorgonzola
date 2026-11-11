
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

int painting = 0;
double mx = 0, my = 0;

void mouse_button(GLFWwindow* w, int b, int a, int m) {
    if (b == GLFW_MOUSE_BUTTON_LEFT) painting = (a == GLFW_PRESS);
}

void cursor(GLFWwindow* w, double x, double y) {
    mx = x;
    my = y;
}

char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char* s = malloc(n + 1);
    if (!s) { fclose(f); return NULL; }
    fread(s, 1, n, f);
    s[n] = 0;
    fclose(f);
    return s;
}

unsigned int compile(GLenum type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "%s\n", log);
    }
    return s;
}

unsigned int make_program(const char* vs_src, const char* fs_src) {
    unsigned int vs = compile(GL_VERTEX_SHADER, vs_src);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fs_src);
    unsigned int p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    int ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, sizeof(log), NULL, log);
        fprintf(stderr, "%s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

int main(void) {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(800, 600, "Sand", NULL, NULL);
    if (!w) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(w);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return 1;
    glfwSetMouseButtonCallback(w, mouse_button);
    glfwSetCursorPosCallback(w, cursor);

    const char* vs =
        "#version 330 core\n"
        "void main(){"
        "vec2 p=vec2(gl_VertexID==1?3.0:-1.0,gl_VertexID==2?3.0:-1.0);"
        "gl_Position=vec4(p,0,1);"
        "}";

    char* fs = read_file("frag.glsl");
    if (!fs) { fprintf(stderr, "Cannot read frag.glsl\n"); return 1; }

    unsigned int sim = make_program(vs, fs);
    free(fs);

    const char* display =
        "#version 330 core\n"
        "out vec4 FragColor;"
        "uniform sampler2D u_state;"
        "void main(){"
        "float s=texelFetch(u_state,ivec2(gl_FragCoord.xy),0).r;"
        "vec3 c=mix(vec3(0.025,0.035,0.05),vec3(0.95,0.72,0.36),s);"
        "FragColor=vec4(c,1);"
        "}";

    unsigned int show = make_program(vs, display);
    unsigned int tex[2], fbo[2], vao;
    glGenTextures(2, tex);
    glGenFramebuffers(2, fbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    unsigned char* empty = calloc(800 * 600, 1);
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, tex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 800, 600, 0, GL_RED, GL_UNSIGNED_BYTE, empty);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "Framebuffer error\n");
            return 1;
        }
    }
    free(empty);

    int current = 0, frame = 0;
    while (!glfwWindowShouldClose(w)) {
        glfwPollEvents();
        if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, 1);

        int next = 1 - current;
        int width, height;
        glfwGetWindowSize(w, &width, &height);
        int x = (int)(mx * 800.0 / width);
        int y = 599 - (int)(my * 600.0 / height);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo[next]);
        glViewport(0, 0, 800, 600);
        glUseProgram(sim);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex[current]);
        glUniform1i(glGetUniformLocation(sim, "u_state"), 0);
        glUniform2i(glGetUniformLocation(sim, "u_size"), 800, 600);
        glUniform2i(glGetUniformLocation(sim, "u_mouse"), x, y);
        glUniform1i(glGetUniformLocation(sim, "u_painting"), painting);
        glUniform1i(glGetUniformLocation(sim, "u_frame"), frame++);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        current = next;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, 800, 600);
        glUseProgram(show);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex[current]);
        glUniform1i(glGetUniformLocation(show, "u_state"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(w);
    }

    glDeleteTextures(2, tex);
    glDeleteFramebuffers(2, fbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(sim);
    glDeleteProgram(show);
    glfwDestroyWindow(w);
    glfwTerminate();
    return 0;
}
