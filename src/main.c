#define _CRT_SECURE_NO_WARNINGS

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../include/stb_truetype.h"
#include "../maths/matrix.h"
#include "../include/Morph.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO :: Place co-ordinate values on the axes -> Loading fonts -> Font loaded -> Almost done
// TODO :: Scaling around point -> Lesszz do it -> Done without matrix
// TODO :: Labels shown at bottom or left if origin isn't within the frame
// TODO :: Minimize floating point errors
// TODO :: Allow customization
// TODO :: Allow multiple graphs be drawn on the same window -> Done simply (No plot device yet)
// TODO :: Load font library and label the axes -> Done
// Make UI appealing -> Partially done

// TODO Later : Add terminal and a parser

#define TriggerBreakpoint()                                                                                            \
    {                                                                                                                  \
        printf("Abort called at func -> %s, line ->  %d.", __func__, __LINE__);                                        \
        abort();                                                                                                       \
    }

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

static int screen_width  = 800;
static int screen_height = 800;

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

struct Graph
{
    unsigned int vao;
    unsigned int vbo;
    unsigned int program;
    float        width;
    float        value;

    Vec2         center;
    Vec2         scale;

    Vec2         slide_scale; // Controls the major scaling on the axes of the graph
};

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

void ErrorCallback(int code, const char *description)
{
    fprintf(stderr, "\nGLFW Error -> %s.\n", description);
}

void FrameChangeCallback(GLFWwindow *window, int width, int height)
{
    screen_width  = width;
    screen_height = height;
    glViewport(0, 0, width, height);
    UserData *data     = glfwGetWindowUserPointer(window);
    *data->OrthoMatrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int mod, int action)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

float MagicNumberGenerator(int n)
{
    // Try deciphering it .. :D 
    int non_neg = n >= 0 ? 1 : ((n = -n - 1, (n = 2 - n % 3 + 3 * (n / 3) + 3)), 0);
    int p       = n / 3;
    n           = n % 3;
    float val   = 1;
    // lets try some inline asm 
    //   __asm {
    //       push rsi
	//	push rax
	//	xor rax, rax
	//	mov  rsi, 1
	//L1:
	//	lea  rsi, [rsi + rsi * 4]
	//	inc  rax
	//	add  rsi, rsi 
	//	cmp  eax, z
	//	jne  L1 
	//	mov  a, esi 
	//	pop rax 
	//	pop rsi 
    //   }
    // Lol not supported in x64 architecture
    for (int i = 0; i < p; ++i)
        val *= 10;
    return non_neg ? (n * n + 1) * val : (n * n + 1) / val;
}

void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    const float origin = 200.0f;
    const float scale  = 5.0f;

    UserData *  data   = glfwGetWindowUserPointer(window);
    Graph *     graph  = data->graph;

    // capture mouse co-ordinates here
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    // shift the origin somewhere far from here

    double scaledFactor = 0.0f;

    float  prev_scale_x = graph->slide_scale.x;
    graph->slide_scale.y += scale * yoffset;
    graph->slide_scale.x += scale * yoffset;
    scaledFactor = graph->slide_scale.x / prev_scale_x;
    // Dynamic scaling looks kinda hard
    static int absScale = 0;

    bool changeX = false, changeY = false;
    if (yoffset < 0)
    {
        float should_scale_x = origin / graph->slide_scale.x * graph->scale.x;
        if (should_scale_x >= MagicNumberGenerator(absScale + 1))
            changeX = true;
    }
    else if (yoffset > 0)
    {
        float should_downscale = origin / graph->slide_scale.x * graph->scale.x;
        if (should_downscale <= MagicNumberGenerator(absScale - 1))
            changeY = true;
    }

    if (changeX || changeY)
    {
        if (changeX)
        {
            graph->scale.x = MagicNumberGenerator(++absScale);
            graph->scale.y = graph->scale.x;
        }
        if (changeY)
        {
            graph->scale.x = MagicNumberGenerator(--absScale);
            graph->scale.y = graph->scale.x;
        }
        graph->slide_scale = (Vec2){origin, origin};
    }
    // Now shift the origin to somewhere else .. don't know where yet
    // Its awkard to do these kinda stuffs without matrix .. haha
    int delX = -xPos + graph->center.x;
    int delY = -yPos + graph->center.y;
    graph->center.x += delX * scaledFactor - delX;
    graph->center.y += delY * scaledFactor - delY;
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

    // glfwWindowHint(GLFW_SAMPLES, 4);

    if (!glfwInit())
    {
        fprintf(stderr, "Failed to load GLFW api\n");
        return NULL;
    }
    glfwSetErrorCallback(ErrorCallback);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwSetFramebufferSizeCallback(window, FrameChangeCallback);
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

    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);
    return window;
}

