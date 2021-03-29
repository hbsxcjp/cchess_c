#include "head/ecco.h"
#include "head/board.h"
#include "head/chessManual.h"
#include "head/htmlfile.h"
#include "head/move.h"
#include "head/operatefile.h"
#include "head/operatesqlite3.h"
#include "head/pcre_wch.h"
#include <math.h>

#define BOUT_MAX 80
#define INSERTBOUT_INDEX 10
#define INSERTBOUT_SPLIT 30
#define INSERTBOUT(partIndex) (INSERTBOUT_INDEX + (partIndex)*INSERTBOUT_SPLIT)
#define BOUTMOVEPART_NUM 2
#define BOUTMOVEOR_NUM 2

extern FILE* test_out;

extern const char* EXTNAMES[];
extern const wchar_t* CMINFO_NAMES[];
extern const int CMINFO_LEN;
extern const wchar_t* FEN_INITVALUE;

typedef struct BoutStr* BoutStr;
struct BoutStr {
    int boutNo;
    wchar_t* mvstr[PIECECOLORNUM][BOUTMOVEOR_NUM];
};

typedef struct Ecco* Ecco;
struct Ecco {
    int orBoutNo[BOUTMOVEPART_NUM][2];
    MyLinkedList infoMyLinkedList;
    MyLinkedList boutStrMyLinkedList;

    void* reg;
};

static const wchar_t* ECCOINFO_NAMES[] = {
    L"SN", L"NAME", L"PRE_MVSTRS", L"MVSTRS", L"NUMS", L"REGSTR"
};
static const int SN_INDEX = 0,
                 NAME_INDEX = 1,
                 PRE_MVSTRS_INDEX = 2,
                 MVSTRS_INDEX = 3,
                 NUMS_INDEX = 4,
                 REGSTR_INDEX = 5,
                 ECCOINFO_LEN = sizeof(ECCOINFO_NAMES) / sizeof(ECCOINFO_NAMES[0]);

static BoutStr newBoutStr__(int boutNo)
{
    BoutStr boutStr = malloc(sizeof(struct BoutStr));
    boutStr->boutNo = boutNo;
    for (int c = RED; c <= BLACK; ++c)
        for (int i = 0; i < BOUTMOVEOR_NUM; ++i)
            boutStr->mvstr[c][i] = NULL;

    return boutStr;
}

static void setBoutStr_mvstr__(BoutStr boutStr, PieceColor color, int or, const wchar_t* mvstr)
{
    setPwstr_value(&boutStr->mvstr[color][or], mvstr);
}

static void delBoutStr__(BoutStr boutStr)
{
    for (int c = RED; c <= BLACK; ++c)
        for (int i = 0; i < BOUTMOVEOR_NUM; ++i)
            free(boutStr->mvstr[c][i]);
    free(boutStr);
}

static void setInfoItem_ecco__(Ecco ecco, const wchar_t* name, const wchar_t* value)
{
    setInfoItem(ecco->infoMyLinkedList, name, value);
}

static Ecco newEcco__(void)
{
    Ecco ecco = malloc(sizeof(struct Ecco));
    for (int p = 0; p < BOUTMOVEPART_NUM; ++p)
        for (int i = 0; i < 2; ++i)
            ecco->orBoutNo[p][i] = 0;

    ecco->infoMyLinkedList = newInfoMyLinkedList();
    for (int i = 0; i < ECCOINFO_LEN; ++i)
        setInfoItem_ecco__(ecco, ECCOINFO_NAMES[i], L"");
    ecco->boutStrMyLinkedList = newMyLinkedList((void (*)(void*))delBoutStr__);

    ecco->reg = NULL;
    return ecco;
}

static void delEcco__(Ecco ecco)
{
    delMyLinkedList(ecco->infoMyLinkedList);
    delMyLinkedList(ecco->boutStrMyLinkedList);
    pcrewch_free(ecco->reg);
    free(ecco);
}

static const wchar_t* getInfoValue_name_ecco__(Ecco ecco, const wchar_t* name)
{
    return getInfoValue_name(ecco->infoMyLinkedList, name);
}

static const wchar_t* getEccoSn__(Ecco ecco)
{
    return getInfoValue_name_ecco__(ecco, ECCOINFO_NAMES[SN_INDEX]);
}

static const wchar_t* getEccoName__(Ecco ecco)
{
    return getInfoValue_name_ecco__(ecco, ECCOINFO_NAMES[NAME_INDEX]);
}

static int eccoSN_cmp__(Ecco ecco, const wchar_t* sn)
{
    return wcscmp(getEccoSn__(ecco), sn);
}

// 设置pre_mvstrs字段, 前置省略内容 有40+74=114项
static void setEcco_pre_mvstrs__(Ecco ecco, MyLinkedList eccoList, void* _0, void* _1)
{
    const wchar_t *ecco_sn = getEccoSn__(ecco),
                  *mvstrs = getInfoValue_name_ecco__(ecco, ECCOINFO_NAMES[MVSTRS_INDEX]);
    if (wcslen(ecco_sn) != 3 || wcslen(mvstrs) < 3)
        return;

    wchar_t sn[4] = { 0 };
    // 三级局面的 C2 C3_C4 C61 C72局面 有40项
    if (mvstrs[0] == L'从')
        wcsncpy(sn, mvstrs + 1, 2);
    // 前置省略的着法 有74项
    else if (mvstrs[0] != L'1') {
        // 截断为两个字符长度
        wcsncpy(sn, ecco_sn, 2);
        if (sn[0] == L'C')
            sn[1] = L'0'; // C0/C1/C5/C6/C7/C8/C9 => C0
        else if (wcscmp(sn, L"D5") == 0)
            wcscpy(sn, L"D51"); // D5 => D51
    } else
        return;

    Ecco tecco = getDataMyLinkedList_cond(eccoList, (int (*)(void*, void*))eccoSN_cmp__, (void*)sn);
    if (tecco == NULL)
        return;

    const wchar_t* pre_mvstrs = getInfoValue_name_ecco__(tecco, ECCOINFO_NAMES[MVSTRS_INDEX]);
    setInfoItem_ecco__(ecco, ECCOINFO_NAMES[PRE_MVSTRS_INDEX], pre_mvstrs);
    /*
    //if (mvstrs[0] == L'从') {
    static int no = 1;
    fwprintf(test_out, L"no:%d \nsn:%ls tpre:%ls\necco_sn:%ls pre:%ls\n",
        no++, sn, pre_mvstrs, ecco_sn, getInfoValue_name_ecco__(ecco, ECCOINFO_NAMES[PRE_MVSTRS_INDEX]));
    //}
    //*/
}

