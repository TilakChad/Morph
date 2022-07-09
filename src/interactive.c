// An interactive UI that pops up from the left side of the screen when pressed TAB
// It will contain a place to write out function for plotting

#define _CRT_SECURE_NO_WARNINGS
// So let's get started

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "./stb_truetype.h"

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

typedef struct
{
    char  *data;
    size_t length;
} String;

#define MakeString(str) (String){.data = str, .length = strlen(str)};

unsigned int LoadProgram(Shader vertex, Shader fragment);

const char  *ShaderTypeName(ShaderType shader)
{
    switch (shader)
    {
    case VERTEX_SHADER:
        return "Vertex Shader";
    case FRAGMENT_SHADER:
        return "Fragment Shader";
    case GEOMETRY_SHADER:
        return "Geometry Shader";
    default:
        __assume(false);
    }
}
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

typedef struct Mat4
{
    float elem[4][4];
} Mat4;

typedef struct Font Font;

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

static Mat4 OrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4 matrix       = {{0}};
    matrix.elem[0][0] = 2.0f / (right - left);
    matrix.elem[1][1] = 2.0f / (top - bottom);
    matrix.elem[2][2] = -2.0f / (zFar - zNear);

    matrix.elem[0][3] = (right + left) / (left - right);
    matrix.elem[1][3] = (top + bottom) / (bottom - top);
    matrix.elem[2][3] = (zFar + zNear) / (zNear - zFar);

    matrix.elem[3][3] = 1.0f;
    return matrix;
}

static Mat4 TranslationMatrix(float x, float y, float z)
{
    Mat4 matrix       = {{0}};

    matrix.elem[0][0] = 1.0f;
    matrix.elem[1][1] = 1.0f;
    matrix.elem[2][2] = 1.0f;

    matrix.elem[0][3] = x;
    matrix.elem[1][3] = y;
    matrix.elem[2][3] = z;
    matrix.elem[3][3] = 1.0f;
    return matrix;
}

static Mat4 MatrixMultiply(Mat4 *mat1, Mat4 *mat2)
{
    Mat4  matrix = {{0}};
    float var    = 0;
    for (uint8_t row = 0; row < 4; ++row)
    {
        for (uint8_t k = 0; k < 4; ++k)
        {
            var = mat1->elem[row][k];
            for (uint8_t col = 0; col < 4; ++col)
                matrix.elem[row][col] += var * mat2->elem[k][col];
        }
    }
    return matrix;
}

static void MatrixVectorMultiply(Mat4 *mat, float vec[4])
{
    float result[4] = {0};
    for (uint8_t i = 0; i < 4; ++i)
    {
        for (uint8_t j = 0; j < 4; ++j)
        {
            result[i] += mat->elem[i][j] * vec[j];
        }
    }
    memcpy(vec, result, sizeof(float) * 4);
}

static Mat4 IdentityMatrix()
{
    Mat4 matrix       = {{0}};
    matrix.elem[0][0] = 1.0f;
    matrix.elem[1][1] = 1.0f;
    matrix.elem[2][2] = 1.0f;
    matrix.elem[3][3] = 1.0f;
    return matrix;
}

typedef struct
{
    // origin and dimension etc should come here later on
    uint32_t line_gap;
    uint32_t font_size;
    uint32_t box_count;
    uint32_t box_gap;
    uint32_t active_box;
} PanelLayout;

typedef struct
{
    Pos2D origin; // the top left corner of the panel
    Pos2D dimension;
    // Might need to enable scissor here
    // specify scissor dimension here
    PanelLayout       layout;
    PanelHistory      panel;
    PanelRenderStruct render;
} Panel;

// Font loading
typedef struct Glyph
{
    Pos2D offset;
    Pos2D size;
    Pos2D bearing;
    int   Advance;
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
    uint32_t     size;
    char         font_buffer[512];
    String       font_name;
    Glyph        character[128]; // For first 128 printable characters only
} Font;

