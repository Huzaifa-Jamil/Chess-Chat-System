#pragma once
#include <string>
#include <cctype>
#include "BoardNode.h"
#include "ChessBoard.h"

inline std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");

    if (a == std::string::npos)
    {
        return "";
    }

    return s.substr(a, b - a + 1);
}

struct PieceProxy
{
    char type;
    bool _empty;

    bool empty() const
    {
        return _empty;
    }
};

struct ChatState
{
    void add() {} // kept minimal (not used anymore so its there for memories only but will be deleted in later releases)
};

class GameState
{
    public:
        ChessBoard chessBoard;
        PieceProxy board[8][8];

        GameState() { sync(); }

        void sync()
        {
            for (int r = 0; r < 8; r++)
            {
                for (int c = 0; c < 8; c++)
                {
                    char p = chessBoard.grid[r][c]->piece;
                    board[r][c].type = p;
                    board[r][c]._empty = (p == '.');
                }
            }
        }

        void reset()
        {
            chessBoard.reset();
            sync();
        }

        bool validateMove(const std::string &m)
        {
            return chessBoard.validateMove(m);
        }

        void applyMove(const std::string &m)
        {
            chessBoard.applyMove(m);
            sync();
        }

        void deserialize(const std::string &s)
        {
            chessBoard.deserialize(s);
            sync();
        }

        std::string serialize()
        {
            return chessBoard.serialize();
        }
};