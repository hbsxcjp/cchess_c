#ifndef CHESSMANUAL_H
#define CHESSMANUAL_H

#include "base.h"

// 新建一个着法
Move newMove();

// 删除move的所有下着move、变着move及自身
void freeMove(Move move);

// 添加着法
Move addMove_rc(Move preMove, const Board board, int frow, int fcol, int trow, int tcol, wchar_t* remark, bool isOther);
Move addMove_rowcol(Move preMove, const Board board, int frowcol, int trowcol, wchar_t* remark, bool isOther);
Move addMove_iccs(Move preMove, const Board board, wchar_t* iccsStr, wchar_t* remark, bool isOther);
Move addMove_zh(Move preMove, const Board board, wchar_t* zhStr, wchar_t* remark, bool isOther);

// 切除move
void cutMove(Move move);

// 根据iccs着法设置内部着法
//void setMove_iccs(Move move, const Board board, const wchar_t* iccsStr);

// 取得ICCS字符串
wchar_t* getICCS(wchar_t* ICCSStr, const Move move);

// 辅助函数：根据中文着法取得走棋方颜色
//PieceColor getColor_zh(const wchar_t* zhStr);

wchar_t* getMoveStr(wchar_t* wstr, const Move move);

// 根据中文着法设置内部着法
//void setMove_zh(Move move, const Board board, const wchar_t* zhStr);

// 根据内部着法表示设置中文字符串
//static void __setMoveZhStr(Move move, const Board board);

// 设置remark
void setRemark(Move move, const wchar_t* remark);

// 按某种变换类型变换着法记录
void changeMove(Move move, ChangeType ct);

// 新建chessManual
ChessManual newChessManual(const char* filename);

// 从文件读取重置chessManual
ChessManual resetChessManual(ChessManual* cm, const char* filename);

// 删除chessManual
void delChessManual(ChessManual cm);

// 添加一个info条目
void addInfoItem(ChessManual cm, const wchar_t* name, const wchar_t* value);

// 将PGN_CC格式的moves信息写入字符串
void writeMove_PGN_CCtoWstr(const ChessManual cm, wchar_t** plineStr);

// 将PGN_CC格式的remark信息写入字符串
void writeRemark_PGN_CCtoWstr(const ChessManual cm, wchar_t** premarkStr);

// 从chessManual存储到文件
void writeChessManual(const ChessManual cm, const char* filename);

// 取得先手方颜色
PieceColor getFirstColor(const ChessManual cm);

// 是否开始
bool isStart(const ChessManual cm);

// 当前着法有无后着
bool hasNext(const ChessManual cm);

// 当前着法有无前着
bool hasPre(const ChessManual cm);

// 当前着法有无变着
bool hasOther(const ChessManual cm);

// 当前着法有无前变
bool hasPreOther(const ChessManual cm);

// 前进一步
void go(ChessManual cm);
// 前进到变着
void goOther(ChessManual cm);
void goEnd(ChessManual cm);
// 前进至指定move
void goTo(ChessManual cm, Move move);

// 后退一步
void back(ChessManual cm);
void backNext(ChessManual cm);
void backOther(ChessManual cm);
void backFirst(ChessManual cm);
// 后退至指定move
void backTo(ChessManual cm, Move move);

// 前进数步
void goInc(ChessManual cm, int inc);

// 转变棋局实例
void changeChessManual(ChessManual cm, ChangeType ct);
RecFormat getRecFormat(const char* ext);

// 转换棋局存储格式
void transDir(const char* dirfrom, RecFormat fmt);
// 批量转换目录的存储格式
void testTransDir(int fromDir, int toDir,
    int fromFmtStart, int fromFmtEnd, int toFmtStart, int toFmtEnd);

// 测试本翻译单元各种对象、函数
void testChessManual(FILE* fout);

#endif