// 读取开局着法内容
static void setEcco_someField__(MyLinkedList eccoList, const wchar_t* wstring)
{
    // 根据正则表达式分解字符串，获取字段内容
    const char* error;
    int errorffset, ovector[PCREARRAY_SIZE], index = 0;
    void* regs[] = {
        // field: sn name nums
        pcrewch_compile(L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)",
            0, &error, &errorffset, NULL),
        // field: sn name nums mvstr B2. C0. D1. D2. D3. D4. \\s不包含"　"(全角空格)
        pcrewch_compile(L"([A-E]\\d)(?:．|\\. )(?:空|(\\S+)\\((共[\\s\\S]+?局)\\)"
                        "[\\s\\n　]*([\\s\\S]*?))(?=[\\s\\n　]*[A-E]\\d0．)",
            0, &error, &errorffset, NULL),
        // field: sn name mvstr nums
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)[\\s\\n　]+"
                        "(?:(?![A-E]\\d|上一)([\\s\\S]*?)[\\s\\n　]*(无|共[\\s\\S]+?局)[\\s\\S]*?(?=上|[A-E]\\d{0,2}．))?",
            0, &error, &errorffset, NULL),
        // field: sn mvstr C20 C30 C61 C72局面字符串
        pcrewch_compile(L"([A-E]\\d)\\d局面 =([\\s\\S\\n]*?)(?=[\\s\\n　]*[A-E]\\d{2}．)",
            0, &error, &errorffset, NULL)
    };

    for (int g = 0; g < sizeof(regs) / sizeof(regs[0]); ++g) {
        int first = 0, last = wcslen(wstring);
        while (first < last) {
            const wchar_t* tempWstr = wstring + first;
            int count = pcrewch_exec(regs[g], NULL, tempWstr, last - first, 0, 0, ovector, PCREARRAY_SIZE);
            if (count <= 0)
                break;

            wchar_t wstr[WCHARSIZE];
            if (g < 3) {
                assert(index < 555);
                Ecco ecco = newEcco__();
                for (int f = 0; f < count - 1; ++f) {
                    pcrewch_copy_substring(tempWstr, ovector, count, f + 1, wstr, WCHARSIZE);
                    //    L"SN", L"NAME", L"PRE_MVSTRS", L"MVSTRS", L"NUMS", L"REGSTR"
                    int idx = ((g == 2) ? (f > 1 ? f + 1 : f)
                                        : (f > 1 ? (f == 2 ? NUMS_INDEX : MVSTRS_INDEX) : f));
                    setInfoItem_ecco__(ecco, ECCOINFO_NAMES[idx], wstr);
                }
                addMyLinkedList(eccoList, ecco);
                index++;
            } else {
                wchar_t sn[10];
                pcrewch_copy_substring(tempWstr, ovector, count, 1, sn, 10);
                pcrewch_copy_substring(tempWstr, ovector, count, 2, wstr, WCHARSIZE);

                // C20 C30 C61 C72局面字符串存至g=1数组, 设置前置着法字符串
                Ecco tecco = getDataMyLinkedList_cond(eccoList,
                    (int (*)(void*, void*))eccoSN_cmp__, sn);
                if (tecco)
                    setInfoItem_ecco__(tecco, ECCOINFO_NAMES[MVSTRS_INDEX], wstr);
                //fwprintf(test_out, L"\nline:%d sn:%ls pre:%ls\n", __LINE__, sn,
                //    getInfoValue_name_ecco__(tecco, ECCOINFO_NAMES[PRE_MVSTRS_INDEX]));
            }

            first += ovector[1];
        }
        //printf("\nline:%d index:%d\n", __LINE__, index);
        pcrewch_free(regs[g]);
    }

    // 设置pre_mvstrs字段, 前置省略内容 有40+74=114项
    traverseMyLinkedList(eccoList, (void (*)(void*, void*, void*, void*))setEcco_pre_mvstrs__,
        eccoList, NULL, NULL);

    if (index != 555)
        printf("\nline:%d index:%d\n", __LINE__, index);
    assert(index == 555);
}

// 将着法描述组合成着法字符串（“此前...”，“...不分先后”）
static void getMoveStrs__(wchar_t* mvstrs, const wchar_t* wstr, bool isBefore,
    void* reg_m, void* reg_bp)
{
    wchar_t mvstr[WCHARSIZE], wstrBak[WCHARSIZE];
    wcscpy(wstrBak, wstr);
    wcscpy(mvstrs, L"");

    // 处理一串着法字符串内存在先后顺序的着法
    int ovector[PCREARRAY_SIZE] = { 0 },
        count = pcrewch_exec(reg_bp, NULL, wstrBak, wcslen(wstrBak), 0, 0, ovector, PCREARRAY_SIZE);
    for (int i = 0; i < count - 1; ++i) {
        int start = ovector[2 * i + 2], end = ovector[2 * i + 3];
        if (start == end)
            continue;

        pcrewch_copy_substring(wstrBak, ovector, count, i + 1, mvstr, WCHARSIZE);
        // C83 可选着法"车二进六"不能加入顺序着法，因为需要回退，以便解决与后续“炮８进２”的冲突
        if (wcscmp(mvstr, L"车二进六") == 0
            && wcsstr(wstr, L"红此前可走马二进三、马八进七、车一平二、车二进六、兵三进一和兵七进一"))
            continue;

        // B22~B24, D42,D52~D55 "马八进七"需要前进，又需要加"|"
        wcscat(mvstrs, wcscmp(mvstr, L"马八进七") == 0 ? L"*" : L"+");
        wcscat(mvstrs, mvstr);
        wcsncpy(wstrBak + start, L"~~~~", end - start);
    }

    // 处理连续描述的着法
    int first = 0, last = wcslen(wstrBak);
    while (first < last) {
        wchar_t* tempWstr = wstrBak + first;
        int count = pcrewch_exec(reg_m, NULL, tempWstr, last - first, 0, 0, ovector, PCREARRAY_SIZE);
        if (count <= 0)
            break;

        pcrewch_copy_substring(tempWstr, ovector, count, 1, mvstr, WCHARSIZE);
        // '-','／': 不前进，加‘|’   '*': 前进，加‘|’    '+': 前进，不加‘|’
        wcscat(mvstrs, (isBefore && first != 0 ? L"-" : L"*"));
        wcscat(mvstrs, mvstr);

        first += ovector[1];
    }
}

// 处理前置着法描述字符串————"局面开始：红方：黑方："
static bool setEcco_boutStr_start__(Ecco ecco, const wchar_t* pre_mvstrs, void* reg_m, void* reg_sp, void* reg_bp)
{
    int ovector[PCREARRAY_SIZE];
    int count = pcrewch_exec(reg_sp, NULL, pre_mvstrs, wcslen(pre_mvstrs), 0, 0, ovector, PCREARRAY_SIZE);
    if (count > 2) {
        wchar_t mvstr[WCHARSIZE], mvstr_c[PIECECOLORNUM][WCHARSIZE];
        for (PieceColor c = RED; c <= BLACK; ++c) {
            pcrewch_copy_substring(pre_mvstrs, ovector, count, c + 1, mvstr, WCHARSIZE);
            getMoveStrs__(mvstr_c[c], mvstr, false, reg_m, reg_bp);
        }

        BoutStr boutStr = newBoutStr__(1);
        setBoutStr_mvstr__(boutStr, RED, 0, mvstr_c[RED]);
        setBoutStr_mvstr__(boutStr, BLACK, 0, mvstr_c[BLACK]);
        addMyLinkedList(ecco->boutStrMyLinkedList, boutStr);
        return true;
    } else
        return false;
}

