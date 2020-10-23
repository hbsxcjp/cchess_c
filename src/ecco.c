#include "head/ecco.h"
#include "head/board.h"
#include "head/chessManual.h"
#include "head/move.h"

#define BOUT_MAX 80
#define INSERTBOUT_INDEX 10
#define INSERTBOUT_SPLIT 30
#define INSERTBOUT(partIndex) (INSERTBOUT_INDEX + (partIndex)*INSERTBOUT_SPLIT)
#define MOVESTR_LEN 4
#define BOUTMOVEPART_NUM 2
#define BOUTMOVEOR_NUM 2
#define APPENDICCS_FAILED 0
#define APPENDICCS_SUCCESS 1
#define APPENDICCS_GROUPSPLIT 2

typedef struct BoutMoveStr* BoutMoveStr;
struct BoutMoveStr {
    int boutNo;
    wchar_t* bmvstr[PIECECOLORNUM][BOUTMOVEOR_NUM];
    BoutMoveStr nboutMoveStr;
};

typedef struct EccoRec* EccoRec;
struct EccoRec {
    int orBoutNo[BOUTMOVEPART_NUM][2];
    wchar_t *sn, *name, *pre_mvstrs, *mvstrs, *nums, *regstr;
    BoutMoveStr rootBoutMoveStr, curBoutMoveStr;
    EccoRec neccoRec;
};

struct RegObj {
    wchar_t sn[4];
    void* reg;
};

//* 输出字符串，检查用
static FILE* fout;

static BoutMoveStr newBoutMoveStr__(BoutMoveStr preBoutMoveStr, int boutNo, const wchar_t* bmvstr[][BOUTMOVEOR_NUM])
{
    BoutMoveStr boutMoveStr = malloc(sizeof(struct BoutMoveStr));
    boutMoveStr->boutNo = boutNo;
    for (int c = RED; c <= BLACK; ++c)
        for (int i = 0; i < BOUTMOVEOR_NUM; ++i) {
            boutMoveStr->bmvstr[c][i] = NULL;
            if (bmvstr[c][i] && wcslen(bmvstr[c][i]) > 0)
                setPwstr_value(&boutMoveStr->bmvstr[c][i], bmvstr[c][i]);
        }
    boutMoveStr->nboutMoveStr = NULL;
    if (preBoutMoveStr)
        preBoutMoveStr->nboutMoveStr = boutMoveStr;
    return boutMoveStr;
}

static void delBoutMoveStr__(BoutMoveStr boutMoveStr)
{
    if (boutMoveStr == NULL)
        return;

    BoutMoveStr nboutMoveStr = boutMoveStr->nboutMoveStr;
    for (int c = RED; c <= BLACK; ++c)
        for (int i = 0; i < BOUTMOVEOR_NUM; ++i)
            free(boutMoveStr->bmvstr[c][i]);
    free(boutMoveStr);

    delBoutMoveStr__(nboutMoveStr);
}

