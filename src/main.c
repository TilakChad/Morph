#include <stdio.h>

#include <math.h>
#include "./Morph.h"

double ImplicitCircle(double x, double y);
double ImplicitEllipse(double x, double y);
double ImplicitHyperbola(double x, double y);
double GaussianIntegral(double x);
double parabola(double x);
double inv(double x);
double lin(double x);
double Square(double x);

double ImplicitHeart(double x, double y)
{
    return x * x + pow(y - pow(x * x, 1.0f / 3), 2.0f) - 1; 
}

void MorphPlot(MorphPlotDevice *device);
void Plot1D(Scene *scene, ParametricFn1D func, Graph *graph, MVec3 color, const char *legend);
int  main(int argc, char **argv)
{
    MorphPlotDevice device = MorphCreateDevice();
    glfwShowWindow(device.window);
    glfwMakeContextCurrent(device.window);

    // ImplicitFunctionPlot2D(&device, ImplicitCircle);
    ImplicitFunctionPlot2D(&device, ImplicitHeart);

    if (true)
    {
        // Plot1D(device.scene, GaussianIntegral, device.graph, (MVec3){0.1f, 0.1f, 0.75f}, "Nothing");
        /*Plot1D(device.scene, Square, device.graph, (MVec3){0.4f, 0.4f, 0.1f}, "Squared");
        Plot1D(device.scene, lin, device.graph, (MVec3){0.9f, 0.1f, 0.1f}, "Squared");*/

        uint32_t count = 0;
        MorphPlot(&device);
        MorphDestroyDevice(&device);
    }
    return 0;
}