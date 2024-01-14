#ifndef __TSPRITE_H__
#define __TSPRITE_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CHANNELS     4                               // r, g, b, a
#define SIZE_RESTORE 6                               // e[ue[B
#define SIZE_SAVE    3                               // e[s
#define SIZE_COL     19                              // e[38;2;111;222;333m
#define SIZE_NULL    1                               // '\0'
#define SIZE_NXLINE_SAVE  (SIZE_RESTORE + SIZE_SAVE) // e[ue[Be[s
#define SIZE_NXLINE_CURSOR 6                         // e[De[B

#define RETURN_ERR(A) {\
    enum tsprite_error err = A;\
    if (err != TSPRITE_ERROR_NONE) {\
        free(new);\
        stbi_image_free(img);\
        return err;\
    }\
}

typedef enum tsprite_error {
    TSPRITE_ERROR_NONE,
    TSPRITE_ERROR_IMAGE,
    TSPRITE_ERROR_MEMORY,
    TSPRITE_ERROR_COORD,
    TSPRITE_ERROR_FILE,
} tsprite_error;

typedef enum tsprite_blend {
    TSPRITE_BLEND_NONE,
    TSPRITE_BLEND_X,
    TSPRITE_BLEND_Y,
    TSPRITE_BLEND_XY,
} tsprite_blend;

typedef enum tsprite_disp {
    TSPRITE_DISP_BKG,
    TSPRITE_DISP_CHAR
} tsprite_disp;

typedef struct tsprite_col {
    int r, g, b, a;
} tsprite_col;

typedef struct tsprite_rect {
    int x, y, w, h;
} tsprite_rect;

typedef struct tsprite_param {
    int scale_x, scale_y;
    struct tsprite_rect* area;

    int range;
    enum tsprite_blend blend;
    void (*filter)(struct tsprite_col*);

    int str_len;
    char const** str_set;
    char const* (*str_func)(struct tsprite_col, int, char const**);
    
    struct tsprite_col* alpha_rep;
    
    bool use_cursor_seq;
    enum tsprite_disp disp;
    int col_threshold;

    char const** ret_str;
    char const* ret_file;
    bool append_file;

    // internal
    int x_len;
    int seq_size; 
} tsprite_param;

enum tsprite_error
tsprite_get_sequence(char const* file, struct tsprite_param* param);

static inline struct tsprite_col
bind_col(struct tsprite_col col);

static inline bool
col_within_threshold(int threshold, struct tsprite_col prev, struct tsprite_col col);

static enum tsprite_error
write_sequence(unsigned char* img, char* loop, struct tsprite_param* param, int x, int y, struct tsprite_rect* area);

static enum tsprite_error
read_img(char const* file, unsigned char** img, int* x, int* y);

static enum tsprite_error
alloc_seq(struct tsprite_param* param, int x, int y, char** new);

static enum tsprite_error
write_file(char const* ret, char* new, bool append_file);

static struct tsprite_col
get_col(unsigned char* img, struct tsprite_param* param, int x, int y, int i, int j);

static void
blend_col(unsigned char* img, int x, int y, int bound, int root, int i, int j, tsprite_col* col, int range, enum tsprite_blend blend);

static inline unsigned char*
get_data(unsigned char* img, int x, int i, int j);

static void
set_default_param(struct tsprite_param* param);

char const*
char_bright(struct tsprite_col col, int str_len, char const** str_set);

char const*
char_first(struct tsprite_col col, int str_len, char const** str_set);

char const*
char_rand(struct tsprite_col col, int str_len, char const** str_set);

void
filter_none(struct tsprite_col* col);

void
filter_sepia(struct tsprite_col* col);

void
filter_greyscale(struct tsprite_col* col);

void
filter_invert(struct tsprite_col* col);

enum tsprite_error
tsprite_get_sequence(char const* file, struct tsprite_param* param)
{
    if (!param) {
        param = &(struct tsprite_param) { 0 };
    }
    set_default_param(param);

    int x, y;
    unsigned char* img;
    char* new = NULL;
    RETURN_ERR(read_img(file, &img, &x, &y))
    RETURN_ERR(alloc_seq(param, x, y, &new));
    
    char* start = param->use_cursor_seq ? new : new + SIZE_SAVE; // no save seq needed first if cursor param
    if (param->area) {
        if (
            param->area->x > x || param->area->y > y ||
            (param->area->x + param->area->w) > x ||
            (param->area->y + param->area->h) > y
        ) {
               return TSPRITE_ERROR_COORD;
        }
        write_sequence(img, start, param, x, y, param->area);
    } else {
        write_sequence(img, start, param, x, y, &(struct tsprite_rect) { 0, 0, x, y });
    }

    if (param->ret_file) {
        RETURN_ERR(write_file(param->ret_file, new, param->append_file));
    }

    if (param->ret_str) {
        *param->ret_str = new;
    } else {
        free(new);
    }
    stbi_image_free(img);
    
    return TSPRITE_ERROR_NONE;
}

