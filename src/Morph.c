#define _CRT_SECURE_NO_WARNINGS
#define _GNU_SOURCE

#include "./render_common.h"

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <time.h>
#include <unistd.h>
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include "./stb_truetype.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./parser.h"

// For exporting to bmp
#include "../utility/bmp.h"

// TODO :: Minimize floating point errors
// TODO :: Allow customization

#if defined(__GNUC__)
#define Unreachable() __builtin_unreachable()
#elif defined(_MSC_VER)
#define Unreachable() __assume(false);
#endif

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

Mat4 OrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar)
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

Mat4 ScalarMatrix(float x, float y, float z)
{
    Mat4 matrix       = {{0}};
    matrix.elem[0][0] = x;
    matrix.elem[1][1] = y;
    matrix.elem[2][2] = z;
    matrix.elem[3][3] = 1.0f;
    return matrix;
}

Mat4 TranslationMatrix(float x, float y, float z)
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

Mat4 TransposeMatrix(Mat4 *mat)
{
    Mat4 matrix = {{0}};
    for (uint8_t row = 0; row < 4; ++row)
        for (uint8_t col = 0; col < 4; ++col)
            matrix.elem[row][col] = mat->elem[col][row];
    return matrix;
}

Mat4 Inverse(Mat4 *mat)
{
    Mat4   matrix = {{0}};
    float *inv    = &matrix.elem[0][0];
    float *m      = &mat->elem[0][0];

    double det;
    int    i;

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
             m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
             m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
             m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
              m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
             m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
             m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
             m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
              m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
             m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
              m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
              m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        inv[i] = inv[i] * det;
    return matrix;
}

Mat4 MatrixMultiply(Mat4 *mat1, Mat4 *mat2)
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

Mat4 InverseMatrix(Mat4 *mat)
{
    Mat4   inverse = {0};
    float  det;
    float *inv = &inverse.elem[0][0];
    float *m   = &mat->elem[0][0];
    int    i;

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
             m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
             m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
             m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
              m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
             m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
             m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
             m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
              m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
             m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
              m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
              m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        inv[i] = inv[i] * det;
    return inverse;
}

void MatrixVectorMultiply(Mat4 *mat, float vec[4])
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

Mat4 IdentityMatrix()
{
    Mat4 matrix       = {{0}};
    matrix.elem[0][0] = 1.0f;
    matrix.elem[1][1] = 1.0f;
    matrix.elem[2][2] = 1.0f;
    matrix.elem[3][3] = 1.0f;
    return matrix;
}

static int screen_width  = 800;
static int screen_height = 600;

struct Graph
{
    unsigned int vao;
    unsigned int vbo;
    unsigned int program;

    float        width;
    float        value;

    MVec2        center;
    MVec2        scale;

    MVec2        slide_scale; // Controls the major scaling on the axes of the graph
};

typedef struct
{
    Mat4   *OrthoMatrix;
    Graph  *graph;
    Scene  *scene;
    Mat4   *scale_transform;
    Mat4   *translation;
    Panel  *panel;
    Parser *parser; // hello to parser
    Mat4   *new_transform;
} UserData;

const char *ShaderTypeName(ShaderType shader)
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
        Unreachable();
    }
}
void ErrorCallback(int code, const char *description)
{
    fprintf(stderr, "\nGLFW Error -> %s.\n", description);
}

void Plot1DFromComputationContext(Scene *scene, ComputationContext *context, Graph *graph, MVec3 color,
                                  const char *legend);

struct
{
    GLuint fbo, tex;
} AlternateFrameBuffer;
unsigned int active_fbo = 0;

void         FrameChangeCallback(GLFWwindow *window, int width, int height)
{
    screen_width  = width;
    screen_height = height;
    glViewport(0, 0, width, height);
    UserData *data     = glfwGetWindowUserPointer(window);
    *data->OrthoMatrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);

    PanelFrameChangeCallback(data->panel, width, height);
}

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_S && mod == GLFW_MOD_CONTROL && action == GLFW_PRESS)
    {
        fprintf(stderr, "\nAttempting to take a screenshot ... ");
        // Take screenshot from the default buffer by blitting it into the alternate buffer

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, AlternateFrameBuffer.fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, screen_width, screen_height, 0, 0, 1080, 720, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, AlternateFrameBuffer.fbo);

        const uint32_t channels = 3;
        uint8_t       *buffer   = malloc(sizeof(uint8_t) * 1080 * 720 * channels);
        glReadPixels(0, 0, 1080, 720, GL_RGB, GL_UNSIGNED_BYTE, buffer);

        // Export to bmp using the handcrafted library
        const uint32_t out_width    = 1080;
        const uint32_t out_height   = 720;
        const char    *out_filename = "screenshot.bmp";

        const uint64_t size_required =
            out_width * out_height * channels +
            10000; // Extra bytes for padding, can be precisely calculated but omitted for now
        {
            BMP bmp = {0};
            InitBMP(&bmp, size_required, channels, false);
            WriteBMPHeader(&bmp);
            WriteBMPData(&bmp, buffer, out_width, out_height, channels);
            WriteBMPToFile(&bmp, out_filename);
            DestroyBMP(&bmp);
        }
        free(buffer);
        return;
    }
    if (key == GLFW_KEY_F && mod == GLFW_MOD_CONTROL && action == GLFW_PRESS)
    {
        if (active_fbo == 0)
            active_fbo = AlternateFrameBuffer.fbo;
        else
            active_fbo = 0;
        return;
    }

    UserData *data = glfwGetWindowUserPointer(window);
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
    {
        // Parse and evaluate the function
        PanelHistory *history      = &data->panel->panel;
        TextPanel    *active_panel = &history->history[history->active_panel];
        UpdateParserData(data->parser, active_panel->buffer, active_panel->len);

        // Get the execution computation context and plot the function
        ParseStart(data->parser);
        // Return the function currently parsed.

        SymbolFn           *fn      = GetLatestParsedFn();
        ComputationContext *context = NewComputation(fn);

        float               rands[3];
        for (uint32_t i = 0; i < 3; ++i)
            rands[i] = (rand() % 100) / 100.0f;

        Plot1DFromComputationContext(data->scene, context, data->graph, *(MVec3 *)(rands), "Plotted from context");
        DestroyComputationContext(context);
    }
    PanelKeyCallback(data->panel, key, scancode, action, mod);
    // TODO :: Update the orthographic projection for that seamless transition and update the scissor window
}

