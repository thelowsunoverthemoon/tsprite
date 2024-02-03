#ifndef __TSPRITE_H__
#define __TSPRITE_H__

#include <stdbool.h>

// Begin Header File

typedef enum tsprite_error {
    TSPRITE_ERROR_NONE,
    TSPRITE_ERROR_IMAGE,
    TSPRITE_ERROR_RESIZE,
    TSPRITE_ERROR_MEMORY,
    TSPRITE_ERROR_COORD,
    TSPRITE_ERROR_FILE
} tsprite_error;

typedef enum tsprite_equiv_type {
    TSPRITE_EQUIV_NONE,
    TSPRITE_EQUIV_ALL,
    TSPRITE_EQUIV_THRESHOLD
} tsprite_equiv_type;

typedef enum tsprite_disp {
    TSPRITE_DISP_BKG = 48,
    TSPRITE_DISP_CHAR = 38
} tsprite_disp;

typedef struct tsprite_col {
    int r, g, b, a;
} tsprite_col;

typedef struct tsprite_rect {
    int x, y, w, h;
} tsprite_rect;

typedef struct tsprite_param {
    int adj_x, adj_y;
    struct tsprite_rect* area;

    void (*filter)(struct tsprite_col*);

    int str_len;
    char const** str_set;
    char const* (*str_func)(struct tsprite_col, int, char const**);
    
    bool use_cursor_seq;
    enum tsprite_disp disp;
    double col_threshold;

    char const** ret_str;
    char const* ret_file;
    bool append_file;

    double equiv_threshold;
    enum tsprite_equiv_type equiv_type;

    // internal
    struct tsprite_col equiv_col;
    int equiv_seq;
    int x;
} tsprite_param;

enum tsprite_error
tsprite_get_sequence(char const* file, struct tsprite_param* param);

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

// Begin Implementation

#ifdef TSPRITE_IMPLEMENTATION

#include <stdlib.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

// All sequence info from https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

#define CHANNELS           4                         // r, g, b, a
#define RGB_MAX            255                       // max value for single channel
#define SIZE_RESTORE       5                         // e8e[B
#define SIZE_DOWN          3                         // e[B
#define SIZE_NEXT          3                         // e[C
#define SIZE_SAVE          2                         // e7
#define SIZE_COL           19                        // e[38;2;111;222;333m
#define SIZE_NULL          1                         // '\0'
#define SIZE_NXLINE_SAVE  (SIZE_RESTORE + SIZE_SAVE) // e8e[Be7
#define SIZE_NXLINE_CURSOR 6                         // e[De[B
#define RGB_INVALID       -1                         // invalid value for rgb

#define RETURN_IF_ERR(A) do {\
    enum tsprite_error err = A;\
    if (err != TSPRITE_ERROR_NONE) {\
        free(new);\
        free(img);\
        return err;\
    }\
} while(0)

struct tsprite_equiv {
    struct tsprite_col col;
    int fore_seq;
    int back_seq;
};

static enum tsprite_error
read_img(char const* file, unsigned char** img, int* x, int* y);

static enum tsprite_error
resize_img(struct tsprite_param* param, unsigned char** img, int x, int y);

static enum tsprite_error
alloc_seq(struct tsprite_param* param, int x, int y, char** new);

static void
write_sequence(unsigned char* img, char* seq, struct tsprite_param* param, int x, int y, struct tsprite_rect* area);

static char*
write_empty_lines(char* seq, int lines_down, int* lines_down_len);

static int
write_character(struct tsprite_param* param, char* seq, struct tsprite_col col, struct tsprite_col* prev, struct tsprite_col* prev_equiv);

static char*
write_alpha(char* seq, int alpha, int cursor_skip, int* cursor_skip_len);

static int
write_pos(char* seq, int pos, char const* with, char const* without, int without_len);

static enum tsprite_error
write_file(char const* ret, char* new, bool append_file);

static struct tsprite_col
get_col(unsigned char* img, struct tsprite_param* param, int x, int i, int j);

static bool
col_has_equiv(struct tsprite_param* param, struct tsprite_col col);

static double
col_dist(tsprite_col col_a, tsprite_col col_b);

static inline unsigned char*
get_data(unsigned char* img, int x, int i, int j);

static void
set_default_param(struct tsprite_param* param, int x, int y);