struct RenderScene
{
    uint32_t iCount;
    uint32_t vCount; // To be rendered vertices

    uint32_t pCount; // Original Vertices

    uint32_t iMax;
    uint32_t vMax;

    // index that marks the discontinuity of the vertices
    // dMax -> Maximum number of discontinuity allowed for a function
    uint32_t  dCount;
    uint32_t  dMax;

    uint32_t *Indices;
    uint32_t *Discontinuity;
    Vec2 *    Vertices;
    Vec2 *    Points;

    // Need support for font rendering here
    uint32_t fCount; // This might seem inconsistent but whatever
    // Each fCount will have 6 vertices with each vertex having 4 floats or 2 vec2's
    Vec2 *fVertices;

    // Now multiple plotting.. we need to setup break point somewhere
    uint32_t    cMaxGraph;
    uint32_t    graphbreak[10]; // returns indices to break at
    uint32_t    graphcount;
    Vec3        graphcolor[10];
    const char *graphname[10];
};

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
    graph->scale       = (Vec2){1.0f, 1.0f};

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

void DestroyRenderScene(RenderScene* render_scene)
{
    free(render_scene->Indices); 
    free(render_scene->Vertices); 
    free(render_scene->Discontinuity);
    free(render_scene->Points); 
    free(render_scene->fVertices);
}

void InitRenderScene(RenderScene *scene_group)
{
    memset(scene_group, 0, sizeof(*scene_group));
    scene_group->iMax          = 500;
    scene_group->vMax          = 5000;
    scene_group->dMax          = 500;

    scene_group->Indices       = malloc(sizeof(*scene_group->Indices) * scene_group->iMax);
    scene_group->Vertices      = malloc(sizeof(*scene_group->Vertices) * scene_group->vMax);
    scene_group->Discontinuity = malloc(sizeof(*scene_group->Discontinuity) * scene_group->dMax);
    scene_group->Points        = malloc(sizeof(*scene_group->Vertices) * scene_group->vMax);

    const int maxFontVertices  = 10000; // will render 10000/6 fonts only
    scene_group->fVertices     = malloc(sizeof(*scene_group->fVertices) * maxFontVertices);
}

void AddSingleVertex(RenderScene *scene_group, Vec2 vertex)
{
    // TODO :: Remove this bound check .. Expensiiiivvve
    assert(scene_group->vCount + 1 <= scene_group->vMax);
    scene_group->Vertices[scene_group->vCount++] = vertex;
}

void AddSinglePoint(RenderScene *scene_group, Vec2 vertex)
{
    assert(scene_group->pCount + 1 <= scene_group->vMax);
    scene_group->Points[scene_group->pCount++] = vertex;
}

