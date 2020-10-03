#include "head/ecco.h"
#include "head/board.h"
#include "head/chessManual.h"
#include "head/move.h"
#include "head/tools.h"

#define BOUT_MAX 80
#define INSERTBOUT_INDEX 10
#define INSERTBOUT_SPLIT 30
#define INSERTBOUT(partIndex) (INSERTBOUT_INDEX + (partIndex)*INSERTBOUT_SPLIT)
#define FIELD_MAX 4
#define MOVESTR_LEN 4
#define BOUTMOVEPART_NUM 2
#define APPENDICCS_FAILED 0
#define APPENDICCS_SUCCESS 1
#define APPENDICCS_GROUPSPLIT 2
#define MOVESTR_ISBEFORE 1
#define MOVESTR_ISECCOSTART 2
#define MOVESTR_NOTPREMOVE 4

typedef struct MoveStr* MoveStr;
struct MoveStr {
    bool isBefore, isOr, isOther;
    PieceColor color;
    wchar_t* mvstr;
    MoveStr nmoveStr;
};

typedef struct BoutMoveStr* BoutMoveStr;
struct BoutMoveStr {
    int no;
    MoveStr rootMoveStr, curMoveStr;
    BoutMoveStr nboutMoveStr;
};

typedef struct EccoRec* EccoRec;
struct EccoRec {
    wchar_t *sn, *name, *pre_mvstrs, *mvstrs, *nums, *regstr;
    BoutMoveStr rootBoutMoveStr, curBoutMoveStr;
    EccoRec neccoRec;
};

//* 输出字符串，检查用
static FILE* fout;

static MoveStr newMoveStr__(bool isBefore, bool isOr, bool isOther, PieceColor color, const wchar_t* mvstr)
{
    MoveStr moveStr = malloc(sizeof(struct MoveStr));
    moveStr->isBefore = isBefore;
    moveStr->isOr = isOr;
    moveStr->isOther = isOther;
    moveStr->color = color;
    moveStr->mvstr = NULL;
    moveStr->nmoveStr = NULL;
    setStruct_wstrField(&moveStr->mvstr, mvstr);
    return moveStr;
}

static void delMoveStr__(MoveStr moveStr)
{
    if (moveStr == NULL)
        return;

    MoveStr nmoveStr = moveStr->nmoveStr;
    free(moveStr->mvstr);
    free(moveStr);

    delMoveStr__(nmoveStr);
}

static BoutMoveStr newBoutMoveStr__()
{
    BoutMoveStr boutMoveStr = malloc(sizeof(struct BoutMoveStr));
    boutMoveStr->no = 0;
    boutMoveStr->rootMoveStr = newMoveStr__(false, false, false, RED, L"");
    boutMoveStr->curMoveStr = boutMoveStr->rootMoveStr;
    boutMoveStr->nboutMoveStr = NULL;
    return boutMoveStr;
}

static void delBoutMoveStr__(BoutMoveStr boutMoveStr)
{
    if (boutMoveStr == NULL)
        return;

    BoutMoveStr nboutMoveStr = boutMoveStr->nboutMoveStr;
    delMoveStr__(boutMoveStr->rootMoveStr);
    free(boutMoveStr);

    delBoutMoveStr__(nboutMoveStr);
}

static EccoRec newEccoRec__()
{
    EccoRec eccoRec = malloc(sizeof(struct EccoRec));
    eccoRec->sn = NULL;
    eccoRec->name = NULL;
    eccoRec->pre_mvstrs = NULL;
    eccoRec->mvstrs = NULL;
    eccoRec->nums = NULL;
    eccoRec->regstr = NULL;
    eccoRec->rootBoutMoveStr = newBoutMoveStr__();
    eccoRec->curBoutMoveStr = eccoRec->rootBoutMoveStr;
    eccoRec->neccoRec = NULL;
    return eccoRec;
}

static void delEccoRec__(EccoRec eccoRec)
{
    if (eccoRec == NULL)
        return;

    EccoRec neccoRec = eccoRec->neccoRec;
    free(eccoRec->sn);
    free(eccoRec->name);
    free(eccoRec->pre_mvstrs);
    free(eccoRec->mvstrs);
    free(eccoRec->nums);
    free(eccoRec->regstr);
    delBoutMoveStr__(eccoRec->rootBoutMoveStr);
    free(eccoRec);

    delEccoRec__(neccoRec);
}

static EccoRec getEccoRec__(EccoRec rootEccoRec, wchar_t* sn)
{
    EccoRec eccoRec = rootEccoRec->neccoRec;
    while (eccoRec && wcscmp(eccoRec->sn, sn) != 0)
        eccoRec = eccoRec->neccoRec;
    return eccoRec;
}

