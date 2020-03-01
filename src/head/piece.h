#ifndef PIECE_H
#define PIECE_H

#include "base.h"

// 取得表示棋子的颜色(判断棋子值的高四位)
#define getColor(piece) ((bool)((piece)&0xF0))

// 取得对方棋子的颜色
#define getOtherColor(piece) (!getColor(piece))

//  取得表示棋子的种类(取棋子值的低四位)
#define getKind(piece) ((piece)&0x0F)

//  取得表示相同种类的对方棋子
#define getOtherPiece(piece) ((getOtherColor(piece) << 4) | getKind(piece))

//  取得表示棋子的字符
wchar_t getChar(Piece piece);

// 取得代表棋子字符的颜色
PieceColor getColor_ch(wchar_t ch);

//  取得棋子
Piece getPiece_ch(wchar_t ch);

// 取得表示棋子的名称
wchar_t getPieName(Piece piece);

// 取得表示棋子文本的名称
wchar_t getPieName_T(Piece piece);

// 取得表示棋子表示字符串的名称
wchar_t* getPieString(wchar_t* pieStr, size_t n, Piece piece);

// 测试本翻译单元各种对象、函数
void testPiece(FILE* fout);

#endif