// 处理"此前..."着法描述字符串
static void setEcco_boutStr_before__(Ecco ecco, const wchar_t* wstr,
    PieceColor color, int or, int partIndex, void* reg_m, void* reg_bp)
{
    int insertBoutIndex = INSERTBOUT(partIndex);
    wchar_t mvstrs[WCHARSIZE];
    getMoveStrs__(mvstrs, wstr, true, reg_m, reg_bp);
    if (wcschr(wstr, L'除'))
        wcscat(mvstrs, L"除");

    int boutSize = myLinkedList_size(ecco->boutStrMyLinkedList);
    for (int i = 0; i < boutSize; ++i) {
        BoutStr boutStr = getDataMyLinkedList_idx(ecco->boutStrMyLinkedList, i);
        if (!boutStr || boutStr->boutNo < insertBoutIndex)
            continue;

        bool isInsert = boutStr->boutNo > insertBoutIndex;
        if (isInsert)
            boutStr = newBoutStr__(insertBoutIndex);
        setBoutStr_mvstr__(boutStr, color, or, mvstrs);
        if (or == 1 && boutStr->mvstr[color][0] == NULL)
            setBoutStr_mvstr__(boutStr, color, 0, mvstrs);
        if (isInsert)
            addMyLinkedList_idx(ecco->boutStrMyLinkedList, i, boutStr);
        break;
    }
    if (or == 1 && partIndex == 1
            && (ecco->orBoutNo[1][0] == 0 || ecco->orBoutNo[1][0] > insertBoutIndex))
        ecco->orBoutNo[1][0] = insertBoutIndex;
}

static int boutNo_comp__(BoutStr boutStr, int* pboutNo)
{
    return boutStr->boutNo - *pboutNo;
}

static BoutStr getBoutStr_no__(Ecco ecco, int no)
{
    return getDataMyLinkedList_cond(ecco->boutStrMyLinkedList, (int (*)(void*, void*))boutNo_comp__, &no);
}

// 处理"不分先后"着法描述字符串
static void setEcco_boutStr_noOrder__(Ecco ecco, int firstBoutNo, void* reg_m, void* reg_bp)
{
    wchar_t wstr[WCHARSIZE], mvstr[WCHARSIZE];
    BoutStr firstBoutStr = getBoutStr_no__(ecco, firstBoutNo);
    assert(firstBoutStr);
    int boutSize = myLinkedList_size(ecco->boutStrMyLinkedList),
        firstBoutStr_nextIdx = -1;
    for (int c = RED; c <= BLACK; ++c) {
        for (int i = 0; i < BOUTMOVEOR_NUM; ++i) {
            wcscpy(wstr, L"");
            for (int b = 0; b < boutSize; ++b) {
                BoutStr boutStr = getDataMyLinkedList_idx(ecco->boutStrMyLinkedList, b);
                if (boutStr->boutNo < firstBoutNo
                    || boutStr->mvstr[c][i] == NULL
                    || wcslen(boutStr->mvstr[c][i]) == 0)
                    continue;

                wcscat(wstr, boutStr->mvstr[c][i]);
                // 设置为第一回合着法之后的序号
                if (firstBoutStr_nextIdx == -1 && boutStr->boutNo > firstBoutNo)
                    firstBoutStr_nextIdx = b;
            }

            getMoveStrs__(mvstr, wstr, false, reg_m, reg_bp);
            setPwstr_value(&firstBoutStr->mvstr[c][i], mvstr);
        }
    }

    // 删除第一回合之后的全部回合着法
    if (firstBoutStr_nextIdx >= 0)
        while (firstBoutStr_nextIdx < myLinkedList_size(ecco->boutStrMyLinkedList))
            removeMyLinkedList_idx(ecco->boutStrMyLinkedList, firstBoutStr_nextIdx);
}

// 获取回合着法字符串数组
static void setEcco_boutStr__(Ecco ecco, const wchar_t* snStr, const wchar_t* mvstrs,
    void* reg_m, void* reg_bm, void* reg_bp, int partIndex)
{
    int first = 0, last = wcslen(mvstrs),
        pre_boutNo = 0, firstBoutNo = 0,
        boutNo = 0, or = 0, ovector[PCREARRAY_SIZE];
    while (first < last) {
        const wchar_t* tempWstr = mvstrs + first;
        int count = pcrewch_exec(reg_bm, NULL, tempWstr, last - first, 0, 0, ovector, PCREARRAY_SIZE);
        if (count < 3)
            break;

        int boutChar = tempWstr[ovector[2]];
        boutNo = boutChar - L'0' + (partIndex == 0 && boutChar > L'a' ? -40 : 0);
        if (firstBoutNo == 0)
            firstBoutNo = boutNo;
        if (or == 0)
            (boutNo > pre_boutNo) ? (pre_boutNo = boutNo) : (or = 1); // 此后着法皆为变着

        wchar_t mvstr_c[PIECECOLORNUM][WCHARSIZE] = { 0 };
        for (int i = 2; i < count; ++i) {
            int start = ovector[2 * i], end = ovector[2 * i + 1];
            if (start == end)
                continue;

            wchar_t wstr[WCHARSIZE];
            pcrewch_copy_substring(tempWstr, ovector, count, i, wstr, WCHARSIZE);
            //fwprintf(test_out, L"\ti:%d %ls\n", i, wstr);
            PieceColor color = i / 5;
            if ((i == 2 || i == 5) && wcscmp(wstr, L"……") != 0) { // 回合的着法
                // D41 该部分字段的着法与前置部分重复
                if (color == BLACK && partIndex == 1 && wcscmp(snStr, L"D41") == 0)
                    continue;
                // B32 第一、二着黑棋的顺序有误
                else if (color == BLACK && boutNo < 3 && wcscmp(snStr, L"B32") == 0)
                    wcscpy(wstr, (boutNo == 1) ? L"炮８平６" : L"马８进７");
                // B21 棋子文字错误
                else if (wcscmp(wstr, L"象七进九") == 0)
                    wstr[0] = L'相';

                swprintf(mvstr_c[color], WCHARSIZE, L"+%ls", wstr);
                BoutStr boutStr = getBoutStr_no__(ecco, boutNo);
                if (!boutStr) {
                    boutStr = newBoutStr__(boutNo);
                    addMyLinkedList(ecco->boutStrMyLinkedList, boutStr);
                }
                setBoutStr_mvstr__(boutStr, color, or, mvstr_c[color]);
            } else if (i == 3 || i == 6) { // "不分先后" i=3, 6
                // 忽略"不分先后"
                if (wcscmp(snStr, L"C11") == 0 || wcscmp(snStr, L"C16") == 0 || wcscmp(snStr, L"C18") == 0
                    || wcscmp(snStr, L"C19") == 0 || wcscmp(snStr, L"C54") == 0)
                    continue; // ? 可能存在少量失误？某些着法和连续着法可以不分先后？

                setEcco_boutStr_noOrder__(ecco, firstBoutNo, reg_m, reg_bp);
            } else if (i == 4 || i == 7) { // "此前..."
                // D29 先处理红棋着法 (此前红可走马八进七，黑可走马２进３、车９平４)
                if (wcscmp(snStr, L"D29") == 0) {
                    wcscpy(wstr, L"黑可走马２进３、车９平４");
                    setEcco_boutStr_before__(ecco, L"马八进七", RED, or, partIndex, reg_m, reg_bp);
                } else if (wcscmp(snStr, L"D41") == 0 && partIndex == 1)
                    color = RED; // D41 转移至前面颜色的字段

                setEcco_boutStr_before__(ecco, wstr, color, or, partIndex, reg_m, reg_bp);
            }
        }

        if (or == 1 && ecco->orBoutNo[partIndex][0] == 0)
            ecco->orBoutNo[partIndex][0] = boutNo;
        first += ovector[1];
    }

    if (or == 1)
        ecco->orBoutNo[partIndex][1] = boutNo;
}