// 读取开局着法内容至数据表
static void setEccoRecField__(EccoRec rootEccoRec, const wchar_t* wstring)
{
    // 根据正则表达式分解字符串，获取字段内容
    const char* error;
    int errorffset, ovector[30], index = 0;
    void* regs[] = {
        // field: sn name nums
        pcrewch_compile(L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)",
            0, &error, &errorffset, NULL),
        // field: sn name nums mvstr B2. C0. D1. D2. D3. D4. \\s不包含"　"(全角空格)
        pcrewch_compile(L"([A-E]\\d)．(空|\\S+(?=\\(共))(?:(?![\\s　]+[A-E]\\d．)\\n|\\((共[\\s\\S]+?局)\\)"
                        "[\\s　]*([\\s\\S]*?)(?=[\\s　]*[A-E]\\d{2}．))",
            0, &error, &errorffset, NULL),
        // field: sn name mvstr nums
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)[\\s　]+"
                        "(?:(?![A-E]\\d|上一)([\\s\\S]*?)[\\s　]*(无|共[\\s\\S]+?局)[\\s\\S]*?(?=上|[A-E]\\d{0,2}．))?",
            0, &error, &errorffset, NULL),
        // field: sn mvstr C20 C30 C61 C72局面字符串
        pcrewch_compile(L"([A-E]\\d)\\d局面 =([\\s\\S\n]*?)(?=[\\s　]*[A-E]\\d{2}．)",
            0, &error, &errorffset, NULL)
    };
    EccoRec eccoRec = rootEccoRec;
    for (int g = 0; g < sizeof(regs) / sizeof(regs[0]); ++g) {
        int first = 0, last = wcslen(wstring);
        while (first < last) {
            const wchar_t* tempWstr = wstring + first;
            int count = pcrewch_exec(regs[g], NULL, tempWstr, last - first, 0, 0, ovector, 30);
            if (count <= 0)
                break;

            wchar_t wstr[WCHARSIZE];
            if (g < 3) {
                assert(index < 555);

                eccoRec->neccoRec = newEccoRec__();
                eccoRec = eccoRec->neccoRec;
                wchar_t** peccoRec_f[] = { &eccoRec->sn, &eccoRec->name, &eccoRec->mvstrs, &eccoRec->nums };
                for (int f = 0; f < count - 1; ++f) {
                    copySubStr(wstr, tempWstr, ovector[2 * f + 2], ovector[2 * f + 3]);
                    setStruct_wstrField(peccoRec_f[g == 1 && f > 1 ? (f == 2 ? 3 : 2) : f], wstr);
                }

                index++;
            } else {
                wchar_t sn[10];
                copySubStr(sn, tempWstr, ovector[2], ovector[3]);
                copySubStr(wstr, tempWstr, ovector[4], ovector[5]);
                // C20 C30 C61 C72局面字符串存至g=1数组
                EccoRec teccoRec = getEccoRec__(rootEccoRec, sn);
                if (teccoRec)
                    setStruct_wstrField(&teccoRec->mvstrs, wstr);
            }
            //fwprintf(fout, L"\n");
            first += ovector[1];
        }
        pcrewch_free(regs[g]);
    }
    assert(index == 555);
}

// 设置pre_mvstrs字段, 前置省略内容 有40+74=114项
static void setEccoRec_pre_mvstrs__(EccoRec eccoRec, EccoRec rootEccoRec, const wchar_t* mvstr)
{
    wchar_t sn[4] = { 0 };
    // 三级局面的 C2 C3_C4 C61 C72局面 有40项
    if (mvstr[0] == L'从')
        wcsncpy(sn, mvstr + 1, 2);
    // 前置省略的着法 有74项
    else if (mvstr[0] != L'1') {
        // 截断为两个字符长度
        wcsncpy(sn, eccoRec->sn, 2);
        if (sn[0] == L'C')
            sn[1] = L'0'; // C0/C1/C5/C6/C7/C8/C9 => C0
        else if (wcscmp(sn, L"D5") == 0)
            wcscpy(sn, L"D51"); // D5 => D51
    }

    EccoRec teccoRec = NULL;
    if (wcslen(sn) > 1 && (teccoRec = getEccoRec__(rootEccoRec, sn)))
        setStruct_wstrField(&eccoRec->pre_mvstrs, teccoRec->mvstrs);
}

