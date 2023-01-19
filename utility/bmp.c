#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp.h"

/* int main(int argc, char **argv) */
/* { */
/*     /\* char  buffer[] = {0x42, 0x4D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, *\/ */

/*     /\*                  0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0x01, 0x00, *\/ */
/*     /\*                  0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, *\/ */
/*     /\*                  0x13, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, *\/ */

/*     /\*                  0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, *\/ */

/*     /\*                  0x00, 0x00, *\/ */

/*     /\*                  0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00}; *\/ */

/*     /\* FILE *fp       = fopen("2pixels.bmp", "wb"); *\/ */
/*     /\* if (!fp) *\/ */
/*     /\* { *\/ */
/*     /\*     fprintf(stderr, "Failed to open file for writing"); *\/ */
/*     /\*     return -1; *\/ */
/*     /\* } *\/ */
/*     /\* fwrite(buffer, sizeof(buffer), 1, fp); *\/ */
/*     /\* fclose(fp); *\/ */
/*     BMP bmp; */
/*     InitBMP(&bmp, 100, 3,false); */
/*     uint8_t image[] = { */
/*     0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, */
/*     0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00 */
/* }; */
/*     WriteBMPHeader(&bmp); */
/*     WriteBMPData(&bmp, image, 2, 2, 3); */
/*     WriteBMPToFile(&bmp, "writer.bmp"); */
/*     return 0; */
/* } */

void InitBMP(BMP *bmp, uint64_t capacity, uint32_t channels, bool topdown)
{
    memset(bmp, 0, sizeof(*bmp));
    bmp->capacity = capacity;
    bmp->channels = channels;
    bmp->buffer   = malloc(sizeof(*bmp->buffer) * bmp->capacity);
    if (!bmp->buffer)
    {
        memset(bmp, 0, sizeof(*bmp));
        return;
    }
    bmp->topdown = topdown;
}

void WriteBMPHeader(BMP *bmp)
{
    memset(bmp->buffer, 0, 54);
    // BMP Header
    bmp->buffer[0x00] = 0x42;
    bmp->buffer[0x01] = 0x4D;

    // Size of the bmp file (54 bytes + actual data) at 0x02
    // Size to be filled later
    // TODO :: Fill it later
    // Offset of actual bitmap data which is 54
    bmp->buffer[0x0A] = 0x36;

    // DIB Header
    bmp->buffer[0x0E] = 0x28; // Number of bytes in the DIB Header section
    // TODO :: Starting at offset 0x12h and 0x16h, fill the width and height of bitmap in pixels in little-endian order

    // Number of color planes
    bmp->buffer[0x1A] = 0x01;
    // Number of bits per pixel
    bmp->buffer[0x1C] =
        0x18; // 24 bits per pixel -- Only channels == 3 supported for now due to jpeg not supporting alpha either way

    // TODO :: Fill the size of raw bitmap data at offset 0x22h including padding bytes

    // Resolution metrics
    bmp->buffer[0x26] = 0x13;
    bmp->buffer[0x27] = 0x0B;
    bmp->buffer[0x2A] = 0x13;
    bmp->buffer[0x2B] = 0x0B;

    // Header written with 4 places to be filled
    bmp->pos = 0x36;
}

void WriteBMPData(BMP *bmp, uint8_t *image_data, uint32_t width, uint32_t height, uint32_t channels)
{
    // TODO :: Maybe change it?
    *(uint32_t *)(bmp->buffer + 0x12) = width;
    *(uint32_t *)(bmp->buffer + 0x16) = height;

    if (bmp->topdown)
        *(uint32_t *)(bmp->buffer + 0x16) = -(int32_t)height;

    // So we have width pixels wide * channels = no.of bytes required
    // Make it to align at 4
    // Use some bit twiddling here and there
    uint32_t hbytes                   = width * channels;
    hbytes                            = (hbytes + 3) & ~(4 - 1); // I guess it shoudl align it to the nearest power of 4
    uint32_t header_size              = hbytes * height + 54;
    *(uint32_t *)(bmp->buffer + 0x02) = header_size;
    *(uint32_t *)(bmp->buffer + 0x22) = hbytes * height;

    // Now write the actual data
    // OOF Extra work for BGR Space -> RGB Space conversion
    uint8_t rgbspace[4];

    if (hbytes * height >= bmp->capacity)
    {
        // Could have written to the memory directly but, its good to have modular approach
        fprintf(stderr, "Not enough memory allocated for bmp creation");
        return;
    }

    uint8_t swap;
    for (uint32_t h = 0; h < height; ++h)
    {
        for (uint32_t w = 0; w < width; ++w)
        {
            memcpy(rgbspace, image_data, channels);
            swap        = rgbspace[2];
            rgbspace[2] = rgbspace[0];
            rgbspace[0] = swap;
            image_data  = image_data + channels;
            // write this buffer to the output of the bitmap data
            memcpy(bmp->buffer + bmp->pos + w * channels, rgbspace, channels);
        }
        bmp->pos = bmp->pos + hbytes;
    }
}

void WriteBMPToFile(BMP *bmp, const char *file_path)
{
    FILE *fp = fopen(file_path, "wb");
    if (!fp)
    {
        fprintf(stderr, "Failed to open %s for writing.", file_path);
        return;
    }

    fwrite(bmp->buffer, sizeof(*bmp->buffer), bmp->pos, fp);
    fclose(fp);

    fprintf(stdout, "\nBMP with file size %lu successfully written to %s.", bmp->pos, file_path);
}

void DestroyBMP(BMP *bmp)
{
    free(bmp->buffer);
}