static enum tsprite_error
write_sequence(unsigned char* img, char* loop, struct tsprite_param* param, int x, int y, struct tsprite_rect* area)
{
    struct tsprite_col prev = { -1 };
    for (int i = area->y; i < area->h + area->y; i += param->scale_y, loop += param->seq_size) {
        int alpha = 0;
        for (int j = area->x; j < area->w + area->x; j += param->scale_x) {
            
            bool skip = false;
            struct tsprite_col col = get_col(img, param, x, y, i, j);
            if (col.a == 0) {
                if (param->alpha_rep) {
                    col = *(param->alpha_rep);
                } else {
                    alpha++;
                    skip = true;
                }
            }
            if (!skip) {
                if (alpha) { // create next cursor seq once alpha stops
                    loop += sprintf(loop, "\x1b[%dC", alpha);
                    alpha = 0;
                }
                if (prev.r != -1 && col_within_threshold(param->col_threshold, prev, col)) {
                    loop += sprintf(loop, "%s", param->str_func(prev, param->str_len, param->str_set));
                } else {
                    loop += sprintf(
                                        loop,
                                        "\x1b[%d;2;%d;%d;%dm%s",
                                        param->disp == TSPRITE_DISP_BKG ? 48 : 38, 
                                        col.r, col.g, col.b, param->str_func(col, param->str_len, param->str_set)
                                    );
                    prev.r = col.r, prev.g = col.g, prev.b = col.b;
                }
            }
        }
        if (param->use_cursor_seq) {
            sprintf(loop, "\x1b[%dD\x1b[B", param->x_len);
        } else {
            memcpy(loop, "\x1b[u\x1b[B\x1b[s", param->seq_size);
        }
        
    }
    return TSPRITE_ERROR_NONE;
}

static void
set_default_param(struct tsprite_param* param)
{
    static char* default_str_set[] = { " " };
    
    if (param->str_len == 0) {
        param->str_len = 1;
    }
    if (!param->str_set) {
        param->str_set = default_str_set;
    }
    if (!param->str_func) {
        param->str_func = char_first;
    }
    if (!param->filter) {
        param->filter = filter_none;
    }
    if (param->scale_x <= 0) {
        param->scale_x = 1;
    }
    if (param->scale_y <= 0) {
        param->scale_y = 1;
    }
}

static inline bool
col_within_threshold(int threshold, struct tsprite_col prev, struct tsprite_col col) {
    return (prev.r <= col.r + threshold && prev.r >= col.r - threshold) &&
           (prev.g <= col.g + threshold && prev.g >= col.g - threshold) &&
           (prev.b <= col.b + threshold && prev.b >= col.b - threshold);
}

static struct tsprite_col
get_col(unsigned char* img, struct tsprite_param* param, int x, int y, int i, int j)
{
    struct tsprite_col col = { 0 };
    int div = param->range * 2 + 1; // both sides including itself
    
    switch(param->blend) {
        case TSPRITE_BLEND_NONE:
            unsigned char* data = get_data(img, x, i, j);
            col.r = data[0], col.g = data[1], col.b = data[2], col.a = data[3];
            break;
        case TSPRITE_BLEND_X:
            blend_col(img, x, y, x, j, i, j, &col, param->range, TSPRITE_BLEND_X);
            col.r /= div, col.g /= div, col.b /= div;
            break;
        case TSPRITE_BLEND_Y:
            blend_col(img, x, y, y, i, i, j, &col, param->range, TSPRITE_BLEND_Y);
            col.r /= div, col.g /= div, col.b /= div;
            break;
        case TSPRITE_BLEND_XY:
            blend_col(img, x, y, x, j, i, j, &col, param->range, TSPRITE_BLEND_X);
            blend_col(img, x, y, y, i, i, j, &col, param->range, TSPRITE_BLEND_Y);
            col.r /= div * 2, col.g /= div * 2, col.b /= div * 2;
            break;
    }
    
    param->filter(&col);
    return bind_col(col);
}