enum tsprite_error
tsprite_get_sequence(char const* file, struct tsprite_param* param)
{
    int x, y;
    unsigned char* img;
    char* new = NULL;
    RETURN_IF_ERR(read_img(file, &img, &x, &y));

    if (!param) {
        param = &(struct tsprite_param) { 0 };
    }
    set_default_param(param, x, y);

    RETURN_IF_ERR(resize_img(param, &img, x, y));
    x = param->adj_x; // after resize set to new x and y
    y = param->adj_y;

    RETURN_IF_ERR(alloc_seq(param, x, y, &new));

    char* seq = new;
    if (!param->use_cursor_seq) { // no save seq needed first if cursor param
        memcpy(seq, "\x1b" "7", SIZE_SAVE);
        seq += SIZE_SAVE;
    }
    if (param->area) {
        if (
            param->area->x > x || param->area->y > y ||
            (param->area->x + param->area->w) > x ||
            (param->area->y + param->area->h) > y
        ) {
            free(img);
            free(new);
            return TSPRITE_ERROR_COORD;
        }
        write_sequence(img, seq, param, x, y, param->area);
    } else {
        write_sequence(img, seq, param, x, y, &(struct tsprite_rect) { 0, 0, x, y });
    }

    if (param->ret_file) {
        RETURN_IF_ERR(write_file(param->ret_file, new, param->append_file));
    }
    free(img);
    if (param->ret_str) {
        *param->ret_str = new;
    } else {
        free(new);
    }
    return TSPRITE_ERROR_NONE;
}

static void
write_sequence(unsigned char* img, char* seq, struct tsprite_param* param, int x, int y, struct tsprite_rect* area)
{
    struct tsprite_col prev_col = { RGB_INVALID }, prev_equiv = { RGB_INVALID };
    int lines_down = 1, lines_down_len = 0;
    int cursor_skip = 0, cursor_skip_len = 0;
    int save_seq_len = 0;

    for (int i = area->y; i < area->h + area->y; i++) {
        int alpha = 0;
        char* seq_start = seq;

        for (int j = area->x; j < area->w + area->x; j++) {
            bool skip = false;
            struct tsprite_col col = get_col(img, param, x, i, j);

            if (col.a == 0) {
                alpha++;
                skip = true;
            }
            if (!skip) {
                if (alpha) {
                    // create next cursor seq once alpha stops
                    seq = write_alpha(seq, alpha, cursor_skip, &cursor_skip_len);
                    alpha = 0;
                }
                seq += write_character(param, seq, col, &prev_col, &prev_equiv);
            }
        }
        if (seq_start == seq) { // if pointer unmoved must be empty line
            seq = write_empty_lines(seq, lines_down, &lines_down_len);
            lines_down++;
        } else {
            // write sequence for next line
            if (param->use_cursor_seq) {
                int x_diff = param->x - alpha;
                cursor_skip_len = write_pos(
                    seq, x_diff, "\x1b[%dD\x1b[B",
                    "\x1b[D\x1b[B", SIZE_NXLINE_CURSOR
                );
                seq += cursor_skip_len;
                cursor_skip = x_diff;
            } else {
                save_seq_len = write_pos(
                    seq, lines_down, "\x1b" "8" "\x1b[%dB\x1b" "7",
                    "\x1b" "8" "\x1b[B\x1b" "7", SIZE_NXLINE_SAVE
                );
                seq += save_seq_len;
            }
            lines_down = 1;
        }  
    }
    // delete all trailing transparency
    if (lines_down != 1) {
        seq -= lines_down_len;
    }
    seq -= param->use_cursor_seq ? cursor_skip_len : save_seq_len;
    *seq = '\0';

}

static char*
write_alpha(char* seq, int alpha, int cursor_skip, int* cursor_skip_len) {
    // side effect : reset cursor_skip_len if not 0
    if (*cursor_skip_len != 0) {
        seq -= *cursor_skip_len;
        seq += write_pos(
            seq, cursor_skip - alpha, "\x1b[%dD\x1b[B",
            "\x1b[D\x1b[B", SIZE_NXLINE_CURSOR
        ); 
        *cursor_skip_len = 0;
    } else {
        seq += write_pos(
            seq, alpha, "\x1b[%dC",
            "\x1b[C", SIZE_DOWN
        );   
    }
    return seq;
}

