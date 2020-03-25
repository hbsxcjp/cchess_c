#ifndef PIECE_H
#define PIECE_H

#include "base.h"

// 生成一副棋子
Pieces newPieces(void);

// 释放一副棋子
void freePieces(Pieces pieces);

// 取得表示棋子的颜色(判断棋子值的高四位)
PieceColor getColor(Piece piece);

// 取得对方棋子的颜色
PieceColor getOtherColor(Piece piece);

//  取得表示棋子的种类(取棋子值的低四位)
PieceKind getKind(Piece piece);

//  取得表示棋子的字符
wchar_t getChar(Piece piece);

// 取得表示棋子的名称
wchar_t getPieName(Piece piece);

// 取得表示棋子文本的名称
wchar_t getPieName_T(Piece piece);

// 判断是否棋子名
bool isPieceName(wchar_t name);

// 判断是否直行棋子名
bool isLinePiece(wchar_t name);

// 判断是否兵棋子名
bool isPawnPieceName(wchar_t name);

//  取得棋子
Piece getPiece_i(CPieces pieces, int index);
Piece getPiece_ch(CPieces pieces, wchar_t ch);

//  取得表示相同种类的对方棋子
Piece getOtherPiece(CPieces pieces, Piece piece);

// 设置为已放置在棋盘上
Piece setOnBoard(Piece piece, bool onBoard);

// 取得表示棋子表示字符串的名称
wchar_t* getPieString(wchar_t* pieStr, Piece piece);

// 测试本翻译单元各种对象、函数
void testPiece(FILE* fout);

#endif