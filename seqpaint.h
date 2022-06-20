#ifndef SEQPAINT_INCLUDE
#define SEQPAINT_INCLUDE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CHANNELS     4                          // R, G, B
#define SIZE_RESTORE 6                          // e[ue[B
#define SIZE_SAVE    3                          // e[s
#define SIZE_COL     23                         // e[38;2;111;222;333mc
#define SIZE_NULL    1                          // '\0'
#define SIZE_NXLINE  (SIZE_RESTORE + SIZE_SAVE) // e[ue[Be[s

#define RETURN_ERR(A) {\
    enum SeqPaintError err = A;\
    if (err != SEQPAINT_ERROR_NONE) {\
        free(new);\
        stbi_image_free(img);\
        return err;\
    }\
}

enum SeqPaintError {
    SEQPAINT_ERROR_NONE,
    SEQPAINT_ERROR_IMAGE,
    SEQPAINT_ERROR_MEMORY,
    SEQPAINT_ERROR_COORD,
    SEQPAINT_ERROR_FILE
};

enum SeqPaintBlend {
    SEQPAINT_BLEND_NONE,
    SEQPAINT_BLEND_X,
    SEQPAINT_BLEND_Y,
    SEQPAINT_BLEND_XY
};

enum SeqPaintFilter {
    SEQPAINT_FILTER_NONE,
    SEQPAINT_FILTER_GRAYSCALE,
    SEQPAINT_FILTER_INVERT,
    SEQPAINT_FILTER_SEPIA,
    SEQPAINT_FILTER_CUSTOM
};

enum SeqPaintDisp {
    SEQPAINT_DISP_BKG,
    SEQPAINT_DISP_CHAR
};

struct SeqPaintCol {
    int r, g, b, a;
};

struct SeqPaintRect {
    int x, y, w, h;
};

struct SeqPaintParam {
    int scaleX, scaleY;
    
    enum SeqPaintFilter filter;
    
    int charLen;
    char** charSet;
    char* (*charFunc)(struct SeqPaintCol, int, char**);
    
    enum SeqPaintDisp disp;
    
    int range;
    enum SeqPaintBlend blend;
    
    struct SeqPaintRect* area;
    
    struct SeqPaintCol* alphaRep;
    
    bool noConsole;
    bool appendFile;
};

enum SeqPaintError
getSequence(char const* file, char const* ret, struct SeqPaintParam* param);

static inline struct SeqPaintCol
bindCol(struct SeqPaintCol col);

static enum SeqPaintError
writeSequence(unsigned char* img, char* loop, struct SeqPaintParam* param, int x, int y, int rootX, int lenX, int rootY, int lenY);

static enum SeqPaintError
readImg(char const* file, unsigned char** img, int* x, int* y);

static enum SeqPaintError
allocSeq(struct SeqPaintParam* param, int x, int y, char** new);

static enum SeqPaintError
writeFile(char const* ret, char* new, bool appendFile);

static struct SeqPaintCol
getCol(unsigned char* img, struct SeqPaintParam* param, int x, int y, int i, int j);

static void
blendCol(unsigned char* img, int x, int y, int bound, int root, int i, int j, struct SeqPaintCol* col, int range, enum SeqPaintBlend blend);

static inline unsigned char*
getData(unsigned char* img, int x, int i, int j);

static void
filterCol(struct SeqPaintCol* col, enum SeqPaintFilter filter);

static void
paramDefault(struct SeqPaintParam* param);

char*
charBright(struct SeqPaintCol col, int charLen, char** charSet);

char*
charDefault(struct SeqPaintCol col, int charLen, char** charSet);

char*
charRand(struct SeqPaintCol col, int charLen, char** charSet);

static void
paramDefault(struct SeqPaintParam* param)
{
    static char* charSet[] = {" "};
    
    if (param->charLen == 0) {
        param->charLen = 1;
    }
    if (!param->charSet) {
        param->charSet = charSet;
    }
    if (!param->charFunc) {
        param->charFunc = charDefault;
    }
    if (param->scaleX <= 0) {
        param->scaleX = 1;
    }
    if (param->scaleY <= 0) {
        param->scaleY = 1;
    }
}

