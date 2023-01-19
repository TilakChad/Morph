// An interactive UI that pops up from the left side of the screen when pressed TAB
// It will contain a place to write out function for plotting

#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./render_common.h"

#include "./stb_truetype.h"

static void FillText(GPUBatch *font_data, Font *font, Pos2D position, String str, uint16_t *advancement, float scale)
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

static void InitPanel(Panel *panel)
{
}

static uint32_t screen_height = 800, screen_width = 600;

Panel          *CreatePanel(uint32_t scr_w, uint32_t scr_h)
{
    Panel *panel = malloc(sizeof(*panel));
    assert(panel != NULL);
    memset(panel, 0, sizeof(*panel));

    panel->dimension            = (Pos2D){250, scr_h};
    panel->origin               = (Pos2D){0, scr_h};

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

    panel->render.local_transform                    = TranslationMatrix(-panel->dimension.x,0.0f,0.0f);
    panel->render.Anim.hidden                        = true;
    return panel;
}

static void PrepareFontBatch(GPUBatch *batch)
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

static void PrepareVertexBatch(GPUBatch *batch)
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

static float AnimateCaret(Panel *panel, float old_pos, float new_pos)
{
    panel->render.CaretAnim.should_animate = true;
    panel->render.CaretAnim.started        = glfwGetTime();
    panel->render.CaretAnim.origin         = old_pos;
    panel->render.CaretAnim.target         = new_pos;
}

static float GetCaretPos(Panel *panel)
{
    float offset = 0;

    for (uint32_t i = panel->panel.history[panel->panel.active_panel].renderdata.visible_start;
         i < panel->panel.history[panel->panel.active_panel].caret_pos; ++i)
    {
        offset = offset + panel->panel.history[panel->panel.active_panel].renderdata.advancement[i];
    }
    return offset;
}

static void UpdatePanel(Panel *panel)
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

            // In case of folding, panel->render.Anim.t should run backward

            float offset = -panel->dimension.x + panel->dimension.x * panel->render.Anim.t;               

            if (panel->render.Anim.hidden)
                offset = panel->render.Anim.t * -panel->dimension.x;

            panel->render.local_transform = TranslationMatrix(offset, 0.0f, 0.0f);
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

        float colors[]   = {0.95f, 0.95f, 0.95f, 0.85f, 0.85f, 0.85f, 0.75f, 0.75f, 0.75f,
                            1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  0.0f,  1.0f};

        // use transformation relative to the parent origin
        float transform_vec[] = {panel->origin.x, panel->origin.y, 0.0f, 1.0f};
        MatrixVectorMultiply(&panel->render.local_transform, transform_vec);

        for (uint32_t box_c = 0; box_c < panel->panel.history_count; box_c++)
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
                offset                = panel->render.CaretAnim.origin + t * (offset - panel->render.CaretAnim.origin);
                panel->render.updated = true;
            }
            else
                panel->render.CaretAnim.should_animate = false;
        }

        uint32_t c_thickness   = 3.0f;
        uint32_t c_offset      = (panel->layout.box_gap - panel->render.font->size) / 2.0f;
        Pos2D    caret         = {.x = transform_vec[0] + offset,
                                  .y = transform_vec[1] - c_offset - (panel->panel.history_count - 1) * panel->layout.box_gap};

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

static void RenderText(GPUBatch *batch, uint32_t font_program, Mat4 *transform, Panel *panel, uint32_t font_texture);

void        RenderPanel(Panel *panel, Font *font, Mat4 *ortho)
{
    // Enable scissor
    // glEnable(GL_SCISSOR_TEST);
    // glScissor(0, 0, panel->dimension.x, panel->dimension.y);
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
             (Pos2D){vec[0], vec[1] - panel->panel.active_panel * panel->layout.box_gap -
                                 (panel->layout.box_gap + panel->render.font->size) / 2.0f},
             (String){.data   = active_panel->buffer + active_panel->renderdata.visible_start,
                      .length = active_panel->len - active_panel->renderdata.visible_start},
             active_panel->renderdata.advancement + active_panel->renderdata.visible_start, 1.0f);

    UpdatePanel(panel);
    // TODO :: Update this segment
    DrawBatch(panel->render.batch, 30);

    for (uint32_t text = 0; text < panel->panel.history_count; ++text)
    {
        if (text != panel->panel.active_panel)
        {
            TextPanel *a_panel = &panel->panel.history[text];
            FillText(panel->render.font_batch, font,
                     (Pos2D){vec[0], vec[1] - text * panel->layout.box_gap -
                                         (panel->layout.box_gap + panel->render.font->size) / 2.0f},
                     (String){.data = a_panel->buffer, .length = a_panel->len}, a_panel->renderdata.advancement, 1.0f);
        }
    }

    panel->render.font_batch->vertex_buffer.dirty = true;
    PrepareFontBatch(panel->render.font_batch);
    RenderText(panel->render.font_batch, panel->render.font->program, ortho, panel, panel->render.font->font_texture);

    // glDisable(GL_SCISSOR_TEST);
}