Shader LoadShadersFromString(const char *cstr, ShaderType type)
{
    unsigned int shader_define;
    Shader       shader;
    switch (type)
    {
    case FRAGMENT_SHADER:
        shader_define = GL_FRAGMENT_SHADER;
        break;
    case VERTEX_SHADER:
        shader_define = GL_VERTEX_SHADER;
        break;
    case GEOMETRY_SHADER:
        shader_define = GL_GEOMETRY_SHADER;
        // default:
        // TriggerBreakpoint();
    }
    shader.shader = glCreateShader(shader_define);
    shader.type   = type;
    glShaderSource(shader.shader, 1, (const char *const *)&cstr, NULL);
    glCompileShader(shader.shader);
    int compiled;
    glGetShaderiv(shader.shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader.shader, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile %s", ShaderTypeName(shader.type));
        fprintf(stderr, "Reason -> %s.", infoLog);
    }
    else
        fprintf(stderr, "\n%s compilation passed.\n", ShaderTypeName(shader.type));
    return shader;
}

uint32_t screen_width = 800, screen_height = 600;

String   ReadFiles(const char *file_path)
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

void LoadFont(Font *font, const char *font_dir)
{
    memset(font, 0, sizeof(*font));
    stbtt_fontinfo sfont;
    String         fontbuffer = ReadFiles(font_dir);
    if (!fontbuffer.data)
    {
        fprintf(stderr, "\nError : Failed to load %s.", font_dir);
        return;
    }

    stbtt_InitFont(&sfont, fontbuffer.data, 0);

    // Load the character's data from stb_truetype
    float font_size = 40;
    float scale     = stbtt_ScaleForPixelHeight(&sfont, font_size);
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

        // int stride = ((int)xpos + x0) + width * (above + y0); //  * (baseline + y0) + (int)xpos + x0;
        int stride = (int)xpos + x0 + width * (baseline + y0);

        stbtt_MakeCodepointBitmapSubpixel(&sfont, bmpbuffers + stride, x1 - x0, y1 - y0, width, scale, scale, x_shift,
                                          0, ch);

        Glyph *glyph   = &font->character[ch];
        glyph->offset  = (Pos2D){(int)xpos, 0};
        glyph->size    = (Pos2D){x1 - x0, y1 - y0};
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
    font->size  = font_size;
    // Font shaders and program
    Shader font_vertex = LoadShadersFromString(
        "#version 330 core \r\n\r\nlayout (location = 0) in vec2 aPos; \r\nlayout (location = 1) in vec2 Tex;\r\n// "
        "might need a matrix somewhere here \r\n\r\nout vec2 TexCoord; \r\nuniform mat4 scene; \r\n\r\nvoid "
        "main()\r\n{\r\n\tgl_Position = scene * vec4(aPos,0.0f,1.0f);\r\n\tTexCoord = Tex; \r\n}",
        VERTEX_SHADER);

    Shader font_fragment =
        LoadShadersFromString("#version 330 core\r\n\r\nout vec4 color_vec; \r\n\r\nin vec2 TexCoord; \r\nuniform "
                              "sampler2D font;\r\n\r\n\r\nvoid main()\r\n{\r\n\tvec4 color = "
                              "texture(font,TexCoord);\r\n\tcolor_vec = vec4(0.0f,0.0f,0.0f,color.r);\r\n}",
                              FRAGMENT_SHADER);

    font->program = LoadProgram(font_vertex, font_fragment);
}

// only font name .. not the whole path
void LoadSystemFont(Font *font, const char *font_name)
{
    char font_path[512] = {0};

#ifdef _WIN32
    strcpy(font_path, getenv("WINDIR"));
    strcat(font_path, "\\Fonts\\");
    strcat(font_path, font_name);

#elif defined(__linux__)
    char shell_command[512] = "fc-match --format=%{file} ";
    strcat(shell_command, font_name);
    FILE *sh = popen(shell_command, "r");
    if (!sh)
    {
        fprintf(stderr, "Failed to load font %s.", font_name);
        return;
    }
    // No safety checks
    fread(font_path, sizeof(unsigned char), 512, sh);
    pclose(sh);
#endif
    LoadFont(font, font_path);
}