static EccoRec newEccoRec__(EccoRec preEccoRec)
{
    EccoRec eccoRec = malloc(sizeof(struct EccoRec));
    for (int p = 0; p < BOUTMOVEPART_NUM; ++p)
        for (int i = 0; i < 2; ++i)
            eccoRec->orBoutNo[p][i] = 0;
    eccoRec->sn = NULL;
    eccoRec->name = NULL;
    eccoRec->pre_mvstrs = NULL;
    eccoRec->mvstrs = NULL;
    eccoRec->nums = NULL;
    eccoRec->regstr = NULL;
    const wchar_t* bmvstr[][BOUTMOVEOR_NUM] = { { NULL, NULL }, { NULL, NULL } };
    eccoRec->rootBoutMoveStr = newBoutMoveStr__(NULL, 0, bmvstr);
    eccoRec->curBoutMoveStr = eccoRec->rootBoutMoveStr;
    eccoRec->neccoRec = NULL;
    if (preEccoRec)
        preEccoRec->neccoRec = eccoRec;
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

static RegObj newRegObj__(const wchar_t* sn, const wchar_t* regstr)
{
    const char* error;
    int errorffset;
    RegObj regObj = malloc(sizeof(struct RegObj));
    wcscpy(regObj->sn, sn);
    regObj->reg = pcrewch_compile(regstr, 0, &error, &errorffset, NULL);
    return regObj;
}

static void delRegObj__(void* regObj)
{
    RegObj robj = regObj;
    pcrewch_free(robj->reg);
    free(robj);
}

static BoutMoveStr getBoutMoveStr__(EccoRec eccoRec, int boutNo, bool isPre)
{
    BoutMoveStr boutMoveStr = eccoRec->rootBoutMoveStr;
    while (boutMoveStr->nboutMoveStr && boutMoveStr->nboutMoveStr->boutNo < boutNo)
        boutMoveStr = boutMoveStr->nboutMoveStr;
    return (isPre ? boutMoveStr
                  : (boutMoveStr->nboutMoveStr && boutMoveStr->nboutMoveStr->boutNo == boutNo
                          ? boutMoveStr->nboutMoveStr
                          : NULL));
}

static EccoRec getEccoRec__(EccoRec rootEccoRec, wchar_t* sn)
{
    EccoRec eccoRec = rootEccoRec->neccoRec;
    while (eccoRec && wcscmp(eccoRec->sn, sn) != 0)
        eccoRec = eccoRec->neccoRec;
    return eccoRec;
}

static void printBoutMoveStr__(BoutMoveStr rootBoutMoveStr)
{
    const wchar_t* blankStr = L"        ";
    BoutMoveStr boutMoveStr = rootBoutMoveStr;
    while ((boutMoveStr = boutMoveStr->nboutMoveStr)) {
        fwprintf(fout, L"\t\t%2d %ls／%ls %ls／%ls\n",
            boutMoveStr->boutNo,
            boutMoveStr->bmvstr[RED][0] ? boutMoveStr->bmvstr[RED][0] : blankStr,
            boutMoveStr->bmvstr[RED][1] ? boutMoveStr->bmvstr[RED][1] : blankStr,
            boutMoveStr->bmvstr[BLACK][0] ? boutMoveStr->bmvstr[BLACK][0] : blankStr,
            boutMoveStr->bmvstr[BLACK][1] ? boutMoveStr->bmvstr[BLACK][1] : blankStr);
    }
}

static void printEccoRec__(EccoRec rootEccoRec)
{
    int no = 0;
    EccoRec eccoRec = rootEccoRec;
    fwprintf(fout, L"\n\nrootEccoRec:");
    while ((eccoRec = eccoRec->neccoRec)) {
        if (wcslen(eccoRec->sn) < 3 || !eccoRec->mvstrs)
            continue;

        fwprintf(fout, L"\nNo.%d %ls\t%ls\t%ls\npre_mvstrs: %ls\nmvstrs: %ls\n",
            no++, eccoRec->sn, eccoRec->name,
            eccoRec->nums ? eccoRec->nums : L"",
            eccoRec->pre_mvstrs ? eccoRec->pre_mvstrs : L"",
            eccoRec->mvstrs ? eccoRec->mvstrs : L"");
        fwprintf(fout, L"orBoutNo:");
        for (int p = 0; p < BOUTMOVEPART_NUM; ++p) {
            for (int i = 0; i < 2; ++i)
                fwprintf(fout, L" %d", eccoRec->orBoutNo[p][i]);
        }
        fwprintf(fout, L"\n");

        printBoutMoveStr__(eccoRec->rootBoutMoveStr);

        //fwprintf(fout, L"regstr: %ls\n", eccoRec->regstr);
    }
}

// 设置pre_mvstrs字段, 前置省略内容 有40+74=114项
static void setEccoRec_pre_mvstrs__(EccoRec eccoRec, EccoRec rootEccoRec)
{
    wchar_t sn[4] = { 0 };
    const wchar_t* mvstr = eccoRec->mvstrs;
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
        setPwstr_value(&eccoRec->pre_mvstrs, teccoRec->mvstrs);
}

// 读取开局着法内容至数据表
static void setEccoRec_someField__(EccoRec rootEccoRec, const wchar_t* wstring)
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

                eccoRec = newEccoRec__(eccoRec);

                wchar_t** peccoRec_f[] = { &eccoRec->sn, &eccoRec->name, &eccoRec->mvstrs, &eccoRec->nums };
                for (int f = 0; f < count - 1; ++f) {
                    copySubStr(wstr, tempWstr, ovector[2 * f + 2], ovector[2 * f + 3]);
                    setPwstr_value(peccoRec_f[(g < 2 && f > 1) ? (f == 2 ? 3 : 2) : f], wstr);
                }

                index++;
            } else {
                wchar_t sn[10];
                copySubStr(sn, tempWstr, ovector[2], ovector[3]);
                copySubStr(wstr, tempWstr, ovector[4], ovector[5]);
                // C20 C30 C61 C72局面字符串存至g=1数组
                EccoRec teccoRec = getEccoRec__(rootEccoRec, sn);
                if (teccoRec)
                    setPwstr_value(&teccoRec->mvstrs, wstr);
            }

            first += ovector[1];
        }
        pcrewch_free(regs[g]);
    }

    eccoRec = rootEccoRec;
    while ((eccoRec = eccoRec->neccoRec)) {
        if (wcslen(eccoRec->sn) > 2 && eccoRec->mvstrs)
            setEccoRec_pre_mvstrs__(eccoRec, rootEccoRec);
    }
    assert(index == 555);
}

