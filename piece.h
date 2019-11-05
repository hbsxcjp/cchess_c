#ifndef PIECE_H
#define PIECE_H

#include "base.h"

//  取得表示棋子的字符
wchar_t getChar(const Piece *piece);

// 取得表示棋子的名称
wchar_t getPieName(const Piece *piece);

// 取得表示棋子文本的名称
wchar_t getPieName_T(const Piece *piece);

// 取得表示棋子表示字符串的名称
wchar_t *getPieString(wchar_t* pieString, size_t n, const Piece *piece);

// 测试本翻译单元各种对象、函数
void testPiece(FILE *fout);

#endif