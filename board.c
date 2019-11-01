#include "board.h"

void setPiece(Board* board, int row, int col, const Piece* piece)
{
    board->piece[row][col] = piece;
}

wchar_t* toString(wchar_t* wstr, Board* board)
{
    static wchar_t textBlankBoard[] = L"┏━┯━┯━┯━┯━┯━┯━┯━┓\n"
                                      "┃　│　│　│╲│╱│　│　│　┃\n"
                                      "┠─┼─┼─┼─╳─┼─┼─┼─┨\n"
                                      "┃　│　│　│╱│╲│　│　│　┃\n"
                                      "┠─╬─┼─┼─┼─┼─┼─╬─┨\n"
                                      "┃　│　│　│　│　│　│　│　┃\n"
                                      "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
                                      "┃　│　│　│　│　│　│　│　┃\n"
                                      "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
                                      "┃　　　　　　　　　　　　　　　┃\n"
                                      "┠─┬─┬─┬─┬─┬─┬─┬─┨\n"
                                      "┃　│　│　│　│　│　│　│　┃\n"
                                      "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
                                      "┃　│　│　│　│　│　│　│　┃\n"
                                      "┠─╬─┼─┼─┼─┼─┼─╬─┨\n"
                                      "┃　│　│　│╲│╱│　│　│　┃\n"
                                      "┠─┼─┼─┼─╳─┼─┼─┼─┨\n"
                                      "┃　│　│　│╱│╲│　│　│　┃\n"
                                      "┗━┷━┷━┷━┷━┷━┷━┷━┛\n"; // 边框粗线
    for (int row = 0; row < BOARDROW; ++row) {
        for (int col = 0; col < BOARDCOL; ++col) {
            const Piece* piece = board->piece[row][col];
            if (piece)
                textBlankBoard[(BOARDCOL - row) * 2 * (BOARDCOL * 2) + col * 2] = getName(piece);
        }
    }
    return wcscpy(wstr, textBlankBoard);
}

// 测试本翻译单元各种对象、函数
void testBoard(void)
{
    Board board;
    for (int row = 0; row < BOARDROW; ++row)
        for (int col = 0; col < BOARDCOL; ++col)
            setPiece(&board, row, col, &(pieces.piece[row < BOARDROW / 2 ? RED : BLACK][(row * col + col) / PIECENUM]));

    wchar_t wstr[1024];
    wprintf(L"%s", toString(wstr, &board));
}