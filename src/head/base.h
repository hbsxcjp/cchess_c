#ifndef BASE_H
#define BASE_H

#include <assert.h>
#include <conio.h>
#include <ctype.h>
#include <io.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <windows.h>

#define WCHARSIZE 256
#define WIDEWCHARSIZE 1024
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

// 字符串长度
#define THOUSAND_SIZE 1024
// 字符串长度
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
    wchar_t* remark; // 注解
    wchar_t zhStr[6]; // 着法名称
    struct Move_ *pmove, *nmove, *omove; // 前着、下着、变着
    int nextNo_, otherNo_, CC_ColNo_; // 走着、变着序号，文本图列号
} Move, *PMove;

// 棋局类型
typedef struct {
    Board* board;
    Move *rootMove, *currentMove; // 根节点、当前节点
    wchar_t* info[INFOSIZE][2];
    int infoCount, movCount_, remCount_, maxRemLen_, maxRow_, maxCol_;
} ChessManual, *PChessManual;

//=================================================================
//棋局演示相关的类型
//=================================================================
/*
typedef enum {
    FIRST = 0x80,
    MIDDLE = 0x08,
    END = 0x01
} Pos;
//*/

// 区域主题颜色配置类型
typedef enum {
    SIMPLE,
    SHOWY,
    HIGHLIGHT
} Thema;

// 控制台焦点区域类型
typedef enum {
    MOVEA,
    CURMOVEA,
    BOARDA,
    MENUA,
    STATUSA
} Area;

// 菜单命令
typedef void (*MENU_FUNC)(void);

// 菜单结构
typedef struct Menu_ {
    wchar_t name[WCHARSIZE], desc[WCHARSIZE];
    MENU_FUNC func; // 菜单关联的命令函数，如有子菜单则应为空
    struct Menu_ *preMenu, *brotherMenu, *childMenu;
    int brotherIndex, childIndex;
} Menu, *PMenu;

// 菜单初始信息结构
typedef struct MenuData_ {
    wchar_t name[WCHARSIZE], desc[WCHARSIZE];
    MENU_FUNC func;
} MenuData, *PMenuData;

// 演示类型结构
typedef struct Console_ {
    HANDLE hIn, hOut;
    PMenu rootMenu, curMenu;
    PChessManual cm;
    Thema thema;
    Area curArea;
    SMALL_RECT WinRect, MenuRect, iMenuRect, StatusRect, iStatusRect;
    SMALL_RECT BoardRect, iBoardRect, CurmoveRect, iCurmoveRect, MoveRect, iMoveRect;
    int cmFirstRow, cmFirstCol, mFirstRow, mFirstCol;
} Console, *PConsole;

#endif