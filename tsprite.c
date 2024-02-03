#include <stdio.h>
#include "args.h"

#define TSPRITE_IMPLEMENTATION
#include "lib_tsprite.h"

#define OPT_FILE  "f"
#define OPT_ADJ_X "ax"
#define OPT_ADJ_Y "ay"
#define OPT_AREA  "a"
#define OPT_USE_CURSOR_SEQ "cs"
#define OPT_DISP_CHAR "dc"
#define OPT_COL_THRESHOLD "ct"
#define OPT_EQUIV_TYPE_ALL "ea"
#define OPT_EQUIV_TYPE_THRESHOLD "et"
#define OPT_EQUIV_THRESHOLD "eh"

#define HELP_TEXT "tsprite --" OPT_FILE " input image (mandatory)\n"\
                  "        --" OPT_ADJ_X " width of output sprite\n"\
                  "        --" OPT_ADJ_Y " height of output sprite\n"\
                  "        --" OPT_AREA " area with four arguments : x y w h. flag must be added for each arg\n"\
                  "        --" OPT_USE_CURSOR_SEQ " if flag set, uses cursor movement sequences, else uses save cursor position sequences\n"\
                  "        --" OPT_DISP_CHAR " if flag set, uses character disp mode, else uses background disp mode\n"\
                  "        --" OPT_COL_THRESHOLD " if the distance between colours is <= this value, the colours are equal [0-765]\n"\
                  "        --" OPT_EQUIV_TYPE_ALL " mode of replacing Truecolour sequences with colour index sequences. always replace. if both modes defined, this takes precedence\n"\
                  "        --" OPT_EQUIV_TYPE_THRESHOLD " mode of replacing Truecolour sequences with colour index sequences. replace within threshold from --" OPT_EQUIV_THRESHOLD "\n"\
                  "        --" OPT_EQUIV_THRESHOLD " if the distance between predefined colours is <= this value, replace [0-765]\n"

#define EXIT_IF_ERROR(cond, msg) do {\
    if (cond) {\
        fprintf(stderr, "Error : %s\n", msg);\
        ap_free(parser);\
        exit(EXIT_FAILURE);\
    }\
} while(0)

char const* get_seq_err_desc(enum tsprite_error err);
void set_parser_options(struct tsprite_param* param, ArgParser* parser);
void get_param_args(struct tsprite_param* param, ArgParser* parser);

int
main(int argc, char* argv[])
{
    ArgParser* parser = ap_new_parser();
    EXIT_IF_ERROR(!parser, "memory error in argument parser creation");
    
    char* disp;
    struct tsprite_param param = {
        .ret_str = &disp
    };
    set_parser_options(&param, parser);
    EXIT_IF_ERROR(!ap_parse(parser, argc, argv), "arguments cannot be parsed");
    
    char* file = ap_get_str_value(parser, OPT_FILE);
    EXIT_IF_ERROR(!file, "no file specified");

    get_param_args(&param, parser);
   
    enum tsprite_error seq_err = tsprite_get_sequence(file, &param);
    EXIT_IF_ERROR(seq_err != TSPRITE_ERROR_NONE, get_seq_err_desc(seq_err));
    
    ap_free(parser);
    puts(disp);
    free(disp);

    return EXIT_SUCCESS;
} 

void
get_param_args(struct tsprite_param* param, ArgParser* parser) {
    param->adj_x = ap_get_int_value(parser, OPT_ADJ_X);
    param->adj_y = ap_get_int_value(parser, OPT_ADJ_Y);
    param->use_cursor_seq = ap_found(parser, OPT_USE_CURSOR_SEQ);
    param->col_threshold = ap_get_dbl_value(parser, OPT_COL_THRESHOLD);
    
    int area_num = ap_count(parser, OPT_AREA);
    if (area_num == 4) {
        param->area = &(struct tsprite_rect) {
            ap_get_int_value_at_index(parser, OPT_AREA, 0),
            ap_get_int_value_at_index(parser, OPT_AREA, 1),
            ap_get_int_value_at_index(parser, OPT_AREA, 2),
            ap_get_int_value_at_index(parser, OPT_AREA, 3)
        };
    } else if (area_num != 0) {
        EXIT_IF_ERROR(true, "invalid area specified");
    }
    
    if (ap_found(parser, OPT_DISP_CHAR)) {
        param->disp = TSPRITE_DISP_CHAR;
        param->str_set = (char*[]) { "█" };
        param->str_len = 1;
    }
    
    if (ap_found(parser, OPT_EQUIV_TYPE_ALL)) {
        param->equiv_type = TSPRITE_EQUIV_ALL;
    } else if (ap_found(parser, OPT_EQUIV_TYPE_THRESHOLD)) {
        param->equiv_type = TSPRITE_EQUIV_THRESHOLD;
        param->equiv_threshold = ap_get_dbl_value(parser, OPT_EQUIV_THRESHOLD);
    }
}

void
set_parser_options(struct tsprite_param* param, ArgParser* parser) {
    ap_set_helptext(parser, HELP_TEXT);
    ap_add_str_opt(parser, OPT_FILE, NULL);
    ap_add_int_opt(parser, OPT_ADJ_X, param->adj_x);
    ap_add_int_opt(parser, OPT_ADJ_Y, param->adj_y);
    ap_add_int_opt(parser, OPT_AREA, 0);
    ap_add_flag(parser, OPT_USE_CURSOR_SEQ);
    ap_add_flag(parser, OPT_DISP_CHAR);
    ap_add_dbl_opt(parser, OPT_COL_THRESHOLD, param->col_threshold);
    ap_add_flag(parser, OPT_EQUIV_TYPE_ALL);
    ap_add_flag(parser, OPT_EQUIV_TYPE_THRESHOLD);
    ap_add_dbl_opt(parser, OPT_EQUIV_THRESHOLD, param->equiv_threshold);
}

char const*
get_seq_err_desc(enum tsprite_error err) {
    switch(err) {
        case TSPRITE_ERROR_IMAGE:
            return "invalid image";
        case TSPRITE_ERROR_RESIZE:
            return "invalid resize";
        case TSPRITE_ERROR_MEMORY:
            return "memory error in sequence creation";
        case TSPRITE_ERROR_COORD:
            return "area out of bounds";
        case TSPRITE_ERROR_FILE:
            return "file could not be opened / created";
        default:
            return "";
    }
}