// 将着法描述组合成着法字符串（3种：“此前...”，“...不分先后”, “...／...”）
static void getMoveStrs__(wchar_t* mvstrs, const wchar_t* wstr, bool isBefore,
    void* reg_m, void* reg_bp)
{
    int count = 0, ovector[30] = { 0 };
    wchar_t mvstr[WCHARSIZE], wstrBak[WCHARSIZE], orderStr[WCHARSIZE] = { 0 };
    wcscpy(wstrBak, wstr);

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

            // B22~B24, D42,D52~D55 "马八进七"需要前进，又需要加"|"
            wcscat(orderStr, wcscmp(mvstr, L"马八进七") == 0 ? L"*" : L"+");
            wcscat(orderStr, mvstr);
            wcsncpy(wstrBak + start, L"~~~~", end - start);
        }
    }

    wcscpy(mvstrs, orderStr);
    int first = 0, last = wcslen(wstrBak);
    while (first < last) {
        wchar_t* tempWstr = wstrBak + first;
        if (pcrewch_exec(reg_m, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
            break;

        // '-','／': 不前进，加‘|’   '*': 前进，加‘|’    '+': 前进，不加‘|’
        wcscat(mvstrs, (isBefore && first != 0 ? L"-" : L"*"));
        copySubStr(mvstr, tempWstr, ovector[2], ovector[3]);
        wcscat(mvstrs, mvstr);

        first += ovector[1];
    }
}

// 处理前置着法描述字符串————"局面开始：红方：黑方："
static bool setStartPreMove__(EccoRec eccoRec, void* reg_m, void* reg_sp, void* reg_bp)
{
    int ovector[30];
    const wchar_t* wstr = eccoRec->pre_mvstrs;
    if (pcrewch_exec(reg_sp, NULL, wstr, wcslen(wstr), 0, 0, ovector, 30) > 2) {
        wchar_t rbmvstr[WCHARSIZE], mvstrs[PIECECOLORNUM][WCHARSIZE];
        for (PieceColor c = RED; c <= BLACK; ++c) {
            copySubStr(rbmvstr, wstr, ovector[2 * c + 2], ovector[2 * c + 3]);
            getMoveStrs__(mvstrs[c], rbmvstr, false, reg_m, reg_bp);
        }

        const wchar_t* bmvstr[][BOUTMOVEOR_NUM] = { { mvstrs[RED], NULL }, { mvstrs[BLACK], NULL } };
        eccoRec->curBoutMoveStr = newBoutMoveStr__(eccoRec->curBoutMoveStr, 1, bmvstr);

        return true;
    }
    return false;
}

// 处理"此前..."着法描述字符串
static void setBeforePreMove__(EccoRec eccoRec, const wchar_t* wstr,
    PieceColor color, int or, int partIndex, void* reg_m, void* reg_bp)
{
    int insertBoutIndex = INSERTBOUT(partIndex);
    wchar_t mvstrs[WCHARSIZE];
    getMoveStrs__(mvstrs, wstr, true, reg_m, reg_bp);
    if (wcschr(wstr, L'除'))
        wcscat(mvstrs, L"除");

    BoutMoveStr preBoutMoveStr = getBoutMoveStr__(eccoRec, insertBoutIndex, true);
    assert(preBoutMoveStr);
    BoutMoveStr boutMoveStr = preBoutMoveStr->nboutMoveStr;
    if (boutMoveStr && boutMoveStr->boutNo == insertBoutIndex) {
        setPwstr_value(&boutMoveStr->bmvstr[color][or], mvstrs);
        if (or == 1 && boutMoveStr->bmvstr[color][0] == NULL)
            setPwstr_value(&boutMoveStr->bmvstr[color][0], mvstrs);
    } else {
        const wchar_t* bmvstr[][BOUTMOVEOR_NUM] = { { NULL, NULL }, { NULL, NULL } };
        bmvstr[color][or] = mvstrs;
        if (or == 1)
            bmvstr[color][0] = mvstrs;
        BoutMoveStr newBoutMoveStr = newBoutMoveStr__(preBoutMoveStr, insertBoutIndex, bmvstr);
        newBoutMoveStr->nboutMoveStr = boutMoveStr;
    }
    if (or == 1 && partIndex == 1
            && (eccoRec->orBoutNo[1][0] == 0 || eccoRec->orBoutNo[1][0] > insertBoutIndex))
        eccoRec->orBoutNo[1][0] = insertBoutIndex;
}

// 处理"不分先后"着法描述字符串
static void setNoOrderMove__(EccoRec eccoRec, int firstBoutNo, void* reg_m, void* reg_bp)
{
    wchar_t wstr[WCHARSIZE];
    BoutMoveStr firstBoutMoveStr = getBoutMoveStr__(eccoRec, firstBoutNo, false);
    for (int c = RED; c <= BLACK; ++c) {
        for (int i = 0; i < BOUTMOVEOR_NUM; ++i) {
            BoutMoveStr boutMoveStr = firstBoutMoveStr;
            wcscpy(wstr, L"");
            do {
                if (boutMoveStr->bmvstr[c][i] == NULL
                    || wcslen(boutMoveStr->bmvstr[c][i]) == 0)
                    continue;

                wcscat(wstr, boutMoveStr->bmvstr[c][i]);
                setPwstr_value(&boutMoveStr->bmvstr[c][i], L"");
            } while ((boutMoveStr = boutMoveStr->nboutMoveStr));

            if (wcslen(wstr) == 0)
                continue;

            wchar_t mvstrs[WCHARSIZE];
            getMoveStrs__(mvstrs, wstr, false, reg_m, reg_bp);
            setPwstr_value(&firstBoutMoveStr->bmvstr[c][i], mvstrs);
        }
    }
}

// 获取回合着法字符串数组,
static void setEccoRec_boutMoveStr__(EccoRec eccoRec, const wchar_t* snStr,
    void* reg_m, void* reg_bm, void* reg_bp, int partIndex)
{
    const wchar_t* mvstr = partIndex == 0 ? eccoRec->pre_mvstrs : eccoRec->mvstrs;
    int first = 0, last = wcslen(mvstr), sn = 0, psn = 0, firstBoutNo = 0, boutNo = 0, or = 0, ovector[30];
    while (first < last) {
        const wchar_t* tempWstr = mvstr + first;
        int count = pcrewch_exec(reg_bm, NULL, tempWstr, last - first, 0, 0, ovector, 30);
        if (count < 3)
            break;

        if ((sn = tempWstr[ovector[2]]) < psn || (sn == psn && tempWstr[-2] == L'／'))
            or = 1; // 此后着法皆为变着
        psn = sn;
        boutNo = sn - L'0' + (partIndex == 0 && sn > L'a' ? -40 : 0);
        if (firstBoutNo == 0)
            firstBoutNo = boutNo;

        wchar_t rbmvstr[PIECECOLORNUM][WCHARSIZE] = { 0 };
        for (int i = 2; i < count; ++i) {
            wchar_t wstr[WCHARSIZE];
            copySubStr(wstr, tempWstr, ovector[2 * i], ovector[2 * i + 1]);
            if (wcslen(wstr) == 0)
                continue;

            //fwprintf(fout, L"\ti:%d %ls\n", i, wstr);
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

                swprintf(rbmvstr[color], WCHARSIZE, L"+%ls", wstr);
                BoutMoveStr boutMoveStr = eccoRec->curBoutMoveStr;
                if (boutMoveStr->boutNo == boutNo
                    || (or == 1 && (boutMoveStr = getBoutMoveStr__(eccoRec, boutNo, false))))
                    setPwstr_value(&boutMoveStr->bmvstr[color][or], rbmvstr[color]);
                else { //if (or == 0 || boutMoveStr == NULL) {
                    const wchar_t* bmvstr[][BOUTMOVEOR_NUM] = { { NULL, NULL }, { NULL, NULL } };
                    bmvstr[color][or] = rbmvstr[color];
                    eccoRec->curBoutMoveStr = newBoutMoveStr__(eccoRec->curBoutMoveStr, boutNo, bmvstr);
                }
            } else if (i == 3 || i == 6) { // "不分先后" i=3, 6
                // 忽略"不分先后"
                if (wcscmp(snStr, L"C11") == 0 || wcscmp(snStr, L"C16") == 0 || wcscmp(snStr, L"C18") == 0
                    || wcscmp(snStr, L"C19") == 0 || wcscmp(snStr, L"C54") == 0)
                    continue; // ? 可能存在少量失误？某些着法和连续着法可以不分先后？

                setNoOrderMove__(eccoRec, firstBoutNo, reg_m, reg_bp);
            } else if (i == 4 || i == 7) { // "此前..."
                // D29 (此前红可走马八进七，黑可走马２进３、车９平４)
                if (wcscmp(snStr, L"D29") == 0 && partIndex == 1 && color == BLACK) {
                    wchar_t wstrBak[WCHARSIZE];
                    wcscpy(wstrBak, wstr);
                    copySubStr(wstr, wstrBak, 6 + MOVESTR_LEN, wcslen(wstrBak)); // 复制后两着内容
                    wstrBak[6 + MOVESTR_LEN] = L'\x0'; // 截取前面一着内容
                    setBeforePreMove__(eccoRec, wstrBak, --color, or, partIndex, reg_m, reg_bp);
                }
                // D41 (此前红可走马二进三、马八进七、兵三进一和兵七进一) 转移至前面颜色的字段
                else if (wcscmp(snStr, L"D41") == 0 && partIndex == 1 && color == BLACK)
                    --color;

                setBeforePreMove__(eccoRec, wstr, color, or, partIndex, reg_m, reg_bp);
            }
        }

        if (or == 1 && eccoRec->orBoutNo[partIndex][0] == 0)
            eccoRec->orBoutNo[partIndex][0] = boutNo;
        first += ovector[1];
    }
    if (or == 1)
        eccoRec->orBoutNo[partIndex][1] = boutNo;
}

