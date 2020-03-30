#ifndef PIECE_H
#define PIECE_H

#include "base.h"

extern const wchar_t* PieceNames[PIECECOLORNUM];

// 生成一副棋子
Pieces newPieces(void);

// 取得表示棋子的颜色(判断棋子值的高四位)
PieceColor getColor(Piece piece);
PieceColor getColor_ch(wchar_t ch);

// 取得对方棋子的颜色
PieceColor getOtherColor(Piece piece);

//  取得表示棋子的种类(取棋子值的低四位)
PieceKind getKind(Piece piece);

//  取得棋子所在的位置
Seat getSeat_p(Piece piece);

//  取得表示棋子的字符
wchar_t getChar(Piece piece);

// 取得表示棋子的名称
wchar_t getPieName(Piece piece);

// 取得表示棋子文本的名称
wchar_t getPieName_T(Piece piece);

// 判断是否棋子名
bool isPieceName(wchar_t name);

// 判断是否直行棋子名
bool isLinePieceName(wchar_t name);

// 判断是否兵棋子名
bool isPawnPieceName(wchar_t name);

// 判断是否马棋子名
bool isKnightPieceName(wchar_t name);

// 判断是否强棋子名
bool isStronge(Piece piece);

//  取得棋子
Piece getKingPiece(Pieces pieces, PieceColor color);

Piece getPiece_i(Pieces pieces, PieceColor color, int index);

Piece getPiece_ch(Pieces pieces, wchar_t ch);

// 设置为已放置在棋盘上
void setSeat(Piece piece, Seat seat);

// 取得表示棋子表示字符串的名称
wchar_t* getPieString(wchar_t* pieStr, Piece piece);

// 测试本翻译单元各种对象、函数
void testPiece(FILE* fout);

#endif