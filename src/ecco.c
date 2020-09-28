#include "head/ecco.h"
#include "head/chessManual.h"
#include "head/move.h"
#include "head/tools.h"

#define BOUT_MAX 80
#define INSERTBOUT_INDEX 10
#define INSERTBOUT_SPLIT 30
#define INSERTBOUT(partIndex) (INSERTBOUT_INDEX + (partIndex)*INSERTBOUT_SPLIT)
#define FIELD_MAX 4
#define MOVESTR_LEN 4
#define APPENDICCS_FAILED 0
#define APPENDICCS_SUCCESS 1
#define APPENDICCS_GROUPSPLIT 2
//* 输出字符串，检查用
static FILE* fout;

// 读取开局着法内容至数据表
static void getSplitFields__(wchar_t tables[][4][WCHARSIZE], int* record, const wchar_t* wstring)
{
    // 根据正则表达式分解字符串，获取字段内容
    const char* error;
    int errorffset, ovector[30], index = 0;
    void* regs[] = {
        // field: sn name nums
        pcrewch_compile(L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)",
            0, &error, &errorffset, NULL),
        // field: sn name nums moveStr B2. C0. D1. D2. D3. D4. \\s不包含"　"(全角空格)
        pcrewch_compile(L"([A-E]\\d)．(空|\\S+(?=\\(共))(?:(?![\\s　]+[A-E]\\d．)\\n|\\((共[\\s\\S]+?局)\\)"
                        "[\\s　]*([\\s\\S]*?)(?=[\\s　]*[A-E]\\d{2}．))",
            0, &error, &errorffset, NULL),
        // field: sn name moveStr nums
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)[\\s　]+"
                        "(?:(?![A-E]\\d|上一)([\\s\\S]*?)[\\s　]*(无|共[\\s\\S]+?局)[\\s\\S]*?(?=上|[A-E]\\d{0,2}．))?",
            0, &error, &errorffset, NULL),
        // field: sn moveStr C20 C30 C61 C72局面字符串
        pcrewch_compile(L"([A-E]\\d)\\d局面 =([\\s\\S\n]*?)(?=[\\s　]*[A-E]\\d{2}．)",
            0, &error, &errorffset, NULL)
    };
    for (int g = 0; g < sizeof(regs) / sizeof(regs[0]); ++g) {
        int first = 0, last = wcslen(wstring);
        while (first < last) {
            const wchar_t* tempWstr = wstring + first;
            int count = pcrewch_exec(regs[g], NULL, tempWstr, last - first, 0, 0, ovector, 30);
            if (count <= 0)
                break;

            wchar_t wstr[WCHARSIZE];
            if (g < 3) {
                assert(index < record[2]);
                for (int f = 0; f < count - 1; ++f) {
                    copySubStr(wstr, tempWstr, ovector[2 * f + 2], ovector[2 * f + 3]);
                    wcscpy(tables[index][g == 2 && f > 1 ? (f == 2 ? 3 : 2) : f], wstr);
                }
                index++;
            } else {
                wchar_t sn[10];
                copySubStr(sn, tempWstr, ovector[2], ovector[3]);
                copySubStr(wstr, tempWstr, ovector[4], ovector[5]);
                // C20 C30 C61 C72局面字符串存至g=1数组
                for (int r = record[0]; r < record[1]; ++r)
                    if (wcscmp(tables[r][0], sn) == 0) {
                        wcscpy(tables[r][3], wstr);
                        break;
                    }
            }
            //fwprintf(fout, L"\n");
            first += ovector[1];
        }
        pcrewch_free(regs[g]);
    }
}

// 获取moveStr字段前置省略内容 有40+74=114项
static wchar_t* getPreMoveStr__(wchar_t tables[][4][WCHARSIZE], int* record, const wchar_t* snStr, const wchar_t* moveStr)
{
    wchar_t sn[8] = { 0 };
    // 三级局面的 C2 C3_C4 C61 C72局面 有40项
    if (moveStr[0] == L'从')
        wcsncpy(sn, moveStr + 1, 2);
    // 前置省略的着法 有74项
    else if (moveStr[0] != L'1') {
        // 截断为两个字符长度
        wcsncpy(sn, snStr, 2);
        if (sn[0] == L'C')
            sn[1] = L'0'; // C0/C1/C5/C6/C7/C8/C9 => C0
        else if (wcscmp(sn, L"D5") == 0)
            wcscpy(sn, L"D51"); // D5 => D51
    }
    if (wcslen(sn) > 1)
        for (int r = record[0]; r < record[2]; ++r)
            if (wcscmp(tables[r][0], sn) == 0)
                return tables[r][3];
    return NULL;
}