void FillText(GPUBatch *font_data, Font *font, Pos2D position, String str, uint16_t *advancement, float scale)
{
    // its quite straightforward
    int32_t x = position.x;
    int32_t y = position.y;

    float   tex0, tex1;

    for (uint32_t i = 0; i < str.length; ++i)
    {
        Glyph glyph      = font->character[(size_t)str.data[i]];
        int   w          = glyph.Advance;
        advancement[i]   = w;

        int h            = font->height;

        tex0             = glyph.offset.x / font->width;
        tex1             = (glyph.offset.x + glyph.Advance) / font->width;

        Pos2D vertices[] = {{x, y},     {tex0, 1.0f}, {x, y + h},     {tex0, 0.0f}, {x + w, y + h}, {tex1, 0.0f},
                            {x + w, y}, {tex1, 1.0f}, {x + w, y + h}, {tex1, 0.0f}, {x, y},         {tex0, 1.0f}};

        assert(font_data->vertex_buffer.count + sizeof(vertices) < font_data->vertex_buffer.max);
        memcpy(font_data->vertex_buffer.data + font_data->vertex_buffer.count, vertices, sizeof(vertices));

        x                              = x + glyph.Advance;
        font_data->vertex_buffer.count = font_data->vertex_buffer.count + sizeof(vertices);
    }
}

// void RenderFont(Panel *panel, Font *font, Mat4 *ortho_transform)
//{
//     glUseProgram(font->program);
//     glUniformMatrix4fv(glGetUniformLocation(font->program, "scene"), 1, GL_TRUE, &ortho_transform->elem[0][0]);
//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, font->font_texture);
//
//     PrepareBatch(panel->render.batch);
//     DrawBatch(panel->render.batch, panel->render.batch->vertex_buffer.count / 6);
//     glUseProgram(0);
// }

GPUBatch *CreateNewBatch(Primitives primitive)
{
    GPUBatch *batch          = malloc(sizeof(*batch));
    batch->vertex_buffer.max = 25000;
    batch->primitive         = primitive;

    glGenVertexArrays(1, &batch->vao);
    glGenBuffers(1, &batch->vertex_buffer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, batch->vertex_buffer.max * sizeof(*batch->vertex_buffer.data), NULL, GL_STATIC_DRAW);

    batch->vertex_buffer.max   = 25000;
    batch->vertex_buffer.count = 0;
    batch->vertex_buffer.dirty = true;
    batch->vertex_buffer.data  = malloc(sizeof(uint8_t) * batch->vertex_buffer.max);

    return batch;
}

void InitPanel(Panel *panel)
{
}

Panel *CreatePanel()
{
    Panel *panel = malloc(sizeof(*panel));
    memset(panel, 0, sizeof(*panel));

    panel->dimension            = (Pos2D){350, screen_height};
    panel->origin               = (Pos2D){0, screen_height};

    panel->panel.history_count  = 1;
    panel->panel.history[0].len = 0;
    panel->panel.history[0].pos = (Pos2D){0, 500};
    panel->panel.active_panel   = 0;

    panel->layout.box_count     = 1;
    panel->layout.box_gap       = 75;
    panel->layout.active_box    = 0;
    // No optimization for now
    panel->render.font_batch                         = CreateNewBatch(GL_TRIANGLES);
    panel->render.batch                              = CreateNewBatch(GL_TRIANGLES);

    panel->render.batch->vertex_buffer.dirty         = true;
    panel->render.updated                            = true;

    panel->render.Anim.last_time                     = 0.0f;
    panel->render.Anim.should_run                    = false;
    panel->render.Anim.t                             = 0;
    *(float *)&panel->render.Anim.time_constant      = 0.25f; // casting away the constness

    panel->render.CaretAnim.started                  = 0.0f;
    panel->render.CaretAnim.should_animate           = false;
    panel->render.CaretAnim.t                        = 0;
    *(float *)&panel->render.CaretAnim.time_constant = 0.05f; // casting away the constness

    panel->render.local_transform                    = IdentityMatrix();
    // TODO :: Create and attach a vao
    return panel;
}