// 将着法描述组合成着法字符串（3种：“此前...”，“...不分先后”, “...／...”）
static void getMoves__(MoveStr* pmoveStr, wchar_t* moves, const wchar_t* wstr, int mtype,
    bool isBefore, bool isOr, void* reg_m, void* reg_bp)
{
    int count = 0, ovector[30] = { 0 };
    wchar_t mvstr[WCHARSIZE], wstrBak[WCHARSIZE], orderStr[WCHARSIZE] = { 0 };
    wcscpy(wstrBak, wstr);

    MoveStr moveStr = *pmoveStr;
    // 处理一串着法字符串内存在先后顺序的着法
    if ((count = pcrewch_exec(reg_bp, NULL, wstrBak, wcslen(wstrBak), 0, 0, ovector, 30)) > 0) {
        for (int i = 0; i < count - 1; ++i) {
            int start = ovector[2 * i + 2], end = ovector[2 * i + 3];
            if (start == end)
                continue;

            copySubStr(mvstr, wstrBak, start, end);
            // C83 可选着法"车二进六"不能加入顺序着法，因为需要回退，以便解决与后续“炮８进２”的冲突
            if (wcscmp(mvstr, L"车二进六") == 0
                && wcsstr(wstr, L"红此前可走马二进三、马八进七、车一平二、车二进六、兵三进一和兵七进一"))
                continue;

            //bool isSplit = wcscmp(mvstr, L"马八进七") == 0 ? true : false;
            moveStr->nmoveStr = newMoveStr__(isBefore, isOr, false, getColor_zh(mvstr), mvstr);
            moveStr = moveStr->nmoveStr;

            //?
            wcscat(orderStr, wcscmp(mvstr, L"马八进七") == 0 ? L"*" : L"+");
            wcscat(orderStr, mvstr);
            wcsncpy(wstrBak + start, L"~~~~", end - start);
        }
    }

    wcscpy(moves, orderStr);
    int first = 0, last = wcslen(wstrBak);
    while (first < last) {
        wchar_t* tempWstr = wstrBak + first;
        if (pcrewch_exec(reg_m, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
            break;

        //?
        // '-','／': 不前进，加‘|’   '*': 前进，加‘|’    '+': 前进，不加‘|’
        wcscat(moves, ((mtype & MOVESTR_ISBEFORE) && first != 0 ? L"-" : L"*")); //(mtype & MOVESTR_NOTPREMOVE) ? L"+" :
        copySubStr(mvstr, tempWstr, ovector[2], ovector[3]);
        wcscat(moves, mvstr);

        bool isOther = (mtype & MOVESTR_ISBEFORE) && first != 0 ? true : false;
        //     isSplit = mtype & MOVESTR_NOTPREMOVE || first == 0 ? true : false;
        moveStr->nmoveStr = newMoveStr__(isBefore, isOr, isOther, getColor_zh(mvstr), mvstr);
        moveStr = moveStr->nmoveStr;

        first += ovector[1];
    }

    *pmoveStr = moveStr;
}

// 处理前置着法描述字符串————"局面开始：红方：黑方："
static bool getStartPreMove__(EccoRec eccoRec, wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE],
    void* reg_m, void* reg_sp, void* reg_bp)
{
    int ovector[30];
    const wchar_t* mvstrs = eccoRec->pre_mvstrs;
    if (pcrewch_exec(reg_sp, NULL, mvstrs, wcslen(mvstrs), 0, 0, ovector, 30) > 2) {
        wchar_t rb_mvstrs[WCHARSIZE], moves[WCHARSIZE];

        MoveStr movestr = eccoRec->curBoutMoveStr->curMoveStr;

        for (PieceColor c = RED; c <= BLACK; ++c) {
            copySubStr(rb_mvstrs, mvstrs, ovector[2 * c + 2], ovector[2 * c + 3]);

            getMoves__(&movestr, moves, rb_mvstrs, MOVESTR_ISECCOSTART, true, false, reg_m, reg_bp);
            // 载入第一个记录的字段
            wcscpy(boutMoveZhStr[1][2 * c], moves);
            //fwprintf(fout, L"s:%d %ls\n", s, moves);
        }

        eccoRec->curBoutMoveStr->curMoveStr = movestr;

        return true;
    }
    return false;
}

// 处理"此前..."着法描述字符串
static void getBeforePreMove__(EccoRec eccoRec, BoutMoveStr* pboutMoveStr, wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE], int choiceBoutIndex[][2],
    const wchar_t* wstr, int fieldIndex, int other, int partIndex, void* reg_m, void* reg_bp)
{
    int insertBoutIndex = INSERTBOUT(partIndex);
    wchar_t moves[WCHARSIZE];

    MoveStr movestr = (*pboutMoveStr)->curMoveStr;

    getMoves__(&movestr, moves, wstr, MOVESTR_ISBEFORE, true, other == 1, reg_m, reg_bp); //pre or ?

    (*pboutMoveStr)->curMoveStr = movestr;

    if (wcschr(wstr, L'除'))
        wcscat(moves, L"除");

    wcscpy(boutMoveZhStr[insertBoutIndex][fieldIndex], moves);
    // (变着，且非变着为空)||(非变着，且属覆盖前置，且前置有变着)，复制“此前”着法
    if (other == 1 && wcslen(boutMoveZhStr[insertBoutIndex][fieldIndex - 1]) == 0)
        wcscpy(boutMoveZhStr[insertBoutIndex][fieldIndex - 1], moves);
    // 变着，且非前置，起始序号设此序号
    if (other == 1 && partIndex == 1 && (choiceBoutIndex[1][0] == 0 || choiceBoutIndex[1][0] > insertBoutIndex))
        choiceBoutIndex[1][0] = insertBoutIndex;
}

// 处理"不分先后"着法描述字符串
static void getNoOrderMove__(EccoRec eccoRec, BoutMoveStr* pboutMoveStr, wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE],
    int firstBoutIndex, int boutIndex, int fieldIndex, void* reg_m, void* reg_bp)
{
    wchar_t wstr[WCHARSIZE];
    for (int f = 0; f < 3; f += 2) {
        int endBoutIndex = (fieldIndex = 0 && f == 2) ? boutIndex - 1 : boutIndex;
        wcscpy(wstr, L"");
        for (int r = firstBoutIndex; r <= endBoutIndex; ++r) {
            if (wcslen(boutMoveZhStr[r][f]) == 0)
                continue;

            wcscat(wstr, boutMoveZhStr[r][f]);
            wcscpy(boutMoveZhStr[r][f], L"");
        }

        if (wcslen(wstr) == 0)
            continue;

        //?
        MoveStr movestr = (*pboutMoveStr)->curMoveStr;

        getMoves__(&movestr, boutMoveZhStr[firstBoutIndex][f], wstr, MOVESTR_NOTPREMOVE,
            firstBoutIndex < INSERTBOUT(0), (fieldIndex % 2 == 1), reg_m, reg_bp);

        (*pboutMoveStr)->curMoveStr = movestr;
    }
}

