#define _CRT_SECURE_NO_WARNINGS
#define _GNU_SOURCE

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "./Morph.h"

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <time.h>
#include <unistd.h>
#endif

// copy paste stb_truetype
//
// stb_truetype.h - v1.26 - public domain
// authored from 2009-2021 by Sean Barrett / RAD Game Tools
//
// =======================================================================
//
//    NO SECURITY GUARANTEE -- DO NOT USE THIS ON UNTRUSTED FONT FILES
//
// This library does no range checking of the offsets found in the file,
// meaning an attacker can use it to read arbitrary memory.
//
// =======================================================================
//
//   This library processes TrueType files:
//        parse files
//        extract glyph metrics
//        extract glyph shapes
//        render glyphs to one-channel bitmaps with antialiasing (box filter)
//        render glyphs to one-channel SDF bitmaps (signed-distance field/function)
//
//   Todo:
//        non-MS cmaps
//        crashproof on bad data
//        hinting? (no longer patented)
//        cleartype-style AA?
//        optimize: use simple memory allocator for intermediates
//        optimize: build edge-list directly from curves
//        optimize: rasterize directly from curves?
//
// ADDITIONAL CONTRIBUTORS
//
//   Mikko Mononen: compound shape support, more cmap formats
//   Tor Andersson: kerning, subpixel rendering
//   Dougall Johnson: OpenType / Type 2 font handling
//   Daniel Ribeiro Maciel: basic GPOS-based kerning
//
//   Misc other:
//       Ryan Gordon
//       Simon Glass
//       github:IntellectualKitty
//       Imanol Celaya
//       Daniel Ribeiro Maciel
//
//   Bug/warning reports/fixes:
//       "Zer" on mollyrocket       Fabian "ryg" Giesen   github:NiLuJe
//       Cass Everitt               Martins Mozeiko       github:aloucks
//       stoiko (Haemimont Games)   Cap Petschulat        github:oyvindjam
//       Brian Hook                 Omar Cornut           github:vassvik
//       Walter van Niftrik         Ryan Griege
//       David Gow                  Peter LaValle
//       David Given                Sergey Popov
//       Ivan-Assen Ivanov          Giumo X. Clanjor
//       Anthony Pesch              Higor Euripedes
//       Johan Duparc               Thomas Fields
//       Hou Qiming                 Derek Vinyard
//       Rob Loach                  Cort Stratton
//       Kenney Phillis Jr.         Brian Costabile
//       Ken Voskuil (kaesve)
//
// VERSION HISTORY
//
//   1.26 (2021-08-28) fix broken rasterizer
//   1.25 (2021-07-11) many fixes
//   1.24 (2020-02-05) fix warning
//   1.23 (2020-02-02) query SVG data for glyphs; query whole kerning table (but only kern not GPOS)
//   1.22 (2019-08-11) minimize missing-glyph duplication; fix kerning if both 'GPOS' and 'kern' are defined
//   1.21 (2019-02-25) fix warning
//   1.20 (2019-02-07) PackFontRange skips missing codepoints; GetScaleFontVMetrics()
//   1.19 (2018-02-11) GPOS kerning, STBTT_fmod
//   1.18 (2018-01-29) add missing function
//   1.17 (2017-07-23) make more arguments const; doc fix
//   1.16 (2017-07-12) SDF support
//   1.15 (2017-03-03) make more arguments const
//   1.14 (2017-01-16) num-fonts-in-TTC function
//   1.13 (2017-01-02) support OpenType fonts, certain Apple fonts
//   1.12 (2016-10-25) suppress warnings about casting away const with -Wcast-qual
//   1.11 (2016-04-02) fix unused-variable warning
//   1.10 (2016-04-02) user-defined fabs(); rare memory leak; remove duplicate typedef
//   1.09 (2016-01-16) warning fix; avoid crash on outofmem; use allocation userdata properly
//   1.08 (2015-09-13) document stbtt_Rasterize(); fixes for vertical & horizontal edges
//   1.07 (2015-08-01) allow PackFontRanges to accept arrays of sparse codepoints;
//                     variant PackFontRanges to pack and render in separate phases;
//                     fix stbtt_GetFontOFfsetForIndex (never worked for non-0 input?);
//                     fixed an assert() bug in the new rasterizer
//                     replace assert() with STBTT_assert() in new rasterizer
//
//   Full history can be found at the end of this file.
//
// LICENSE
//
//   See end of file for license information.
//
// USAGE
//
//   Include this file in whatever places need to refer to it. In ONE C/C++
//   file, write:
//      #define STB_TRUETYPE_IMPLEMENTATION
//   before the #include of this file. This expands out the actual
//   implementation into that C/C++ file.
//
//   To make the implementation private to the file that generates the implementation,
//      #define STBTT_STATIC
//
//   Simple 3D API (don't ship this, but it's fine for tools and quick start)
//           stbtt_BakeFontBitmap()               -- bake a font to a bitmap for use as texture
//           stbtt_GetBakedQuad()                 -- compute quad to draw for a given char
//
//   Improved 3D API (more shippable):
//           #include "stb_rect_pack.h"           -- optional, but you really want it
//           stbtt_PackBegin()
//           stbtt_PackSetOversampling()          -- for improved quality on small fonts
//           stbtt_PackFontRanges()               -- pack and renders
//           stbtt_PackEnd()
//           stbtt_GetPackedQuad()
//
//   "Load" a font file from a memory buffer (you have to keep the buffer loaded)
//           stbtt_InitFont()
//           stbtt_GetFontOffsetForIndex()        -- indexing for TTC font collections
//           stbtt_GetNumberOfFonts()             -- number of fonts for TTC font collections
//
//   Render a unicode codepoint to a bitmap
//           stbtt_GetCodepointBitmap()           -- allocates and returns a bitmap
//           stbtt_MakeCodepointBitmap()          -- renders into bitmap you provide
//           stbtt_GetCodepointBitmapBox()        -- how big the bitmap must be
//
//   Character advance/positioning
//           stbtt_GetCodepointHMetrics()
//           stbtt_GetFontVMetrics()
//           stbtt_GetFontVMetricsOS2()
//           stbtt_GetCodepointKernAdvance()
//
//   Starting with version 1.06, the rasterizer was replaced with a new,
//   faster and generally-more-precise rasterizer. The new rasterizer more
//   accurately measures pixel coverage for anti-aliasing, except in the case
//   where multiple shapes overlap, in which case it overestimates the AA pixel
//   coverage. Thus, anti-aliasing of intersecting shapes may look wrong. If
//   this turns out to be a problem, you can re-enable the old rasterizer with
//        #define STBTT_RASTERIZER_VERSION 1
//   which will incur about a 15% speed hit.
//
// ADDITIONAL DOCUMENTATION
//
//   Immediately after this block comment are a series of sample programs.
//
//   After the sample programs is the "header file" section. This section
//   includes documentation for each API function.
//
//   Some important concepts to understand to use this library:
//
//      Codepoint
//         Characters are defined by unicode codepoints, e.g. 65 is
//         uppercase A, 231 is lowercase c with a cedilla, 0x7e30 is
//         the hiragana for "ma".
//
//      Glyph
//         A visual character shape (every codepoint is rendered as
//         some glyph)
//
//      Glyph index
//         A font-specific integer ID representing a glyph
//
//      Baseline
//         Glyph shapes are defined relative to a baseline, which is the
//         bottom of uppercase characters. Characters extend both above
//         and below the baseline.
//
//      Current Point
//         As you draw text to the screen, you keep track of a "current point"
//         which is the origin of each character. The current point's vertical
//         position is the baseline. Even "baked fonts" use this model.
//
//      Vertical Font Metrics
//         The vertical qualities of the font, used to vertically position
//         and space the characters. See docs for stbtt_GetFontVMetrics.
//
//      Font Size in Pixels or Points
//         The preferred interface for specifying font sizes in stb_truetype
//         is to specify how tall the font's vertical extent should be in pixels.
//         If that sounds good enough, skip the next paragraph.
//
//         Most font APIs instead use "points", which are a common typographic
//         measurement for describing font size, defined as 72 points per inch.
//         stb_truetype provides a point API for compatibility. However, true
//         "per inch" conventions don't make much sense on computer displays
//         since different monitors have different number of pixels per
//         inch. For example, Windows traditionally uses a convention that
//         there are 96 pixels per inch, thus making 'inch' measurements have
//         nothing to do with inches, and thus effectively defining a point to
//         be 1.333 pixels. Additionally, the TrueType font data provides
//         an explicit scale factor to scale a given font's glyphs to points,
//         but the author has observed that this scale factor is often wrong
//         for non-commercial fonts, thus making fonts scaled in points
//         according to the TrueType spec incoherently sized in practice.
//
// DETAILED USAGE:
//
//  Scale:
//    Select how high you want the font to be, in points or pixels.
//    Call ScaleForPixelHeight or ScaleForMappingEmToPixels to compute
//    a scale factor SF that will be used by all other functions.
//
//  Baseline:
//    You need to select a y-coordinate that is the baseline of where
//    your text will appear. Call GetFontBoundingBox to get the baseline-relative
//    bounding box for all characters. SF*-y0 will be the distance in pixels
//    that the worst-case character could extend above the baseline, so if
//    you want the top edge of characters to appear at the top of the
//    screen where y=0, then you would set the baseline to SF*-y0.
//
//  Current point:
//    Set the current point where the first character will appear. The
//    first character could extend left of the current point; this is font
//    dependent. You can either choose a current point that is the leftmost
//    point and hope, or add some padding, or check the bounding box or
//    left-side-bearing of the first character to be displayed and set
//    the current point based on that.
//
//  Displaying a character:
//    Compute the bounding box of the character. It will contain signed values
//    relative to <current_point, baseline>. I.e. if it returns x0,y0,x1,y1,
//    then the character should be displayed in the rectangle from
//    <current_point+SF*x0, baseline+SF*y0> to <current_point+SF*x1,baseline+SF*y1).
//
//  Advancing for the next character:
//    Call GlyphHMetrics, and compute 'current_point += SF * advance'.
//
//
// ADVANCED USAGE
//
//   Quality:
//
//    - Use the functions with Subpixel at the end to allow your characters
//      to have subpixel positioning. Since the font is anti-aliased, not
//      hinted, this is very import for quality. (This is not possible with
//      baked fonts.)
//
//    - Kerning is now supported, and if you're supporting subpixel rendering
//      then kerning is worth using to give your text a polished look.
//
//   Performance:
//
//    - Convert Unicode codepoints to glyph indexes and operate on the glyphs;
//      if you don't do this, stb_truetype is forced to do the conversion on
//      every call.
//
//    - There are a lot of memory allocations. We should modify it to take
//      a temp buffer and allocate from the temp buffer (without freeing),
//      should help performance a lot.
//
// NOTES
//
//   The system uses the raw data found in the .ttf file without changing it
//   and without building auxiliary data structures. This is a bit inefficient
//   on little-endian systems (the data is big-endian), but assuming you're
//   caching the bitmaps or glyph shapes this shouldn't be a big deal.
//
//   It appears to be very hard to programmatically determine what font a
//   given file is in a general way. I provide an API for this, but I don't
//   recommend it.
//
//
// PERFORMANCE MEASUREMENTS FOR 1.06:
//
//                      32-bit     64-bit
//   Previous release:  8.83 s     7.68 s
//   Pool allocations:  7.72 s     6.34 s
//   Inline sort     :  6.54 s     5.65 s
//   New rasterizer  :  5.63 s     5.00 s

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
////
////  SAMPLE PROGRAMS
////
//
//  Incomplete text-in-3d-api example, which draws quads properly aligned to be lossless.
//  See "tests/truetype_demo`.c" for a complete version.
#if 0
#define STB_TRUETYPE_IMPLEMENTATION // force following include to generate implementation
#include "stb_truetype.h"

unsigned char ttf_buffer[1<<20];
unsigned char temp_bitmap[512*512];

stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
GLuint ftex;

void my_stbtt_initfont(void)
{
   fread(ttf_buffer, 1, 1<<20, fopen("c:/windows/fonts/times.ttf", "rb"));
   stbtt_BakeFontBitmap(ttf_buffer,0, 32.0, temp_bitmap,512,512, 32,96, cdata); // no guarantee this fits!
   // can free ttf_buffer at this point
   glGenTextures(1, &ftex);
   glBindTexture(GL_TEXTURE_2D, ftex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
   // can free temp_bitmap at this point
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void my_stbtt_print(float x, float y, char *text)
{
   // assume orthographic projection with units = screen pixels, origin at top left
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, ftex);
   glBegin(GL_QUADS);
   while (*text) {
      if (*text >= 32 && *text < 128) {
         stbtt_aligned_quad q;
         stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
         glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
         glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
         glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
         glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
      }
      ++text;
   }
   glEnd();
}
#endif
//
//
//////////////////////////////////////////////////////////////////////////////
//
// Complete program (this compiles): get a single bitmap, print as ASCII art
//
#if 0
#include <stdio.h>
#define STB_TRUETYPE_IMPLEMENTATION // force following include to generate implementation
#include "stb_truetype.h"

char ttf_buffer[1<<25];

int main(int argc, char **argv)
{
   stbtt_fontinfo font;
   unsigned char *bitmap;
   int w,h,i,j,c = (argc > 1 ? atoi(argv[1]) : 'a'), s = (argc > 2 ? atoi(argv[2]) : 20);

   fread(ttf_buffer, 1, 1<<25, fopen(argc > 3 ? argv[3] : "c:/windows/fonts/arialbd.ttf", "rb"));

   stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0));
   bitmap = stbtt_GetCodepointBitmap(&font, 0,stbtt_ScaleForPixelHeight(&font, s), c, &w, &h, 0,0);

   for (j=0; j < h; ++j) {
      for (i=0; i < w; ++i)
         putchar(" .:ioVM@"[bitmap[j*w+i]>>5]);
      putchar('\n');
   }
   return 0;
}
#endif
//
// Output:
//
//     .ii.
//    @@@@@@.
//   V@Mio@@o
//   :i.  V@V
//     :oM@@M
//   :@@@MM@M
//   @@o  o@M
//  :@@.  M@M
//   @@@o@@@@
//   :M@@V:@@.
//
//////////////////////////////////////////////////////////////////////////////
//
// Complete program: print "Hello World!" banner, with bugs
//
#if 0
char buffer[24<<20];
unsigned char screen[20][79];