static void RenderText(GPUBatch *batch, uint32_t font_program, Mat4 *transform, Panel *panel, uint32_t font_texture)
{
    glUseProgram(font_program);
    glUniformMatrix4fv(glGetUniformLocation(font_program, "scene"), 1, GL_TRUE, &transform->elem[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    // DrawBatch(panel->render.font_batch, panel->render.font_batch->vertex_buffer.count / 4);
    // uint32_t draw_count = panel->panel.history[panel->panel.active_panel].len -
    //                      panel->panel.history[panel->panel.active_panel].renderdata.visible_start;
    DrawBatch(batch, batch->vertex_buffer.count / (4 * 4));
}

static void HandleArrows(Panel *panel, uint32_t arrow)
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

static void HandleEvents(GLFWwindow *window, Panel *panel)
{
    // if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
    //{
    //     panel->render.Anim.should_run = true;
    //     panel->render.Anim.last_time  = glfwGetTime();
    //     panel->render.Anim.t          = 0.0f;
    //     panel->render.Anim.fold       = !panel->render.Anim.fold;
    // }
}

typedef struct
{
    Mat4  *OrthoMatrix;
    Panel *Panel;
} UserData;

// static void FrameChangeCallback(Panel* panel, Mat4* ortho_matrix, int width, int height)
//{
//     screen_width  = width;
//     screen_height = height;
//     glViewport(0, 0, width, height);
//     UserData *data              = (UserData *)glfwGetWindowUserPointer(window);
//     *data->OrthoMatrix          = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);
//     data->Panel->dimension.x    = 0.30 * width;
//     data->Panel->dimension.y    = height;
//     data->Panel->origin.y       = height;
//     data->Panel->origin.x       = 0;
//     data->Panel->render.updated = true;
// }
void PanelFrameChangeCallback(Panel *panel, int width, int height)
{
    screen_width  = width;
    screen_height = height;
    glViewport(0, 0, width, height);
    panel->dimension.x    = 0.20 * width;
    panel->dimension.y    = height;
    panel->origin.y       = height;
    panel->origin.x       = 0;
    panel->render.updated = true;
}

static void RemoveCharAtIndex(char *string, uint32_t len, uint32_t index)
{
    // There might be overlapping memories, so use manual copying instead of memcpy()
    for (uint32_t i = index; i < len; ++i)
        string[i] = string[i + 1];
    string[len - 1] = '\0';
}

static void InsertCharAtIndex(char *str, uint32_t len, uint32_t index, char c)
{
    // start from the last
    for (int32_t i = len; i > index; --i)
        str[i] = str[i - 1];
    str[index] = c;
}

void PanelCharCallback(Panel *panel, unsigned int codepoint)
{
    PanelHistory *history      = &panel->panel;
    TextPanel    *active_panel = &history->history[history->active_panel];

    InsertCharAtIndex(active_panel->buffer, active_panel->len, active_panel->caret_pos, (char)codepoint);
    // Insert glyph here too
    panel->render.updated = true;

    // if (enable_caret_animation)
    // enable animation for caret

    active_panel->caret_pos = active_panel->caret_pos + 1;
    active_panel->len       = active_panel->len + 1;
}

void HandleArrows(Panel *panel, uint32_t arrows);

void PanelKeyCallback(Panel *panel, int key, int scancode, int action, int mods)
{

    //{
    PanelHistory *history      = &panel->panel;

    TextPanel    *active_panel = &history->history[history->active_panel];
    bool          both_action  = action & (GLFW_PRESS | GLFW_REPEAT);
    float         old          = GetCaretPos(panel);
    if (key == GLFW_KEY_BACKSPACE && both_action)
    {

        if (active_panel->len && active_panel->caret_pos)
        {
            // The caret will be drawn right before the character to be drawn
            // So, backspacing at carat 1 should erase 0
            RemoveCharAtIndex(active_panel->buffer, active_panel->len, active_panel->caret_pos - 1);
            active_panel->caret_pos--;
            active_panel->len--;
            panel->render.updated = true;
        }
    }
    if (both_action)
        HandleArrows(panel, key);
    AnimateCaret(panel, old, GetCaretPos(panel));

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
    {
        // simply change the active panel
        panel->panel.history_count++;
        panel->panel.active_panel++;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        panel->render.Anim.hidden       = !panel->render.Anim.hidden;
        panel->render.Anim.should_run = true;
        panel->render.Anim.last_time  = glfwGetTime();
        panel->render.Anim.t          = 0.0f;
    }
}

int nmain(int argc, char **argv)
{
    GLFWwindow *window = LoadGLFW(screen_width, screen_height, "Interactive Panel Creation");
    glfwShowWindow(window);
    glfwMakeContextCurrent(window);

    Panel *panel       = CreatePanel(800, 600);
    Shader vertex      = LoadShader("./src/shader/common_2D.vs", VERTEX_SHADER);
    Shader fragment    = LoadShader("./src/shader/common_2D.fs", FRAGMENT_SHADER);

    panel->render.font = malloc(sizeof(Font));
    // LoadFont(ComicSans, "./include/comic.ttf");
    LoadSystemFont(panel->render.font, "consolas.ttf");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    uint32_t program = LoadProgram(vertex, fragment);

    glUseProgram(program);

    Mat4 identity = IdentityMatrix();
    Mat4 ortho    = OrthographicProjection(0, 800, 0, 600, -1.0f, 1.0f);

    // UserData data     = {.OrthoMatrix = &ortho, .Panel = panel};
    // glfwSetWindowUserPointer(window, &data);

    // glfwSetFramebufferSizeCallback(window, FrameChangeCallback);
    // glfwSetKeyCallback(window, KeyCallback);
    // glfwSetCharCallback(window, CharCallback);

    while (!glfwWindowShouldClose(window))
    {
        glUseProgram(program);
        identity = MatrixMultiply(&ortho, &panel->render.local_transform);
        glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_TRUE, (GLfloat *)&identity.elem[0][0]);
        glClearColor(0.70f, 0.70f, 0.70f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        RenderPanel(panel, panel->render.font, &ortho);
        HandleEvents(window, panel);
        glfwSwapBuffers(window);
        glfwPollEvents();
        panel->render.font_batch->vertex_buffer.count = 0;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