// 获取回合着法字符串数组,
static void getBoutMove__(EccoRec eccoRec, wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE], int choiceBoutIndex[][2],
    const wchar_t* snStr, void* reg_m, void* reg_bm, void* reg_bp, int partIndex)
{
    const wchar_t* mvstr = partIndex == 0 ? eccoRec->pre_mvstrs : eccoRec->mvstrs;
    int first = 0, last = wcslen(mvstr), sn = 0, psn = 0, firstBoutIndex = 0, boutIndex = 0, other = 0, ovector[30];

    BoutMoveStr boutMoveStr = eccoRec->curBoutMoveStr;

    while (first < last) {
        const wchar_t* tempWstr = mvstr + first;
        int count = pcrewch_exec(reg_bm, NULL, tempWstr, last - first, 0, 0, ovector, 30);
        if (count < 3)
            break;

        boutMoveStr->nboutMoveStr = newBoutMoveStr__();
        boutMoveStr->nboutMoveStr->no = boutMoveStr->no + 1;
        boutMoveStr = boutMoveStr->nboutMoveStr;

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
            int fieldIndex = (i / 5) * 2 + other, group = i % 3;
            if (group == 0) { // "不分先后" i=3, 6
                // 忽略"不分先后"
                if (wcscmp(snStr, L"C11") == 0 || wcscmp(snStr, L"C16") == 0 || wcscmp(snStr, L"C18") == 0
                    || wcscmp(snStr, L"C19") == 0 || wcscmp(snStr, L"C54") == 0)
                    continue;

                //?
                getNoOrderMove__(eccoRec, &boutMoveStr, boutMoveZhStr, firstBoutIndex, boutIndex, fieldIndex, reg_m, reg_bp);
            } else if (group == 1) { // "此前..." i=4, 7
                // D29 (此前红可走马八进七，黑可走马２进３、车９平４)
                if (wcscmp(snStr, L"D29") == 0 && partIndex == 1 && fieldIndex >= 2) {
                    wchar_t wstrBak[WCHARSIZE];
                    wcscpy(wstrBak, wstr);
                    copySubStr(wstr, wstrBak, 6 + MOVESTR_LEN, wcslen(wstrBak)); // 复制后两着内容
                    wstrBak[6 + MOVESTR_LEN] = L'\x0'; // 截取前面一着内容
                    getBeforePreMove__(eccoRec, &boutMoveStr, boutMoveZhStr, choiceBoutIndex, wstrBak, fieldIndex - 2, other, partIndex, reg_m, reg_bp);
                }
                // D41 (此前红可走马二进三、马八进七、兵三进一和兵七进一) 转移至前面颜色的字段
                else if (wcscmp(snStr, L"D41") == 0 && partIndex == 1 && fieldIndex >= 2)
                    fieldIndex -= 2;

                getBeforePreMove__(eccoRec, &boutMoveStr, boutMoveZhStr, choiceBoutIndex, wstr, fieldIndex, other, partIndex, reg_m, reg_bp);
            } else if (wcscmp(wstr, L"……") != 0) { // if (group == 2) // 回合的着法 i=2, 5
                // D41 该部分字段的着法与前部分重复
                if (wcscmp(snStr, L"D41") == 0 && partIndex == 1 && fieldIndex >= 2)
                    continue;

                int adjustBoutIndex = boutIndex;
                // 棋子文字错误
                if (wcscmp(wstr, L"象七进九") == 0)
                    wstr[0] = L'相';
                // B32 "1. 炮二平五，马８进７　2. 兵七进一，炮８平６　3. 马八进七" 着法有误:"炮８平６"必须走在“马８进７”之前
                if (wcscmp(snStr, L"B32") == 0 && fieldIndex == 2)
                    adjustBoutIndex = (boutIndex == 1 ? 2 : 1);

                //?
                swprintf(boutMoveZhStr[adjustBoutIndex][fieldIndex], WCHARSIZE, L"+%ls", wstr);
            }
        }

        if (other == 1 && choiceBoutIndex[partIndex][0] == 0)
            choiceBoutIndex[partIndex][0] = boutIndex;
        first += ovector[1];
    }
    if (other == 1)
        choiceBoutIndex[partIndex][1] = boutIndex;

    eccoRec->curBoutMoveStr = boutMoveStr;
}

// 获取各自组合的回合数组
static void getBoutIndex__(int boutIndex[][PIECECOLORNUM][2][3], int choiceBoutIndex[][2])
{
    fwprintf(fout, L"\nBoutIndex: ");
    for (int comb = 0; comb < 4; ++comb) {
        if (comb > 0 && ((comb % 2 == 1 && choiceBoutIndex[1][0] == 0) || (comb >= 2 && choiceBoutIndex[0][0] == 0)))
            continue;

        fwprintf(fout, L"\n\tcomb:%d", comb);
        for (int color = RED; color <= BLACK; ++color) {

            fwprintf(fout, L"  color:%d\t", color);
            for (int part = 0; part < 2; ++part) {
                boutIndex[comb][color][part][0] = (part == 1 ? (choiceBoutIndex[part][0]
                                                           ? choiceBoutIndex[part][0]
                                                           : (choiceBoutIndex[part - 1][1] ? choiceBoutIndex[part - 1][1] + 1 : INSERTBOUT(part)))
                                                             : 1); // start
                boutIndex[comb][color][part][1] = (part == 0 ? (choiceBoutIndex[part][1]
                                                           ? choiceBoutIndex[part][1]
                                                           : (choiceBoutIndex[part + 1][0] ? choiceBoutIndex[part + 1][0] - 1 : INSERTBOUT(part + 1) - 1))
                                                             : BOUT_MAX - 1); // end
                boutIndex[comb][color][part][2] = color * 2 + comb / 2 + (part == 1 ? (comb == 2 ? -1 : (comb == 1 ? 1 : 0)) : 0); // field

                fwprintf(fout, L"%d-%d[%d] ", boutIndex[comb][color][part][0], boutIndex[comb][color][part][1], boutIndex[comb][color][part][2]);
            }
        }
    }
}

