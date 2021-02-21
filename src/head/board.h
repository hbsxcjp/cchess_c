#ifndef BOARD_H
#define BOARD_H

#include "base.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

// 新建一个board
Board newBoard(void);
// 释放board
void delBoard(Board board);

// 获取行列值
int getRow_s(CSeat seat);
int getCol_s(CSeat seat);

// 获取相对位置的Seat
Seat getOtherSeat(Board board, Seat seat, ChangeType ct);

// 获取行列整数值
int getRowCol_rc(int row, int col);
int getRowCol_s(CSeat seat);
int getRow_rowcol(int rowcol);
int getCol_rowcol(int rowcol);

// 根据中文着法取得走棋颜色
PieceColor getColor_zh(const wchar_t* zhStr);

// 根据行、列值获取seat
Seat getSeat_rc(Board board, int row, int col);
Seat getSeat_rowcol(Board board, int rowcol);

// 获取某棋盘内某行、某列位置的一个棋子
Piece getPiece_s(CSeat seat);
Piece getPiece_rc(Board board, int row, int col);
Piece getPiece_rowcol(Board board, int rowcol);

// 取得棋盘局面的pieChars字符串表示, 包含90个字符
wchar_t* getPieChars_board(wchar_t* pieChars, Board board);

// pieChars表示的棋盘局面转换成FEN字符串，返回FEN
wchar_t* getFEN_pieChars(wchar_t* FEN, const wchar_t* pieChars);

// 取得棋盘局面的FEN字符串表示
wchar_t* getFEN_board(wchar_t* FEN, Board board);

// FEN字符串转换成pieChars表示的棋盘局面
wchar_t* getPieChars_FEN(wchar_t* pieChars, const wchar_t* FEN);

// 棋盘复位
void resetBoard(Board board);
// 使用一个pieChars字符串设置棋盘局面, 包含90个字符
void setBoard_pieChars(Board board, const wchar_t* pieChars);
// 使用一个FEN字符串设置棋盘局面
void setBoard_FEN(Board board, const wchar_t* FEN);

// 给定颜色是否在棋盘底边
bool isBottomSide(CBoard board, PieceColor color);

// 取得某方将帅的位置seat
Seat getKingSeat(Board board, PieceColor color);

// 取得活的棋子位置
int getLiveSeats_bc(Seat* seats, CBoard board, PieceColor color, bool filterStronge);

// 移动棋子，返回目的地棋子
Piece movePiece(Seat fseat, Seat tseat, Piece eatPiece);

// 某棋盘红黑定方向后某种棋子可置入位置的集合，返回位置个数（至少90个）
int putSeats(Seat* seats, Board board, bool isBottom, PieceKind kind);
// 某棋盘局面下某位置的棋子“行走规则(筛除同色，未筛将军)”可移入位置的集合（至多17个）
int moveSeats(Seat* seats, Board board, Seat fseat);
// 某棋盘局面下某位置的棋子在“行走规则”基础上筛除将帅对面、被将军的位置后可移入位置的集合（至多17个）
int canMoveSeats(Seat* seats, Board board, Seat fseat, bool sure);

// 28.1　将
//　　凡走子直接攻击对方帅(将) 者，称为“照将”，简称“将”。
bool isKilling(Board board, Seat fseat, Seat tseat);
// 着法走之后，判断是否已输棋 (将死、对面、困毙)
// 第3条　将死和困毙
//　　3.1　一方的棋子攻击对方的帅(将) ，并在下一着要把它吃掉，称为“照将”，或简称“将”。“照将”不必声明。
//　　被“照将”的一方必须立即“应将”，即用自己的着法去化解被“将”的状态。
//　　如果被“照将”而无法“应将”，就算被“将死”。
//　　3.2　轮到走棋的一方，无子可走，就算被“困毙”。
bool isFail(Board board, PieceColor color);
// 未测试
// 28.2　杀
//　　凡走子企图在下一着照将或连续照将，将死对方者，称为“杀着”，简称“杀”。
bool isWillKill(Board board, Seat fseat, Seat tseat);
// 未测试
// 28.3　捉
//　　凡走子后能够造成在下一着(包括从下一着开始运用连续照将或连续交换的手段) 吃掉对方某个无根子，称为“捉”。
bool isCatch(Board board, Seat fseat, Seat tseat);
// 28.4　打
//　　将、杀、捉等攻击手段，统称为“打”。
bool isClout(Board board, Seat fseat, Seat tseat);
// 28.9　闲
//　　凡走子性质不属于将、杀、捉，统称为“闲”。兑、献、拦、跟，均属“闲”的范畴。
bool isIdle(Board board, Seat fseat, Seat tseat);

// 着法走之前，判断起止位置是可走的位置(遵守“行走规则”，并排除：棋子同色、将帅对面、被将军等的情况)
bool isCanMove(Board board, Seat fseat, Seat tseat);

// 按某种变换类型变换棋盘局面
void changeBoard(Board board, ChangeType ct);

// 设置着法的起止位置
bool getSeats_zh(Seat* pfseat, Seat* ptseat, Board board, const wchar_t* zhStr);
// 设置着法的中文字符串
void getZhStr_seats(wchar_t* zhStr, Board board, Seat fseat, Seat tseat);

// 获取位置的iccs描述字符串
const wchar_t* getICCS_s(wchar_t* iccs, Seat fseat, Seat tseat);

// 取得表示位置字符串的名称
wchar_t* getSeatString(wchar_t* seatStr, CSeat seat);
// 输出某棋盘局面的文本字符串，长度小于1024
wchar_t* getBoardString_pieceChars(wchar_t* boardStr, const wchar_t* pieChars);
// 输出某棋盘局面的文本字符串，长度小于1024
wchar_t* getBoardString(wchar_t* boardStr, Board board);
// 棋盘上边标识字符串
wchar_t* getBoardPreString(wchar_t* preStr, CBoard board);
// 棋盘下边标识字符串
wchar_t* getBoardSufString(wchar_t* sufStr, CBoard board);

void printBoard(Board board, int arg1, int arg2, const wchar_t* arg3);

bool seat_equal(CSeat seat0, CSeat seat1);

bool board_equal(Board board0, Board board1);

#endif