#ifndef BOARD_H
#define BOARD_H

#include "piece.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

// 棋盘行数
#define BOARDROW 10
// 棋盘列数
#define BOARDCOL 9

// 一副棋盘结构类型
typedef struct
{
    const Piece* piece[BOARDROW][BOARDCOL];
    PieceColor bottomColor;
} Board;

// 棋盘变换类型
enum ChangeType {
    EXCHANGE,
    ROTATE,
    SYMMETRY
};

// 置入某棋盘内某行、某列位置一个棋子
void setPiece(Board* board, int row, int col, const Piece* piece);

// 输出某棋盘局面的文本字符串
wchar_t* toString(wchar_t* wstr, Board* board);

// 测试本翻译单元各种对象、函数
void testBoard(void);

#endif