static int
write_pos(char* seq, int pos, char const* with, char const* without, int without_len)
{
    int written;
    if (pos == 1) {
        memcpy(seq, without, without_len);
        written = without_len;
    } else {
        written = sprintf(seq, with, pos);
    }
    return written;
}

static char*
write_empty_lines(char* seq, int lines_down, int* lines_down_len)
{
    // side effect : sets lines_down_len to # chars written to seq
    if (lines_down == 1) {
        memcpy(seq, "\x1b[B", SIZE_DOWN);
        *lines_down_len = SIZE_DOWN;
    } else {
        seq -= *lines_down_len; // if seq exists, go back and overwrite
        *lines_down_len = sprintf(seq, "\x1b[%dB", lines_down);
    }
    seq += *lines_down_len;
    return seq;
}

static int
write_character(struct tsprite_param* param, char* seq, struct tsprite_col col, struct tsprite_col* prev_col, struct tsprite_col* prev_equiv)
{
    // side effect : sets prev_col and prev_equiv to respective current colours
    if (prev_col->r != RGB_INVALID && col_dist(*prev_col, col) <= param->col_threshold) { // squash if same previous Truecolour
        return sprintf(seq, "%s", param->str_func(*prev_col, param->str_len, param->str_set));
    } else {
        prev_col->r = col.r, prev_col->g = col.g, prev_col->b = col.b;
        if (param->equiv_type != TSPRITE_EQUIV_NONE && col_has_equiv(param, col)) {
            if (
                prev_equiv->r == param->equiv_col.r &&
                prev_equiv->g == param->equiv_col.g &&
                prev_equiv->b == param->equiv_col.b
            ) { // squash if same previous equivalent
                return sprintf(seq, "%s", param->str_func(*prev_equiv, param->str_len, param->str_set));
            } else {
                prev_equiv->r = param->equiv_col.r, prev_equiv->g = param->equiv_col.g, prev_equiv->b = param->equiv_col.b;
                return sprintf(
                    seq,
                    "\x1b[%dm%s",
                    param->equiv_seq,
                    param->str_func(param->equiv_col, param->str_len, param->str_set)
                );
            }
        } else {
            prev_equiv->r = RGB_INVALID;
            return sprintf(
                seq,
                "\x1b[%d;2;%d;%d;%dm%s",
                param->disp, 
                col.r, col.g, col.b,
                param->str_func(col, param->str_len, param->str_set)
            );
        }
       
    }
}

static void
set_default_param(struct tsprite_param* param, int x, int y)
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
    if (param->adj_y <= 0) {
        param->adj_y = y;
    }
    if (param->adj_x <= 0) {
        param->adj_x = x;
    }
    if (param->disp == 0) {
        param->disp = TSPRITE_DISP_BKG;
    }
}

static struct tsprite_col
get_col(unsigned char* img, struct tsprite_param* param, int x, int i, int j)
{
    unsigned char* data = get_data(img, x, i, j);
    struct tsprite_col col = {
        data[0], data[1], data[2], data[3]
    };
    param->filter(&col);

    // make sure not out of bounds
    col.r = col.r > RGB_MAX ? RGB_MAX : col.r < 0 ? 0 : col.r;
    col.g = col.g > RGB_MAX ? RGB_MAX : col.g < 0 ? 0 : col.g;
    col.b = col.b > RGB_MAX ? RGB_MAX : col.b < 0 ? 0 : col.b;
    col.a = col.a > RGB_MAX ? RGB_MAX : col.a < 0 ? 0 : col.a;
    return col;
}

