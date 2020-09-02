#ifndef BASE_H
#define BASE_H

// 'Droid Sans Mono', 'monospace', monospace, 'Droid Sans Fallback'

#include "pcre.h"
#include "sqlite3.h"
#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#define THOUSAND 1024
#define WCHARSIZE 256
#define WIDEWCHARSIZE THOUSAND
#define SUPERWIDEWCHARSIZE (WIDEWCHARSIZE * 12)

//=================================================================
//棋子相关的类型
//=================================================================
// 两方
#define PIECECOLORNUM 2
// 棋子种类数量
#define PIECEKINDNUM 7
// 一方棋子个数
#define SIDEPIECENUM 16
#define PIECENUM (PIECECOLORNUM * SIDEPIECENUM)

// 棋子颜色类型
typedef enum {
    RED,
    BLACK,
    NOTCOLOR
} PieceColor;

// 棋子种类类型
typedef enum {
    KING,
    ADVISOR,
    BISHOP,
    KNIGHT,
    ROOK,
    CANNON,
    PAWN,
    NOTKIND
} PieceKind;

// 棋子指针类型（不透明）
typedef struct Piece* Piece;
typedef const struct Piece* CPiece;
typedef struct Pieces* Pieces;
typedef const struct Pieces* CPieces;

//=================================================================
//棋盘相关的类型
//=================================================================
// 棋盘行数
#define BOARDROW 10
// 棋盘列数
#define BOARDCOL 9
// 棋盘位置个数
#define SEATNUM (BOARDROW * BOARDCOL)

// 棋盘变换类型
typedef enum {
    EXCHANGE,
    ROTATE,
    SYMMETRY_H,
    SYMMETRY_V
} ChangeType;

// 棋盘位置类型
typedef struct Seat* Seat;
typedef const struct Seat* CSeat;
// 一副棋盘结构类型
typedef struct Board* Board;
typedef const struct Board* CBoard;

//=================================================================
//棋局相关的类型
//=================================================================

// 棋局存储类型
typedef enum {
    XQF,
    BIN,
    JSON,
    PGN_ICCS,
    PGN_ZH,
    PGN_CC,
    NOTFMT
} RecFormat;

// 局面存储类型
typedef enum {
    //FEN_MovePtr,
    FEN_MRValue,
    Hash_MRValue
} AspFormat;

// 着法类型
typedef struct Move* Move;
typedef const struct Move* CMove;

typedef struct MoveRec* MoveRec;

// 局面记录类型
typedef struct Aspects* Aspects;
typedef const struct Aspects* CAspects;
// 棋局类型
typedef struct ChessManual* ChessManual;
typedef const struct ChessManual* CChessManual;

//=================================================================
//棋局演示相关的类型
//=================================================================
// 区域主题颜色配置类型
typedef enum {
    SIMPLE,
    SHOWY,
    //HIGHLIGHT
} Thema;

// 控制台焦点区域类型
typedef enum {
    MOVEA,
    CURMOVEA,
    BOARDA,
    OPENFILEA,
    SAVEFILEA,
    ABOUTA,
    MENUA,
    DECAREA, // 代表递减
    INCAREA, // 代表递增
    OLDAREA // 代表旧区域
} Area;

typedef struct Play* Play;
typedef const struct Play* CPlay;

// 演示类型结构
typedef struct Console* PConsole;

#endif