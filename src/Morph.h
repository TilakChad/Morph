#pragma once

// Ye .. this API will be called Morph -> Morphism now
#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct Scene Scene;
typedef struct Graph Graph;
typedef struct Font  Font;
typedef struct Mat4  Mat4;
typedef struct State State;

typedef struct
{
    float x;
    float y;
} MVec2;

typedef struct Vec3
{
    float x;
    float y;
    float z;
} MVec3;

typedef double (*ParametricFn1D)(double);
// Parameterized by a single parameter
typedef MVec2 (*ParametricFn2D)(double);

typedef double (*ImplicitFn2D)(double x, double y);

typedef struct Panel Panel;

typedef struct
{
    unsigned int program;
    unsigned int vao;
    unsigned int vbo;
    GLFWwindow  *window;
    Scene       *scene;
    Graph       *graph;
    Font        *font;
    Mat4        *transform;
    Mat4        *world_transform;
    Mat4        *scale_matrix;

    Mat4        *new_transform;
    State       *panner;
    bool         should_close;
    struct Timer
    {
        uint64_t frequency;
        uint64_t count;
    } timer;
    Panel *panel;
} MorphPlotDevice;

// Exposed func
MorphPlotDevice MorphCreateDevice();
// This call will block... There's a plan to make it nonblocking
void MorphShow(MorphPlotDevice *device);
// This is a non blocking version but it doesn't respond to any events
void   MorphPhantomShow(MorphPlotDevice *);
void   MorphDestroyDevice(MorphPlotDevice *device);
void   MorphPlotList(MorphPlotDevice *device, float *xpts, float *ypts, int length, MVec3 rgb, const char *cstronly);
void   MorphPlotFunc(MorphPlotDevice *device, ParametricFn1D fn, MVec3 color, float xinit, float xend,
                     const char *cstronly, float step);
void   MorphParametric2DPlot(MorphPlotDevice *device, ParametricFn2D fn, float tInit, float tTerm, MVec3 rgb,
                             const char *cstronly, float step);
double MorphTimeSinceCreation(MorphPlotDevice *device);
void   MorphResetPlotting(MorphPlotDevice *device);
bool   MorphShouldWindowClose(MorphPlotDevice *device);
void   ImplicitFunctionPlot2D(MorphPlotDevice *device, ImplicitFn2D fn);