void RenderRenderScene(RenderScene *scene_group, unsigned int program, bool showPoints)
{
    glLineWidth(4);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(*scene_group->Vertices) * scene_group->vCount, scene_group->Vertices);

    // Plot everything in the way
    // glDrawArrays(GL_LINE_STRIP, 0, scene_group->vCount);

    // We going for multiple rendering calls .. more work on GPU side
    int vertices = 0;
    for (int graph_c = 0; graph_c < scene_group->graphcount; ++graph_c)
    {
        Vec3 inColor = scene_group->graphcolor[graph_c];
        glUniform3f(glGetUniformLocation(program, "inColor"), inColor.x, inColor.y, inColor.z);
        int discontinuous = 0;
        for (int i = vertices; i < (int32_t)scene_group->dCount - 1; ++i)
        {
            glDrawArrays(GL_LINE_STRIP, discontinuous, scene_group->Discontinuity[i] - discontinuous);
            discontinuous = scene_group->Discontinuity[i] + 1;
            if (graph_c < scene_group->graphcount - 1)
            {
                if (discontinuous >= scene_group->graphbreak[scene_group->graphcount + 1])
                    break;
            }
        }

        //// Pick up from the last discontinuity and continue the graph from there
        if (scene_group->dCount)
            glDrawArrays(GL_LINE_STRIP, scene_group->Discontinuity[scene_group->dCount - 1] + 1,
                         scene_group->vCount - (scene_group->Discontinuity[scene_group->dCount - 1] + 1));
        else
            glDrawArrays(GL_LINE_STRIP, vertices, scene_group->graphbreak[graph_c] - vertices);

        if (showPoints)
        {
            glUniform3f(glGetUniformLocation(program, "inColor"), 1.0f, 0.0f, 0.0f);
            glPointSize(10);
            glDrawArrays(GL_POINTS, vertices, scene_group->vCount);
        }
        vertices = scene_group->graphbreak[graph_c];
    }
}

// I will try drawing a grid where graph plotting will take place
void ResetRenderScene(RenderScene *scene_group)
{
    scene_group->iCount     = 0;
    scene_group->vCount     = 0;
    scene_group->dCount     = 0;
    scene_group->fCount     = 0;
    scene_group->pCount     = 0;
    scene_group->graphcount = 0;
}

void PlotGraph(RenderScene *scene_group, oneparamfn func, Graph *graph, Vec3 color, const char *legend)
{
    Vec2  vec1, vec2;

    float step = 0.02f;
    float init = -10.0f;
    float term = 10.0f;

    vec1.x     = graph->center.x + init * graph->slide_scale.x / (graph->scale.x);
    vec1.y     = graph->center.y + func(init) * graph->slide_scale.y / (graph->scale.y);

    for (float x = init; x <= term; x += step)
    {
        // Check for discontinuity of the function
        vec2.x = graph->center.x + (x + step) * graph->slide_scale.x / (graph->scale.x);
        vec2.y = graph->center.y + func(x + step) * graph->slide_scale.y / (graph->scale.y);

        AddSingleVertex(scene_group, vec1);
        float slope = atan(fabs((vec2.y - vec1.y) / (vec2.x - vec1.x)));
        if (slope > 1.57f)
        {
            // mark current point as discontinuity
            assert(scene_group->dCount < scene_group->dMax);
            scene_group->Discontinuity[scene_group->dCount++] = scene_group->vCount;
            // fprintf(stderr, "Point of discontinuity for inverse function is at %f.\n", slope);
        }
        vec1 = vec2;
    }
    // Add number of vertices in the current graph
    assert(scene_group->graphcount < 10);
    scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
    scene_group->graphcolor[scene_group->graphcount - 1] = color;
    scene_group->graphname[scene_group->graphcount - 1]  = legend; // only static strings are expected as of yet
}

Vec2 RoseCurves(double x)
{
    int   k = 10;
    int   a = 2;
    float r = a * cos(k * x);
    return (Vec2){.x = r * cos(x), .y = r * sin(x)};
}

// TODO :: Turn on anit-aliasing

