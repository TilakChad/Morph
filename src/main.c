#include <stdio.h>

#include "./Morph.h"
#include <math.h>

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

void  MorphPlot(MorphPlotDevice *device);
void  Plot1D(Scene *scene, ParametricFn1D func, Graph *graph, MVec3 color, const char *legend);

MVec2 Circle2D(double t)
{
    return (MVec2){.x = cosf(t), .y = sinf(t)};
}

MVec2 Butterfly2D(double t)
{
    MVec2  point;
    double s = sin(t), c = cos(t), c4 = cos(4 * t), s5 = pow(sin(t / 12.0), 5);
    double inner = exp(c) - 2 * c4 - s5;
    point.x      = s * inner;
    point.y      = c * inner;
    return point;
}

MVec2 VectorFieldXY(double x, double y)
{
    return (MVec2){.x = x, .y = y};
}

int main(int argc, char **argv)
{
    MorphPlotDevice device = MorphCreateDevice();
    glfwShowWindow(device.window);
    glfwMakeContextCurrent(device.window);

    const double PI = 4 * atan(1);
    // API related functions
    // ImplicitFunctionPlot2D(&device, ImplicitCircle);
    // ImplicitFunctionPlot2D(&device, ImplicitHeart);
    // MorphParametric2DPlot(device.scene, Circle2D, 0.0f, 1.67f, (MVec3){.x = 0.9f, .y = 0.9f}, "circle", 0.05f);

    /* MorphParametric2DPlot(device.scene, Butterfly2D, 0.0f, 12 * PI, (MVec3){.x = 0.1f, .y = 0.2f, .z = 0.9f},
                           "Butterfly", 0.1f);*/

    if (true)
    {
        // Plot1D(device.scene, GaussianIntegral, device.graph, (MVec3){0.1f, 0.1f, 0.75f}, "Nothing");
        /*   Plot1D(device.scene, Square, device.graph, (MVec3){0.4f, 0.4f, 0.1f}, "Squared");
           Plot1D(device.scene, lin, device.graph, (MVec3){0.9f, 0.1f, 0.1f}, "Squared");*/

        MorphPlotVectorField2D(&device, VectorFieldXY, (Range){-5.0f, 5.0f}, (Range){-5.0f, 5.0f});
        MorphPlot(&device);
        MorphDestroyDevice(&device);
    }

    // Enable to see the butterfly in action
    else
    {
        // Animating gif of butterfly in action
        const float step_size = 0.05f;
        while (!MorphShouldWindowClose(&device))
        {
            float now = MorphTimeSinceCreation(&device);
            MorphResetPlotting(&device);

            float end = now / 25.0f;
            MorphParametric2DPlot(device.scene, Butterfly2D, 0.0f, now * 2.0f, (MVec3){.x = 0.1f, .y = 0.2f, .z = 0.9f},
                                  "Butterfly2D", step_size);
            // Plot1D(device.scene, GaussianIntegral, device.graph, (MVec3){0.1f, 0.1f, 0.75f}, "Nothing");

            MorphPhantomShow(&device);
        }
        MorphDestroyDevice(&device);
    }
    return 0;
}