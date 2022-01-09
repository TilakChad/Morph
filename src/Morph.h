#pragma once 

// Ye .. this API will be called Morph -> Morphism now 
#include <GLFW/glfw3.h>

typedef struct RenderScene RenderScene;
typedef struct Graph       Graph;
typedef struct Font        Font;
typedef struct Mat4        Mat4;

typedef double (*oneparamfn)(double);

typedef struct 
{
    unsigned int program;
    unsigned int vao;
    unsigned int vbo;
    GLFWwindow  *window; 
    RenderScene *render_scene;
    Graph *      graph; 
    Font *       font;  
    Mat4 *       transform;
} MorphPlotDevice;

// Exposed func
MorphPlotDevice  MorphCreateDevice();
// This call will block... There's a plan to make it nonblocking 
void        MorphShow(MorphPlotDevice* device);
void        MorphDestroyDevice(MorphPlotDevice *device);
void        MorphAddList(MorphPlotDevice *device, float *xpts, float *ypts, int length, float r, float g, float b,
                         const char *cstronly);
void        MorphPlotFunc(MorphPlotDevice *device, oneparamfn fn, float r, float g, float b, float xinit, float xend, const char *cstronly, float step);