static void
blend_col(unsigned char* img, int x, int y, int bound, int root, int i, int j, struct tsprite_col* col, int range, enum tsprite_blend blend)
{
    unsigned char* alpha = get_data(img, x, i, j);
    if (alpha[3] == 0) {
        return;
    }
    
    col->a = alpha[3];
    int min = root - range;
    int max = root + range;

    if (min < 0) { // edge case for negative indice, use current col instead
        unsigned char* first = blend == TSPRITE_BLEND_X ? get_data(img, x, i, 0) :
                                                          get_data(img, x, 0, j);
        col->r += first[0] * -min, col->g += first[1] * -min, col->b += first[2] * -min;
        min = 0;
    }
    if (max >= bound) { // edge case for too big indice, use current col instead
        unsigned char* last = blend == TSPRITE_BLEND_X ? get_data(img, x, i, x - 1) :
                                                         get_data(img, x, y - 1, j);
        col->r += last[0] * (max - bound), col->g += last[1] * (max - bound), col->b += last[2] * (max - bound);
        max = bound - 1;
    }

    for (int a = min; a <= max; a++) { // blend remaining
        unsigned char* data = blend == TSPRITE_BLEND_X ? get_data(img, x, i, a) :
                                                         get_data(img, x, a, j);
        col->r += data[0], col->g += data[1], col->b += data[2];
    }
    
}

static inline struct tsprite_col
bind_col(struct tsprite_col col)
{
    return (struct tsprite_col) {
        col.r > 255 ? 255 : col.r < 0 ? 0 : col.r,
        col.g > 255 ? 255 : col.g < 0 ? 0 : col.g,
        col.b > 255 ? 255 : col.b < 0 ? 0 : col.b,
        col.a > 255 ? 255 : col.a < 0 ? 0 : col.a
    };
}

static inline unsigned char*
get_data(unsigned char* img, int x, int i, int j)
{
    return img + CHANNELS * (i * x + j);
}

static enum tsprite_error
read_img(char const* file, unsigned char** img, int* x, int* y)
{
    *img = stbi_load(file, x, y, NULL, CHANNELS);
    return *img ? TSPRITE_ERROR_NONE : TSPRITE_ERROR_IMAGE;
}

static enum tsprite_error
alloc_seq(struct tsprite_param* param, int x, int y, char** new)
{
    if (param->area) {
        x = param->area->w / param->scale_x, y = param->area->h / param->scale_y;
    } else {
        x = x / param->scale_x, y = y / param->scale_y;
    }

    int gtr_strlen = 0;
    for (int i = 0; i < param->str_len; i++) {
        if (strlen(param->str_set[i]) > gtr_strlen) {
            gtr_strlen = strlen(param->str_set[i]);
        }
    }

    param->x_len = x;
    // formula gets the number digits in length for back sequence
    param->seq_size = param->use_cursor_seq ? (int) (floor(log10(x)) + 1) + SIZE_NXLINE_CURSOR : SIZE_NXLINE_SAVE;   

    *new = calloc(
        ((SIZE_COL + gtr_strlen) * x * y) + (y * param->seq_size) + (param->use_cursor_seq * SIZE_SAVE) + SIZE_NULL,
        sizeof(char)
    );
    if (!*new) {
        return TSPRITE_ERROR_MEMORY;
    }

    if (!param->use_cursor_seq) {
        memcpy(*new, "\x1b[s", SIZE_SAVE);
    }
    return TSPRITE_ERROR_NONE;
}

static enum tsprite_error
write_file(char const* ret, char* new, bool append_file)
{
    FILE* writ = fopen(ret, append_file ? "a" : "w");
    if (!writ) {
        return TSPRITE_ERROR_FILE;
    }

    fprintf(writ, "%s\n", new);
    fclose(writ);

    return TSPRITE_ERROR_NONE;
}

char const*
char_bright(struct tsprite_col col, int str_len, char const** str_set)
{
    int bright = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
    return str_set[(int) roundf(((str_len - 1) / 255.0) * bright)];
}

char const*
char_first(struct tsprite_col col, int str_len, char const** str_set)
{
    return str_set[0];
}

char const*
char_rand(struct tsprite_col col, int str_len, char const** str_set)
{
    return str_set[rand() % str_len];
}

void
filter_none(struct tsprite_col* col)
{
    // dummy function
}

void
filter_sepia(struct tsprite_col* col)
{
    col->r = (col->r * 0.393) + (col->g * 0.769) + (col->b * 0.189);
    col->g = (col->r * 0.349) + (col->g * 0.686) + (col->b * 0.168);
    col->b = (col->r * 0.272) + (col->g * 0.534) + (col->b * 0.131);
}

void
filter_greyscale(struct tsprite_col* col)
{
    col->r = col->g = col->b = col->r * 0.3 + col->g * 0.59 + col->b * 0.11;
}

void
filter_invert(struct tsprite_col* col)
{
    col->r = (255 - col->r);
    col->g = (255 - col->g);
    col->b = (255 - col->b);
}

#endif
