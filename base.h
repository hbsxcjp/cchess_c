#ifndef BASE_H
#define BASE_H

#include <assert.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

//=================================================================
//棋子相关的类型
//=================================================================

// 两方
#define PIECECOLORNUM 2
// 一方棋子个数
#define PIECENUM 16
// 空子的字符表示
#define BLANKCHAR L'_'
// 临时字符串长度
#define TEMPSTR_SIZE 1024

// 棋子颜色类型
typedef enum {
    RED,
    BLACK
} PieceColor;

// 棋子种类类型
typedef enum {
    KING,
    ADVISOR,
    BISHOP,
    KNIGHT,
    ROOK,
    CANNON,
    PAWN
} PieceKind;

// 棋子结构类型
typedef struct
{
    const PieceColor color;
    const PieceKind kind;
} Piece;

// 一副棋子结构类型
typedef struct
{
    const Piece piece[PIECECOLORNUM][PIECENUM];
} Pieces;

//=================================================================
//棋盘相关的类型
//=================================================================

// 棋盘行数
#define BOARDROW 10
// 棋盘列数
#define BOARDCOL 9
// 棋盘位置个数
#define SEATNUM BOARDROW* BOARDCOL

// 棋子移动方向
typedef enum {
    SW,
    SE,
    NW,
    NE,
    WS,
    ES,
    WN,
    EN
} MoveDirection;

// 棋子可置入位置结构类型
typedef struct
{
    int row, col;
} Seat;

// 一副棋盘结构类型
typedef struct
{
    const Piece* piece[BOARDROW][BOARDCOL];
    PieceColor bottomColor;
} Board;

// 棋盘变换类型
typedef enum {
    EXCHANGE,
    ROTATE,
    SYMMETRY
} ChangeType;

//=================================================================
//着法相关的类型
//=================================================================

struct Move;
typedef struct {
    Seat fseat, tseat;
    const Piece* tpiece;
    struct Move *nmove, *omove;
} Move;

#endif