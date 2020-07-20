#ifndef PLAY_H
#define PLAY_H

#include "chessManual.h"
#include "move.h"

// 前进一步
void go(Play play);
// 前进到变着
void goOther(Play play);
void goEnd(Play play);
// 前进至指定move
void goTo(Play play, Move move);

// 后退一步
void back(Play play);
void backNext(Play play);
void backOther(Play play);
void backFirst(Play play);
// 后退至指定move
void backTo(Play play, Move move);

// 前进或后退数步
void goInc(Play play, int inc);


#endif