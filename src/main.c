#define _CRT_SECURE_NO_WARNINGS

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../maths/matrix.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TriggerBreakpoint()                                                                                            \
    { printf("Abort called at func -> %s, line ->  %d.",__func__,__LINE__); abort(0);}

typedef double (*trigfn)(double);

double GaussianIntegral(double x)
{
    return 4 * exp(-x * x / 2);
}

double inv(double x) 
{
    return 1/x; 
}

double lin(double x) 
{
    return x; 
}

#define no_default_case() __assume(0)

int screen_width  = 800;
int screen_height = 800;

typedef struct Vec2
{
    float x;
    float y;
} Vec2;

typedef struct Vec3
{
    float x;
    float y;
    float z;
} Vec3;

typedef struct
{
    unsigned int vao;
    unsigned int vbo;
    unsigned int program;
    float        width;

    Vec2         center;
    Vec2         scale;
} Graph;

typedef struct
{
    Mat4 * OrthoMatrix;
    Graph *graph;
} UserData;

typedef struct
{
    char * data;
    size_t length;
} String;

typedef enum
{
    VERTEX_SHADER,
    FRAGMENT_SHADER
} shader_type;

typedef struct
{
    unsigned int shader;
    shader_type  type;
} Shader;

void error_callback(int code, const char *description)
{
    fprintf(stderr, "Error is %s.", description);
}

void frame_change_callback(GLFWwindow *window, int width, int height)
{
    screen_width  = width;
    screen_height = height;
    glViewport(0, 0, width, height);
    UserData *data    = glfwGetWindowUserPointer(window);
    *data->OrthoMatrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);
}

void key_callback(GLFWwindow *window, int key, int scancode, int mod, int action)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void scroll_callback(GLFWwindow* window,double xoffset, double yoffset)
{
    // a suitable scaling value for scrolling 
    const float scale = 5.0f; 
    UserData *  data  = glfwGetWindowUserPointer(window); 
    Graph *     graph = data->graph; 
    graph->scale.y -= scale * yoffset; 
    graph->scale.x -= scale * yoffset; 
    if (graph->scale.y <= 0)
        graph->scale.y = 1;
    if (graph->scale.x <= 0)
        graph->scale.x = 1;
}

unsigned int LoadProgram(Shader vertex, Shader fragment)
    {
    if (vertex.type != VERTEX_SHADER && fragment.type != FRAGMENT_SHADER)
    {
        fprintf(stderr, "Shaders mismatched for program\n");
        return -1;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex.shader);
    glAttachShader(program, fragment.shader);

    glLinkProgram(program);

    int linked = 0;

    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Failed to link program \n -> %s.", infoLog);
        return -1;
    }
    return program;
}

String ReadFile(const char *file_path)
{
    FILE *fp = fopen(file_path, "rb");
    if (!fp)
        return (String){.data = NULL, .length = 0};

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    String contents;
    contents.data       = malloc(sizeof(char) * (size + 1));
    contents.length     = size;

    size_t read_size    = fread(contents.data, sizeof(char), size + 1, fp);
    contents.data[size] = '\0';
    assert(read_size == size);
    return contents;
}

Shader LoadShader(const char *shader_path, shader_type type)
{
    String       str = ReadFile(shader_path);
    Shader       shader;
    unsigned int shader_define;
    switch (type)
    {
    case FRAGMENT_SHADER:
        shader_define = GL_FRAGMENT_SHADER;
        break;
    case VERTEX_SHADER:
        shader_define = GL_VERTEX_SHADER;
        break;
    default:
        TriggerBreakpoint();
    }
    shader.shader = glCreateShader(shader_define);
    shader.type   = type;
    glShaderSource(shader.shader, 1, &str.data, NULL);
    glCompileShader(shader.shader);
    int compiled;
    glGetShaderiv(shader.shader, GL_COMPILE_STATUS, &compiled);

    free(str.data);

    if (!compiled)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader.shader, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile %s",
                (shader_define == GL_VERTEX_SHADER ? "vertex shader" : "fragment shader"));
        fprintf(stderr, "Reason -> %s.", infoLog);
    }
    else
        fprintf(stderr, "\n%s compilation passed.\n",
                (shader_define == GL_VERTEX_SHADER ? "vertex shader" : "fragment shader"));
    return shader;
}