// 将着法描述组合成着法字符串（“此前...”，“...不分先后”）
static void getMoves__(wchar_t* moves, wchar_t* wstr, bool isPreMoveStr, void* reg_m, void* reg_bp)
{
    int count = 0, ovector[30] = { 0 };
    wchar_t move[WCHARSIZE], wstrBak[WCHARSIZE], orderStr[WCHARSIZE] = { 0 };
    wcscpy(wstrBak, wstr);
    // 处理存在先后顺序的着法
    if ((count = pcrewch_exec(reg_bp, NULL, wstrBak, wcslen(wstrBak), 0, 0, ovector, 30)) > 0) {
        for (int i = 0; i < count; ++i) {
            int start = ovector[2 * i + 2], end = ovector[2 * i + 3];
            if (start == end)
                continue;

            copySubStr(move, wstrBak, start, end);
            wcscat(orderStr, L"+");
            wcscat(orderStr, move);
            wcsncpy(wstrBak + start, L"~~~~~", end - start);
        }
    }

    int first = 0, last = wcslen(wstrBak);
    //wcscpy(moves, L"");
    wcscpy(moves, orderStr);
    while (first < last) {
        wchar_t* tempWstr = wstrBak + first;
        if (pcrewch_exec(reg_m, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
            break;

        copySubStr(move, tempWstr, ovector[2], ovector[3]);
        // '-','／': 不前进，加‘|’   '*': 前进，加‘|’    '+': 前进，不加‘|’
        wcscat(moves, isPreMoveStr && first != 0 ? L"-" : L"*");
        wcscat(moves, move);
        first += ovector[1];
    }
    //if (wcslen(moves) > 0)
    //    moves[wcslen(moves) - 1] = L'\x0';

    //wcscat(moves, orderStr);
}

// 处理前置着法描述字符串————"局面开始：红方：黑方："
static bool getStartPreMove__(wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE], wchar_t* moveStr,
    void* reg_m, void* reg_sp, void* reg_bp)
{
    int ovector[30];
    if (pcrewch_exec(reg_sp, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, 30) > 2) {
        wchar_t splitStr[WCHARSIZE], moves[WCHARSIZE];
        for (int s = 0; s < 2; ++s) {
            copySubStr(splitStr, moveStr, ovector[2 * s + 2], ovector[2 * s + 3]);
            getMoves__(moves, splitStr, true, reg_m, reg_bp);
            wcscpy(boutMoveZhStr[1][2 * s], moves);
            //fwprintf(fout, L"s:%d %ls\n", s, moves);
        }
        return true;
    }
    return false;
}

// 处理"此前..."着法描述字符串
static void getBeforePreMove__(wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE], int choiceIndex[][2], wchar_t* wstr,
    int fieldIndex, int other, int partIndex, void* reg_m, void* reg_bp)
{
    int insertBoutIndex = INSERTBOUT(partIndex);
    wchar_t moves[WCHARSIZE];
    getMoves__(moves, wstr, true, reg_m, reg_bp);
    if (wcschr(wstr, L'除'))
        wcscat(moves, L"除");

    wcscpy(boutMoveZhStr[insertBoutIndex][fieldIndex], moves);
    // (变着，且非变着为空)||(非变着，且属覆盖前置，且前置有变着)，复制“此前”着法
    if (other == 1 && wcslen(boutMoveZhStr[insertBoutIndex][fieldIndex - 1]) == 0)
        wcscpy(boutMoveZhStr[insertBoutIndex][fieldIndex - 1], moves);
    // 变着，且非前置，起始序号设此序号
    if (other == 1 && partIndex == 1 && (choiceIndex[1][0] == 0 || choiceIndex[1][0] > insertBoutIndex))
        choiceIndex[1][0] = insertBoutIndex;
}

// 处理"不分先后"着法描述字符串
static void getNoOrderMove__(wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE],
    int firstBoutIndex, int boutIndex, int fieldIndex, void* reg_m, void* reg_bp)
{
    wchar_t wstr[WCHARSIZE];
    for (int f = 0; f < 3; f += 2) {
        int endBoutIndex = (fieldIndex = 0 && f == 2) ? boutIndex - 1 : boutIndex;
        wcscpy(wstr, L"");
        for (int r = firstBoutIndex; r <= endBoutIndex; ++r)
            if (wcslen(boutMoveZhStr[r][f]) > 0) {
                -wcscat(wstr, L"-");
                wcscat(wstr, boutMoveZhStr[r][f]);
                wcscpy(boutMoveZhStr[r][f], L"");
            }
        assert(wcslen(wstr) > 0);
        getMoves__(boutMoveZhStr[firstBoutIndex][f], wstr, false, reg_m, reg_bp);
    }
}