static void CharCallback(GLFWwindow *window, unsigned int codepoint)
{
    UserData *data = glfwGetWindowUserPointer(window);
    PanelCharCallback(data->panel, codepoint);
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
    // L1:
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

struct ScaleTransition
{
    bool  should_animate;
    float g_init, g_term;

    float s_init, s_term;
    float sc_init, sc_term;
    float start;
    float duration_constant;

    bool  offset_changed;
    float offset;
} scroll_animation;

void AnimateScrolling(Graph *graph, Mat4 *scale_transform)
{
}

void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    const float origin = 200.0f;
    const float scale  = 5.0f;

    UserData   *data   = glfwGetWindowUserPointer(window);
    Graph      *graph  = data->graph;

    // capture mouse co-ordinates here
    double xPos, yPos;
    double X, Y;
    glfwGetCursorPos(window, &X, &Y);

    // shift the origin somewhere far from here
    // scroll_animation.s_init = graph->slide_scale.x;

    graph->slide_scale.y += scale * yoffset;
    graph->slide_scale.x += scale * yoffset;
    // adjust the scaling of the graph

    // Dynamic scaling looks kinda hard
    static int absScale = 0;

    bool       changeX = false, changeY = false;
    float      s = 1.0f;
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

    // Enters the transition phase
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
        graph->slide_scale = (MVec2){origin, origin};
    }

    // Mat4 *scale_transform           = data->scale_transform;
    // s                               = graph->slide_scale.x / graph->scale.x;
    //*scale_transform                = ScalarMatrix(s, s, 1.0f);

    // scroll_animation.should_animate = true;
    // scroll_animation.start          = glfwGetTime();
    // scroll_animation.g_term         = graph->slide_scale.x / graph->scale.x;
    // scroll_animation.sc_term        = graph->scale.x;
    // scroll_animation.s_term         = graph->slide_scale.x;
    float sx    = graph->slide_scale.x / graph->scale.x;

    yPos        = screen_height - Y;
    X           = X - scroll_animation.offset;
    float vec[] = {X, yPos, 0.0f, 1.0f};
    fprintf(stderr, "Old pos : (%5g, %5g).\n", vec[0], vec[1]);
    Mat4 inverse = InverseMatrix(data->new_transform);
    MatrixVectorMultiply(&inverse, vec);

    fprintf(stderr, "xPos : %5g and yPos : %5g\n", vec[0], vec[1]);

    *data->new_transform = IdentityMatrix();
    Mat4 translation     = TranslationMatrix(-vec[0], -vec[1], 0.0f);
    Mat4 scalar          = ScalarMatrix(sx / 100.0f, sx / 100.0f, 1.0f);
    *data->new_transform = MatrixMultiply(&scalar, &translation);
    translation          = TranslationMatrix(vec[0], vec[1], 0.0f);
    *data->new_transform = MatrixMultiply(&translation, data->new_transform);

    scalar               = ScalarMatrix(100, 100, 1.0f);
    *data->new_transform = MatrixMultiply(&scalar, data->new_transform);
    *data->new_transform = MatrixMultiply(data->translation, data->new_transform);

    // Add a translation vector that will transform the previous position exactly to the new position
    // Vec[0] and vec[1] are the fixed points, so translate the scene accordingly
    float newpos[] = {vec[0], vec[1], 0.0f, 1.0f};
    MatrixVectorMultiply(data->new_transform, newpos);
    fprintf(stdout, "Newposition are : (%5g,%5g).\n", newpos[0], newpos[1]);

    Mat4 newtranslation  = TranslationMatrix(X - newpos[0], (screen_height - Y) - newpos[1], 0.0f);
    *data->new_transform = MatrixMultiply(&newtranslation, data->new_transform);
    newpos[0]            = vec[0];
    newpos[1]            = vec[1];
    MatrixVectorMultiply(data->new_transform, newpos);
    fprintf(stdout, "Newposition after fixing are : (%5g,%5g).\n", newpos[0], newpos[1]);

    fprintf(stdout, "SX : %5g.\n", sx);
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

unsigned int LoadProgram3(Shader vertex, Shader fragment, Shader geometry)
{
    if (vertex.type != VERTEX_SHADER && fragment.type != FRAGMENT_SHADER && geometry.type != GEOMETRY_SHADER)
    {
        fprintf(stderr, "Shaders mismatched for program\n");
        return -1;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex.shader);
    glAttachShader(program, fragment.shader);
    glAttachShader(program, geometry.shader);

    glLinkProgram(program);
    glHint(GL_LINE_SMOOTH, GL_NICEST);

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

String ReadFiles(const char *file_path)
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

Shader LoadShadersFromString(const char *cstr, ShaderType type);
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
        Unreachable();
    }
    shader.shader = glCreateShader(shader_define);
    shader.type   = type;
    glShaderSource(shader.shader, 1, (const char *const *)&str.data, NULL);
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
    glfwSetCharCallback(window, CharCallback);
    return window;
}

typedef struct FunctionPlotData
{
    bool          updated;

    FunctionType  fn_type;
    void         *function;
    GPUBatch     *batch;
    uint32_t      max;
    uint32_t      count;
    MVec3         color;
    VertexData2D *samples;
    char          plot_name[25];
} FunctionPlotData;

typedef struct FontData
{
    bool      updated;
    GPUBatch *batch;
    uint32_t  max;
    MVec3     color;
    uint32_t  count;
    MVec2    *data;
} FontData;

typedef struct PlotArray
{
    uint32_t          max;
    uint32_t          count;
    int32_t           current_selection;
    FunctionPlotData *functions;
} PlotArray;

typedef struct FontArray
{
    uint32_t  max;
    uint32_t  count;
    FontData *fonts;
} FontArray;

typedef struct Scene
{
    PlotArray plots;
    FontData  axes_labels;
    FontData  legends;
} Scene;

struct State
{
    bool   bPressed;
    double xpos;
    double ypos;
};

GPUBatch *CreateNewBatch(Primitives primitive)
{
    GPUBatch *batch          = malloc(sizeof(*batch));
    batch->vertex_buffer.max = 35000;
    batch->primitive         = primitive;

    glGenVertexArrays(1, &batch->vao);
    glGenBuffers(1, &batch->vertex_buffer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, batch->vertex_buffer.max * sizeof(*batch->vertex_buffer.data), NULL, GL_STATIC_DRAW);

    batch->vertex_buffer.max   = 35000;
    batch->vertex_buffer.count = 0;
    batch->vertex_buffer.dirty = true;
    batch->vertex_buffer.data  = malloc(sizeof(uint8_t) * batch->vertex_buffer.max);

    return batch;
}

void DrawBatch(GPUBatch *batch, uint32_t counts)
{
    glBindVertexArray(batch->vao);
    glDrawArrays(batch->primitive, 0, counts);
}

void PrepareBatch(GPUBatch *batch)
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

