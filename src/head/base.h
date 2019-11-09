#ifndef BASE_H
#define BASE_H

#include <assert.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>
#include <string.h>

//=================================================================
//棋子相关的类型
//=================================================================

// 两方
#define PIECECOLORNUM 2
// 棋子种类数量
#define PIECEKINDNUM 7
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
    const Piece* pieces[BOARDROW][BOARDCOL]; // 存储位置可用row,col或Seat表示
    PieceColor bottomColor;
} Board;

// 棋盘变换类型
typedef enum {
    EXCHANGE,
    ROTATE,
    SYMMETRY
} ChangeType;

//=================================================================
//棋局相关的类型
//=================================================================

#define REMARKSIZE TEMPSTR_SIZE / 2
#define INFOSIZE 32

// 棋局存储类型
typedef enum {
    XQF,
    PGN_ICCS,
    PGN_ZH,
    PGN_CC,
    BIN,
    JSON
} RecFormat;

// 着法类型
struct structMove;
struct structMove {
    const Seat **fseat, **tseat;
    const Piece** tpiece; // 指向const Piece*的指针
    wchar_t* remark; 
    struct structMove *pmove, *nmove, *omove;
    int nextNo_, otherNo_, CC_ColNo_; // 图中列位置
};
typedef struct structMove Move;

// 棋局类型
typedef struct {
    Board* board;
    Move *rootMove, *currentMove;
    wchar_t** info_name;
    wchar_t** info_value;
    int infoCount, movCount_, remCount_, remLenMax_, maxRow_, maxCol_;
} Instance;

#endif