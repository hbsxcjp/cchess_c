#ifndef BASE_H
#define BASE_H

#include <assert.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <wchar.h>
//#include <math.h>
#include <ctype.h>
#include <string.h>
#include <wctype.h>

//=================================================================
//棋子相关的类型
//=================================================================

// 两方
#define PIECECOLORNUM 2
// 棋子种类数量
#define PIECEKINDNUM 7
// 一方棋子个数
#define PIECENUM 32
// 空子的字符表示
#define BLANKCHAR L'_'

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

// 棋子类型 高四位表示颜色，低四位表示种类，-1表示空子
typedef enum {
    BLANKPIECE = -1,
    REDKING = 0x0,
    REDADVISOR,
    REDBISHOP,
    REDKNIGHT,
    REDROOK,
    REDCANNON,
    REDPAWN,
    BLACKKING = 0x10,
    BLACKADVISOR,
    BLACKBISHOP,
    BLACKKNIGHT,
    BLACKROOK,
    BLACKCANNON,
    BLACKPAWN
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
// 棋盘位置类型
#define Seat int
// 棋子数组长度
#define BOARDLEN 0x99

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

// 一副棋盘结构类型
typedef struct
{
    Piece pieces[BOARDLEN]; // 位置0x00, 高四位表示行，低四位表示列
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

// 临时字符串长度
#define THOUSAND_SIZE 1024
// 存储棋局字符串长度
#define HUNDRED_THOUSAND_SIZE THOUSAND_SIZE * 128
// 棋局信息数量
#define INFOSIZE 32

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

// 着法类型
typedef struct Move_ {
    Seat fseat, tseat; // 起止位置0x00
    Piece tpiece; // 目标位置棋子
    wchar_t* remark; // 本着注解
    struct Move_ *pmove, *nmove, *omove; // 前着、下着、变着
    int nextNo_, otherNo_, CC_ColNo_; // 走着、变着序号，文本图列号
} Move;

// 棋局类型
typedef struct {
    Board* board;
    Move *rootMove, *currentMove; // 根节点、当前节点
    wchar_t* info[INFOSIZE][2];
    int infoCount, movCount_, remCount_, maxRemLen_, maxRow_, maxCol_;
} Instance;

#endif