typedef MVec2 (*parametricfn)(double);

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

    // Shader vertex   = LoadShader("./include/grid_vertex.glsl", VERTEX_SHADER);
    // Shader fragment = LoadShader("./include/grid_fragment.glsl", FRAGMENT_SHADER);

    Shader vertex = LoadShadersFromString("#version 330 core \r\n\r\nlayout (location = 0) in vec2 aPos; \r\n\r\nvoid "
                                          "main() \r\n{ \r\n\tgl_Position = vec4(aPos,0.0f,1.0f);\r\n}",
                                          VERTEX_SHADER);
    Shader fragment = LoadShadersFromString(
        "#version 330 core \r\n\r\nout vec4 color; \r\n\r\n// not working with uniform buffer for now \r\n// Lets "
        "try "
        "drawing a checkerboard \r\n// uniform int scale;\r\n\r\nuniform vec2 scale;\r\nuniform int "
        "grid_width;\r\n\r\nuniform vec2 center; \r\n\r\nvoid main() \r\n{\r\n\tvec2 scr = "
        "gl_FragCoord.xy;\r\n\tint "
        "delX = abs(int(scr.x-center.x)); \r\n\tint delY = abs(int(scr.y-center.y));\r\n\t\r\n\tint X = "
        "int(scale.x); "
        "\r\n\tif (X % 2 != 0) \r\n\t\tX = X + 1; \r\n\r\n\tint Y = X;\r\n\r\n\tint halfX = X / 2; \r\n\tint halfY "
        "= "
        "halfX; \r\n\r\n\t// TODO :: Rewrite it in branchless way \r\n\tif ( (delX % halfX <= grid_width) || (delY "
        "% "
        "halfY <= grid_width))\r\n\t\tcolor = vec4(0.0f,0.7f,0.7f,1.0f); \r\n\telse\r\n\t\tcolor = "
        "vec4(1.0f,1.0f,1.0f,1.0f);\r\n\t\t\r\n\tif ( (delX % X <= grid_width+2) || (delY % Y <= "
        "grid_width+2))\r\n\t\tcolor = vec4(0.5f,0.5f,0.5f,1.0f);\r\n\r\n\r\n\tif (abs(scr.x - center.x) < 3.0f) "
        "\r\n\t\tcolor = vec4(1.0f,0.0f,0.0f,1.0f);\r\n\tif (abs(scr.y - center.y) < 3.0f) \r\n\t\tcolor = "
        "vec4(1.0f,0.0f,0.0f,1.0f);\r\n}",
        FRAGMENT_SHADER);

    graph->program = LoadProgram(vertex, fragment);
    graph->center  = (MVec2){0.0f, 0.0f};

    // Make graph->scale use value instead of pixel scale
    graph->scale       = (MVec2){1.0f, 1.0f};
    graph->slide_scale = (MVec2){200.0f, 200.0f};
}

void RenderGraph(Graph *graph, Mat4 *transform, float X, float Y)
{
    glUseProgram(graph->program);
    glBindVertexArray(graph->vao);
    glUniform1i(glGetUniformLocation(graph->program, "grid_width"), 0);
    float center[4] = {graph->center.x, graph->center.y, 0.0f, 1.0f};
    MatrixVectorMultiply(transform, center);
    glUniform2f(glGetUniformLocation(graph->program, "center"), center[0], center[1]);
    glUniform2f(glGetUniformLocation(graph->program, "scale"), X, Y);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glUseProgram(0);
    glBindVertexArray(0);
}

void Destroy2DScene(Scene *scene)
{
    /*free(render_scene->Indices);
    free(render_scene->Vertices);
    free(render_scene->Discontinuity);
    free(render_scene->Points);
    free(render_scene->fVertices);*/
}

void Init2DScene(Scene *scene)
{
    memset(scene, 0, sizeof(*scene));
    // Init enough memory for 10 graphs
    scene->plots.max               = 10;
    scene->plots.functions         = malloc(sizeof(*scene->plots.functions) * scene->plots.max);
    scene->plots.current_selection = -1;

    for (uint32_t plot = 0; plot < scene->plots.max; ++plot)
    {
        /*scene->plots.functions[plot].batch = NULL;
        scene->plots.functions[plot].count = 0; */
        memset(scene->plots.functions + plot, 0, sizeof(*scene->plots.functions));
    }

    scene->axes_labels.count   = 0;
    scene->axes_labels.max     = 500000;
    scene->axes_labels.batch   = CreateNewBatch(TRIANGLES);
    scene->axes_labels.updated = true;
    scene->axes_labels.data    = malloc(sizeof(*scene->axes_labels.data) * scene->axes_labels.max);
}

void RenderScene(Scene *scene, unsigned int program, bool showPoints, Mat4 *mscene, Mat4 *transform)
{
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "scene"), 1, GL_TRUE, &mscene->elem[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_TRUE, &transform->elem[0][0]);

    for (uint32_t graph = 0; graph < scene->plots.count; ++graph)
    {
        FunctionPlotData *function = &scene->plots.functions[graph];
        glUniform3f(glGetUniformLocation(program, "inColor"), function->color.x, function->color.y, function->color.z);
        // Transfer data to the batch
        glUniform1f(glGetUniformLocation(program, "thickness"), 3.0f);
        if (scene->plots.current_selection == graph)
            glUniform1f(glGetUniformLocation(program, "thickness"), 6.0f);

        if (function->updated)
        {
            assert(function->count * 4 * 4 < function->batch->vertex_buffer.max);
            memcpy(function->batch->vertex_buffer.data, function->samples, sizeof(uint8_t) * function->count * 4 * 4);
            function->batch->vertex_buffer.count = function->count * 4 * 4;
            function->updated                    = false;
            function->batch->vertex_buffer.dirty = true;
        }

        PrepareBatch(function->batch);
        DrawBatch(function->batch, function->batch->vertex_buffer.count / (4 * 4));
    }
}

void PrepareScene(Scene *scene, Graph *graph)
{
    // for (uint32_t graph_ = 0; graph_ < scene->plots.count; ++graph_)
    //{
    //     FunctionPlotData *function = &scene->plots.functions[graph_];
    //     for (uint32_t pt = 0; pt < function->count; ++pt)
    //     {
    //         function->samples[pt].x = function->samples[pt].x * graph->slide_scale.x / graph->scale.x;
    //         function->samples[pt].y = function->samples[pt].y * graph->slide_scale.y / graph->scale.y;
    //     }
    // }
}

void ResetScene(Scene *scene_group)
{
    // No op
}

// static void PlotGraph(Scene *scene, ParametricFn1D func, Graph *graph, MVec3 color, const char *legend)
//{
//     // No check done here currently
//     const uint32_t max_verts                           = 1000;
//     scene->plots.functions[scene->plots.count].max     = max_verts; // 1000 vertices for each graph at most
//     scene->plots.functions[scene->plots.count].samples = malloc(sizeof(MVec2) * max_verts);
//
//     MVec2 vec1, vec2;
//
//     float step                 = 0.5f;
//     float init                 = -10.0f;
//     float term                 = 10.0f;
//
//     vec1.x                     = graph->center.x + init * graph->slide_scale.x / (graph->scale.x);
//     vec1.y                     = graph->center.y + func(init) * graph->slide_scale.y / (graph->scale.y);
//
//     FunctionPlotData *function = &scene->plots.functions[scene->plots.count];
//
//     for (float x = init; x <= term; x += step)
//     {
//         vec2.x = graph->center.x + (x + step) * graph->slide_scale.x / (graph->scale.x);
//         vec2.y = graph->center.y + func(x + step) * graph->slide_scale.y / (graph->scale.y);
//
//         // Excluded for now
//         // float slope = atan(fabs((MVec2.y - vec1.y) / (MVec2.x - vec1.x)));
//         assert(function->count < function->max);
//         function->samples[function->count++] = vec1;
//         vec1                                 = vec2;
//     }
//     scene->plots.count++;
// }

