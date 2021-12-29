#define _CRT_SECURE_NO_WARNINGS

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../include/stb_truetype.h"
#include "../maths/matrix.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO :: Place co-ordinate values on the axes -> Loading fonts -> Font loaded -> Almost done 
// TODO :: Scaling around point
// TODO :: Labels shown at bottom or left if origin isn't within the frame 
// TODO :: Minimize floating point errors 
// TODO :: Allow customization
// TODO :: Allow multiple graphs be drawn on the same window
// TODO :: Load font library and label the axes -> Done 
// Make UI appealing -> Partially done

// TODO Later : Add terminal and a parser

#define TriggerBreakpoint()                                                                                            \
    {                                                                                                                  \
        printf("Abort called at func -> %s, line ->  %d.", __func__, __LINE__);                                        \
        abort();                                                                                                       \
    }

typedef double (*trigfn)(double);

double GaussianIntegral(double x)
{
    return 4 * exp(-x * x / 2);
}

double parabola(double x)
{
    return x * x;
}

double inv(double x)
{
    return 1 / x;
}

double lin(double x)
{
    return x;
}

double discont(double x)
{
    return 1 / (x * x - 1);
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

#define MAJOR_SCALE 200
#define MINOR_SCALE 100

typedef struct
{
    unsigned int vao;
    unsigned int vbo;
    unsigned int program;
    float        width;
    float        value;

    Vec2         center;
    Vec2         scale;

    Vec2         slide_scale; // Controls the major scaling on the axes of the graph
    Vec2         mini_scale; // Controls the minor scaling on the axes of the graph
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

#define MakeString(str) (String){.data = str, .length = strlen(str)};

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
    UserData *data     = glfwGetWindowUserPointer(window);
    *data->OrthoMatrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);
}

void key_callback(GLFWwindow *window, int key, int scancode, int mod, int action)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    // a suitable scaling value for scrolling
    const float origin = 200.0f;
    const float scale  = 5.0f;

    UserData *  data   = glfwGetWindowUserPointer(window);
    Graph *     graph  = data->graph;

    float       off = scale * yoffset;

    graph->slide_scale.y += off;
    graph->slide_scale.x += off;

    if (graph->scale.y <= 0)
        graph->scale.y = 1;
    if (graph->scale.x <= 0)
        graph->scale.x = 1;

    // if scale in both x and y direction reaches certain threshold, reset the values to its original and scale the
    // graph scale accordingly
    if (graph->slide_scale.x < 100.0f || graph->slide_scale.x > 400.0f)
    {
        if (graph->slide_scale.x < 100.0f)
            graph->slide_scale.x = 100.0f;
        if (graph->slide_scale.x > 400.0f)
            graph->slide_scale.x = 400.0f;

        // This changes scaling which controls the point of the label we will be plotting
        graph->scale.x = graph->scale.x * (origin / graph->slide_scale.x);
        graph->scale.y = graph->scale.y * (origin / graph->slide_scale.x);

        // Update the original scaling
        // Once reset change the value accordingly
        graph->value *= origin / graph->slide_scale.x;

        // We should be resetting something here, but what to reset exactly?
        graph->slide_scale = (Vec2){origin, origin};
    }
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

    glfwWindowHint(GLFW_SAMPLES, 4);

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
    uint32_t iCount;
    uint32_t vCount;

    uint32_t iMax;
    uint32_t vMax;

    // index that marks the discontinuity of the vertices
    // dMax -> Maximum number of discontinuity allowed for a function
    uint32_t  dCount;
    uint32_t  dMax;

    uint32_t *Indices;
    uint32_t *Discontinuity;
    Vec2 *    Vertices;

    // Need support for font rendering here
    uint32_t fCount; // This might seem inconsistent but whatever
    // Each fCount will have 6 vertices with each vertex having 4 floats or 2 vec2's
    Vec2 *fVertices;

    // Now multiple plotting.. we need to setup break point somewhere
    uint32_t graphbreak[10]; // returns indices to break at 
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

    // Make graph->scale use value instead of pixel scale
    graph->scale      = (Vec2){1.0f, 1.0f};
    graph->value      = 1.0f;

    graph->slide_scale = (Vec2){200.0f, 200.0f};
}

