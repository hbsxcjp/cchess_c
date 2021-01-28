#include "head/play.h"
#include <assert.h>

struct Play {
    ChessManual cm;
};

Play newPlay(const char* fileName)
{
    Play play = malloc(sizeof(struct Play));
    assert(play);
    play->cm = getChessManual_file(fileName);
    return play;
}

void delPlay(Play play)
{
    delChessManual(play->cm);
    free(play);
}