int main(int arg, char **argv)
{
   stbtt_fontinfo font;
   int i,j,ascent,baseline,ch=0;
   float scale, xpos=2; // leave a little padding in case the character extends left
   char *text = "Heljo World!"; // intentionally misspelled to show 'lj' brokenness

   fread(buffer, 1, 1000000, fopen("c:/windows/fonts/arialbd.ttf", "rb"));
   stbtt_InitFont(&font, buffer, 0);

   scale = stbtt_ScaleForPixelHeight(&font, 15);
   stbtt_GetFontVMetrics(&font, &ascent,0,0);
   baseline = (int) (ascent*scale);

   while (text[ch]) {
      int advance,lsb,x0,y0,x1,y1;
      float x_shift = xpos - (float) floor(xpos);
      stbtt_GetCodepointHMetrics(&font, text[ch], &advance, &lsb);
      stbtt_GetCodepointBitmapBoxSubpixel(&font, text[ch], scale,scale,x_shift,0, &x0,&y0,&x1,&y1);
      stbtt_MakeCodepointBitmapSubpixel(&font, &screen[baseline + y0][(int) xpos + x0], x1-x0,y1-y0, 79, scale,scale,x_shift,0, text[ch]);
      // note that this stomps the old data, so where character boxes overlap (e.g. 'lj') it's wrong
      // because this API is really for baking character bitmaps into textures. if you want to render
      // a sequence of characters, you really need to render each bitmap to a temp buffer, then
      // "alpha blend" that into the working buffer
      xpos += (advance * scale);
      if (text[ch+1])
         xpos += scale*stbtt_GetCodepointKernAdvance(&font, text[ch],text[ch+1]);
      ++ch;
   }

   for (j=0; j < 20; ++j) {
      for (i=0; i < 78; ++i)
         putchar(" .:ioVM@"[screen[j][i]>>5]);
      putchar('\n');
   }

   return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
////
////   INTEGRATION WITH YOUR CODEBASE
////
////   The following sections allow you to supply alternate definitions
////   of C library functions used by stb_truetype, e.g. if you don't
////   link with the C runtime library.

#ifdef STB_TRUETYPE_IMPLEMENTATION
// #define your own (u)stbtt_int8/16/32 before including to override this
#ifndef stbtt_uint8
typedef unsigned char  stbtt_uint8;
typedef signed char    stbtt_int8;
typedef unsigned short stbtt_uint16;
typedef signed short   stbtt_int16;
typedef unsigned int   stbtt_uint32;
typedef signed int     stbtt_int32;
#endif

typedef char stbtt__check_size32[sizeof(stbtt_int32) == 4 ? 1 : -1];
typedef char stbtt__check_size16[sizeof(stbtt_int16) == 2 ? 1 : -1];

// e.g. #define your own STBTT_ifloor/STBTT_iceil() to avoid math.h
#ifndef STBTT_ifloor
#include <math.h>
#define STBTT_ifloor(x) ((int)floor(x))
#define STBTT_iceil(x) ((int)ceil(x))
#endif

#ifndef STBTT_sqrt
#include <math.h>
#define STBTT_sqrt(x) sqrt(x)
#define STBTT_pow(x, y) pow(x, y)
#endif

#ifndef STBTT_fmod
#include <math.h>
#define STBTT_fmod(x, y) fmod(x, y)
#endif

#ifndef STBTT_cos
#include <math.h>
#define STBTT_cos(x) cos(x)
#define STBTT_acos(x) acos(x)
#endif

#ifndef STBTT_fabs
#include <math.h>
#define STBTT_fabs(x) fabs(x)
#endif

// #define your own functions "STBTT_malloc" / "STBTT_free" to avoid malloc.h
#ifndef STBTT_malloc
#include <stdlib.h>
#define STBTT_malloc(x, u) ((void)(u), malloc(x))
#define STBTT_free(x, u) ((void)(u), free(x))
#endif

#ifndef STBTT_assert
#include <assert.h>
#define STBTT_assert(x) assert(x)
#endif

#ifndef STBTT_strlen
#include <string.h>
#define STBTT_strlen(x) strlen(x)
#endif

#ifndef STBTT_memcpy
#include <string.h>
#define STBTT_memcpy memcpy
#define STBTT_memset memset
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////
////   INTERFACE
////
////

#ifndef __STB_INCLUDE_STB_TRUETYPE_H__
#define __STB_INCLUDE_STB_TRUETYPE_H__

#ifdef STBTT_STATIC
#define STBTT_DEF static
#else
#define STBTT_DEF extern
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    // private structure
    typedef struct
    {
        unsigned char *data;
        int            cursor;
        int            size;
    } stbtt__buf;

    //////////////////////////////////////////////////////////////////////////////
    //
    // TEXTURE BAKING API
    //
    // If you use this API, you only have to call two functions ever.
    //

    typedef struct
    {
        unsigned short x0, y0, x1, y1; // coordinates of bbox in bitmap
        float          xoff, yoff, xadvance;
    } stbtt_bakedchar;

    STBTT_DEF int stbtt_BakeFontBitmap(const unsigned char *data,
                                       int                  offset,       // font location (use offset=0 for plain .ttf)
                                       float                pixel_height, // height of font in pixels
                                       unsigned char *pixels, int pw, int ph, // bitmap to be filled in
                                       int first_char, int num_chars,         // characters to bake
                                       stbtt_bakedchar *chardata);            // you allocate this, it's num_chars long
    // if return is positive, the first unused row of the bitmap
    // if return is negative, returns the negative of the number of characters that fit
    // if return is 0, no characters fit and no rows were used
    // This uses a very crappy packing.

    typedef struct
    {
        float x0, y0, s0, t0; // top-left
        float x1, y1, s1, t1; // bottom-right
    } stbtt_aligned_quad;

    STBTT_DEF void stbtt_GetBakedQuad(const stbtt_bakedchar *chardata, int pw, int ph, // same data as above
                                      int                 char_index,                  // character to display
                                      float              *xpos,
                                      float              *ypos, // pointers to current position in screen pixel space
                                      stbtt_aligned_quad *q,    // output: quad to draw
                                      int opengl_fillrule);     // true if opengl fill rule; false if DX9 or earlier
    // Call GetBakedQuad with char_index = 'character - first_char', and it
    // creates the quad you need to draw and advances the current position.
    //
    // The coordinate system used assumes y increases downwards.
    //
    // Characters will extend both above and below the current position;
    // see discussion of "BASELINE" above.
    //
    // It's inefficient; you might want to c&p it and optimize it.

    STBTT_DEF void stbtt_GetScaledFontVMetrics(const unsigned char *fontdata, int index, float size, float *ascent,
                                               float *descent, float *lineGap);
    // Query the font vertical metrics without having to create a font first.

    //////////////////////////////////////////////////////////////////////////////
    //
    // NEW TEXTURE BAKING API
    //
    // This provides options for packing multiple fonts into one atlas, not
    // perfectly but better than nothing.

    typedef struct
    {
        unsigned short x0, y0, x1, y1; // coordinates of bbox in bitmap
        float          xoff, yoff, xadvance;
        float          xoff2, yoff2;
    } stbtt_packedchar;

    typedef struct stbtt_pack_context stbtt_pack_context;
    typedef struct stbtt_fontinfo     stbtt_fontinfo;
#ifndef STB_RECT_PACK_VERSION
    typedef struct stbrp_rect stbrp_rect;
#endif

    STBTT_DEF int stbtt_PackBegin(stbtt_pack_context *spc, unsigned char *pixels, int width, int height,
                                  int stride_in_bytes, int padding, void *alloc_context);
    // Initializes a packing context stored in the passed-in stbtt_pack_context.
    // Future calls using this context will pack characters into the bitmap passed
    // in here: a 1-channel bitmap that is width * height. stride_in_bytes is
    // the distance from one row to the next (or 0 to mean they are packed tightly
    // together). "padding" is the amount of padding to leave between each
    // character (normally you want '1' for bitmaps you'll use as textures with
    // bilinear filtering).
    //
    // Returns 0 on failure, 1 on success.

    STBTT_DEF void stbtt_PackEnd(stbtt_pack_context *spc);
    // Cleans up the packing context and frees all memory.

#define STBTT_POINT_SIZE(x) (-(x))

    STBTT_DEF int stbtt_PackFontRange(stbtt_pack_context *spc, const unsigned char *fontdata, int font_index,
                                      float font_size, int first_unicode_char_in_range, int num_chars_in_range,
                                      stbtt_packedchar *chardata_for_range);
    // Creates character bitmaps from the font_index'th font found in fontdata (use
    // font_index=0 if you don't know what that is). It creates num_chars_in_range
    // bitmaps for characters with unicode values starting at first_unicode_char_in_range
    // and increasing. Data for how to render them is stored in chardata_for_range;
    // pass these to stbtt_GetPackedQuad to get back renderable quads.
    //
    // font_size is the full height of the character from ascender to descender,
    // as computed by stbtt_ScaleForPixelHeight. To use a point size as computed
    // by stbtt_ScaleForMappingEmToPixels, wrap the point size in STBTT_POINT_SIZE()
    // and pass that result as 'font_size':
    //       ...,                  20 , ... // font max minus min y is 20 pixels tall
    //       ..., STBTT_POINT_SIZE(20), ... // 'M' is 20 pixels tall

    typedef struct
    {
        float font_size;
        int   first_unicode_codepoint_in_range; // if non-zero, then the chars are continuous, and this is the first
                                                // codepoint
        int              *array_of_unicode_codepoints; // if non-zero, then this is an array of unicode codepoints
        int               num_chars;
        stbtt_packedchar *chardata_for_range;         // output
        unsigned char     h_oversample, v_oversample; // don't set these, they're used internally
    } stbtt_pack_range;

    STBTT_DEF int stbtt_PackFontRanges(stbtt_pack_context *spc, const unsigned char *fontdata, int font_index,
                                       stbtt_pack_range *ranges, int num_ranges);
    // Creates character bitmaps from multiple ranges of characters stored in
    // ranges. This will usually create a better-packed bitmap than multiple
    // calls to stbtt_PackFontRange. Note that you can call this multiple
    // times within a single PackBegin/PackEnd.

    STBTT_DEF void stbtt_PackSetOversampling(stbtt_pack_context *spc, unsigned int h_oversample,
                                             unsigned int v_oversample);
    // Oversampling a font increases the quality by allowing higher-quality subpixel
    // positioning, and is especially valuable at smaller text sizes.
    //
    // This function sets the amount of oversampling for all following calls to
    // stbtt_PackFontRange(s) or stbtt_PackFontRangesGatherRects for a given
    // pack context. The default (no oversampling) is achieved by h_oversample=1
    // and v_oversample=1. The total number of pixels required is
    // h_oversample*v_oversample larger than the default; for example, 2x2
    // oversampling requires 4x the storage of 1x1. For best results, render
    // oversampled textures with bilinear filtering. Look at the readme in
    // stb/tests/oversample for information about oversampled fonts
    //
    // To use with PackFontRangesGather etc., you must set it before calls
    // call to PackFontRangesGatherRects.

    STBTT_DEF void stbtt_PackSetSkipMissingCodepoints(stbtt_pack_context *spc, int skip);
    // If skip != 0, this tells stb_truetype to skip any codepoints for which
    // there is no corresponding glyph. If skip=0, which is the default, then
    // codepoints without a glyph recived the font's "missing character" glyph,
    // typically an empty box by convention.

    STBTT_DEF void stbtt_GetPackedQuad(const stbtt_packedchar *chardata, int pw, int ph, // same data as above
                                       int                 char_index,                   // character to display
                                       float              *xpos,
                                       float              *ypos, // pointers to current position in screen pixel space
                                       stbtt_aligned_quad *q,    // output: quad to draw
                                       int                 align_to_integer);

    STBTT_DEF int  stbtt_PackFontRangesGatherRects(stbtt_pack_context *spc, const stbtt_fontinfo *info,
                                                   stbtt_pack_range *ranges, int num_ranges, stbrp_rect *rects);
    STBTT_DEF void stbtt_PackFontRangesPackRects(stbtt_pack_context *spc, stbrp_rect *rects, int num_rects);
    STBTT_DEF int  stbtt_PackFontRangesRenderIntoRects(stbtt_pack_context *spc, const stbtt_fontinfo *info,
                                                       stbtt_pack_range *ranges, int num_ranges, stbrp_rect *rects);
    // Calling these functions in sequence is roughly equivalent to calling
    // stbtt_PackFontRanges(). If you more control over the packing of multiple
    // fonts, or if you want to pack custom data into a font texture, take a look
    // at the source to of stbtt_PackFontRanges() and create a custom version
    // using these functions, e.g. call GatherRects multiple times,
    // building up a single array of rects, then call PackRects once,
    // then call RenderIntoRects repeatedly. This may result in a
    // better packing than calling PackFontRanges multiple times
    // (or it may not).

    // this is an opaque structure that you shouldn't mess with which holds
    // all the context needed from PackBegin to PackEnd.
    struct stbtt_pack_context
    {
        void          *user_allocator_context;
        void          *pack_info;
        int            width;
        int            height;
        int            stride_in_bytes;
        int            padding;
        int            skip_missing;
        unsigned int   h_oversample, v_oversample;
        unsigned char *pixels;
        void          *nodes;
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    // FONT LOADING
    //
    //

    STBTT_DEF int stbtt_GetNumberOfFonts(const unsigned char *data);
    // This function will determine the number of fonts in a font file.  TrueType
    // collection (.ttc) files may contain multiple fonts, while TrueType font
    // (.ttf) files only contain one font. The number of fonts can be used for
    // indexing with the previous function where the index is between zero and one
    // less than the total fonts. If an error occurs, -1 is returned.

    STBTT_DEF int stbtt_GetFontOffsetForIndex(const unsigned char *data, int index);
    // Each .ttf/.ttc file may have more than one font. Each font has a sequential
    // index number starting from 0. Call this function to get the font offset for
    // a given index; it returns -1 if the index is out of range. A regular .ttf
    // file will only define one font and it always be at offset 0, so it will
    // return '0' for index 0, and -1 for all other indices.

    // The following structure is defined publicly so you can declare one on
    // the stack or as a global or etc, but you should treat it as opaque.
    struct stbtt_fontinfo
    {
        void          *userdata;
        unsigned char *data;      // pointer to .ttf file
        int            fontstart; // offset of start of font

        int            numGlyphs; // number of glyphs, needed for range checking

        int            loca, head, glyf, hhea, hmtx, kern, gpos, svg; // table locations as offset from start of .ttf
        int            index_map;        // a cmap mapping for our chosen character encoding
        int            indexToLocFormat; // format needed to map from glyph index to glyph

        stbtt__buf     cff;         // cff font data
        stbtt__buf     charstrings; // the charstring index
        stbtt__buf     gsubrs;      // global charstring subroutines index
        stbtt__buf     subrs;       // private charstring subroutines index
        stbtt__buf     fontdicts;   // array of font dicts
        stbtt__buf     fdselect;    // map from glyph to fontdict
    };

    STBTT_DEF int stbtt_InitFont(stbtt_fontinfo *info, const unsigned char *data, int offset);
    // Given an offset into the file that defines a font, this function builds
    // the necessary cached info for the rest of the system. You must allocate
    // the stbtt_fontinfo yourself, and stbtt_InitFont will fill it out. You don't
    // need to do anything special to free it, because the contents are pure
    // value data with no additional data structures. Returns 0 on failure.

    //////////////////////////////////////////////////////////////////////////////
    //
    // CHARACTER TO GLYPH-INDEX CONVERSIOn

    STBTT_DEF int stbtt_FindGlyphIndex(const stbtt_fontinfo *info, int unicode_codepoint);
    // If you're going to perform multiple operations on the same character
    // and you want a speed-up, call this function with the character you're
    // going to process, then use glyph-based functions instead of the
    // codepoint-based functions.
    // Returns 0 if the character codepoint is not defined in the font.

    //////////////////////////////////////////////////////////////////////////////
    //
    // CHARACTER PROPERTIES
    //

    STBTT_DEF float stbtt_ScaleForPixelHeight(const stbtt_fontinfo *info, float pixels);
    // computes a scale factor to produce a font whose "height" is 'pixels' tall.
    // Height is measured as the distance from the highest ascender to the lowest
    // descender; in other words, it's equivalent to calling stbtt_GetFontVMetrics
    // and computing:
    //       scale = pixels / (ascent - descent)
    // so if you prefer to measure height by the ascent only, use a similar calculation.

    STBTT_DEF float stbtt_ScaleForMappingEmToPixels(const stbtt_fontinfo *info, float pixels);
    // computes a scale factor to produce a font whose EM size is mapped to
    // 'pixels' tall. This is probably what traditional APIs compute, but
    // I'm not positive.

    STBTT_DEF void stbtt_GetFontVMetrics(const stbtt_fontinfo *info, int *ascent, int *descent, int *lineGap);
    // ascent is the coordinate above the baseline the font extends; descent
    // is the coordinate below the baseline the font extends (i.e. it is typically negative)
    // lineGap is the spacing between one row's descent and the next row's ascent...
    // so you should advance the vertical position by "*ascent - *descent + *lineGap"
    //   these are expressed in unscaled coordinates, so you must multiply by
    //   the scale factor for a given size

    STBTT_DEF int stbtt_GetFontVMetricsOS2(const stbtt_fontinfo *info, int *typoAscent, int *typoDescent,
                                           int *typoLineGap);
    // analogous to GetFontVMetrics, but returns the "typographic" values from the OS/2
    // table (specific to MS/Windows TTF files).
    //
    // Returns 1 on success (table present), 0 on failure.

    STBTT_DEF void stbtt_GetFontBoundingBox(const stbtt_fontinfo *info, int *x0, int *y0, int *x1, int *y1);
    // the bounding box around all possible characters

    STBTT_DEF void stbtt_GetCodepointHMetrics(const stbtt_fontinfo *info, int codepoint, int *advanceWidth,
                                              int *leftSideBearing);
    // leftSideBearing is the offset from the current horizontal position to the left edge of the character
    // advanceWidth is the offset from the current horizontal position to the next horizontal position
    //   these are expressed in unscaled coordinates

    STBTT_DEF int stbtt_GetCodepointKernAdvance(const stbtt_fontinfo *info, int ch1, int ch2);
    // an additional amount to add to the 'advance' value between ch1 and ch2

    STBTT_DEF int stbtt_GetCodepointBox(const stbtt_fontinfo *info, int codepoint, int *x0, int *y0, int *x1, int *y1);
    // Gets the bounding box of the visible part of the glyph, in unscaled coordinates

    STBTT_DEF void stbtt_GetGlyphHMetrics(const stbtt_fontinfo *info, int glyph_index, int *advanceWidth,
                                          int *leftSideBearing);
    STBTT_DEF int  stbtt_GetGlyphKernAdvance(const stbtt_fontinfo *info, int glyph1, int glyph2);
    STBTT_DEF int  stbtt_GetGlyphBox(const stbtt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1);
    // as above, but takes one or more glyph indices for greater efficiency

    typedef struct stbtt_kerningentry
    {
        int glyph1; // use stbtt_FindGlyphIndex
        int glyph2;
        int advance;
    } stbtt_kerningentry;

    STBTT_DEF int stbtt_GetKerningTableLength(const stbtt_fontinfo *info);
    STBTT_DEF int stbtt_GetKerningTable(const stbtt_fontinfo *info, stbtt_kerningentry *table, int table_length);
    // Retrieves a complete list of all of the kerning pairs provided by the font
    // stbtt_GetKerningTable never writes more than table_length entries and returns how many entries it did write.
    // The table will be sorted by (a.glyph1 == b.glyph1)?(a.glyph2 < b.glyph2):(a.glyph1 < b.glyph1)

    //////////////////////////////////////////////////////////////////////////////
    //
    // GLYPH SHAPES (you probably don't need these, but they have to go before
    // the bitmaps for C declaration-order reasons)
    //

#ifndef STBTT_vmove // you can predefine these to use different values (but why?)
    enum
    {
        STBTT_vmove = 1,
        STBTT_vline,
        STBTT_vcurve,
        STBTT_vcubic
    };
#endif

#ifndef stbtt_vertex            // you can predefine this to use different values
                                // (we share this with other code at RAD)
#define stbtt_vertex_type short // can't use stbtt_int16 because that's not visible in the header file
    typedef struct
    {
        stbtt_vertex_type x, y, cx, cy, cx1, cy1;
        unsigned char     type, padding;
    } stbtt_vertex;
#endif

    STBTT_DEF int stbtt_IsGlyphEmpty(const stbtt_fontinfo *info, int glyph_index);
    // returns non-zero if nothing is drawn for this glyph

    STBTT_DEF int stbtt_GetCodepointShape(const stbtt_fontinfo *info, int unicode_codepoint, stbtt_vertex **vertices);
    STBTT_DEF int stbtt_GetGlyphShape(const stbtt_fontinfo *info, int glyph_index, stbtt_vertex **vertices);
    // returns # of vertices and fills *vertices with the pointer to them
    //   these are expressed in "unscaled" coordinates
    //
    // The shape is a series of contours. Each one starts with
    // a STBTT_moveto, then consists of a series of mixed
    // STBTT_lineto and STBTT_curveto segments. A lineto
    // draws a line from previous endpoint to its x,y; a curveto
    // draws a quadratic bezier from previous endpoint to
    // its x,y, using cx,cy as the bezier control point.

    STBTT_DEF void stbtt_FreeShape(const stbtt_fontinfo *info, stbtt_vertex *vertices);
    // frees the data allocated above

    STBTT_DEF unsigned char *stbtt_FindSVGDoc(const stbtt_fontinfo *info, int gl);
    STBTT_DEF int            stbtt_GetCodepointSVG(const stbtt_fontinfo *info, int unicode_codepoint, const char **svg);
    STBTT_DEF int            stbtt_GetGlyphSVG(const stbtt_fontinfo *info, int gl, const char **svg);
    // fills svg with the character's SVG data.
    // returns data size or 0 if SVG not found.

    //////////////////////////////////////////////////////////////////////////////
    //
    // BITMAP RENDERING
    //

    STBTT_DEF void stbtt_FreeBitmap(unsigned char *bitmap, void *userdata);
    // frees the bitmap allocated below

    STBTT_DEF unsigned char *stbtt_GetCodepointBitmap(const stbtt_fontinfo *info, float scale_x, float scale_y,
                                                      int codepoint, int *width, int *height, int *xoff, int *yoff);
    // allocates a large-enough single-channel 8bpp bitmap and renders the
    // specified character/glyph at the specified scale into it, with
    // antialiasing. 0 is no coverage (transparent), 255 is fully covered (opaque).
    // *width & *height are filled out with the width & height of the bitmap,
    // which is stored left-to-right, top-to-bottom.
    //
    // xoff/yoff are the offset it pixel space from the glyph origin to the top-left of the bitmap

    STBTT_DEF unsigned char *stbtt_GetCodepointBitmapSubpixel(const stbtt_fontinfo *info, float scale_x, float scale_y,
                                                              float shift_x, float shift_y, int codepoint, int *width,
                                                              int *height, int *xoff, int *yoff);
    // the same as stbtt_GetCodepoitnBitmap, but you can specify a subpixel
    // shift for the character

    STBTT_DEF void stbtt_MakeCodepointBitmap(const stbtt_fontinfo *info, unsigned char *output, int out_w, int out_h,
                                             int out_stride, float scale_x, float scale_y, int codepoint);
    // the same as stbtt_GetCodepointBitmap, but you pass in storage for the bitmap
    // in the form of 'output', with row spacing of 'out_stride' bytes. the bitmap
    // is clipped to out_w/out_h bytes. Call stbtt_GetCodepointBitmapBox to get the
    // width and height and positioning info for it first.

    STBTT_DEF void stbtt_MakeCodepointBitmapSubpixel(const stbtt_fontinfo *info, unsigned char *output, int out_w,
                                                     int out_h, int out_stride, float scale_x, float scale_y,
                                                     float shift_x, float shift_y, int codepoint);
    // same as stbtt_MakeCodepointBitmap, but you can specify a subpixel
    // shift for the character

    STBTT_DEF void stbtt_MakeCodepointBitmapSubpixelPrefilter(const stbtt_fontinfo *info, unsigned char *output,
                                                              int out_w, int out_h, int out_stride, float scale_x,
                                                              float scale_y, float shift_x, float shift_y,
                                                              int oversample_x, int oversample_y, float *sub_x,
                                                              float *sub_y, int codepoint);
    // same as stbtt_MakeCodepointBitmapSubpixel, but prefiltering
    // is performed (see stbtt_PackSetOversampling)

    STBTT_DEF void stbtt_GetCodepointBitmapBox(const stbtt_fontinfo *font, int codepoint, float scale_x, float scale_y,
                                               int *ix0, int *iy0, int *ix1, int *iy1);
    // get the bbox of the bitmap centered around the glyph origin; so the
    // bitmap width is ix1-ix0, height is iy1-iy0, and location to place
    // the bitmap top left is (leftSideBearing*scale,iy0).
    // (Note that the bitmap uses y-increases-down, but the shape uses
    // y-increases-up, so CodepointBitmapBox and CodepointBox are inverted.)

    STBTT_DEF void stbtt_GetCodepointBitmapBoxSubpixel(const stbtt_fontinfo *font, int codepoint, float scale_x,
                                                       float scale_y, float shift_x, float shift_y, int *ix0, int *iy0,
                                                       int *ix1, int *iy1);
    // same as stbtt_GetCodepointBitmapBox, but you can specify a subpixel
    // shift for the character

    // the following functions are equivalent to the above functions, but operate
    // on glyph indices instead of Unicode codepoints (for efficiency)
    STBTT_DEF unsigned char *stbtt_GetGlyphBitmap(const stbtt_fontinfo *info, float scale_x, float scale_y, int glyph,
                                                  int *width, int *height, int *xoff, int *yoff);
    STBTT_DEF unsigned char *stbtt_GetGlyphBitmapSubpixel(const stbtt_fontinfo *info, float scale_x, float scale_y,
                                                          float shift_x, float shift_y, int glyph, int *width,
                                                          int *height, int *xoff, int *yoff);
    STBTT_DEF void stbtt_MakeGlyphBitmap(const stbtt_fontinfo *info, unsigned char *output, int out_w, int out_h,
                                         int out_stride, float scale_x, float scale_y, int glyph);
    STBTT_DEF void stbtt_MakeGlyphBitmapSubpixel(const stbtt_fontinfo *info, unsigned char *output, int out_w,
                                                 int out_h, int out_stride, float scale_x, float scale_y, float shift_x,
                                                 float shift_y, int glyph);
    STBTT_DEF void stbtt_MakeGlyphBitmapSubpixelPrefilter(const stbtt_fontinfo *info, unsigned char *output, int out_w,
                                                          int out_h, int out_stride, float scale_x, float scale_y,
                                                          float shift_x, float shift_y, int oversample_x,
                                                          int oversample_y, float *sub_x, float *sub_y, int glyph);
    STBTT_DEF void stbtt_GetGlyphBitmapBox(const stbtt_fontinfo *font, int glyph, float scale_x, float scale_y,
                                           int *ix0, int *iy0, int *ix1, int *iy1);
    STBTT_DEF void stbtt_GetGlyphBitmapBoxSubpixel(const stbtt_fontinfo *font, int glyph, float scale_x, float scale_y,
                                                   float shift_x, float shift_y, int *ix0, int *iy0, int *ix1,
                                                   int *iy1);

    // @TODO: don't expose this structure
    typedef struct
    {
        int            w, h, stride;
        unsigned char *pixels;
    } stbtt__bitmap;

    // rasterize a shape with quadratic beziers into a bitmap
    STBTT_DEF void stbtt_Rasterize(stbtt__bitmap *result,             // 1-channel bitmap to draw into
                                   float          flatness_in_pixels, // allowable error of curve in pixels
                                   stbtt_vertex  *vertices,           // array of vertices defining shape
                                   int            num_verts,          // number of vertices in above array
                                   float scale_x, float scale_y,      // scale applied to input vertices
                                   float shift_x, float shift_y,      // translation applied to input vertices
                                   int x_off, int y_off,              // another translation applied to input
                                   int   invert,                      // if non-zero, vertically flip shape
                                   void *userdata);                   // context for to STBTT_MALLOC

    //////////////////////////////////////////////////////////////////////////////
    //
    // Signed Distance Function (or Field) rendering

    STBTT_DEF void stbtt_FreeSDF(unsigned char *bitmap, void *userdata);
    // frees the SDF bitmap allocated below

    STBTT_DEF unsigned char *stbtt_GetGlyphSDF(const stbtt_fontinfo *info, float scale, int glyph, int padding,
                                               unsigned char onedge_value, float pixel_dist_scale, int *width,
                                               int *height, int *xoff, int *yoff);
    STBTT_DEF unsigned char *stbtt_GetCodepointSDF(const stbtt_fontinfo *info, float scale, int codepoint, int padding,
                                                   unsigned char onedge_value, float pixel_dist_scale, int *width,
                                                   int *height, int *xoff, int *yoff);
    // These functions compute a discretized SDF field for a single character, suitable for storing
    // in a single-channel texture, sampling with bilinear filtering, and testing against
    // larger than some threshold to produce scalable fonts.
    //        info              --  the font
    //        scale             --  controls the size of the resulting SDF bitmap, same as it would be creating a
    //        regular bitmap glyph/codepoint   --  the character to generate the SDF for padding           --  extra
    //        "pixels" around the character which are filled with the distance to the character (not 0),
    //                                 which allows effects like bit outlines
    //        onedge_value      --  value 0-255 to test the SDF against to reconstruct the character (i.e. the
    //        isocontour of the character) pixel_dist_scale  --  what value the SDF should increase by when moving
    //        one SDF "pixel" away from the edge (on the 0..255 scale)
    //                                 if positive, > onedge_value is inside; if negative, < onedge_value is inside
    //        width,height      --  output height & width of the SDF bitmap (including padding)
    //        xoff,yoff         --  output origin of the character
    //        return value      --  a 2D array of bytes 0..255, width*height in size
    //
    // pixel_dist_scale & onedge_value are a scale & bias that allows you to make
    // optimal use of the limited 0..255 for your application, trading off precision
    // and special effects. SDF values outside the range 0..255 are clamped to 0..255.
    //
    // Example:
    //      scale = stbtt_ScaleForPixelHeight(22)
    //      padding = 5
    //      onedge_value = 180
    //      pixel_dist_scale = 180/5.0 = 36.0
    //
    //      This will create an SDF bitmap in which the character is about 22 pixels
    //      high but the whole bitmap is about 22+5+5=32 pixels high. To produce a filled
    //      shape, sample the SDF at each pixel and fill the pixel if the SDF value
    //      is greater than or equal to 180/255. (You'll actually want to antialias,
    //      which is beyond the scope of this example.) Additionally, you can compute
    //      offset outlines (e.g. to stroke the character border inside & outside,
    //      or only outside). For example, to fill outside the character up to 3 SDF
    //      pixels, you would compare against (180-36.0*3)/255 = 72/255. The above
    //      choice of variables maps a range from 5 pixels outside the shape to
    //      2 pixels inside the shape to 0..255; this is intended primarily for apply
    //      outside effects only (the interior range is needed to allow proper
    //      antialiasing of the font at *smaller* sizes)
    //
    // The function computes the SDF analytically at each SDF pixel, not by e.g.
    // building a higher-res bitmap and approximating it. In theory the quality
    // should be as high as possible for an SDF of this size & representation, but
    // unclear if this is true in practice (perhaps building a higher-res bitmap
    // and computing from that can allow drop-out prevention).
    //
    // The algorithm has not been optimized at all, so expect it to be slow
    // if computing lots of characters or very large sizes.

    //////////////////////////////////////////////////////////////////////////////
    //
    // Finding the right font...
    //
    // You should really just solve this offline, keep your own tables
    // of what font is what, and don't try to get it out of the .ttf file.
    // That's because getting it out of the .ttf file is really hard, because
    // the names in the file can appear in many possible encodings, in many
    // possible languages, and e.g. if you need a case-insensitive comparison,
    // the details of that depend on the encoding & language in a complex way
    // (actually underspecified in truetype, but also gigantic).
    //
    // But you can use the provided functions in two possible ways:
    //     stbtt_FindMatchingFont() will use *case-sensitive* comparisons on
    //             unicode-encoded names to try to find the font you want;
    //             you can run this before calling stbtt_InitFont()
    //
    //     stbtt_GetFontNameString() lets you get any of the various strings
    //             from the file yourself and do your own comparisons on them.
    //             You have to have called stbtt_InitFont() first.

    STBTT_DEF int stbtt_FindMatchingFont(const unsigned char *fontdata, const char *name, int flags);
// returns the offset (not index) of the font that matches, or -1 if none
//   if you use STBTT_MACSTYLE_DONTCARE, use a font name like "Arial Bold".
//   if you use any other flag, use a font name like "Arial"; this checks
//     the 'macStyle' header field; i don't know if fonts set this consistently
#define STBTT_MACSTYLE_DONTCARE 0
#define STBTT_MACSTYLE_BOLD 1
#define STBTT_MACSTYLE_ITALIC 2
#define STBTT_MACSTYLE_UNDERSCORE 4
#define STBTT_MACSTYLE_NONE 8 // <= not same as 0, this makes us check the bitfield is 0

    STBTT_DEF int stbtt_CompareUTF8toUTF16_bigendian(const char *s1, int len1, const char *s2, int len2);
    // returns 1/0 whether the first string interpreted as utf8 is identical to
    // the second string interpreted as big-endian utf16... useful for strings from next func

    STBTT_DEF const char *stbtt_GetFontNameString(const stbtt_fontinfo *font, int *length, int platformID,
                                                  int encodingID, int languageID, int nameID);
    // returns the string (which may be big-endian double byte, e.g. for unicode)
    // and puts the length in bytes in *length.
    //
    // some of the values for the IDs are below; for more see the truetype spec:
    //     http://developer.apple.com/textfonts/TTRefMan/RM06/Chap6name.html
    //     http://www.microsoft.com/typography/otspec/name.htm

    enum
    { // platformID
        STBTT_PLATFORM_ID_UNICODE   = 0,
        STBTT_PLATFORM_ID_MAC       = 1,
        STBTT_PLATFORM_ID_ISO       = 2,
        STBTT_PLATFORM_ID_MICROSOFT = 3
    };

    enum
    { // encodingID for STBTT_PLATFORM_ID_UNICODE
        STBTT_UNICODE_EID_UNICODE_1_0      = 0,
        STBTT_UNICODE_EID_UNICODE_1_1      = 1,
        STBTT_UNICODE_EID_ISO_10646        = 2,
        STBTT_UNICODE_EID_UNICODE_2_0_BMP  = 3,
        STBTT_UNICODE_EID_UNICODE_2_0_FULL = 4
    };

    enum
    { // encodingID for STBTT_PLATFORM_ID_MICROSOFT
        STBTT_MS_EID_SYMBOL       = 0,
        STBTT_MS_EID_UNICODE_BMP  = 1,
        STBTT_MS_EID_SHIFTJIS     = 2,
        STBTT_MS_EID_UNICODE_FULL = 10
    };

    enum
    { // encodingID for STBTT_PLATFORM_ID_MAC; same as Script Manager codes
        STBTT_MAC_EID_ROMAN        = 0,
        STBTT_MAC_EID_ARABIC       = 4,
        STBTT_MAC_EID_JAPANESE     = 1,
        STBTT_MAC_EID_HEBREW       = 5,
        STBTT_MAC_EID_CHINESE_TRAD = 2,
        STBTT_MAC_EID_GREEK        = 6,
        STBTT_MAC_EID_KOREAN       = 3,
        STBTT_MAC_EID_RUSSIAN      = 7
    };

    enum
    { // languageID for STBTT_PLATFORM_ID_MICROSOFT; same as LCID...
      // problematic because there are e.g. 16 english LCIDs and 16 arabic LCIDs
        STBTT_MS_LANG_ENGLISH  = 0x0409,
        STBTT_MS_LANG_ITALIAN  = 0x0410,
        STBTT_MS_LANG_CHINESE  = 0x0804,
        STBTT_MS_LANG_JAPANESE = 0x0411,
        STBTT_MS_LANG_DUTCH    = 0x0413,
        STBTT_MS_LANG_KOREAN   = 0x0412,
        STBTT_MS_LANG_FRENCH   = 0x040c,
        STBTT_MS_LANG_RUSSIAN  = 0x0419,
        STBTT_MS_LANG_GERMAN   = 0x0407,
        STBTT_MS_LANG_SPANISH  = 0x0409,
        STBTT_MS_LANG_HEBREW   = 0x040d,
        STBTT_MS_LANG_SWEDISH  = 0x041D
    };

    enum
    { // languageID for STBTT_PLATFORM_ID_MAC
        STBTT_MAC_LANG_ENGLISH            = 0,
        STBTT_MAC_LANG_JAPANESE           = 11,
        STBTT_MAC_LANG_ARABIC             = 12,
        STBTT_MAC_LANG_KOREAN             = 23,
        STBTT_MAC_LANG_DUTCH              = 4,
        STBTT_MAC_LANG_RUSSIAN            = 32,
        STBTT_MAC_LANG_FRENCH             = 1,
        STBTT_MAC_LANG_SPANISH            = 6,
        STBTT_MAC_LANG_GERMAN             = 2,
        STBTT_MAC_LANG_SWEDISH            = 5,
        STBTT_MAC_LANG_HEBREW             = 10,
        STBTT_MAC_LANG_CHINESE_SIMPLIFIED = 33,
        STBTT_MAC_LANG_ITALIAN            = 3,
        STBTT_MAC_LANG_CHINESE_TRAD       = 19
    };

#ifdef __cplusplus
}
#endif

#endif // __STB_INCLUDE_STB_TRUETYPE_H__

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////
////   IMPLEMENTATION
////
////

#ifdef STB_TRUETYPE_IMPLEMENTATION

#ifndef STBTT_MAX_OVERSAMPLE
#define STBTT_MAX_OVERSAMPLE 8
#endif

#if STBTT_MAX_OVERSAMPLE > 255
#error "STBTT_MAX_OVERSAMPLE cannot be > 255"
#endif

typedef int stbtt__test_oversample_pow2[(STBTT_MAX_OVERSAMPLE & (STBTT_MAX_OVERSAMPLE - 1)) == 0 ? 1 : -1];

#ifndef STBTT_RASTERIZER_VERSION
#define STBTT_RASTERIZER_VERSION 2
#endif

#ifdef _MSC_VER
#define STBTT__NOTUSED(v) (void)(v)
#else
#define STBTT__NOTUSED(v) (void)sizeof(v)
#endif

//////////////////////////////////////////////////////////////////////////
//
// stbtt__buf helpers to parse data from file
//

static stbtt_uint8 stbtt__buf_get8(stbtt__buf *b)
{
    if (b->cursor >= b->size)
        return 0;
    return b->data[b->cursor++];
}

static stbtt_uint8 stbtt__buf_peek8(stbtt__buf *b)
{
    if (b->cursor >= b->size)
        return 0;
    return b->data[b->cursor];
}

static void stbtt__buf_seek(stbtt__buf *b, int o)
{
    STBTT_assert(!(o > b->size || o < 0));
    b->cursor = (o > b->size || o < 0) ? b->size : o;
}

static void stbtt__buf_skip(stbtt__buf *b, int o)
{
    stbtt__buf_seek(b, b->cursor + o);
}

static stbtt_uint32 stbtt__buf_get(stbtt__buf *b, int n)
{
    stbtt_uint32 v = 0;
    int          i;
    STBTT_assert(n >= 1 && n <= 4);
    for (i = 0; i < n; i++)
        v = (v << 8) | stbtt__buf_get8(b);
    return v;
}

static stbtt__buf stbtt__new_buf(const void *p, size_t size)
{
    stbtt__buf r;
    STBTT_assert(size < 0x40000000);
    r.data   = (stbtt_uint8 *)p;
    r.size   = (int)size;
    r.cursor = 0;
    return r;
}

#define stbtt__buf_get16(b) stbtt__buf_get((b), 2)
#define stbtt__buf_get32(b) stbtt__buf_get((b), 4)

static stbtt__buf stbtt__buf_range(const stbtt__buf *b, int o, int s)
{
    stbtt__buf r = stbtt__new_buf(NULL, 0);
    if (o < 0 || s < 0 || o > b->size || s > b->size - o)
        return r;
    r.data = b->data + o;
    r.size = s;
    return r;
}

static stbtt__buf stbtt__cff_get_index(stbtt__buf *b)
{
    int count, start, offsize;
    start = b->cursor;
    count = stbtt__buf_get16(b);
    if (count)
    {
        offsize = stbtt__buf_get8(b);
        STBTT_assert(offsize >= 1 && offsize <= 4);
        stbtt__buf_skip(b, offsize * count);
        stbtt__buf_skip(b, stbtt__buf_get(b, offsize) - 1);
    }
    return stbtt__buf_range(b, start, b->cursor - start);
}

static stbtt_uint32 stbtt__cff_int(stbtt__buf *b)
{
    int b0 = stbtt__buf_get8(b);
    if (b0 >= 32 && b0 <= 246)
        return b0 - 139;
    else if (b0 >= 247 && b0 <= 250)
        return (b0 - 247) * 256 + stbtt__buf_get8(b) + 108;
    else if (b0 >= 251 && b0 <= 254)
        return -(b0 - 251) * 256 - stbtt__buf_get8(b) - 108;
    else if (b0 == 28)
        return stbtt__buf_get16(b);
    else if (b0 == 29)
        return stbtt__buf_get32(b);
    STBTT_assert(0);
    return 0;
}

static void stbtt__cff_skip_operand(stbtt__buf *b)
{
    int v, b0 = stbtt__buf_peek8(b);
    STBTT_assert(b0 >= 28);
    if (b0 == 30)
    {
        stbtt__buf_skip(b, 1);
        while (b->cursor < b->size)
        {
            v = stbtt__buf_get8(b);
            if ((v & 0xF) == 0xF || (v >> 4) == 0xF)
                break;
        }
    }
    else
    {
        stbtt__cff_int(b);
    }
}

static stbtt__buf stbtt__dict_get(stbtt__buf *b, int key)
{
    stbtt__buf_seek(b, 0);
    while (b->cursor < b->size)
    {
        int start = b->cursor, end, op;
        while (stbtt__buf_peek8(b) >= 28)
            stbtt__cff_skip_operand(b);
        end = b->cursor;
        op  = stbtt__buf_get8(b);
        if (op == 12)
            op = stbtt__buf_get8(b) | 0x100;
        if (op == key)
            return stbtt__buf_range(b, start, end - start);
    }
    return stbtt__buf_range(b, 0, 0);
}

static void stbtt__dict_get_ints(stbtt__buf *b, int key, int outcount, stbtt_uint32 *out)
{
    int        i;
    stbtt__buf operands = stbtt__dict_get(b, key);
    for (i = 0; i < outcount && operands.cursor < operands.size; i++)
        out[i] = stbtt__cff_int(&operands);
}

static int stbtt__cff_index_count(stbtt__buf *b)
{
    stbtt__buf_seek(b, 0);
    return stbtt__buf_get16(b);
}

static stbtt__buf stbtt__cff_index_get(stbtt__buf b, int i)
{
    int count, offsize, start, end;
    stbtt__buf_seek(&b, 0);
    count   = stbtt__buf_get16(&b);
    offsize = stbtt__buf_get8(&b);
    STBTT_assert(i >= 0 && i < count);
    STBTT_assert(offsize >= 1 && offsize <= 4);
    stbtt__buf_skip(&b, i * offsize);
    start = stbtt__buf_get(&b, offsize);
    end   = stbtt__buf_get(&b, offsize);
    return stbtt__buf_range(&b, 2 + (count + 1) * offsize + start, end - start);
}

//////////////////////////////////////////////////////////////////////////
//
// accessors to parse data from file
//

// on platforms that don't allow misaligned reads, if we want to allow
// truetype fonts that aren't padded to alignment, define ALLOW_UNALIGNED_TRUETYPE

#define ttBYTE(p) (*(stbtt_uint8 *)(p))
#define ttCHAR(p) (*(stbtt_int8 *)(p))
#define ttFixed(p) ttLONG(p)

static stbtt_uint16 ttUSHORT(stbtt_uint8 *p)
{
    return p[0] * 256 + p[1];
}
static stbtt_int16 ttSHORT(stbtt_uint8 *p)
{
    return p[0] * 256 + p[1];
}
static stbtt_uint32 ttULONG(stbtt_uint8 *p)
{
    return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}
static stbtt_int32 ttLONG(stbtt_uint8 *p)
{
    return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}

#define stbtt_tag4(p, c0, c1, c2, c3) ((p)[0] == (c0) && (p)[1] == (c1) && (p)[2] == (c2) && (p)[3] == (c3))
#define stbtt_tag(p, str) stbtt_tag4(p, str[0], str[1], str[2], str[3])

static int stbtt__isfont(stbtt_uint8 *font)
{
    // check the version number
    if (stbtt_tag4(font, '1', 0, 0, 0))
        return 1; // TrueType 1
    if (stbtt_tag(font, "typ1"))
        return 1; // TrueType with type 1 font -- we don't support this!
    if (stbtt_tag(font, "OTTO"))
        return 1; // OpenType with CFF
    if (stbtt_tag4(font, 0, 1, 0, 0))
        return 1; // OpenType 1.0
    if (stbtt_tag(font, "true"))
        return 1; // Apple specification for TrueType fonts
    return 0;
}

// @OPTIMIZE: binary search
static stbtt_uint32 stbtt__find_table(stbtt_uint8 *data, stbtt_uint32 fontstart, const char *tag)
{
    stbtt_int32  num_tables = ttUSHORT(data + fontstart + 4);
    stbtt_uint32 tabledir   = fontstart + 12;
    stbtt_int32  i;
    for (i = 0; i < num_tables; ++i)
    {
        stbtt_uint32 loc = tabledir + 16 * i;
        if (stbtt_tag(data + loc + 0, tag))
            return ttULONG(data + loc + 8);
    }
    return 0;
}

static int stbtt_GetFontOffsetForIndex_internal(unsigned char *font_collection, int index)
{
    // if it's just a font, there's only one valid index
    if (stbtt__isfont(font_collection))
        return index == 0 ? 0 : -1;

    // check if it's a TTC
    if (stbtt_tag(font_collection, "ttcf"))
    {
        // version 1?
        if (ttULONG(font_collection + 4) == 0x00010000 || ttULONG(font_collection + 4) == 0x00020000)
        {
            stbtt_int32 n = ttLONG(font_collection + 8);
            if (index >= n)
                return -1;
            return ttULONG(font_collection + 12 + index * 4);
        }
    }
    return -1;
}

static int stbtt_GetNumberOfFonts_internal(unsigned char *font_collection)
{
    // if it's just a font, there's only one valid font
    if (stbtt__isfont(font_collection))
        return 1;

    // check if it's a TTC
    if (stbtt_tag(font_collection, "ttcf"))
    {
        // version 1?
        if (ttULONG(font_collection + 4) == 0x00010000 || ttULONG(font_collection + 4) == 0x00020000)
        {
            return ttLONG(font_collection + 8);
        }
    }
    return 0;
}

static stbtt__buf stbtt__get_subrs(stbtt__buf cff, stbtt__buf fontdict)
{
    stbtt_uint32 subrsoff = 0, private_loc[2] = {0, 0};
    stbtt__buf   pdict;
    stbtt__dict_get_ints(&fontdict, 18, 2, private_loc);
    if (!private_loc[1] || !private_loc[0])
        return stbtt__new_buf(NULL, 0);
    pdict = stbtt__buf_range(&cff, private_loc[1], private_loc[0]);
    stbtt__dict_get_ints(&pdict, 19, 1, &subrsoff);
    if (!subrsoff)
        return stbtt__new_buf(NULL, 0);
    stbtt__buf_seek(&cff, private_loc[1] + subrsoff);
    return stbtt__cff_get_index(&cff);
}

// since most people won't use this, find this table the first time it's needed
static int stbtt__get_svg(stbtt_fontinfo *info)
{
    stbtt_uint32 t;
    if (info->svg < 0)
    {
        t = stbtt__find_table(info->data, info->fontstart, "SVG ");
        if (t)
        {
            stbtt_uint32 offset = ttULONG(info->data + t + 2);
            info->svg           = t + offset;
        }
        else
        {
            info->svg = 0;
        }
    }
    return info->svg;
}

static int stbtt_InitFont_internal(stbtt_fontinfo *info, unsigned char *data, int fontstart)
{
    stbtt_uint32 cmap, t;
    stbtt_int32  i, numTables;

    info->data      = data;
    info->fontstart = fontstart;
    info->cff       = stbtt__new_buf(NULL, 0);

    cmap            = stbtt__find_table(data, fontstart, "cmap"); // required
    info->loca      = stbtt__find_table(data, fontstart, "loca"); // required
    info->head      = stbtt__find_table(data, fontstart, "head"); // required
    info->glyf      = stbtt__find_table(data, fontstart, "glyf"); // required
    info->hhea      = stbtt__find_table(data, fontstart, "hhea"); // required
    info->hmtx      = stbtt__find_table(data, fontstart, "hmtx"); // required
    info->kern      = stbtt__find_table(data, fontstart, "kern"); // not required
    info->gpos      = stbtt__find_table(data, fontstart, "GPOS"); // not required

    if (!cmap || !info->head || !info->hhea || !info->hmtx)
        return 0;
    if (info->glyf)
    {
        // required for truetype
        if (!info->loca)
            return 0;
    }
    else
    {
        // initialization for CFF / Type2 fonts (OTF)
        stbtt__buf   b, topdict, topdictidx;
        stbtt_uint32 cstype = 2, charstrings = 0, fdarrayoff = 0, fdselectoff = 0;
        stbtt_uint32 cff;

        cff = stbtt__find_table(data, fontstart, "CFF ");
        if (!cff)
            return 0;

        info->fontdicts = stbtt__new_buf(NULL, 0);
        info->fdselect  = stbtt__new_buf(NULL, 0);

        // @TODO this should use size from table (not 512MB)
        info->cff = stbtt__new_buf(data + cff, 512 * 1024 * 1024);
        b         = info->cff;

        // read the header
        stbtt__buf_skip(&b, 2);
        stbtt__buf_seek(&b, stbtt__buf_get8(&b)); // hdrsize

        // @TODO the name INDEX could list multiple fonts,
        // but we just use the first one.
        stbtt__cff_get_index(&b); // name INDEX
        topdictidx = stbtt__cff_get_index(&b);
        topdict    = stbtt__cff_index_get(topdictidx, 0);
        stbtt__cff_get_index(&b); // string INDEX
        info->gsubrs = stbtt__cff_get_index(&b);

        stbtt__dict_get_ints(&topdict, 17, 1, &charstrings);
        stbtt__dict_get_ints(&topdict, 0x100 | 6, 1, &cstype);
        stbtt__dict_get_ints(&topdict, 0x100 | 36, 1, &fdarrayoff);
        stbtt__dict_get_ints(&topdict, 0x100 | 37, 1, &fdselectoff);
        info->subrs = stbtt__get_subrs(b, topdict);

        // we only support Type 2 charstrings
        if (cstype != 2)
            return 0;
        if (charstrings == 0)
            return 0;

        if (fdarrayoff)
        {
            // looks like a CID font
            if (!fdselectoff)
                return 0;
            stbtt__buf_seek(&b, fdarrayoff);
            info->fontdicts = stbtt__cff_get_index(&b);
            info->fdselect  = stbtt__buf_range(&b, fdselectoff, b.size - fdselectoff);
        }

        stbtt__buf_seek(&b, charstrings);
        info->charstrings = stbtt__cff_get_index(&b);
    }

    t = stbtt__find_table(data, fontstart, "maxp");
    if (t)
        info->numGlyphs = ttUSHORT(data + t + 4);
    else
        info->numGlyphs = 0xffff;

    info->svg = -1;

    // find a cmap encoding table we understand *now* to avoid searching
    // later. (todo: could make this installable)
    // the same regardless of glyph.
    numTables       = ttUSHORT(data + cmap + 2);
    info->index_map = 0;
    for (i = 0; i < numTables; ++i)
    {
        stbtt_uint32 encoding_record = cmap + 4 + 8 * i;
        // find an encoding we understand:
        switch (ttUSHORT(data + encoding_record))
        {
        case STBTT_PLATFORM_ID_MICROSOFT:
            switch (ttUSHORT(data + encoding_record + 2))
            {
            case STBTT_MS_EID_UNICODE_BMP:
            case STBTT_MS_EID_UNICODE_FULL:
                // MS/Unicode
                info->index_map = cmap + ttULONG(data + encoding_record + 4);
                break;
            }
            break;
        case STBTT_PLATFORM_ID_UNICODE:
            // Mac/iOS has these
            // all the encodingIDs are unicode, so we don't bother to check it
            info->index_map = cmap + ttULONG(data + encoding_record + 4);
            break;
        }
    }
    if (info->index_map == 0)
        return 0;

    info->indexToLocFormat = ttUSHORT(data + info->head + 50);
    return 1;
}

STBTT_DEF int stbtt_FindGlyphIndex(const stbtt_fontinfo *info, int unicode_codepoint)
{
    stbtt_uint8 *data      = info->data;
    stbtt_uint32 index_map = info->index_map;

    stbtt_uint16 format    = ttUSHORT(data + index_map + 0);
    if (format == 0)
    { // apple byte encoding
        stbtt_int32 bytes = ttUSHORT(data + index_map + 2);
        if (unicode_codepoint < bytes - 6)
            return ttBYTE(data + index_map + 6 + unicode_codepoint);
        return 0;
    }
    else if (format == 6)
    {
        stbtt_uint32 first = ttUSHORT(data + index_map + 6);
        stbtt_uint32 count = ttUSHORT(data + index_map + 8);
        if ((stbtt_uint32)unicode_codepoint >= first && (stbtt_uint32)unicode_codepoint < first + count)
            return ttUSHORT(data + index_map + 10 + (unicode_codepoint - first) * 2);
        return 0;
    }
    else if (format == 2)
    {
        STBTT_assert(0); // @TODO: high-byte mapping for japanese/chinese/korean
        return 0;
    }
    else if (format == 4)
    { // standard mapping for windows fonts: binary search collection of ranges
        stbtt_uint16 segcount      = ttUSHORT(data + index_map + 6) >> 1;
        stbtt_uint16 searchRange   = ttUSHORT(data + index_map + 8) >> 1;
        stbtt_uint16 entrySelector = ttUSHORT(data + index_map + 10);
        stbtt_uint16 rangeShift    = ttUSHORT(data + index_map + 12) >> 1;

        // do a binary search of the segments
        stbtt_uint32 endCount = index_map + 14;
        stbtt_uint32 search   = endCount;

        if (unicode_codepoint > 0xffff)
            return 0;

        // they lie from endCount .. endCount + segCount
        // but searchRange is the nearest power of two, so...
        if (unicode_codepoint >= ttUSHORT(data + search + rangeShift * 2))
            search += rangeShift * 2;

        // now decrement to bias correctly to find smallest
        search -= 2;
        while (entrySelector)
        {
            stbtt_uint16 end;
            searchRange >>= 1;
            end = ttUSHORT(data + search + searchRange * 2);
            if (unicode_codepoint > end)
                search += searchRange * 2;
            --entrySelector;
        }
        search += 2;

        {
            stbtt_uint16 offset, start, last;
            stbtt_uint16 item = (stbtt_uint16)((search - endCount) >> 1);

            start             = ttUSHORT(data + index_map + 14 + segcount * 2 + 2 + 2 * item);
            last              = ttUSHORT(data + endCount + 2 * item);
            if (unicode_codepoint < start || unicode_codepoint > last)
                return 0;

            offset = ttUSHORT(data + index_map + 14 + segcount * 6 + 2 + 2 * item);
            if (offset == 0)
                return (stbtt_uint16)(unicode_codepoint + ttSHORT(data + index_map + 14 + segcount * 4 + 2 + 2 * item));

            return ttUSHORT(data + offset + (unicode_codepoint - start) * 2 + index_map + 14 + segcount * 6 + 2 +
                            2 * item);
        }
    }
    else if (format == 12 || format == 13)
    {
        stbtt_uint32 ngroups = ttULONG(data + index_map + 12);
        stbtt_int32  low, high;
        low  = 0;
        high = (stbtt_int32)ngroups;
        // Binary search the right group.
        while (low < high)
        {
            stbtt_int32  mid        = low + ((high - low) >> 1); // rounds down, so low <= mid < high
            stbtt_uint32 start_char = ttULONG(data + index_map + 16 + mid * 12);
            stbtt_uint32 end_char   = ttULONG(data + index_map + 16 + mid * 12 + 4);
            if ((stbtt_uint32)unicode_codepoint < start_char)
                high = mid;
            else if ((stbtt_uint32)unicode_codepoint > end_char)
                low = mid + 1;
            else
            {
                stbtt_uint32 start_glyph = ttULONG(data + index_map + 16 + mid * 12 + 8);
                if (format == 12)
                    return start_glyph + unicode_codepoint - start_char;
                else // format == 13
                    return start_glyph;
            }
        }
        return 0; // not found
    }
    // @TODO
    STBTT_assert(0);
    return 0;
}

STBTT_DEF int stbtt_GetCodepointShape(const stbtt_fontinfo *info, int unicode_codepoint, stbtt_vertex **vertices)
{
    return stbtt_GetGlyphShape(info, stbtt_FindGlyphIndex(info, unicode_codepoint), vertices);
}

static void stbtt_setvertex(stbtt_vertex *v, stbtt_uint8 type, stbtt_int32 x, stbtt_int32 y, stbtt_int32 cx,
                            stbtt_int32 cy)
{
    v->type = type;
    v->x    = (stbtt_int16)x;
    v->y    = (stbtt_int16)y;
    v->cx   = (stbtt_int16)cx;
    v->cy   = (stbtt_int16)cy;
}

static int stbtt__GetGlyfOffset(const stbtt_fontinfo *info, int glyph_index)
{
    int g1, g2;

    STBTT_assert(!info->cff.size);

    if (glyph_index >= info->numGlyphs)
        return -1; // glyph index out of range
    if (info->indexToLocFormat >= 2)
        return -1; // unknown index->glyph map format

    if (info->indexToLocFormat == 0)
    {
        g1 = info->glyf + ttUSHORT(info->data + info->loca + glyph_index * 2) * 2;
        g2 = info->glyf + ttUSHORT(info->data + info->loca + glyph_index * 2 + 2) * 2;
    }
    else
    {
        g1 = info->glyf + ttULONG(info->data + info->loca + glyph_index * 4);
        g2 = info->glyf + ttULONG(info->data + info->loca + glyph_index * 4 + 4);
    }

    return g1 == g2 ? -1 : g1; // if length is 0, return -1
}

static int    stbtt__GetGlyphInfoT2(const stbtt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1);

STBTT_DEF int stbtt_GetGlyphBox(const stbtt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1)
{
    if (info->cff.size)
    {
        stbtt__GetGlyphInfoT2(info, glyph_index, x0, y0, x1, y1);
    }
    else
    {
        int g = stbtt__GetGlyfOffset(info, glyph_index);
        if (g < 0)
            return 0;

        if (x0)
            *x0 = ttSHORT(info->data + g + 2);
        if (y0)
            *y0 = ttSHORT(info->data + g + 4);
        if (x1)
            *x1 = ttSHORT(info->data + g + 6);
        if (y1)
            *y1 = ttSHORT(info->data + g + 8);
    }
    return 1;
}

STBTT_DEF int stbtt_GetCodepointBox(const stbtt_fontinfo *info, int codepoint, int *x0, int *y0, int *x1, int *y1)
{
    return stbtt_GetGlyphBox(info, stbtt_FindGlyphIndex(info, codepoint), x0, y0, x1, y1);
}

STBTT_DEF int stbtt_IsGlyphEmpty(const stbtt_fontinfo *info, int glyph_index)
{
    stbtt_int16 numberOfContours;
    int         g;
    if (info->cff.size)
        return stbtt__GetGlyphInfoT2(info, glyph_index, NULL, NULL, NULL, NULL) == 0;
    g = stbtt__GetGlyfOffset(info, glyph_index);
    if (g < 0)
        return 1;
    numberOfContours = ttSHORT(info->data + g);
    return numberOfContours == 0;
}

static int stbtt__close_shape(stbtt_vertex *vertices, int num_vertices, int was_off, int start_off, stbtt_int32 sx,
                              stbtt_int32 sy, stbtt_int32 scx, stbtt_int32 scy, stbtt_int32 cx, stbtt_int32 cy)
{
    if (start_off)
    {
        if (was_off)
            stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, (cx + scx) >> 1, (cy + scy) >> 1, cx, cy);
        stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, sx, sy, scx, scy);
    }
    else
    {
        if (was_off)
            stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, sx, sy, cx, cy);
        else
            stbtt_setvertex(&vertices[num_vertices++], STBTT_vline, sx, sy, 0, 0);
    }
    return num_vertices;
}