// 获取回合着法字符串数组,
static void getBoutMove__(wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE], int choiceIndex[][2],
    const wchar_t* snStr, const wchar_t* moveStr, void* reg_m, void* reg_bm, void* reg_bp, int partIndex)
{
    int first = 0, last = wcslen(moveStr), sn = 0, psn = 0, firstBoutIndex = 0, boutIndex = 0, other = 0, ovector[30];
    while (first < last) {
        const wchar_t* tempWstr = moveStr + first;
        int count = pcrewch_exec(reg_bm, NULL, tempWstr, last - first, 0, 0, ovector, 30);
        if (count < 3)
            break;

        if ((sn = tempWstr[ovector[2]]) < psn || (sn == psn && tempWstr[-2] == L'／'))
            other = 1; // 此后着法皆为变着
        psn = sn;
        boutIndex = sn - L'0' + (partIndex == 0 && sn > L'a' ? -40 : 0);
        if (firstBoutIndex == 0)
            firstBoutIndex = boutIndex;
        for (int i = 2; i < count; ++i) {
            wchar_t wstr[WCHARSIZE];
            copySubStr(wstr, tempWstr, ovector[2 * i], ovector[2 * i + 1]);
            if (wcslen(wstr) == 0)
                continue;

            //fwprintf(fout, L"\ti:%d %ls\n", i, wstr);
            int fieldIndex = (i / 5) * 2 + other, x = i % 3;
            if (x == 0) { // "不分先后" i=3, 6
                //fwprintf(fout, L"\ti:%d fb:%d b:%d fi:%d %ls\n", i, firstBoutIndex, boutIndex, fieldIndex, wstr);
                if (snStr && wcscmp(snStr, L"C11") != 0 && wcscmp(snStr, L"C16") != 0
                    && wcscmp(snStr, L"C18") != 0 && wcscmp(snStr, L"C19") != 0 && wcscmp(snStr, L"C54") != 0)
                    getNoOrderMove__(boutMoveZhStr, firstBoutIndex, boutIndex, fieldIndex, reg_m, reg_bp);
            } else if (x == 1) { // "此前..." i=4, 7
                //fwprintf(fout, L"\ti:%d %ls\n", i, wstr);
                getBeforePreMove__(boutMoveZhStr, choiceIndex, wstr, fieldIndex, other, partIndex, reg_m, reg_bp);
            } else if (wcscmp(wstr, L"……") != 0) { // if (x == 2) // 回合的着法 i=2, 5
                int adjustBoutIndex = boutIndex;
                if (wcscmp(wstr, L"象七进九") == 0)
                    wstr[0] = L'相';
                // 1. 炮二平五，马８进７　2. 兵七进一，炮８平６　3. 马八进七 :有误！
                if (snStr && wcscmp(snStr, L"B32") == 0 && fieldIndex == 2)
                    adjustBoutIndex = (boutIndex == 1 ? 2 : 1);
                swprintf(boutMoveZhStr[adjustBoutIndex][fieldIndex], WCHARSIZE, L"+%ls", wstr);
            }
        }

        if (other == 1 && choiceIndex[partIndex][0] == 0)
            choiceIndex[partIndex][0] = boutIndex;
        first += ovector[1];
    }
    if (other == 1)
        choiceIndex[partIndex][1] = boutIndex;
}

// 获取各自组合的回合数组
static void getBoutIndex__(int boutIndex[][2][2][3], int choiceIndex[][2])
{
    fwprintf(fout, L"\nBoutIndex: ");
    for (int comb = 0; comb < 4; ++comb) {
        if ((comb % 2 == 1 && choiceIndex[1][0] == 0)
            || (comb > 1 && choiceIndex[0][0] == 0))
            continue;

        fwprintf(fout, L"\n\ti:%d", comb);
        for (int color = 0; color < 2; ++color) {

            fwprintf(fout, L"  color:%d\t", color);
            for (int part = 0; part < 2; ++part) {
                boutIndex[comb][color][part][0] = (part == 1 ? (choiceIndex[part][0]
                                                           ? choiceIndex[part][0]
                                                           : (choiceIndex[part - 1][1] ? choiceIndex[part - 1][1] + 1 : INSERTBOUT(part)))
                                                             : 1);
                boutIndex[comb][color][part][1] = (part == 0 ? (choiceIndex[part][1]
                                                           ? choiceIndex[part][1]
                                                           : (choiceIndex[part + 1][0] ? choiceIndex[part + 1][0] - 1 : INSERTBOUT(part + 1) - 1))
                                                             : BOUT_MAX - 1);
                boutIndex[comb][color][part][2] = color * 2 + comb / 2 + (part == 1 ? (comb == 2 ? -1 : (comb == 1 ? 1 : 0)) : 0);

                fwprintf(fout, L"%d-%d[%d] ", boutIndex[comb][color][part][0], boutIndex[comb][color][part][1], boutIndex[comb][color][part][2]);
            }
        }
    }
}

