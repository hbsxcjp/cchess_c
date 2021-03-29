#ifndef CHESSMANUAL_H
#define CHESSMANUAL_H

#include "base.h"
#include "ecco.h"
#include "tools.h"

typedef enum {
    TITLE_INDEX,
    EVENT_INDEX,
    DATE_INDEX,
    SITE_INDEX,
    BLACK_INDEX,
    RED_INDEX,
    OPENING_INDEX,
    WRITER_INDEX,
    AUTHOR_INDEX,
    TYPE_INDEX,
    RESULT_INDEX,
    VERSION_INDEX,
    SOURCE_INDEX,
    FEN_INDEX,
    ICCSSTR_INDEX,
    ECCOSN_INDEX,
    ECCONAME_INDEX,
    MOVESTR_INDEX
} CMINFO_INDEX;

bool isChessManualFile(const char* fileName);

// 新建chessManual
ChessManual newChessManual(void);

ChessManual getChessManual_file(const char* fileName);

// 从文件读取重置chessManual
ChessManual resetChessManual(ChessManual* cm, const char* fileName);

// 删除chessManual
void delChessManual(ChessManual cm);

Move getRootMove(ChessManual cm);

MyLinkedList getInfoMyLinkedList_cm(ChessManual cm);

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

// 将PGN_CC格式的info、move、remark信息字符串读入cm，返回已读字符串之后指针
wchar_t* readInfo_PGN(ChessManual cm, wchar_t* wstr);

// 读取字符串至cm
void readPGN_wstr(ChessManual cm, wchar_t* wstr, RecFormat fmt);

// 将PGN_CC格式的info、move、remark信息写入字符串
void writeInfo_PGN_infolist(wchar_t** pinfoStr, size_t* psize, MyLinkedList infoMyLinkedList);
void writeInfo_PGN(wchar_t** pinfoStr, size_t* psize, ChessManual cm);

void writeMoveRemark_PGN_ICCSZH(wchar_t** pmoveStr, ChessManual cm, RecFormat fmt);
void writeMove_PGN_CC(wchar_t** pmoveStr, ChessManual cm);
void writeRemark_PGN_CC(wchar_t** premStr, ChessManual cm);

void writePGNtoWstr(wchar_t** pstr, ChessManual cm, RecFormat fmt);

// cm写入文件
void writePGN(FILE* fout, ChessManual cm, RecFormat fmt);

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

bool chessManual_equal(ChessManual cm0, ChessManual cm1);

#endif