static int stbtt__GetGlyphShapeTT(const stbtt_fontinfo *info, int glyph_index, stbtt_vertex **pvertices)
{
    stbtt_int16   numberOfContours;
    stbtt_uint8  *endPtsOfContours;
    stbtt_uint8  *data         = info->data;
    stbtt_vertex *vertices     = 0;
    int           num_vertices = 0;
    int           g            = stbtt__GetGlyfOffset(info, glyph_index);

    *pvertices                 = NULL;

    if (g < 0)
        return 0;

    numberOfContours = ttSHORT(data + g);

    if (numberOfContours > 0)
    {
        stbtt_uint8  flags = 0, flagcount;
        stbtt_int32  ins, i, j = 0, m, n, next_move, was_off = 0, off, start_off = 0;
        stbtt_int32  x, y, cx, cy, sx, sy, scx, scy;
        stbtt_uint8 *points;
        endPtsOfContours = (data + g + 10);
        ins              = ttUSHORT(data + g + 10 + numberOfContours * 2);
        points           = data + g + 10 + numberOfContours * 2 + 2 + ins;

        n                = 1 + ttUSHORT(endPtsOfContours + numberOfContours * 2 - 2);

        m                = n + 2 * numberOfContours; // a loose bound on how many vertices we might need
        vertices         = (stbtt_vertex *)STBTT_malloc(m * sizeof(vertices[0]), info->userdata);
        if (vertices == 0)
            return 0;

        next_move = 0;
        flagcount = 0;

        // in first pass, we load uninterpreted data into the allocated array
        // above, shifted to the end of the array so we won't overwrite it when
        // we create our final data starting from the front

        off = m - n; // starting offset for uninterpreted data, regardless of how m ends up being calculated

        // first load flags

        for (i = 0; i < n; ++i)
        {
            if (flagcount == 0)
            {
                flags = *points++;
                if (flags & 8)
                    flagcount = *points++;
            }
            else
                --flagcount;
            vertices[off + i].type = flags;
        }

        // now load x coordinates
        x = 0;
        for (i = 0; i < n; ++i)
        {
            flags = vertices[off + i].type;
            if (flags & 2)
            {
                stbtt_int16 dx = *points++;
                x += (flags & 16) ? dx : -dx; // ???
            }
            else
            {
                if (!(flags & 16))
                {
                    x = x + (stbtt_int16)(points[0] * 256 + points[1]);
                    points += 2;
                }
            }
            vertices[off + i].x = (stbtt_int16)x;
        }

        // now load y coordinates
        y = 0;
        for (i = 0; i < n; ++i)
        {
            flags = vertices[off + i].type;
            if (flags & 4)
            {
                stbtt_int16 dy = *points++;
                y += (flags & 32) ? dy : -dy; // ???
            }
            else
            {
                if (!(flags & 32))
                {
                    y = y + (stbtt_int16)(points[0] * 256 + points[1]);
                    points += 2;
                }
            }
            vertices[off + i].y = (stbtt_int16)y;
        }

        // now convert them to our format
        num_vertices = 0;
        sx = sy = cx = cy = scx = scy = 0;
        for (i = 0; i < n; ++i)
        {
            flags = vertices[off + i].type;
            x     = (stbtt_int16)vertices[off + i].x;
            y     = (stbtt_int16)vertices[off + i].y;

            if (next_move == i)
            {
                if (i != 0)
                    num_vertices =
                        stbtt__close_shape(vertices, num_vertices, was_off, start_off, sx, sy, scx, scy, cx, cy);

                // now start the new one
                start_off = !(flags & 1);
                if (start_off)
                {
                    // if we start off with an off-curve point, then when we need to find a point on the curve
                    // where we can start, and we need to save some state for when we wraparound.
                    scx = x;
                    scy = y;
                    if (!(vertices[off + i + 1].type & 1))
                    {
                        // next point is also a curve point, so interpolate an on-point curve
                        sx = (x + (stbtt_int32)vertices[off + i + 1].x) >> 1;
                        sy = (y + (stbtt_int32)vertices[off + i + 1].y) >> 1;
                    }
                    else
                    {
                        // otherwise just use the next point as our start point
                        sx = (stbtt_int32)vertices[off + i + 1].x;
                        sy = (stbtt_int32)vertices[off + i + 1].y;
                        ++i; // we're using point i+1 as the starting point, so skip it
                    }
                }
                else
                {
                    sx = x;
                    sy = y;
                }
                stbtt_setvertex(&vertices[num_vertices++], STBTT_vmove, sx, sy, 0, 0);
                was_off   = 0;
                next_move = 1 + ttUSHORT(endPtsOfContours + j * 2);
                ++j;
            }
            else
            {
                if (!(flags & 1))
                {                // if it's a curve
                    if (was_off) // two off-curve control points in a row means interpolate an on-curve midpoint
                        stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, (cx + x) >> 1, (cy + y) >> 1, cx, cy);
                    cx      = x;
                    cy      = y;
                    was_off = 1;
                }
                else
                {
                    if (was_off)
                        stbtt_setvertex(&vertices[num_vertices++], STBTT_vcurve, x, y, cx, cy);
                    else
                        stbtt_setvertex(&vertices[num_vertices++], STBTT_vline, x, y, 0, 0);
                    was_off = 0;
                }
            }
        }
        num_vertices = stbtt__close_shape(vertices, num_vertices, was_off, start_off, sx, sy, scx, scy, cx, cy);
    }
    else if (numberOfContours < 0)
    {
        // Compound shapes.
        int          more = 1;
        stbtt_uint8 *comp = data + g + 10;
        num_vertices      = 0;
        vertices          = 0;
        while (more)
        {
            stbtt_uint16  flags, gidx;
            int           comp_num_verts = 0, i;
            stbtt_vertex *comp_verts = 0, *tmp = 0;
            float         mtx[6] = {1, 0, 0, 1, 0, 0}, m, n;

            flags                = ttSHORT(comp);
            comp += 2;
            gidx = ttSHORT(comp);
            comp += 2;

            if (flags & 2)
            { // XY values
                if (flags & 1)
                { // shorts
                    mtx[4] = ttSHORT(comp);
                    comp += 2;
                    mtx[5] = ttSHORT(comp);
                    comp += 2;
                }
                else
                {
                    mtx[4] = ttCHAR(comp);
                    comp += 1;
                    mtx[5] = ttCHAR(comp);
                    comp += 1;
                }
            }
            else
            {
                // @TODO handle matching point
                STBTT_assert(0);
            }
            if (flags & (1 << 3))
            { // WE_HAVE_A_SCALE
                mtx[0] = mtx[3] = ttSHORT(comp) / 16384.0f;
                comp += 2;
                mtx[1] = mtx[2] = 0;
            }
            else if (flags & (1 << 6))
            { // WE_HAVE_AN_X_AND_YSCALE
                mtx[0] = ttSHORT(comp) / 16384.0f;
                comp += 2;
                mtx[1] = mtx[2] = 0;
                mtx[3]          = ttSHORT(comp) / 16384.0f;
                comp += 2;
            }
            else if (flags & (1 << 7))
            { // WE_HAVE_A_TWO_BY_TWO
                mtx[0] = ttSHORT(comp) / 16384.0f;
                comp += 2;
                mtx[1] = ttSHORT(comp) / 16384.0f;
                comp += 2;
                mtx[2] = ttSHORT(comp) / 16384.0f;
                comp += 2;
                mtx[3] = ttSHORT(comp) / 16384.0f;
                comp += 2;
            }

            // Find transformation scales.
            m = (float)STBTT_sqrt(mtx[0] * mtx[0] + mtx[1] * mtx[1]);
            n = (float)STBTT_sqrt(mtx[2] * mtx[2] + mtx[3] * mtx[3]);

            // Get indexed glyph.
            comp_num_verts = stbtt_GetGlyphShape(info, gidx, &comp_verts);
            if (comp_num_verts > 0)
            {
                // Transform vertices.
                for (i = 0; i < comp_num_verts; ++i)
                {
                    stbtt_vertex     *v = &comp_verts[i];
                    stbtt_vertex_type x, y;
                    x     = v->x;
                    y     = v->y;
                    v->x  = (stbtt_vertex_type)(m * (mtx[0] * x + mtx[2] * y + mtx[4]));
                    v->y  = (stbtt_vertex_type)(n * (mtx[1] * x + mtx[3] * y + mtx[5]));
                    x     = v->cx;
                    y     = v->cy;
                    v->cx = (stbtt_vertex_type)(m * (mtx[0] * x + mtx[2] * y + mtx[4]));
                    v->cy = (stbtt_vertex_type)(n * (mtx[1] * x + mtx[3] * y + mtx[5]));
                }
                // Append vertices.
                tmp = (stbtt_vertex *)STBTT_malloc((num_vertices + comp_num_verts) * sizeof(stbtt_vertex),
                                                   info->userdata);
                if (!tmp)
                {
                    if (vertices)
                        STBTT_free(vertices, info->userdata);
                    if (comp_verts)
                        STBTT_free(comp_verts, info->userdata);
                    return 0;
                }
                if (num_vertices > 0 && vertices)
                    STBTT_memcpy(tmp, vertices, num_vertices * sizeof(stbtt_vertex));
                STBTT_memcpy(tmp + num_vertices, comp_verts, comp_num_verts * sizeof(stbtt_vertex));
                if (vertices)
                    STBTT_free(vertices, info->userdata);
                vertices = tmp;
                STBTT_free(comp_verts, info->userdata);
                num_vertices += comp_num_verts;
            }
            // More components ?
            more = flags & (1 << 5);
        }
    }
    else
    {
        // numberOfCounters == 0, do nothing
    }

    *pvertices = vertices;
    return num_vertices;
}

typedef struct
{
    int           bounds;
    int           started;
    float         first_x, first_y;
    float         x, y;
    stbtt_int32   min_x, max_x, min_y, max_y;

    stbtt_vertex *pvertices;
    int           num_vertices;
} stbtt__csctx;

#define STBTT__CSCTX_INIT(bounds)                                                                                      \
    {                                                                                                                  \
        bounds, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0                                                                     \
    }

static void stbtt__track_vertex(stbtt__csctx *c, stbtt_int32 x, stbtt_int32 y)
{
    if (x > c->max_x || !c->started)
        c->max_x = x;
    if (y > c->max_y || !c->started)
        c->max_y = y;
    if (x < c->min_x || !c->started)
        c->min_x = x;
    if (y < c->min_y || !c->started)
        c->min_y = y;
    c->started = 1;
}

static void stbtt__csctx_v(stbtt__csctx *c, stbtt_uint8 type, stbtt_int32 x, stbtt_int32 y, stbtt_int32 cx,
                           stbtt_int32 cy, stbtt_int32 cx1, stbtt_int32 cy1)
{
    if (c->bounds)
    {
        stbtt__track_vertex(c, x, y);
        if (type == STBTT_vcubic)
        {
            stbtt__track_vertex(c, cx, cy);
            stbtt__track_vertex(c, cx1, cy1);
        }
    }
    else
    {
        stbtt_setvertex(&c->pvertices[c->num_vertices], type, x, y, cx, cy);
        c->pvertices[c->num_vertices].cx1 = (stbtt_int16)cx1;
        c->pvertices[c->num_vertices].cy1 = (stbtt_int16)cy1;
    }
    c->num_vertices++;
}

static void stbtt__csctx_close_shape(stbtt__csctx *ctx)
{
    if (ctx->first_x != ctx->x || ctx->first_y != ctx->y)
        stbtt__csctx_v(ctx, STBTT_vline, (int)ctx->first_x, (int)ctx->first_y, 0, 0, 0, 0);
}

static void stbtt__csctx_rmove_to(stbtt__csctx *ctx, float dx, float dy)
{
    stbtt__csctx_close_shape(ctx);
    ctx->first_x = ctx->x = ctx->x + dx;
    ctx->first_y = ctx->y = ctx->y + dy;
    stbtt__csctx_v(ctx, STBTT_vmove, (int)ctx->x, (int)ctx->y, 0, 0, 0, 0);
}

static void stbtt__csctx_rline_to(stbtt__csctx *ctx, float dx, float dy)
{
    ctx->x += dx;
    ctx->y += dy;
    stbtt__csctx_v(ctx, STBTT_vline, (int)ctx->x, (int)ctx->y, 0, 0, 0, 0);
}

static void stbtt__csctx_rccurve_to(stbtt__csctx *ctx, float dx1, float dy1, float dx2, float dy2, float dx3, float dy3)
{
    float cx1 = ctx->x + dx1;
    float cy1 = ctx->y + dy1;
    float cx2 = cx1 + dx2;
    float cy2 = cy1 + dy2;
    ctx->x    = cx2 + dx3;
    ctx->y    = cy2 + dy3;
    stbtt__csctx_v(ctx, STBTT_vcubic, (int)ctx->x, (int)ctx->y, (int)cx1, (int)cy1, (int)cx2, (int)cy2);
}

static stbtt__buf stbtt__get_subr(stbtt__buf idx, int n)
{
    int count = stbtt__cff_index_count(&idx);
    int bias  = 107;
    if (count >= 33900)
        bias = 32768;
    else if (count >= 1240)
        bias = 1131;
    n += bias;
    if (n < 0 || n >= count)
        return stbtt__new_buf(NULL, 0);
    return stbtt__cff_index_get(idx, n);
}

static stbtt__buf stbtt__cid_get_glyph_subrs(const stbtt_fontinfo *info, int glyph_index)
{
    stbtt__buf fdselect = info->fdselect;
    int        nranges, start, end, v, fmt, fdselector = -1, i;

    stbtt__buf_seek(&fdselect, 0);
    fmt = stbtt__buf_get8(&fdselect);
    if (fmt == 0)
    {
        // untested
        stbtt__buf_skip(&fdselect, glyph_index);
        fdselector = stbtt__buf_get8(&fdselect);
    }
    else if (fmt == 3)
    {
        nranges = stbtt__buf_get16(&fdselect);
        start   = stbtt__buf_get16(&fdselect);
        for (i = 0; i < nranges; i++)
        {
            v   = stbtt__buf_get8(&fdselect);
            end = stbtt__buf_get16(&fdselect);
            if (glyph_index >= start && glyph_index < end)
            {
                fdselector = v;
                break;
            }
            start = end;
        }
    }
    if (fdselector == -1)
        stbtt__new_buf(NULL, 0);
    return stbtt__get_subrs(info->cff, stbtt__cff_index_get(info->fontdicts, fdselector));
}

static int stbtt__run_charstring(const stbtt_fontinfo *info, int glyph_index, stbtt__csctx *c)
{
    int        in_header = 1, maskbits = 0, subr_stack_height = 0, sp = 0, v, i, b0;
    int        has_subrs = 0, clear_stack;
    float      s[48];
    stbtt__buf subr_stack[10], subrs = info->subrs, b;
    float      f;

#define STBTT__CSERR(s) (0)

    // this currently ignores the initial width value, which isn't needed if we have hmtx
    b = stbtt__cff_index_get(info->charstrings, glyph_index);
    while (b.cursor < b.size)
    {
        i           = 0;
        clear_stack = 1;
        b0          = stbtt__buf_get8(&b);
        switch (b0)
        {
        // @TODO implement hinting
        case 0x13: // hintmask
        case 0x14: // cntrmask
            if (in_header)
                maskbits += (sp / 2); // implicit "vstem"
            in_header = 0;
            stbtt__buf_skip(&b, (maskbits + 7) / 8);
            break;

        case 0x01: // hstem
        case 0x03: // vstem
        case 0x12: // hstemhm
        case 0x17: // vstemhm
            maskbits += (sp / 2);
            break;

        case 0x15: // rmoveto
            in_header = 0;
            if (sp < 2)
                return STBTT__CSERR("rmoveto stack");
            stbtt__csctx_rmove_to(c, s[sp - 2], s[sp - 1]);
            break;
        case 0x04: // vmoveto
            in_header = 0;
            if (sp < 1)
                return STBTT__CSERR("vmoveto stack");
            stbtt__csctx_rmove_to(c, 0, s[sp - 1]);
            break;
        case 0x16: // hmoveto
            in_header = 0;
            if (sp < 1)
                return STBTT__CSERR("hmoveto stack");
            stbtt__csctx_rmove_to(c, s[sp - 1], 0);
            break;

        case 0x05: // rlineto
            if (sp < 2)
                return STBTT__CSERR("rlineto stack");
            for (; i + 1 < sp; i += 2)
                stbtt__csctx_rline_to(c, s[i], s[i + 1]);
            break;

            // hlineto/vlineto and vhcurveto/hvcurveto alternate horizontal and vertical
            // starting from a different place.

        case 0x07: // vlineto
            if (sp < 1)
                return STBTT__CSERR("vlineto stack");
            goto vlineto;
        case 0x06: // hlineto
            if (sp < 1)
                return STBTT__CSERR("hlineto stack");
            for (;;)
            {
                if (i >= sp)
                    break;
                stbtt__csctx_rline_to(c, s[i], 0);
                i++;
            vlineto:
                if (i >= sp)
                    break;
                stbtt__csctx_rline_to(c, 0, s[i]);
                i++;
            }
            break;

        case 0x1F: // hvcurveto
            if (sp < 4)
                return STBTT__CSERR("hvcurveto stack");
            goto hvcurveto;
        case 0x1E: // vhcurveto
            if (sp < 4)
                return STBTT__CSERR("vhcurveto stack");
            for (;;)
            {
                if (i + 3 >= sp)
                    break;
                stbtt__csctx_rccurve_to(c, 0, s[i], s[i + 1], s[i + 2], s[i + 3], (sp - i == 5) ? s[i + 4] : 0.0f);
                i += 4;
            hvcurveto:
                if (i + 3 >= sp)
                    break;
                stbtt__csctx_rccurve_to(c, s[i], 0, s[i + 1], s[i + 2], (sp - i == 5) ? s[i + 4] : 0.0f, s[i + 3]);
                i += 4;
            }
            break;

        case 0x08: // rrcurveto
            if (sp < 6)
                return STBTT__CSERR("rcurveline stack");
            for (; i + 5 < sp; i += 6)
                stbtt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]);
            break;

        case 0x18: // rcurveline
            if (sp < 8)
                return STBTT__CSERR("rcurveline stack");
            for (; i + 5 < sp - 2; i += 6)
                stbtt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]);
            if (i + 1 >= sp)
                return STBTT__CSERR("rcurveline stack");
            stbtt__csctx_rline_to(c, s[i], s[i + 1]);
            break;

        case 0x19: // rlinecurve
            if (sp < 8)
                return STBTT__CSERR("rlinecurve stack");
            for (; i + 1 < sp - 6; i += 2)
                stbtt__csctx_rline_to(c, s[i], s[i + 1]);
            if (i + 5 >= sp)
                return STBTT__CSERR("rlinecurve stack");
            stbtt__csctx_rccurve_to(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]);
            break;

        case 0x1A: // vvcurveto
        case 0x1B: // hhcurveto
            if (sp < 4)
                return STBTT__CSERR("(vv|hh)curveto stack");
            f = 0.0;
            if (sp & 1)
            {
                f = s[i];
                i++;
            }
            for (; i + 3 < sp; i += 4)
            {
                if (b0 == 0x1B)
                    stbtt__csctx_rccurve_to(c, s[i], f, s[i + 1], s[i + 2], s[i + 3], 0.0);
                else
                    stbtt__csctx_rccurve_to(c, f, s[i], s[i + 1], s[i + 2], 0.0, s[i + 3]);
                f = 0.0;
            }
            break;

        case 0x0A: // callsubr
            if (!has_subrs)
            {
                if (info->fdselect.size)
                    subrs = stbtt__cid_get_glyph_subrs(info, glyph_index);
                has_subrs = 1;
            }
            // FALLTHROUGH
        case 0x1D: // callgsubr
            if (sp < 1)
                return STBTT__CSERR("call(g|)subr stack");
            v = (int)s[--sp];
            if (subr_stack_height >= 10)
                return STBTT__CSERR("recursion limit");
            subr_stack[subr_stack_height++] = b;
            b                               = stbtt__get_subr(b0 == 0x0A ? subrs : info->gsubrs, v);
            if (b.size == 0)
                return STBTT__CSERR("subr not found");
            b.cursor    = 0;
            clear_stack = 0;
            break;

        case 0x0B: // return
            if (subr_stack_height <= 0)
                return STBTT__CSERR("return outside subr");
            b           = subr_stack[--subr_stack_height];
            clear_stack = 0;
            break;

        case 0x0E: // endchar
            stbtt__csctx_close_shape(c);
            return 1;

        case 0x0C:
        { // two-byte escape
            float dx1, dx2, dx3, dx4, dx5, dx6, dy1, dy2, dy3, dy4, dy5, dy6;
            float dx, dy;
            int   b1 = stbtt__buf_get8(&b);
            switch (b1)
            {
            // @TODO These "flex" implementations ignore the flex-depth and resolution,
            // and always draw beziers.
            case 0x22: // hflex
                if (sp < 7)
                    return STBTT__CSERR("hflex stack");
                dx1 = s[0];
                dx2 = s[1];
                dy2 = s[2];
                dx3 = s[3];
                dx4 = s[4];
                dx5 = s[5];
                dx6 = s[6];
                stbtt__csctx_rccurve_to(c, dx1, 0, dx2, dy2, dx3, 0);
                stbtt__csctx_rccurve_to(c, dx4, 0, dx5, -dy2, dx6, 0);
                break;

            case 0x23: // flex
                if (sp < 13)
                    return STBTT__CSERR("flex stack");
                dx1 = s[0];
                dy1 = s[1];
                dx2 = s[2];
                dy2 = s[3];
                dx3 = s[4];
                dy3 = s[5];
                dx4 = s[6];
                dy4 = s[7];
                dx5 = s[8];
                dy5 = s[9];
                dx6 = s[10];
                dy6 = s[11];
                // fd is s[12]
                stbtt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, dy3);
                stbtt__csctx_rccurve_to(c, dx4, dy4, dx5, dy5, dx6, dy6);
                break;

            case 0x24: // hflex1
                if (sp < 9)
                    return STBTT__CSERR("hflex1 stack");
                dx1 = s[0];
                dy1 = s[1];
                dx2 = s[2];
                dy2 = s[3];
                dx3 = s[4];
                dx4 = s[5];
                dx5 = s[6];
                dy5 = s[7];
                dx6 = s[8];
                stbtt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, 0);
                stbtt__csctx_rccurve_to(c, dx4, 0, dx5, dy5, dx6, -(dy1 + dy2 + dy5));
                break;

            case 0x25: // flex1
                if (sp < 11)
                    return STBTT__CSERR("flex1 stack");
                dx1 = s[0];
                dy1 = s[1];
                dx2 = s[2];
                dy2 = s[3];
                dx3 = s[4];
                dy3 = s[5];
                dx4 = s[6];
                dy4 = s[7];
                dx5 = s[8];
                dy5 = s[9];
                dx6 = dy6 = s[10];
                dx        = dx1 + dx2 + dx3 + dx4 + dx5;
                dy        = dy1 + dy2 + dy3 + dy4 + dy5;
                if (STBTT_fabs(dx) > STBTT_fabs(dy))
                    dy6 = -dy;
                else
                    dx6 = -dx;
                stbtt__csctx_rccurve_to(c, dx1, dy1, dx2, dy2, dx3, dy3);
                stbtt__csctx_rccurve_to(c, dx4, dy4, dx5, dy5, dx6, dy6);
                break;

            default:
                return STBTT__CSERR("unimplemented");
            }
        }
        break;

        default:
            if (b0 != 255 && b0 != 28 && b0 < 32)
                return STBTT__CSERR("reserved operator");

            // push immediate
            if (b0 == 255)
            {
                f = (float)(stbtt_int32)stbtt__buf_get32(&b) / 0x10000;
            }
            else
            {
                stbtt__buf_skip(&b, -1);
                f = (float)(stbtt_int16)stbtt__cff_int(&b);
            }
            if (sp >= 48)
                return STBTT__CSERR("push stack overflow");
            s[sp++]     = f;
            clear_stack = 0;
            break;
        }
        if (clear_stack)
            sp = 0;
    }
    return STBTT__CSERR("no endchar");

#undef STBTT__CSERR
}

static int stbtt__GetGlyphShapeT2(const stbtt_fontinfo *info, int glyph_index, stbtt_vertex **pvertices)
{
    // runs the charstring twice, once to count and once to output (to avoid realloc)
    stbtt__csctx count_ctx  = STBTT__CSCTX_INIT(1);
    stbtt__csctx output_ctx = STBTT__CSCTX_INIT(0);
    if (stbtt__run_charstring(info, glyph_index, &count_ctx))
    {
        *pvertices = (stbtt_vertex *)STBTT_malloc(count_ctx.num_vertices * sizeof(stbtt_vertex), info->userdata);
        output_ctx.pvertices = *pvertices;
        if (stbtt__run_charstring(info, glyph_index, &output_ctx))
        {
            STBTT_assert(output_ctx.num_vertices == count_ctx.num_vertices);
            return output_ctx.num_vertices;
        }
    }
    *pvertices = NULL;
    return 0;
}

static int stbtt__GetGlyphInfoT2(const stbtt_fontinfo *info, int glyph_index, int *x0, int *y0, int *x1, int *y1)
{
    stbtt__csctx c = STBTT__CSCTX_INIT(1);
    int          r = stbtt__run_charstring(info, glyph_index, &c);
    if (x0)
        *x0 = r ? c.min_x : 0;
    if (y0)
        *y0 = r ? c.min_y : 0;
    if (x1)
        *x1 = r ? c.max_x : 0;
    if (y1)
        *y1 = r ? c.max_y : 0;
    return r ? c.num_vertices : 0;
}

STBTT_DEF int stbtt_GetGlyphShape(const stbtt_fontinfo *info, int glyph_index, stbtt_vertex **pvertices)
{
    if (!info->cff.size)
        return stbtt__GetGlyphShapeTT(info, glyph_index, pvertices);
    else
        return stbtt__GetGlyphShapeT2(info, glyph_index, pvertices);
}

STBTT_DEF void stbtt_GetGlyphHMetrics(const stbtt_fontinfo *info, int glyph_index, int *advanceWidth,
                                      int *leftSideBearing)
{
    stbtt_uint16 numOfLongHorMetrics = ttUSHORT(info->data + info->hhea + 34);
    if (glyph_index < numOfLongHorMetrics)
    {
        if (advanceWidth)
            *advanceWidth = ttSHORT(info->data + info->hmtx + 4 * glyph_index);
        if (leftSideBearing)
            *leftSideBearing = ttSHORT(info->data + info->hmtx + 4 * glyph_index + 2);
    }
    else
    {
        if (advanceWidth)
            *advanceWidth = ttSHORT(info->data + info->hmtx + 4 * (numOfLongHorMetrics - 1));
        if (leftSideBearing)
            *leftSideBearing =
                ttSHORT(info->data + info->hmtx + 4 * numOfLongHorMetrics + 2 * (glyph_index - numOfLongHorMetrics));
    }
}

