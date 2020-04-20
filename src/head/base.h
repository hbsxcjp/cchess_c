#ifndef BASE_H
#define BASE_H

#include "cJSON.h"
#include "pcre.h"
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
    SYMMETRY
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

// 着法类型
typedef struct Move* Move;
typedef const struct Move* CMove;
// 局面记录类型
typedef struct AspectTable* AspectTable;
typedef const struct AspectTable* CAspectTable;
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

struct _Menu;
// 演示类型结构
typedef struct Console_ {
    HANDLE hIn, hOut;
    ChessManual cm;
    Thema thema;
    Area curArea;
    int fileIndex;
    char fileName[FILENAME_MAX];
    SMALL_RECT WinRect, MenuRect, BoardRect, CurmoveRect, MoveRect, StatusRect, OpenFileRect;
    SMALL_RECT iMenuRect, iBoardRect, iCurmoveRect, iMoveRect, iStatusRect, iOpenFileRect;
    struct _Menu *rootMenu, *curMenu;
    //CHAR_INFO chBuf[40 * 120];
    //SMALL_RECT chBufRect;
} Console, *PConsole;

// 菜单命令
typedef void (*MENU_FUNC)(PConsole);

// 菜单结构
typedef struct _Menu {
    wchar_t name[WCHARSIZE], desc[WCHARSIZE];
    MENU_FUNC func; // 菜单关联的命令函数，如有子菜单则应为空
    struct _Menu *preMenu, *brotherMenu, *childMenu;
    int brotherIndex, childIndex;
} Menu, *PMenu;

// 菜单初始信息结构
typedef struct _MenuData {
    wchar_t name[WCHARSIZE], desc[WCHARSIZE];
    MENU_FUNC func;
} MenuData, *PMenuData;

#endif