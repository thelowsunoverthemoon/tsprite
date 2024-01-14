#include "tsprite.h"

// Basic example that displays Google image.

int
main(void)
{
    char* str;
    struct tsprite_param param = {
        .scale_x = 4,
        .scale_y = 8,
        .range = 3,
        .blend = TSPRITE_BLEND_XY,
        .ret_str = &str,
        .col_threshold = 5,
        .use_cursor_seq = true,
        .disp = TSPRITE_DISP_CHAR,
        .str_set = (char*[]) { "█" },
        .str_len = 1
    };

    enum tsprite_error err = tsprite_get_sequence("google.jpg", &param);
    if (err != TSPRITE_ERROR_NONE) {
        printf("tsprite error %d", err);
        return EXIT_FAILURE;
    }

    printf("\x1b[1;1H%s\x1b[0m", str);
    return EXIT_SUCCESS;

}