void DrawBatch(GPUBatch *batch, uint32_t counts)
{
    glBindVertexArray(batch->vao);
    glDrawArrays(batch->primitive, 0, counts);
}

void PrepareFontBatch(GPUBatch *batch)
{
    if (batch->vertex_buffer.dirty)
    {
        glBindVertexArray(batch->vao);
        glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.vbo);
        void *access = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        // This is redundant
        memcpy(access, batch->vertex_buffer.data, sizeof(uint8_t) * batch->vertex_buffer.count);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void *)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        batch->vertex_buffer.dirty = false;
    }
}

void PrepareVertexBatch(GPUBatch *batch)
{
    if (batch->vertex_buffer.dirty)
    {
        glBindVertexArray(batch->vao);
        glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.vbo);
        void *access = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        // This is redundant
        memcpy(access, batch->vertex_buffer.data, sizeof(uint8_t) * batch->vertex_buffer.count);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), NULL);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void *)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        batch->vertex_buffer.dirty = false;
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

float AnimateCaret(Panel *panel, float old_pos, float new_pos)
{
    panel->render.CaretAnim.should_animate = true;
    panel->render.CaretAnim.started        = glfwGetTime();
    panel->render.CaretAnim.origin         = old_pos;
    panel->render.CaretAnim.target         = new_pos;
}

float GetCaretPos(Panel *panel)
{
    float offset = 0;

    for (uint32_t i = panel->panel.history[panel->panel.active_panel].renderdata.visible_start;
         i < panel->panel.history[panel->panel.active_panel].caret_pos; ++i)
    {
        offset = offset + panel->panel.history[panel->panel.active_panel].renderdata.advancement[i];
    }
    return offset;
}