enum SeqPaintError
getSequence(char const* file, char const* ret, struct SeqPaintParam* param)
{
    if (!param) {
        param = &(struct SeqPaintParam) {0};
    }
    paramDefault(param);

    int x, y;
    unsigned char* img;
    char* new = NULL;
    RETURN_ERR(readImg(file, &img, &x, &y))
    RETURN_ERR(allocSeq(param, x, y, &new));
    
    if (param->area) {
        if (param->area->x > x || param->area->y > y ||
           (param->area->x + param->area->w) > x ||
           (param->area->y + param->area->h) > y) {
               return SEQPAINT_ERROR_COORD;
        }
        writeSequence(img, new + SIZE_SAVE, param, x, y, param->area->x, param->area->w, param->area->y, param->area->h);
    } else {
        writeSequence(img, new + SIZE_SAVE, param, x, y, 0, x, 0, y);
    }
    
    RETURN_ERR(writeFile(ret, new, param->appendFile));
    
    if (!param->noConsole) {
        printf("\x1b[1;1H%s\x1b[0m", new);
    }
    
    free(new);
    stbi_image_free(img);
    
    return SEQPAINT_ERROR_NONE;
}

static enum SeqPaintError
writeSequence(unsigned char* img, char* loop, struct SeqPaintParam* param, int x, int y, int rootX, int lenX, int rootY, int lenY)
{
    struct SeqPaintCol prev = {-1, -1, -1};
    for (int i = rootY; i < lenY; i += param->scaleY, loop += SIZE_NXLINE) {
        int alpha = 0;
        for (int j = rootX; j < lenX; j += param->scaleX) {
            
            bool skip = false;
            struct SeqPaintCol col = getCol(img, param, x, y, i, j);
            if (col.a == 0) {
                if (param->alphaRep) {
                    col = *(param->alphaRep);
                } else {
                    alpha++;
                    skip = true;
                }
            }
            if (!skip) {
                if (alpha) {
                    loop += sprintf(loop, "\x1b[%dC", alpha);
                    alpha = 0;
                }
                if (prev.r == col.r && prev.g == col.g && prev.b == col.b) {
                    loop += sprintf(loop, "%s", param->charFunc(prev, param->charLen, param->charSet));
                } else {
                    loop += sprintf(
                                        loop,
                                        "\x1b[%d;2;%d;%d;%dm%s",
                                        param->disp == SEQPAINT_DISP_BKG ? 48 : 38, 
                                        col.r, col.g, col.b, param->charFunc(col, param->charLen, param->charSet)
                                    );
                    prev.r = col.r, prev.g = col.g, prev.b = col.b;
                }
            }
        }
        strcpy(loop, "\x1b[u\x1b[B\x1b[s");
    }
    
    return SEQPAINT_ERROR_NONE;
}

char*
charBright(struct SeqPaintCol col, int charLen, char** charSet)
{
    int bright = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
    return charSet[(int) roundf(((charLen - 1) / 255.0) * bright)];
}

char*
charDefault(struct SeqPaintCol col, int charLen, char** charSet)
{
    return charSet[0];
}

char*
charRand(struct SeqPaintCol col, int charLen, char** charSet)
{
    return charSet[rand() % charLen];
}

static struct SeqPaintCol
getCol(unsigned char* img, struct SeqPaintParam* param, int x, int y, int i, int j)
{
    struct SeqPaintCol col = {0};
    int div = param->range * 2 + 1;
    
    switch(param->blend) {
        case SEQPAINT_BLEND_NONE:
            unsigned char* data = getData(img, x, i, j);
            col.r = data[0], col.g = data[1], col.b = data[2], col.a = data[3];
            break;
        case SEQPAINT_BLEND_X:
            blendCol(img, x, y, x, j, i, j, &col, param->range, SEQPAINT_BLEND_X);
            col.r /= div, col.g /= div, col.b /= div;
            break;
        case SEQPAINT_BLEND_Y:
            blendCol(img, x, y, y, i, i, j, &col, param->range, SEQPAINT_BLEND_Y);
            col.r /= div, col.g /= div, col.b /= div;
            break;
        case SEQPAINT_BLEND_XY:
            blendCol(img, x, y, x, j, i, j, &col, param->range, SEQPAINT_BLEND_X);
            blendCol(img, x, y, y, i, i, j, &col, param->range, SEQPAINT_BLEND_Y);
            col.r /= div * 2, col.g /= div * 2, col.b /= div * 2;
            break;
    }
    
    filterCol(&col, param->filter);
    return bindCol(col);
}

