#ifndef PIECE_H
#define PIECE_H

#include "base.h"

// 生成一副棋子
Pieces newPieces(void);
// 释放一副棋子
void delPieces(Pieces pieces);

// 遍历每个棋子
void piecesMap(Pieces pieces, void apply(Piece, void*), void* ptr);

// 取得表示棋子的颜色
PieceColor getColor(CPiece piece);
PieceColor getColor_ch(wchar_t ch);
// 取得对方颜色
PieceColor getOtherColor(PieceColor color);

//  取得表示棋子的种类(取棋子值的低四位)
PieceKind getKind(CPiece piece);
PieceKind getKind_ch(wchar_t ch);

//  取得表示棋子的字符
wchar_t getBlankChar();
wchar_t getChar(CPiece piece);
// 取得表示棋子的名称
wchar_t getPieName_ch(wchar_t ch);
wchar_t getPieName(CPiece piece);
// 取得表示棋子文本的名称
wchar_t getPieName_T_ch(wchar_t ch);
wchar_t getPieName_T(CPiece piece);
const wchar_t* getPieceNames(void);

// 判断是否棋子名
bool isPieceName(wchar_t name);
// 判断是否直行棋子名
bool isLinePieceName(wchar_t name);
// 判断是否兵棋子名
bool isPawnPieceName(wchar_t name);
// 判断是否马棋子名
bool isKnightPieceName(wchar_t name);
// 判断是否强棋子名
bool isStronge(CPiece piece);
// 判断是否空棋子
bool isBlankPiece(CPiece piece);

//  取得棋子
Piece getBlankPiece();
Piece getKingPiece(Pieces pieces, PieceColor color);
Piece getOtherPiece(Pieces pieces, CPiece piece);
Piece getPiece_ch(Pieces pieces, wchar_t ch);

//  取得棋子所在的位置
Seat getSeat_p(CPiece piece);
// 设置棋子的位置
void setNullSeat(Piece piece);
void setSeat(Piece piece, Seat seat);
// 取得活的棋子位置
int getLiveSeats_c(Seat* seats, Pieces pieces, PieceColor color);
// 取得活的强棋子位置
int getLiveSeats_cs(Seat* seats, Pieces pieces, PieceColor color);

// 取得表示棋子表示字符串的名称
wchar_t* getPieString(wchar_t* pieStr, CPiece piece);
// 测试本翻译单元各种对象、函数
void testPiece(wchar_t* wstr);

#endif