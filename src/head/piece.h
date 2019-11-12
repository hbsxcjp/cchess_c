#ifndef PIECE_H
#define PIECE_H

#include "base.h"

//  取得表示棋子的颜色
PieceColor getColor(Piece piece);

// 取得表示棋子的种类
int getKind(Piece piece);

//  取得表示棋子的字符
wchar_t getChar(Piece piece);

//  取得棋子
Piece getPiece_ch(wchar_t ch);

// 取得表示棋子的名称
wchar_t getPieName(Piece piece);

// 取得表示棋子文本的名称
wchar_t getPieName_T(Piece piece);

// 取得表示棋子表示字符串的名称
wchar_t *getPieString(wchar_t* pieStr, size_t n, Piece piece);

// 测试本翻译单元各种对象、函数
void testPiece(FILE *fout);

#endif