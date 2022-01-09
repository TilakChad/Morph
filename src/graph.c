#define _CRT_SECURE_NO_WARNINGS

#include "./Morph.h"
#include <math.h>

double what(double x)
{
    return x * x * x;
}

int main()
{
    MorphPlotDevice device = MorphCreateDevice();
    MorphPlotFunc(&device, what, 0.0f, 0.0f, 1.0f, "x^3");
    MorphPlotFunc(&device, sqrt, 0.0f, 1.0f, 1.0f, "sqrt");

    MorphShow(&device);
    return 0;
}