void Plot1D(Scene *scene, ParametricFn1D func, Graph *graph, MVec3 color, const char *legend)
{
    // No check done here currently
    const uint32_t max_verts                           = 1000;
    scene->plots.functions[scene->plots.count].max     = max_verts; // 1000 vertices for each graph at most
    scene->plots.functions[scene->plots.count].samples = malloc(sizeof(MVec2) * max_verts);

    float             init                             = -10.0f;
    float             term                             = 10.0f;
    float             step                             = 0.1f;

    FunctionPlotData *function                         = &scene->plots.functions[scene->plots.count];
    function->fn_type                                  = PARAMETRIC_1D;

    VertexData2D vec;
    vec.x                                = init;
    vec.y                                = func(vec.x);
    vec.n_x                              = 1.0f;
    vec.n_y                              = 1.0f;

    function->samples[function->count++] = vec;

    float n_x, n_y;
    for (float x = init + step; x <= term; x += step)
    {
        vec.x = x;
        vec.y = func(vec.x);
        assert(function->count < function->max);
        n_x                                        = x - function->samples[function->count - 1].x;
        n_y                                        = vec.y - function->samples[function->count - 1].y;
        function->samples[function->count - 1].n_x = n_x;
        function->samples[function->count - 1].n_y = n_y;
        function->samples[function->count++]       = vec;
    }
    function->color    = color;
    function->function = func;
    function->batch    = CreateNewBatch(LINE_STRIP);
    function->updated  = true;
    scene->plots.count++;
}

void Plot1DFromComputationContext(Scene *scene, ComputationContext *context, Graph *graph, MVec3 color,
                                  const char *legend)
{
    // No check done here currently
    const uint32_t max_verts                           = 1000;
    scene->plots.functions[scene->plots.count].max     = max_verts; // 1000 vertices for each graph at most
    scene->plots.functions[scene->plots.count].samples = malloc(sizeof(MVec2) * max_verts);

    float             init                             = -10.0f;
    float             term                             = 10.0f;
    float             step                             = 0.1f;

    FunctionPlotData *function                         = &scene->plots.functions[scene->plots.count];
    function->fn_type                                  = PARAMETRIC_1D;

    VertexData2D vec;
    vec.x                                = init;
    vec.y                                = EvalFromContext(context, vec.x, 0.0f);
    vec.n_x                              = 1.0f;
    vec.n_y                              = 1.0f;

    function->samples[function->count++] = vec;

    float n_x, n_y;
    for (float x = init + step; x <= term; x += step)
    {
        vec.x = x;
        vec.y = EvalFromContext(context, vec.x, 0.0f);
        assert(function->count < function->max);
        n_x                                        = x - function->samples[function->count - 1].x;
        n_y                                        = vec.y - function->samples[function->count - 1].y;
        function->samples[function->count - 1].n_x = n_x;
        function->samples[function->count - 1].n_y = n_y;
        function->samples[function->count++]       = vec;
    }
    function->color = color;

    // Gotta treat both function as same
    function->function = parabola;
    function->batch    = CreateNewBatch(LINE_STRIP);
    function->updated  = true;
    scene->plots.count++;
}

void PlotParametric(Scene *scene, parametricfn func, Graph *graph)
{
    /*MVec2 vec;
    for (float i = -3.141592f * 2; i <= 3.141592f * 2; i += 0.04f)
    {
        vec   = func(i);
        vec.x = graph->center.x + vec.x * graph->scale.x;
        vec.y = graph->center.y + vec.y * graph->scale.y;
        AddSingleVertex(scene_group, vec);
    }*/
}