STBTT_DEF int stbtt_GetKerningTableLength(const stbtt_fontinfo *info)
{
    stbtt_uint8 *data = info->data + info->kern;

    // we only look at the first table. it must be 'horizontal' and format 0.
    if (!info->kern)
        return 0;
    if (ttUSHORT(data + 2) < 1) // number of tables, need at least 1
        return 0;
    if (ttUSHORT(data + 8) != 1) // horizontal flag must be set in format
        return 0;

    return ttUSHORT(data + 10);
}

STBTT_DEF int stbtt_GetKerningTable(const stbtt_fontinfo *info, stbtt_kerningentry *table, int table_length)
{
    stbtt_uint8 *data = info->data + info->kern;
    int          k, length;

    // we only look at the first table. it must be 'horizontal' and format 0.
    if (!info->kern)
        return 0;
    if (ttUSHORT(data + 2) < 1) // number of tables, need at least 1
        return 0;
    if (ttUSHORT(data + 8) != 1) // horizontal flag must be set in format
        return 0;

    length = ttUSHORT(data + 10);
    if (table_length < length)
        length = table_length;

    for (k = 0; k < length; k++)
    {
        table[k].glyph1  = ttUSHORT(data + 18 + (k * 6));
        table[k].glyph2  = ttUSHORT(data + 20 + (k * 6));
        table[k].advance = ttSHORT(data + 22 + (k * 6));
    }

    return length;
}

static int stbtt__GetGlyphKernInfoAdvance(const stbtt_fontinfo *info, int glyph1, int glyph2)
{
    stbtt_uint8 *data = info->data + info->kern;
    stbtt_uint32 needle, straw;
    int          l, r, m;

    // we only look at the first table. it must be 'horizontal' and format 0.
    if (!info->kern)
        return 0;
    if (ttUSHORT(data + 2) < 1) // number of tables, need at least 1
        return 0;
    if (ttUSHORT(data + 8) != 1) // horizontal flag must be set in format
        return 0;

    l      = 0;
    r      = ttUSHORT(data + 10) - 1;
    needle = glyph1 << 16 | glyph2;
    while (l <= r)
    {
        m     = (l + r) >> 1;
        straw = ttULONG(data + 18 + (m * 6)); // note: unaligned read
        if (needle < straw)
            r = m - 1;
        else if (needle > straw)
            l = m + 1;
        else
            return ttSHORT(data + 22 + (m * 6));
    }
    return 0;
}

static stbtt_int32 stbtt__GetCoverageIndex(stbtt_uint8 *coverageTable, int glyph)
{
    stbtt_uint16 coverageFormat = ttUSHORT(coverageTable);
    switch (coverageFormat)
    {
    case 1:
    {
        stbtt_uint16 glyphCount = ttUSHORT(coverageTable + 2);

        // Binary search.
        stbtt_int32 l = 0, r      = glyphCount - 1, m;
        int         straw, needle = glyph;
        while (l <= r)
        {
            stbtt_uint8 *glyphArray = coverageTable + 4;
            stbtt_uint16 glyphID;
            m       = (l + r) >> 1;
            glyphID = ttUSHORT(glyphArray + 2 * m);
            straw   = glyphID;
            if (needle < straw)
                r = m - 1;
            else if (needle > straw)
                l = m + 1;
            else
            {
                return m;
            }
        }
        break;
    }

    case 2:
    {
        stbtt_uint16 rangeCount = ttUSHORT(coverageTable + 2);
        stbtt_uint8 *rangeArray = coverageTable + 4;

        // Binary search.
        stbtt_int32 l = 0, r = rangeCount - 1, m;
        int         strawStart, strawEnd, needle = glyph;
        while (l <= r)
        {
            stbtt_uint8 *rangeRecord;
            m           = (l + r) >> 1;
            rangeRecord = rangeArray + 6 * m;
            strawStart  = ttUSHORT(rangeRecord);
            strawEnd    = ttUSHORT(rangeRecord + 2);
            if (needle < strawStart)
                r = m - 1;
            else if (needle > strawEnd)
                l = m + 1;
            else
            {
                stbtt_uint16 startCoverageIndex = ttUSHORT(rangeRecord + 4);
                return startCoverageIndex + glyph - strawStart;
            }
        }
        break;
    }

    default:
        return -1; // unsupported
    }

    return -1;
}

static stbtt_int32 stbtt__GetGlyphClass(stbtt_uint8 *classDefTable, int glyph)
{
    stbtt_uint16 classDefFormat = ttUSHORT(classDefTable);
    switch (classDefFormat)
    {
    case 1:
    {
        stbtt_uint16 startGlyphID        = ttUSHORT(classDefTable + 2);
        stbtt_uint16 glyphCount          = ttUSHORT(classDefTable + 4);
        stbtt_uint8 *classDef1ValueArray = classDefTable + 6;

        if (glyph >= startGlyphID && glyph < startGlyphID + glyphCount)
            return (stbtt_int32)ttUSHORT(classDef1ValueArray + 2 * (glyph - startGlyphID));
        break;
    }

    case 2:
    {
        stbtt_uint16 classRangeCount   = ttUSHORT(classDefTable + 2);
        stbtt_uint8 *classRangeRecords = classDefTable + 4;

        // Binary search.
        stbtt_int32 l = 0, r = classRangeCount - 1, m;
        int         strawStart, strawEnd, needle = glyph;
        while (l <= r)
        {
            stbtt_uint8 *classRangeRecord;
            m                = (l + r) >> 1;
            classRangeRecord = classRangeRecords + 6 * m;
            strawStart       = ttUSHORT(classRangeRecord);
            strawEnd         = ttUSHORT(classRangeRecord + 2);
            if (needle < strawStart)
                r = m - 1;
            else if (needle > strawEnd)
                l = m + 1;
            else
                return (stbtt_int32)ttUSHORT(classRangeRecord + 4);
        }
        break;
    }

    default:
        return -1; // Unsupported definition type, return an error.
    }

    // "All glyphs not assigned to a class fall into class 0". (OpenType spec)
    return 0;
}

// Define to STBTT_assert(x) if you want to break on unimplemented formats.
#define STBTT_GPOS_TODO_assert(x)

static stbtt_int32 stbtt__GetGlyphGPOSInfoAdvance(const stbtt_fontinfo *info, int glyph1, int glyph2)
{
    stbtt_uint16 lookupListOffset;
    stbtt_uint8 *lookupList;
    stbtt_uint16 lookupCount;
    stbtt_uint8 *data;
    stbtt_int32  i, sti;

    if (!info->gpos)
        return 0;

    data = info->data + info->gpos;

    if (ttUSHORT(data + 0) != 1)
        return 0; // Major version 1
    if (ttUSHORT(data + 2) != 0)
        return 0; // Minor version 0

    lookupListOffset = ttUSHORT(data + 8);
    lookupList       = data + lookupListOffset;
    lookupCount      = ttUSHORT(lookupList);

    for (i = 0; i < lookupCount; ++i)
    {
        stbtt_uint16 lookupOffset    = ttUSHORT(lookupList + 2 + 2 * i);
        stbtt_uint8 *lookupTable     = lookupList + lookupOffset;

        stbtt_uint16 lookupType      = ttUSHORT(lookupTable);
        stbtt_uint16 subTableCount   = ttUSHORT(lookupTable + 4);
        stbtt_uint8 *subTableOffsets = lookupTable + 6;
        if (lookupType != 2) // Pair Adjustment Positioning Subtable
            continue;

        for (sti = 0; sti < subTableCount; sti++)
        {
            stbtt_uint16 subtableOffset = ttUSHORT(subTableOffsets + 2 * sti);
            stbtt_uint8 *table          = lookupTable + subtableOffset;
            stbtt_uint16 posFormat      = ttUSHORT(table);
            stbtt_uint16 coverageOffset = ttUSHORT(table + 2);
            stbtt_int32  coverageIndex  = stbtt__GetCoverageIndex(table + coverageOffset, glyph1);
            if (coverageIndex == -1)
                continue;

            switch (posFormat)
            {
            case 1:
            {
                stbtt_int32  l, r, m;
                int          straw, needle;
                stbtt_uint16 valueFormat1 = ttUSHORT(table + 4);
                stbtt_uint16 valueFormat2 = ttUSHORT(table + 6);
                if (valueFormat1 == 4 && valueFormat2 == 0)
                { // Support more formats?
                    stbtt_int32  valueRecordPairSizeInBytes = 2;
                    stbtt_uint16 pairSetCount               = ttUSHORT(table + 8);
                    stbtt_uint16 pairPosOffset              = ttUSHORT(table + 10 + 2 * coverageIndex);
                    stbtt_uint8 *pairValueTable             = table + pairPosOffset;
                    stbtt_uint16 pairValueCount             = ttUSHORT(pairValueTable);
                    stbtt_uint8 *pairValueArray             = pairValueTable + 2;

                    if (coverageIndex >= pairSetCount)
                        return 0;

                    needle = glyph2;
                    r      = pairValueCount - 1;
                    l      = 0;

                    // Binary search.
                    while (l <= r)
                    {
                        stbtt_uint16 secondGlyph;
                        stbtt_uint8 *pairValue;
                        m           = (l + r) >> 1;
                        pairValue   = pairValueArray + (2 + valueRecordPairSizeInBytes) * m;
                        secondGlyph = ttUSHORT(pairValue);
                        straw       = secondGlyph;
                        if (needle < straw)
                            r = m - 1;
                        else if (needle > straw)
                            l = m + 1;
                        else
                        {
                            stbtt_int16 xAdvance = ttSHORT(pairValue + 2);
                            return xAdvance;
                        }
                    }
                }
                else
                    return 0;
                break;
            }

            case 2:
            {
                stbtt_uint16 valueFormat1 = ttUSHORT(table + 4);
                stbtt_uint16 valueFormat2 = ttUSHORT(table + 6);
                if (valueFormat1 == 4 && valueFormat2 == 0)
                { // Support more formats?
                    stbtt_uint16 classDef1Offset = ttUSHORT(table + 8);
                    stbtt_uint16 classDef2Offset = ttUSHORT(table + 10);
                    int          glyph1class     = stbtt__GetGlyphClass(table + classDef1Offset, glyph1);
                    int          glyph2class     = stbtt__GetGlyphClass(table + classDef2Offset, glyph2);

                    stbtt_uint16 class1Count     = ttUSHORT(table + 12);
                    stbtt_uint16 class2Count     = ttUSHORT(table + 14);
                    stbtt_uint8 *class1Records, *class2Records;
                    stbtt_int16  xAdvance;

                    if (glyph1class < 0 || glyph1class >= class1Count)
                        return 0; // malformed
                    if (glyph2class < 0 || glyph2class >= class2Count)
                        return 0; // malformed

                    class1Records = table + 16;
                    class2Records = class1Records + 2 * (glyph1class * class2Count);
                    xAdvance      = ttSHORT(class2Records + 2 * glyph2class);
                    return xAdvance;
                }
                else
                    return 0;
                break;
            }

            default:
                return 0; // Unsupported position format
            }
        }
    }

    return 0;
}

STBTT_DEF int stbtt_GetGlyphKernAdvance(const stbtt_fontinfo *info, int g1, int g2)
{
    int xAdvance = 0;

    if (info->gpos)
        xAdvance += stbtt__GetGlyphGPOSInfoAdvance(info, g1, g2);
    else if (info->kern)
        xAdvance += stbtt__GetGlyphKernInfoAdvance(info, g1, g2);

    return xAdvance;
}

STBTT_DEF int stbtt_GetCodepointKernAdvance(const stbtt_fontinfo *info, int ch1, int ch2)
{
    if (!info->kern && !info->gpos) // if no kerning table, don't waste time looking up both codepoint->glyphs
        return 0;
    return stbtt_GetGlyphKernAdvance(info, stbtt_FindGlyphIndex(info, ch1), stbtt_FindGlyphIndex(info, ch2));
}

STBTT_DEF void stbtt_GetCodepointHMetrics(const stbtt_fontinfo *info, int codepoint, int *advanceWidth,
                                          int *leftSideBearing)
{
    stbtt_GetGlyphHMetrics(info, stbtt_FindGlyphIndex(info, codepoint), advanceWidth, leftSideBearing);
}

STBTT_DEF void stbtt_GetFontVMetrics(const stbtt_fontinfo *info, int *ascent, int *descent, int *lineGap)
{
    if (ascent)
        *ascent = ttSHORT(info->data + info->hhea + 4);
    if (descent)
        *descent = ttSHORT(info->data + info->hhea + 6);
    if (lineGap)
        *lineGap = ttSHORT(info->data + info->hhea + 8);
}

STBTT_DEF int stbtt_GetFontVMetricsOS2(const stbtt_fontinfo *info, int *typoAscent, int *typoDescent, int *typoLineGap)
{
    int tab = stbtt__find_table(info->data, info->fontstart, "OS/2");
    if (!tab)
        return 0;
    if (typoAscent)
        *typoAscent = ttSHORT(info->data + tab + 68);
    if (typoDescent)
        *typoDescent = ttSHORT(info->data + tab + 70);
    if (typoLineGap)
        *typoLineGap = ttSHORT(info->data + tab + 72);
    return 1;
}

STBTT_DEF void stbtt_GetFontBoundingBox(const stbtt_fontinfo *info, int *x0, int *y0, int *x1, int *y1)
{
    *x0 = ttSHORT(info->data + info->head + 36);
    *y0 = ttSHORT(info->data + info->head + 38);
    *x1 = ttSHORT(info->data + info->head + 40);
    *y1 = ttSHORT(info->data + info->head + 42);
}

STBTT_DEF float stbtt_ScaleForPixelHeight(const stbtt_fontinfo *info, float height)
{
    int fheight = ttSHORT(info->data + info->hhea + 4) - ttSHORT(info->data + info->hhea + 6);
    return (float)height / fheight;
}

STBTT_DEF float stbtt_ScaleForMappingEmToPixels(const stbtt_fontinfo *info, float pixels)
{
    int unitsPerEm = ttUSHORT(info->data + info->head + 18);
    return pixels / unitsPerEm;
}

STBTT_DEF void stbtt_FreeShape(const stbtt_fontinfo *info, stbtt_vertex *v)
{
    STBTT_free(v, info->userdata);
}

STBTT_DEF stbtt_uint8 *stbtt_FindSVGDoc(const stbtt_fontinfo *info, int gl)
{
    int          i;
    stbtt_uint8 *data         = info->data;
    stbtt_uint8 *svg_doc_list = data + stbtt__get_svg((stbtt_fontinfo *)info);

    int          numEntries   = ttUSHORT(svg_doc_list);
    stbtt_uint8 *svg_docs     = svg_doc_list + 2;

    for (i = 0; i < numEntries; i++)
    {
        stbtt_uint8 *svg_doc = svg_docs + (12 * i);
        if ((gl >= ttUSHORT(svg_doc)) && (gl <= ttUSHORT(svg_doc + 2)))
            return svg_doc;
    }
    return 0;
}

STBTT_DEF int stbtt_GetGlyphSVG(const stbtt_fontinfo *info, int gl, const char **svg)
{
    stbtt_uint8 *data = info->data;
    stbtt_uint8 *svg_doc;

    if (info->svg == 0)
        return 0;

    svg_doc = stbtt_FindSVGDoc(info, gl);
    if (svg_doc != NULL)
    {
        *svg = (char *)data + info->svg + ttULONG(svg_doc + 4);
        return ttULONG(svg_doc + 8);
    }
    else
    {
        return 0;
    }
}

STBTT_DEF int stbtt_GetCodepointSVG(const stbtt_fontinfo *info, int unicode_codepoint, const char **svg)
{
    return stbtt_GetGlyphSVG(info, stbtt_FindGlyphIndex(info, unicode_codepoint), svg);
}

//////////////////////////////////////////////////////////////////////////////
//
// antialiasing software rasterizer
//

STBTT_DEF void stbtt_GetGlyphBitmapBoxSubpixel(const stbtt_fontinfo *font, int glyph, float scale_x, float scale_y,
                                               float shift_x, float shift_y, int *ix0, int *iy0, int *ix1, int *iy1)
{
    int x0 = 0, y0 = 0, x1, y1; // =0 suppresses compiler warning
    if (!stbtt_GetGlyphBox(font, glyph, &x0, &y0, &x1, &y1))
    {
        // e.g. space character
        if (ix0)
            *ix0 = 0;
        if (iy0)
            *iy0 = 0;
        if (ix1)
            *ix1 = 0;
        if (iy1)
            *iy1 = 0;
    }
    else
    {
        // move to integral bboxes (treating pixels as little squares, what pixels get touched)?
        if (ix0)
            *ix0 = STBTT_ifloor(x0 * scale_x + shift_x);
        if (iy0)
            *iy0 = STBTT_ifloor(-y1 * scale_y + shift_y);
        if (ix1)
            *ix1 = STBTT_iceil(x1 * scale_x + shift_x);
        if (iy1)
            *iy1 = STBTT_iceil(-y0 * scale_y + shift_y);
    }
}

STBTT_DEF void stbtt_GetGlyphBitmapBox(const stbtt_fontinfo *font, int glyph, float scale_x, float scale_y, int *ix0,
                                       int *iy0, int *ix1, int *iy1)
{
    stbtt_GetGlyphBitmapBoxSubpixel(font, glyph, scale_x, scale_y, 0.0f, 0.0f, ix0, iy0, ix1, iy1);
}

STBTT_DEF void stbtt_GetCodepointBitmapBoxSubpixel(const stbtt_fontinfo *font, int codepoint, float scale_x,
                                                   float scale_y, float shift_x, float shift_y, int *ix0, int *iy0,
                                                   int *ix1, int *iy1)
{
    stbtt_GetGlyphBitmapBoxSubpixel(font, stbtt_FindGlyphIndex(font, codepoint), scale_x, scale_y, shift_x, shift_y,
                                    ix0, iy0, ix1, iy1);
}

STBTT_DEF void stbtt_GetCodepointBitmapBox(const stbtt_fontinfo *font, int codepoint, float scale_x, float scale_y,
                                           int *ix0, int *iy0, int *ix1, int *iy1)
{
    stbtt_GetCodepointBitmapBoxSubpixel(font, codepoint, scale_x, scale_y, 0.0f, 0.0f, ix0, iy0, ix1, iy1);
}

//////////////////////////////////////////////////////////////////////////////
//
//  Rasterizer

typedef struct stbtt__hheap_chunk
{
    struct stbtt__hheap_chunk *next;
} stbtt__hheap_chunk;

typedef struct stbtt__hheap
{
    struct stbtt__hheap_chunk *head;
    void                      *first_free;
    int                        num_remaining_in_head_chunk;
} stbtt__hheap;

static void *stbtt__hheap_alloc(stbtt__hheap *hh, size_t size, void *userdata)
{
    if (hh->first_free)
    {
        void *p        = hh->first_free;
        hh->first_free = *(void **)p;
        return p;
    }
    else
    {
        if (hh->num_remaining_in_head_chunk == 0)
        {
            int                 count = (size < 32 ? 2000 : size < 128 ? 800 : 100);
            stbtt__hheap_chunk *c =
                (stbtt__hheap_chunk *)STBTT_malloc(sizeof(stbtt__hheap_chunk) + size * count, userdata);
            if (c == NULL)
                return NULL;
            c->next                         = hh->head;
            hh->head                        = c;
            hh->num_remaining_in_head_chunk = count;
        }
        --hh->num_remaining_in_head_chunk;
        return (char *)(hh->head) + sizeof(stbtt__hheap_chunk) + size * hh->num_remaining_in_head_chunk;
    }
}

static void stbtt__hheap_free(stbtt__hheap *hh, void *p)
{
    *(void **)p    = hh->first_free;
    hh->first_free = p;
}

static void stbtt__hheap_cleanup(stbtt__hheap *hh, void *userdata)
{
    stbtt__hheap_chunk *c = hh->head;
    while (c)
    {
        stbtt__hheap_chunk *n = c->next;
        STBTT_free(c, userdata);
        c = n;
    }
}

typedef struct stbtt__edge
{
    float x0, y0, x1, y1;
    int   invert;
} stbtt__edge;

typedef struct stbtt__active_edge
{
    struct stbtt__active_edge *next;
#if STBTT_RASTERIZER_VERSION == 1
    int   x, dx;
    float ey;
    int   direction;
#elif STBTT_RASTERIZER_VERSION == 2
    float fx, fdx, fdy;
    float direction;
    float sy;
    float ey;
#else
#error "Unrecognized value of STBTT_RASTERIZER_VERSION"
#endif
} stbtt__active_edge;

#if STBTT_RASTERIZER_VERSION == 1
#define STBTT_FIXSHIFT 10
#define STBTT_FIX (1 << STBTT_FIXSHIFT)
#define STBTT_FIXMASK (STBTT_FIX - 1)

static stbtt__active_edge *stbtt__new_active(stbtt__hheap *hh, stbtt__edge *e, int off_x, float start_point,
                                             void *userdata)
{
    stbtt__active_edge *z    = (stbtt__active_edge *)stbtt__hheap_alloc(hh, sizeof(*z), userdata);
    float               dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
    STBTT_assert(z != NULL);
    if (!z)
        return z;

    // round dx down to avoid overshooting
    if (dxdy < 0)
        z->dx = -STBTT_ifloor(STBTT_FIX * -dxdy);
    else
        z->dx = STBTT_ifloor(STBTT_FIX * dxdy);

    z->x = STBTT_ifloor(STBTT_FIX * e->x0 +
                        z->dx * (start_point - e->y0)); // use z->dx so when we offset later it's by the same amount
    z->x -= off_x * STBTT_FIX;

    z->ey        = e->y1;
    z->next      = 0;
    z->direction = e->invert ? 1 : -1;
    return z;
}
#elif STBTT_RASTERIZER_VERSION == 2
static stbtt__active_edge *stbtt__new_active(stbtt__hheap *hh, stbtt__edge *e, int off_x, float start_point,
                                             void *userdata)
{
    stbtt__active_edge *z = (stbtt__active_edge *)stbtt__hheap_alloc(hh, sizeof(*z), userdata);
    float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
    STBTT_assert(z != NULL);
    // STBTT_assert(e->y0 <= start_point);
    if (!z)
        return z;
    z->fdx = dxdy;
    z->fdy = dxdy != 0.0f ? (1.0f / dxdy) : 0.0f;
    z->fx = e->x0 + dxdy * (start_point - e->y0);
    z->fx -= off_x;
    z->direction = e->invert ? 1.0f : -1.0f;
    z->sy = e->y0;
    z->ey = e->y1;
    z->next = 0;
    return z;
}
#else
#error "Unrecognized value of STBTT_RASTERIZER_VERSION"
#endif

#if STBTT_RASTERIZER_VERSION == 1
// note: this routine clips fills that extend off the edges... ideally this
// wouldn't happen, but it could happen if the truetype glyph bounding boxes
// are wrong, or if the user supplies a too-small bitmap
static void stbtt__fill_active_edges(unsigned char *scanline, int len, stbtt__active_edge *e, int max_weight)
{
    // non-zero winding fill
    int x0 = 0, w = 0;

    while (e)
    {
        if (w == 0)
        {
            // if we're currently at zero, we need to record the edge start point
            x0 = e->x;
            w += e->direction;
        }
        else
        {
            int x1 = e->x;
            w += e->direction;
            // if we went to zero, we need to draw
            if (w == 0)
            {
                int i = x0 >> STBTT_FIXSHIFT;
                int j = x1 >> STBTT_FIXSHIFT;

                if (i < len && j >= 0)
                {
                    if (i == j)
                    {
                        // x0,x1 are the same pixel, so compute combined coverage
                        scanline[i] = scanline[i] + (stbtt_uint8)((x1 - x0) * max_weight >> STBTT_FIXSHIFT);
                    }
                    else
                    {
                        if (i >= 0) // add antialiasing for x0
                            scanline[i] =
                                scanline[i] +
                                (stbtt_uint8)(((STBTT_FIX - (x0 & STBTT_FIXMASK)) * max_weight) >> STBTT_FIXSHIFT);
                        else
                            i = -1; // clip

                        if (j < len) // add antialiasing for x1
                            scanline[j] =
                                scanline[j] + (stbtt_uint8)(((x1 & STBTT_FIXMASK) * max_weight) >> STBTT_FIXSHIFT);
                        else
                            j = len; // clip

                        for (++i; i < j; ++i) // fill pixels between x0 and x1
                            scanline[i] = scanline[i] + (stbtt_uint8)max_weight;
                    }
                }
            }
        }

        e = e->next;
    }
}

static void stbtt__rasterize_sorted_edges(stbtt__bitmap *result, stbtt__edge *e, int n, int vsubsample, int off_x,
                                          int off_y, void *userdata)
{
    stbtt__hheap        hh     = {0, 0, 0};
    stbtt__active_edge *active = NULL;
    int                 y, j = 0;
    int                 max_weight = (255 / vsubsample); // weight per vertical scanline
    int                 s;                               // vertical subsample index
    unsigned char       scanline_data[512], *scanline;

    if (result->w > 512)
        scanline = (unsigned char *)STBTT_malloc(result->w, userdata);
    else
        scanline = scanline_data;

    y       = off_y * vsubsample;
    e[n].y0 = (off_y + result->h) * (float)vsubsample + 1;

    while (j < result->h)
    {
        STBTT_memset(scanline, 0, result->w);
        for (s = 0; s < vsubsample; ++s)
        {
            // find center of pixel for this scanline
            float                scan_y = y + 0.5f;
            stbtt__active_edge **step   = &active;

            // update all active edges;
            // remove all active edges that terminate before the center of this scanline
            while (*step)
            {
                stbtt__active_edge *z = *step;
                if (z->ey <= scan_y)
                {
                    *step = z->next; // delete from list
                    STBTT_assert(z->direction);
                    z->direction = 0;
                    stbtt__hheap_free(&hh, z);
                }
                else
                {
                    z->x += z->dx;           // advance to position for current scanline
                    step = &((*step)->next); // advance through list
                }
            }

            // resort the list if needed
            for (;;)
            {
                int changed = 0;
                step        = &active;
                while (*step && (*step)->next)
                {
                    if ((*step)->x > (*step)->next->x)
                    {
                        stbtt__active_edge *t = *step;
                        stbtt__active_edge *q = t->next;

                        t->next               = q->next;
                        q->next               = t;
                        *step                 = q;
                        changed               = 1;
                    }
                    step = &(*step)->next;
                }
                if (!changed)
                    break;
            }

            // insert all edges that start before the center of this scanline -- omit ones that also end on this
            // scanline
            while (e->y0 <= scan_y)
            {
                if (e->y1 > scan_y)
                {
                    stbtt__active_edge *z = stbtt__new_active(&hh, e, off_x, scan_y, userdata);
                    if (z != NULL)
                    {
                        // find insertion point
                        if (active == NULL)
                            active = z;
                        else if (z->x < active->x)
                        {
                            // insert at front
                            z->next = active;
                            active  = z;
                        }
                        else
                        {
                            // find thing to insert AFTER
                            stbtt__active_edge *p = active;
                            while (p->next && p->next->x < z->x)
                                p = p->next;
                            // at this point, p->next->x is NOT < z->x
                            z->next = p->next;
                            p->next = z;
                        }
                    }
                }
                ++e;
            }

            // now process all active edges in XOR fashion
            if (active)
                stbtt__fill_active_edges(scanline, result->w, active, max_weight);

            ++y;
        }
        STBTT_memcpy(result->pixels + j * result->stride, scanline, result->w);
        ++j;
    }

    stbtt__hheap_cleanup(&hh, userdata);

    if (scanline != scanline_data)
        STBTT_free(scanline, userdata);
}

#elif STBTT_RASTERIZER_VERSION == 2

// the edge passed in here does not cross the vertical line at x or the vertical line at x+1
// (i.e. it has already been clipped to those)
static void stbtt__handle_clipped_edge(float *scanline, int x, stbtt__active_edge *e, float x0, float y0, float x1,
                                       float y1)
{
    if (y0 == y1)
        return;
    STBTT_assert(y0 < y1);
    STBTT_assert(e->sy <= e->ey);
    if (y0 > e->ey)
        return;
    if (y1 < e->sy)
        return;
    if (y0 < e->sy)
    {
        x0 += (x1 - x0) * (e->sy - y0) / (y1 - y0);
        y0 = e->sy;
    }
    if (y1 > e->ey)
    {
        x1 += (x1 - x0) * (e->ey - y1) / (y1 - y0);
        y1 = e->ey;
    }

    if (x0 == x)
        STBTT_assert(x1 <= x + 1);
    else if (x0 == x + 1)
        STBTT_assert(x1 >= x);
    else if (x0 <= x)
        STBTT_assert(x1 <= x);
    else if (x0 >= x + 1)
        STBTT_assert(x1 >= x + 1);
    else
        STBTT_assert(x1 >= x && x1 <= x + 1);

    if (x0 <= x && x1 <= x)
        scanline[x] += e->direction * (y1 - y0);
    else if (x0 >= x + 1 && x1 >= x + 1)
        ;
    else
    {
        STBTT_assert(x0 >= x && x0 <= x + 1 && x1 >= x && x1 <= x + 1);
        scanline[x] += e->direction * (y1 - y0) * (1 - ((x0 - x) + (x1 - x)) / 2); // coverage = 1 - average x position
    }
}

static float stbtt__sized_trapezoid_area(float height, float top_width, float bottom_width)
{
    STBTT_assert(top_width >= 0);
    STBTT_assert(bottom_width >= 0);
    return (top_width + bottom_width) / 2.0f * height;
}

static float stbtt__position_trapezoid_area(float height, float tx0, float tx1, float bx0, float bx1)
{
    return stbtt__sized_trapezoid_area(height, tx1 - tx0, bx1 - bx0);
}

static float stbtt__sized_triangle_area(float height, float width)
{
    return height * width / 2;
}