void UpdatePanel(Panel *panel)
{
    if (panel->render.updated)
    {
        if (panel->render.Anim.should_run)
        {
            // Just translate in the horizontal direction
            float now            = glfwGetTime(); // time in seconds
            panel->render.Anim.t = (now - panel->render.Anim.last_time) / panel->render.Anim.time_constant;

            if (panel->render.Anim.t >= 1.0f)
            {
                panel->render.Anim.should_run = false;
                panel->render.Anim.t          = 1.0f;
            }
            panel->render.local_transform =
                TranslationMatrix((panel->render.Anim.t - 1) * panel->dimension.x, 0.0f, 0.0f);
        }

        // Emit triangles first to draw the frame
        GPUBatch *batch            = panel->render.batch;
        batch->primitive           = TRIANGLES;

        batch->vertex_buffer.count = 0;

        // Render everything as a triangle
        float box[]           = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f};

        float default_color[] = {0.8f, 0.8f, 0.8f};
        panel->render.updated = false;

        for (uint32_t corner = 0; corner < 6; ++corner)
        {
            float vertex[] = {panel->origin.x + box[corner * 2] * panel->dimension.x,
                              panel->origin.y + box[corner * 2 + 1] * panel->dimension.y};
            memcpy(batch->vertex_buffer.data + batch->vertex_buffer.count, vertex, sizeof(vertex));
            batch->vertex_buffer.count += sizeof(vertex);
            memcpy(batch->vertex_buffer.data + batch->vertex_buffer.count, default_color, sizeof(default_color));
            batch->vertex_buffer.count += sizeof(default_color);
        }
        // How many box to draw?

        default_color[0] = 1.0f;
        default_color[2] = 0.7f;

        float colors[]   = {0.95f, 0.95f, 0.95f, 0.0f, 0.75f, 0.0f, 0.0f, 0.0f, 0.75f,
                          1.0f,  1.0f,  0.0f,  0.0f, 1.0f,  1.0f, 1.0f, 0.0f, 1.0f};

        // use transformation relative to the parent origin
        float transform_vec[] = {panel->origin.x, panel->origin.y, 0.0f, 1.0f};
        MatrixVectorMultiply(&panel->render.local_transform, transform_vec);

        for (uint32_t box_c = 0; box_c < panel->layout.box_count; box_c++)
        {
            for (uint32_t corner = 0; corner < 6; ++corner)
            {
                float vertex[] = {transform_vec[0] + box[corner * 2] * panel->dimension.x,
                                  transform_vec[1] - box_c * panel->layout.box_gap +
                                      box[corner * 2 + 1] * panel->layout.box_gap};
                memcpy(batch->vertex_buffer.data + batch->vertex_buffer.count, vertex, sizeof(vertex));
                batch->vertex_buffer.count += sizeof(vertex);
                memcpy(batch->vertex_buffer.data + batch->vertex_buffer.count, colors + box_c * 3,
                       sizeof(default_color));
                batch->vertex_buffer.count += sizeof(default_color);
            }
        }

        batch->vertex_buffer.dirty = true;
        // Render caret
        bool  render_caret = true;

        float offset       = GetCaretPos(panel);

        if (panel->render.CaretAnim.should_animate)
        {
            float time = glfwGetTime();
            float t    = (time - panel->render.CaretAnim.started) / (panel->render.CaretAnim.time_constant);
            if (t < 1.0f)
            {
                offset = panel->render.CaretAnim.origin + t * (offset - panel->render.CaretAnim.origin);
                panel->render.updated = true;
            }
            else
                panel->render.CaretAnim.should_animate = false;
        }

        uint32_t c_thickness   = 3.0f;
        uint32_t c_offset      = (panel->layout.box_gap - panel->render.font->size) / 2.0f;
        Pos2D    caret         = {.x = transform_vec[0] + offset,
                                  .y = transform_vec[1] - c_offset - panel->layout.active_box * panel->layout.box_gap};

        float    caret_color[] = {1.00f, 0.10f, 0.75f, 0.0f};
        // blink the caret
        if (render_caret)
        {
            // Get the active box and render a caret there
            // Assuming its the first box thats active now
            // Just render a thick line nothing more
            for (uint32_t corner = 0; corner < 6; ++corner)
            {
                float vertex[] = {caret.x + box[corner * 2] * c_thickness,
                                  caret.y + box[corner * 2 + 1] * panel->render.font->size}; // caret length

                memcpy(batch->vertex_buffer.data + batch->vertex_buffer.count, vertex, sizeof(vertex));
                batch->vertex_buffer.count += sizeof(vertex);
                memcpy(batch->vertex_buffer.data + batch->vertex_buffer.count, caret_color, sizeof(default_color));
                batch->vertex_buffer.count += sizeof(default_color);
            }
        }
        PrepareVertexBatch(batch);
    }

    if (panel->render.font_batch->vertex_buffer.dirty)
    {
    }
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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);

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

    return window;
}
void RenderText(uint32_t font_program, Mat4 *transform, Panel *panel, uint32_t font_texture);