void HandleEvents(GLFWwindow *window, State *state, Graph *graph, Mat4 *world_transform, Mat4 *scale_matrix,
                  Panel *panel, Mat4 *new_transform, Mat4 *scene_transform);

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
    float fontSize = 25;
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
        glyph->offset  = (MVec2){(int)xpos, 0};
        glyph->size    = (MVec2){x1 - x0, y1 - y0};
        glyph->Advance = (int)(advance * scale + 0.5f);
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
    font->size  = fontSize;
    // Font shaders and program
    Shader font_vertex = LoadShadersFromString(
        "#version 330 core \r\n\r\nlayout (location = 0) in vec2 aPos; \r\nlayout (location = 1) in vec2 "
        "Tex;\r\n// "
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

// position in pixel where (0,0) is the lower left corner of the screen
void FillText(FontData *font_data, Font *font, MVec2 position, String str, int scale)
{
    // its quite straightforward
    int32_t x = position.x;
    int32_t y = position.y;

    float   tex0, tex1;

    for (uint32_t i = 0; i < str.length; ++i)
    {
        assert(font_data->count + 12 < font_data->max);
        Glyph glyph      = font->character[(size_t)str.data[i]];
        int   w          = glyph.Advance;
        int   h          = font->height;

        tex0             = glyph.offset.x / font->width;
        tex1             = (glyph.offset.x + w) / font->width;

        MVec2 vertices[] = {{x, y},     {tex0, 1.0f}, {x, y + h},     {tex0, 0.0f}, {x + w, y + h}, {tex1, 0.0f},
                            {x + w, y}, {tex1, 1.0f}, {x + w, y + h}, {tex1, 0.0f}, {x, y},         {tex0, 1.0f}};

        memcpy(font_data->data + font_data->count, vertices, sizeof(vertices));

        x                = x + glyph.Advance;
        font_data->count = font_data->count + 12;
    }
}

void RenderFont(Scene *scene, Font *font, Mat4 *scene_transform)
{
    glUseProgram(font->program);
    glUniformMatrix4fv(glGetUniformLocation(font->program, "scene"), 1, GL_TRUE, &scene_transform->elem[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->font_texture);

    assert(scene->axes_labels.count * 2 * 4 < scene->axes_labels.batch->vertex_buffer.max);
    memcpy(scene->axes_labels.batch->vertex_buffer.data, scene->axes_labels.data,
           sizeof(uint8_t) * scene->axes_labels.count * 2 * 4);
    scene->axes_labels.batch->vertex_buffer.count = scene->axes_labels.count * 2 * 4;
    scene->axes_labels.batch->vertex_buffer.dirty = true;

    PrepareBatch(scene->axes_labels.batch);
    DrawBatch(scene->axes_labels.batch, scene->axes_labels.count / (2));
}

void RenderLabels(Scene *scene, Font *font, Graph *graph, Mat4 *combined_matrix)
{
    // These should be fine recalculating per frame

    scene->axes_labels.count = 0;
    float vec[]              = {0.0f, 0.0f, 0.0f, 1.0f};
    MatrixVectorMultiply(combined_matrix, vec);

    MVec2 origin = (MVec2){vec[0], vec[1]};
    MVec2 position;

    // Calculate the min and max vertical bar visible on the current frame first
    int xLow  = -origin.x / graph->slide_scale.x - 1;
    int xHigh = (screen_width - origin.x) / graph->slide_scale.x + 1;

    for (int i = xLow * 2; i <= xHigh * 2; ++i)
    {
        position.x = origin.x + i * graph->slide_scale.x / 2 - font->height / 2;
        position.y = origin.y - font->height;
        // Now calculate the value at the position

        // Clamp the value so that they remain inside the screen
        if (position.y < 0)
            position.y = font->height / 2;
        else if (position.y > screen_height)
            position.y = screen_height - font->height;

        float val   = i * graph->scale.x / 2;
        int   count = snprintf(NULL, 0, "%3g", val);
        snprintf(font->font_buffer, count + 1, "%3g", val);
        String str = {.data = font->font_buffer, .length = count};
        FillText(&scene->axes_labels, font, position, str, 0);
    }

    int yLow  = -origin.y / graph->slide_scale.y - 1;
    int yHigh = (screen_height - origin.y) / graph->slide_scale.y + 1;

    for (int y = yLow * 2; y <= yHigh * 2; ++y)
    {
        if (y == 0)
            continue;
        position.x = origin.x - font->height * 1.5f;
        position.y = origin.y + y * graph->slide_scale.y / 2 - font->height / 2;

        if (position.x < 0)
            position.x = font->height;
        else if (position.x > screen_width)
            position.x = screen_width - font->height;

        float val   = y * graph->scale.y / 2;
        int   count = snprintf(NULL, 0, "%3g", val);
        snprintf(font->font_buffer, count + 1, "%3g", val);
        String str = {.data = font->font_buffer, .length = count};
        FillText(&scene->axes_labels, font, position, str, 0);
    }
}

// void DrawLegends(Scene *scene, Font *font, Graph *graph) // choose location automatically
//{
//     // First draw a appropriate bounding box -> Easily handled by line strips
//     // Draw a small rectangle filled with that color -> Might need a new pixel shader -> Used simple line instead
//     // Finally draw text with plot name -> Already available
//     // Find the appropriate place to put the legend
//     MVec2 pos = {0};
//     pos.x     = screen_width - 200;
//     pos.y     = screen_height - font->height;
//
//     for (int label = 0; label < scene->fonts.count; ++label)
//     {
//         // Add a line indicator with given color for legends
//         AddSingleVertex(scene_group, pos);
//         AddSingleVertex(scene_group, (MVec2){pos.x + 50, pos.y});
//         scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
//         scene_group->graphcolor[scene_group->graphcount - 1] = scene_group->graphcolor[label];
//         FillText(scene_group, font, (MVec2){pos.x + 75, pos.y - font->height / 2},
//                  (String){.data = scene_group->graphname[label], .length =
//                  strlen(scene_group->graphname[label])}, 0);
//
//         pos.y -= font->height;
//     }
//
//     // Add a graph break for a small box around legends
//     AddSingleVertex(scene_group, (MVec2){pos.x - 10, screen_height});
//     AddSingleVertex(scene_group, (MVec2){pos.x - 10, pos.y - 20});
//     AddSingleVertex(scene_group, (MVec2){screen_width, pos.y - 20});
//
//     assert(scene_group->graphcount < scene_group->cMaxGraph);
//     scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
//     scene_group->graphcolor[scene_group->graphcount - 1] = (MVec3){0.5f, 0.5f, 0.5f};
//     scene_group->graphname[scene_group->graphcount - 1]  = ""; // only static strings are expected as of yet
// }

// void ShowList(RenderScene *scene_group, Font *font, Graph *graph, float *x, float *y, int length,
//               const char *GiveOnlyStaticStrings, MVec3 color)
//{
//     MVec2 vec;
//     for (int points = 0; points < length; ++points)
//     {
//         vec.x = graph->center.x + x[points] * graph->slide_scale.x / (graph->scale.x);
//         vec.y = graph->center.y + y[points] * graph->slide_scale.y / (graph->scale.y);
//         AddSingleVertex(scene_group, vec);
//     }
//
//     assert(scene_group->graphcount < scene_group->cMaxGraph);
//     scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
//     scene_group->graphcolor[scene_group->graphcount - 1] = color;
//     scene_group->graphname[scene_group->graphcount - 1]  = GiveOnlyStaticStrings;
// }

// From computation context not handled yet
bool InvokeAndTestFunction(void *function, FunctionType type, MVec2 vec)
{
    switch (type)
    {
    case PARAMETRIC_1D:
        return fabs(((ParametricFn1D)function)(vec.x) - vec.y) < 0.04f;
    case IMPLICIT_2D:
        return fabs(((ImplicitFn2D)function)(vec.x, vec.y)) < 0.04f;
    }
    return false;
}

void HandleEvents(GLFWwindow *window, Scene *scene, State *state, Graph *graph, Mat4 *translate_matrix,
                  Mat4 *scale_matrix, Panel *panel, Mat4 *new_transform, Mat4 *scene_transform)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
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
            double delX       = xpos - state->xpos;
            double delY       = ypos - state->ypos;

            Mat4   translate  = TranslationMatrix(delX, -delY, 0.0f);
            *translate_matrix = MatrixMultiply(&translate, translate_matrix);

            //// The transform need to be carried out in current screen position
            //// Do the inverse transformation first
            // Mat4 translation = TranslationMatrix(delX, -delY, 0.0f);
            *new_transform = MatrixMultiply(&translate, new_transform);

            state->xpos    = xpos;
            state->ypos    = ypos;
        }
        }
    }
    else
    {
        state->bPressed = false;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // use the inverse mapping to find the original vector point

        float vec[] = {xpos - scroll_animation.offset, screen_height - ypos, 0.0f, 1.0f};
        // Mat4  inverse_translate = TranslationMatrix(-translate_matrix->elem[0][3], -translate_matrix->elem[1][3],
        // 0.0f); Mat4  inverse_scale     = ScalarMatrix(1.0f / scale_matrix->elem[0][0], 1.0f /
        // scale_matrix->elem[1][1], 1.0f); Mat4  inverse           = MatrixMultiply(&inverse_scale,
        // &inverse_translate);
        Mat4 inverse = InverseMatrix(new_transform);
        MatrixVectorMultiply(&inverse, vec);

        fprintf(stderr, "Points selected : %5g and %5g.\n", vec[0], vec[1]);
        // loop through all the functions
        scene->plots.current_selection = -1;
        for (uint32_t fn = 0; fn < scene->plots.count; ++fn)
        {
            if (InvokeAndTestFunction(scene->plots.functions[fn].function, scene->plots.functions[fn].fn_type,
                                      (MVec2){vec[0], vec[1]}))
            {
                scene->plots.current_selection = fn;
                break;
            }
        }
    }

    if (scroll_animation.should_animate)
    {
        float t = (glfwGetTime() - scroll_animation.start) / scroll_animation.duration_constant;
        if (t >= 1.0f)
        {
            t                               = 1.0f;
            scroll_animation.should_animate = false;
        }
        float s              = scroll_animation.g_init + t * (scroll_animation.g_term - scroll_animation.g_init);
        graph->slide_scale.x = scroll_animation.s_init + t * (scroll_animation.s_term - scroll_animation.s_init);
        *scale_matrix        = ScalarMatrix(s, s, 1.0f);
        graph->slide_scale.y = graph->slide_scale.x;
    }

    // Viewport shifting
    if (panel->render.Anim.should_run)
    {
        float now            = glfwGetTime(); // time in seconds
        panel->render.Anim.t = (now - panel->render.Anim.last_time) / panel->render.Anim.time_constant;

        if (panel->render.Anim.t >= 1.0f)
            panel->render.Anim.t = 1.0f;

        float offset = (1.0f - panel->render.Anim.t) * panel->dimension.x;

        if (!panel->render.Anim.hidden)
            offset = panel->render.Anim.t * panel->dimension.x;

        scroll_animation.offset_changed = true;
        scroll_animation.offset         = offset;
        // Update the ortho projection matrix and restore to its previous position later on
    }
}

// Functions related to API
// To expose to API we need two sets of data .. first the original values and second scaled values

// void APIRecalculate(MorphPlotDevice *device)
//{
//     device->render_scene->vCount = 0;
//     MVec2 vec;
//     for (int re = 0; re < device->render_scene->pCount; ++re)
//     {
//         vec.x = device->graph->center.x +
//                 device->render_scene->Points[re].x * device->graph->slide_scale.x / (device->graph->scale.x);
//         vec.y = device->graph->center.y +
//                 device->render_scene->Points[re].y * device->graph->slide_scale.y / (device->graph->scale.y);
//         AddSingleVertex(device->render_scene, vec);
//     }
// }

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
    default:
        TriggerBreakpoint();
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