// '-','／': 不前进，加‘|’   '*': 前进，加‘|’    '+': 前进，不加‘|’
static int wcscatIccses__(wchar_t* iccses, const wchar_t* mvstr, int index, ChessManual cm, ChangeType ct)
{
    // 静态变量，标记之前的首着分组是否成功
    static bool firstGroupSplit_success = true;
    bool isOther = index > 0 && (mvstr[0] == L'／' || mvstr[0] == L'-');

    // 添加着法，并前进至此着法
    if (appendMove(cm, mvstr + 1, PGN_ZH, NULL, isOther && firstGroupSplit_success) == NULL) {
        if (index == 0)
            firstGroupSplit_success = false; //首着分组失败
        //fwprintf(test_out, L"\n\tFailed:%ls", mvstr);
        return 0;
    }

    int split = (index != 0 && (isOther || mvstr[0] == L'*') ? 1 : 0);
    if (split) {
        if (firstGroupSplit_success) { // 首着分组成功
            wcscat(iccses, L"|");
        } else
            split = 0; // 本着应为首着，已在前调用函数计入分组数
    }

    wchar_t iccs[MOVESTR_LEN + 1];
    wcscat(iccses, getCurMoveICCS_cm(iccs, cm, ct));
    firstGroupSplit_success = true; // 本着成功后重置状态
    //fwprintf(test_out, L"\n\tmvstr:%ls iccs:%ls", mvstr, iccs);

    // 返回1：成功 返回2：成功且分组
    return 1 + split;
}

// 获取着法的iccses字符串，返回分组('|')的数量
static int getIccses__(wchar_t* iccses, const wchar_t* mvstrs, ChessManual cm, ChangeType ct, bool isBefore)
{
    int count = wcslen(mvstrs) / (MOVESTR_LEN + 1),
        groupCount = 1; // 首着计入分组数
    wcscpy(iccses, count <= 1 ? L"" : L"(?:");
    for (int i = 0; i < count; ++i) {
        wchar_t mvstr[MOVESTR_LEN + 2] = { 0 };
        wcsncpy(mvstr, mvstrs + (MOVESTR_LEN + 1) * i, MOVESTR_LEN + 1);
        if (wcscatIccses__(iccses, mvstr, i, cm, ct) > 1)
            ++groupCount;
    }
    if (count > 1)
        wcscat(iccses, L")");

    //fwprintf(test_out, L"\n\tmvstrs:%ls %ls{%d}", mvstrs, iccses, groupCount);
    return groupCount;
}

// 获取“此前..."的before字符串
static int getBeforeIccses__(wchar_t* before, wchar_t* anyMove, const wchar_t* mvstrs,
    ChessManual cm, ChangeType ct)
{
    if (wcslen(mvstrs) < MOVESTR_LEN + 1)
        return 0;

    wchar_t iccses[WCHARSIZE];
    int count = getIccses__(iccses, mvstrs, cm, ct, true);
    if (count == 0)
        return 0;

    if (mvstrs[wcslen(mvstrs) - 1] == L'除')
        swprintf(before, WCHARSIZE, L"(?:(?=!%ls)%ls)*", iccses, anyMove);
    else
        swprintf(before, WCHARSIZE, L"%ls{0,%d}", iccses, count);
    return count;
}

// 添加一种颜色的完整着法，包括”此前“和正常着法
static int getColorIccses__(wchar_t* redBlackStr_c, wchar_t* before, const wchar_t* mvstrs,
    ChessManual cm, ChangeType ct)
{
    if (wcslen(mvstrs) < MOVESTR_LEN + 1)
        return 0;

    wchar_t iccses[WCHARSIZE], tempStr[WCHARSIZE];
    int count = getIccses__(iccses, mvstrs, cm, ct, false);
    if (count == 0)
        return 0;

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
    return count;
}