GLFWwindow *LoadGLFW(int width, int height, const char *title)
{
    glfwWindowHint(GLFW_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!glfwInit())
    {
        fprintf(stderr, "Failed to load GLFW api\n");
        return NULL;
    }
    glfwSetErrorCallback(error_callback);

    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwSetFramebufferSizeCallback(window, frame_change_callback);
    if (!window)
    {
        fprintf(stderr, "Failed to create window");
        return NULL;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to load glad");
        return NULL;
    }

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    return window;
}


typedef struct
{
    uint32_t  iCount;
    uint32_t  vCount;

    uint32_t  iMax;
    uint32_t  vMax;

    // index that marks the discontinuity of the vertices 
    uint32_t  iDiscontinuity; 
    uint32_t  dMax; 


    uint32_t *Indices;
    uint32_t *Discontinuity; 
    Vec2 *    Vertices;
} RenderGroup;



typedef struct
{
    bool   bPressed;
    double xpos;
    double ypos;
} State;


typedef Vec2 (*parametricfn)(double);

void InitGraph(Graph *graph)
{
    glGenVertexArrays(1, &graph->vao);
    glBindVertexArray(graph->vao);
    glGenBuffers(1, &graph->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, graph->vbo);

    // use fullscreen to render grid
    float buffer[] = {-1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    Shader vertex   = LoadShader("./include/grid_vertex.glsl", VERTEX_SHADER);
    Shader fragment = LoadShader("./include/grid_fragment.glsl", FRAGMENT_SHADER);
    graph->program  = LoadProgram(vertex, fragment);

    graph->center   = (Vec2){400.0f, 400.0f};
    graph->scale    = (Vec2){100.0f, 100.0f};
}

void RenderGraph(Graph *graph)
{
    glUseProgram(graph->program);
    glBindVertexArray(graph->vao);
    glUniform1i(glGetUniformLocation(graph->program, "grid_width"), 0);
    glUniform2f(glGetUniformLocation(graph->program, "center"), graph->center.x, graph->center.y);
    glUniform2f(glGetUniformLocation(graph->program, "scale"), graph->scale.x, graph->scale.y);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glUseProgram(0);
    glBindVertexArray(0);
}

void InitRenderGroup(RenderGroup *render_group)
{
    memset(render_group, 0, sizeof(*render_group));
    render_group->iMax     = 500;
    render_group->vMax     = 1000;

    render_group->Indices  = malloc(sizeof(*render_group->Indices) * render_group->iMax);
    render_group->Vertices = malloc(sizeof(*render_group->Vertices) * render_group->vMax);
}

void AddSingleVertex(RenderGroup *render_group, Vec2 vertex)
{
    assert(render_group->vCount + 1 <= render_group->vMax);
    render_group->Vertices[render_group->vCount++] = vertex;
}

void RenderRenderGroup(RenderGroup *render_group, unsigned int program, bool showPoints)
{
    glUniform3f(glGetUniformLocation(program, "inColor"), 0.0f, 1.0f, 0.0f);
    glLineWidth(5);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(*render_group->Vertices) * render_group->vCount, render_group->Vertices);
    glDrawArrays(GL_LINE_STRIP, 0, render_group->vCount);

    // #if render vertices too then, 
    if (showPoints)
    {
        glUniform3f(glGetUniformLocation(program, "inColor"), 1.0f, 0.0f, 0.0f);
        glPointSize(10);
        glDrawArrays(GL_POINTS, 0, render_group->vCount);
    }
}

// I will try drawing a grid where graph plotting will take place
void ResetRenderGroup(RenderGroup *render_group)
{
    render_group->iCount = 0;
    render_group->vCount = 0;
}

void PlotGraph(RenderGroup *render_group, trigfn func, Graph *graph)
{
    Vec2 vec;
    //for (float x = -3.141592f * 2; x < 3.141592f * 2; x += 0.2f)
    //{
    //    vec.x = graph->center.x + x * graph->scale.x;
    //    vec.y = graph->center.y + func(x) * graph->scale.y;
    //    AddSingleVertex(render_group, vec);
    //}
    for (float x = -5.0f; x <= 5.0f; x+=1.0f)
    {
        vec.x = graph->center.x + x * graph->scale.x; 
        vec.y = graph->center.y + func(x) * graph->scale.y; 
        AddSingleVertex(render_group, vec);
    }
}