static int appendIccs__(wchar_t* iccses, const wchar_t* zhStrs, int index, ChessManual cm, ChangeType ct)
{
    if (wcslen(zhStrs) == 0)
        return APPENDICCS_FAILED;

    // 静态变量，标记之前的首着分组是否成功
    static bool firstGroupSplit_success = true;
    const wchar_t* zhStr = zhStrs + (MOVESTR_LEN + 1) * index;
    wchar_t wstr[MOVESTR_LEN + 1] = { 0 }, tag = zhStr[0];
    bool isOther = index > 0 && (tag == L'／' || tag == L'-');
    wcsncpy(wstr, zhStr + 1, MOVESTR_LEN);
    if (appendMove(cm, wstr, PGN_ZH, NULL, isOther && firstGroupSplit_success) == NULL) {

        fwprintf(fout, L"\tFailed:%ls", wstr);
        if (index == 0)
            firstGroupSplit_success = false; //首着分组失败
        return APPENDICCS_FAILED;
    }

    int groupSplit = (index != 0 && (isOther || tag == L'*') ? APPENDICCS_GROUPSPLIT : 0);
    if (groupSplit) {
        if (firstGroupSplit_success) { // 首着分组成功
            wcscat(iccses, L"|");
        } else {
            groupSplit = 0; // 本着应为首着，已在前调用函数计入分组数
        }
    }
    firstGroupSplit_success = true; // 本着成功后重置状态
    wcscat(iccses, getICCS_cm(wstr, cm, ct));

    return APPENDICCS_SUCCESS | groupSplit;
}

// 获取着法的iccses字符串
static int getIccses__(wchar_t* iccses, const wchar_t* zhStrs, ChessManual cm, ChangeType ct, bool isBefore)
{
    int count = wcslen(zhStrs) / (MOVESTR_LEN + 1), groupCount = 1; // 首着计入分组数
    (count <= 1) ? wcscpy(iccses, L"") : wcscpy(iccses, L"(?:");
    for (int i = 0; i < count; ++i) {
        int result = appendIccs__(iccses, zhStrs, i, cm, ct);
        if ((result & APPENDICCS_SUCCESS) && (result & APPENDICCS_GROUPSPLIT))
            ++groupCount;
    }

    if (count > 1)
        wcscat(iccses, L")");

    fwprintf(fout, L"\n\tzhStrs:%ls %ls{%d}", zhStrs, iccses, groupCount);
    if (wcslen(iccses) == 0)
        return 0;

    return groupCount;
}

// 获取“此前..."的before字符串
static void getBeforeIccses__(wchar_t* before, wchar_t* anyMove, const wchar_t* zhStrs, ChessManual cm, ChangeType ct)
{
    if (wcslen(zhStrs) == 0)
        return;
    wchar_t iccses[WCHARSIZE];
    int count = getIccses__(iccses, zhStrs, cm, ct, true);
    if (count == 0)
        return;

    if (zhStrs[wcslen(zhStrs) - 1] == L'除')
        swprintf(before, WCHARSIZE, L"(?:(?=!%ls)%ls)*", iccses, anyMove);
    else
        swprintf(before, WCHARSIZE, L"%ls{0,%d}", iccses, count);
}

// 添加一种颜色的完整着法，包括”此前“和正常着法
static void appendColorIccses__(wchar_t* redBlackStr_c, wchar_t* before, const wchar_t* zhStrs, ChessManual cm, ChangeType ct)
{
    if (wcslen(zhStrs) == 0)
        return;
    wchar_t iccses[WCHARSIZE], tempStr[WCHARSIZE];
    int count = getIccses__(iccses, zhStrs, cm, ct, false);
    if (count == 0)
        return;

    if (wcslen(before) == 0) {
        wcscat(redBlackStr_c, iccses);
    } else {
        swprintf(tempStr, WCHARSIZE, L"(?:%ls%ls)", before, iccses);
        wcscat(redBlackStr_c, tempStr);
    }
    if (count > 1) {
        swprintf(tempStr, WCHARSIZE, L"{%d}", count);
        wcscat(redBlackStr_c, tempStr);
    }
}

// 添加一个组合的着法Iccs字符串
static void appendCombIccses__(wchar_t* combStr, wchar_t* anyMove, wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE],
    int boutIndex_c[][2][3], ChessManual cm, ChangeType ct)
{
    wchar_t redBlackStr[PIECECOLORNUM][WIDEWCHARSIZE] = { 0 };
    for (int part = 0; part < 2; ++part) {
        const wchar_t* zhStrs_bak[PIECECOLORNUM] = { NULL };
        wchar_t before[PIECECOLORNUM][WCHARSIZE] = { 0 };
        int startBoutIndex = fmin(boutIndex_c[RED][part][0], boutIndex_c[BLACK][part][0]),
            endBoutIndex = fmax(boutIndex_c[RED][part][1], boutIndex_c[BLACK][part][1]);
        for (int bout = startBoutIndex; bout <= endBoutIndex; ++bout) {
            for (int color = RED; color <= BLACK; ++color) {
                // 不在范围的回合数跳过
                if (bout < boutIndex_c[color][part][0] || bout > boutIndex_c[color][part][1])
                    continue;

                const wchar_t* zhStrs = boutMoveZhStr[bout][boutIndex_c[color][part][2]];
                if (wcslen(zhStrs) == 0)
                    continue;

                if (bout == INSERTBOUT(0) || bout == INSERTBOUT(1)) {
                    // C11~C14=>"车一平四" D40~D43=>"车８进５"
                    if (wcsstr(zhStrs, L"车一平四") || wcsstr(zhStrs, L"车８进５"))
                        zhStrs_bak[color] = zhStrs;
                    else
                        getBeforeIccses__(before[color], anyMove, zhStrs, cm, ct);
                    continue;
                }

                // 处理“此前...”着法(每次单独处理) C11~C14=>"车一平四" D40~D43=>"车８进５"
                if (zhStrs_bak[color]) {
                    getBeforeIccses__(before[color], anyMove, zhStrs_bak[color], cm, ct);
                    if (bout < 62) // C11 bout==62时不回退
                        backToPre(cm); // 回退至前着，避免重复走最后一着
                }

                appendColorIccses__(redBlackStr[color], before[color], zhStrs, cm, ct);
            }
        }
    }

    for (int color = RED; color <= BLACK; ++color) {
        if (wcslen(redBlackStr[color]) == 0)
            continue;

        wcscat(combStr, redBlackStr[color]);
        wcscat(combStr, anyMove);
        wcscat(combStr, L"&");
    }
}