MorphPlotDevice MorphCreateDevice()
{
    MorphPlotDevice device;

    device.window = LoadGLFW(screen_width, screen_height, "Morph Graph");

    srand(time(NULL));
    // Shader vertex =
    //     LoadShadersFromString("#version 330 core \r\nlayout (location = 0) in vec2 aPos; \r\n\r\nuniform mat4
    //     scene;
    //     "
    //                           "\r\n\r\nvoid main() \r\n{\r\n\tgl_Position = scene * vec4(aPos,0.0f,1.0f);\r\n}",
    //                           VERTEX_SHADER);

    // Shader fragment = LoadShadersFromString(
    //     "#version 330 core \r\n\r\nuniform vec3 inColor; \r\nout vec4 color; \r\nvoid main()\r\n{\r\n\t// "
    //     "color = vec4(0.0f,1.0f,0.0f,1.0f);\r\n\tcolor = vec4(inColor,1.0f);\r\n}",
    //     FRAGMENT_SHADER);

    Shader vertex   = LoadShader("./src/shader/aaline.vs", VERTEX_SHADER);
    Shader fragment = LoadShader("./src/shader/aaline.fs", FRAGMENT_SHADER);
    Shader geometry = LoadShader("./src/shader/aaline.gs", GEOMETRY_SHADER);

    device.program  = LoadProgram3(vertex, fragment, geometry);

    // Enable the multi sampling
    glEnable(GL_MULTISAMPLE);

    Scene *scene = malloc(sizeof(*scene));
    Init2DScene(scene);

    Mat4 *ortho_matrix = malloc(sizeof(Mat4));
    if (ortho_matrix)
        *ortho_matrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);

    Mat4 *world_matrix = malloc(sizeof(Mat4));
    if (world_matrix)
        /**world_matrix = TranslationMatrix(400,400,0);*/
        *world_matrix = IdentityMatrix();

    Mat4 *scale_matrix = malloc(sizeof(Mat4));
    if (scale_matrix)
        *scale_matrix = IdentityMatrix();

    Mat4 *new_transform = malloc(sizeof(Mat4));
    if (new_transform)
        *new_transform = IdentityMatrix();

    *new_transform = ScalarMatrix(200.0f, 200.0f, 1.0f);
    Graph *graph   = malloc(sizeof(*graph));
    InitGraph(graph);

    Font *ComicSans = malloc(sizeof(*ComicSans));
    // LoadFont(ComicSans, "./include/comic.ttf");
    LoadSystemFont(ComicSans, "comic.ttf");

    Panel *panel       = CreatePanel(screen_width, screen_height);
    panel->render.font = ComicSans;

    Parser   *parser   = CreateParser(NULL, 0);

    UserData *data     = malloc(sizeof(*data));
    if (data)
        *data = (UserData){.OrthoMatrix     = ortho_matrix,
                           .graph           = graph,
                           .scale_transform = scale_matrix,
                           .panel           = panel,
                           .parser          = parser,
                           .scene           = scene,
                           .new_transform   = new_transform, // The ultimate transformation
                           .translation     = world_matrix};

    glfwSetWindowUserPointer(device.window, data);

    device.panner = malloc(sizeof(*device.panner));
    if (device.panner)
        memset(device.panner, 0, sizeof(*device.panner));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    device.scene           = scene;
    device.graph           = graph;
    device.font            = ComicSans;
    device.panel           = panel;
    device.transform       = ortho_matrix;
    device.world_transform = world_matrix;
    device.scale_matrix    = scale_matrix;
    device.new_transform   = new_transform; // Make the operations cumulatives

// Initialize timer here
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    device.timer.count     = counter.QuadPart;
    device.timer.frequency = frequency.QuadPart;
#elif defined(__linux__)
    // using monotonic clock instead of realtime
    struct timespec ts;
    clock_getres(CLOCK_MONOTONIC, &ts);
    device.timer.frequency = (uint64_t)(1000000000ULL / ts.tv_nsec);
    clock_gettime(CLOCK_MONOTONIC, &ts);
    device.timer.count =
        (uint64_t)(ts.tv_sec * device.timer.frequency + ts.tv_nsec * device.timer.frequency / 1000000000ULL);
#endif

    device.should_close = false;

    InitInterpreter();
    scroll_animation.duration_constant = 0.05f;
    scroll_animation.offset_changed    = true;
    scroll_animation.offset            = 0.0f;
    return device;
}
//
// void MorphPlotFunc(MorphPlotDevice *device, ParametricFn1D fn, MVec3 rgb, float xstart, float xend,
//                   const char *cstronly, float samplesize)
//{
//    float init = -10.0f;
//    float term = 10.0f;
//    float step = 0.05f;
//    if (fabsf(xstart - xend) > FLT_EPSILON)
//    {
//        init = xstart > xend ? xend : xstart;
//        term = xstart > xend ? xstart : xend;
//    }
//    if (samplesize > FLT_EPSILON)
//        step = samplesize;
//    MVec2 vec;
//
//    for (float x = init; x <= term; x += step)
//    {
//        // This API version doesn't check for discontinuity .. Above function checks for discontinuity of one
//        function vec.x = x; vec.y = fn(vec.x); AddSinglePoint(device->render_scene, vec);
//    }
//    // Add number of vertices in the current graph
//    assert(device->render_scene->graphcount < device->render_scene->cMaxGraph);
//    device->render_scene->graphbreak[device->render_scene->graphcount++]   = device->render_scene->pCount;
//    device->render_scene->graphcolor[device->render_scene->graphcount - 1] = rgb;
//    device->render_scene->graphname[device->render_scene->graphcount - 1]  = cstronly;
//}

// void MorphParametric2DPlot(MorphPlotDevice *device, ParametricFn2D fn, float tInit, float tTerm, MVec3 rgb,
//                            const char *cstronly, float step)
//{
//     MVec2 vec;
//     for (float t = tInit; t <= tTerm; t += step)
//         AddSinglePoint(device->render_scene, fn(t));
//     assert(device->render_scene->graphcount < device->render_scene->cMaxGraph);
//     device->render_scene->graphbreak[device->render_scene->graphcount++]   = device->render_scene->pCount;
//     device->render_scene->graphcolor[device->render_scene->graphcount - 1] = rgb;
//     device->render_scene->graphname[device->render_scene->graphcount - 1]  = cstronly;
// }

double MorphTimeSinceCreation(MorphPlotDevice *device)
{
    double elapsed_time = 0;
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    elapsed_time = (((uint64_t)counter.QuadPart - device->timer.count) * 1.0) / device->timer.frequency;
#elif defined(__linux__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t count = ts.tv_sec * device->timer.frequency + ts.tv_nsec * device->timer.frequency / 1000000000ULL;
    elapsed_time   = (((uint64_t)count - device->timer.count) * 1.0) / device->timer.frequency;
#endif

    return elapsed_time;
}
//
// void APIReset(MorphPlotDevice *device, uint32_t hold)
//{
//    device->render_scene->fCount     = 0;
//    device->render_scene->graphcount = hold;
//}