static int appendIccs__(wchar_t* iccses, const wchar_t* zhStrs, int index, ChessManual cm, ChangeType ct) //, bool isOther
{
    const wchar_t* zhStr = zhStrs + (MOVESTR_LEN + 1) * index;
    wchar_t wstr[MOVESTR_LEN + 1] = { 0 }, tag = zhStr[0];
    bool isOther = index > 0 && (tag == L'／' || tag == L'-');
    wcsncpy(wstr, zhStr + 1, MOVESTR_LEN);
    if (appendMove(cm, wstr, PGN_ZH, NULL, isOther) == NULL) {
        //if (wcscmp(wstr, L"车９平８") != 0)
        fwprintf(fout, L"\tFailed:%ls", wstr);
        return APPENDICCS_FAILED;
    }

    int groupSplit = (index != 0 && (isOther || tag == L'*') ? APPENDICCS_GROUPSPLIT : 0);
    if (groupSplit)
        wcscat(iccses, L"|");
    wcscat(iccses, getICCS_cm(wstr, cm, ct));
    return APPENDICCS_SUCCESS | groupSplit;
}

// 获取着法的iccses字符串
static int getIccses__(wchar_t* iccses, const wchar_t* zhStrs, ChessManual cm, ChangeType ct, bool isBefore)
{
    wchar_t zhStrsBak[WCHARSIZE]; //, *orderStr;
    if (wcschr(zhStrs, L'／'))
        wcscat(zhStrsBak, L"-");
    wcscpy(zhStrsBak, zhStrs);
    /*
    orderStr = wcschr(zhStrsBak, L'-');
    if (orderStr != NULL) {
        zhStrsBak[orderStr - zhStrsBak] = L'\x0';
        orderStr++;
    }//*/

    int count = wcslen(zhStrsBak) / (MOVESTR_LEN + 1), groupCount = 1;
    //bool hasOrder = false;
    (count == 1) ? wcscpy(iccses, L"") : wcscpy(iccses, L"(?:");
    for (int i = 0; i < count; ++i) {
        int result = appendIccs__(iccses, zhStrsBak, i, cm, ct);
        if (result & APPENDICCS_SUCCESS && result & APPENDICCS_GROUPSPLIT)
            ++groupCount;
    }
    /*/ 处理存在先后顺序的着法
    if (orderStr != NULL) {
        int order = (wcslen(orderStr) + 1) / (MOVESTR_LEN + 1);
        for (int i = 0; i < order; ++i) {
            success = appendIccs__(iccses, orderStr, i, cm, ct, false);
        }
        if (success) {
            wcscat(iccses, L"|");
            count++;
        }
    }
    //*/
    if (count > 1) 
        wcscat(iccses, L")");
    //fwprintf(fout, L"\n\tzhStrs:%ls %ls %ls{%d}", zhStrsBak, orderStr ? orderStr : L"", iccses, count);
    fwprintf(fout, L"\n\tzhStrs:%ls %ls{%d}", zhStrsBak, iccses, groupCount);
    return groupCount;
}

// 获取“此前..."的before字符串
static void getBefore__(wchar_t* before, wchar_t* anyMove, wchar_t* zhStrs, ChessManual cm, ChangeType ct)
{
    wchar_t iccses[WCHARSIZE];
    int count = getIccses__(iccses, zhStrs, cm, ct, true);
    assert(count > 1);

    if (zhStrs[wcslen(zhStrs) - 1] == L'除')
        swprintf(before, WCHARSIZE, L"(?:(?=!%ls)%ls)*", iccses, anyMove);
    else
        swprintf(before, WCHARSIZE, L"%ls{0,%d}", iccses, count);
}

// 添加”此前“和正常着法
static void appendBeforeIccses__(wchar_t* combStr, wchar_t* before, wchar_t* zhStrs, ChessManual cm, ChangeType ct)
{
    wchar_t iccses[WCHARSIZE], tempStr[WCHARSIZE];
    int count = getIccses__(iccses, zhStrs, cm, ct, false);

    if (wcslen(before) == 0) {
        wcscat(combStr, iccses);
    } else {
        swprintf(tempStr, WCHARSIZE, L"(?:%ls%ls)", before, iccses);
        wcscat(combStr, tempStr);
    }
    if (count > 1) {
        swprintf(tempStr, WCHARSIZE, L"{%d}", count);
        wcscat(combStr, tempStr);
    }
}

// 添加一部分的着法Iccs字符串
static void appendPartIccsStr__(wchar_t* combStr, wchar_t* anyMove, wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE],
    int* boutIndex_i, ChessManual cm, ChangeType ct)
{
    wchar_t before[WCHARSIZE] = { 0 }, tempStr[WCHARSIZE], zhStrs_bak[WCHARSIZE] = { 0 };
    for (int bout = boutIndex_i[0]; bout <= boutIndex_i[1]; ++bout) {
        wchar_t* zhStrs = boutMoveZhStr[bout][boutIndex_i[2]];
        if (wcslen(zhStrs) == 0)
            continue;

        // 处理“此前...”着法
        wchar_t* rook1to4 = wcsstr(zhStrs, L"车一平四");
        if (rook1to4) {
            wcscpy(zhStrs_bak, zhStrs);
            wcscpy(tempStr, zhStrs);
            wcscpy(&tempStr[rook1to4 - zhStrs], rook1to4 + (MOVESTR_LEN + 1) * 2);
            getBefore__(before, anyMove, tempStr, cm, ct); // 修订-初始化before
            continue;
        } else if (bout == INSERTBOUT(0) || bout == INSERTBOUT(1)) {
            getBefore__(before, anyMove, zhStrs, cm, ct); // 正常初始化before
            continue;
        } else if (wcslen(zhStrs_bak) > 0) {
            if (wcsstr(zhStrs, L"车一进一") == NULL) {
                getBefore__(before, anyMove, zhStrs_bak, cm, ct); // 恢复原始-初始化before
                wcscpy(zhStrs_bak, L"");
            }
        }
        // 处理正常着法
        appendBeforeIccses__(combStr, before, zhStrs, cm, ct);
    }
}

