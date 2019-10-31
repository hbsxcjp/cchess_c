#include "board.h"

const Seat *getSeat(const Board *board, int row, int col)
{
    return &(board->seat[row][col]);
}

Board creatBoard(void)
{
    Board board;
    for (int row = 0; row < BOARDROW; ++row)
    {
        for (int col = 0; col < BOARDCOL; ++col)
        {
            Seat seat = {row, col, NULL};
            board.seat[row][col] = seat;
        }
    }
    board.bottomColor = RED;
    return board;
}

void toString(wchar_t *wstr, size_t count, const Board *board)
{
    wchar_t textBlankBoard[] = L"┏━┯━┯━┯━┯━┯━┯━┯━┓\n"
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
}