void RenderPanel(Panel *panel, Font *font, Mat4 *ortho)
{
    // Enable scissor
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, panel->dimension.x, panel->dimension.y);
    // Fill the text and starts rendering
    TextPanel *active_panel = &panel->panel.history[panel->panel.active_panel];

    float      vec[]        = {panel->origin.x, panel->origin.y, 0.0f, 1.0f};
    MatrixVectorMultiply(&panel->render.local_transform, vec);

    // Determine if the text panel needs to be slided back and forth ?
    // For that, calculate the advancement from current visible start to end of the text stream
    // if this is greater than the size of the panel, update the visible start, retarget the caret
    float width = GetCaretPos(panel);

    if ((width + 1.25f * panel->render.font->size) > panel->dimension.x)
    {
        // Increase the visible start by 1
        active_panel->renderdata.visible_start += 1;
        panel->render.updated = true;
        // caret pos is fine
    }
    if ((width + 2.0f * panel->render.font->size) < panel->dimension.x)
    {
        if (active_panel->renderdata.visible_start != 0)
            active_panel->renderdata.visible_start -= 1;
        panel->render.updated = true;
    }

    FillText(panel->render.font_batch, font,
             (Pos2D){vec[0], vec[1] - (panel->layout.box_gap + panel->render.font->size) / 2.0f},
             (String){.data   = active_panel->buffer + active_panel->renderdata.visible_start,
                      .length = active_panel->len - active_panel->renderdata.visible_start},
             active_panel->renderdata.advancement + active_panel->renderdata.visible_start, 1.0f);

    UpdatePanel(panel);
    DrawBatch(panel->render.batch, 30);

    panel->render.font_batch->vertex_buffer.dirty = true;
    PrepareFontBatch(panel->render.font_batch);
    RenderText(panel->render.font->program, ortho, panel, panel->render.font->font_texture);
    glDisable(GL_SCISSOR_TEST);
}

void RenderText(uint32_t font_program, Mat4 *transform, Panel *panel, uint32_t font_texture)
{
    glUseProgram(font_program);
    glUniformMatrix4fv(glGetUniformLocation(font_program, "scene"), 1, GL_TRUE, &transform->elem[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    // DrawBatch(panel->render.font_batch, panel->render.font_batch->vertex_buffer.count / 4);
    uint32_t draw_count = panel->panel.history[panel->panel.active_panel].len -
                          panel->panel.history[panel->panel.active_panel].renderdata.visible_start;
    DrawBatch(panel->render.font_batch, draw_count * 6);
}

Shader LoadShader(const char *shader_path, ShaderType type)
{
    String       str = ReadFiles(shader_path);
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
    case GEOMETRY_SHADER:
        shader_define = GL_GEOMETRY_SHADER;
    default:
        __assume(false);
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
        fprintf(stderr, "Failed to compile %s", ShaderTypeName(type));
        fprintf(stderr, "Reason -> %s.", infoLog);
    }
    else
        fprintf(stderr, "\n%s compilation passed.\n", ShaderTypeName(type));
    return shader;
}

void HandleArrows(Panel *panel, uint32_t arrow)
{
    uint32_t *pos         = &panel->panel.history[panel->panel.active_panel].caret_pos;
    uint32_t *visible     = &panel->panel.history[panel->panel.active_panel].renderdata.visible_start;
    panel->render.updated = true;
    switch (arrow)
    {
    case GLFW_KEY_LEFT:
    {
        (*pos > 0) ? (*pos)-- : 0;
        if ((*pos == *visible) && *pos != 0)
            (*visible)--;
        break;
    }
    case GLFW_KEY_RIGHT:
    {
        uint32_t len = panel->panel.history[panel->panel.active_panel].len;
        (*pos < len) ? (*pos)++ : len;
        break;
    }
    }
}

void HandleEvents(GLFWwindow *window, Panel *panel)
{
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
    {
        panel->render.Anim.should_run = true;
        panel->render.Anim.last_time  = glfwGetTime();
        panel->render.Anim.t          = 0.0f;
    }
}

typedef struct
{
    Mat4  *OrthoMatrix;
    Panel *Panel;
} UserData;

void FrameChangeCallback(GLFWwindow *window, int width, int height)
{
    screen_width  = width;
    screen_height = height;
    glViewport(0, 0, width, height);
    UserData *data              = (UserData *)glfwGetWindowUserPointer(window);
    *data->OrthoMatrix          = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);
    data->Panel->dimension.x    = 0.30 * width;
    data->Panel->dimension.y    = height;
    data->Panel->origin.y       = height;
    data->Panel->origin.x       = 0;
    data->Panel->render.updated = true;
}

