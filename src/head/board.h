#ifndef BOARD_H
#define BOARD_H

#include "base.h"

// 新建一个board
Board newBoard(void);
// 释放board
void delBoard(Board board);

// 获取行列值
int getRow_s(CSeat seat);
int getCol_s(CSeat seat);

// 获取对称行列值
int getOtherRow_r(int row);
int getOtherCol_c(int col);

// 获取对称行列值
int getOtherRow_s(CSeat seat);
int getOtherCol_s(CSeat seat);

// 获取行列整数值
int getRowCol_rc(int row, int col);
int getRowCol_s(CSeat seat);
int getRow_rowcol(int rowcol);
int getCol_rowcol(int rowcol);

// 判断位置是否相同
bool isSameSeat(CSeat aseat, CSeat bseat);

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
// 按给定参数查找活的棋子
int getLiveSeats_cn(Seat* seats, Board board, PieceColor color, wchar_t name);
int getLiveSeats_cnc(Seat* seats, Board board, PieceColor color, wchar_t name, int col);
// 取得某方"兵"的棋子位置seats
int getSortPawnLiveSeats(Seat* seats, Board board, PieceColor color, wchar_t name);

// 是否将（凡走子直接攻击对方帅(将)者，称为“照将”，简称“将”）
bool isKill(Board board, PieceColor color);
// 是否杀（凡走子企图在下一着照将或连续照将，将死对方者，称为“杀着”，简称“杀”）
bool isWillKill(Board board, PieceColor color);
// 是否捉（凡走子后能够造成在下一着(包括从下一着开始运用连续照将或连续交换的手段)吃掉对方某个无根子，称为“捉”）
bool isCatch(Board board, PieceColor color);
// 是否将帅对面
bool isFace(Board board, PieceColor color);
// 是否困毙
bool isUnableMove(Board board, PieceColor color);
// 是否已输棋 (根据象棋规则，还需添加更多条件)
bool isFail(Board board, PieceColor color);

// 某棋盘红黑定方向后某种棋子可置入位置的集合，返回位置个数（至少90个）
int putSeats(Seat* seats, Board board, bool isBottom, PieceKind kind);
// 某棋盘局面下某位置的棋子“行走规则(筛除同色，未筛将军)”可移入位置的集合（至多17个）
int moveSeats(Seat* seats, Board board, Seat fseat);
// 某棋盘局面下某位置的棋子在“行走规则”基础上筛除被将军位置后可移入位置的集合（至多17个）
int ableMoveSeats(Seat* seats, int count, Board board, Seat fseat);
// 某棋盘局面下某位置的棋子在“行走规则”基础上，移动后将被将军位置的集合（至多17个）
int unableMoveSeats(Seat* seats, int count, Board board, Seat fseat);

// 移动棋子，返回目的地棋子
Piece movePiece(Seat fseat, Seat tseat, Piece eatPiece);

// 按某种变换类型变换棋盘局面
void changeBoard(Board board, ChangeType ct);

// 取得活的棋子位置
int getLiveSeats_bc(Seat* seats, CBoard board, PieceColor color);
// 取得活的强棋子位置
int getLiveSeats_bcs(Seat* seats, CBoard board, PieceColor color);

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


#endif