void RenderGraph(Graph *graph)
{
    glUseProgram(graph->program);
    glBindVertexArray(graph->vao);
    glUniform1i(glGetUniformLocation(graph->program, "grid_width"), 0);
    glUniform2f(glGetUniformLocation(graph->program, "center"), graph->center.x, graph->center.y);
    glUniform2f(glGetUniformLocation(graph->program, "scale"), graph->slide_scale.x, graph->slide_scale.y);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glUseProgram(0);
    glBindVertexArray(0);
}

void InitRenderGroup(RenderGroup *render_group)
{
    memset(render_group, 0, sizeof(*render_group));
    render_group->iMax          = 500;
    render_group->vMax          = 2000;
    render_group->dMax          = 500;

    render_group->Indices       = malloc(sizeof(*render_group->Indices) * render_group->iMax);
    render_group->Vertices      = malloc(sizeof(*render_group->Vertices) * render_group->vMax);
    render_group->Discontinuity = malloc(sizeof(*render_group->Discontinuity) * render_group->dMax);

    const int maxFontVertices   = 10000; // will render 500/6 fonts only
    render_group->fVertices     = malloc(sizeof(*render_group->fVertices) * maxFontVertices);
}

void AddSingleVertex(RenderGroup *render_group, Vec2 vertex)
{
    assert(render_group->vCount + 1 <= render_group->vMax);
    render_group->Vertices[render_group->vCount++] = vertex;
}

void RenderRenderGroup(RenderGroup *render_group, unsigned int program, bool showPoints)
{
    glUniform3f(glGetUniformLocation(program, "inColor"), 0.0f, 0.0f, 1.0f);
    glLineWidth(4);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(*render_group->Vertices) * render_group->vCount, render_group->Vertices);

    // Plot everything in the way
    // glDrawArrays(GL_LINE_STRIP, 0, render_group->vCount);

    // We going for multiple rendering calls .. more work on GPU side

    int discontinuous = 0;
    for (int i = 0; i < (int32_t)render_group->dCount - 1; ++i)
    {
        glDrawArrays(GL_LINE_STRIP, discontinuous, render_group->Discontinuity[i] - discontinuous);
        discontinuous = render_group->Discontinuity[i] + 1;
    }

    //// Pick up from the last discontinuity and continue the graph from there
    if (render_group->dCount)
        glDrawArrays(GL_LINE_STRIP, render_group->Discontinuity[render_group->dCount - 1] + 1,
                     render_group->vCount - (render_group->Discontinuity[render_group->dCount - 1] + 1));
    else
        glDrawArrays(GL_LINE_STRIP, 0, render_group->vCount);

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
    render_group->dCount = 0;
    render_group->fCount = 0;
}