void PlotParametric(RenderScene *scene_group, parametricfn func, Graph *graph)
{
    Vec2 vec;
    for (float i = -3.141592f * 2; i <= 3.141592f * 2; i += 0.04f)
    {
        vec   = func(i);
        vec.x = graph->center.x + vec.x * graph->scale.x;
        vec.y = graph->center.y + vec.y * graph->scale.y;
        AddSingleVertex(scene_group, vec);
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
    unsigned int program;
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
    if (!fontbuffer.data)
    {
        fprintf(stderr, "\nError : Failed to load %s.", font_dir);
        return;
    }
    stbtt_InitFont(&sfont, fontbuffer.data, 0);

    // Load the character's data from stb_truetype
    float fontSize = 30;
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

    // Font shaders and program
    Shader font_vertex   = LoadShader("./include/text_vertex.glsl", VERTEX_SHADER);
    Shader font_fragment = LoadShader("./include/text_fragment.glsl", FRAGMENT_SHADER);
    font->program        = LoadProgram(font_vertex, font_fragment);
}

// position in pixel where (0,0) is the lower left corner of the screen
void FillText(RenderScene *scene_group, Font *font, Vec2 position, String str, int scale)
{
    // its quite straightforward
    int32_t x = position.x;
    int32_t y = position.y;

    float   tex0, tex1;

    for (uint32_t i = 0; i < str.length; ++i)
    {
        int   count = scene_group->fCount;
        Glyph glyph = font->character[(size_t)str.data[i]];
        int   w     = glyph.Advance;
        int   h     = font->height;

        tex0        = glyph.offset.x / font->width;
        tex1        = (glyph.offset.x + w) / font->width;
        // lower left corner
        scene_group->fVertices[count * 12 + 0] = (Vec2){x, y};
        scene_group->fVertices[count * 12 + 1] = (Vec2){tex0, 1.0f};
        // upper left corner
        scene_group->fVertices[count * 12 + 2] = (Vec2){x, y + h};
        scene_group->fVertices[count * 12 + 3] = (Vec2){tex0, 0.0f};
        // upper right corner
        scene_group->fVertices[count * 12 + 4] = (Vec2){x + w, y + h};
        scene_group->fVertices[count * 12 + 5] = (Vec2){tex1, 0.0f};
        // lower left corner
        scene_group->fVertices[count * 12 + 6] = (Vec2){x + w, y};
        scene_group->fVertices[count * 12 + 7] = (Vec2){tex1, 1.0f};
        // duplicate lower left and upper right
        // upper right corner
        scene_group->fVertices[count * 12 + 8] = (Vec2){x + w, y + h};
        scene_group->fVertices[count * 12 + 9] = (Vec2){tex1, 0.0f};
        // lower left corner
        scene_group->fVertices[count * 12 + 10] = (Vec2){x, y};
        scene_group->fVertices[count * 12 + 11] = (Vec2){tex0, 1.0f};

        x                                       = x + glyph.Advance;
        scene_group->fCount += 1;
    }
}

void RenderFont(RenderScene *scene_group, Font *font, Mat4 *scene_transform)
{
    glUseProgram(font->program);
    glBindVertexArray(font->vao);

    glUniformMatrix4fv(glGetUniformLocation(font->program, "scene"), 1, GL_TRUE, &scene_transform->elem[0][0]);
    glBindTexture(GL_TEXTURE_2D, font->font_texture);

    glBindBuffer(GL_ARRAY_BUFFER, font->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, scene_group->fCount * 12 * sizeof(Vec2), scene_group->fVertices);
    glDrawArrays(GL_TRIANGLES, 0, scene_group->fCount * 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

void RenderLabels(RenderScene *scene_group, Font *font, Graph *graph, Mat4 *orthoMatrix)
{
    Vec2 origin = graph->center;
    Vec2 position;

    // Calculate the min and max vertical bar visible on the current frame first
    int xLow  = -origin.x / graph->slide_scale.x - 1;
    int xHigh = (screen_width - origin.x) / graph->slide_scale.x + 1;

    for (int i = xLow * 2; i <= xHigh * 2; ++i)
    {
        position.x = origin.x + i * graph->slide_scale.x / 2 - font->height / 2;
        position.y = origin.y - font->height;
        // Now calculate the value at the position

        float val   = i * graph->scale.x / 2;
        int   count = snprintf(NULL, 0, "%3g", val);
        snprintf(font->font_buffer, count + 1, "%3g", val);
        String str = {.data = font->font_buffer, .length = count};
        FillText(scene_group, font, position, str, 0);
    }

    int yLow  = -origin.y / graph->slide_scale.y - 1;
    int yHigh = (screen_height - origin.y) / graph->slide_scale.y + 1;

    for (int y = yLow * 2; y <= yHigh * 2; ++y)
    {
        if (y == 0)
            continue;
        position.x  = origin.x - font->height * 1.5f;
        position.y  = origin.y + y * graph->slide_scale.y / 2 - font->height / 2;

        float val   = y * graph->scale.y / 2;
        int   count = snprintf(NULL, 0, "%3g", val);
        snprintf(font->font_buffer, count + 1, "%3g", val);
        String str = {.data = font->font_buffer, .length = count};
        FillText(scene_group, font, position, str, 0);
    }
}

void DrawLegends(RenderScene *scene_group, Font *font, Graph *graph) // choose location automatically
{
    // First draw a appropriate bounding box -> Easily handled by line strips
    // Draw a small rectangle filled with that color -> Might need a new pixel shader -> Used simple line instead
    // Finally draw text with plot name -> Already available
    // Find the appropriate place to put the legend
    Vec2 pos   = {0};
    pos.x      = screen_width - 200;
    pos.y      = screen_height - font->height;

    int gcount = scene_group->graphcount;
    for (int label = 0; label < gcount; ++label)
    {
        // Add a line indicator with given color for legends
        AddSingleVertex(scene_group, pos);
        AddSingleVertex(scene_group, (Vec2){pos.x + 50, pos.y});
        scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
        scene_group->graphcolor[scene_group->graphcount - 1] = scene_group->graphcolor[label];
        FillText(scene_group, font, (Vec2){pos.x + 75, pos.y - font->height / 2},
                 (String){.data = scene_group->graphname[label], .length = strlen(scene_group->graphname[label])}, 0);

        pos.y -= font->height;
    }

    // Add a graph break for a small box around legends
    AddSingleVertex(scene_group, (Vec2){pos.x - 10, screen_height});
    AddSingleVertex(scene_group, (Vec2){pos.x - 10, pos.y - 20});
    AddSingleVertex(scene_group, (Vec2){screen_width, pos.y - 20});

    assert(scene_group->graphcount < 10);
    scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
    scene_group->graphcolor[scene_group->graphcount - 1] = (Vec3){0.5f, 0.5f, 0.5f};
    scene_group->graphname[scene_group->graphcount - 1]  = ""; // only static strings are expected as of yet
}

void ShowList(RenderScene *scene_group, Font *font, Graph *graph, float *x, float *y, int length,
              const char *GiveOnlyStaticStrings, Vec3 color)
{
    Vec2 vec;
    for (int points = 0; points < length; ++points)
    {
        vec.x = graph->center.x + x[points] * graph->slide_scale.x / (graph->scale.x);
        vec.y = graph->center.y + y[points] * graph->slide_scale.y / (graph->scale.y);
        AddSingleVertex(scene_group, vec);
    }

    assert(scene_group->graphcount < 10);
    scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
    scene_group->graphcolor[scene_group->graphcount - 1] = color;
    scene_group->graphname[scene_group->graphcount - 1]  = GiveOnlyStaticStrings;
}

int nolongermain(int argc, char **argv)
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

    glBufferData(GL_ARRAY_BUFFER, 5000 * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RenderScene graph;
    InitRenderScene(&graph);

    Mat4  scene_matrix = IdentityMatrix();
    Mat4  ortho_matrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);

    Graph graphs;
    InitGraph(&graphs);

    UserData data = {.OrthoMatrix = &ortho_matrix, .graph = &graphs};
    glfwSetWindowUserPointer(window, &data);

    // PlotParametric(&graph, RoseCurves, &graphs);
    State panner = {0};

    Font  ComicSans;
    LoadFont(&ComicSans, "./include/comic.ttf");

    String str = MakeString("@ComicSans");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // glEnable(GL_MULTISAMPLE);

    // Mulitple discontinuous graphs aren't supported fully for now
    float *x     = malloc(sizeof(float) * 10);
    float *y     = malloc(sizeof(float) * 10);
    int    count = 10;
    for (int i = 0; i < 10; ++i)
    {
        x[i] = i;
        y[i] = i * i;
    }

    while (!glfwWindowShouldClose(window))
    {
        scene_matrix = IdentityMatrix();
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ResetRenderScene(&graph);
        FillText(&graph, &ComicSans, (Vec2){50.0f, 50.0f}, str, 0);

        PlotGraph(&graph, sqrt, &graphs, (Vec3){0.0f, 0.0f, 1.0f}, "sqrt");
        PlotGraph(&graph, GaussianIntegral, &graphs, (Vec3){0.0f, 1.0f, 0.0f}, "Gaussian");
        // PlotGraph(&graph, cos, &graphs, (Vec3){1.0f, 0.0f, 0.0f}, "cosine");
        PlotGraph(&graph, tanh, &graphs, (Vec3){1.0f, 0.0f, 1.0f}, "tanh");
        ShowList(&graph, &ComicSans, &graphs, x, y, count, "List Plot", (Vec3){1.0f, 1.0f, 0.0f});
        RenderGraph(&graphs);
        RenderLabels(&graph, &ComicSans, &graphs, &scene_matrix);
        glUseProgram(program);

        //// implies column constitue base vectors not the other one
        glUniformMatrix4fv(glGetUniformLocation(program, "scene"), 1, GL_TRUE, &ortho_matrix.elem[0][0]);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        DrawLegends(&graph, &ComicSans, &graphs);
        RenderRenderScene(&graph, program, false);

        RenderFont(&graph, &ComicSans, &ortho_matrix);
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

// Functions related to API
// To expose to API we need two sets of data .. first the original values and second scaled values

void APIRecalculate(MorphPlotDevice *device)
{
    device->render_scene->vCount = 0;
    Vec2 vec;
    for (int re = 0; re < device->render_scene->pCount; ++re)
    {
        vec.x = device->graph->center.x +
                device->render_scene->Points[re].x * device->graph->slide_scale.x / (device->graph->scale.x);
        vec.y = device->graph->center.y +
                device->render_scene->Points[re].y * device->graph->slide_scale.y / (device->graph->scale.y);
        AddSingleVertex(device->render_scene, vec);
    }
}

MorphPlotDevice MorphCreateDevice()
{
    MorphPlotDevice device;

    device.window   = LoadGLFW(screen_width, screen_height, "Graph FFI");

    Shader vertex   = LoadShader("./include/vertex.glsl", VERTEX_SHADER);
    Shader fragment = LoadShader("./include/fragment.glsl", FRAGMENT_SHADER);
    device.program  = LoadProgram(vertex, fragment);

    glGenVertexArrays(1, &device.vao);
    glGenBuffers(1, &device.vbo);

    glBindBuffer(GL_ARRAY_BUFFER, device.vbo);
    glBindVertexArray(device.vao);

    // magic constant 5000 is total number of vertices allowed
    glBufferData(GL_ARRAY_BUFFER, 5000 * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RenderScene *scene = malloc(sizeof(*scene));
    InitRenderScene(scene);

    Mat4 *ortho_matrix = malloc(sizeof(Mat4));
    *ortho_matrix      = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);

    Graph *graph       = malloc(sizeof(*graph));
    InitGraph(graph);

    UserData *data = malloc(sizeof(*data));
    *data          = (UserData){.OrthoMatrix = ortho_matrix, .graph = graph};
    glfwSetWindowUserPointer(device.window, data);

    State panner    = {0};

    Font *ComicSans = malloc(sizeof(*ComicSans));
    LoadFont(ComicSans, "./include/comic.ttf");

    String str = MakeString("@ComicSans");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    device.render_scene = scene;
    device.graph        = graph;
    device.font         = ComicSans;
    device.transform    = ortho_matrix;
    return device;
}

void APIHandleEvents(MorphPlotDevice *device, State *state);

void MorphPlotFunc(MorphPlotDevice *device, oneparamfn fn, float r, float g, float b, const char *cstronly)
{
    float init = -10.0f;
    float term = 10.0f;
    float step = 0.05f;
    Vec2  vec;

    for (float x = init; x <= term; x += step)
    {
        // This API version doesn't check for discontinuity .. Above function checks for discontinuity of one function
        vec.x = x;
        vec.y = fn(vec.x);
        AddSinglePoint(device->render_scene, vec);
    }
    // Add number of vertices in the current graph
    assert(device->render_scene->graphcount < 10);
    device->render_scene->graphbreak[device->render_scene->graphcount++]   = device->render_scene->pCount;
    device->render_scene->graphcolor[device->render_scene->graphcount - 1] = (Vec3){r,g,b};
    device->render_scene->graphname[device->render_scene->graphcount - 1]  = cstronly;
}

void APIReset(MorphPlotDevice* device, uint32_t hold)
{
    device->render_scene->fCount     = 0;
    device->render_scene->graphcount = hold; 
}

void MorphShow(MorphPlotDevice *device)
{
    glfwShowWindow(device->window);
    Mat4  scene_matrix = IdentityMatrix();
    State panner       = {0};

    MorphPlotFunc(device, GaussianIntegral, 0.0f, 1.0f, 0.0f, "Gaussian");
    // PlotGraph(device->render_scene, cos, device->graph, (Vec3){1.0f, 0.0f, 0.0f}, "cosine");
    // PlotGraph(device->render_scene, tanh, device->graph, (Vec3){1.0f, 0.0f, 1.0f}, "tanh");
    // RenderLabels(device->render_scene, device->font, device->graph, &scene_matrix);

    uint32_t hold = device->render_scene->graphcount; 

    while (!glfwWindowShouldClose(device->window))
    {
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // Instead of re-rendering, change the scale of the already plotted points
        APIReset(device,hold);
        APIRecalculate(device);
        RenderGraph(device->graph);
        glUseProgram(device->program);
        glUniformMatrix4fv(glGetUniformLocation(device->program, "scene"), 1, GL_TRUE, &device->transform->elem[0][0]);
        glBindVertexArray(device->vao);
        glBindBuffer(GL_ARRAY_BUFFER, device->vbo);

        RenderLabels(device->render_scene, device->font, device->graph, device->transform);
        DrawLegends(device->render_scene, device->font, device->graph);
        RenderRenderScene(device->render_scene, device->program, false);
        RenderFont(device->render_scene, device->font, device->transform);

        APIHandleEvents(device, &panner);
        glfwSwapBuffers(device->window);
        glfwPollEvents();
    }
}

void APIHandleEvents(MorphPlotDevice *device, State *state)
{
    if (glfwGetMouseButton(device->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(device->window, &xpos, &ypos);
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
            device->graph->center.x += delX;
            device->graph->center.y -= delY;

            // Now shift all the plotted points
            for (int pt = 0; pt < device->render_scene->vCount; ++pt)
            {
            }
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

void MorphDestroyDevice(MorphPlotDevice *device)
{
    free(device->render_scene);
    UserData *data = glfwGetWindowUserPointer(device->window);
    free(data);
    free(device->transform);
    free(device->graph);
    free(device->font);
    DestroyRenderScene(device->render_scene);
    glfwDestroyWindow(device->window);
    glfwTerminate();
}

void MorphAddList(MorphPlotDevice *device, float *xpts, float *ypts, int length, float r, float g, float b,
                  const char *cstronly)
{
    Vec2 vec;
    for (int points = 0; points < length; ++points)
    {
        vec.x = device->graph->center.x + xpts[points] * device->graph->slide_scale.x / (device->graph->scale.x);
        vec.y = device->graph->center.y + ypts[points] * device->graph->slide_scale.y / (device->graph->scale.y);
        AddSingleVertex(device->render_scene, vec);
    }

    assert(device->render_scene->graphcount < 10);
    device->render_scene->graphbreak[device->render_scene->graphcount++]   = device->render_scene->vCount;
    device->render_scene->graphcolor[device->render_scene->graphcount - 1] = (Vec3){r, g, b};
    device->render_scene->graphname[device->render_scene->graphcount - 1]  = cstronly;
}