void RemoveCharAtIndex(char *string, uint32_t len, uint32_t index)
{
    // There might be overlapping memories, so use manual copying instead of memcpy()
    for (uint32_t i = index; i < len; ++i)
        string[i] = string[i + 1];
    string[len - 1] = '\0';
}

void InsertCharAtIndex(char *str, uint32_t len, uint32_t index, char c)
{
    // start from the last
    for (int32_t i = len; i > index; --i)
        str[i] = str[i - 1];
    str[index] = c;
}

static void CharCallback(GLFWwindow *window, unsigned int codepoint)
{
    UserData     *data         = glfwGetWindowUserPointer(window);
    PanelHistory *history      = &data->Panel->panel;
    TextPanel    *active_panel = &history->history[history->active_panel];

    InsertCharAtIndex(active_panel->buffer, active_panel->len, active_panel->caret_pos, (char)codepoint);
    // Insert glyph here too
    data->Panel->render.updated = true;

    // if (enable_caret_animation)
    // enable animation for caret

    active_panel->caret_pos = active_panel->caret_pos + 1;
    active_panel->len       = active_panel->len + 1;
}

void        HandleArrows(Panel *panel, uint32_t arrows);

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // if ((key >= GLFW_KEY_A && key <= GLFW_KEY_Z) && action == GLFW_PRESS)
    //{
    UserData     *data         = glfwGetWindowUserPointer(window);
    PanelHistory *history      = &data->Panel->panel;

    TextPanel    *active_panel = &history->history[history->active_panel];
    bool          both_action  = action & (GLFW_PRESS | GLFW_REPEAT);
    float         old          = GetCaretPos(data->Panel);
    if (key == GLFW_KEY_BACKSPACE && both_action)
    {

        if (active_panel->len && active_panel->caret_pos)
        {
            // The caret will be drawn right before the character to be drawn
            // So, backspacing at carat 1 should erase 0
            RemoveCharAtIndex(active_panel->buffer, active_panel->len, active_panel->caret_pos - 1);
            active_panel->caret_pos--;
            active_panel->len--;
            data->Panel->render.updated = true;
        }
    }
    if (both_action)
        HandleArrows(data->Panel, key);
    AnimateCaret(data->Panel, old, GetCaretPos(data->Panel));
}

int main(int argc, char **argv)
{
    GLFWwindow *window = LoadGLFW(screen_width, screen_height, "Interactive Panel Creation");
    glfwShowWindow(window);
    glfwMakeContextCurrent(window);

    Panel *panel       = CreatePanel();
    Shader vertex      = LoadShader("./src/shader/common_2D.vs", VERTEX_SHADER);
    Shader fragment    = LoadShader("./src/shader/common_2D.fs", FRAGMENT_SHADER);

    panel->render.font = malloc(sizeof(Font));
    // LoadFont(ComicSans, "./include/comic.ttf");
    LoadSystemFont(panel->render.font, "comic.ttf");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    uint32_t program = LoadProgram(vertex, fragment);

    glUseProgram(program);

    Mat4     identity = IdentityMatrix();
    Mat4     ortho    = OrthographicProjection(0, 800, 0, 600, -1.0f, 1.0f);

    UserData data     = {.OrthoMatrix = &ortho, .Panel = panel};
    glfwSetWindowUserPointer(window, &data);

    glfwSetFramebufferSizeCallback(window, FrameChangeCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCharCallback(window, CharCallback);

    while (!glfwWindowShouldClose(window))
    {
        glUseProgram(program);
        identity = MatrixMultiply(&ortho, &panel->render.local_transform);
        glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_TRUE, (GLfloat *)&identity.elem[0][0]);
        glClearColor(0.70f, 0.70f, 0.70f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        RenderPanel(panel, panel->render.font, &ortho);
        HandleEvents(window, panel);
        panel->render.font_batch->vertex_buffer.count = 0;
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