Vec2 RoseCurves(double x)
{
    int   k = 10;
    int   a = 2;
    float r = a * cos(k * x);
    return (Vec2){.x = r * cos(x), .y = r * sin(x)};
}

// TODO :: Turn on anit-aliasing

void PlotParametric(RenderGroup *render_group, parametricfn func, Graph *graph)
{
    Vec2 vec;
    for (float i = -3.141592f * 2; i <= 3.141592f * 2; i += 0.04f)
    {
        vec   = func(i);
        vec.x = graph->center.x + vec.x * graph->scale.x;
        vec.y = graph->center.y + vec.y * graph->scale.y;
        AddSingleVertex(render_group, vec);
    }
}

void HandleEvents(GLFWwindow *window, State *state, Graph *graph);

int  main(int argc, char **argv)
{
    GLFWwindow * window   = LoadGLFW(screen_width, screen_height, "Graph FFI");

    Shader       vertex   = LoadShader("./include/vertex.glsl", VERTEX_SHADER);
    Shader       fragment = LoadShader("./include/fragment.glsl", FRAGMENT_SHADER);

    unsigned int program  = LoadProgram(vertex, fragment);
    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindVertexArray(vao);

    glBufferData(GL_ARRAY_BUFFER, 1000 * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RenderGroup graph;
    InitRenderGroup(&graph);

    // AddSingleVertex(&graph, (Vec2){0, 0});
    // AddSingleVertex(&graph, (Vec2){400, 400});
    // AddSingleVertex(&graph, (Vec2){800, 0});

    Mat4 scene_matrix = IdentityMatrix();
    // update the ortho matrix when the frame changes
    Mat4 ortho_matrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);

    Graph graphs;
    InitGraph(&graphs);

    UserData data         = {.OrthoMatrix = &ortho_matrix, .graph = &graphs};
    glfwSetWindowUserPointer(window, &data);
    // PlotGraph(&graph, tan, &graphs);
    // PlotGraph(&graph, cos,&graphs);
    // PlotGraph(&graph, GaussianIntegral, &graphs);
    PlotParametric(&graph, RoseCurves, &graphs);
    State panner = {0};

    while (!glfwWindowShouldClose(window))
    {
        scene_matrix = IdentityMatrix();
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ResetRenderGroup(&graph);
        // PlotParametric(&graph, RoseCurves, &graphs);
        // PlotGraph(&graph, exp, &graphs);
        PlotGraph(&graph, lin, &graphs);
        RenderGraph(&graphs);
        glUseProgram(program);

        // Mat4 translation = TranslationMatrix(0.0f, 0.0f, 0.0f);
        // scene_matrix     = MatrixMultiply(&translation, &scene_matrix);
        // scene_matrix = TranslationMatrix(0.5f, 0.0f, 0.0f);
        // Mat4 scale   = ScalarMatrix(2.0f, 2.0f, 2.0f);

        // scene_matrix = MatrixMultiply(&scene_matrix, &scale);

        //// implies column constitue base vectors not the other one
        glUniformMatrix4fv(glGetUniformLocation(program, "scene"), 1, GL_TRUE, &ortho_matrix.elem[0][0]);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        RenderRenderGroup(&graph, program,false);
        glBindVertexArray(0);
        HandleEvents(window, &panner, &graphs);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void HandleEvents(GLFWwindow *window, State *state, Graph *graph)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        switch (state->bPressed)
        {
        case false:
            state->bPressed = true;
            state->xpos     = xpos;
            state->ypos     = ypos;
            break;
        case true:
        {
            double delX = xpos - state->xpos;
            double delY = ypos - state->ypos;
            graph->center.x += delX;
            graph->center.y -= delY;
            state->xpos = xpos; 
            state->ypos = ypos;
        }
        break;
        default:
            TriggerBreakpoint();
        }
    }
    else 
        state->bPressed = false; 
}