// 获取Iccs正则字符串
static void getEccoRecStr__(wchar_t* eccoRecStr, wchar_t boutMoveZhStr[][FIELD_MAX][WCHARSIZE],
    int choiceBoutIndex[][2])
{
    // boutIndex[comb][color][part][start_end_field]
    int boutIndex[4][PIECECOLORNUM][2][3] = { 0 }, combNum = 0;
    //wchar_t* anyMove = L"(?:[a-i]\\d[a-i]\\d)*"; // 精确版
    wchar_t* anyMove = L".*"; // 简易版
    wchar_t combStr[SUPERWIDEWCHARSIZE] = { 0 };
    ChessManual cm = newChessManual(NULL);
    getBoutIndex__(boutIndex, choiceBoutIndex);

    fwprintf(fout, L"\nGetCombStr:");
    for (int comb = 0; comb < 4; ++comb) {
        if (boutIndex[comb][0][0][0] == 0)
            continue;

        combNum++;
        wcscat(combStr, L"(?:");
        for (int ct = EXCHANGE; ct <= SYMMETRY_V; ++ct) {
            backFirst(cm); // 棋盘位置复原
            wcscat(combStr, L"(?:^");

            appendCombIccses__(combStr, anyMove, boutMoveZhStr, boutIndex[comb], cm, ct);

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
        wcscpy(eccoRecStr, combStr + 3);
        eccoRecStr[wcslen(eccoRecStr) - 1] = L'\x0'; // 删除最后一个')‘
    } else
        wcscpy(eccoRecStr, combStr);

    fwprintf(fout, L"\nEccoRecStr:\n\t%ls", eccoRecStr);
    delChessManual(cm);
}

static void getEccoRec_regstr__(EccoRec rootEccoRec)
{
    const char* error;
    int errorffset, no = 0;
    wchar_t ZhWChars[WCHARSIZE], mvstr[WCHARSIZE], mvStrs[WCHARSIZE], rich_mvStr[WIDEWCHARSIZE], bout_rich_mvStr[WIDEWCHARSIZE];
    getZhWChars(ZhWChars); // 着法字符组
    swprintf(mvstr, WCHARSIZE, L"([%ls]{4})", ZhWChars);
    // 着法，含多选的复式着法
    swprintf(mvStrs, WCHARSIZE, L"(?:[%ls]{4}(?:／[%ls]{4})*)", ZhWChars, ZhWChars);
    // 捕捉一步着法: 1.着法，2.可能的“此前...”着法，3. 可能的“／\\n...$”着法
    swprintf(rich_mvStr, WIDEWCHARSIZE, L"(%ls|……)(?:%ls)(\\(?不.先后[\\)，])?([^%ls]*?此前[^\\)]+?\\))?",
        mvStrs, L"[，、；\\s　和\\(\\)以／]|$", ZhWChars);
    // 捕捉一个回合着法：1.序号，2.一步着法的首着，3-5.着法或“此前”，4-6.着法或“此前”或“／\\n...$”
    swprintf(bout_rich_mvStr, WIDEWCHARSIZE, L"([\\da-z]). ?%ls(?:%ls)?", rich_mvStr, rich_mvStr);
    void *reg_m = pcrewch_compile(mvstr, 0, &error, &errorffset, NULL),
         *reg_bm = pcrewch_compile(bout_rich_mvStr, 0, &error, &errorffset, NULL),
         *reg_sp = pcrewch_compile(L"红方：(.+)\\n黑方：(.+)", 0, &error, &errorffset, NULL),
         *reg_bp = pcrewch_compile(L"(炮二平五)?.(马二进三).*(车一平二).(车二进六)?"
                                   "|(马８进７).+(车９平８)"
                                   "|此前可走(马二进三)、(马八进七)、兵三进一和兵七进一",
             0, &error, &errorffset, NULL);
    //fwprintf(fout, L"%ls\n%ls\n%ls\n%ls\n\n", ZhWChars, mvStrs, rich_mvStr, bout_rich_mvStr);

    int iccsLen = 0, msLen = 0;
    EccoRec eccoRec = rootEccoRec;
    while ((eccoRec = eccoRec->neccoRec)) {
        wchar_t* mvstrs = eccoRec->mvstrs;
        if (wcslen(eccoRec->sn) < 3 || !mvstrs)
            continue;

        // 着法可选变着的起止boutIndex [Pre-Next][Start-End]
        int choiceBoutIndex[2][2] = { 0 };
        // 着法记录数组，以序号字符ASCII值为数组索引存入(例如：L'a'-L'1')
        wchar_t boutMoveZhStr[BOUT_MAX][FIELD_MAX][WCHARSIZE] = { 0 },
                eccoRecStr[4 * WIDEWCHARSIZE] = { 0 };

        setEccoRec_pre_mvstrs__(eccoRec, rootEccoRec, mvstrs);
        //BoutMoveStr boutMoveStr = eccoRec->curBoutMoveStr;

        if (eccoRec->pre_mvstrs && !getStartPreMove__(eccoRec, boutMoveZhStr, reg_m, reg_sp, reg_bp))
            getBoutMove__(eccoRec, boutMoveZhStr, choiceBoutIndex, L"", reg_m, reg_bm, reg_bp, 0);

        getBoutMove__(eccoRec, boutMoveZhStr, choiceBoutIndex, eccoRec->sn, reg_m, reg_bm, reg_bp, 1);

        //*/
        //if (pre_mvstrs) {
        if (1) {
            fwprintf(fout, L"No.%d %ls\t%ls\nPreMoveStr: %ls\nMoveStr: %ls\nBoutMove: ",
                no++, eccoRec->sn, eccoRec->name, eccoRec->pre_mvstrs ? eccoRec->pre_mvstrs : L"", eccoRec->mvstrs);

            for (int i = 0; i < BOUT_MAX; ++i)
                if (boutMoveZhStr[i][0][0] || boutMoveZhStr[i][1][0] || boutMoveZhStr[i][2][0] || boutMoveZhStr[i][3][0])
                    fwprintf(fout, L"\n\t%d. %ls|%ls\t%ls|%ls",
                        i, boutMoveZhStr[i][0], boutMoveZhStr[i][1], boutMoveZhStr[i][2], boutMoveZhStr[i][3]);

            fwprintf(fout, L"\nChoiceIndex: ");
            if (choiceBoutIndex[0][0] > 0 || choiceBoutIndex[1][0] > 0)
                fwprintf(fout, L"%d-%d\t%d-%d", choiceBoutIndex[0][0], choiceBoutIndex[0][1], choiceBoutIndex[1][0], choiceBoutIndex[1][1]); // 最后一个取值：BOUT_MAX

            getEccoRecStr__(eccoRecStr, boutMoveZhStr, choiceBoutIndex);
            fwprintf(fout, L"\n\n");
        }
        //*/
        //getEccoRecStr__(eccoRecStr, boutMoveZhStr, choiceBoutIndex);
        msLen = fmax(msLen, wcslen(mvstrs));
        iccsLen = fmax(iccsLen, wcslen(eccoRecStr));

        setStruct_wstrField(&eccoRec->regstr, eccoRecStr);
    }
    fwprintf(fout, L"eccoRecStr iccsLen:%d mslen:%d", iccsLen, msLen);

    pcrewch_free(reg_m);
    pcrewch_free(reg_bm);
    pcrewch_free(reg_sp);
    pcrewch_free(reg_bp);
}

// 获取查询记录结果的数量
static int callCount__(void* count, int argc, char** argv, char** azColName)
{
    *(int*)count = (argv[0] ? atoi(argv[0]) : 0);
    return 0;
}

static int getRecCount__(sqlite3* db, char* tblName, char* where)
{
    int count = 0;
    char sql[WCHARSIZE], *zErrMsg = 0;
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
    sprintf(sql, "WHERE type = 'table' AND name = '%s'", tblName);
    if (getRecCount__(db, "sqlite_master", sql) > 0) {
        // 删除表
        sprintf(sql, "DROP TABLE %s;", tblName);
        int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "\nTable %s deleted error: %s", tblName, zErrMsg);
            sqlite3_free(zErrMsg);
            return -1;
        } // else
        // fprintf(stdout, "\nTable %s deleted successfully.", tblName);
    }

    // 创建表
    sprintf(sql, "CREATE TABLE %s(%s);", tblName, colNames);
    int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s created error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    } //else
    //fprintf(stdout, "\nTable %s created successfully.", tblName);

    return 0;
}