static void stbtt__fill_active_edges_new(float *scanline, float *scanline_fill, int len, stbtt__active_edge *e,
                                         float y_top)
{
    float y_bottom = y_top + 1;

    while (e)
    {
        // brute force every pixel

        // compute intersection points with top & bottom
        STBTT_assert(e->ey >= y_top);

        if (e->fdx == 0)
        {
            float x0 = e->fx;
            if (x0 < len)
            {
                if (x0 >= 0)
                {
                    stbtt__handle_clipped_edge(scanline, (int)x0, e, x0, y_top, x0, y_bottom);
                    stbtt__handle_clipped_edge(scanline_fill - 1, (int)x0 + 1, e, x0, y_top, x0, y_bottom);
                }
                else
                {
                    stbtt__handle_clipped_edge(scanline_fill - 1, 0, e, x0, y_top, x0, y_bottom);
                }
            }
        }
        else
        {
            float x0 = e->fx;
            float dx = e->fdx;
            float xb = x0 + dx;
            float x_top, x_bottom;
            float sy0, sy1;
            float dy = e->fdy;
            STBTT_assert(e->sy <= y_bottom && e->ey >= y_top);

            // compute endpoints of line segment clipped to this scanline (if the
            // line segment starts on this scanline. x0 is the intersection of the
            // line with y_top, but that may be off the line segment.
            if (e->sy > y_top)
            {
                x_top = x0 + dx * (e->sy - y_top);
                sy0 = e->sy;
            }
            else
            {
                x_top = x0;
                sy0 = y_top;
            }
            if (e->ey < y_bottom)
            {
                x_bottom = x0 + dx * (e->ey - y_top);
                sy1 = e->ey;
            }
            else
            {
                x_bottom = xb;
                sy1 = y_bottom;
            }

            if (x_top >= 0 && x_bottom >= 0 && x_top < len && x_bottom < len)
            {
                // from here on, we don't have to range check x values

                if ((int)x_top == (int)x_bottom)
                {
                    float height;
                    // simple case, only spans one pixel
                    int x = (int)x_top;
                    height = (sy1 - sy0) * e->direction;
                    STBTT_assert(x >= 0 && x < len);
                    scanline[x] += stbtt__position_trapezoid_area(height, x_top, x + 1.0f, x_bottom, x + 1.0f);
                    scanline_fill[x] += height; // everything right of this pixel is filled
                }
                else
                {
                    int x, x1, x2;
                    float y_crossing, y_final, step, sign, area;
                    // covers 2+ pixels
                    if (x_top > x_bottom)
                    {
                        // flip scanline vertically; signed area is the same
                        float t;
                        sy0 = y_bottom - (sy0 - y_top);
                        sy1 = y_bottom - (sy1 - y_top);
                        t = sy0, sy0 = sy1, sy1 = t;
                        t = x_bottom, x_bottom = x_top, x_top = t;
                        dx = -dx;
                        dy = -dy;
                        t = x0, x0 = xb, xb = t;
                    }
                    STBTT_assert(dy >= 0);
                    STBTT_assert(dx >= 0);

                    x1 = (int)x_top;
                    x2 = (int)x_bottom;
                    // compute intersection with y axis at x1+1
                    y_crossing = y_top + dy * (x1 + 1 - x0);

                    // compute intersection with y axis at x2
                    y_final = y_top + dy * (x2 - x0);

                    //           x1    x_top                            x2    x_bottom
                    //     y_top  +------|-----+------------+------------+--------|---+------------+
                    //            |            |            |            |            |            |
                    //            |            |            |            |            |            |
                    //       sy0  |      Txxxxx|............|............|............|............|
                    // y_crossing |            *xxxxx.......|............|............|............|
                    //            |            |     xxxxx..|............|............|............|
                    //            |            |     /-   xx*xxxx........|............|............|
                    //            |            | dy <       |    xxxxxx..|............|............|
                    //   y_final  |            |     \-     |          xx*xxx.........|............|
                    //       sy1  |            |            |            |   xxxxxB...|............|
                    //            |            |            |            |            |            |
                    //            |            |            |            |            |            |
                    //  y_bottom  +------------+------------+------------+------------+------------+
                    //
                    // goal is to measure the area covered by '.' in each pixel

                    // if x2 is right at the right edge of x1, y_crossing can blow up, github #1057
                    // @TODO: maybe test against sy1 rather than y_bottom?
                    if (y_crossing > y_bottom)
                        y_crossing = y_bottom;

                    sign = e->direction;

                    // area of the rectangle covered from sy0..y_crossing
                    area = sign * (y_crossing - sy0);

                    // area of the triangle (x_top,sy0), (x1+1,sy0), (x1+1,y_crossing)
                    scanline[x1] += stbtt__sized_triangle_area(area, x1 + 1 - x_top);

                    // check if final y_crossing is blown up; no test case for this
                    if (y_final > y_bottom)
                    {
                        y_final = y_bottom;
                        dy = (y_final - y_crossing) /
                             (x2 - (x1 + 1)); // if denom=0, y_final = y_crossing, so y_final <= y_bottom
                    }

                    // in second pixel, area covered by line segment found in first pixel
                    // is always a rectangle 1 wide * the height of that line segment; this
                    // is exactly what the variable 'area' stores. it also gets a contribution
                    // from the line segment within it. the THIRD pixel will get the first
                    // pixel's rectangle contribution, the second pixel's rectangle contribution,
                    // and its own contribution. the 'own contribution' is the same in every pixel except
                    // the leftmost and rightmost, a trapezoid that slides down in each pixel.
                    // the second pixel's contribution to the third pixel will be the
                    // rectangle 1 wide times the height change in the second pixel, which is dy.

                    step = sign * dy * 1; // dy is dy/dx, change in y for every 1 change in x,
                    // which multiplied by 1-pixel-width is how much pixel area changes for each step in x
                    // so the area advances by 'step' every time

                    for (x = x1 + 1; x < x2; ++x)
                    {
                        scanline[x] += area + step / 2; // area of trapezoid is 1*step/2
                        area += step;
                    }
                    STBTT_assert(STBTT_fabs(area) <=
                                 1.01f); // accumulated error from area += step unless we round step down
                    STBTT_assert(sy1 > y_final - 0.01f);

                    // area covered in the last pixel is the rectangle from all the pixels to the left,
                    // plus the trapezoid filled by the line segment in this pixel all the way to the right edge
                    scanline[x2] += area + sign * stbtt__position_trapezoid_area(sy1 - y_final, (float)x2, x2 + 1.0f,
                                                                                 x_bottom, x2 + 1.0f);

                    // the rest of the line is filled based on the total height of the line segment in this pixel
                    scanline_fill[x2] += sign * (sy1 - sy0);
                }
            }
            else
            {
                // if edge goes outside of box we're drawing, we require
                // clipping logic. since this does not match the intended use
                // of this library, we use a different, very slow brute
                // force implementation
                // note though that this does happen some of the time because
                // x_top and x_bottom can be extrapolated at the top & bottom of
                // the shape and actually lie outside the bounding box
                int x;
                for (x = 0; x < len; ++x)
                {
                    // cases:
                    //
                    // there can be up to two intersections with the pixel. any intersection
                    // with left or right edges can be handled by splitting into two (or three)
                    // regions. intersections with top & bottom do not necessitate case-wise logic.
                    //
                    // the old way of doing this found the intersections with the left & right edges,
                    // then used some simple logic to produce up to three segments in sorted order
                    // from top-to-bottom. however, this had a problem: if an x edge was epsilon
                    // across the x border, then the corresponding y position might not be distinct
                    // from the other y segment, and it might ignored as an empty segment. to avoid
                    // that, we need to explicitly produce segments based on x positions.

                    // rename variables to clearly-defined pairs
                    float y0 = y_top;
                    float x1 = (float)(x);
                    float x2 = (float)(x + 1);
                    float x3 = xb;
                    float y3 = y_bottom;

                    // x = e->x + e->dx * (y-y_top)
                    // (y-y_top) = (x - e->x) / e->dx
                    // y = (x - e->x) / e->dx + y_top
                    float y1 = (x - x0) / dx + y_top;
                    float y2 = (x + 1 - x0) / dx + y_top;

                    if (x0 < x1 && x3 > x2)
                    { // three segments descending down-right
                        stbtt__handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
                        stbtt__handle_clipped_edge(scanline, x, e, x1, y1, x2, y2);
                        stbtt__handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
                    }
                    else if (x3 < x1 && x0 > x2)
                    { // three segments descending down-left
                        stbtt__handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
                        stbtt__handle_clipped_edge(scanline, x, e, x2, y2, x1, y1);
                        stbtt__handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
                    }
                    else if (x0 < x1 && x3 > x1)
                    { // two segments across x, down-right
                        stbtt__handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
                        stbtt__handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
                    }
                    else if (x3 < x1 && x0 > x1)
                    { // two segments across x, down-left
                        stbtt__handle_clipped_edge(scanline, x, e, x0, y0, x1, y1);
                        stbtt__handle_clipped_edge(scanline, x, e, x1, y1, x3, y3);
                    }
                    else if (x0 < x2 && x3 > x2)
                    { // two segments across x+1, down-right
                        stbtt__handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
                        stbtt__handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
                    }
                    else if (x3 < x2 && x0 > x2)
                    { // two segments across x+1, down-left
                        stbtt__handle_clipped_edge(scanline, x, e, x0, y0, x2, y2);
                        stbtt__handle_clipped_edge(scanline, x, e, x2, y2, x3, y3);
                    }
                    else
                    { // one segment
                        stbtt__handle_clipped_edge(scanline, x, e, x0, y0, x3, y3);
                    }
                }
            }
        }
        e = e->next;
    }
}

// directly AA rasterize edges w/o supersampling
static void stbtt__rasterize_sorted_edges(stbtt__bitmap *result, stbtt__edge *e, int n, int vsubsample, int off_x,
                                          int off_y, void *userdata)
{
    stbtt__hheap hh = {0, 0, 0};
    stbtt__active_edge *active = NULL;
    int y, j = 0, i;
    float scanline_data[129], *scanline, *scanline2;

    STBTT__NOTUSED(vsubsample);

    if (result->w > 64)
        scanline = (float *)STBTT_malloc((result->w * 2 + 1) * sizeof(float), userdata);
    else
        scanline = scanline_data;

    scanline2 = scanline + result->w;

    y = off_y;
    e[n].y0 = (float)(off_y + result->h) + 1;

    while (j < result->h)
    {
        // find center of pixel for this scanline
        float scan_y_top = y + 0.0f;
        float scan_y_bottom = y + 1.0f;
        stbtt__active_edge **step = &active;

        STBTT_memset(scanline, 0, result->w * sizeof(scanline[0]));
        STBTT_memset(scanline2, 0, (result->w + 1) * sizeof(scanline[0]));

        // update all active edges;
        // remove all active edges that terminate before the top of this scanline
        while (*step)
        {
            stbtt__active_edge *z = *step;
            if (z->ey <= scan_y_top)
            {
                *step = z->next; // delete from list
                STBTT_assert(z->direction);
                z->direction = 0;
                stbtt__hheap_free(&hh, z);
            }
            else
            {
                step = &((*step)->next); // advance through list
            }
        }

        // insert all edges that start before the bottom of this scanline
        while (e->y0 <= scan_y_bottom)
        {
            if (e->y0 != e->y1)
            {
                stbtt__active_edge *z = stbtt__new_active(&hh, e, off_x, scan_y_top, userdata);
                if (z != NULL)
                {
                    if (j == 0 && off_y != 0)
                    {
                        if (z->ey < scan_y_top)
                        {
                            // this can happen due to subpixel positioning and some kind of fp rounding error i
                            // think
                            z->ey = scan_y_top;
                        }
                    }
                    STBTT_assert(z->ey >=
                                 scan_y_top); // if we get really unlucky a tiny bit of an edge can be out of bounds
                    // insert at front
                    z->next = active;
                    active = z;
                }
            }
            ++e;
        }

        // now process all active edges
        if (active)
            stbtt__fill_active_edges_new(scanline, scanline2 + 1, result->w, active, scan_y_top);

        {
            float sum = 0;
            for (i = 0; i < result->w; ++i)
            {
                float k;
                int m;
                sum += scanline2[i];
                k = scanline[i] + sum;
                k = (float)STBTT_fabs(k) * 255 + 0.5f;
                m = (int)k;
                if (m > 255)
                    m = 255;
                result->pixels[j * result->stride + i] = (unsigned char)m;
            }
        }
        // advance all the edges
        step = &active;
        while (*step)
        {
            stbtt__active_edge *z = *step;
            z->fx += z->fdx;         // advance to position for current scanline
            step = &((*step)->next); // advance through list
        }

        ++y;
        ++j;
    }

    stbtt__hheap_cleanup(&hh, userdata);

    if (scanline != scanline_data)
        STBTT_free(scanline, userdata);
}
#else
#error "Unrecognized value of STBTT_RASTERIZER_VERSION"
#endif

#define STBTT__COMPARE(a, b) ((a)->y0 < (b)->y0)

static void stbtt__sort_edges_ins_sort(stbtt__edge *p, int n)
{
    int i, j;
    for (i = 1; i < n; ++i)
    {
        stbtt__edge t = p[i], *a = &t;
        j = i;
        while (j > 0)
        {
            stbtt__edge *b = &p[j - 1];
            int          c = STBTT__COMPARE(a, b);
            if (!c)
                break;
            p[j] = p[j - 1];
            --j;
        }
        if (i != j)
            p[j] = t;
    }
}

static void stbtt__sort_edges_quicksort(stbtt__edge *p, int n)
{
    /* threshold for transitioning to insertion sort */
    while (n > 12)
    {
        stbtt__edge t;
        int         c01, c12, c, m, i, j;

        /* compute median of three */
        m   = n >> 1;
        c01 = STBTT__COMPARE(&p[0], &p[m]);
        c12 = STBTT__COMPARE(&p[m], &p[n - 1]);
        /* if 0 >= mid >= end, or 0 < mid < end, then use mid */
        if (c01 != c12)
        {
            /* otherwise, we'll need to swap something else to middle */
            int z;
            c = STBTT__COMPARE(&p[0], &p[n - 1]);
            /* 0>mid && mid<n:  0>n => n; 0<n => 0 */
            /* 0<mid && mid>n:  0>n => 0; 0<n => n */
            z    = (c == c12) ? 0 : n - 1;
            t    = p[z];
            p[z] = p[m];
            p[m] = t;
        }
        /* now p[m] is the median-of-three */
        /* swap it to the beginning so it won't move around */
        t    = p[0];
        p[0] = p[m];
        p[m] = t;

        /* partition loop */
        i = 1;
        j = n - 1;
        for (;;)
        {
            /* handling of equality is crucial here */
            /* for sentinels & efficiency with duplicates */
            for (;; ++i)
            {
                if (!STBTT__COMPARE(&p[i], &p[0]))
                    break;
            }
            for (;; --j)
            {
                if (!STBTT__COMPARE(&p[0], &p[j]))
                    break;
            }
            /* make sure we haven't crossed */
            if (i >= j)
                break;
            t    = p[i];
            p[i] = p[j];
            p[j] = t;

            ++i;
            --j;
        }
        /* recurse on smaller side, iterate on larger */
        if (j < (n - i))
        {
            stbtt__sort_edges_quicksort(p, j);
            p = p + i;
            n = n - i;
        }
        else
        {
            stbtt__sort_edges_quicksort(p + i, n - i);
            n = j;
        }
    }
}

static void stbtt__sort_edges(stbtt__edge *p, int n)
{
    stbtt__sort_edges_quicksort(p, n);
    stbtt__sort_edges_ins_sort(p, n);
}

typedef struct
{
    float x, y;
} stbtt__point;

static void stbtt__rasterize(stbtt__bitmap *result, stbtt__point *pts, int *wcount, int windings, float scale_x,
                             float scale_y, float shift_x, float shift_y, int off_x, int off_y, int invert,
                             void *userdata)
{
    float        y_scale_inv = invert ? -scale_y : scale_y;
    stbtt__edge *e;
    int          n, i, j, k, m;
#if STBTT_RASTERIZER_VERSION == 1
    int vsubsample = result->h < 8 ? 15 : 5;
#elif STBTT_RASTERIZER_VERSION == 2
    int vsubsample = 1;
#else
#error "Unrecognized value of STBTT_RASTERIZER_VERSION"
#endif
    // vsubsample should divide 255 evenly; otherwise we won't reach full opacity

    // now we have to blow out the windings into explicit edge lists
    n = 0;
    for (i = 0; i < windings; ++i)
        n += wcount[i];

    e = (stbtt__edge *)STBTT_malloc(sizeof(*e) * (n + 1), userdata); // add an extra one as a sentinel
    if (e == 0)
        return;
    n = 0;

    m = 0;
    for (i = 0; i < windings; ++i)
    {
        stbtt__point *p = pts + m;
        m += wcount[i];
        j = wcount[i] - 1;
        for (k = 0; k < wcount[i]; j = k++)
        {
            int a = k, b = j;
            // skip the edge if horizontal
            if (p[j].y == p[k].y)
                continue;
            // add edge from j to k to the list
            e[n].invert = 0;
            if (invert ? p[j].y > p[k].y : p[j].y < p[k].y)
            {
                e[n].invert = 1;
                a = j, b = k;
            }
            e[n].x0 = p[a].x * scale_x + shift_x;
            e[n].y0 = (p[a].y * y_scale_inv + shift_y) * vsubsample;
            e[n].x1 = p[b].x * scale_x + shift_x;
            e[n].y1 = (p[b].y * y_scale_inv + shift_y) * vsubsample;
            ++n;
        }
    }

    // now sort the edges by their highest point (should snap to integer, and then by x)
    // STBTT_sort(e, n, sizeof(e[0]), stbtt__edge_compare);
    stbtt__sort_edges(e, n);

    // now, traverse the scanlines and find the intersections on each scanline, use xor winding rule
    stbtt__rasterize_sorted_edges(result, e, n, vsubsample, off_x, off_y, userdata);

    STBTT_free(e, userdata);
}

static void stbtt__add_point(stbtt__point *points, int n, float x, float y)
{
    if (!points)
        return; // during first pass, it's unallocated
    points[n].x = x;
    points[n].y = y;
}

// tessellate until threshold p is happy... @TODO warped to compensate for non-linear stretching
static int stbtt__tesselate_curve(stbtt__point *points, int *num_points, float x0, float y0, float x1, float y1,
                                  float x2, float y2, float objspace_flatness_squared, int n)
{
    // midpoint
    float mx = (x0 + 2 * x1 + x2) / 4;
    float my = (y0 + 2 * y1 + y2) / 4;
    // versus directly drawn line
    float dx = (x0 + x2) / 2 - mx;
    float dy = (y0 + y2) / 2 - my;
    if (n > 16) // 65536 segments on one curve better be enough!
        return 1;
    if (dx * dx + dy * dy > objspace_flatness_squared)
    { // half-pixel error allowed... need to be smaller if AA
        stbtt__tesselate_curve(points, num_points, x0, y0, (x0 + x1) / 2.0f, (y0 + y1) / 2.0f, mx, my,
                               objspace_flatness_squared, n + 1);
        stbtt__tesselate_curve(points, num_points, mx, my, (x1 + x2) / 2.0f, (y1 + y2) / 2.0f, x2, y2,
                               objspace_flatness_squared, n + 1);
    }
    else
    {
        stbtt__add_point(points, *num_points, x2, y2);
        *num_points = *num_points + 1;
    }
    return 1;
}

static void stbtt__tesselate_cubic(stbtt__point *points, int *num_points, float x0, float y0, float x1, float y1,
                                   float x2, float y2, float x3, float y3, float objspace_flatness_squared, int n)
{
    // @TODO this "flatness" calculation is just made-up nonsense that seems to work well enough
    float dx0              = x1 - x0;
    float dy0              = y1 - y0;
    float dx1              = x2 - x1;
    float dy1              = y2 - y1;
    float dx2              = x3 - x2;
    float dy2              = y3 - y2;
    float dx               = x3 - x0;
    float dy               = y3 - y0;
    float longlen          = (float)(STBTT_sqrt(dx0 * dx0 + dy0 * dy0) + STBTT_sqrt(dx1 * dx1 + dy1 * dy1) +
                            STBTT_sqrt(dx2 * dx2 + dy2 * dy2));
    float shortlen         = (float)STBTT_sqrt(dx * dx + dy * dy);
    float flatness_squared = longlen * longlen - shortlen * shortlen;

    if (n > 16) // 65536 segments on one curve better be enough!
        return;

    if (flatness_squared > objspace_flatness_squared)
    {
        float x01 = (x0 + x1) / 2;
        float y01 = (y0 + y1) / 2;
        float x12 = (x1 + x2) / 2;
        float y12 = (y1 + y2) / 2;
        float x23 = (x2 + x3) / 2;
        float y23 = (y2 + y3) / 2;

        float xa  = (x01 + x12) / 2;
        float ya  = (y01 + y12) / 2;
        float xb  = (x12 + x23) / 2;
        float yb  = (y12 + y23) / 2;

        float mx  = (xa + xb) / 2;
        float my  = (ya + yb) / 2;

        stbtt__tesselate_cubic(points, num_points, x0, y0, x01, y01, xa, ya, mx, my, objspace_flatness_squared, n + 1);
        stbtt__tesselate_cubic(points, num_points, mx, my, xb, yb, x23, y23, x3, y3, objspace_flatness_squared, n + 1);
    }
    else
    {
        stbtt__add_point(points, *num_points, x3, y3);
        *num_points = *num_points + 1;
    }
}

// returns number of contours
static stbtt__point *stbtt_FlattenCurves(stbtt_vertex *vertices, int num_verts, float objspace_flatness,
                                         int **contour_lengths, int *num_contours, void *userdata)
{
    stbtt__point *points                    = 0;
    int           num_points                = 0;

    float         objspace_flatness_squared = objspace_flatness * objspace_flatness;
    int           i, n = 0, start = 0, pass;

    // count how many "moves" there are to get the contour count
    for (i = 0; i < num_verts; ++i)
        if (vertices[i].type == STBTT_vmove)
            ++n;

    *num_contours = n;
    if (n == 0)
        return 0;

    *contour_lengths = (int *)STBTT_malloc(sizeof(**contour_lengths) * n, userdata);

    if (*contour_lengths == 0)
    {
        *num_contours = 0;
        return 0;
    }

    // make two passes through the points so we don't need to realloc
    for (pass = 0; pass < 2; ++pass)
    {
        float x = 0, y = 0;
        if (pass == 1)
        {
            points = (stbtt__point *)STBTT_malloc(num_points * sizeof(points[0]), userdata);
            if (points == NULL)
                goto error;
        }
        num_points = 0;
        n          = -1;
        for (i = 0; i < num_verts; ++i)
        {
            switch (vertices[i].type)
            {
            case STBTT_vmove:
                // start the next contour
                if (n >= 0)
                    (*contour_lengths)[n] = num_points - start;
                ++n;
                start = num_points;

                x = vertices[i].x, y = vertices[i].y;
                stbtt__add_point(points, num_points++, x, y);
                break;
            case STBTT_vline:
                x = vertices[i].x, y = vertices[i].y;
                stbtt__add_point(points, num_points++, x, y);
                break;
            case STBTT_vcurve:
                stbtt__tesselate_curve(points, &num_points, x, y, vertices[i].cx, vertices[i].cy, vertices[i].x,
                                       vertices[i].y, objspace_flatness_squared, 0);
                x = vertices[i].x, y = vertices[i].y;
                break;
            case STBTT_vcubic:
                stbtt__tesselate_cubic(points, &num_points, x, y, vertices[i].cx, vertices[i].cy, vertices[i].cx1,
                                       vertices[i].cy1, vertices[i].x, vertices[i].y, objspace_flatness_squared, 0);
                x = vertices[i].x, y = vertices[i].y;
                break;
            }
        }
        (*contour_lengths)[n] = num_points - start;
    }

    return points;
error:
    STBTT_free(points, userdata);
    STBTT_free(*contour_lengths, userdata);
    *contour_lengths = 0;
    *num_contours    = 0;
    return NULL;
}

STBTT_DEF void stbtt_Rasterize(stbtt__bitmap *result, float flatness_in_pixels, stbtt_vertex *vertices, int num_verts,
                               float scale_x, float scale_y, float shift_x, float shift_y, int x_off, int y_off,
                               int invert, void *userdata)
{
    float         scale           = scale_x > scale_y ? scale_y : scale_x;
    int           winding_count   = 0;
    int          *winding_lengths = NULL;
    stbtt__point *windings = stbtt_FlattenCurves(vertices, num_verts, flatness_in_pixels / scale, &winding_lengths,
                                                 &winding_count, userdata);
    if (windings)
    {
        stbtt__rasterize(result, windings, winding_lengths, winding_count, scale_x, scale_y, shift_x, shift_y, x_off,
                         y_off, invert, userdata);
        STBTT_free(winding_lengths, userdata);
        STBTT_free(windings, userdata);
    }
}

STBTT_DEF void stbtt_FreeBitmap(unsigned char *bitmap, void *userdata)
{
    STBTT_free(bitmap, userdata);
}

STBTT_DEF unsigned char *stbtt_GetGlyphBitmapSubpixel(const stbtt_fontinfo *info, float scale_x, float scale_y,
                                                      float shift_x, float shift_y, int glyph, int *width, int *height,
                                                      int *xoff, int *yoff)
{
    int           ix0, iy0, ix1, iy1;
    stbtt__bitmap gbm;
    stbtt_vertex *vertices;
    int           num_verts = stbtt_GetGlyphShape(info, glyph, &vertices);

    if (scale_x == 0)
        scale_x = scale_y;
    if (scale_y == 0)
    {
        if (scale_x == 0)
        {
            STBTT_free(vertices, info->userdata);
            return NULL;
        }
        scale_y = scale_x;
    }

    stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale_x, scale_y, shift_x, shift_y, &ix0, &iy0, &ix1, &iy1);

    // now we get the size
    gbm.w      = (ix1 - ix0);
    gbm.h      = (iy1 - iy0);
    gbm.pixels = NULL; // in case we error

    if (width)
        *width = gbm.w;
    if (height)
        *height = gbm.h;
    if (xoff)
        *xoff = ix0;
    if (yoff)
        *yoff = iy0;

    if (gbm.w && gbm.h)
    {
        gbm.pixels = (unsigned char *)STBTT_malloc(gbm.w * gbm.h, info->userdata);
        if (gbm.pixels)
        {
            gbm.stride = gbm.w;

            stbtt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale_x, scale_y, shift_x, shift_y, ix0, iy0, 1,
                            info->userdata);
        }
    }
    STBTT_free(vertices, info->userdata);
    return gbm.pixels;
}

STBTT_DEF unsigned char *stbtt_GetGlyphBitmap(const stbtt_fontinfo *info, float scale_x, float scale_y, int glyph,
                                              int *width, int *height, int *xoff, int *yoff)
{
    return stbtt_GetGlyphBitmapSubpixel(info, scale_x, scale_y, 0.0f, 0.0f, glyph, width, height, xoff, yoff);
}

STBTT_DEF void stbtt_MakeGlyphBitmapSubpixel(const stbtt_fontinfo *info, unsigned char *output, int out_w, int out_h,
                                             int out_stride, float scale_x, float scale_y, float shift_x, float shift_y,
                                             int glyph)
{
    int           ix0, iy0;
    stbtt_vertex *vertices;
    int           num_verts = stbtt_GetGlyphShape(info, glyph, &vertices);
    stbtt__bitmap gbm;

    stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale_x, scale_y, shift_x, shift_y, &ix0, &iy0, 0, 0);
    gbm.pixels = output;
    gbm.w      = out_w;
    gbm.h      = out_h;
    gbm.stride = out_stride;

    if (gbm.w && gbm.h)
        stbtt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale_x, scale_y, shift_x, shift_y, ix0, iy0, 1,
                        info->userdata);

    STBTT_free(vertices, info->userdata);
}

STBTT_DEF void stbtt_MakeGlyphBitmap(const stbtt_fontinfo *info, unsigned char *output, int out_w, int out_h,
                                     int out_stride, float scale_x, float scale_y, int glyph)
{
    stbtt_MakeGlyphBitmapSubpixel(info, output, out_w, out_h, out_stride, scale_x, scale_y, 0.0f, 0.0f, glyph);
}

STBTT_DEF unsigned char *stbtt_GetCodepointBitmapSubpixel(const stbtt_fontinfo *info, float scale_x, float scale_y,
                                                          float shift_x, float shift_y, int codepoint, int *width,
                                                          int *height, int *xoff, int *yoff)
{
    return stbtt_GetGlyphBitmapSubpixel(info, scale_x, scale_y, shift_x, shift_y, stbtt_FindGlyphIndex(info, codepoint),
                                        width, height, xoff, yoff);
}

STBTT_DEF void stbtt_MakeCodepointBitmapSubpixelPrefilter(const stbtt_fontinfo *info, unsigned char *output, int out_w,
                                                          int out_h, int out_stride, float scale_x, float scale_y,
                                                          float shift_x, float shift_y, int oversample_x,
                                                          int oversample_y, float *sub_x, float *sub_y, int codepoint)
{
    stbtt_MakeGlyphBitmapSubpixelPrefilter(info, output, out_w, out_h, out_stride, scale_x, scale_y, shift_x, shift_y,
                                           oversample_x, oversample_y, sub_x, sub_y,
                                           stbtt_FindGlyphIndex(info, codepoint));
}

STBTT_DEF void stbtt_MakeCodepointBitmapSubpixel(const stbtt_fontinfo *info, unsigned char *output, int out_w,
                                                 int out_h, int out_stride, float scale_x, float scale_y, float shift_x,
                                                 float shift_y, int codepoint)
{
    stbtt_MakeGlyphBitmapSubpixel(info, output, out_w, out_h, out_stride, scale_x, scale_y, shift_x, shift_y,
                                  stbtt_FindGlyphIndex(info, codepoint));
}

STBTT_DEF unsigned char *stbtt_GetCodepointBitmap(const stbtt_fontinfo *info, float scale_x, float scale_y,
                                                  int codepoint, int *width, int *height, int *xoff, int *yoff)
{
    return stbtt_GetCodepointBitmapSubpixel(info, scale_x, scale_y, 0.0f, 0.0f, codepoint, width, height, xoff, yoff);
}

STBTT_DEF void stbtt_MakeCodepointBitmap(const stbtt_fontinfo *info, unsigned char *output, int out_w, int out_h,
                                         int out_stride, float scale_x, float scale_y, int codepoint)
{
    stbtt_MakeCodepointBitmapSubpixel(info, output, out_w, out_h, out_stride, scale_x, scale_y, 0.0f, 0.0f, codepoint);
}

//////////////////////////////////////////////////////////////////////////////
//
// bitmap baking
//
// This is SUPER-CRAPPY packing to keep source code small

static int stbtt_BakeFontBitmap_internal(unsigned char *data,
                                         int            offset,       // font location (use offset=0 for plain .ttf)
                                         float          pixel_height, // height of font in pixels
                                         unsigned char *pixels, int pw, int ph, // bitmap to be filled in
                                         int first_char, int num_chars,         // characters to bake
                                         stbtt_bakedchar *chardata)
{
    float          scale;
    int            x, y, bottom_y, i;
    stbtt_fontinfo f;
    f.userdata = NULL;
    if (!stbtt_InitFont(&f, data, offset))
        return -1;
    STBTT_memset(pixels, 0, pw * ph); // background of 0 around pixels
    x = y    = 1;
    bottom_y = 1;

    scale    = stbtt_ScaleForPixelHeight(&f, pixel_height);

    for (i = 0; i < num_chars; ++i)
    {
        int advance, lsb, x0, y0, x1, y1, gw, gh;
        int g = stbtt_FindGlyphIndex(&f, first_char + i);
        stbtt_GetGlyphHMetrics(&f, g, &advance, &lsb);
        stbtt_GetGlyphBitmapBox(&f, g, scale, scale, &x0, &y0, &x1, &y1);
        gw = x1 - x0;
        gh = y1 - y0;
        if (x + gw + 1 >= pw)
            y = bottom_y, x = 1; // advance to next row
        if (y + gh + 1 >= ph)    // check if it fits vertically AFTER potentially moving to next row
            return -i;
        STBTT_assert(x + gw < pw);
        STBTT_assert(y + gh < ph);
        stbtt_MakeGlyphBitmap(&f, pixels + x + y * pw, gw, gh, pw, scale, scale, g);
        chardata[i].x0       = (stbtt_int16)x;
        chardata[i].y0       = (stbtt_int16)y;
        chardata[i].x1       = (stbtt_int16)(x + gw);
        chardata[i].y1       = (stbtt_int16)(y + gh);
        chardata[i].xadvance = scale * advance;
        chardata[i].xoff     = (float)x0;
        chardata[i].yoff     = (float)y0;
        x                    = x + gw + 1;
        if (y + gh + 1 > bottom_y)
            bottom_y = y + gh + 1;
    }
    return bottom_y;
}

STBTT_DEF void stbtt_GetBakedQuad(const stbtt_bakedchar *chardata, int pw, int ph, int char_index, float *xpos,
                                  float *ypos, stbtt_aligned_quad *q, int opengl_fillrule)
{
    float                  d3d_bias = opengl_fillrule ? 0 : -0.5f;
    float                  ipw = 1.0f / pw, iph = 1.0f / ph;
    const stbtt_bakedchar *b       = chardata + char_index;
    int                    round_x = STBTT_ifloor((*xpos + b->xoff) + 0.5f);
    int                    round_y = STBTT_ifloor((*ypos + b->yoff) + 0.5f);

    q->x0                          = round_x + d3d_bias;
    q->y0                          = round_y + d3d_bias;
    q->x1                          = round_x + b->x1 - b->x0 + d3d_bias;
    q->y1                          = round_y + b->y1 - b->y0 + d3d_bias;

    q->s0                          = b->x0 * ipw;
    q->t0                          = b->y0 * iph;
    q->s1                          = b->x1 * ipw;
    q->t1                          = b->y1 * iph;

    *xpos += b->xadvance;
}

//////////////////////////////////////////////////////////////////////////////
//
// rectangle packing replacement routines if you don't have stb_rect_pack.h
//

#ifndef STB_RECT_PACK_VERSION

typedef int stbrp_coord;

////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                                                                                //
// COMPILER WARNING ?!?!?                                                         //
//                                                                                //
//                                                                                //
// if you get a compile warning due to these symbols being defined more than      //
// once, move #include "stb_rect_pack.h" before #include "stb_truetype.h"         //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    int width, height;
    int x, y, bottom_y;
} stbrp_context;

typedef struct
{
    unsigned char x;
} stbrp_node;

struct stbrp_rect
{
    stbrp_coord x, y;
    int         id, w, h, was_packed;
};

static void stbrp_init_target(stbrp_context *con, int pw, int ph, stbrp_node *nodes, int num_nodes)
{
    con->width    = pw;
    con->height   = ph;
    con->x        = 0;
    con->y        = 0;
    con->bottom_y = 0;
    STBTT__NOTUSED(nodes);
    STBTT__NOTUSED(num_nodes);
}

static void stbrp_pack_rects(stbrp_context *con, stbrp_rect *rects, int num_rects)
{
    int i;
    for (i = 0; i < num_rects; ++i)
    {
        if (con->x + rects[i].w > con->width)
        {
            con->x = 0;
            con->y = con->bottom_y;
        }
        if (con->y + rects[i].h > con->height)
            break;
        rects[i].x          = con->x;
        rects[i].y          = con->y;
        rects[i].was_packed = 1;
        con->x += rects[i].w;
        if (con->y + rects[i].h > con->bottom_y)
            con->bottom_y = con->y + rects[i].h;
    }
    for (; i < num_rects; ++i)
        rects[i].was_packed = 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// bitmap baking
//
// This is SUPER-AWESOME (tm Ryan Gordon) packing using stb_rect_pack.h. If
// stb_rect_pack.h isn't available, it uses the BakeFontBitmap strategy.

STBTT_DEF int stbtt_PackBegin(stbtt_pack_context *spc, unsigned char *pixels, int pw, int ph, int stride_in_bytes,
                              int padding, void *alloc_context)
{
    stbrp_context *context   = (stbrp_context *)STBTT_malloc(sizeof(*context), alloc_context);
    int            num_nodes = pw - padding;
    stbrp_node    *nodes     = (stbrp_node *)STBTT_malloc(sizeof(*nodes) * num_nodes, alloc_context);

    if (context == NULL || nodes == NULL)
    {
        if (context != NULL)
            STBTT_free(context, alloc_context);
        if (nodes != NULL)
            STBTT_free(nodes, alloc_context);
        return 0;
    }

    spc->user_allocator_context = alloc_context;
    spc->width                  = pw;
    spc->height                 = ph;
    spc->pixels                 = pixels;
    spc->pack_info              = context;
    spc->nodes                  = nodes;
    spc->padding                = padding;
    spc->stride_in_bytes        = stride_in_bytes != 0 ? stride_in_bytes : pw;
    spc->h_oversample           = 1;
    spc->v_oversample           = 1;
    spc->skip_missing           = 0;

    stbrp_init_target(context, pw - padding, ph - padding, nodes, num_nodes);

    if (pixels)
        STBTT_memset(pixels, 0, pw * ph); // background of 0 around pixels

    return 1;
}

STBTT_DEF void stbtt_PackEnd(stbtt_pack_context *spc)
{
    STBTT_free(spc->nodes, spc->user_allocator_context);
    STBTT_free(spc->pack_info, spc->user_allocator_context);
}

STBTT_DEF void stbtt_PackSetOversampling(stbtt_pack_context *spc, unsigned int h_oversample, unsigned int v_oversample)
{
    STBTT_assert(h_oversample <= STBTT_MAX_OVERSAMPLE);
    STBTT_assert(v_oversample <= STBTT_MAX_OVERSAMPLE);
    if (h_oversample <= STBTT_MAX_OVERSAMPLE)
        spc->h_oversample = h_oversample;
    if (v_oversample <= STBTT_MAX_OVERSAMPLE)
        spc->v_oversample = v_oversample;
}

STBTT_DEF void stbtt_PackSetSkipMissingCodepoints(stbtt_pack_context *spc, int skip)
{
    spc->skip_missing = skip;
}

#define STBTT__OVER_MASK (STBTT_MAX_OVERSAMPLE - 1)

static void stbtt__h_prefilter(unsigned char *pixels, int w, int h, int stride_in_bytes, unsigned int kernel_width)
{
    unsigned char buffer[STBTT_MAX_OVERSAMPLE];
    int           safe_w = w - kernel_width;
    int           j;
    STBTT_memset(buffer, 0, STBTT_MAX_OVERSAMPLE); // suppress bogus warning from VS2013 -analyze
    for (j = 0; j < h; ++j)
    {
        int          i;
        unsigned int total;
        STBTT_memset(buffer, 0, kernel_width);

        total = 0;

        // make kernel_width a constant in common cases so compiler can optimize out the divide
        switch (kernel_width)
        {
        case 2:
            for (i = 0; i <= safe_w; ++i)
            {
                total += pixels[i] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i];
                pixels[i]                                     = (unsigned char)(total / 2);
            }
            break;
        case 3:
            for (i = 0; i <= safe_w; ++i)
            {
                total += pixels[i] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i];
                pixels[i]                                     = (unsigned char)(total / 3);
            }
            break;
        case 4:
            for (i = 0; i <= safe_w; ++i)
            {
                total += pixels[i] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i];
                pixels[i]                                     = (unsigned char)(total / 4);
            }
            break;
        case 5:
            for (i = 0; i <= safe_w; ++i)
            {
                total += pixels[i] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i];
                pixels[i]                                     = (unsigned char)(total / 5);
            }
            break;
        default:
            for (i = 0; i <= safe_w; ++i)
            {
                total += pixels[i] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i];
                pixels[i]                                     = (unsigned char)(total / kernel_width);
            }
            break;
        }

        for (; i < w; ++i)
        {
            STBTT_assert(pixels[i] == 0);
            total -= buffer[i & STBTT__OVER_MASK];
            pixels[i] = (unsigned char)(total / kernel_width);
        }

        pixels += stride_in_bytes;
    }
}