bool MorphShouldWindowClose(MorphPlotDevice *device)
{
    return device->should_close;
}

// void MorphShow(MorphPlotDevice *device)
//{
//     glfwShowWindow(device->window);
//     State    panner = {0};
//
//     uint32_t hold   = device->render_scene->graphcount;
//
//     while (!glfwWindowShouldClose(device->window))
//     {
//         glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT);
//         // Instead of re-rendering, change the scale of the already plotted points
//         APIReset(device, hold);
//         APIRecalculate(device);
//         RenderGraph(device->graph);
//         glUseProgram(device->program);
//         glUniformMatrix4fv(glGetUniformLocation(device->program, "scene"), 1, GL_TRUE,
//         &device->transform->elem[0][0]); glBindVertexArray(device->vao); glBindBuffer(GL_ARRAY_BUFFER,
//         device->vbo);
//
//         RenderLabels(device->render_scene, device->font, device->graph, device->transform);
//         DrawLegends(device->render_scene, device->font, device->graph);
//         RenderRenderScene(device->render_scene, device->program, false);
//         RenderFont(device->render_scene, device->font, device->transform);
//
//         HandleEvents(device->window, &panner, device->graph);
//         glfwSwapBuffers(device->window);
//         glfwPollEvents();
//     }
// }

// void MorphPhantomShow(MorphPlotDevice *device)
//{
//     uint32_t hold = device->render_scene->graphcount;
//
//     glfwShowWindow(device->window);
//     glfwMakeContextCurrent(device->window);
//
//     glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
//     glClear(GL_COLOR_BUFFER_BIT);
//
//     APIReset(device, hold);
//     APIRecalculate(device);
//     RenderGraph(device->graph);
//
//     glUseProgram(device->program);
//     glUniformMatrix4fv(glGetUniformLocation(device->program, "scene"), 1, GL_TRUE,
//     &device->transform->elem[0][0]); glBindVertexArray(device->vao); glBindBuffer(GL_ARRAY_BUFFER, device->vbo);
//
//     RenderLabels(device->render_scene, device->font, device->graph, device->transform);
//     DrawLegends(device->render_scene, device->font, device->graph);
//     RenderRenderScene(device->render_scene, device->program, false);
//     RenderFont(device->render_scene, device->font, device->transform);
//
//     glfwSwapBuffers(device->window);
//
//     HandleEvents(device->window, device->panner, device->graph);
//     glfwPollEvents();
//
//     device->render_scene->graphcount -= hold + 1;
//     device->should_close = glfwWindowShouldClose(device->window);
// }

void MorphDestroyDevice(MorphPlotDevice *device)
{
    Destroy2DScene(device->scene);
    free(device->scene);
    UserData *data = glfwGetWindowUserPointer(device->window);
    free(data);
    free(device->transform);
    free(device->graph);
    free(device->font);
    glfwDestroyWindow(device->window);
    glfwTerminate();
}

// void MorphPlotList(MorphPlotDevice *device, float *x, float *y, int length, MVec3 rgb, const char *cstronly)
//{
//     for (int points = 0; points < length; ++points)
//         AddSinglePoint(device->render_scene, (MVec2){x[points], y[points]});
//
//     assert(device->render_scene->graphcount < device->render_scene->cMaxGraph);
//     device->render_scene->graphbreak[device->render_scene->graphcount++]   = device->render_scene->pCount;
//     device->render_scene->graphcolor[device->render_scene->graphcount - 1] = rgb;
//     device->render_scene->graphname[device->render_scene->graphcount - 1]  = cstronly;
// }

// void MorphResetPlotting(MorphPlotDevice *device)
//{
//     ResetRenderScene(device->render_scene);
// }

double ImplicitCircle(double x, double y)
{
    return x * x + y * y - 4; // Implicit circle of radius 2
}

double ImplicitEllipse(double x, double y)
{
    return 2 * x * x + 5 * y * y - 40;
}

double ImplicitHyperbola(double x, double y)
{
    return x * x - y * y - 1;
}

double PartialDerivativeX(ImplicitFn2D fn, double x, double y, double h)
{
    return (fn(x + h, y) - fn(x, y)) / h;
}

double PartialDerivativeY(ImplicitFn2D fn, double x, double y, double h)
{
    return (fn(x, y + h) - fn(x, y)) / h;
}

MVec2 Gradient2D(ImplicitFn2D fn, double x, double y, double h)
{
    return (MVec2){PartialDerivativeX(fn, x, y, h), PartialDerivativeY(fn, x, y, h)};
}

MVec2 ContourDirection(ImplicitFn2D fn, double x, double y, double h)
{
    MVec2 grad = Gradient2D(fn, x, y, h);
    float norm = sqrtf(grad.x * grad.x + grad.y * grad.y);
    return (MVec2){.x = -grad.y / norm, .y = grad.x / norm};
}

// Takes functions of the form f(x,y) - c to plot f(x,y) = c

void ImplicitFunctionPlot2D(MorphPlotDevice *device, ImplicitFn2D fn)
{
    Scene         *scene     = device->scene;

    const uint32_t max_verts = 5000;

    // First determine a point in the function using any method
    // My approach :
    // Start at the origin and move along the partial derivatives to reach to the initial position on the surface

    MVec2        vec   = {0, 0}; // start at the origin and land on the contour
    float        hstep = 0.025f, kstep = 0.025f;

    float        h            = 0.0005;

    uint32_t     sample_count = 0;
    VertexData2D vertex;

    uint32_t     counter          = 0;

    uint32_t     plots            = 4;
    int32_t      dir[]            = {1, -1, 1, -1};
    float        origin_offsets[] = {0.1f, 0.1f, -0.1f, -0.1f};

    while (plots--)
    {
        // To reach the contour line, we must travel along the partial derivatives first and use Newton Raphson
        // method to find the point of contour Taking y constant for now and moving in the direction of partial
        // derivative
        scene->plots.functions[scene->plots.count].max     = max_verts; // 1000 vertices for each graph at most
        scene->plots.functions[scene->plots.count].samples = malloc(sizeof(MVec2) * max_verts);

        FunctionPlotData *function                         = &scene->plots.functions[scene->plots.count];
        function->fn_type                                  = IMPLICIT_2D;
        function->color                                    = (MVec3){1.0f, 0.5f, 0.6f};
        function->function                                 = fn;

        vec.x                                              = origin_offsets[plots];
        vec.y                                              = 0.0f;

        int32_t max_movement                               = 15000;
        while (fabs(fn(vec.x, vec.y)) > 0.0025f)
        {
            // vec.y = vec.y - fn(vec.x, vec.y) / PartialDerivativeY(fn, vec.x, vec.y, hstep);
            vec.x = vec.x - fn(vec.x, vec.y) / PartialDerivativeX(fn, vec.x, vec.y, hstep);
        }

        vertex.x                             = vec.x;
        vertex.y                             = vec.y;

        function->samples[function->count++] = vertex;

        // Now move along the contour of the implicit function.
        // Calculate partial derivative along X and Y direction.
        // Gradient of the function

        while (max_movement--)
        {
            // Calculate delX and delY such that they remains in the contour
            // dy = -hstep * PartialDerivativeX(fn, vec.x, vec.y, h) / PartialDerivativeY(fn, vec.x + hstep, vec.y,
            // h);

            // If the path can't remain in the contour, then dx will be ??
            // dx = hstep; // -hstep * PartialDerivativeY(fn, vec.x, vec.y, h) / PartialDerivativeX(fn, vec.x, vec.y
            // + kstep, h); else
            // {
            //     x_inc =
            //         -hstep * PartialDerivativeY(fn, vec.x, vec.y, h) / PartialDerivativeX(fn, vec.x, vec.y +
            //         kstep, h);

            // }
            // Trace back to the contour after the increment
            // Move toward that and again trace back to the contour
            // Lets apply concept from the higher dimensional vector calculus

            MVec2 tangent = ContourDirection(fn, vec.x, vec.y, h);
            float dx      = dir[plots] * tangent.x * h * 2.5f;
            float dy      = dir[plots] * tangent.y * h * 2.5f;

            // once this is finished move toward the negative contour
            vec.x = vec.x + dx;
            vec.y = vec.y + dy;

            // while (fabs(fn(vec.x, vec.y)) > FLT_EPSILON)
            //{
            //     vec.y = vec.y - fn(vec.x, vec.y) / PartialDerivativeY(fn, vec.x, vec.y, hstep);
            // }
            if (counter++ % 100 == 0)
            {
                assert(function->count < function->max);
                vertex.x                                   = vec.x;
                vertex.y                                   = vec.y;
                function->samples[function->count - 1].n_x = vec.x - function->samples[function->count - 1].x;
                function->samples[function->count - 1].n_y = vec.y - function->samples[function->count - 1].y;
                function->samples[function->count++]       = vertex;
            }
        }
        function->color   = (MVec3){1.0f, 0.0f, 1.0f};
        function->batch   = CreateNewBatch(LINE_STRIP);
        function->updated = true;
        scene->plots.count++;
    }
}

