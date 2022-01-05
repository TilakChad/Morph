#include "../include/Morph.h"

double what(double x)
{
    return x * x * x;
}

int main()
{
    MorphPlotDevice device = MorphCreateDevice();
    MorphPlotFunc(&device, what, 0.0f, 0.0f, 1.0f, "sqrt");
    MorphShow(&device);
    return 0;
}