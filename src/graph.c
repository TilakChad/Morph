#include "../include/Graphy.h"

double what(double x)
{
    return x * x * x;
}

int main()
{
    PlotDevice device = CreatePlottingDevice();
    APIPlotFunc(device, what, 0.0f, 0.0f, 1.0f, "sqrt");
    ShowPlot(&device);
    return 0;
}