static void stbtt__v_prefilter(unsigned char *pixels, int w, int h, int stride_in_bytes, unsigned int kernel_width)
{
    unsigned char buffer[STBTT_MAX_OVERSAMPLE];
    int           safe_h = h - kernel_width;
    int           j;
    STBTT_memset(buffer, 0, STBTT_MAX_OVERSAMPLE); // suppress bogus warning from VS2013 -analyze
    for (j = 0; j < w; ++j)
    {
        int          i;
        unsigned int total;
        STBTT_memset(buffer, 0, kernel_width);

        total = 0;

        // make kernel_width a constant in common cases so compiler can optimize out the divide
        switch (kernel_width)
        {
        case 2:
            for (i = 0; i <= safe_h; ++i)
            {
                total += pixels[i * stride_in_bytes] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i * stride_in_bytes];
                pixels[i * stride_in_bytes]                   = (unsigned char)(total / 2);
            }
            break;
        case 3:
            for (i = 0; i <= safe_h; ++i)
            {
                total += pixels[i * stride_in_bytes] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i * stride_in_bytes];
                pixels[i * stride_in_bytes]                   = (unsigned char)(total / 3);
            }
            break;
        case 4:
            for (i = 0; i <= safe_h; ++i)
            {
                total += pixels[i * stride_in_bytes] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i * stride_in_bytes];
                pixels[i * stride_in_bytes]                   = (unsigned char)(total / 4);
            }
            break;
        case 5:
            for (i = 0; i <= safe_h; ++i)
            {
                total += pixels[i * stride_in_bytes] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i * stride_in_bytes];
                pixels[i * stride_in_bytes]                   = (unsigned char)(total / 5);
            }
            break;
        default:
            for (i = 0; i <= safe_h; ++i)
            {
                total += pixels[i * stride_in_bytes] - buffer[i & STBTT__OVER_MASK];
                buffer[(i + kernel_width) & STBTT__OVER_MASK] = pixels[i * stride_in_bytes];
                pixels[i * stride_in_bytes]                   = (unsigned char)(total / kernel_width);
            }
            break;
        }

        for (; i < h; ++i)
        {
            STBTT_assert(pixels[i * stride_in_bytes] == 0);
            total -= buffer[i & STBTT__OVER_MASK];
            pixels[i * stride_in_bytes] = (unsigned char)(total / kernel_width);
        }

        pixels += 1;
    }
}

static float stbtt__oversample_shift(int oversample)
{
    if (!oversample)
        return 0.0f;

    // The prefilter is a box filter of width "oversample",
    // which shifts phase by (oversample - 1)/2 pixels in
    // oversampled space. We want to shift in the opposite
    // direction to counter this.
    return (float)-(oversample - 1) / (2.0f * (float)oversample);
}

// rects array must be big enough to accommodate all characters in the given ranges
STBTT_DEF int stbtt_PackFontRangesGatherRects(stbtt_pack_context *spc, const stbtt_fontinfo *info,
                                              stbtt_pack_range *ranges, int num_ranges, stbrp_rect *rects)
{
    int i, j, k;
    int missing_glyph_added = 0;

    k                       = 0;
    for (i = 0; i < num_ranges; ++i)
    {
        float fh    = ranges[i].font_size;
        float scale = fh > 0 ? stbtt_ScaleForPixelHeight(info, fh) : stbtt_ScaleForMappingEmToPixels(info, -fh);
        ranges[i].h_oversample = (unsigned char)spc->h_oversample;
        ranges[i].v_oversample = (unsigned char)spc->v_oversample;
        for (j = 0; j < ranges[i].num_chars; ++j)
        {
            int x0, y0, x1, y1;
            int codepoint = ranges[i].array_of_unicode_codepoints == NULL
                                ? ranges[i].first_unicode_codepoint_in_range + j
                                : ranges[i].array_of_unicode_codepoints[j];
            int glyph     = stbtt_FindGlyphIndex(info, codepoint);
            if (glyph == 0 && (spc->skip_missing || missing_glyph_added))
            {
                rects[k].w = rects[k].h = 0;
            }
            else
            {
                stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale * spc->h_oversample, scale * spc->v_oversample, 0, 0,
                                                &x0, &y0, &x1, &y1);
                rects[k].w = (stbrp_coord)(x1 - x0 + spc->padding + spc->h_oversample - 1);
                rects[k].h = (stbrp_coord)(y1 - y0 + spc->padding + spc->v_oversample - 1);
                if (glyph == 0)
                    missing_glyph_added = 1;
            }
            ++k;
        }
    }

    return k;
}

STBTT_DEF void stbtt_MakeGlyphBitmapSubpixelPrefilter(const stbtt_fontinfo *info, unsigned char *output, int out_w,
                                                      int out_h, int out_stride, float scale_x, float scale_y,
                                                      float shift_x, float shift_y, int prefilter_x, int prefilter_y,
                                                      float *sub_x, float *sub_y, int glyph)
{
    stbtt_MakeGlyphBitmapSubpixel(info, output, out_w - (prefilter_x - 1), out_h - (prefilter_y - 1), out_stride,
                                  scale_x, scale_y, shift_x, shift_y, glyph);

    if (prefilter_x > 1)
        stbtt__h_prefilter(output, out_w, out_h, out_stride, prefilter_x);

    if (prefilter_y > 1)
        stbtt__v_prefilter(output, out_w, out_h, out_stride, prefilter_y);

    *sub_x = stbtt__oversample_shift(prefilter_x);
    *sub_y = stbtt__oversample_shift(prefilter_y);
}

// rects array must be big enough to accommodate all characters in the given ranges
STBTT_DEF int stbtt_PackFontRangesRenderIntoRects(stbtt_pack_context *spc, const stbtt_fontinfo *info,
                                                  stbtt_pack_range *ranges, int num_ranges, stbrp_rect *rects)
{
    int i, j, k, missing_glyph = -1, return_value = 1;

    // save current values
    int old_h_over = spc->h_oversample;
    int old_v_over = spc->v_oversample;

    k              = 0;
    for (i = 0; i < num_ranges; ++i)
    {
        float fh    = ranges[i].font_size;
        float scale = fh > 0 ? stbtt_ScaleForPixelHeight(info, fh) : stbtt_ScaleForMappingEmToPixels(info, -fh);
        float recip_h, recip_v, sub_x, sub_y;
        spc->h_oversample = ranges[i].h_oversample;
        spc->v_oversample = ranges[i].v_oversample;
        recip_h           = 1.0f / spc->h_oversample;
        recip_v           = 1.0f / spc->v_oversample;
        sub_x             = stbtt__oversample_shift(spc->h_oversample);
        sub_y             = stbtt__oversample_shift(spc->v_oversample);
        for (j = 0; j < ranges[i].num_chars; ++j)
        {
            stbrp_rect *r = &rects[k];
            if (r->was_packed && r->w != 0 && r->h != 0)
            {
                stbtt_packedchar *bc = &ranges[i].chardata_for_range[j];
                int               advance, lsb, x0, y0, x1, y1;
                int               codepoint = ranges[i].array_of_unicode_codepoints == NULL
                                                  ? ranges[i].first_unicode_codepoint_in_range + j
                                                  : ranges[i].array_of_unicode_codepoints[j];
                int               glyph     = stbtt_FindGlyphIndex(info, codepoint);
                stbrp_coord       pad       = (stbrp_coord)spc->padding;

                // pad on left and top
                r->x += pad;
                r->y += pad;
                r->w -= pad;
                r->h -= pad;
                stbtt_GetGlyphHMetrics(info, glyph, &advance, &lsb);
                stbtt_GetGlyphBitmapBox(info, glyph, scale * spc->h_oversample, scale * spc->v_oversample, &x0, &y0,
                                        &x1, &y1);
                stbtt_MakeGlyphBitmapSubpixel(info, spc->pixels + r->x + r->y * spc->stride_in_bytes,
                                              r->w - spc->h_oversample + 1, r->h - spc->v_oversample + 1,
                                              spc->stride_in_bytes, scale * spc->h_oversample,
                                              scale * spc->v_oversample, 0, 0, glyph);

                if (spc->h_oversample > 1)
                    stbtt__h_prefilter(spc->pixels + r->x + r->y * spc->stride_in_bytes, r->w, r->h,
                                       spc->stride_in_bytes, spc->h_oversample);

                if (spc->v_oversample > 1)
                    stbtt__v_prefilter(spc->pixels + r->x + r->y * spc->stride_in_bytes, r->w, r->h,
                                       spc->stride_in_bytes, spc->v_oversample);

                bc->x0       = (stbtt_int16)r->x;
                bc->y0       = (stbtt_int16)r->y;
                bc->x1       = (stbtt_int16)(r->x + r->w);
                bc->y1       = (stbtt_int16)(r->y + r->h);
                bc->xadvance = scale * advance;
                bc->xoff     = (float)x0 * recip_h + sub_x;
                bc->yoff     = (float)y0 * recip_v + sub_y;
                bc->xoff2    = (x0 + r->w) * recip_h + sub_x;
                bc->yoff2    = (y0 + r->h) * recip_v + sub_y;

                if (glyph == 0)
                    missing_glyph = j;
            }
            else if (spc->skip_missing)
            {
                return_value = 0;
            }
            else if (r->was_packed && r->w == 0 && r->h == 0 && missing_glyph >= 0)
            {
                ranges[i].chardata_for_range[j] = ranges[i].chardata_for_range[missing_glyph];
            }
            else
            {
                return_value = 0; // if any fail, report failure
            }

            ++k;
        }
    }

    // restore original values
    spc->h_oversample = old_h_over;
    spc->v_oversample = old_v_over;

    return return_value;
}

STBTT_DEF void stbtt_PackFontRangesPackRects(stbtt_pack_context *spc, stbrp_rect *rects, int num_rects)
{
    stbrp_pack_rects((stbrp_context *)spc->pack_info, rects, num_rects);
}

STBTT_DEF int stbtt_PackFontRanges(stbtt_pack_context *spc, const unsigned char *fontdata, int font_index,
                                   stbtt_pack_range *ranges, int num_ranges)
{
    stbtt_fontinfo info;
    int            i, j, n, return_value = 1;
    // stbrp_context *context = (stbrp_context *) spc->pack_info;
    stbrp_rect *rects;

    // flag all characters as NOT packed
    for (i = 0; i < num_ranges; ++i)
        for (j = 0; j < ranges[i].num_chars; ++j)
            ranges[i].chardata_for_range[j].x0     = ranges[i].chardata_for_range[j].y0 =
                ranges[i].chardata_for_range[j].x1 = ranges[i].chardata_for_range[j].y1 = 0;

    n = 0;
    for (i = 0; i < num_ranges; ++i)
        n += ranges[i].num_chars;

    rects = (stbrp_rect *)STBTT_malloc(sizeof(*rects) * n, spc->user_allocator_context);
    if (rects == NULL)
        return 0;

    info.userdata = spc->user_allocator_context;
    stbtt_InitFont(&info, fontdata, stbtt_GetFontOffsetForIndex(fontdata, font_index));

    n = stbtt_PackFontRangesGatherRects(spc, &info, ranges, num_ranges, rects);

    stbtt_PackFontRangesPackRects(spc, rects, n);

    return_value = stbtt_PackFontRangesRenderIntoRects(spc, &info, ranges, num_ranges, rects);

    STBTT_free(rects, spc->user_allocator_context);
    return return_value;
}

STBTT_DEF int stbtt_PackFontRange(stbtt_pack_context *spc, const unsigned char *fontdata, int font_index,
                                  float font_size, int first_unicode_codepoint_in_range, int num_chars_in_range,
                                  stbtt_packedchar *chardata_for_range)
{
    stbtt_pack_range range;
    range.first_unicode_codepoint_in_range = first_unicode_codepoint_in_range;
    range.array_of_unicode_codepoints      = NULL;
    range.num_chars                        = num_chars_in_range;
    range.chardata_for_range               = chardata_for_range;
    range.font_size                        = font_size;
    return stbtt_PackFontRanges(spc, fontdata, font_index, &range, 1);
}

STBTT_DEF void stbtt_GetScaledFontVMetrics(const unsigned char *fontdata, int index, float size, float *ascent,
                                           float *descent, float *lineGap)
{
    int            i_ascent, i_descent, i_lineGap;
    float          scale;
    stbtt_fontinfo info;
    stbtt_InitFont(&info, fontdata, stbtt_GetFontOffsetForIndex(fontdata, index));
    scale = size > 0 ? stbtt_ScaleForPixelHeight(&info, size) : stbtt_ScaleForMappingEmToPixels(&info, -size);
    stbtt_GetFontVMetrics(&info, &i_ascent, &i_descent, &i_lineGap);
    *ascent  = (float)i_ascent * scale;
    *descent = (float)i_descent * scale;
    *lineGap = (float)i_lineGap * scale;
}

STBTT_DEF void stbtt_GetPackedQuad(const stbtt_packedchar *chardata, int pw, int ph, int char_index, float *xpos,
                                   float *ypos, stbtt_aligned_quad *q, int align_to_integer)
{
    float                   ipw = 1.0f / pw, iph = 1.0f / ph;
    const stbtt_packedchar *b = chardata + char_index;

    if (align_to_integer)
    {
        float x = (float)STBTT_ifloor((*xpos + b->xoff) + 0.5f);
        float y = (float)STBTT_ifloor((*ypos + b->yoff) + 0.5f);
        q->x0   = x;
        q->y0   = y;
        q->x1   = x + b->xoff2 - b->xoff;
        q->y1   = y + b->yoff2 - b->yoff;
    }
    else
    {
        q->x0 = *xpos + b->xoff;
        q->y0 = *ypos + b->yoff;
        q->x1 = *xpos + b->xoff2;
        q->y1 = *ypos + b->yoff2;
    }

    q->s0 = b->x0 * ipw;
    q->t0 = b->y0 * iph;
    q->s1 = b->x1 * ipw;
    q->t1 = b->y1 * iph;

    *xpos += b->xadvance;
}

//////////////////////////////////////////////////////////////////////////////
//
// sdf computation
//

#define STBTT_min(a, b) ((a) < (b) ? (a) : (b))
#define STBTT_max(a, b) ((a) < (b) ? (b) : (a))

static int stbtt__ray_intersect_bezier(float orig[2], float ray[2], float q0[2], float q1[2], float q2[2],
                                       float hits[2][2])
{
    float q0perp = q0[1] * ray[0] - q0[0] * ray[1];
    float q1perp = q1[1] * ray[0] - q1[0] * ray[1];
    float q2perp = q2[1] * ray[0] - q2[0] * ray[1];
    float roperp = orig[1] * ray[0] - orig[0] * ray[1];

    float a      = q0perp - 2 * q1perp + q2perp;
    float b      = q1perp - q0perp;
    float c      = q0perp - roperp;

    float s0 = 0., s1 = 0.;
    int   num_s = 0;

    if (a != 0.0)
    {
        float discr = b * b - a * c;
        if (discr > 0.0)
        {
            float rcpna = -1 / a;
            float d     = (float)STBTT_sqrt(discr);
            s0          = (b + d) * rcpna;
            s1          = (b - d) * rcpna;
            if (s0 >= 0.0 && s0 <= 1.0)
                num_s = 1;
            if (d > 0.0 && s1 >= 0.0 && s1 <= 1.0)
            {
                if (num_s == 0)
                    s0 = s1;
                ++num_s;
            }
        }
    }
    else
    {
        // 2*b*s + c = 0
        // s = -c / (2*b)
        s0 = c / (-2 * b);
        if (s0 >= 0.0 && s0 <= 1.0)
            num_s = 1;
    }

    if (num_s == 0)
        return 0;
    else
    {
        float rcp_len2 = 1 / (ray[0] * ray[0] + ray[1] * ray[1]);
        float rayn_x = ray[0] * rcp_len2, rayn_y = ray[1] * rcp_len2;

        float q0d  = q0[0] * rayn_x + q0[1] * rayn_y;
        float q1d  = q1[0] * rayn_x + q1[1] * rayn_y;
        float q2d  = q2[0] * rayn_x + q2[1] * rayn_y;
        float rod  = orig[0] * rayn_x + orig[1] * rayn_y;

        float q10d = q1d - q0d;
        float q20d = q2d - q0d;
        float q0rd = q0d - rod;

        hits[0][0] = q0rd + s0 * (2.0f - 2.0f * s0) * q10d + s0 * s0 * q20d;
        hits[0][1] = a * s0 + b;

        if (num_s > 1)
        {
            hits[1][0] = q0rd + s1 * (2.0f - 2.0f * s1) * q10d + s1 * s1 * q20d;
            hits[1][1] = a * s1 + b;
            return 2;
        }
        else
        {
            return 1;
        }
    }
}

static int equal(float *a, float *b)
{
    return (a[0] == b[0] && a[1] == b[1]);
}

static int stbtt__compute_crossings_x(float x, float y, int nverts, stbtt_vertex *verts)
{
    int   i;
    float orig[2], ray[2] = {1, 0};
    float y_frac;
    int   winding = 0;

    // make sure y never passes through a vertex of the shape
    y_frac = (float)STBTT_fmod(y, 1.0f);
    if (y_frac < 0.01f)
        y += 0.01f;
    else if (y_frac > 0.99f)
        y -= 0.01f;

    orig[0] = x;
    orig[1] = y;

    // test a ray from (-infinity,y) to (x,y)
    for (i = 0; i < nverts; ++i)
    {
        if (verts[i].type == STBTT_vline)
        {
            int x0 = (int)verts[i - 1].x, y0 = (int)verts[i - 1].y;
            int x1 = (int)verts[i].x, y1 = (int)verts[i].y;
            if (y > STBTT_min(y0, y1) && y < STBTT_max(y0, y1) && x > STBTT_min(x0, x1))
            {
                float x_inter = (y - y0) / (y1 - y0) * (x1 - x0) + x0;
                if (x_inter < x)
                    winding += (y0 < y1) ? 1 : -1;
            }
        }
        if (verts[i].type == STBTT_vcurve)
        {
            int x0 = (int)verts[i - 1].x, y0 = (int)verts[i - 1].y;
            int x1 = (int)verts[i].cx, y1 = (int)verts[i].cy;
            int x2 = (int)verts[i].x, y2 = (int)verts[i].y;
            int ax = STBTT_min(x0, STBTT_min(x1, x2)), ay = STBTT_min(y0, STBTT_min(y1, y2));
            int by = STBTT_max(y0, STBTT_max(y1, y2));
            if (y > ay && y < by && x > ax)
            {
                float q0[2], q1[2], q2[2];
                float hits[2][2];
                q0[0] = (float)x0;
                q0[1] = (float)y0;
                q1[0] = (float)x1;
                q1[1] = (float)y1;
                q2[0] = (float)x2;
                q2[1] = (float)y2;
                if (equal(q0, q1) || equal(q1, q2))
                {
                    x0 = (int)verts[i - 1].x;
                    y0 = (int)verts[i - 1].y;
                    x1 = (int)verts[i].x;
                    y1 = (int)verts[i].y;
                    if (y > STBTT_min(y0, y1) && y < STBTT_max(y0, y1) && x > STBTT_min(x0, x1))
                    {
                        float x_inter = (y - y0) / (y1 - y0) * (x1 - x0) + x0;
                        if (x_inter < x)
                            winding += (y0 < y1) ? 1 : -1;
                    }
                }
                else
                {
                    int num_hits = stbtt__ray_intersect_bezier(orig, ray, q0, q1, q2, hits);
                    if (num_hits >= 1)
                        if (hits[0][0] < 0)
                            winding += (hits[0][1] < 0 ? -1 : 1);
                    if (num_hits >= 2)
                        if (hits[1][0] < 0)
                            winding += (hits[1][1] < 0 ? -1 : 1);
                }
            }
        }
    }
    return winding;
}

static float stbtt__cuberoot(float x)
{
    if (x < 0)
        return -(float)STBTT_pow(-x, 1.0f / 3.0f);
    else
        return (float)STBTT_pow(x, 1.0f / 3.0f);
}

// x^3 + a*x^2 + b*x + c = 0
static int stbtt__solve_cubic(float a, float b, float c, float *r)
{
    float s  = -a / 3;
    float p  = b - a * a / 3;
    float q  = a * (2 * a * a - 9 * b) / 27 + c;
    float p3 = p * p * p;
    float d  = q * q + 4 * p3 / 27;
    if (d >= 0)
    {
        float z = (float)STBTT_sqrt(d);
        float u = (-q + z) / 2;
        float v = (-q - z) / 2;
        u       = stbtt__cuberoot(u);
        v       = stbtt__cuberoot(v);
        r[0]    = s + u + v;
        return 1;
    }
    else
    {
        float u = (float)STBTT_sqrt(-p / 3);
        float v = (float)STBTT_acos(-STBTT_sqrt(-27 / p3) * q / 2) / 3; // p3 must be negative, since d is negative
        float m = (float)STBTT_cos(v);
        float n = (float)STBTT_cos(v - 3.141592 / 2) * 1.732050808f;
        r[0]    = s + u * 2 * m;
        r[1]    = s - u * (m + n);
        r[2]    = s - u * (m - n);

        // STBTT_assert( STBTT_fabs(((r[0]+a)*r[0]+b)*r[0]+c) < 0.05f);  // these asserts may not be safe at all
        // scales, though they're in bezier t parameter units so maybe? STBTT_assert(
        // STBTT_fabs(((r[1]+a)*r[1]+b)*r[1]+c) < 0.05f); STBTT_assert( STBTT_fabs(((r[2]+a)*r[2]+b)*r[2]+c) <
        // 0.05f);
        return 3;
    }
}

STBTT_DEF unsigned char *stbtt_GetGlyphSDF(const stbtt_fontinfo *info, float scale, int glyph, int padding,
                                           unsigned char onedge_value, float pixel_dist_scale, int *width, int *height,
                                           int *xoff, int *yoff)
{
    float          scale_x = scale, scale_y = scale;
    int            ix0, iy0, ix1, iy1;
    int            w, h;
    unsigned char *data;

    if (scale == 0)
        return NULL;

    stbtt_GetGlyphBitmapBoxSubpixel(info, glyph, scale, scale, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);

    // if empty, return NULL
    if (ix0 == ix1 || iy0 == iy1)
        return NULL;

    ix0 -= padding;
    iy0 -= padding;
    ix1 += padding;
    iy1 += padding;

    w = (ix1 - ix0);
    h = (iy1 - iy0);

    if (width)
        *width = w;
    if (height)
        *height = h;
    if (xoff)
        *xoff = ix0;
    if (yoff)
        *yoff = iy0;

    // invert for y-downwards bitmaps
    scale_y = -scale_y;

    {
        int           x, y, i, j;
        float        *precompute;
        stbtt_vertex *verts;
        int           num_verts = stbtt_GetGlyphShape(info, glyph, &verts);
        data                    = (unsigned char *)STBTT_malloc(w * h, info->userdata);
        precompute              = (float *)STBTT_malloc(num_verts * sizeof(float), info->userdata);

        for (i = 0, j = num_verts - 1; i < num_verts; j = i++)
        {
            if (verts[i].type == STBTT_vline)
            {
                float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;
                float x1 = verts[j].x * scale_x, y1 = verts[j].y * scale_y;
                float dist    = (float)STBTT_sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
                precompute[i] = (dist == 0) ? 0.0f : 1.0f / dist;
            }
            else if (verts[i].type == STBTT_vcurve)
            {
                float x2 = verts[j].x * scale_x, y2 = verts[j].y * scale_y;
                float x1 = verts[i].cx * scale_x, y1 = verts[i].cy * scale_y;
                float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;
                float bx = x0 - 2 * x1 + x2, by = y0 - 2 * y1 + y2;
                float len2 = bx * bx + by * by;
                if (len2 != 0.0f)
                    precompute[i] = 1.0f / (bx * bx + by * by);
                else
                    precompute[i] = 0.0f;
            }
            else
                precompute[i] = 0.0f;
        }

        for (y = iy0; y < iy1; ++y)
        {
            for (x = ix0; x < ix1; ++x)
            {
                float val;
                float min_dist = 999999.0f;
                float sx       = (float)x + 0.5f;
                float sy       = (float)y + 0.5f;
                float x_gspace = (sx / scale_x);
                float y_gspace = (sy / scale_y);

                int   winding =
                    stbtt__compute_crossings_x(x_gspace, y_gspace, num_verts,
                                               verts); // @OPTIMIZE: this could just be a rasterization, but needs
                                                       // to be line vs. non-tesselated curves so a new path

                for (i = 0; i < num_verts; ++i)
                {
                    float x0 = verts[i].x * scale_x, y0 = verts[i].y * scale_y;

                    if (verts[i].type == STBTT_vline && precompute[i] != 0.0f)
                    {
                        float x1 = verts[i - 1].x * scale_x, y1 = verts[i - 1].y * scale_y;

                        float dist, dist2                       = (x0 - sx) * (x0 - sx) + (y0 - sy) * (y0 - sy);
                        if (dist2 < min_dist * min_dist)
                            min_dist = (float)STBTT_sqrt(dist2);

                        // coarse culling against bbox
                        // if (sx > STBTT_min(x0,x1)-min_dist && sx < STBTT_max(x0,x1)+min_dist &&
                        //    sy > STBTT_min(y0,y1)-min_dist && sy < STBTT_max(y0,y1)+min_dist)
                        dist = (float)STBTT_fabs((x1 - x0) * (y0 - sy) - (y1 - y0) * (x0 - sx)) * precompute[i];
                        STBTT_assert(i != 0);
                        if (dist < min_dist)
                        {
                            // check position along line
                            // x' = x0 + t*(x1-x0), y' = y0 + t*(y1-y0)
                            // minimize (x'-sx)*(x'-sx)+(y'-sy)*(y'-sy)
                            float dx = x1 - x0, dy = y1 - y0;
                            float px = x0 - sx, py = y0 - sy;
                            // minimize (px+t*dx)^2 + (py+t*dy)^2 = px*px + 2*px*dx*t + t^2*dx*dx + py*py +
                            // 2*py*dy*t + t^2*dy*dy derivative: 2*px*dx + 2*py*dy + (2*dx*dx+2*dy*dy)*t, set to 0
                            // and solve
                            float t = -(px * dx + py * dy) / (dx * dx + dy * dy);
                            if (t >= 0.0f && t <= 1.0f)
                                min_dist = dist;
                        }
                    }
                    else if (verts[i].type == STBTT_vcurve)
                    {
                        float x2 = verts[i - 1].x * scale_x, y2 = verts[i - 1].y * scale_y;
                        float x1 = verts[i].cx * scale_x, y1 = verts[i].cy * scale_y;
                        float box_x0 = STBTT_min(STBTT_min(x0, x1), x2);
                        float box_y0 = STBTT_min(STBTT_min(y0, y1), y2);
                        float box_x1 = STBTT_max(STBTT_max(x0, x1), x2);
                        float box_y1 = STBTT_max(STBTT_max(y0, y1), y2);
                        // coarse culling against bbox to avoid computing cubic unnecessarily
                        if (sx > box_x0 - min_dist && sx < box_x1 + min_dist && sy > box_y0 - min_dist &&
                            sy < box_y1 + min_dist)
                        {
                            int   num = 0;
                            float ax = x1 - x0, ay = y1 - y0;
                            float bx = x0 - 2 * x1 + x2, by = y0 - 2 * y1 + y2;
                            float mx = x0 - sx, my = y0 - sy;
                            float res[3] = {0.f, 0.f, 0.f};
                            float px, py, t, it, dist2;
                            float a_inv = precompute[i];
                            if (a_inv == 0.0)
                            { // if a_inv is 0, it's 2nd degree so use quadratic formula
                                float a = 3 * (ax * bx + ay * by);
                                float b = 2 * (ax * ax + ay * ay) + (mx * bx + my * by);
                                float c = mx * ax + my * ay;
                                if (a == 0.0)
                                { // if a is 0, it's linear
                                    if (b != 0.0)
                                    {
                                        res[num++] = -c / b;
                                    }
                                }
                                else
                                {
                                    float discriminant = b * b - 4 * a * c;
                                    if (discriminant < 0)
                                        num = 0;
                                    else
                                    {
                                        float root = (float)STBTT_sqrt(discriminant);
                                        res[0]     = (-b - root) / (2 * a);
                                        res[1]     = (-b + root) / (2 * a);
                                        num = 2; // don't bother distinguishing 1-solution case, as code below will
                                                 // still work
                                    }
                                }
                            }
                            else
                            {
                                float b = 3 * (ax * bx + ay * by) *
                                          a_inv; // could precompute this as it doesn't depend on sample point
                                float c = (2 * (ax * ax + ay * ay) + (mx * bx + my * by)) * a_inv;
                                float d = (mx * ax + my * ay) * a_inv;
                                num     = stbtt__solve_cubic(b, c, d, res);
                            }
                            dist2 = (x0 - sx) * (x0 - sx) + (y0 - sy) * (y0 - sy);
                            if (dist2 < min_dist * min_dist)
                                min_dist = (float)STBTT_sqrt(dist2);

                            if (num >= 1 && res[0] >= 0.0f && res[0] <= 1.0f)
                            {
                                t = res[0], it = 1.0f - t;
                                px    = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                                py    = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                                if (dist2 < min_dist * min_dist)
                                    min_dist = (float)STBTT_sqrt(dist2);
                            }
                            if (num >= 2 && res[1] >= 0.0f && res[1] <= 1.0f)
                            {
                                t = res[1], it = 1.0f - t;
                                px    = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                                py    = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                                if (dist2 < min_dist * min_dist)
                                    min_dist = (float)STBTT_sqrt(dist2);
                            }
                            if (num >= 3 && res[2] >= 0.0f && res[2] <= 1.0f)
                            {
                                t = res[2], it = 1.0f - t;
                                px    = it * it * x0 + 2 * t * it * x1 + t * t * x2;
                                py    = it * it * y0 + 2 * t * it * y1 + t * t * y2;
                                dist2 = (px - sx) * (px - sx) + (py - sy) * (py - sy);
                                if (dist2 < min_dist * min_dist)
                                    min_dist = (float)STBTT_sqrt(dist2);
                            }
                        }
                    }
                }
                if (winding == 0)
                    min_dist = -min_dist; // if outside the shape, value is negative
                val = onedge_value + pixel_dist_scale * min_dist;
                if (val < 0)
                    val = 0;
                else if (val > 255)
                    val = 255;
                data[(y - iy0) * w + (x - ix0)] = (unsigned char)val;
            }
        }
        STBTT_free(precompute, info->userdata);
        STBTT_free(verts, info->userdata);
    }
    return data;
}

STBTT_DEF unsigned char *stbtt_GetCodepointSDF(const stbtt_fontinfo *info, float scale, int codepoint, int padding,
                                               unsigned char onedge_value, float pixel_dist_scale, int *width,
                                               int *height, int *xoff, int *yoff)
{
    return stbtt_GetGlyphSDF(info, scale, stbtt_FindGlyphIndex(info, codepoint), padding, onedge_value,
                             pixel_dist_scale, width, height, xoff, yoff);
}

STBTT_DEF void stbtt_FreeSDF(unsigned char *bitmap, void *userdata)
{
    STBTT_free(bitmap, userdata);
}

//////////////////////////////////////////////////////////////////////////////
//
// font name matching -- recommended not to use this
//

// check if a utf8 string contains a prefix which is the utf16 string; if so return length of matching utf8 string
static stbtt_int32 stbtt__CompareUTF8toUTF16_bigendian_prefix(stbtt_uint8 *s1, stbtt_int32 len1, stbtt_uint8 *s2,
                                                              stbtt_int32 len2)
{
    stbtt_int32 i = 0;

    // convert utf16 to utf8 and compare the results while converting
    while (len2)
    {
        stbtt_uint16 ch = s2[0] * 256 + s2[1];
        if (ch < 0x80)
        {
            if (i >= len1)
                return -1;
            if (s1[i++] != ch)
                return -1;
        }
        else if (ch < 0x800)
        {
            if (i + 1 >= len1)
                return -1;
            if (s1[i++] != 0xc0 + (ch >> 6))
                return -1;
            if (s1[i++] != 0x80 + (ch & 0x3f))
                return -1;
        }
        else if (ch >= 0xd800 && ch < 0xdc00)
        {
            stbtt_uint32 c;
            stbtt_uint16 ch2 = s2[2] * 256 + s2[3];
            if (i + 3 >= len1)
                return -1;
            c = ((ch - 0xd800) << 10) + (ch2 - 0xdc00) + 0x10000;
            if (s1[i++] != 0xf0 + (c >> 18))
                return -1;
            if (s1[i++] != 0x80 + ((c >> 12) & 0x3f))
                return -1;
            if (s1[i++] != 0x80 + ((c >> 6) & 0x3f))
                return -1;
            if (s1[i++] != 0x80 + ((c)&0x3f))
                return -1;
            s2 += 2; // plus another 2 below
            len2 -= 2;
        }
        else if (ch >= 0xdc00 && ch < 0xe000)
        {
            return -1;
        }
        else
        {
            if (i + 2 >= len1)
                return -1;
            if (s1[i++] != 0xe0 + (ch >> 12))
                return -1;
            if (s1[i++] != 0x80 + ((ch >> 6) & 0x3f))
                return -1;
            if (s1[i++] != 0x80 + ((ch)&0x3f))
                return -1;
        }
        s2 += 2;
        len2 -= 2;
    }
    return i;
}

