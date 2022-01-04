#pragma once 

#include <GLFW/glfw3.h>
#include "../maths/matrix.h"

typedef struct RenderScene RenderScene;
typedef struct Graph       Graph;
typedef struct Font        Font;

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
}PlotDevice;

PlotDevice  CreatePlottingDevice();
// This call will block... There's a plan to make it nonblocking 
void        ShowPlot(PlotDevice* device);
void        DestroyPlottingDevice(PlotDevice *device);