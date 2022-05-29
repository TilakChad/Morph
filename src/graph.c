#define _CRT_SECURE_NO_WARNINGS

#include "./Morph.h"
#include <math.h>

double Heaviside(double x)
{
    return x <= 0 ? 0 : 1;
}

static MVec2 RoseCurves(double t)
{

    int   k = 10;
    int   a = 5;
    float r = a * cos(k * t);
    return (MVec2){.x = r * cos(t), .y = r * sin(t)};
}

static MVec2 RoseCurvesCircleVersion(double t)
{

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


double expGamma(double t)
{
    return pow(t,  2.2);
}

double linear(double t)
{
    return t; 
}

MVec2 HypoDesmos(double t)
{
    float r = 54; 
    float R = 140; 
    float o = 0.6f; 
    float d = o*r; 
    MVec2 vec; 
    vec.x = 0.1f * ((R - r) * cos(t) + d * cos ((R-r)/r * t));
    vec.y = 0.1f * ((R - r) * sin(t) - d * sin((R - r) / r * t));
    return vec; 
}

MVec2 intcurve(double t)
{
    return (MVec2){t - 1.6 * cos(24 * t), t - 1.6 * sin(25 * t)};
}

int main() 
{
    MorphPlotDevice device = MorphCreateDevice(); 
    double          now    = MorphTimeSinceCreation(&device); 
    double          then   = now; 
    float           step   = 1; 
    float           total  = -3.0f;
    while (!MorphShouldWindowClose(&device))
    {
        now = MorphTimeSinceCreation(&device);
        MorphResetPlotting(&device);
        MorphParametric2DPlot(&device, intcurve, 0, total, (MVec3){0.0f, 0.4921f, 0.0f}, "Hypotrochoid", 0.01f);
        total += step * (now - then);
        then = now;
        if (total > 30.0f)
            step = -1; 
        MorphPhantomShow(&device);
    }
    MorphDestroyDevice(&device);
}