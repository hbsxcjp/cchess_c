#ifndef BOARD_H
#define BOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include "piece.h"

#define BOARDROW 10 // 棋盘行数
#define BOARDCOL 9  // 棋盘列数

// 棋盘变换类型
enum ChangeType
{
    EXCHANGE,
    ROTATE,
    SYMMETRY
};

// 棋盘格子结构类型
typedef struct
{
    const int row, col;
    Piece *ppiece;
} Seat;

// 一副棋盘结构类型
typedef struct
{
    const Seat seat[BOARDROW][BOARDCOL];
    PieceColor bottomColor;
} Board;

Seat getSeat(int row, int col);

Board creatBoard(void);

void toString(wchar_t *wstr, size_t count, const Board *board);

#endif