// 添加一个组合的着法Iccs字符串
static void getCombIccses__(wchar_t* combStr, wchar_t* anyMove, Ecco ecco,
    int partBoutNo[][2], int combPartOr_c[BOUTMOVEPART_NUM], ChessManual cm, ChangeType ct)
{
    wchar_t redBlackStr[PIECECOLORNUM][WIDEWCHARSIZE] = { 0 };
    for (int part = 0; part < 2; ++part) {
        wchar_t before[PIECECOLORNUM][WCHARSIZE] = { 0 };
        const wchar_t* mvstrs_bak[PIECECOLORNUM] = { NULL };
        int boutSize = myLinkedList_size(ecco->boutStrMyLinkedList);
        for (int b = 0; b < boutSize; ++b) {
            BoutStr boutStr = getDataMyLinkedList_idx(ecco->boutStrMyLinkedList, b);
            if (boutStr->boutNo < partBoutNo[part][0])
                continue;
            else if (boutStr->boutNo > partBoutNo[part][1])
                break;

            for (int color = RED; color <= BLACK; ++color) {
                const wchar_t* mvstrs = boutStr->mvstr[color][combPartOr_c[part]];
                if (mvstrs == NULL || wcslen(mvstrs) == 0)
                    continue;

                if (boutStr->boutNo == INSERTBOUT(part)) {
                    // C11~C14=>"车一平四" D40~D43=>"车８进５", 做备份以便后续单独处理
                    if (wcsstr(mvstrs, L"车一平四") || wcsstr(mvstrs, L"车８进５"))
                        mvstrs_bak[color] = mvstrs;
                    else
                        getBeforeIccses__(before[color], anyMove, mvstrs, cm, ct);
                    continue;
                }

                // 处理“此前...”着法(每次单独处理) C11~C14=>"车一平四" D40~D43=>"车８进５"
                if (mvstrs_bak[color]) {
                    getBeforeIccses__(before[color], anyMove, mvstrs_bak[color], cm, ct);
                    if (boutStr->boutNo < 62) // C11 bout==62时不回退
                        backToPre(cm); // 回退至前着，避免重复走最后一着
                }

                getColorIccses__(redBlackStr[color], before[color], mvstrs, cm, ct);
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

static void setRegStr__(Ecco ecco)
{
    int combNum = 0;
    //wchar_t* anyMove = L"(?:[a-i]\\d[a-i]\\d)*"; // 精确版
    wchar_t* anyMove = L".*"; // 简易版
    wchar_t regStr[SUPERWIDEWCHARSIZE] = { 0 };
    wchar_t combStr[SUPERWIDEWCHARSIZE] = { 0 };
    int partBoutNo[BOUTMOVEPART_NUM][2] = {
        { 1,
            (ecco->orBoutNo[0][1] ? ecco->orBoutNo[0][1]
                                  : (ecco->orBoutNo[1][0] ? ecco->orBoutNo[1][0] - 1 : INSERTBOUT(1) - 1)) },
        { (ecco->orBoutNo[1][0] ? ecco->orBoutNo[1][0]
                                : (ecco->orBoutNo[0][1] ? ecco->orBoutNo[0][1] + 1 : INSERTBOUT(1))),
            BOUT_MAX }
    },
        combPartOr[4][BOUTMOVEPART_NUM] = { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };
    for (int comb = 0; comb < 4; ++comb) {
        if ((comb == 1 && ecco->orBoutNo[1][0] == 0)
            || (comb == 2 && ecco->orBoutNo[0][0] == 0)
            || (comb == 3 && (ecco->orBoutNo[0][0] == 0 || ecco->orBoutNo[1][0] == 0)))
            continue;

        combNum++;
        wcscat(combStr, L"(?:");
        // 考虑原始着法和水平对称着法两种情形
        for (int ct = NOCHANGE; ct >= SYMMETRY_H; ct -= (NOCHANGE - SYMMETRY_H)) {
            ChessManual cm = newChessManual();
            wcscat(combStr, L"(?:^");

            getCombIccses__(combStr, anyMove, ecco, partBoutNo, combPartOr[comb], cm, ct);

            combStr[wcslen(combStr) - 1] = L')'; // 最后一个'&‘替换为')'
            wcscat(combStr, L"|"); // 多种转换棋盘

            delChessManual(cm);
        }
        combStr[wcslen(combStr) - 1] = L')'; // 最后一个'|‘替换为')'
        wcscat(combStr, L"|"); //, 可选匹配 "\n\t"调试显示用\n\t
    }
    if (wcslen(combStr) > 0)
        combStr[wcslen(combStr) - 1] = L'\x0'; // 删除最后一个'|‘

    // 如果只有一种组合，则把外围的括号去掉
    if (combNum == 1) {
        wcscpy(regStr, combStr + 3);
        regStr[wcslen(regStr) - 1] = L'\x0'; // 删除最后一个')‘
    } else
        wcscpy(regStr, combStr);

    setInfoItem_ecco__(ecco, ECCOINFO_NAMES[REGSTR_INDEX], regStr);
    //fwprintf(test_out, L"\nregStr:\n\t%ls", regStr);
}

static void setEcco_regstr__(Ecco ecco, void** regs, void* _0, void* _1)
{
    const wchar_t *sn = getEccoSn__(ecco),
                  *mvstrs = getInfoValue_name_ecco__(ecco, ECCOINFO_NAMES[MVSTRS_INDEX]);
    if (wcslen(sn) != 3 || wcslen(mvstrs) < 1)
        return;

    const wchar_t* pre_mvstrs = getInfoValue_name_ecco__(ecco, ECCOINFO_NAMES[PRE_MVSTRS_INDEX]);
    if (wcslen(pre_mvstrs) > 0
        && !setEcco_boutStr_start__(ecco, pre_mvstrs, regs[0], regs[2], regs[3]))
        setEcco_boutStr__(ecco, sn, pre_mvstrs, regs[0], regs[1], regs[3], 0);

    setEcco_boutStr__(ecco, sn, mvstrs, regs[0], regs[1], regs[3], 1);

    setRegStr__(ecco);
}

static void setEcco_regstrField__(MyLinkedList eccoList)
{
    const char* error;
    int errorffset;
    wchar_t ZhWChars[WCHARSIZE], mvstr[WCHARSIZE], mvStrs[WCHARSIZE],
        rich_mvStr[WIDEWCHARSIZE], bout_rich_mvStr[WIDEWCHARSIZE];

    // 着法中文字符组
    getZhWChars(ZhWChars);
    swprintf(mvstr, WCHARSIZE, L"([%ls]{4})", ZhWChars);

    // 着法，含多选的复式着法
    swprintf(mvStrs, WCHARSIZE, L"(?:[%ls]{4}(?:／[%ls]{4})*)", ZhWChars, ZhWChars);

    // 捕捉一步着法: 1.着法，2.可能的“此前...”着法，3. 可能的“／\\n...$”着法
    swprintf(rich_mvStr, WIDEWCHARSIZE, L"(%ls|……)(?:%ls)(\\(?不.先后[\\)，])?([^%ls]*?此前[^\\)]+?\\))?",
        mvStrs, L"[，、；\\s　和\\(\\)以／]|$", ZhWChars);

    // 捕捉一个回合着法：1.序号，2.一步着法的首着，3-5.着法或“此前”，4-6.着法或“此前”或“／\\n...$”
    swprintf(bout_rich_mvStr, WIDEWCHARSIZE, L"([\\da-z]).[ \\n\\r\\s]?%ls(?:%ls)?", rich_mvStr, rich_mvStr);
    void* regs[] = {
        pcrewch_compile(mvstr, 0, &error, &errorffset, NULL),
        pcrewch_compile(bout_rich_mvStr, 0, &error, &errorffset, NULL),
        pcrewch_compile(L"红方：(.+)[\\n\\r]+黑方：(.+)", 0, &error, &errorffset, NULL),
        pcrewch_compile(L"(炮二平五)?.(马二进三).*(车一平二).(车二进六)?"
                        "|(马８进７).+(车９平８)"
                        "|此前可走(马二进三)、(马八进七)、兵三进一和兵七进一",
            0, &error, &errorffset, NULL)
    };
    //fwprintf(test_out, L"%ls\n%ls\n%ls\n%ls\n\n", ZhWChars, mvStrs, rich_mvStr, bout_rich_mvStr);

    traverseMyLinkedList(eccoList, (void (*)(void*, void*, void*, void*))setEcco_regstr__,
        (void*)regs, NULL, NULL);

    for (int i = 0; i < sizeof(regs) / sizeof(regs[0]); ++i)
        pcrewch_free(regs[i]);
}

static void printBoutStr__(BoutStr boutStr, FILE* test_out, const wchar_t* blankStr, void* _1)
{
    fwprintf(test_out, L"\t\t%2d %ls／%ls %ls／%ls\n",
        boutStr->boutNo,
        boutStr->mvstr[RED][0] ? boutStr->mvstr[RED][0] : blankStr,
        boutStr->mvstr[RED][1] ? boutStr->mvstr[RED][1] : blankStr,
        boutStr->mvstr[BLACK][0] ? boutStr->mvstr[BLACK][0] : blankStr,
        boutStr->mvstr[BLACK][1] ? boutStr->mvstr[BLACK][1] : blankStr);
}

static void printEccoStr__(Ecco ecco, FILE* test_out, int* pno, void* _)
{
    const wchar_t *sn = getEccoSn__(ecco),
                  *mvstrs = getInfoValue_name_ecco__(ecco, ECCOINFO_NAMES[MVSTRS_INDEX]);
    if (wcslen(sn) != 3 || wcslen(mvstrs) < 1)
        return;

    fwprintf(test_out, L"No:%d\n", (*pno)++);
    printInfoMyLinkedList(test_out, ecco->infoMyLinkedList);

    fwprintf(test_out, L"orBoutNo:");
    for (int p = 0; p < BOUTMOVEPART_NUM; ++p) {
        for (int i = 0; i < 2; ++i)
            fwprintf(test_out, L" %d", ecco->orBoutNo[p][i]);
    }
    fwprintf(test_out, L"\n");

    fwprintf(test_out, L"boutStr:\n");
    const wchar_t* blankStr = L"        ";
    traverseMyLinkedList(ecco->boutStrMyLinkedList, (void (*)(void*, void*, void*, void*))printBoutStr__,
        test_out, (void*)blankStr, NULL);
    fwprintf(test_out, L"\tsizeof(reg):%ld\n\n", sizeof(ecco->reg));
}

static void printEccoMyLinkedList__(FILE* test_out, MyLinkedList eccoList)
{
    fwprintf(test_out, L"eccoList:\n");
    int no = 0;
    traverseMyLinkedList(eccoList, (void (*)(void*, void*, void*, void*))printEccoStr__,
        test_out, &no, NULL);
}

MyLinkedList getEccoMyLinkedList_file(const char* eccoWebFileName)
{
    FILE* fin = fopen(eccoWebFileName, "r");
    wchar_t* eccoClearWstring = getWString(fin);
    fclose(fin);

    MyLinkedList eccoList = newMyLinkedList((void (*)(void*))delEcco__);
    setEcco_someField__(eccoList, eccoClearWstring);
    setEcco_regstrField__(eccoList);

    printEccoMyLinkedList__(test_out, eccoList);
    free(eccoClearWstring);
    return eccoList;
}

static int addEccoMyLinkedList__(void* eccoList, int argc, char** argv, char** colNames)
{
    if (!argv)
        return -1;

    Ecco ecco = newEcco__();
    for (int i = 0; i < argc; ++i) {
        size_t size_n = mbstowcs(NULL, colNames[i], 0) + 1,
               size_v = mbstowcs(NULL, argv[i], 0) + 1;
        wchar_t colName[size_n], colValue[size_v];
        mbstowcs(colName, colNames[i], size_n);
        mbstowcs(colValue, argv[i], size_v);
        setInfoItem_ecco__(ecco, colName, colValue);

        //fwprintf(test_out, L"%ls: %ls\n", colName, colValue);
    }
    //fwprintf(test_out, L"\n");

    const wchar_t* regstr = getInfoValue_name_ecco__(ecco, ECCOINFO_NAMES[REGSTR_INDEX]);
    if (wcslen(regstr) > 0) {
        const char* error;
        int errorffset;
        ecco->reg = pcrewch_compile(regstr, 0, &error, &errorffset, NULL);
    }

    addMyLinkedList_idx(eccoList, 0, ecco);
    return 0; // 表示执行成功
}

MyLinkedList getEccoMyLinkedList_db(const char* dbName, const char* lib_tblName)
{
    sqlite3* db = NULL;
    if (sqlite3_open(dbName, &db)) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    MyLinkedList eccoList = newMyLinkedList((void (*)(void*))delEcco__);
    char sql[WIDEWCHARSIZE], *zErrMsg = 0;
    sprintf(sql, "SELECT * FROM %s "
                 "WHERE length(sn) == 3 AND length(regstr) > 0 ORDER BY sn ASC;",
        lib_tblName);
    if (sqlite3_exec(db, sql, addEccoMyLinkedList__, eccoList, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "\nTable %s get records error: %s", lib_tblName, zErrMsg);
        sqlite3_free(zErrMsg);
    }

    //printEccoMyLinkedList__(test_out, eccoList);
    sqlite3_close(db);
    assert(myLinkedList_size(eccoList) == 260); // 网站现已确定的开局数量
    return eccoList;
}

static void setECCO_cm__(void* cm, void* eccoList, void* _0, void* _1)
{
    setECCO_cm(cm, eccoList);
}

static const wchar_t* getIccsStr__(wchar_t* wIccsStr, ChessManual cm)
{
    wchar_t iccs[6], redStr[WCHARSIZE] = { 0 }, blackStr[WCHARSIZE] = { 0 };
    backFirst(cm);
    Move firstMove = getNext(getRootMove(cm));
    ChangeType ct = (firstMove == NULL || (getRow_rowcol(getFromRowCol_m(firstMove)) < BOARDROW / 2)
            ? NOCHANGE
            : SYMMETRY_H);
    while (go(cm)) {
        wcscat(redStr, getCurMoveICCS_cm(iccs, cm, ct));
        if (go(cm))
            wcscat(blackStr, getCurMoveICCS_cm(iccs, cm, ct));
    }
    backFirst(cm);
    swprintf(wIccsStr, WIDEWCHARSIZE, L"%ls&%ls", redStr, blackStr);
    return wIccsStr;
}

// 返回0:匹配成功
static int compare_iccsStr__(Ecco ecco, wchar_t* iccsStr)
{
    int ovector[PCREARRAY_SIZE];
    return (pcrewch_exec(ecco->reg, NULL, iccsStr, wcslen(iccsStr), 0, 0, ovector, PCREARRAY_SIZE) > 0 ? 0 : 1);
}

bool setECCO_cm(ChessManual cm, MyLinkedList eccoList)
{
    // 非全局棋谱不设置
    if (wcscmp(getInfoValue_name_cm(cm, CMINFO_NAMES[FEN_INDEX]), FEN_INITVALUE) != 0)
        return false;

    wchar_t iccsStr[WIDEWCHARSIZE];
    getIccsStr__(iccsStr, cm);
    setInfoItem_cm(cm, CMINFO_NAMES[ICCSSTR_INDEX], iccsStr);

    // 从前到后比较对象，获取符合条件的对象
    Ecco ecco = getDataMyLinkedList_cond(eccoList, (int (*)(void*, void*))compare_iccsStr__, iccsStr);
    if (ecco) {
        setInfoItem_cm(cm, CMINFO_NAMES[ECCOSN_INDEX], getEccoSn__(ecco));
        setInfoItem_cm(cm, CMINFO_NAMES[ECCONAME_INDEX], getEccoName__(ecco));
    }

    return ecco;
}

static void printCmStr__(ChessManual cm, FILE* test_out, int* no, void* _)
{
    fwprintf(test_out, L"No:%d\n", (*no)++);
    printInfoMyLinkedList(test_out, getInfoMyLinkedList_cm(cm));
}

static void printCmMyLinkedList__(FILE* test_out, MyLinkedList cmList)
{
    fwprintf(test_out, L"cmList:\n");
    int no = 1;
    traverseMyLinkedList(cmList, (void (*)(void*, void*, void*, void*))printCmStr__,
        test_out, &no, NULL);
}

static void addCmMyLinkedList_file__(FileInfo fileInfo, MyLinkedList cmList)
{
    const char* fileName = fileInfo->name;
    if (!isChessManualFile(fileName))
        return;

    addMyLinkedList(cmList, getChessManual_file(fileName));
}

MyLinkedList getCmMyLinkedList_dir(const char* dirName, RecFormat fromfmt, MyLinkedList eccoList)
{
    MyLinkedList cmList = newMyLinkedList((void (*)(void*))delChessManual);

    char fromDir[FILENAME_MAX];
    sprintf(fromDir, "%s%s", dirName, EXTNAMES[fromfmt]);
    operateDir(fromDir, (void (*)(void*, void*))addCmMyLinkedList_file__, cmList, true);

    traverseMyLinkedList(cmList, setECCO_cm__, eccoList, NULL, NULL);

    printCmMyLinkedList__(test_out, cmList);
    return cmList;
}

MyLinkedList getCmMyLinkedList_webfile(const char* cmWebFileName)
{
    MyLinkedList cmList = newMyLinkedList((void (*)(void*))delChessManual);
    FILE* fin = fopen(cmWebFileName, "r");
    wchar_t lineStr[SUPERWIDEWCHARSIZE] = { 0 }, wstr[SUPERWIDEWCHARSIZE] = { 0 };
    while (fgetws(lineStr, SUPERWIDEWCHARSIZE, fin)) {
        if (lineStr[0] == L'\n') { // 判断是否换行符
            ChessManual cm = newChessManual();
            readInfo_PGN(cm, wstr);
            addMyLinkedList(cmList, cm);
            wcscpy(wstr, L"");
        } else
            wcscat(wstr, lineStr);
    }
    fclose(fin);

    printCmMyLinkedList__(test_out, cmList);
    return cmList;
}

static int addCmMyLinkedList_db__(void* cmList, int argc, char** argv, char** colNames)
{
    if (!argv)
        return -1;

    ChessManual cm = newChessManual();
    for (int i = 0; i < argc; ++i) {
        size_t size_n = mbstowcs(NULL, colNames[i], 0) + 1,
               size_v = mbstowcs(NULL, argv[i], 0) + 1;
        wchar_t colName[size_n], colValue[size_v];
        mbstowcs(colName, colNames[i], size_n);
        mbstowcs(colValue, argv[i], size_v);

        setInfoItem_cm(cm, colName, colValue);
    }

    /* 验证读取棋谱的正确性
    static int no = 1;
    printCmStr__(cm, test_out, &no, NULL);

    wchar_t movestr[SUPERWIDEWCHARSIZE];
    wcscpy(movestr, getInfoValue_name_cm(cm, CMINFO_NAMES[MOVESTR_INDEX]));
    readPGN_wstr(cm, movestr, PGN_ZH);
    if (getNext(getRootMove(cm)))
        setMoveNumZhStr(cm); // 驱动函数

    //writePGN(test_out, cm, PGN_ZH);
    //*/

    addMyLinkedList(cmList, cm);
    return 0; // 表示执行成功
}

MyLinkedList getCmMyLinkedList_db(const char* dbName, const char* man_tblName)
{
    sqlite3* db = NULL;
    if (sqlite3_open(dbName, &db)) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    MyLinkedList cmList = newMyLinkedList((void (*)(void*))delChessManual);
    char sql[WIDEWCHARSIZE], *zErrMsg = 0;
    sprintf(sql, "SELECT * FROM %s "
                 "WHERE length(title) > 0;",
        man_tblName);
    if (sqlite3_exec(db, sql, addCmMyLinkedList_db__, cmList, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "\nTable %s get records error: %s", man_tblName, zErrMsg);
        sqlite3_free(zErrMsg);
    }

    printCmMyLinkedList__(test_out, cmList);
    sqlite3_close(db);
    return cmList;
}

static MyLinkedList getInfoMyLinkedList_ecco__(Ecco eccoObj)
{
    return eccoObj->infoMyLinkedList;
}

int storeEccolib(const char* dbName, const char* lib_tblName, MyLinkedList eccoList)
{
    sqlite3* db = NULL;
    if (sqlite3_open(dbName, &db)) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    // 存储对象
    if (sqlite3_existTable(db, lib_tblName))
        sqlite3_deleteTable(db, lib_tblName);
    int result = storeObjMyLinkedList(db, lib_tblName, eccoList, (MyLinkedList(*)(void*))getInfoMyLinkedList_ecco__);

    sqlite3_close(db);
    return result;
}

int storeChessManual(const char* dbName, const char* man_tblName, MyLinkedList cmList)
{
    sqlite3* db = NULL;
    if (sqlite3_open(dbName, &db)) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    int result = storeObjMyLinkedList(db, man_tblName, cmList, (MyLinkedList(*)(void*))getInfoMyLinkedList_cm);

    sqlite3_close(db);
    return result;
}

typedef struct NoMatchOffset* NoMatchOffset;
struct NoMatchOffset {
    int start;
    int end;
};

static NoMatchOffset newNoMatchOffset__(int start, int end)
{
    NoMatchOffset noMatchOffset = malloc(sizeof(struct NoMatchOffset));
    noMatchOffset->start = start;
    noMatchOffset->end = end;

    return noMatchOffset;
}

static MyLinkedList getNoMatchOffsetMyLinkedList__(const wchar_t* wstr, void* reg)
{
    MyLinkedList noMatchOffsetMyLinkedList = newMyLinkedList(free);

    int ovector[PCREARRAY_SIZE], first = 0, last = wcslen(wstr);
    while (first < last) {
        const wchar_t* tempWstr = wstr + first;
        int count = pcrewch_exec(reg, NULL, tempWstr, last - first, 0, 0, ovector, PCREARRAY_SIZE);
        if (count <= 0)
            break;

        addMyLinkedList(noMatchOffsetMyLinkedList, newNoMatchOffset__(first, first + ovector[0]));
        first += ovector[1];
    }

    return noMatchOffsetMyLinkedList;
}

/*
static void printNoMatchOffset__(NoMatchOffset noMatchOffset, FILE* fout, int* pno, void* _)
{
    fwprintf(fout, L"No:%d %d-%d\n", (*pno)++, noMatchOffset->start, noMatchOffset->end);
}

static void printNoMatchOffsetMyLinkedList__(FILE* fout, MyLinkedList noMatchOffsetMyLinkedList)
{
    fwprintf(fout, L"noMatchOffsetMyLinkedList:\n");
    int no = 0;
    traverseMyLinkedList(noMatchOffsetMyLinkedList, (void (*)(void*, void*, void*, void*))printNoMatchOffset__,
        fout, &no, NULL);
}
//*/

static void wcscatDestStr__(NoMatchOffset noMatchOffset, wchar_t* destWstr, const wchar_t* wstr, void* _)
{
    wchar_t subStr[noMatchOffset->end - noMatchOffset->start + 1];
    copySubStr(subStr, wstr, noMatchOffset->start, noMatchOffset->end);
    wcscat(destWstr, subStr);
}

bool downEccoLibWeb(const char* eccoWebFileName)
{
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t* webWstr = malloc(size * sizeof(wchar_t));
    wchar_t *wurl = L"https://www.xqbase.com/ecco/ecco_%c.htm", wurl_x[WCHARSIZE];
    for (wchar_t sn_0 = L'a'; sn_0 <= L'e'; ++sn_0) {
        swprintf(wurl_x, WCHARSIZE, wurl, sn_0);
        //fwprintf(test_out, L"%ls\n", wurl_x);

        wchar_t* tempWstr = getWebWstr(wurl_x);
        if (!tempWstr) {
            fwprintf(test_out, L"\n页面没有找到：%ls\n", wurl_x);
            continue;
        }

        supper_wcscat(&webWstr, &size, tempWstr);
        free(tempWstr);
    }

    wchar_t* clearwstr = malloc((wcslen(webWstr) + 1) * sizeof(wchar_t));
    const char* error;
    int errorffset;
    wchar_t* regStr[] = {
        L"</?(?:div|font|img|strong|center|meta|dl|dt|table|tr|td|em|p|li|dir"
        "|html|head|body|title|a|b|tbody|script|br|span)[^>]*>",
        L"(?<=\\n|\\r)\\s+" //(?=\\n)
    };
    wchar_t* tempWstr = webWstr;
    for (int i = 0; i < sizeof(regStr) / sizeof(regStr[0]); ++i) {
        void* reg = pcrewch_compile(regStr[i], 0, &error, &errorffset, NULL);
        MyLinkedList noMatchOffsetMyLinkedList = getNoMatchOffsetMyLinkedList__(tempWstr, reg);
        wcscpy(clearwstr, L"");
        traverseMyLinkedList(noMatchOffsetMyLinkedList, (void (*)(void*, void*, void*, void*))wcscatDestStr__,
            clearwstr, (void*)tempWstr, NULL);

        tempWstr = clearwstr;
        //printNoMatchOffsetMyLinkedList__(test_out, noMatchOffsetMyLinkedList);
        delMyLinkedList(noMatchOffsetMyLinkedList);
        pcrewch_free(reg);
    }

    FILE* fout = fopen(eccoWebFileName, "w");
    int len = fwprintf(fout, clearwstr);
    fclose(fout);

    free(webWstr);
    free(clearwstr);
    return len > 0;
}

bool downChessManualWeb(const char* cmWebFileName, int first, int last, int step)
{
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t* webWstr = calloc(size, sizeof(wchar_t));

    wchar_t* regStr[] = {
        L"\\<title\\>(.*)\\</title\\>",
        L"\\>([^\\>]+赛[^\\>]*)\\<",
        L"\\>(\\d+年\\d+月\\d+日) ([^\\<]*)\\<",
        L"\\>黑方 ([^\\<]*)\\<",
        L"\\>红方 ([^\\<]*)\\<",
        L"\\>([A-E]\\d{2})\\. ([^\\<]*)\\<",
        L"\\<pre\\>\\s*(1\\.[\\s\\S]*?)\\((.+)\\)\\</pre\\>"
    };
    int regNum = sizeof(regStr) / sizeof(regStr[0]);
    void** regs = malloc(regNum * sizeof(void*));
    const char* error;
    int errorffset;
    for (int i = 0; i < regNum; ++i)
        regs[i] = pcrewch_compile(regStr[i], 0, &error, &errorffset, NULL);

    // 获取网页idUrl链表
    wchar_t *wurl = L"https://www.xqbase.com/xqbase/?gameid=%d", wurl_x[WCHARSIZE];
    for (int start = first, end = fmin(start + step, last + 1);
         start < end;
         start += step, end = fmin(end + step, last + 1)) {
        for (int id = start; id < end; ++id) {
            swprintf(wurl_x, WCHARSIZE, wurl, id);

            int repeat = 0;
            do {
                wchar_t* wstr = getWebWstr(wurl_x);
                if (wcslen(wstr) == 0) {
                    fwprintf(test_out, L"Gameid: %6d 下载失败 %3d 次！\n", id, ++repeat);
                } else {
                    MyLinkedList infoMyLinkedList = newInfoMyLinkedList();
                    setInfoItem(infoMyLinkedList, CMINFO_NAMES[SOURCE_INDEX], wurl_x);
                    wchar_t value[SUPERWIDEWCHARSIZE];
                    int ovector[PCREARRAY_SIZE], len = wcslen(wstr);
                    for (int i = 0; i < regNum; ++i) {
                        int count = pcrewch_exec(regs[i], NULL, wstr, len, 0, 0, ovector, PCREARRAY_SIZE);
                        for (int c = 1; c < count; ++c) {
                            pcrewch_copy_substring(wstr, ovector, count, c, value, SUPERWIDEWCHARSIZE);
                            int index = (c == 1 ? (i < 3 ? i : (i < 5 ? i + 1 : (i == 5 ? ECCOSN_INDEX : MOVESTR_INDEX)))
                                                : (i == 2 ? SITE_INDEX : (i == 5 ? ECCONAME_INDEX : RESULT_INDEX)));
                            if (c == 1 && i == 6) {
                                int idx = wcslen(value);
                                while (--idx >= 0)
                                    if (value[idx] == L'\r' || value[idx] == L'\n')
                                        value[idx] = L' ';
                            }
                            setInfoItem(infoMyLinkedList, CMINFO_NAMES[index], value);
                        }
                    }

                    writeInfo_PGN_infolist(&webWstr, &size, infoMyLinkedList);
                    supper_wcscat(&webWstr, &size, L"\n");

                    delMyLinkedList(infoMyLinkedList);
                    free(wstr);
                    break;
                }
            } while (true);
        }

        fwprintf(test_out, L"Gameid:%d-%d 下载棋谱完成！\n", start, end - 1);
    }

    FILE* fout = fopen(cmWebFileName, "w");
    int len = fwprintf(fout, webWstr);
    fclose(fout);

    free(webWstr);
    return len > 0;
}
