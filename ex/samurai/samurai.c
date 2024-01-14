#include "tsprite.h"

// Create sprites from spritesheet to import to a Batch file game

#define FILE_NAME "sprite.txt"
#define SPRITESHEET_NAME "samurai.png"
#define SPRITE_NUM 8
#define SPRITE_HEIGHT 23
#define SPRITE_WIDTH 64

int
main(void)
{
    FILE* new = fopen(FILE_NAME, "w"); // create empty file
    if (!new) {
        puts("Error creating new file");
        return EXIT_FAILURE;
    }
    fclose(new);

    struct tsprite_rect area = {
        .w = SPRITE_WIDTH,
        .h = SPRITE_HEIGHT
    };
    struct tsprite_param param = {
        .scale_x = 1,
        .scale_y = 1,
        .blend = TSPRITE_BLEND_NONE,
        .ret_file = FILE_NAME,
        .col_threshold = 5,
        .use_cursor_seq = true,
        .area = &area,
        .disp = TSPRITE_DISP_CHAR,
        .str_set = (char*[]) { "█" },
        .str_len = 1,
        .ret_file = FILE_NAME,
        .append_file = true
    };
        
    for (int i = 0; i < SPRITE_NUM; i++) {
        enum tsprite_error err = tsprite_get_sequence(SPRITESHEET_NAME, &param);
        if (err != TSPRITE_ERROR_NONE) {
            printf("tsprite error %d", err);
            return EXIT_FAILURE;
        }
        area.x += SPRITE_WIDTH;
    }

    return EXIT_SUCCESS;

}