// 获取Iccs正则字符串
static void getIccsRegStr__(wchar_t* iccsRegStr, wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE], int choiceIndex[][2])
{
    // boutIndex[comb][color][part][start_end_field]
    int boutIndex[4][2][2][3] = { 0 }, combNum = 0;
    //wchar_t* anyMove = L"(?:[a-i]\\d[a-i]\\d)*"; // 精确版
    wchar_t* anyMove = L".*"; // 简易版
    wchar_t combStr[SUPERWIDEWCHARSIZE] = { 0 };
    ChessManual cm = newChessManual(NULL);
    getBoutIndex__(boutIndex, choiceIndex);
    for (int comb = 0; comb < 4; ++comb) {
        if (boutIndex[comb][0][0][0] == 0)
            continue;

        combNum++;
        wcscat(combStr, L"(?:");
        for (int ct = EXCHANGE; ct <= SYMMETRY_V; ++ct) {
            backFirst(cm); // 棋盘位置复原
            wcscat(combStr, L"(?:^");
            for (int color = 0; color < 2; ++color) {
                for (int part = 0; part < 2; ++part)
                    appendPartIccsStr__(combStr, anyMove, boutMoveZhStr, boutIndex[comb][color][part], cm, ct);

                // 两种颜色着法需同时匹配
                wcscat(combStr, anyMove);
                wcscat(combStr, L"&");
            }
            combStr[wcslen(combStr) - 1] = L')'; // 最后一个'&‘替换为')'
            wcscat(combStr, L"|"); // 多种转换棋盘
        }
        combStr[wcslen(combStr) - 1] = L')'; // 最后一个'|‘替换为')'
        wcscat(combStr, L"|"); //, 可选匹配 "\n\t"调试显示用\n\t
    }
    if (wcslen(combStr) > 0)
        combStr[wcslen(combStr) - 1] = L'\x0'; // 删除最后一个'|‘

    // 如果只有一种组合，则把外围的括号去掉
    if (combNum == 1) {
        wcscpy(iccsRegStr, combStr + 3);
        iccsRegStr[wcslen(iccsRegStr) - 1] = L'\x0'; // 删除最后一个')‘
    } else
        wcscpy(iccsRegStr, combStr);

    fwprintf(fout, L"\nIccsRegStr:\n\t%ls", iccsRegStr);
    delChessManual(cm);
}