double Square(double x)
{
    return x * x;
}

// The whole transform is messy, gotta clean it up
void Draw(MorphPlotDevice *device, Mat4 *translate, Mat4 *scale, bool show_points)
{
    // When offset changes the point of origin also shifts at certain distance away
    float Y               = device->graph->slide_scale.x;

    Mat4  outer_transform = *device->new_transform;
    *device->transform =
        OrthographicProjection(0, screen_width - scroll_animation.offset, 0, screen_height, -1.0f, 1.0f);
    glViewport(scroll_animation.offset, 0, screen_width - scroll_animation.offset, screen_height);

    Mat4 translater              = TranslationMatrix(scroll_animation.offset, 0.0f, 0.0f);

    Mat4 graph_transform         = *device->new_transform;
    graph_transform              = MatrixMultiply(&translater, &graph_transform);

    float f                      = 1.0f;
    device->graph->slide_scale.x = Y;
    device->graph->slide_scale.y = Y;

    Mat4 nscalar                 = ScalarMatrix(scale->elem[0][0] * f, scale->elem[1][1] * f, 1.0f);
    Mat4 ntransform              = MatrixMultiply(translate, &nscalar);

    RenderGraph(device->graph, &graph_transform, Y, Y);
    RenderScene(device->scene, device->program, false, device->transform, device->new_transform);

    RenderLabels(device->scene, device->font, device->graph, &outer_transform);
    RenderFont(device->scene, device->font, device->transform);

    device->graph->slide_scale.x = Y;
    device->graph->slide_scale.y = Y;
}

// void MorphPlot(MorphPlotDevice *device)
//{
//     Shader   vertex   = LoadShader("./src/shader/common_2D.vs", VERTEX_SHADER);
//     Shader   fragment = LoadShader("./src/shader/common_2D.fs", FRAGMENT_SHADER);
//     uint32_t program  = LoadProgram(vertex, fragment);
//
//     Mat4     identity;
//
//     // Since all of them need to respond to function only once, it needs to go inside key callback
//
//     while (!glfwWindowShouldClose(device->window))
//     {
//         // matrix = MatrixMultiply(device->world_transform, device->scale_matrix);
//         // RenderGraph(device->graph, device->world_transform);
//         // RenderScene(device->scene, device->program, false, device->transform, &matrix);
//         // RenderLabels(device->scene, device->font, device->graph, &matrix);
//         // RenderFont(device->scene, device->font, device->transform);
//         Draw(device, device->world_transform, device->scale_matrix, false);
//
//         glUseProgram(program);
//         identity = MatrixMultiply(device->transform, &device->panel->render.local_transform);
//         glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_TRUE,
//                            (const GLfloat *)&identity.elem[0][0]);
//
//         glViewport(0, 0, screen_width, screen_height);
//         RenderPanel(device->panel, device->panel->render.font, device->transform);
//
//         device->panel->render.font_batch->vertex_buffer.count = 0;
//         device->scene->axes_labels.count                      = 0;
//
//         HandleEvents(device->window, device->scene, device->panner, device->graph, device->world_transform,
//                      device->scale_matrix, device->panel, device->new_transform, device->transform);
//         glfwSwapBuffers(device->window);
//         glfwPollEvents();
//     }
// }

bool CreateAlternateFrameBuffer(uint32_t width, uint32_t height)
{
    GLuint fbo;
    glGenFramebuffers(1, &fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Texture attachments
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Bind the texture as the color attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    // Check for framebuffer completion
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "GLFramebuffer incomplete");
        return false;
    }
    else
        fprintf(stderr, "GLFRamebuffer completed");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    AlternateFrameBuffer.fbo = fbo;
    AlternateFrameBuffer.tex = tex;
    return true;
}

void MorphPlot(MorphPlotDevice *device)
{
    Shader   vertex   = LoadShader("./src/shader/common_2D.vs", VERTEX_SHADER);
    Shader   fragment = LoadShader("./src/shader/common_2D.fs", FRAGMENT_SHADER);
    uint32_t program  = LoadProgram(vertex, fragment);

    Mat4     identity;

    GLuint   fbo = CreateAlternateFrameBuffer(1080, 720);
    // Since all of them need to respond to function only once, it needs to go inside key callback

    while (!glfwWindowShouldClose(device->window))
    {
        // matrix = MatrixMultiply(device->world_transform, device->scale_matrix);
        // RenderGraph(device->graph, device->world_transform);
        // RenderScene(device->scene, device->program, false, device->transform, &matrix);
        // RenderLabels(device->scene, device->font, device->graph, &matrix);
        // RenderFont(device->scene, device->font, device->transform);
        if (active_fbo == 0)
        {

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            Draw(device, device->world_transform, device->scale_matrix, false);

            glUseProgram(program);
            identity = MatrixMultiply(device->transform, &device->panel->render.local_transform);
            glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_TRUE,
                               (const GLfloat *)&identity.elem[0][0]);

            glViewport(0, 0, screen_width, screen_height);
            RenderPanel(device->panel, device->panel->render.font, device->transform);

            device->panel->render.font_batch->vertex_buffer.count = 0;
            device->scene->axes_labels.count                      = 0;

            HandleEvents(device->window, device->scene, device->panner, device->graph, device->world_transform,
                         device->scale_matrix, device->panel, device->new_transform, device->transform);
        }
        else
        {
            // Blit just once
            glBindFramebuffer(GL_READ_FRAMEBUFFER, active_fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

            glBlitFramebuffer(0, 0, 1080, 720, 0, 0, screen_width, screen_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        glfwSwapBuffers(device->window);
        glfwPollEvents();
    }
}
