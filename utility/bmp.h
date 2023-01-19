#ifndef BMP_H_
#define BMP_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct BMP
{
    // Storage format is BGR
    bool     topdown; // false means bottomup
    uint8_t  depth;
    uint32_t width;
    uint32_t height;
    uint32_t channels; // will usually be 3 in the BGR format
    uint8_t *buffer;
    uint64_t pos;
    uint64_t capacity;
} BMP;

void InitBMP(BMP *bmp, uint64_t capacity, uint32_t channels, bool topdown);
void WriteBMPHeader(BMP *bmp);
void DestroyBMP(BMP *bmp);
void WriteBMPData(BMP *bmp, uint8_t *image_data, uint32_t width, uint32_t height, uint32_t channels);
void WriteBMPToFile(BMP* bmp, const char* file_path);

#endif // BMP_H_