static bool
col_has_equiv(struct tsprite_param* param, struct tsprite_col col) {
    // from first 16 entries of CMD color defaults
    static struct tsprite_equiv const equiv_table[] = {
        {{12, 12, 12}, 30, 40},     // Black
        {{197, 15, 31}, 31, 41},    // Red
        {{19, 161, 14}, 32, 42},    // Green
        {{193, 156, 0}, 33, 43},    // Yellow
        {{0, 55, 218}, 34, 44},     // Blue
        {{136, 23, 152}, 35, 45},   // Magenta
        {{58, 150, 221}, 36, 46},   // Cyan
        {{204, 204, 204}, 37, 47},  // White
        {{118, 118, 118}, 90, 100}, // Bright Black
        {{231, 72, 86}, 91, 101},   // Bright Red
        {{22, 198, 12}, 92, 102},   // Bright Green
        {{249, 241, 165}, 93, 103}, // Bright Yellow
        {{59, 120, 255}, 94, 104},  // Bright Blue
        {{180, 0, 158}, 95, 105},   // Bright Magenta
        {{97, 214, 214}, 96, 106},  // Bright Cyan
        {{242, 242, 242}, 97, 107}  // Bright White
    };

    struct tsprite_col check = equiv_table[0].col;
    int lss_index = 0;
    double lss_diff = col_dist(check, col);
    for (int i = 1; i < sizeof(equiv_table) / sizeof(*equiv_table); i++) {
        check = equiv_table[i].col;
        double diff = col_dist(check, col);

        if (diff < lss_diff) {
            lss_index = i;
            lss_diff = diff;
        }
    }

    if (param->equiv_type == TSPRITE_EQUIV_ALL || lss_diff <= param->equiv_threshold) {
        struct tsprite_equiv ret = equiv_table[lss_index];
        param->equiv_col = ret.col;
        param->equiv_seq = param->disp == TSPRITE_DISP_BKG ? ret.back_seq : ret.fore_seq;
        return true;
    }
    return false;
}

// taken directly from https://www.compuphase.com/cmetric.htm
static double
col_dist(tsprite_col col_a, tsprite_col col_b)
{
    long r_mean = ((long) col_a.r + (long) col_b.r) / 2;
    long r = (long) col_a.r - (long) col_b.r;
    long g = (long) col_a.g - (long) col_b.g;
    long b = (long) col_a.b - (long) col_b.b;
    return sqrt((((512+r_mean) * r * r) >> 8) + 4 * g * g + (((767 - r_mean) * b * b) >> 8));
}

static inline unsigned char*
get_data(unsigned char* img, int x, int i, int j)
{
    return img + CHANNELS * (i * x + j);
}

static enum tsprite_error
resize_img(struct tsprite_param* param, unsigned char** img, int x, int y) {
    unsigned char* orig = *img;    
    *img = stbir_resize_uint8_srgb(
        *img, x, y, 0,
        NULL, param->adj_x, param->adj_y, 0, STBIR_RGBA
    );
    free(orig);
    return *img ? TSPRITE_ERROR_NONE : TSPRITE_ERROR_RESIZE;
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
        x = param->area->w, y = param->area->h;
    }
    param->x = x;

    // get largest string size (since possible Unicode which can be more than one byte)
    size_t gtr_strlen = 0;
    for (int i = 0; i < param->str_len; i++) {
        if (strlen(param->str_set[i]) > gtr_strlen) {
            gtr_strlen = strlen(param->str_set[i]);
        }
    }
    
    // formula gets the number digits in length for back sequence
    int seq_size = param->use_cursor_seq ? (int) (floor(log10(x)) + 1) + SIZE_NXLINE_CURSOR : SIZE_NXLINE_SAVE;   
    *new = malloc( // note : approx formula that is >= needed memory
        ((SIZE_COL + gtr_strlen) * x * y) + (y * seq_size) + (param->use_cursor_seq * SIZE_SAVE) + SIZE_NULL
    );
    return *new ? TSPRITE_ERROR_NONE : TSPRITE_ERROR_MEMORY;
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
    // formula from https://contrastchecker.online/color-relative-luminance-calculator
    int bright = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
    return str_set[(int) round(((str_len - 1) / 255.0) * bright)];
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
    // formula from https://dyclassroom.com/image-processing-project/how-to-convert-a-color-image-into-sepia-image
    col->r = round((col->r * 0.393) + (col->g * 0.769) + (col->b * 0.189));
    col->g = round((col->r * 0.349) + (col->g * 0.686) + (col->b * 0.168));
    col->b = round((col->r * 0.272) + (col->g * 0.534) + (col->b * 0.131));
}

void
filter_greyscale(struct tsprite_col* col)
{
    // formula from https://www.tutorialspoint.com/dip/grayscale_to_rgb_conversion.htm
    col->r = col->g = col->b = round(col->r * 0.3 + col->g * 0.59 + col->b * 0.11);
}

void
filter_invert(struct tsprite_col* col)
{
    col->r = (RGB_MAX - col->r);
    col->g = (RGB_MAX - col->g);
    col->b = (RGB_MAX - col->b);
}

#endif
#endif