static void
blendCol(unsigned char* img, int x, int y, int bound, int root, int i, int j, struct SeqPaintCol* col, int range, enum SeqPaintBlend blend)
{
    unsigned char* alpha = getData(img, x, i, j);
    if (alpha[3] == 0) {
        return;
    }
    
    col->a = alpha[3];
    int min = root - range;
    int max = root + range;

    if (min < 0) {
        unsigned char* first = blend == SEQPAINT_BLEND_X ? getData(img, x, i, 0) :
                                                           getData(img, x, 0, j);
        col->r += first[0] * -min, col->g += first[1] * -min, col->b += first[2] * -min;
        min = 0;
    }
    if (max >= bound) {
        unsigned char* last = blend == SEQPAINT_BLEND_X ? getData(img, x, i, x - 1) :
                                                          getData(img, x, y - 1, j);
        col->r += last[0] * (max - bound), col->g += last[1] * (max - bound), col->b += last[2] * (max - bound);
        max = bound - 1;
    }

    for (int a = min; a <= max; a++) {
        unsigned char* data = blend == SEQPAINT_BLEND_X ? getData(img, x, i, a) :
                                                          getData(img, x, a, j);
        col->r += data[0], col->g += data[1], col->b += data[2];
    }
    
}

static void
filterCol(struct SeqPaintCol* col, enum SeqPaintFilter filter)
{
    switch(filter) {
        case SEQPAINT_FILTER_SEPIA:
            col->r = (col->r * 0.393) + (col->g * 0.769) + (col->b * 0.189);
            col->g = (col->r * 0.349) + (col->g * 0.686) + (col->b * 0.168);
            col->b = (col->r * 0.272) + (col->g * 0.534) + (col->b * 0.131);
            break;
        case SEQPAINT_FILTER_GRAYSCALE:
            col->r = col->g = col->b = col->r * 0.3 + col->g * 0.59 + col->b * 0.11;
            break;
        case SEQPAINT_FILTER_INVERT:
            col->r = (255 - col->r);
            col->g = (255 - col->g);
            col->b = (255 - col->b);
            break;
        #ifdef SEQPAINT_FILTER_EXPR
        case SEQPAINT_FILTER_CUSTOM:
            SEQPAINT_FILTER_EXPR;
            break;
        #endif
    }
}

static inline unsigned char*
getData(unsigned char* img, int x, int i, int j)
{
    return img + CHANNELS * (i * x + j);
}

static inline struct SeqPaintCol
bindCol(struct SeqPaintCol col)
{
    return (struct SeqPaintCol) {
        col.r > 255 ? 255 : col.r < 0 ? 0 : col.r,
        col.g > 255 ? 255 : col.g < 0 ? 0 : col.g,
        col.b > 255 ? 255 : col.b < 0 ? 0 : col.b,
        col.a
    };
}

static enum SeqPaintError
readImg(char const* const file, unsigned char** img, int* x, int* y)
{
    *img = stbi_load(file, x, y, NULL, CHANNELS);
    return *img ? SEQPAINT_ERROR_NONE : SEQPAINT_ERROR_IMAGE;
}

static enum SeqPaintError
allocSeq(struct SeqPaintParam* param, int x, int y, char** new)
{
    x = x / param->scaleX, y = y / param->scaleY;
    *new = malloc((SIZE_COL * x * y) + (y * SIZE_NXLINE) + SIZE_SAVE + SIZE_NULL);
    if (!*new) {
        return SEQPAINT_ERROR_MEMORY;
    }
    strcpy(*new, "\x1b[s");
    return SEQPAINT_ERROR_NONE;
}

static enum SeqPaintError
writeFile(char const* ret, char* new, bool appendFile)
{
    FILE* writ = fopen(ret, appendFile ? "a" : "w");
    if (!writ) {
        return SEQPAINT_ERROR_FILE;
    }
    
    fprintf(writ, "%s\n", new);
    
    fclose(writ);
    return SEQPAINT_ERROR_NONE;
}

#endif