/*
// 提取插入记录的字符串
static void getEccoSql__(char** initEccoSql, char* tblName, wchar_t* fileWstring)
{
    const char* error;
    int errorffset, ovector[30],
        first, last = wcslen(fileWstring), index = 0,
               num0 = 555, num1 = 4, num = num0 + num1;
    wchar_t *tempWstr, sn[num][10], name[num][WCHARSIZE], nums[num][WCHARSIZE], mvstr[num][WIDEWCHARSIZE];
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
            mvstr[index][0] = L'\x0';
            copySubStr(i < 3 ? name[index] : mvstr[index], tempWstr, ovector[4], ovector[5]);

            if (i < 3) {
                if (wcscmp(name[index], L"空") != 0) { // i=0,1,2
                    copySubStr(nums[index], tempWstr, ovector[i < 2 ? 6 : 8], ovector[i < 2 ? 7 : 9]);

                    if (i > 0) // i=1,2
                        copySubStr(mvstr[index], tempWstr, ovector[i < 2 ? 8 : 6], ovector[i < 2 ? 9 : 7]);
                } else
                    name[index][0] = L'\x0';
            }
            first += ovector[1];

            //fwprintf(fout, L"%d\t%ls\t%ls\t%ls\t%ls\n", index, sn[index], name[index], nums[index], mvstr[index]);
            index++;
        }
        pcrewch_free(regs[i]);
    }
    if (index != num)
        printf("index:%d num:%d.\n", index, num);
    assert(index == num);

    // 修正moveStr
    //int No = 0;
    void *reg_r = pcrewch_compile(L"^[2-9a-z].", 0, &error, &errorffset, NULL),
         *reg1 = pcrewch_compile(L"([2-9a-z]. ?)([^，a-z\\f\\r\\t\\v]+)(，[^　／a-z\\f\\r\\t\\v]+)?",
             0, &error, &errorffset, NULL);
    // ([2-9a-z]. ?)([^，a-z\f\r\t\v]+)(，[^　／a-z\f\r\t\v]+)?

    // wcslen(sn[i]) == 3  有74项
    wchar_t tsn[10], fmoveStr[WIDEWCHARSIZE], moves[WCHARSIZE]; //tmoveStr[WIDEWCHARSIZE],
    for (int i = 55; i < num0; ++i) {
        if (pcrewch_exec(reg_r, NULL, mvstr[i], wcslen(mvstr[i]), 0, 0, ovector, 30) <= 0)
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
                wcscpy(fmoveStr, mvstr[j]);
                break;
            }

        //fwprintf(fout, L"No:%d\n%ls\t%ls\n\n", No++, sn[i], mvstr[i]);
        first = 0;
        last = wcslen(mvstr[i]);
        while (first < last) {
            tempWstr = mvstr[i] + first;
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
            wtblName, sn[i], name[i], nums[i], mvstr[i]);
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
    int len = strlen(str) + 1, ovector[30]; // * sizeof(wchar_t)
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
//*/

