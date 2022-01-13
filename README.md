# Morph
A simple graph plotter API that is readily callable from C and is easy to interface with other languages.

A simple graph plotter that can plot real valued function (discontinuous plotting is restricted to one function for now). <br><br>
Morph stands for Morphism in a sense that graph plotting is basically mapping points obtained from function to screen pixels:D.  <br>

# Build 
open(``Morph.sln``).then(``Ctrl+F5``)

## Linux Build 
Just provide required glfw's *.c files to cc and some -I\* and -L\* things and it compiles. If not clear, wait for build.sh or cmakelists.txt file. 

# Demo -> Func Plot
```c
#define _CRT_SECURE_NO_WARNINGS

#include "./Morph.h"
#include <math.h>

double xcube(double x)
{
    return x * x * x;
}

int main()
{
    MorphPlotDevice device = MorphCreateDevice();
    // Add Gaussian * 4 func here 
    MorphPlotFunc(&device, xcube, 0.0f, 0.0f, 1.0f,-10.0f,10.0f, "x^3",0.0f);
    MorphPlotFunc(&device, sqrt, 1.0f, 0.0f, 1.0f,0.0f,200.0f, "sqrt",0.1f);
    MorphShow(&device);
    return 0;
}
```

# Output 
<p align="left">
  <img src="multigraph.png">

# Demo -> List Plot 

 ```c
#include "./Morph.h"

int main()
{
    MorphPlotDevice device = MorphCreateDevice();
    
    float x[] = {1, 3, 4, 6, 7};
    float y[] = {3, -4, 3, -2, 0};
    MorphPlotList(&device, x, y, 5, 0.3f, 0.2f, 0.9f, "MagicFunc");
   
    MorphShow(&device);
    return 0;
}
```

# Output 
<p align="left">
    <img src="listplot.png">

#Demo -> Simple animation 
```c
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
```
## Output
<p align="left">
    <img src="./hypotrochoid.gif">
    