static int stbtt_CompareUTF8toUTF16_bigendian_internal(char *s1, int len1, char *s2, int len2)
{
    return len1 == stbtt__CompareUTF8toUTF16_bigendian_prefix((stbtt_uint8 *)s1, len1, (stbtt_uint8 *)s2, len2);
}

// returns results in whatever encoding you request... but note that 2-byte encodings
// will be BIG-ENDIAN... use stbtt_CompareUTF8toUTF16_bigendian() to compare
STBTT_DEF const char *stbtt_GetFontNameString(const stbtt_fontinfo *font, int *length, int platformID, int encodingID,
                                              int languageID, int nameID)
{
    stbtt_int32  i, count, stringOffset;
    stbtt_uint8 *fc     = font->data;
    stbtt_uint32 offset = font->fontstart;
    stbtt_uint32 nm     = stbtt__find_table(fc, offset, "name");
    if (!nm)
        return NULL;

    count        = ttUSHORT(fc + nm + 2);
    stringOffset = nm + ttUSHORT(fc + nm + 4);
    for (i = 0; i < count; ++i)
    {
        stbtt_uint32 loc = nm + 6 + 12 * i;
        if (platformID == ttUSHORT(fc + loc + 0) && encodingID == ttUSHORT(fc + loc + 2) &&
            languageID == ttUSHORT(fc + loc + 4) && nameID == ttUSHORT(fc + loc + 6))
        {
            *length = ttUSHORT(fc + loc + 8);
            return (const char *)(fc + stringOffset + ttUSHORT(fc + loc + 10));
        }
    }
    return NULL;
}

static int stbtt__matchpair(stbtt_uint8 *fc, stbtt_uint32 nm, stbtt_uint8 *name, stbtt_int32 nlen,
                            stbtt_int32 target_id, stbtt_int32 next_id)
{
    stbtt_int32 i;
    stbtt_int32 count        = ttUSHORT(fc + nm + 2);
    stbtt_int32 stringOffset = nm + ttUSHORT(fc + nm + 4);

    for (i = 0; i < count; ++i)
    {
        stbtt_uint32 loc = nm + 6 + 12 * i;
        stbtt_int32  id  = ttUSHORT(fc + loc + 6);
        if (id == target_id)
        {
            // find the encoding
            stbtt_int32 platform = ttUSHORT(fc + loc + 0), encoding = ttUSHORT(fc + loc + 2),
                        language = ttUSHORT(fc + loc + 4);

            // is this a Unicode encoding?
            if (platform == 0 || (platform == 3 && encoding == 1) || (platform == 3 && encoding == 10))
            {
                stbtt_int32 slen = ttUSHORT(fc + loc + 8);
                stbtt_int32 off  = ttUSHORT(fc + loc + 10);

                // check if there's a prefix match
                stbtt_int32 matchlen =
                    stbtt__CompareUTF8toUTF16_bigendian_prefix(name, nlen, fc + stringOffset + off, slen);
                if (matchlen >= 0)
                {
                    // check for target_id+1 immediately following, with same encoding & language
                    if (i + 1 < count && ttUSHORT(fc + loc + 12 + 6) == next_id &&
                        ttUSHORT(fc + loc + 12) == platform && ttUSHORT(fc + loc + 12 + 2) == encoding &&
                        ttUSHORT(fc + loc + 12 + 4) == language)
                    {
                        slen = ttUSHORT(fc + loc + 12 + 8);
                        off  = ttUSHORT(fc + loc + 12 + 10);
                        if (slen == 0)
                        {
                            if (matchlen == nlen)
                                return 1;
                        }
                        else if (matchlen < nlen && name[matchlen] == ' ')
                        {
                            ++matchlen;
                            if (stbtt_CompareUTF8toUTF16_bigendian_internal((char *)(name + matchlen), nlen - matchlen,
                                                                            (char *)(fc + stringOffset + off), slen))
                                return 1;
                        }
                    }
                    else
                    {
                        // if nothing immediately following
                        if (matchlen == nlen)
                            return 1;
                    }
                }
            }

            // @TODO handle other encodings
        }
    }
    return 0;
}

static int stbtt__matches(stbtt_uint8 *fc, stbtt_uint32 offset, stbtt_uint8 *name, stbtt_int32 flags)
{
    stbtt_int32  nlen = (stbtt_int32)STBTT_strlen((char *)name);
    stbtt_uint32 nm, hd;
    if (!stbtt__isfont(fc + offset))
        return 0;

    // check italics/bold/underline flags in macStyle...
    if (flags)
    {
        hd = stbtt__find_table(fc, offset, "head");
        if ((ttUSHORT(fc + hd + 44) & 7) != (flags & 7))
            return 0;
    }

    nm = stbtt__find_table(fc, offset, "name");
    if (!nm)
        return 0;

    if (flags)
    {
        // if we checked the macStyle flags, then just check the family and ignore the subfamily
        if (stbtt__matchpair(fc, nm, name, nlen, 16, -1))
            return 1;
        if (stbtt__matchpair(fc, nm, name, nlen, 1, -1))
            return 1;
        if (stbtt__matchpair(fc, nm, name, nlen, 3, -1))
            return 1;
    }
    else
    {
        if (stbtt__matchpair(fc, nm, name, nlen, 16, 17))
            return 1;
        if (stbtt__matchpair(fc, nm, name, nlen, 1, 2))
            return 1;
        if (stbtt__matchpair(fc, nm, name, nlen, 3, -1))
            return 1;
    }

    return 0;
}

static int stbtt_FindMatchingFont_internal(unsigned char *font_collection, char *name_utf8, stbtt_int32 flags)
{
    stbtt_int32 i;
    for (i = 0;; ++i)
    {
        stbtt_int32 off = stbtt_GetFontOffsetForIndex(font_collection, i);
        if (off < 0)
            return off;
        if (stbtt__matches((stbtt_uint8 *)font_collection, off, (stbtt_uint8 *)name_utf8, flags))
            return off;
    }
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

STBTT_DEF int stbtt_BakeFontBitmap(const unsigned char *data, int offset, float pixel_height, unsigned char *pixels,
                                   int pw, int ph, int first_char, int num_chars, stbtt_bakedchar *chardata)
{
    return stbtt_BakeFontBitmap_internal((unsigned char *)data, offset, pixel_height, pixels, pw, ph, first_char,
                                         num_chars, chardata);
}

STBTT_DEF int stbtt_GetFontOffsetForIndex(const unsigned char *data, int index)
{
    return stbtt_GetFontOffsetForIndex_internal((unsigned char *)data, index);
}

STBTT_DEF int stbtt_GetNumberOfFonts(const unsigned char *data)
{
    return stbtt_GetNumberOfFonts_internal((unsigned char *)data);
}

STBTT_DEF int stbtt_InitFont(stbtt_fontinfo *info, const unsigned char *data, int offset)
{
    return stbtt_InitFont_internal(info, (unsigned char *)data, offset);
}

STBTT_DEF int stbtt_FindMatchingFont(const unsigned char *fontdata, const char *name, int flags)
{
    return stbtt_FindMatchingFont_internal((unsigned char *)fontdata, (char *)name, flags);
}

STBTT_DEF int stbtt_CompareUTF8toUTF16_bigendian(const char *s1, int len1, const char *s2, int len2)
{
    return stbtt_CompareUTF8toUTF16_bigendian_internal((char *)s1, len1, (char *)s2, len2);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif // STB_TRUETYPE_IMPLEMENTATION

// FULL VERSION HISTORY
//
//   1.25 (2021-07-11) many fixes
//   1.24 (2020-02-05) fix warning
//   1.23 (2020-02-02) query SVG data for glyphs; query whole kerning table (but only kern not GPOS)
//   1.22 (2019-08-11) minimize missing-glyph duplication; fix kerning if both 'GPOS' and 'kern' are defined
//   1.21 (2019-02-25) fix warning
//   1.20 (2019-02-07) PackFontRange skips missing codepoints; GetScaleFontVMetrics()
//   1.19 (2018-02-11) OpenType GPOS kerning (horizontal only), STBTT_fmod
//   1.18 (2018-01-29) add missing function
//   1.17 (2017-07-23) make more arguments const; doc fix
//   1.16 (2017-07-12) SDF support
//   1.15 (2017-03-03) make more arguments const
//   1.14 (2017-01-16) num-fonts-in-TTC function
//   1.13 (2017-01-02) support OpenType fonts, certain Apple fonts
//   1.12 (2016-10-25) suppress warnings about casting away const with -Wcast-qual
//   1.11 (2016-04-02) fix unused-variable warning
//   1.10 (2016-04-02) allow user-defined fabs() replacement
//                     fix memory leak if fontsize=0.0
//                     fix warning from duplicate typedef
//   1.09 (2016-01-16) warning fix; avoid crash on outofmem; use alloc userdata for PackFontRanges
//   1.08 (2015-09-13) document stbtt_Rasterize(); fixes for vertical & horizontal edges
//   1.07 (2015-08-01) allow PackFontRanges to accept arrays of sparse codepoints;
//                     allow PackFontRanges to pack and render in separate phases;
//                     fix stbtt_GetFontOFfsetForIndex (never worked for non-0 input?);
//                     fixed an assert() bug in the new rasterizer
//                     replace assert() with STBTT_assert() in new rasterizer
//   1.06 (2015-07-14) performance improvements (~35% faster on x86 and x64 on test machine)
//                     also more precise AA rasterizer, except if shapes overlap
//                     remove need for STBTT_sort
//   1.05 (2015-04-15) fix misplaced definitions for STBTT_STATIC
//   1.04 (2015-04-15) typo in example
//   1.03 (2015-04-12) STBTT_STATIC, fix memory leak in new packing, various fixes
//   1.02 (2014-12-10) fix various warnings & compile issues w/ stb_rect_pack, C++
//   1.01 (2014-12-08) fix subpixel position when oversampling to exactly match
//                        non-oversampled; STBTT_POINT_SIZE for packed case only
//   1.00 (2014-12-06) add new PackBegin etc. API, w/ support for oversampling
//   0.99 (2014-09-18) fix multiple bugs with subpixel rendering (ryg)
//   0.9  (2014-08-07) support certain mac/iOS fonts without an MS platformID
//   0.8b (2014-07-07) fix a warning
//   0.8  (2014-05-25) fix a few more warnings
//   0.7  (2013-09-25) bugfix: subpixel glyph bug fixed in 0.5 had come back
//   0.6c (2012-07-24) improve documentation
//   0.6b (2012-07-20) fix a few more warnings
//   0.6  (2012-07-17) fix warnings; added stbtt_ScaleForMappingEmToPixels,
//                        stbtt_GetFontBoundingBox, stbtt_IsGlyphEmpty
//   0.5  (2011-12-09) bugfixes:
//                        subpixel glyph renderer computed wrong bounding box
//                        first vertex of shape can be off-curve (FreeSans)
//   0.4b (2011-12-03) fixed an error in the font baking example
//   0.4  (2011-12-01) kerning, subpixel rendering (tor)
//                    bugfixes for:
//                        codepoint-to-glyph conversion using table fmt=12
//                        codepoint-to-glyph conversion using table fmt=4
//                        stbtt_GetBakedQuad with non-square texture (Zer)
//                    updated Hello World! sample to use kerning and subpixel
//                    fixed some warnings
//   0.3  (2009-06-24) cmap fmt=12, compound shapes (MM)
//                    userdata, malloc-from-userdata, non-zero fill (stb)
//   0.2  (2009-03-11) Fix unsigned/signed char warnings
//   0.1  (2009-03-09) First public release
//

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO :: Minimize floating point errors
// TODO :: Allow customization
// TODO :: Allow multiple graphs be drawn on the same window -> Done simply (No plot device yet)
// TODO :: Load font library and label the axes -> Done// Make UI appealing -> Partially done

// TODO Later : Add terminal and a parser

#define TriggerBreakpoint()                                                                                            \
    {                                                                                                                  \
        printf("Abort called at func -> %s, line ->  %d.", __func__, __LINE__);                                        \
        abort();                                                                                                       \
    }

double GaussianIntegral(double x)
{
    return 4 * exp(-x * x / 2);
}

double parabola(double x)
{
    return x * x;
}

double inv(double x)
{
    return 1 / x;
}

double lin(double x)
{
    return x;
}

double discont(double x)
{
    return 1 / (x * x - 1);
}

#define no_default_case() __assume(0)

struct Mat4
{
    float elem[4][4];
};

static Mat4 OrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar)
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

static Mat4 ScalarMatrix(float x, float y, float z)
{
    Mat4 matrix       = {{0}};
    matrix.elem[0][0] = x;
    matrix.elem[1][1] = y;
    matrix.elem[2][2] = z;
    matrix.elem[3][3] = 1.0f;
    return matrix;
}

static Mat4 TranslationMatrix(float x, float y, float z)
{
    Mat4 matrix       = {{0}};

    matrix.elem[0][0] = 1.0f;
    matrix.elem[1][1] = 1.0f;
    matrix.elem[2][2] = 1.0f;

    matrix.elem[0][3] = x;
    matrix.elem[1][3] = y;
    matrix.elem[2][3] = z;
    matrix.elem[3][3] = 1.0f;
    return matrix;
}

static Mat4 TransposeMatrix(Mat4 *mat)
{
    Mat4 matrix = {{0}};
    for (uint8_t row = 0; row < 4; ++row)
        for (uint8_t col = 0; col < 4; ++col)
            matrix.elem[row][col] = mat->elem[col][row];
    return matrix;
}

static Mat4 Inverse(Mat4 *mat)
{
    Mat4   matrix = {{0}};
    float *inv    = &matrix.elem[0][0];
    float *m      = &mat->elem[0][0];

    double det;
    int    i;

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
             m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
             m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
             m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
              m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
             m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
             m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
             m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
              m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
             m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
             m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
              m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
              m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        inv[i] = inv[i] * det;
    return matrix;
}

static Mat4 MatrixMultiply(Mat4 *mat1, Mat4 *mat2)
{
    Mat4  matrix = {{0}};
    float var    = 0;
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

static void MatrixVectorMultiply(Mat4 *mat, float vec[4])
{
    float result[4] = {0};
    for (uint8_t i = 0; i < 4; ++i)
    {
        for (uint8_t j = 0; j < 4; ++j)
        {
            result[i] += mat->elem[i][j] * vec[j];
        }
    }
    memcpy(vec, result, sizeof(float) * 4);
}

static Mat4 IdentityMatrix()
{
    Mat4 matrix       = {{0}};
    matrix.elem[0][0] = 1.0f;
    matrix.elem[1][1] = 1.0f;
    matrix.elem[2][2] = 1.0f;
    matrix.elem[3][3] = 1.0f;
    return matrix;
}

static int screen_width  = 800;
static int screen_height = 600;

struct Graph
{
    unsigned int vao;
    unsigned int vbo;
    unsigned int program;

    float        width;
    float        value;

    MVec2        center;
    MVec2        scale;

    MVec2        slide_scale; // Controls the major scaling on the axes of the graph
};

typedef struct
{
    Mat4  *OrthoMatrix;
    Graph *graph;
    Mat4  *scale_transform;
} UserData;

typedef struct
{
    char  *data;
    size_t length;
} String;

#define MakeString(str) (String){.data = str, .length = strlen(str)};

typedef enum
{
    VERTEX_SHADER,
    FRAGMENT_SHADER,
    GEOMETRY_SHADER
} ShaderType;

typedef struct
{
    unsigned int shader;
    ShaderType   type;
} Shader;

const char *ShaderTypeName(ShaderType shader)
{
    switch (shader)
    {
    case VERTEX_SHADER:
        return "Vertex Shader";
    case FRAGMENT_SHADER:
        return "Fragment Shader";
    case GEOMETRY_SHADER:
        return "Geometry Shader";
    default:
        __assume(false);
    }
}
void ErrorCallback(int code, const char *description)
{
    fprintf(stderr, "\nGLFW Error -> %s.\n", description);
}

void FrameChangeCallback(GLFWwindow *window, int width, int height)
{
    screen_width  = width;
    screen_height = height;
    glViewport(0, 0, width, height);
    UserData *data     = glfwGetWindowUserPointer(window);
    *data->OrthoMatrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int mod, int action)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

float MagicNumberGenerator(int n)
{
    // Try deciphering it .. :D
    int non_neg = n >= 0 ? 1 : ((n = -n - 1, (n = 2 - n % 3 + 3 * (n / 3) + 3)), 0);
    int p       = n / 3;
    n           = n % 3;
    float val   = 1;
    // lets try some inline asm
    //   __asm {
    //       push rsi
    //	push rax
    //	xor rax, rax
    //	mov  rsi, 1
    // L1:
    //	lea  rsi, [rsi + rsi * 4]
    //	inc  rax
    //	add  rsi, rsi
    //	cmp  eax, z
    //	jne  L1
    //	mov  a, esi
    //	pop rax
    //	pop rsi
    //   }
    // Lol not supported in x64 architecture
    for (int i = 0; i < p; ++i)
        val *= 10;
    return non_neg ? (n * n + 1) * val : (n * n + 1) / val;
}

void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    const float origin = 200.0f;
    const float scale  = 5.0f;

    UserData   *data   = glfwGetWindowUserPointer(window);
    Graph      *graph  = data->graph;

    // capture mouse co-ordinates here
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    // shift the origin somewhere far from here
    const int scale_factor = 100;

    double    scaledFactor = 0.0f;

    float     prev_scale_x = graph->slide_scale.x;
    graph->slide_scale.y += scale * yoffset;
    graph->slide_scale.x += scale * yoffset;
    scaledFactor = graph->slide_scale.x / prev_scale_x;
    // Dynamic scaling looks kinda hard
    static int absScale = 0;

    bool       changeX = false, changeY = false;
    float      s = 1.0f;
    if (yoffset < 0)
    {
        float should_scale_x = origin / graph->slide_scale.x * graph->scale.x;
        if (should_scale_x >= MagicNumberGenerator(absScale + 1))
            changeX = true;
    }
    else if (yoffset > 0)
    {
        float should_downscale = origin / graph->slide_scale.x * graph->scale.x;
        if (should_downscale <= MagicNumberGenerator(absScale - 1))
            changeY = true;
    }

    if (changeX || changeY)
    {
        if (changeX)
        {
            graph->scale.x = MagicNumberGenerator(++absScale);
            graph->scale.y = graph->scale.x;
        }
        if (changeY)
        {
            graph->scale.x = MagicNumberGenerator(--absScale);
            graph->scale.y = graph->scale.x;
        }
        graph->slide_scale = (MVec2){origin, origin};
    }

    // adjust the scaling of the graph

    Mat4 *scale_transform = data->scale_transform;
    s                     = graph->slide_scale.x / graph->scale.x;
    *scale_transform      = ScalarMatrix(s, s, 1.0f);
}

unsigned int LoadProgram(Shader vertex, Shader fragment)
{
    if (vertex.type != VERTEX_SHADER && fragment.type != FRAGMENT_SHADER)
    {
        fprintf(stderr, "Shaders mismatched for program\n");
        return -1;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex.shader);
    glAttachShader(program, fragment.shader);

    glLinkProgram(program);

    int linked = 0;

    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Failed to link program \n -> %s.", infoLog);
        return -1;
    }
    return program;
}

unsigned int LoadProgram3(Shader vertex, Shader fragment, Shader geometry)
{
    if (vertex.type != VERTEX_SHADER && fragment.type != FRAGMENT_SHADER && geometry.type != GEOMETRY_SHADER)
    {
        fprintf(stderr, "Shaders mismatched for program\n");
        return -1;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex.shader);
    glAttachShader(program, fragment.shader);
    glAttachShader(program, geometry.shader);

    glLinkProgram(program);
    glHint(GL_LINE_SMOOTH, GL_NICEST);

    int linked = 0;

    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Failed to link program \n -> %s.", infoLog);
        return -1;
    }
    return program;
}

String ReadFiles(const char *file_path)
{
    FILE *fp = fopen(file_path, "rb");
    if (!fp)
        return (String){.data = NULL, .length = 0};

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    String contents;
    contents.data       = malloc(sizeof(char) * (size + 1));
    contents.length     = size;

    size_t read_size    = fread(contents.data, sizeof(char), size + 1, fp);
    contents.data[size] = '\0';
    assert(read_size == size);
    return contents;
}

Shader LoadShadersFromString(const char *cstr, ShaderType type);
Shader LoadShader(const char *shader_path, ShaderType type)
{
    String       str = ReadFiles(shader_path);
    Shader       shader;
    unsigned int shader_define;
    switch (type)
    {
    case FRAGMENT_SHADER:
        shader_define = GL_FRAGMENT_SHADER;
        break;
    case VERTEX_SHADER:
        shader_define = GL_VERTEX_SHADER;
        break;
    case GEOMETRY_SHADER:
        shader_define = GL_GEOMETRY_SHADER;
    default:
        __assume(false);
    }
    shader.shader = glCreateShader(shader_define);
    shader.type   = type;
    glShaderSource(shader.shader, 1, &str.data, NULL);
    glCompileShader(shader.shader);
    int compiled;
    glGetShaderiv(shader.shader, GL_COMPILE_STATUS, &compiled);

    free(str.data);

    if (!compiled)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader.shader, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile %s", ShaderTypeName(type));
        fprintf(stderr, "Reason -> %s.", infoLog);
    }
    else
        fprintf(stderr, "\n%s compilation passed.\n", ShaderTypeName(type));
    return shader;
}

GLFWwindow *LoadGLFW(int width, int height, const char *title)
{
    glfwWindowHint(GLFW_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4);

    if (!glfwInit())
    {
        fprintf(stderr, "Failed to load GLFW api\n");
        return NULL;
    }
    glfwSetErrorCallback(ErrorCallback);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwSetFramebufferSizeCallback(window, FrameChangeCallback);
    if (!window)
    {
        fprintf(stderr, "Failed to create window");
        return NULL;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to load glad");
        return NULL;
    }

    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);
    return window;
}

typedef struct VertexBuffer
{
    bool     dirty;
    uint32_t count;
    uint32_t max;
    uint32_t vbo;
    uint8_t *data;
} VertexBuffer;

typedef enum Primitives
{
    TRIANGLES      = GL_TRIANGLES,
    LINES          = GL_LINES,
    LINE_STRIP     = GL_LINE_STRIP,
    LINE_LOOP      = GL_LINE_LOOP,
    TRIANGLE_STRIP = GL_TRIANGLE_STRIP
} Primitives;

typedef enum FunctionType
{
    LIST, 
    PARAMETRIC_1D,
    PARAMETRIC_2D, 
    IMPLICIT_2D
} FunctionType; 

typedef struct GPUBatch
{
    VertexBuffer vertex_buffer;
    uint32_t     vao;
    Primitives   primitive;
} GPUBatch;

typedef struct VertexData2D
{
    float x, y;
    float n_x, n_y;
} VertexData2D;

typedef struct FunctionPlotData
{
    bool           updated;

    FunctionType   fn_type; 
    void*          function;
    GPUBatch      *batch;
    uint32_t       max;
    uint32_t       count;
    MVec3          color;
    VertexData2D  *samples;
    char           plot_name[25];
} FunctionPlotData;

typedef struct FontData
{
    bool      updated;
    GPUBatch *batch;
    uint32_t  max;
    MVec3     color;
    uint32_t  count;
    MVec2    *data;
} FontData;

typedef struct PlotArray
{
    uint32_t          max;
    uint32_t          count;
    int32_t           current_selection;
    FunctionPlotData *functions;
} PlotArray;

typedef struct FontArray
{
    uint32_t  max;
    uint32_t  count;
    FontData *fonts;
} FontArray;

typedef struct Scene
{
    PlotArray plots;
    FontData  axes_labels;
    FontData  legends;
} Scene;

struct State
{
    bool   bPressed;
    double xpos;
    double ypos;
};

GPUBatch *CreateNewBatch(Primitives primitive)
{
    GPUBatch *batch          = malloc(sizeof(*batch));
    batch->vertex_buffer.max = 25000;
    batch->primitive         = primitive;

    glGenVertexArrays(1, &batch->vao);
    glGenBuffers(1, &batch->vertex_buffer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, batch->vertex_buffer.max * sizeof(*batch->vertex_buffer.data), NULL, GL_STATIC_DRAW);

    batch->vertex_buffer.max   = 25000;
    batch->vertex_buffer.count = 0;
    batch->vertex_buffer.dirty = true;
    batch->vertex_buffer.data  = malloc(sizeof(uint8_t) * batch->vertex_buffer.max);

    return batch;
}

void DrawBatch(GPUBatch *batch, uint32_t counts)
{
    glBindVertexArray(batch->vao);
    glDrawArrays(batch->primitive, 0, counts);
}

void PrepareBatch(GPUBatch *batch)
{
    if (batch->vertex_buffer.dirty)
    {
        glBindVertexArray(batch->vao);
        glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer.vbo);
        void *access = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        // This is redundant
        memcpy(access, batch->vertex_buffer.data, sizeof(uint8_t) * batch->vertex_buffer.count);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void *)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        batch->vertex_buffer.dirty = false;
    }
}

typedef MVec2 (*parametricfn)(double);

void InitGraph(Graph *graph)
{
    glGenVertexArrays(1, &graph->vao);
    glBindVertexArray(graph->vao);
    glGenBuffers(1, &graph->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, graph->vbo);

    // use fullscreen to render grid
    float buffer[] = {-1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Shader vertex   = LoadShader("./include/grid_vertex.glsl", VERTEX_SHADER);
    // Shader fragment = LoadShader("./include/grid_fragment.glsl", FRAGMENT_SHADER);

    Shader vertex = LoadShadersFromString("#version 330 core \r\n\r\nlayout (location = 0) in vec2 aPos; \r\n\r\nvoid "
                                          "main() \r\n{ \r\n\tgl_Position = vec4(aPos,0.0f,1.0f);\r\n}",
                                          VERTEX_SHADER);
    Shader fragment = LoadShadersFromString(
        "#version 330 core \r\n\r\nout vec4 color; \r\n\r\n// not working with uniform buffer for now \r\n// Lets try "
        "drawing a checkerboard \r\n// uniform int scale;\r\n\r\nuniform vec2 scale;\r\nuniform int "
        "grid_width;\r\n\r\nuniform vec2 center; \r\n\r\nvoid main() \r\n{\r\n\tvec2 scr = gl_FragCoord.xy;\r\n\tint "
        "delX = abs(int(scr.x-center.x)); \r\n\tint delY = abs(int(scr.y-center.y));\r\n\t\r\n\tint X = int(scale.x); "
        "\r\n\tif (X % 2 != 0) \r\n\t\tX = X + 1; \r\n\r\n\tint Y = X;\r\n\r\n\tint halfX = X / 2; \r\n\tint halfY = "
        "halfX; \r\n\r\n\t// TODO :: Rewrite it in branchless way \r\n\tif ( (delX % halfX <= grid_width) || (delY % "
        "halfY <= grid_width))\r\n\t\tcolor = vec4(0.0f,0.7f,0.7f,1.0f); \r\n\telse\r\n\t\tcolor = "
        "vec4(1.0f,1.0f,1.0f,1.0f);\r\n\t\t\r\n\tif ( (delX % X <= grid_width+2) || (delY % Y <= "
        "grid_width+2))\r\n\t\tcolor = vec4(0.5f,0.5f,0.5f,1.0f);\r\n\r\n\r\n\tif (abs(scr.x - center.x) < 3.0f) "
        "\r\n\t\tcolor = vec4(1.0f,0.0f,0.0f,1.0f);\r\n\tif (abs(scr.y - center.y) < 3.0f) \r\n\t\tcolor = "
        "vec4(1.0f,0.0f,0.0f,1.0f);\r\n}",
        FRAGMENT_SHADER);

    graph->program = LoadProgram(vertex, fragment);
    graph->center  = (MVec2){0.0f, 0.0f};

    // Make graph->scale use value instead of pixel scale
    graph->scale       = (MVec2){1.0f, 1.0f};
    graph->slide_scale = (MVec2){200.0f, 200.0f};
}

void RenderGraph(Graph *graph, Mat4 *transform)
{
    glUseProgram(graph->program);
    glBindVertexArray(graph->vao);
    glUniform1i(glGetUniformLocation(graph->program, "grid_width"), 0);
    float center[4] = {graph->center.x, graph->center.y, 0.0f, 1.0f};
    MatrixVectorMultiply(transform, center);
    glUniform2f(glGetUniformLocation(graph->program, "center"), center[0], center[1]);
    glUniform2f(glGetUniformLocation(graph->program, "scale"), graph->slide_scale.x, graph->slide_scale.y);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glUseProgram(0);
    glBindVertexArray(0);
}

void Destroy2DScene(Scene *scene)
{
    /*free(render_scene->Indices);
    free(render_scene->Vertices);
    free(render_scene->Discontinuity);
    free(render_scene->Points);
    free(render_scene->fVertices);*/
}

void Init2DScene(Scene *scene)
{
    memset(scene, 0, sizeof(*scene));
    // Init enough memory for 10 graphs
    scene->plots.max               = 10;
    scene->plots.functions         = malloc(sizeof(*scene->plots.functions) * scene->plots.max);
    scene->plots.current_selection = -1;

    for (uint32_t plot = 0; plot < scene->plots.max; ++plot)
    {
        /*scene->plots.functions[plot].batch = NULL;
        scene->plots.functions[plot].count = 0; */
        memset(scene->plots.functions + plot, 0, sizeof(*scene->plots.functions));
    }

    scene->axes_labels.count   = 0;
    scene->axes_labels.max     = 100000;
    scene->axes_labels.batch   = CreateNewBatch(TRIANGLES);
    scene->axes_labels.updated = true;
    scene->axes_labels.data    = malloc(sizeof(*scene->axes_labels.data) * scene->axes_labels.max);
}

void RenderScene(Scene *scene, unsigned int program, bool showPoints, Mat4 *mscene, Mat4 *transform)
{
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "scene"), 1, GL_TRUE, &mscene->elem[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_TRUE, &transform->elem[0][0]);

    for (uint32_t graph = 0; graph < scene->plots.count; ++graph)
    {
        FunctionPlotData *function = &scene->plots.functions[graph];
        glUniform3f(glGetUniformLocation(program, "inColor"), function->color.x, function->color.y, function->color.z);
        // Transfer data to the batch
        glUniform1f(glGetUniformLocation(program, "thickness"), 3.0f);
        if (scene->plots.current_selection == graph)
            glUniform1f(glGetUniformLocation(program, "thickness"), 6.0f);

        if (function->updated)
        {
            assert(function->count * 4 * 4 < function->batch->vertex_buffer.max);
            memcpy(function->batch->vertex_buffer.data, function->samples, sizeof(uint8_t) * function->count * 4 * 4);
            function->batch->vertex_buffer.count = function->count * 4 * 4;
            function->updated                    = false;
            function->batch->vertex_buffer.dirty = true;
        }

        PrepareBatch(function->batch);
        DrawBatch(function->batch, function->batch->vertex_buffer.count / (4 * 4));
    }
}

void PrepareScene(Scene *scene, Graph *graph)
{
    // for (uint32_t graph_ = 0; graph_ < scene->plots.count; ++graph_)
    //{
    //     FunctionPlotData *function = &scene->plots.functions[graph_];
    //     for (uint32_t pt = 0; pt < function->count; ++pt)
    //     {
    //         function->samples[pt].x = function->samples[pt].x * graph->slide_scale.x / graph->scale.x;
    //         function->samples[pt].y = function->samples[pt].y * graph->slide_scale.y / graph->scale.y;
    //     }
    // }
}

void ResetScene(Scene *scene_group)
{
    // No op
}

// static void PlotGraph(Scene *scene, ParametricFn1D func, Graph *graph, MVec3 color, const char *legend)
//{
//     // No check done here currently
//     const uint32_t max_verts                           = 1000;
//     scene->plots.functions[scene->plots.count].max     = max_verts; // 1000 vertices for each graph at most
//     scene->plots.functions[scene->plots.count].samples = malloc(sizeof(MVec2) * max_verts);
//
//     MVec2 vec1, vec2;
//
//     float step                 = 0.5f;
//     float init                 = -10.0f;
//     float term                 = 10.0f;
//
//     vec1.x                     = graph->center.x + init * graph->slide_scale.x / (graph->scale.x);
//     vec1.y                     = graph->center.y + func(init) * graph->slide_scale.y / (graph->scale.y);
//
//     FunctionPlotData *function = &scene->plots.functions[scene->plots.count];
//
//     for (float x = init; x <= term; x += step)
//     {
//         vec2.x = graph->center.x + (x + step) * graph->slide_scale.x / (graph->scale.x);
//         vec2.y = graph->center.y + func(x + step) * graph->slide_scale.y / (graph->scale.y);
//
//         // Excluded for now
//         // float slope = atan(fabs((MVec2.y - vec1.y) / (MVec2.x - vec1.x)));
//         assert(function->count < function->max);
//         function->samples[function->count++] = vec1;
//         vec1                                 = vec2;
//     }
//     scene->plots.count++;
// }

