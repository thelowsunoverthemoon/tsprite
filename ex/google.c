#include "seqpaint.h"

int
main(void)
{
    struct SeqPaintParam param = {
        .scaleX = 4,
        .scaleY = 8,
        .range = 3,
        .blend = SEQPAINT_BLEND_XY
    };
    getSequence("google.jpg", "seq.txt", &param);
}