static void getEccoSql__(char** initEccoSql, char* tblName, EccoRec rootEccoRec)
{
}

// 初始化开局类型编码名称
static void initEccoTable__(sqlite3* db, char* tblName, char* colNames, EccoRec rootEccoRec)
{
    if (createTable__(db, tblName, colNames) < 0)
        return;

    // 获取插入记录字符串，并插入
    char *initEccoSql = NULL, *zErrMsg;
    getEccoSql__(&initEccoSql, tblName, rootEccoRec);

    int rc = sqlite3_exec(db, initEccoSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s operate error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
    } else
        fprintf(stdout, "\nTable %s have records: %d\n", tblName, getRecCount__(db, tblName, ""));

    free(initEccoSql);
}

static void printMoveStr__(MoveStr rootMoveStr)
{
    MoveStr moveStr = rootMoveStr;
    while ((moveStr = moveStr->nmoveStr)) {
        fwprintf(fout, L"\t\t\tmvstr: %ls color:%d isBefore:%d isOr:%d isOther:%d\n",
            moveStr->mvstr, moveStr->color, moveStr->isBefore, moveStr->isOr, moveStr->isOther);
    }
}

static void printBoutMoveStr__(BoutMoveStr rootBoutMoveStr)
{
    BoutMoveStr boutMoveStr = rootBoutMoveStr;
    fwprintf(fout, L"\tboutMoveStr:\n");
    while ((boutMoveStr = boutMoveStr->nboutMoveStr)) {
        fwprintf(fout, L"\t\tbout no:%d\n", boutMoveStr->no);

        printMoveStr__(boutMoveStr->rootMoveStr);
    }
    //fwprintf(fout, L"\n");
}

static void printEccoRec__(EccoRec rootEccoRec)
{
    int no = 0;
    EccoRec eccoRec = rootEccoRec;
    fwprintf(fout, L"\n\nrootEccoRec:\n");
    while ((eccoRec = eccoRec->neccoRec)) {
        if (wcslen(eccoRec->sn) < 3 || !eccoRec->mvstrs)
            continue;

        fwprintf(fout, L"No.%d %ls\t%ls\t%ls\npre_mvstrs: %ls\nmvstrs: %ls\n",
            no++, eccoRec->sn, eccoRec->name,
            eccoRec->nums ? eccoRec->nums : L"",
            eccoRec->pre_mvstrs ? eccoRec->pre_mvstrs : L"",
            eccoRec->mvstrs ? eccoRec->mvstrs : L"");

        printBoutMoveStr__(eccoRec->rootBoutMoveStr);

        fwprintf(fout, L"regstr: %ls\n\n", eccoRec->regstr ? eccoRec->regstr : L"");
    }

    //msLen = fmax(msLen, wcslen(mvstrs));
    //iccsLen = fmax(iccsLen, wcslen(eccoRecStr));
    //fwprintf(fout, L"\n\n");
}

static void storeEccolib__(sqlite3* db, wchar_t* fileWstring)
{
    // 读取全部所需的局面记录
    EccoRec rootEccoRec = newEccoRec__();
    setEccoRecField__(rootEccoRec, fileWstring);
    getEccoRec_regstr__(rootEccoRec);

    printEccoRec__(rootEccoRec);

    char* tblName = "ecco";
    char colNames[] = "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "SN TEXT NOT NULL,"
                      "NAME TEXT,"
                      "NUMS TEXT,"
                      "MOVESTR TEXT,"
                      "ICCSREGSTR TEXT";
    initEccoTable__(db, tblName, colNames, rootEccoRec);
    //updateEccoField__(db, tblName);

    delEccoRec__(rootEccoRec);
}

void initEcco(char* dbName)
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

    sqlite3* db = NULL;
    int rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
    } else {
        storeEccolib__(db, fileWstring);
    }
    sqlite3_close(db);

    free(fileWstring);
    fclose(fin);
    fclose(fout);
}