// 格式化处理tables[r][3]=>moveStr字段
static void formatMoveStrs__(wchar_t tables[][4][WCHARSIZE], int* record)
{
    const char* error;
    int errorffset, no = 0;
    wchar_t ZhWChars[WCHARSIZE], mv[WCHARSIZE], move[WCHARSIZE], rmove[WIDEWCHARSIZE], brmove[WIDEWCHARSIZE];
    getZhWChars(ZhWChars); // 着法字符组
    swprintf(mv, WCHARSIZE, L"([%ls]{4})", ZhWChars);
    // 着法，含多选的复式着法
    swprintf(move, WCHARSIZE, L"(?:[%ls]{4}(?:／[%ls]{4})*)", ZhWChars, ZhWChars);
    // 捕捉一步着法: 1.着法，2.可能的“此前...”着法，3. 可能的“／\\n...$”着法
    swprintf(rmove, WIDEWCHARSIZE, L"(%ls|……)(?:%ls)(\\(?不.先后[\\)，])?([^%ls]*?此前[^\\)]+?\\))?",
        move, L"[，、；\\s　和\\(\\)以／]|$", ZhWChars);
    // 捕捉一个回合着法：1.序号，2.一步着法的1，3-5.着法或“此前”，4-6.着法或“此前”或“／\\n...$”
    swprintf(brmove, WIDEWCHARSIZE, L"([\\da-z]). ?%ls(?:%ls)?", rmove, rmove);
    void *reg_m = pcrewch_compile(mv, 0, &error, &errorffset, NULL),
         *reg_bm = pcrewch_compile(brmove, 0, &error, &errorffset, NULL),
         *reg_sp = pcrewch_compile(L"红方：(.+)\\n黑方：(.+)", 0, &error, &errorffset, NULL),
         *reg_bp = pcrewch_compile(L"(炮二平五)?.*(马二进三).+(车一平二).*(车二进[四六])?", 0, &error, &errorffset, NULL);
    //fwprintf(fout, L"%ls\n%ls\n%ls\n%ls\n\n", ZhWChars, move, rmove, brmove);

    for (int r = record[1]; r < record[2]; ++r) {
        wchar_t* moveStr = tables[r][3];
        if (wcslen(moveStr) < 2)
            continue;

        // 着法可选变着的起止boutIndex [Pre-Next][Start-End]
        int choiceIndex[2][2] = { 0 };
        // 着法记录数组，以序号字符ASCII值为数组索引存入(例如：L'a'-L'1')
        wchar_t boutMoveZhStr[BOUT_MAX][FIELD_MAX][WCHARSIZE] = { 0 },
                iccsRegStr[SUPERWIDEWCHARSIZE],
                *preMoveStr = getPreMoveStr__(tables, record, tables[r][0], moveStr);
        if (preMoveStr && !getStartPreMove__(boutMoveZhStr, preMoveStr, reg_m, reg_sp, reg_bp))
            getBoutMove__(boutMoveZhStr, choiceIndex, NULL, preMoveStr, reg_m, reg_bm, reg_bp, 0);

        getBoutMove__(boutMoveZhStr, choiceIndex, tables[r][0], moveStr, reg_m, reg_bm, reg_bp, 1);

        //*/
        //if (preMoveStr) {
        if (1) {
            fwprintf(fout, L"No.%d %ls\t%ls\nPreMoveStr: %ls\nMoveStr: %ls\nBoutMove: ",
                no++, tables[r][0], tables[r][1], preMoveStr ? preMoveStr : L"", tables[r][3]);

            for (int i = 0; i < BOUT_MAX; ++i)
                if (boutMoveZhStr[i][0][0] || boutMoveZhStr[i][1][0] || boutMoveZhStr[i][2][0] || boutMoveZhStr[i][3][0])
                    fwprintf(fout, L"\n\t%d. %ls|%ls\t%ls|%ls",
                        i, boutMoveZhStr[i][0], boutMoveZhStr[i][1], boutMoveZhStr[i][2], boutMoveZhStr[i][3]);

            fwprintf(fout, L"\nChoiceIndex: ");
            if (choiceIndex[0][0] > 0 || choiceIndex[1][0] > 0)
                fwprintf(fout, L"%d-%d\t%d-", choiceIndex[0][0], choiceIndex[0][1], choiceIndex[1][0]); // 最后一个取值：BOUT_MAX

            getIccsRegStr__(iccsRegStr, boutMoveZhStr, choiceIndex);
            fwprintf(fout, L"\n\n");
        }
        //*/
        //getIccsRegStr__(iccsRegStr, boutMoveZhStr, choiceIndex);
    }

    pcrewch_free(reg_m);
    pcrewch_free(reg_bm);
    pcrewch_free(reg_sp);
}

static void testGetFields__(wchar_t* fileWstring)
{
    // 读取全部所需的字符串数组
    int record[] = { 5, 55, 555 };
    wchar_t tables[555][4][WCHARSIZE] = { 0 };
    getSplitFields__(tables, record, fileWstring);

    formatMoveStrs__(tables, record);
}

// 获取查询记录结果的数量
static int callCount__(void* count, int argc, char** argv, char** azColName)
{
    *(int*)count = (argv[0] ? atoi(argv[0]) : 0);
    return 0;
}

/*// 获取单一查询记录(字符串)
static int getFieldStr__(void* str, int argc, char** argv, char** azColName)
{
    strcpy(str, argv[0]);
    return 0;
}
//*/

static int getRecCount__(sqlite3* db, char* tblName, char* where)
{
    // 查找表
    char sql[WCHARSIZE], *zErrMsg = 0;
    int count = 0;
    sprintf(sql, "SELECT count(*) FROM %s %s;", tblName, where);
    int rc = sqlite3_exec(db, sql, callCount__, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s get records error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }

    return count;
}

// 初始化表
static int createTable__(sqlite3* db, char* tblName, char* colNames)
{
    char sql[WCHARSIZE], *zErrMsg = 0;
    int rc;
    sprintf(sql, "WHERE type = 'table' AND name = '%s'", tblName);
    if (getRecCount__(db, "sqlite_master", sql) > 0) {
        // 删除表
        sprintf(sql, "DROP TABLE %s;", tblName);
        rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "\nTable %s deleted error: %s", tblName, zErrMsg);
            sqlite3_free(zErrMsg);
            return -1;
        } // else
        // fprintf(stdout, "\nTable %s deleted successfully.", tblName);
    }

    // 创建表
    sprintf(sql, "CREATE TABLE %s(%s);", tblName, colNames);
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s created error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    } //else
    //fprintf(stdout, "\nTable %s created successfully.", tblName);

    return 0;
}

