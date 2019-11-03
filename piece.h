#ifndef PIECE_H
#define PIECE_H

#include "base.h"

//  取得表示棋子的字符
wchar_t getChar(const Piece* piece);

// 取得表示棋子的名称
wchar_t getName(const Piece* piece);

// 取得表示棋子输出文本的名称
wchar_t getTextName(const Piece* piece);

// 测试本翻译单元各种对象、函数
void testPiece(void);

#endif