static int appendIccs__(wchar_t* iccses, const wchar_t* mvstrs, int index, ChessManual cm, ChangeType ct)
{
    if (wcslen(mvstrs) == 0)
        return APPENDICCS_FAILED;

    // 静态变量，标记之前的首着分组是否成功
    static bool firstGroupSplit_success = true;
    const wchar_t* zhStr = mvstrs + (MOVESTR_LEN + 1) * index;
    wchar_t wstr[MOVESTR_LEN + 1] = { 0 }, tag = zhStr[0];
    bool isOther = index > 0 && (tag == L'／' || tag == L'-');
    wcsncpy(wstr, zhStr + 1, MOVESTR_LEN);
    if (appendMove(cm, wstr, PGN_ZH, NULL, isOther && firstGroupSplit_success) == NULL) {

        //fwprintf(fout, L"\n\tFailed:%ls", wstr);
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
static int getIccses__(wchar_t* iccses, const wchar_t* mvstrs, ChessManual cm, ChangeType ct, bool isBefore)
{
    int count = wcslen(mvstrs) / (MOVESTR_LEN + 1), groupCount = 1; // 首着计入分组数
    (count <= 1) ? wcscpy(iccses, L"") : wcscpy(iccses, L"(?:");
    for (int i = 0; i < count; ++i) {
        int result = appendIccs__(iccses, mvstrs, i, cm, ct);
        if ((result & APPENDICCS_SUCCESS) && (result & APPENDICCS_GROUPSPLIT))
            ++groupCount;
    }

    if (count > 1)
        wcscat(iccses, L")");

    //fwprintf(fout, L"\n\tmvstrs:%ls %ls{%d}", mvstrs, iccses, groupCount);
    if (wcslen(iccses) == 0)
        return 0;

    return groupCount;
}

// 获取“此前..."的before字符串
static int getBeforeIccses__(wchar_t* before, wchar_t* anyMove, const wchar_t* mvstrs,
    ChessManual cm, ChangeType ct)
{
    if (wcslen(mvstrs) == 0)
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
    if (wcslen(mvstrs) == 0)
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
static void getCombIccses__(wchar_t* combStr, wchar_t* anyMove, EccoRec eccoRec,
    int partBoutNo[][2], int combPartOr_c[BOUTMOVEPART_NUM], ChessManual cm, ChangeType ct)
{
    wchar_t redBlackStr[PIECECOLORNUM][WIDEWCHARSIZE] = { 0 };
    for (int part = 0; part < 2; ++part) {
        wchar_t before[PIECECOLORNUM][WCHARSIZE] = { 0 };
        const wchar_t* mvstrs_bak[PIECECOLORNUM] = { NULL };
        BoutMoveStr boutMoveStr = getBoutMoveStr__(eccoRec, partBoutNo[part][0], true);
        while ((boutMoveStr = boutMoveStr->nboutMoveStr)
            && boutMoveStr->boutNo <= partBoutNo[part][1]) {

            for (int color = RED; color <= BLACK; ++color) {
                const wchar_t* mvstrs = boutMoveStr->bmvstr[color][combPartOr_c[part]];
                if (mvstrs == NULL || wcslen(mvstrs) == 0)
                    continue;

                if (boutMoveStr->boutNo == INSERTBOUT(part)) {
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
                    if (boutMoveStr->boutNo < 62) // C11 bout==62时不回退
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

static void doSetRegStr__(EccoRec eccoRec)
{
    int combNum = 0;
    //wchar_t* anyMove = L"(?:[a-i]\\d[a-i]\\d)*"; // 精确版
    wchar_t* anyMove = L".*"; // 简易版
    wchar_t eccoRecStr[SUPERWIDEWCHARSIZE] = { 0 };
    wchar_t combStr[SUPERWIDEWCHARSIZE] = { 0 };
    ChessManual cm = newChessManual(NULL);
    int partBoutNo[BOUTMOVEPART_NUM][2] = { { 1,
                                                (eccoRec->orBoutNo[0][1] ? eccoRec->orBoutNo[0][1]
                                                                         : (eccoRec->orBoutNo[1][0] ? eccoRec->orBoutNo[1][0] - 1 : INSERTBOUT(1) - 1)) },
        { (eccoRec->orBoutNo[1][0] ? eccoRec->orBoutNo[1][0]
                                   : (eccoRec->orBoutNo[0][1] ? eccoRec->orBoutNo[0][1] + 1 : INSERTBOUT(1))),
            BOUT_MAX } },
        combPartOr[4][BOUTMOVEPART_NUM] = { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };
    for (int comb = 0; comb < 4; ++comb) {
        if ((comb == 1 && eccoRec->orBoutNo[1][0] == 0)
            || (comb == 2 && eccoRec->orBoutNo[0][0] == 0)
            || (comb == 3 && (eccoRec->orBoutNo[0][0] == 0 || eccoRec->orBoutNo[1][0] == 0)))
            continue;

        combNum++;
        wcscat(combStr, L"(?:");
        for (int ct = EXCHANGE; ct <= SYMMETRY_V; ++ct) {
            backFirst(cm); // 棋盘位置复原
            wcscat(combStr, L"(?:^");

            getCombIccses__(combStr, anyMove, eccoRec, partBoutNo, combPartOr[comb], cm, ct);

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

    setPwstr_value(&eccoRec->regstr, eccoRecStr);

    //fwprintf(fout, L"\nEccoRecStr:\n\t%ls", eccoRecStr);
    delChessManual(cm);
}

static void setEccoRec_regstr__(EccoRec rootEccoRec)
{
    const char* error;
    int errorffset;
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

        if (eccoRec->pre_mvstrs && !setStartPreMove__(eccoRec, reg_m, reg_sp, reg_bp))
            setEccoRec_boutMoveStr__(eccoRec, eccoRec->sn, reg_m, reg_bm, reg_bp, 0);

        setEccoRec_boutMoveStr__(eccoRec, eccoRec->sn, reg_m, reg_bm, reg_bp, 1);

        doSetRegStr__(eccoRec);
        msLen = fmax(msLen, wcslen(mvstrs));
        iccsLen = fmax(iccsLen, wcslen(eccoRec->regstr));
        //assert(wcscmp(eccoRec->regstr, eccoRecStr) == 0);
    }
    //fwprintf(fout, L"eccoRecStr iccsLen:%d mslen:%d", iccsLen, msLen);

    pcrewch_free(reg_m);
    pcrewch_free(reg_bm);
    pcrewch_free(reg_sp);
    pcrewch_free(reg_bp);
}

static int appendRegObj_Item__(void* ppreItem, int argc, char** argv, char** colNames)
{
    wchar_t sn[4], regstr[SUPERWIDEWCHARSIZE];
    mbstowcs(sn, argv[0], 4);
    mbstowcs(regstr, argv[1], SUPERWIDEWCHARSIZE);
    RegObj regObj = newRegObj__(sn, regstr);
    *(LinkedItem*)ppreItem = newLinkedItem(*(LinkedItem*)ppreItem, regObj);

    //wprintf(L"sn:%ls regstr:%ls regObj:%p\n", sn, regstr, regObj);
    return 0; // 表示执行成功
}

LinkedItem getRootRegObj_LinkedItem(sqlite3* db, const char* lib_tblName)
{
    LinkedItem rootRegObj_item = newLinkedItem(NULL, NULL),
               preRegObj_item = rootRegObj_item;
    char sql[WCHARSIZE], *zErrMsg = 0;
    sprintf(sql, "SELECT sn, regstr FROM %s "
                 "WHERE length(sn) == 3 AND length(regstr) > 0 ORDER BY sn DESC;",
        lib_tblName);
    int rc = sqlite3_exec(db, sql, appendRegObj_Item__, &preRegObj_item, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s get records error: %s", lib_tblName, zErrMsg);
        sqlite3_free(zErrMsg);
    }

    return rootRegObj_item;
}

void delRootRegObj_LinkedItem(LinkedItem rootRegObj_item)
{
    delLinkedItem(rootRegObj_item, delRegObj__);
}

const wchar_t* getEcco_sn(RegObj regObj, const wchar_t* iccsStr)
{
    int ovector[30];
    return (pcrewch_exec(regObj->reg, NULL, iccsStr, wcslen(iccsStr), 0, 0, ovector, 30) > 0
            ? regObj->sn
            : NULL);
}

static void getInitEccoSql__(char** initEccoSql,
    const char* lib_tblName, const wchar_t* insertFormat, EccoRec rootEccoRec)
{
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t wtblName[WCHARSIZE], lineStr[SUPERWIDEWCHARSIZE],
        *wInitEccoSql = malloc(size * sizeof(wchar_t));
    assert(wInitEccoSql);
    wInitEccoSql[0] = L'\x0';
    mbstowcs(wtblName, lib_tblName, WCHARSIZE);

    EccoRec eccoRec = rootEccoRec;
    while ((eccoRec = eccoRec->neccoRec)) {
        swprintf(lineStr, SUPERWIDEWCHARSIZE, insertFormat,
            wtblName, eccoRec->sn, eccoRec->name,
            eccoRec->pre_mvstrs ? eccoRec->pre_mvstrs : L"",
            eccoRec->mvstrs ? eccoRec->mvstrs : L"",
            eccoRec->nums ? eccoRec->nums : L"",
            eccoRec->regstr ? eccoRec->regstr : L"");
        supper_wcscat(&wInitEccoSql, &size, lineStr);
    }
    //fwprintf(fout, L"%ls\n\nwInitEccoSql len:%d", wInitEccoSql, wcslen(wInitEccoSql));

    size = wcstombs(NULL, wInitEccoSql, 0) + 1;
    *initEccoSql = malloc(size * sizeof(char));
    wcstombs(*initEccoSql, wInitEccoSql, size);
    free(wInitEccoSql);
}

static char* catColNames__(char* colNames, const char** col_names, int size, const char* splitStr)
{
    char tempStr[WCHARSIZE];
    for (int i = 0; i < size; ++i) {
        sprintf(tempStr, "%s%s", col_names[i], splitStr);
        strcat(colNames, tempStr);
    }
    colNames[strlen(colNames) - 1] = '\x0'; //去掉最后的分隔逗号
    return colNames;
}

static void getInsertFormat__(wchar_t* insertFormat, const char* tblName, const char** col_names, int col_len)
{
    char colNames[WCHARSIZE];
    wchar_t wtblName[WCHARSIZE], wcolNames[WCHARSIZE] = { 0 }, wcolFmt[WCHARSIZE] = { 0 };
    mbstowcs(wtblName, tblName, WCHARSIZE);
    strcpy(colNames, "");
    catColNames__(colNames, col_names, col_len, ",");
    mbstowcs(wcolNames, colNames, WCHARSIZE);
    for (int i = 0; i < col_len; ++i)
        wcscat(wcolFmt, L"'%ls',");
    wcolFmt[wcslen(wcolFmt) - 1] = L'\x0'; //去掉最后的逗号
    swprintf(insertFormat, WIDEWCHARSIZE, L"INSERT INTO %ls (%ls) VALUES (%ls);\n",
        wtblName, wcolNames, wcolFmt);
}

static void storeEccolib__(sqlite3* db, const char* lib_tblName, const char** col_names, int col_len, const char* fileName)
{
    char* rm;
#ifdef __linux
    rm = "r";
#else
    rm = "r, ccs=UTF-8";
#endif

    FILE* fin = fopen(fileName, rm);
    wchar_t* fileWstring = getWString(fin);
    assert(fileWstring);
    EccoRec rootEccoRec = newEccoRec__(NULL);
    setEccoRec_someField__(rootEccoRec, fileWstring);
    setEccoRec_regstr__(rootEccoRec);
    printEccoRec__(rootEccoRec);

    free(fileWstring);
    fclose(fin);

    char colNames[WCHARSIZE];
    strcpy(colNames, "ID INTEGER PRIMARY KEY AUTOINCREMENT,");
    catColNames__(colNames, col_names, col_len, " TEXT,");
    if (sqlite3_existTable(db, lib_tblName))
        sqlite3_deleteTable(db, lib_tblName, "1=1");
    else if (sqlite3_createTable(db, lib_tblName, colNames) != SQLITE_OK)
        return;
 
    // 获取插入记录字符串，并插入
    char* initEccoSql = NULL;
    wchar_t insertFormat[WIDEWCHARSIZE];
    getInsertFormat__(insertFormat, lib_tblName, col_names, col_len);
    getInitEccoSql__(&initEccoSql, lib_tblName, insertFormat, rootEccoRec);
    sqlite3_exec(db, initEccoSql, NULL, NULL, NULL);
    free(initEccoSql);
    delEccoRec__(rootEccoRec);

    /*
    size = wcstombs(NULL, wInsertSql, 0) + 1;
    char insertSql[size];
    wcstombs(insertSql, wInsertSql, size);
    sqlite3_exec(db, insertSql, NULL, NULL, NULL);
    free(wInsertSql);
    //*/
}

static void wcscatCM_Str__(ChessManual cm, wchar_t** pwInsertSql, const wchar_t* insertFormat, size_t* psize)
{
    wchar_t fileName[WCHARSIZE], lineStr[SUPERWIDEWCHARSIZE], *movestr = NULL;
    mbstowcs(fileName, getFileName_cm(cm), WCHARSIZE - 1);
    writeMoveRemark_PGN_ICCSZHtoWstr(&movestr, cm, PGN_ZH);
    swprintf(lineStr, SUPERWIDEWCHARSIZE, insertFormat,
        getInfoValue(cm, L"ECCO"),
        fileName,
        getInfoValue(cm, L"TitleA"),
        getInfoValue(cm, L"Event"),
        getInfoValue(cm, L"Date"),
        getInfoValue(cm, L"Site"),
        getInfoValue(cm, L"Red"),
        getInfoValue(cm, L"Black"),
        getInfoValue(cm, L"Opening"),
        getInfoValue(cm, L"RMKWriter"),
        getInfoValue(cm, L"Author"),
        getInfoValue(cm, L"PlayType"),
        getInfoValue(cm, L"FEN"),
        getInfoValue(cm, L"Result"),
        getInfoValue(cm, L"Version"),
        movestr ? movestr : L"");
    supper_wcscat(pwInsertSql, psize, lineStr);
    free(movestr);
}

void storeManual(sqlite3* db, const char* man_tblName, const char** col_names, int col_len,
    const char* dirName, RecFormat fromfmt, LinkedItem rootRegObj_item)
{
    char colNames[WCHARSIZE];
    strcpy(colNames, "ID INTEGER PRIMARY KEY AUTOINCREMENT,");
    catColNames__(colNames, col_names, col_len, " TEXT,");
    if (sqlite3_existTable(db, man_tblName))
        sqlite3_deleteTable(db, man_tblName, "1=1");
    else if (sqlite3_createTable(db, man_tblName, colNames) != SQLITE_OK)
        return;

    // 获取插入记录字符串，并插入
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t insertFormat[WIDEWCHARSIZE],
        *wInsertSql = calloc(sizeof(wchar_t), size);
    getInsertFormat__(insertFormat, man_tblName, col_names, col_len);

    LinkedItem rootCM_LinkedItem = getRootCM_LinkedItem(dirName, fromfmt, rootRegObj_item);
    traverseLinkedItem(rootCM_LinkedItem, (void (*)(void*, void*, void*, size_t*))wcscatCM_Str__,
        &wInsertSql, insertFormat, &size);

    fwprintf(fout, L"\n\n%ls\n\nwInsertSql len:%d", wInsertSql, wcslen(wInsertSql));
    printCM_LinkedItem(fout, rootCM_LinkedItem);
    delRootCM_LinkedItem(rootCM_LinkedItem);

    size = wcstombs(NULL, wInsertSql, 0) + 1;
    char insertSql[size];
    wcstombs(insertSql, wInsertSql, size);
    sqlite3_exec(db, insertSql, NULL, NULL, NULL);
    free(wInsertSql);
}

void initEcco(char* dbName)
{
    char* wm;
#ifdef __linux
    wm = "w";
#else
    wm = "w, ccs=UTF-8";
#endif

    fout = fopen("chessManual/eccolib", wm);
    setbuf(fout, NULL);

    sqlite3* db = NULL;
    int rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
    } else {
        const char *lib_tblName = "ecco", *srcFileName = "chessManual/eccolib_src",
                   *man_tblName = "manual", *manualDirName = "chessManual/示例文件",
                   *lib_col_names[] = { "SN", "NAME", "PRE_MVSTRS", "MVSTRS", "NUMS", "REGSTR" },
                   *man_col_names[] = { "Ecco_sn", "FileName", "TitleA", "Event", "Date", "Site", "Red", "Black",
                       "Opening", "RMKWriter", "Author", "PlayType", "FEN", "Result", "Version", "movestr" };
        int lib_col_len = sizeof(lib_col_names) / sizeof(lib_col_names[0]),
            man_col_len = sizeof(man_col_names) / sizeof(man_col_names[0]);

        storeEccolib__(db, lib_tblName, lib_col_names, lib_col_len, srcFileName);

        LinkedItem rootRegObj_item = getRootRegObj_LinkedItem(db, lib_tblName);
        storeManual(db, man_tblName, man_col_names, man_col_len, manualDirName, XQF, rootRegObj_item);
        delRootRegObj_LinkedItem(rootRegObj_item);
    }
    sqlite3_close(db);

    fclose(fout);
}