// 提取插入记录的字符串
static void getEccoSql__(char** initEccoSql, char* tblName, wchar_t* fileWstring)
{
    const char* error;
    int errorffset, ovector[30],
        first, last = wcslen(fileWstring), index = 0,
               num0 = 555, num1 = 4, num = num0 + num1;
    wchar_t *tempWstr, sn[num][10], name[num][WCHARSIZE], nums[num][WCHARSIZE], moveStr[num][WIDEWCHARSIZE];
    void* regs[] = {
        pcrewch_compile(L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)", 0, &error, &errorffset, NULL),
        // B2. C0. D1. D2. D3. D4. \\s不包含"　"(全角空格)
        pcrewch_compile(L"([A-E]\\d)．(空|\\S+(?=\\(共))(?:(?![\\s　]+[A-E]\\d．)\\n|\\((共[\\s\\S]+?局)\\)"
                        "[\\s　]*([\\s\\S]*?)(?=[\\s　]*[A-E]\\d{2}．))",
            0, &error, &errorffset, NULL),
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)[\\s　]+"
                        "(?:(?![A-E]\\d)([\\s\\S]*?)[\\s　]*(无|共[\\s\\S]+?局)[\\s\\S]*?(?=\\s+上|\\s+[A-E]\\d{0,2}．))?",
            0, &error, &errorffset, NULL),

        // C20 C30 C61 C72局面
        pcrewch_compile(L"([A-E]\\d\\d?局面) =([\\s\\S\n]*?)(?=[\\s　]*[A-E]\\d{2}．)",
            0, &error, &errorffset, NULL),
    };
    for (int i = 0; i < sizeof(regs) / sizeof(regs[0]); ++i) {
        assert(regs[i]);

        first = 0;
        while (first < last) {
            tempWstr = fileWstring + first;
            if (pcrewch_exec(regs[i], NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
                break;

            assert(index < num);
            copySubStr(sn[index], tempWstr, ovector[2], ovector[3]);
            name[index][0] = L'\x0';
            nums[index][0] = L'\x0';
            moveStr[index][0] = L'\x0';
            copySubStr(i < 3 ? name[index] : moveStr[index], tempWstr, ovector[4], ovector[5]);

            if (i < 3) {
                if (wcscmp(name[index], L"空") != 0) { // i=0,1,2
                    copySubStr(nums[index], tempWstr, ovector[i < 2 ? 6 : 8], ovector[i < 2 ? 7 : 9]);

                    if (i > 0) // i=1,2
                        copySubStr(moveStr[index], tempWstr, ovector[i < 2 ? 8 : 6], ovector[i < 2 ? 9 : 7]);
                } else
                    name[index][0] = L'\x0';
            }
            first += ovector[1];

            //fwprintf(fout, L"%d\t%ls\t%ls\t%ls\t%ls\n", index, sn[index], name[index], nums[index], moveStr[index]);
            index++;
        }
        pcrewch_free(regs[i]);
    }
    if (index != num)
        printf("index:%d num:%d.\n", index, num);
    assert(index == num);

    /*/ 修正C20 C30 C61 C72局面
    for (int i = num0; i < num; ++i) // 555-558
        for (int j = 0; j < num0; ++j)
            if (wcscmp(sn[j], sn[i]) == 0) {
                wcscpy(moveStr[j], moveStr[i]);
                break;
            }
            //*/

    // 修正moveStr
    //int No = 0;
    void *reg_r = pcrewch_compile(L"^[2-9a-z].", 0, &error, &errorffset, NULL),
         *reg1 = pcrewch_compile(L"([2-9a-z]. ?)([^，a-z\\f\\r\\t\\v]+)(，[^　／a-z\\f\\r\\t\\v]+)?",
             0, &error, &errorffset, NULL);
    // ([2-9a-z]. ?)([^，a-z\f\r\t\v]+)(，[^　／a-z\f\r\t\v]+)?

    // wcslen(sn[i]) == 3  有74项
    wchar_t tsn[10], fmoveStr[WIDEWCHARSIZE], moves[WCHARSIZE]; //tmoveStr[WIDEWCHARSIZE],
    for (int i = 55; i < num0; ++i) {
        if (pcrewch_exec(reg_r, NULL, moveStr[i], wcslen(moveStr[i]), 0, 0, ovector, 30) <= 0)
            continue;

        // 取得替换模板字符串
        wcscpy(tsn, sn[i]);
        if (tsn[0] == L'C')
            tsn[1] = L'0'; // C0/C1/C5/C6/C7/C8/C9 => C0
        tsn[2] = L'\x0';
        if (wcscmp(tsn, L"D5") == 0)
            wcscpy(tsn, L"D51"); // D51 => D5
        for (int j = 0; j < num0; ++j)
            if (wcscmp(sn[j], tsn) == 0) {
                wcscpy(fmoveStr, moveStr[j]);
                break;
            }

        //fwprintf(fout, L"No:%d\n%ls\t%ls\n\n", No++, sn[i], moveStr[i]);
        first = 0;
        last = wcslen(moveStr[i]);
        while (first < last) {
            tempWstr = moveStr[i] + first;
            if (pcrewch_exec(reg1, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
                break;

            copySubStr(moves, tempWstr, ovector[2], ovector[3]);

            //fwprintf(fout, L"%ls\n", moves);
            first += ovector[1];
        }
        //fwprintf(fout, L"\nfmoveStr:%ls\n", fmoveStr);
    }
    pcrewch_free(reg_r);
    pcrewch_free(reg1);

    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t wtblName[WCHARSIZE], lineStr[WIDEWCHARSIZE],
        *wInitEccoSql = malloc(size * sizeof(wchar_t));
    assert(wInitEccoSql);
    wInitEccoSql[0] = L'\x0';
    mbstowcs(wtblName, tblName, WCHARSIZE);
    for (int i = 0; i < num0; ++i) {
        swprintf(lineStr, WIDEWCHARSIZE, L"INSERT INTO %ls (SN, NAME, NUMS, MOVESTR) "
                                         "VALUES ('%ls', '%ls', '%ls', '%ls' );\n",
            wtblName, sn[i], name[i], nums[i], moveStr[i]);
        supper_wcscat(&wInitEccoSql, &size, lineStr);
    }
    //fwprintf(fout, wInitEccoSql);

    size = wcslen(wInitEccoSql) * sizeof(wchar_t) + 1;
    *initEccoSql = malloc(size);
    wcstombs(*initEccoSql, wInitEccoSql, size);
    free(wInitEccoSql);
}

// 初始化开局类型编码名称
static void initEccoTable__(sqlite3* db, char* tblName, char* colNames, wchar_t* fileWstring)
{
    if (createTable__(db, tblName, colNames) < 0)
        return;

    // 获取插入记录字符串，并插入
    char *initEccoSql = NULL, *zErrMsg;
    getEccoSql__(&initEccoSql, tblName, fileWstring);

    int rc = sqlite3_exec(db, initEccoSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s operate error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
    } else
        fprintf(stdout, "\nTable %s have records: %d\n", tblName, getRecCount__(db, tblName, ""));

    free(initEccoSql);
}

static bool isMatch__(const char* str, const void* reg)
{
    int len = strlen(str) * sizeof(wchar_t) + 1, ovector[30];
    wchar_t wstr[len];
    mbstowcs(wstr, str, len);
    return pcrewch_exec(reg, NULL, wstr, wcslen(wstr), 0, 0, ovector, 30) > 0;
}

// 处理查询记录
static int updateMoveStr__(void* arg, int argc, char** argv, char** azColName)
{

    const char* error;
    int errorffset = 0;
    void* reg = pcrewch_compile(L"^[2-9a-z].", 0, &error, &errorffset, NULL);
    if (!isMatch__(argv[1], reg))
        return 0;

    //fprintf(fout, "%s\t%s\n", argv[0], argv[1]);
    return 0;
}

static void updateEccoField__(sqlite3* db, char* tblName)
{
    char sql[WCHARSIZE];
    //int count = 0;
    sprintf(sql, "SELECT SN, MOVESTR FROM %s "
                 //"WHERE LENGTH(SN) = 2 AND LENGTH(MOVESTR) > 0;",
                 "WHERE LENGTH(SN) = 3;",
        tblName);

    sqlite3_exec(db, sql, updateMoveStr__, NULL, NULL);
}

void eccoInit(char* dbName)
{
    fout = fopen("chessManual/eccolib", "w");
    // 读取开局库源文件内容
    FILE* fin = fopen("chessManual/eccolib_src", "r");
    wchar_t* fileWstring = getWString(fin);
    assert(fileWstring);

    sqlite3* db = NULL;
    int rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
    } else {
        char* tblName = "ecco";
        char colNames[] = "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "SN TEXT NOT NULL,"
                          "NAME TEXT,"
                          "NUMS TEXT,"
                          "MOVESTR TEXT,"
                          "MOVEROWCOL TEXT";
        initEccoTable__(db, tblName, colNames, fileWstring);

        updateEccoField__(db, tblName);
    }
    sqlite3_close(db);

    free(fileWstring);
    fclose(fin);
    fclose(fout);
}

void testEcco(void)
{
    char *wm, *rm;
#ifdef __linux
    wm = "w";
    rm = "r";
#else
    wm = "w, ccs=UTF-8";
    rm = "r, ccs=UTF-8";
#endif

    fout = fopen("chessManual/eccolib", wm);
    setbuf(fout, NULL);

    FILE* fin = fopen("chessManual/eccolib_src", rm);
    wchar_t* fileWstring = getWString(fin);
    assert(fileWstring);

    testGetFields__(fileWstring);

    free(fileWstring);
    fclose(fin);
    fclose(fout);
}