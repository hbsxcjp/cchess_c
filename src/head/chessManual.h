#ifndef CHESSMANUAL_H
#define CHESSMANUAL_H

#include "base.h"
#include "ecco.h"
#include "tools.h"

extern const char* EXTNAMES[];

// 新建chessManual
ChessManual newChessManual(const char* fileName);

// 从文件读取重置chessManual
ChessManual resetChessManual(ChessManual* cm, const char* fileName);

// 删除chessManual
void delChessManual(ChessManual cm);

Move getRootMove(ChessManual cm);
//const char* getFileName_cm(ChessManual cm);

// 取得正则表达式所需的中文字符组
wchar_t* getZhWChars(wchar_t* ZhWChars);

// 前进一步
bool go(ChessManual cm);
// 前进到变着
bool goOther(ChessManual cm);
int goEnd(ChessManual cm);
// 前进至指定move
int goTo(ChessManual cm, Move move);

// 后退一步
bool back(ChessManual cm);
bool backNext(ChessManual cm);
bool backOther(ChessManual cm);
int backToPre(ChessManual cm);
int backFirst(ChessManual cm);
// 后退至指定move
int backTo(ChessManual cm, Move move);

// 前进或后退数步
int goInc(ChessManual cm, int inc);

// 添加着法
Move appendMove(ChessManual cm, const wchar_t* wstr, RecFormat fmt, wchar_t* remark, bool isOther);

// 获取棋盘转换后当前着法ICCS字符串
const wchar_t* getCurMoveICCS_cm(wchar_t* iccs, ChessManual cm, ChangeType ct);

// 添加/删除一个info条目
void setInfoItem_cm(ChessManual cm, const wchar_t* name, const wchar_t* value);
const wchar_t* getInfoValue_name_cm(ChessManual cm, const wchar_t* name);
void delInfoItem_cm(ChessManual cm, const wchar_t* name);

// 增删改move后，更新zhStr、行列数值
void setMoveNumZhStr(ChessManual cm);

// 将PGN_CC格式的info、move、remark信息写入字符串
void writeInfo_PGNtoWstr(wchar_t** pinfoStr, ChessManual cm);

void writeMoveRemark_PGN_ICCSZHtoWstr(wchar_t** pmoveStr, ChessManual cm, RecFormat fmt);
void writeMove_PGN_CCtoWstr(wchar_t** pmoveStr, ChessManual cm);
void writeRemark_PGN_CCtoWstr(wchar_t** premStr, ChessManual cm);

void writePGNtoWstr(wchar_t** pstr, ChessManual cm, RecFormat fmt);

// 从chessManual存储到文件，根据文件扩展名选择存储格式
void writeChessManual(ChessManual cm, const char* fileName);

// 遍历每个着法
void moveMap(ChessManual cm, void apply(Move, Board, void*), void* ptr);

// 转变棋局实例
void changeChessManual(ChessManual cm, ChangeType ct);

// 批量转换棋局存储格式
void transDir(const char* dirName, RecFormat fromfmt, RecFormat tofmt, bool isPrint);

// 取得棋谱有关的数据
void getChessManualNumStr(char* str, ChessManual cm);

// 取得棋谱的拟匹配开局正则表达式的着法字符串(iccs)
const wchar_t* getIccsStr(wchar_t* iccsStr, ChessManual cm);

bool chessManual_equal(ChessManual cm0, ChessManual cm1);

// 设置棋谱的开局编号
bool setECCO_cm(ChessManual cm, MyLinkedList eccoMyLinkedList);

// 获取棋谱对象链表
MyLinkedList getCmMyLinkedList(const char* dirName, RecFormat fromfmt, MyLinkedList regObj_MyLinkedList);

// 打印输出棋谱链表
void printCmMyLinkedList(FILE* fout, MyLinkedList cmMyLinkedList);

// 存储文件棋谱至数据库
void storeChessManual_db(sqlite3* db, const char* lib_tblName, const char* man_tblName,
    const char* dirName, RecFormat fromfmt);
#endif