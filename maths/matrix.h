#pragma once

#include <stdint.h>

typedef struct
{
    float elem[4][4];
} Mat4;

Mat4 OrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4 matrix       = {{0}};
    matrix.elem[0][0] = 2.0f / (right - left);
    matrix.elem[1][1] = 2.0f / (top - bottom);
    matrix.elem[2][2] = -2.0f / (zFar - zNear);

    matrix.elem[0][3] = (right + left) / (left - right);
    matrix.elem[1][3] = (top + bottom) / (bottom - top);
    matrix.elem[2][3] = (zFar + zNear) / (zNear - zFar);

    matrix.elem[3][3] = 1.0f;
    return matrix;
}

Mat4 ScalarMatrix(float x, float y, float z)
{
    Mat4 matrix       = {{0}};
    matrix.elem[0][0] = x;
    matrix.elem[1][1] = y;
    matrix.elem[2][2] = z;
    matrix.elem[3][3] = 1.0f;
    return matrix;
}

Mat4 TranslationMatrix(float x, float y, float z)
{
    Mat4 matrix = {{0}};

    matrix.elem[0][0] = 1.0f; 
    matrix.elem[1][1] = 1.0f; 
    matrix.elem[2][2] = 1.0f; 

    matrix.elem[0][3] = x; 
    matrix.elem[1][3] = y; 
    matrix.elem[2][3] = z; 
    matrix.elem[3][3] = 1.0f; 
    return matrix; 
}

Mat4 TransposeMatrix(Mat4* mat)
{
    Mat4 matrix = {{0}};
    for (uint8_t row = 0; row < 4; ++row)
        for (uint8_t col = 0; col < 4; ++col)
            matrix.elem[row][col] = mat->elem[col][row];
    return matrix; 
}

Mat4 MatrixMultiply(Mat4 *mat1, Mat4 *mat2)
{
    Mat4  matrix = {{0}};
    float var = 0;
    for (uint8_t row = 0; row < 4; ++row)
    {
        for (uint8_t k = 0; k < 4; ++k)
        {
            var = mat1->elem[row][k];
            for (uint8_t col = 0; col < 4; ++col)
                matrix.elem[row][col] += var * mat2->elem[k][col];
        }
    }
    return matrix;
}

Mat4 IdentityMatrix()
{
    Mat4 matrix = {{0}};
    matrix.elem[0][0] = 1.0f; 
    matrix.elem[1][1] = 1.0f; 
    matrix.elem[2][2] = 1.0f; 
    matrix.elem[3][3] = 1.0f; 
    return matrix; 
}