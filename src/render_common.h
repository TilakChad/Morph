#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdint.h>

#include "./Morph.h"

typedef struct VertexBuffer
{
    bool     dirty;
    uint32_t count;
    uint32_t max;
    uint32_t vbo;
    uint8_t *data;
} VertexBuffer;

typedef enum Primitives
{
    TRIANGLES      = GL_TRIANGLES,
    LINES          = GL_LINES,
    LINE_STRIP     = GL_LINE_STRIP,
    LINE_LOOP      = GL_LINE_LOOP,
    TRIANGLE_STRIP = GL_TRIANGLE_STRIP
} Primitives;

typedef enum FunctionType
{
    LIST,
    PARAMETRIC_1D,
    PARAMETRIC_2D,
    IMPLICIT_2D
} FunctionType;

typedef struct GPUBatch
{
    VertexBuffer vertex_buffer;
    uint32_t     vao;
    Primitives   primitive;
} GPUBatch;

typedef struct VertexData2D
{
    float x, y;
    float n_x, n_y;
} VertexData2D;

typedef struct
{
    char  *data;
    size_t length;
} String;

#define MakeString(str) (String){.data = str, .length = strlen(str)};

typedef enum
{
    VERTEX_SHADER,
    FRAGMENT_SHADER,
    GEOMETRY_SHADER
} ShaderType;

typedef struct
{
    unsigned int shader;
    ShaderType   type;
} Shader;

typedef struct Mat4
{
    float elem[4][4];
} Mat4;

typedef struct Glyph
{
    MVec2 offset;
    MVec2 size;
    MVec2 bearing;
    int   Advance;
} Glyph;

typedef struct Font
{
    unsigned int font_texture;
    unsigned int vao;
    unsigned int vbo;
    unsigned int program;
    float        rasterScale;
    int32_t      width;
    int32_t      height;
    uint32_t     size;
    char         font_buffer[512];
    String       font_name;
    Glyph        character[128]; // For first 128 printable characters only
} Font;

// TODO :: Use STB Pack to properly pack the glyphs.
typedef struct
{
    uint32_t  program;
    bool      updated;
    Font     *font;
    GPUBatch *batch;
    GPUBatch *font_batch;
    struct
    {
        bool        should_run;
        bool        hidden; 
        float       last_time;
        const float time_constant;
        float       t; // use linear interpolation
    } Anim;

    struct
    {
        bool        should_animate;
        const float time_constant;
        float       started;
        float       t;
        float       origin;
        float       target;
    } CaretAnim;

    Mat4 local_transform;
} PanelRenderStruct;

typedef struct
{
    float x, y;
} Pos2D;

typedef struct
{
    // I guess we need to apply local transform here relative to the box above
    Pos2D    pos;
    Pos2D    dimension;
    char     buffer[256];
    uint32_t len;
    uint32_t caret_pos; // position of the caret currently in the panel

    struct
    {
        uint16_t advancement[256];
        uint16_t visible_start;
        uint16_t visible_end;
    } renderdata;

} TextPanel;

typedef struct
{
    uint32_t  history_count;
    uint32_t  active_panel;
    TextPanel history[100];
} PanelHistory;

typedef struct
{
    // origin and dimension etc should come here later on
    uint32_t line_gap;
    uint32_t font_size;
    uint32_t box_count;
    uint32_t box_gap;
    uint32_t active_box;
} PanelLayout;

struct Panel
{
    Pos2D origin; // the top left corner of the panel
    Pos2D dimension;
    // Might need to enable scissor here
    // specify scissor dimension here
    PanelLayout       layout;
    PanelHistory      panel;
    PanelRenderStruct render;
};

const char  *ShaderTypeName(ShaderType shader);
unsigned int LoadProgram(Shader vertex, Shader fragment);
Mat4         OrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar);
Mat4         TranslationMatrix(float x, float y, float z);
Mat4         MatrixMultiply(Mat4 *mat1, Mat4 *mat2);
void         MatrixVectorMultiply(Mat4 *mat, float vec[4]);
Mat4         IdentityMatrix();
Shader       LoadShadersFromString(const char *cstr, ShaderType type);
String       ReadFiles(const char *file_path);
GPUBatch    *CreateNewBatch(Primitives primitive);
Shader       LoadShader(const char *shader_path, ShaderType type);
GLFWwindow  *LoadGLFW(int width, int height, const char *title);

// From interactive.h
Panel *CreatePanel(uint32_t, uint32_t);
void   RenderPanel(Panel *panel, Font *font, Mat4 *ortho);
void   PanelFrameChangeCallback(Panel *panel, int width, int height);
void   PanelKeyCallback(Panel *panel, int key, int scancode, int action, int mods);
void   PanelCharCallback(Panel *panel, unsigned int codepoint);