
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600

int painting = 0;
double mx = 0, my = 0;

void mouse_button(GLFWwindow* w, int b, int a, int m)
{
    if (b == GLFW_MOUSE_BUTTON_LEFT)
        painting = (a == GLFW_PRESS);
}

void cursor(GLFWwindow* w, double x, double y)
{
    mx = x;
    my = y;
}

char* read_file(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);

    char* s = malloc(n + 1);
    if (!s) {
        fclose(f);
        return NULL;
    }

    fread(s, 1, n, f);
    s[n] = 0;

    fclose(f);
    return s;
}

unsigned int make_compute_program(const char* src)
{
    unsigned int shader = glCreateShader(GL_COMPUTE_SHADER);

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "compute shader error:\n%s\n", log);
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &ok);

    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fprintf(stderr, "Compute program error:\n%s\n", log);
    }

    glDeleteShader(shader);
    return program;
}

unsigned int make_program(const char* vs_src, const char* fs_src)
{
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, NULL);
    glCompileShader(vs);

    int ok;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);

    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(vs, sizeof(log), NULL, log);
        fprintf(stderr, "Vertex shader error:\n%s\n", log);
    }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);

    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(fs, sizeof(log), NULL, log);
        fprintf(stderr, "FRagment shader error:\n%s\n", log);
    }

    unsigned int p = glCreateProgram();

    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    glGetProgramiv(p, GL_LINK_STATUS, &ok);

    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, sizeof(log), NULL, log);
        fprintf(stderr, "Ddisplay program error:\n%s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return p;
}

int main(void)
{
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* w = glfwCreateWindow(
        WIDTH, HEIGHT, "sand", NULL, NULL
    );

    if (!w) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(w);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "failed to initialize GLAD\n");
        return 1;
    }

    glfwSetMouseButtonCallback(w, mouse_button);
    glfwSetCursorPosCallback(w, cursor);

    printf("OpenGL: %s\n", glGetString(GL_VERSION));

    const char* vs =
        "#version 330 core\n"
        "void main(){"
        "vec2 p=vec2("
        "gl_VertexID==1?3.0:-1.0,"
        "gl_VertexID==2?3.0:-1.0"
        ");"
        "gl_Position=vec4(p,0,1);"
        "}";

    const char* display =
        "#version 330 core\n"
        "out vec4 FragColor;"
        "uniform sampler2D u_state;"
        "uniform sampler2D u_temperature;"
        "void main(){"
        "ivec2 p=ivec2(gl_FragCoord.xy);"
        "vec4 s=texelFetch(u_state,p,0);"
        "float temp=texelFetch(u_temperature,p,0).r;"
        "float heat=clamp((temp-20.0)/500.0,0.0,1.0);"
        "vec3 heatColor=mix("
            "vec3(1.0,0.12,0.0),"
            "vec3(1.0,0.95,0.75),"
            "smoothstep(100.0,800.0,temp)"
        ");"
        "if(s.a<0.5 || s.r+s.g+s.b<0.02){"

            "float emptyHeat=heat*0.12;"

            "s.rgb=mix("
                "vec3(0.0),"
                "heatColor,"
                "emptyHeat"
            ");"

        "} else {"
            "s.rgb=mix("
                "s.rgb,"
                "heatColor,"
                "heat*0.8"
            ");"

        "}"

        "FragColor=s;"
        "}";

    char* cs = read_file("sim.comp");

    if (!cs) {
        fprintf(stderr, "Cannot read sim.comp\n");
        return 1;
    }

    unsigned int sim = make_compute_program(cs);
    free(cs);

    unsigned int show = make_program(vs, display);

    unsigned int tex[2];

    unsigned int temperatureTex[2];

    unsigned int vao;

    glGenTextures(2, tex);
    glGenTextures(2, temperatureTex);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    float* empty = calloc(
        WIDTH * HEIGHT * 4,
        sizeof(float)
    );

    if (!empty) {
        fprintf(stderr, "failed to allocate material data\n");
        return 1;
    }

    for (int i = 0; i < 2; i++) {

        glBindTexture(GL_TEXTURE_2D, tex[i]);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA16F,
            WIDTH,
            HEIGHT,
            0,
            GL_RGBA,
            GL_FLOAT,
            empty
        );

        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    free(empty);

    float* initialTemperature = malloc(
        WIDTH * HEIGHT * sizeof(float)
    );

    if (!initialTemperature) {
        fprintf(stderr, "failed to allocate temperature data\n");
        return 1;
    }
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        initialTemperature[i] = 20.0f;
    }

    for (int i = 0; i < 2; i++) {

        glBindTexture(GL_TEXTURE_2D, temperatureTex[i]);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R16F,
            WIDTH,
            HEIGHT,
            0,
            GL_RED,
            GL_FLOAT,
            initialTemperature
        );

        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    free(initialTemperature);

    glUseProgram(sim);

    GLint sizeLoc =
        glGetUniformLocation(sim, "u_size");

    GLint mouseLoc =
        glGetUniformLocation(sim, "u_mouse");

    GLint paintingLoc =
        glGetUniformLocation(sim, "u_painting");

    GLint frameLoc =
        glGetUniformLocation(sim, "u_frame");

    glUseProgram(show);

    GLint stateLoc =
        glGetUniformLocation(show, "u_state");

    GLint temperatureLoc =
        glGetUniformLocation(show, "u_temperature");

    int current = 0;
    int frame = 0;

    while (!glfwWindowShouldClose(w))
    {
        glfwPollEvents();

        if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(w, 1);

        int next = 1 - current;

        int width, height;
        glfwGetWindowSize(w, &width, &height);

        int x = (int)(mx * WIDTH / width);
        int y = HEIGHT - 1 - (int)(my * HEIGHT / height);

        glUseProgram(sim);
        glBindImageTexture(
            0, tex[current], 0, GL_FALSE, 0,
            GL_READ_ONLY, GL_RGBA16F
        );
        glBindImageTexture(
            1, tex[next], 0, GL_FALSE, 0,
            GL_WRITE_ONLY, GL_RGBA16F
        );
        glBindImageTexture(
            2, temperatureTex[current], 0, GL_FALSE, 0,
            GL_READ_ONLY, GL_R16F
        );
        glBindImageTexture(
            3, temperatureTex[next], 0, GL_FALSE, 0,
            GL_WRITE_ONLY, GL_R16F
        );

        glUniform2i(sizeLoc, WIDTH, HEIGHT);
        glUniform2i(mouseLoc, x, y);
        glUniform1i(paintingLoc, painting);
        glUniform1f(frameLoc, (float)frame++);

        glDispatchCompute(
            (WIDTH + 7) / 8,
            (HEIGHT + 7) / 8,
            1
        );
        glMemoryBarrier(
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_TEXTURE_FETCH_BARRIER_BIT
        );

        current = next;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, width, height);

        glUseProgram(show);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex[current]);
        glUniform1i(stateLoc, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(
            GL_TEXTURE_2D,
            temperatureTex[current]
        );
        glUniform1i(temperatureLoc, 1);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(w);
    }

    glDeleteTextures(2, tex);
    glDeleteTextures(2, temperatureTex);

    glDeleteVertexArrays(1, &vao);

    glDeleteProgram(sim);
    glDeleteProgram(show);

    glfwDestroyWindow(w);
    glfwTerminate();

    return 0;
}