void PlotGraph(RenderGroup *render_group, trigfn func, Graph *graph)
{
    Vec2 vec1, vec2;
    // for (float x = -3.141592f * 2; x < 3.141592f * 2; x += 0.2f)
    //{
    //    vec.x = graph->center.x + x * graph->scale.x;
    //    vec.y = graph->center.y + func(x) * graph->scale.y;
    //    AddSingleVertex(render_group, vec);
    //}
    float step = 0.02f;
    float init = -20.0f;
    float term = 20.0f;

    vec1.x     = graph->center.x + init * graph->slide_scale.x / (graph->scale.x);
    vec1.y     = graph->center.y + func(init) * graph->slide_scale.y / (graph->scale.y);

    for (float x = init; x <= term; x += step)
    {
        // Check for discontinuity of the function
        vec2.x = graph->center.x + (x + step) * graph->slide_scale.x / (graph->scale.x);
        vec2.y = graph->center.y + func(x + step) * graph->slide_scale.y / (graph->scale.y);

        AddSingleVertex(render_group, vec1);
        float slope = atan(fabs((vec2.y - vec1.y) / (vec2.x - vec1.x)));
        if (slope > 1.57f)
        {
            // mark current point as discontinuity
            assert(render_group->dCount < render_group->dMax);
            render_group->Discontinuity[render_group->dCount++] = render_group->vCount;
            // fprintf(stderr, "Point of discontinuity for inverse function is at %f.\n", slope);
        }
        vec1 = vec2;
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

typedef struct Glyph
{
    Vec2 offset;
    Vec2 size;
    Vec2 bearing;
    int  Advance;
} Glyph;

typedef struct Font
{
    unsigned int font_texture;
    unsigned int vao;
    unsigned int vbo;
    float        rasterScale;
    int          width;
    int          height;
    char         font_buffer[512];
    String       font_name;
    Glyph        character[128]; // For first 128 printable characters only
} Font;

void LoadFont(Font *font, const char *font_dir)
{
    memset(font, 0, sizeof(*font));
    stbtt_fontinfo sfont;
    String         fontbuffer = ReadFile(font_dir);
    stbtt_InitFont(&sfont, fontbuffer.data, 0);

    // Load the character's data from stb_truetype
    float fontSize = 35;
    float scale    = stbtt_ScaleForPixelHeight(&sfont, fontSize);
    int   ascent, descent, baseline;
    stbtt_GetFontVMetrics(&sfont, &ascent, &descent, 0);
    baseline = (int)(ascent * scale);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    font->rasterScale = scale;

    int height        = (int)((ascent - descent) * scale);
    int width         = 0;

    font->height      = height;

    for (int ch = 0; ch < 128; ++ch)
    {
        int advance, lsb; // left side bearing
        stbtt_GetCodepointHMetrics(&sfont, ch, &advance, &lsb);
        width += advance;
    }
    width = width * scale;

    // Text Rendering starts here
    unsigned char *bmpbuffers = malloc((size_t)width * height);
    memset(bmpbuffers, 0, width * height);

    float xpos  = 0.0f;
    int   above = ascent * scale;

    for (int ch = 0; ch < 128; ++ch)
    {
        int   advance, lsb, x0, y0, x1, y1;
        float x_shift = xpos - (float)(floor(xpos));
        stbtt_GetCodepointHMetrics(&sfont, ch, &advance, &lsb);
        stbtt_GetCodepointBitmapBoxSubpixel(&sfont, ch, scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);

        // auto stride = ((int)xpos + x0) + width * (above + y0); //  * (baseline + y0) + (int)xpos + x0;
        int stride = (int)xpos + x0 + width * (baseline + y0);

        stbtt_MakeCodepointBitmapSubpixel(&sfont, bmpbuffers + stride, x1 - x0, y1 - y0, width, scale, scale, x_shift,
                                          0, ch);

        Glyph *glyph   = &font->character[ch];
        glyph->offset  = (Vec2){(int)xpos, 0};
        glyph->size    = (Vec2){x1 - x0, y1 - y0};
        glyph->Advance = (int)(advance * scale);
        xpos += advance * scale;
    }

    glGenTextures(1, &font->font_texture);
    glBindTexture(GL_TEXTURE_2D, font->font_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load texture into OpenGL memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, bmpbuffers);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &font->vao);
    glGenBuffers(1, &font->vbo);

    glBindVertexArray(font->vao);
    glBindBuffer(GL_ARRAY_BUFFER, font->vbo);

    const float max_font_limit = 1000 * 50;
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * max_font_limit, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    font->width = width;
    // Allocate size for temporary numbers to string conversion
}

// position in pixel where (0,0) is the lower left corner of the screen
void FillText(RenderGroup *render_group, Font *font, Vec2 position, String str, int scale)
{
    // its quite straightforward
    int32_t x = position.x;
    int32_t y = position.y;

    float    tex0, tex1;
    
    for (uint32_t i = 0; i < str.length; ++i)
    {
        int   count = render_group->fCount;
        Glyph glyph = font->character[(size_t)str.data[i]];
        int   w     = glyph.Advance;
        int   h     = font->height;

        tex0        = glyph.offset.x / font->width;
        tex1        = (glyph.offset.x + w) / font->width;
        // lower left corner
        render_group->fVertices[count * 12 + 0] = (Vec2){x, y};
        render_group->fVertices[count * 12 + 1] = (Vec2){tex0, 1.0f};
        // upper left corner
        render_group->fVertices[count * 12 + 2] = (Vec2){x, y + h};
        render_group->fVertices[count * 12 + 3] = (Vec2){tex0, 0.0f};
        // upper right corner
        render_group->fVertices[count * 12 + 4] = (Vec2){x + w, y + h};
        render_group->fVertices[count * 12 + 5] = (Vec2){tex1, 0.0f};
        // lower left corner
        render_group->fVertices[count * 12 + 6] = (Vec2){x + w, y};
        render_group->fVertices[count * 12 + 7] = (Vec2){tex1, 1.0f};
        // duplicate lower left and upper right
        // upper right corner
        render_group->fVertices[count * 12 + 8] = (Vec2){x + w, y + h};
        render_group->fVertices[count * 12 + 9] = (Vec2){tex1, 0.0f};
        // lower left corner
        render_group->fVertices[count * 12 + 10] = (Vec2){x, y};
        render_group->fVertices[count * 12 + 11] = (Vec2){tex0, 1.0f};

        x                                        = x + glyph.Advance;
        render_group->fCount += 1;
    }
}

void RenderFont(RenderGroup *render_group, Font *font, unsigned int program)
{
    glUseProgram(program);
    glBindVertexArray(font->vao);

    glBindBuffer(GL_ARRAY_BUFFER, font->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, render_group->fCount * 12 * sizeof(Vec2), render_group->fVertices);
    glDrawArrays(GL_TRIANGLES, 0, render_group->fCount * 6);
    glBindVertexArray(0);
    glUseProgram(0);

    // for (int i = 0; i < render_group->fCount*12; ++i)
    //{
    //    fprintf(stderr, "\nx and y are : %f %f.", render_group->fVertices[i].x, render_group->fVertices[i].y);
    //}
    // fprintf(stderr, "\n\n");
}

void RenderLabels(RenderGroup *render_group, Font *font, Graph *graph, Mat4 *orthoMatrix)
{
    // first find the center of the graph and put numbers around here adn there
    // Let's try labeling x-axis
    // calculate the spacing of the major points first
    Vec2 origin = graph->center;
    Vec2 position;
    // first horizontal line is at : pixel scaling of x axis first, then scale value
    // plot across whole label

    for (int i = -15; i <= 15; ++i)
    {
        position.x = origin.x + i * graph->slide_scale.x/2 - font->height / 2;
        position.y = origin.y - font->height;
        // Now calculate the value at the position

        float val   = i * graph->scale.x/2;
        int   count = snprintf(NULL, 0, "%3g", val);
        snprintf(font->font_buffer, count + 1, "%3g", val);
        String str = {.data = font->font_buffer, .length = count};
        FillText(render_group, font, position, str, 0);
    }
    for (int y = -15; y <= 15; ++y)
    {
        if (y == 0)
            continue;
        position.x = origin.x - font->height * 1.5f;
        position.y = origin.y + y * graph->slide_scale.y/2 - font->height / 2;
        // Now calculate the value at the position

        float val   = y * graph->scale.y/2;
        int   count = snprintf(NULL, 0, "%3g", val);
        snprintf(font->font_buffer, count + 1, "%3g", val);
        String str = {.data = font->font_buffer, .length = count};
        FillText(render_group, font, position, str, 0);
    }
}

int main(int argc, char **argv)
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

    glBufferData(GL_ARRAY_BUFFER, 2000 * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
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
    Mat4  ortho_matrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);

    Graph graphs;
    InitGraph(&graphs);

    UserData data = {.OrthoMatrix = &ortho_matrix, .graph = &graphs};
    glfwSetWindowUserPointer(window, &data);
    // PlotGraph(&graph, tan, &graphs);
    // PlotGraph(&graph, cos,&graphs);
    // PlotGraph(&graph, GaussianIntegral, &graphs);
    PlotParametric(&graph, RoseCurves, &graphs);
    State panner = {0};

    // Font shaders and program
    Shader       font_vertex   = LoadShader("./include/text_vertex.glsl", VERTEX_SHADER);
    Shader       font_fragment = LoadShader("./include/text_fragment.glsl", FRAGMENT_SHADER);
    unsigned int fProgram      = LoadProgram(font_vertex, font_fragment);

    Font         ComicSans;
    LoadFont(&ComicSans, "./include/comic.ttf");

    // String str = MakeString("Hello this is from graphing calculator that rivals geogebra calc");
    String str = MakeString("@ComicSans");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);

    while (!glfwWindowShouldClose(window))
    {
        scene_matrix = IdentityMatrix();
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ResetRenderGroup(&graph);
        FillText(&graph, &ComicSans, (Vec2){50.0f, 50.0f}, str, 0);
        //// PlotParametric(&graph, RoseCurves, &graphs);
        //// PlotGraph(&graph, exp, &graphs);
        PlotGraph(&graph, inv, &graphs);
        RenderGraph(&graphs);
        RenderLabels(&graph, &ComicSans, &graphs, &scene_matrix);
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

        RenderRenderGroup(&graph, program, false);
        glUseProgram(fProgram);
        glUniformMatrix4fv(glGetUniformLocation(fProgram, "scene"), 1, GL_TRUE, &ortho_matrix.elem[0][0]);
        glBindTexture(GL_TEXTURE_2D, ComicSans.font_texture);
        RenderFont(&graph, &ComicSans, fProgram);

        // glBindVertexArray(0);
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