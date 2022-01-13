#define _CRT_SECURE_NO_WARNINGS

#include "./Morph.h"
#include <Windows.h>
#include <math.h>

double Heaviside(double x)
{
    return x <= 0 ? 0 : 1;
}

static MVec2 RoseCurves(double t)
{
    // For function like these, partial application would've been sooo nicccee
    // k -> 0 & a -> 1 for a circle
    int   k = 10;
    int   a = 5;
    float r = a * cos(k * t);
    return (MVec2){.x = r * cos(t), .y = r * sin(t)};
}

static MVec2 RoseCurvesCircleVersion(double t)
{
    // For function like these, partial application would've been nicccee
    // k -> 0 & a -> 1 for a circle
    int   k = 0;
    int   a = 6;
    float r = a * cos(k * t);
    return (MVec2){.x = r * cos(t), .y = r * sin(t)};
}

static MVec2 Hypotrochoid(double theta)
{
    float R = 5, r = 3, d = 5;
    MVec2 vec;
    vec.x = (R - r) * cos(theta) + d * cos((R - r) * theta / r);
    vec.y = (R - r) * sin(theta) - d * sin((R - r) * theta / r);
    return vec;
}

int main()
{
    MorphPlotDevice device = MorphCreateDevice();

    // Demo Animation of plotting hypotrochoid
    float  t    = 0;
    double now  = MorphTimeSinceCreation(&device);
    double then = now;

    // For circle drawing and line 
    float  circle_x[50], circle_y[50];
    int    count = 0;
    float  line_x[5], line_y[5];

    MVec2  temp;        now = MorphTimeSinceCreation(&device);
    then = now; 
    while (!MorphShouldWindowClose(&device) && (then - now) < 20.0f)
    {
        count   = 0;
        float t = then - now;
        for (int i = 0; i <= 360 + 15; i += 15)
        {
            float theta     = i * 3.1415f / 180;
            circle_x[count] = 3 * cos(t) + 3 * cos(theta);
            circle_y[count] = 3 * sin(t) + 3 * sin(theta);
            count++;
        }
        line_x[0] = 3 * cos(t);
        line_y[0] = 3 * sin(t);
        temp      = Hypotrochoid(t);
        line_x[1] = temp.x;
        line_y[1] = temp.y;
        MorphResetPlotting(&device);
        MorphParametric2DPlot(&device, RoseCurvesCircleVersion, 0, 3.1942599 * 2, (MVec3){0.0f, 0.0f, 1.0f},
                              "RoseCircle", 0.05f);
        MorphParametric2DPlot(&device, RoseCurves, 0, t, (MVec3){1.0f, 0.0f, 1.0f}, "RoseCurves", 0.005f);
        MorphParametric2DPlot(&device, Hypotrochoid, 0, t, (MVec3){0.6, 0.0, 0.3}, "Hypotrochoid", 0.05f);
        MorphPlotList(&device, circle_x, circle_y, count, (MVec3){1.0f, 1.0f, 0.0f}, "HypoCircle");
        MorphPlotList(&device, line_x, line_y, 2, (MVec3){0.0f, 1.0f, 0.0f}, "HypoTrace");

        then = MorphTimeSinceCreation(&device);
        MorphPhantomShow(&device);
    }
    MorphDestroyDevice(&device);
    return 0;
}