static void Plot1D(Scene *scene, ParametricFn1D func, Graph *graph, MVec3 color, const char *legend)
{
    // No check done here currently
    const uint32_t max_verts                           = 1000;
    scene->plots.functions[scene->plots.count].max     = max_verts; // 1000 vertices for each graph at most
    scene->plots.functions[scene->plots.count].samples = malloc(sizeof(MVec2) * max_verts);

    float             init                             = -10.0f;
    float             term                             = 10.0f;
    float             step                             = 0.1f;

    FunctionPlotData *function                         = &scene->plots.functions[scene->plots.count];
    function->fn_type                                  = PARAMETRIC_1D; 

    VertexData2D      vec;
    vec.x                                = init;
    vec.y                                = func(vec.x);
    vec.n_x                              = 1.0f;
    vec.n_y                              = 1.0f;

    function->samples[function->count++] = vec;

    float n_x, n_y;
    for (float x = init + step; x <= term; x += step)
    {
        vec.x = x;
        vec.y = func(vec.x);
        assert(function->count < function->max);
        n_x                                        = x - function->samples[function->count - 1].x;
        n_y                                        = vec.y - function->samples[function->count - 1].y;
        function->samples[function->count - 1].n_x = n_x;
        function->samples[function->count - 1].n_y = n_y;
        function->samples[function->count++]       = vec;
    }
    function->color    = color;
    function->function = func;
    function->batch    = CreateNewBatch(LINE_STRIP);
    function->updated  = true;
    scene->plots.count++;
}

void PlotParametric(Scene *scene, parametricfn func, Graph *graph)
{
    /*MVec2 vec;
    for (float i = -3.141592f * 2; i <= 3.141592f * 2; i += 0.04f)
    {
        vec   = func(i);
        vec.x = graph->center.x + vec.x * graph->scale.x;
        vec.y = graph->center.y + vec.y * graph->scale.y;
        AddSingleVertex(scene_group, vec);
    }*/
}

void HandleEvents(GLFWwindow *window, State *state, Graph *graph, Mat4 *world_transform, Mat4 *scale_matrix);

typedef struct Glyph
{
    MVec2 offset;
    MVec2 size;
    MVec2 bearing;
    int   Advance;
} Glyph;

typedef struct Font
{
    unsigned int font_texture;
    unsigned int vao;
    unsigned int vbo;
    unsigned int program;
    float        rasterScale;
    int          width;
    int          height;
    char         font_buffer[512];
    String       font_name;
    Glyph        character[128]; // For first 128 printable characters only
} Font;

void LoadFont(Font *font, const char *font_dir)
{
    memset(font, 0, sizeof(*font));
    stbtt_fontinfo sfont;
    String         fontbuffer = ReadFiles(font_dir);
    if (!fontbuffer.data)
    {
        fprintf(stderr, "\nError : Failed to load %s.", font_dir);
        return;
    }

    stbtt_InitFont(&sfont, fontbuffer.data, 0);

    // Load the character's data from stb_truetype
    float fontSize = 25;
    float scale    = stbtt_ScaleForPixelHeight(&sfont, fontSize);
    int   ascent, descent, baseline;
    stbtt_GetFontVMetrics(&sfont, &ascent, &descent, 0);
    baseline = (int)(ascent * scale);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    font->rasterScale = scale;

    int height        = (int)((ascent - descent) * scale);
    int width         = 0;

    font->height      = height;

    for (int ch = 0; ch < 128; ++ch)
    {
        int advance, lsb; // left side bearing
        stbtt_GetCodepointHMetrics(&sfont, ch, &advance, &lsb);
        width += advance;
    }
    width = width * scale;

    // Text Rendering starts here
    unsigned char *bmpbuffers = malloc((size_t)width * height);
    memset(bmpbuffers, 0, width * height);

    float xpos  = 0.0f;
    int   above = ascent * scale;

    for (int ch = 0; ch < 128; ++ch)
    {
        int   advance, lsb, x0, y0, x1, y1;
        float x_shift = xpos - (float)(floor(xpos));
        stbtt_GetCodepointHMetrics(&sfont, ch, &advance, &lsb);
        stbtt_GetCodepointBitmapBoxSubpixel(&sfont, ch, scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);

        // auto stride = ((int)xpos + x0) + width * (above + y0); //  * (baseline + y0) + (int)xpos + x0;
        int stride = (int)xpos + x0 + width * (baseline + y0);

        stbtt_MakeCodepointBitmapSubpixel(&sfont, bmpbuffers + stride, x1 - x0, y1 - y0, width, scale, scale, x_shift,
                                          0, ch);

        Glyph *glyph   = &font->character[ch];
        glyph->offset  = (MVec2){(int)xpos, 0};
        glyph->size    = (MVec2){x1 - x0, y1 - y0};
        glyph->Advance = (int)(advance * scale);
        xpos += advance * scale;
    }

    glGenTextures(1, &font->font_texture);
    glBindTexture(GL_TEXTURE_2D, font->font_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load texture into OpenGL memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, bmpbuffers);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &font->vao);
    glGenBuffers(1, &font->vbo);

    glBindVertexArray(font->vao);
    glBindBuffer(GL_ARRAY_BUFFER, font->vbo);

    const float max_font_limit = 1000 * 50;
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * max_font_limit, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    font->width = width;

    // Font shaders and program
    Shader font_vertex = LoadShadersFromString(
        "#version 330 core \r\n\r\nlayout (location = 0) in vec2 aPos; \r\nlayout (location = 1) in vec2 Tex;\r\n// "
        "might need a matrix somewhere here \r\n\r\nout vec2 TexCoord; \r\nuniform mat4 scene; \r\n\r\nvoid "
        "main()\r\n{\r\n\tgl_Position = scene * vec4(aPos,0.0f,1.0f);\r\n\tTexCoord = Tex; \r\n}",
        VERTEX_SHADER);

    Shader font_fragment =
        LoadShadersFromString("#version 330 core\r\n\r\nout vec4 color_vec; \r\n\r\nin vec2 TexCoord; \r\nuniform "
                              "sampler2D font;\r\n\r\n\r\nvoid main()\r\n{\r\n\tvec4 color = "
                              "texture(font,TexCoord);\r\n\tcolor_vec = vec4(0.0f,0.0f,0.0f,color.r);\r\n}",
                              FRAGMENT_SHADER);

    font->program = LoadProgram(font_vertex, font_fragment);
}

// only font name .. not the whole path
void LoadSystemFont(Font *font, const char *font_name)
{
    char font_path[512] = {0};

#ifdef _WIN32
    strcpy(font_path, getenv("WINDIR"));
    strcat(font_path, "\\Fonts\\");
    strcat(font_path, font_name);

#elif defined(__linux__)
    char shell_command[512] = "fc-match --format=%{file} ";
    strcat(shell_command, font_name);
    FILE *sh = popen(shell_command, "r");
    if (!sh)
    {
        fprintf(stderr, "Failed to load font %s.", font_name);
        return;
    }
    // No safety checks
    fread(font_path, sizeof(unsigned char), 512, sh);
    pclose(sh);
#endif
    LoadFont(font, font_path);
}

// position in pixel where (0,0) is the lower left corner of the screen
void FillText(FontData *font_data, Font *font, MVec2 position, String str, int scale)
{
    // its quite straightforward
    int32_t x = position.x;
    int32_t y = position.y;

    float   tex0, tex1;

    for (uint32_t i = 0; i < str.length; ++i)
    {
        assert(font_data->count + 12 < font_data->max);
        Glyph glyph      = font->character[(size_t)str.data[i]];
        int   w          = glyph.Advance;
        int   h          = font->height;

        tex0             = glyph.offset.x / font->width;
        tex1             = (glyph.offset.x + w) / font->width;

        MVec2 vertices[] = {{x, y},     {tex0, 1.0f}, {x, y + h},     {tex0, 0.0f}, {x + w, y + h}, {tex1, 0.0f},
                            {x + w, y}, {tex1, 1.0f}, {x + w, y + h}, {tex1, 0.0f}, {x, y},         {tex0, 1.0f}};

        memcpy(font_data->data + font_data->count, vertices, sizeof(vertices));

        x                = x + glyph.Advance;
        font_data->count = font_data->count + 12;
    }
}

void RenderFont(Scene *scene, Font *font, Mat4 *scene_transform)
{
    glUseProgram(font->program);
    glUniformMatrix4fv(glGetUniformLocation(font->program, "scene"), 1, GL_TRUE, &scene_transform->elem[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->font_texture);

    assert(scene->axes_labels.count * 2 * 4 < scene->axes_labels.batch->vertex_buffer.max);
    memcpy(scene->axes_labels.batch->vertex_buffer.data, scene->axes_labels.data,
           sizeof(uint8_t) * scene->axes_labels.count * 2 * 4);
    scene->axes_labels.batch->vertex_buffer.count = scene->axes_labels.count * 2 * 4;
    scene->axes_labels.batch->vertex_buffer.dirty = true;

    PrepareBatch(scene->axes_labels.batch);
    DrawBatch(scene->axes_labels.batch, scene->axes_labels.count / (2));
}

void RenderLabels(Scene *scene, Font *font, Graph *graph, Mat4 *combined_matrix)
{
    // These should be fine recalculating per frame

    scene->axes_labels.count = 0;
    float vec[]              = {0.0f, 0.0f, 0.0f, 1.0f};
    MatrixVectorMultiply(combined_matrix, vec);

    MVec2 origin = (MVec2){vec[0], vec[1]};
    MVec2 position;

    // Calculate the min and max vertical bar visible on the current frame first
    int xLow  = -origin.x / graph->slide_scale.x - 1;
    int xHigh = (screen_width - origin.x) / graph->slide_scale.x + 1;

    for (int i = xLow * 2; i <= xHigh * 2; ++i)
    {
        position.x = origin.x + i * graph->slide_scale.x / 2 - font->height / 2;
        position.y = origin.y - font->height;
        // Now calculate the value at the position

        // Clamp the value so that they remain inside the screen
        if (position.y < 0)
            position.y = font->height / 2;
        else if (position.y > screen_height)
            position.y = screen_height - font->height;

        float val   = i * graph->scale.x / 2;
        int   count = snprintf(NULL, 0, "%3g", val);
        snprintf(font->font_buffer, count + 1, "%3g", val);
        String str = {.data = font->font_buffer, .length = count};
        FillText(&scene->axes_labels, font, position, str, 0);
    }

    int yLow  = -origin.y / graph->slide_scale.y - 1;
    int yHigh = (screen_height - origin.y) / graph->slide_scale.y + 1;

    for (int y = yLow * 2; y <= yHigh * 2; ++y)
    {
        if (y == 0)
            continue;
        position.x = origin.x - font->height * 1.5f;
        position.y = origin.y + y * graph->slide_scale.y / 2 - font->height / 2;

        if (position.x < 0)
            position.x = font->height;
        else if (position.x > screen_width)
            position.x = screen_width - font->height;

        float val   = y * graph->scale.y / 2;
        int   count = snprintf(NULL, 0, "%3g", val);
        snprintf(font->font_buffer, count + 1, "%3g", val);
        String str = {.data = font->font_buffer, .length = count};
        FillText(&scene->axes_labels, font, position, str, 0);
    }
}

// void DrawLegends(Scene *scene, Font *font, Graph *graph) // choose location automatically
//{
//     // First draw a appropriate bounding box -> Easily handled by line strips
//     // Draw a small rectangle filled with that color -> Might need a new pixel shader -> Used simple line instead
//     // Finally draw text with plot name -> Already available
//     // Find the appropriate place to put the legend
//     MVec2 pos = {0};
//     pos.x     = screen_width - 200;
//     pos.y     = screen_height - font->height;
//
//     for (int label = 0; label < scene->fonts.count; ++label)
//     {
//         // Add a line indicator with given color for legends
//         AddSingleVertex(scene_group, pos);
//         AddSingleVertex(scene_group, (MVec2){pos.x + 50, pos.y});
//         scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
//         scene_group->graphcolor[scene_group->graphcount - 1] = scene_group->graphcolor[label];
//         FillText(scene_group, font, (MVec2){pos.x + 75, pos.y - font->height / 2},
//                  (String){.data = scene_group->graphname[label], .length = strlen(scene_group->graphname[label])},
//                  0);
//
//         pos.y -= font->height;
//     }
//
//     // Add a graph break for a small box around legends
//     AddSingleVertex(scene_group, (MVec2){pos.x - 10, screen_height});
//     AddSingleVertex(scene_group, (MVec2){pos.x - 10, pos.y - 20});
//     AddSingleVertex(scene_group, (MVec2){screen_width, pos.y - 20});
//
//     assert(scene_group->graphcount < scene_group->cMaxGraph);
//     scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
//     scene_group->graphcolor[scene_group->graphcount - 1] = (MVec3){0.5f, 0.5f, 0.5f};
//     scene_group->graphname[scene_group->graphcount - 1]  = ""; // only static strings are expected as of yet
// }

// void ShowList(RenderScene *scene_group, Font *font, Graph *graph, float *x, float *y, int length,
//               const char *GiveOnlyStaticStrings, MVec3 color)
//{
//     MVec2 vec;
//     for (int points = 0; points < length; ++points)
//     {
//         vec.x = graph->center.x + x[points] * graph->slide_scale.x / (graph->scale.x);
//         vec.y = graph->center.y + y[points] * graph->slide_scale.y / (graph->scale.y);
//         AddSingleVertex(scene_group, vec);
//     }
//
//     assert(scene_group->graphcount < scene_group->cMaxGraph);
//     scene_group->graphbreak[scene_group->graphcount++]   = scene_group->vCount;
//     scene_group->graphcolor[scene_group->graphcount - 1] = color;
//     scene_group->graphname[scene_group->graphcount - 1]  = GiveOnlyStaticStrings;
// }

bool InvokeAndTestFunction(void* function, FunctionType type, MVec2 vec)
{
    switch (type)
    {
        case PARAMETRIC_1D : 
            return fabs( ((ParametricFn1D)function)(vec.x) - vec.y) < 0.04f; 
        case IMPLICIT_2D: 
            return fabs(((ImplicitFn2D)function)(vec.x,vec.y)) < 0.04f; 
    }
    return false; 
}

void HandleEvents(GLFWwindow *window, Scene *scene, State *state, Graph *graph, Mat4 *translate_matrix,
                  Mat4 *scale_matrix)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        switch (state->bPressed)
        {
        case false:
            state->bPressed = true;
            state->xpos     = xpos;
            state->ypos     = ypos;
            break;
        case true:
        {
            double delX       = xpos - state->xpos;
            double delY       = ypos - state->ypos;

            Mat4   translate  = TranslationMatrix(delX, -delY, 0.0f);
            *translate_matrix = MatrixMultiply(&translate, translate_matrix);

            state->xpos       = xpos;
            state->ypos       = ypos;
        }
        }
    }
    else
    {
        state->bPressed = false;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // use the inverse mapping to find the original vector point

        float vec[]             = {xpos, screen_height - ypos, 0.0f, 1.0f};
        Mat4  inverse_translate = TranslationMatrix(-translate_matrix->elem[0][3], -translate_matrix->elem[1][3], 0.0f);
        Mat4  inverse_scale     = ScalarMatrix(1.0f / scale_matrix->elem[0][0], 1.0f / scale_matrix->elem[1][1], 1.0f);
        Mat4  inverse           = MatrixMultiply(&inverse_scale, &inverse_translate);
        MatrixVectorMultiply(&inverse, vec);

        // loop through all the functions
        scene->plots.current_selection = -1;
        for (uint32_t fn = 0; fn < scene->plots.count; ++fn)
        {
            if (InvokeAndTestFunction(scene->plots.functions[fn].function,scene->plots.functions[fn].fn_type,(MVec2){vec[0],vec[1]}))
            {
                scene->plots.current_selection = fn;
                break;
            }
        }
    }
}

// Functions related to API
// To expose to API we need two sets of data .. first the original values and second scaled values

// void APIRecalculate(MorphPlotDevice *device)
//{
//     device->render_scene->vCount = 0;
//     MVec2 vec;
//     for (int re = 0; re < device->render_scene->pCount; ++re)
//     {
//         vec.x = device->graph->center.x +
//                 device->render_scene->Points[re].x * device->graph->slide_scale.x / (device->graph->scale.x);
//         vec.y = device->graph->center.y +
//                 device->render_scene->Points[re].y * device->graph->slide_scale.y / (device->graph->scale.y);
//         AddSingleVertex(device->render_scene, vec);
//     }
// }

Shader LoadShadersFromString(const char *cstr, ShaderType type)
{
    unsigned int shader_define;
    Shader       shader;
    switch (type)
    {
    case FRAGMENT_SHADER:
        shader_define = GL_FRAGMENT_SHADER;
        break;
    case VERTEX_SHADER:
        shader_define = GL_VERTEX_SHADER;
        break;
    case GEOMETRY_SHADER:
        shader_define = GL_GEOMETRY_SHADER;
    default:
        TriggerBreakpoint();
    }
    shader.shader = glCreateShader(shader_define);
    shader.type   = type;
    glShaderSource(shader.shader, 1, (const char *const *)&cstr, NULL);
    glCompileShader(shader.shader);
    int compiled;
    glGetShaderiv(shader.shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader.shader, 512, NULL, infoLog);
        fprintf(stderr, "Failed to compile %s", ShaderTypeName(shader.type));
        fprintf(stderr, "Reason -> %s.", infoLog);
    }
    else
        fprintf(stderr, "\n%s compilation passed.\n", ShaderTypeName(shader.type));
    return shader;
}

MorphPlotDevice MorphCreateDevice()
{
    MorphPlotDevice device;

    device.window = LoadGLFW(screen_width, screen_height, "Morph Graph");

    // Shader vertex =
    //     LoadShadersFromString("#version 330 core \r\nlayout (location = 0) in vec2 aPos; \r\n\r\nuniform mat4 scene;
    //     "
    //                           "\r\n\r\nvoid main() \r\n{\r\n\tgl_Position = scene * vec4(aPos,0.0f,1.0f);\r\n}",
    //                           VERTEX_SHADER);

    // Shader fragment = LoadShadersFromString(
    //     "#version 330 core \r\n\r\nuniform vec3 inColor; \r\nout vec4 color; \r\nvoid main()\r\n{\r\n\t// "
    //     "color = vec4(0.0f,1.0f,0.0f,1.0f);\r\n\tcolor = vec4(inColor,1.0f);\r\n}",
    //     FRAGMENT_SHADER);

    Shader vertex   = LoadShader("./src/shader/aaline.vs", VERTEX_SHADER);
    Shader fragment = LoadShader("./src/shader/aaline.fs", FRAGMENT_SHADER);
    Shader geometry = LoadShader("./src/shader/aaline.gs", GEOMETRY_SHADER);

    device.program  = LoadProgram3(vertex, fragment, geometry);

    // Enable the multi sampling
    glEnable(GL_MULTISAMPLE);

    Scene *scene = malloc(sizeof(*scene));
    Init2DScene(scene);

    Mat4 *ortho_matrix = malloc(sizeof(Mat4));
    if (ortho_matrix)
        *ortho_matrix = OrthographicProjection(0, screen_width, 0, screen_height, -1, 1);

    Mat4 *world_matrix = malloc(sizeof(Mat4));
    if (world_matrix)
        /**world_matrix = TranslationMatrix(400,400,0);*/
        *world_matrix = IdentityMatrix();

    Mat4 *scale_matrix = malloc(sizeof(Mat4));
    if (scale_matrix)
        *scale_matrix = IdentityMatrix();

    Graph *graph = malloc(sizeof(*graph));
    InitGraph(graph);

    UserData *data = malloc(sizeof(*data));
    if (data)
        *data = (UserData){.OrthoMatrix = ortho_matrix, .graph = graph, .scale_transform = scale_matrix};
    glfwSetWindowUserPointer(device.window, data);

    device.panner = malloc(sizeof(*device.panner));
    if (device.panner)
        memset(device.panner, 0, sizeof(*device.panner));

    Font *ComicSans = malloc(sizeof(*ComicSans));
    // LoadFont(ComicSans, "./include/comic.ttf");
    LoadSystemFont(ComicSans, "comic.ttf");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    device.scene           = scene;
    device.graph           = graph;
    device.font            = ComicSans;
    device.transform       = ortho_matrix;
    device.world_transform = world_matrix;
    device.scale_matrix    = scale_matrix;

// Initialize timer here
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    device.timer.count     = counter.QuadPart;
    device.timer.frequency = frequency.QuadPart;
#elif defined(__linux__)
    // using monotonic clock instead of realtime
    struct timespec ts;
    clock_getres(CLOCK_MONOTONIC, &ts);
    device.timer.frequency = (uint64_t)(1000000000ULL / ts.tv_nsec);
    clock_gettime(CLOCK_MONOTONIC, &ts);
    device.timer.count =
        (uint64_t)(ts.tv_sec * device.timer.frequency + ts.tv_nsec * device.timer.frequency / 1000000000ULL);
#endif

    device.should_close = false;
    return device;
}
//
// void MorphPlotFunc(MorphPlotDevice *device, ParametricFn1D fn, MVec3 rgb, float xstart, float xend,
//                   const char *cstronly, float samplesize)
//{
//    float init = -10.0f;
//    float term = 10.0f;
//    float step = 0.05f;
//    if (fabsf(xstart - xend) > FLT_EPSILON)
//    {
//        init = xstart > xend ? xend : xstart;
//        term = xstart > xend ? xstart : xend;
//    }
//    if (samplesize > FLT_EPSILON)
//        step = samplesize;
//    MVec2 vec;
//
//    for (float x = init; x <= term; x += step)
//    {
//        // This API version doesn't check for discontinuity .. Above function checks for discontinuity of one function
//        vec.x = x;
//        vec.y = fn(vec.x);
//        AddSinglePoint(device->render_scene, vec);
//    }
//    // Add number of vertices in the current graph
//    assert(device->render_scene->graphcount < device->render_scene->cMaxGraph);
//    device->render_scene->graphbreak[device->render_scene->graphcount++]   = device->render_scene->pCount;
//    device->render_scene->graphcolor[device->render_scene->graphcount - 1] = rgb;
//    device->render_scene->graphname[device->render_scene->graphcount - 1]  = cstronly;
//}

// void MorphParametric2DPlot(MorphPlotDevice *device, ParametricFn2D fn, float tInit, float tTerm, MVec3 rgb,
//                            const char *cstronly, float step)
//{
//     MVec2 vec;
//     for (float t = tInit; t <= tTerm; t += step)
//         AddSinglePoint(device->render_scene, fn(t));
//     assert(device->render_scene->graphcount < device->render_scene->cMaxGraph);
//     device->render_scene->graphbreak[device->render_scene->graphcount++]   = device->render_scene->pCount;
//     device->render_scene->graphcolor[device->render_scene->graphcount - 1] = rgb;
//     device->render_scene->graphname[device->render_scene->graphcount - 1]  = cstronly;
// }

double MorphTimeSinceCreation(MorphPlotDevice *device)
{
    double elapsed_time = 0;
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    elapsed_time = (((uint64_t)counter.QuadPart - device->timer.count) * 1.0) / device->timer.frequency;
#elif defined(__linux__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t count = ts.tv_sec * device->timer.frequency + ts.tv_nsec * device->timer.frequency / 1000000000ULL;
    elapsed_time   = (((uint64_t)count - device->timer.count) * 1.0) / device->timer.frequency;
#endif

    return elapsed_time;
}
//
// void APIReset(MorphPlotDevice *device, uint32_t hold)
//{
//    device->render_scene->fCount     = 0;
//    device->render_scene->graphcount = hold;
//}

bool MorphShouldWindowClose(MorphPlotDevice *device)
{
    return device->should_close;
}

// void MorphShow(MorphPlotDevice *device)
//{
//     glfwShowWindow(device->window);
//     State    panner = {0};
//
//     uint32_t hold   = device->render_scene->graphcount;
//
//     while (!glfwWindowShouldClose(device->window))
//     {
//         glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT);
//         // Instead of re-rendering, change the scale of the already plotted points
//         APIReset(device, hold);
//         APIRecalculate(device);
//         RenderGraph(device->graph);
//         glUseProgram(device->program);
//         glUniformMatrix4fv(glGetUniformLocation(device->program, "scene"), 1, GL_TRUE,
//         &device->transform->elem[0][0]); glBindVertexArray(device->vao); glBindBuffer(GL_ARRAY_BUFFER, device->vbo);
//
//         RenderLabels(device->render_scene, device->font, device->graph, device->transform);
//         DrawLegends(device->render_scene, device->font, device->graph);
//         RenderRenderScene(device->render_scene, device->program, false);
//         RenderFont(device->render_scene, device->font, device->transform);
//
//         HandleEvents(device->window, &panner, device->graph);
//         glfwSwapBuffers(device->window);
//         glfwPollEvents();
//     }
// }

// void MorphPhantomShow(MorphPlotDevice *device)
//{
//     uint32_t hold = device->render_scene->graphcount;
//
//     glfwShowWindow(device->window);
//     glfwMakeContextCurrent(device->window);
//
//     glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
//     glClear(GL_COLOR_BUFFER_BIT);
//
//     APIReset(device, hold);
//     APIRecalculate(device);
//     RenderGraph(device->graph);
//
//     glUseProgram(device->program);
//     glUniformMatrix4fv(glGetUniformLocation(device->program, "scene"), 1, GL_TRUE, &device->transform->elem[0][0]);
//     glBindVertexArray(device->vao);
//     glBindBuffer(GL_ARRAY_BUFFER, device->vbo);
//
//     RenderLabels(device->render_scene, device->font, device->graph, device->transform);
//     DrawLegends(device->render_scene, device->font, device->graph);
//     RenderRenderScene(device->render_scene, device->program, false);
//     RenderFont(device->render_scene, device->font, device->transform);
//
//     glfwSwapBuffers(device->window);
//
//     HandleEvents(device->window, device->panner, device->graph);
//     glfwPollEvents();
//
//     device->render_scene->graphcount -= hold + 1;
//     device->should_close = glfwWindowShouldClose(device->window);
// }

void MorphDestroyDevice(MorphPlotDevice *device)
{
    Destroy2DScene(device->scene);
    free(device->scene);
    UserData *data = glfwGetWindowUserPointer(device->window);
    free(data);
    free(device->transform);
    free(device->graph);
    free(device->font);
    glfwDestroyWindow(device->window);
    glfwTerminate();
}

// void MorphPlotList(MorphPlotDevice *device, float *x, float *y, int length, MVec3 rgb, const char *cstronly)
//{
//     for (int points = 0; points < length; ++points)
//         AddSinglePoint(device->render_scene, (MVec2){x[points], y[points]});
//
//     assert(device->render_scene->graphcount < device->render_scene->cMaxGraph);
//     device->render_scene->graphbreak[device->render_scene->graphcount++]   = device->render_scene->pCount;
//     device->render_scene->graphcolor[device->render_scene->graphcount - 1] = rgb;
//     device->render_scene->graphname[device->render_scene->graphcount - 1]  = cstronly;
// }

// void MorphResetPlotting(MorphPlotDevice *device)
//{
//     ResetRenderScene(device->render_scene);
// }

double ImplicitCircle(double x, double y)
{
    return x * x + y * y - 4; // Implicit circle of radius 2
}

double ImplicitEllipse(double x, double y)
{
    return 2 * x * x + 5 * y * y - 40; 
}

double ImplicitHyperbola(double x, double y)
{
    return x * x - y * y  - 1; 
}

double PartialDerivativeX(ImplicitFn2D fn, double x, double y, double h)
{
    return (fn(x + h, y) - fn(x, y)) / h;
}

double PartialDerivativeY(ImplicitFn2D fn, double x, double y, double h)
{
    return (fn(x, y + h) - fn(x, y)) / h;
}

MVec2 Gradient2D(ImplicitFn2D fn, double x, double y, double h)
{
    return (MVec2){PartialDerivativeX(fn, x, y, h), PartialDerivativeY(fn, x, y, h)};
}

MVec2 ContourDirection(ImplicitFn2D fn, double x, double y, double h)
{
    MVec2 grad = Gradient2D(fn, x, y, h);
    float norm = sqrtf(grad.x * grad.x + grad.y * grad.y);
    return (MVec2){.x = -grad.y / norm, .y = grad.x / norm};
}

// Takes functions of the form f(x,y) - c to plot f(x,y) = c 

void ImplicitFunctionPlot2D(MorphPlotDevice *device, ImplicitFn2D fn)
{
    Scene         *scene                               = device->scene;

    const uint32_t max_verts                           = 5000;

    // First determine a point in the function using any method
    // My approach :
    // Start at the origin and move along the partial derivatives to reach to the initial position on the surface

    MVec2        vec   = {0, 0}; // start at the origin and land on the contour
    float        hstep = 0.025f, kstep = 0.025f;

    float        h            = 0.0005;

    uint32_t     sample_count = 0;
    VertexData2D vertex;


    uint32_t     counter      = 0;

    uint32_t    plots = 4;
    int32_t     dir[] = {1, -1,1,-1}; 
    float        origin_offsets[]    = {0.1f, 0.1f, -0.1f, -0.1f};

    while (plots--)
    {
        // To reach the contour line, we must travel along the partial derivatives first and use Newton Raphson method
        // to find the point of contour Taking y constant for now and moving in the direction of partial derivative
        scene->plots.functions[scene->plots.count].max     = max_verts; // 1000 vertices for each graph at most
        scene->plots.functions[scene->plots.count].samples = malloc(sizeof(MVec2) * max_verts);

        FunctionPlotData *function                         = &scene->plots.functions[scene->plots.count];
        function->fn_type                                  = IMPLICIT_2D;
        function->color                                    = (MVec3){1.0f, 0.5f, 0.6f};
        function->function                                 = fn;

        vec.x                                              = origin_offsets[plots]; 
        vec.y                                              = 0.0f; 

        int32_t max_movement                               = 15000;
        while (fabs(fn(vec.x, vec.y)) > 0.0025f)
        {
            // vec.y = vec.y - fn(vec.x, vec.y) / PartialDerivativeY(fn, vec.x, vec.y, hstep);
            vec.x = vec.x - fn(vec.x, vec.y) / PartialDerivativeX(fn, vec.x, vec.y, hstep);
        }

        vertex.x                             = vec.x;
        vertex.y                             = vec.y;

        function->samples[function->count++] = vertex;

        // Now move along the contour of the implicit function.
        // Calculate partial derivative along X and Y direction.
        // Gradient of the function

        while (max_movement--)
        {
            // Calculate delX and delY such that they remains in the contour
            // dy = -hstep * PartialDerivativeX(fn, vec.x, vec.y, h) / PartialDerivativeY(fn, vec.x + hstep, vec.y, h);

            // If the path can't remain in the contour, then dx will be ??
            // dx = hstep; // -hstep * PartialDerivativeY(fn, vec.x, vec.y, h) / PartialDerivativeX(fn, vec.x, vec.y +
            // kstep, h); else
            // {
            //     x_inc =
            //         -hstep * PartialDerivativeY(fn, vec.x, vec.y, h) / PartialDerivativeX(fn, vec.x, vec.y + kstep,
            //         h);

            // }
            // Trace back to the contour after the increment
            // Move toward that and again trace back to the contour
            // Lets apply concept from the higher dimensional vector calculus

            MVec2 tangent = ContourDirection(fn, vec.x, vec.y, h);
            float dx      = dir[plots] * tangent.x * h * 2.5f;
            float dy      = dir[plots] * tangent.y * h * 2.5f;

            // once this is finished move toward the negative contour 
            vec.x         = vec.x + dx;
            vec.y         = vec.y + dy;

            // while (fabs(fn(vec.x, vec.y)) > FLT_EPSILON)
            //{
            //     vec.y = vec.y - fn(vec.x, vec.y) / PartialDerivativeY(fn, vec.x, vec.y, hstep);
            // }
            if (counter++ % 100 == 0)
            {
                assert(function->count < function->max);
                vertex.x                                   = vec.x;
                vertex.y                                   = vec.y;
                function->samples[function->count - 1].n_x = vec.x - function->samples[function->count - 1].x;
                function->samples[function->count - 1].n_y = vec.y - function->samples[function->count - 1].y;
                function->samples[function->count++]       = vertex;
            }
        }
        function->color   = (MVec3){1.0f, 0.0f, 1.0f};
        function->batch   = CreateNewBatch(LINE_STRIP);
        function->updated = true;
        scene->plots.count++;
    }

}

double Square(double x)
{
    return x * x;
}

#include <time.h>

int main(int argc, char **argv)
{

    MorphPlotDevice device = MorphCreateDevice();
    glfwShowWindow(device.window);
    glfwMakeContextCurrent(device.window);

    // ImplicitFunctionPlot2D(&device, ImplicitCircle);
    ImplicitFunctionPlot2D(&device,ImplicitEllipse); 

    if (true)
    {
        Plot1D(device.scene, GaussianIntegral, device.graph, (MVec3){0.1f, 0.1f, 0.75f}, "Nothing");
        Plot1D(device.scene, Square, device.graph, (MVec3){0.4f, 0.4f, 0.1f}, "Squared");
        Plot1D(device.scene, lin, device.graph, (MVec3){0.9f, 0.1f, 0.1f}, "Squared");
        

        clock_t  now = clock(), then = clock();
        uint32_t count = 0;

        Mat4     matrix;
        while (!glfwWindowShouldClose(device.window))
        {
            if (count >= 60)
            {
                now   = clock();
                count = count - 60;
                fprintf(stderr, "Approximate fps : %.5g.\n", 60.0 / ((now - then) / (double)CLOCKS_PER_SEC));
                then = now;
            }
            count++;
            matrix = MatrixMultiply(device.world_transform, device.scale_matrix);

            RenderGraph(device.graph, device.world_transform);
            RenderScene(device.scene, device.program, false, device.transform, &matrix);
            RenderLabels(device.scene, device.font, device.graph, &matrix);
            RenderFont(device.scene, device.font, device.transform);

            HandleEvents(device.window, device.scene, device.panner, device.graph, device.world_transform,
                         device.scale_matrix);

            device.scene->axes_labels.count = 0;
            glfwSwapBuffers(device.window);
            glfwPollEvents();
        }

        MorphDestroyDevice(&device);
    }
    return 0;
}