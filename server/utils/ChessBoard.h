#pragma once
#include <string>
#include "BoardNode.h"

class ChessBoard
{
public:
    BoardNode *grid[8][8];
    char turn;

    ChessBoard()
    {
        turn = 'w';

        for (int r = 0; r < 8; r++)
        {
            for (int c = 0; c < 8; c++)
            {
                grid[r][c] = new BoardNode(r, c);
            }
        }

        for (int r = 0; r < 8; r++)
        {
            for (int c = 0; c < 8; c++)
            {
                grid[r][c]->up = (r > 0) ? grid[r - 1][c] : NULL;
                grid[r][c]->down = (r < 7) ? grid[r + 1][c] : NULL;
                grid[r][c]->left = (c > 0) ? grid[r][c - 1] : NULL;
                grid[r][c]->right = (c < 7) ? grid[r][c + 1] : NULL;
            }
        }

        reset();
    }

    ~ChessBoard()
    {
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                delete grid[r][c];
    }

    void reset()
    {
        const char back[] = {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'};
        for (int c = 0; c < 8; c++)
        {
            grid[0][c]->piece = back[c];
            grid[1][c]->piece = 'p';
            grid[6][c]->piece = 'P';
            grid[7][c]->piece = (char)(back[c] - 32); // uppercase
        }
        for (int r = 2; r <= 5; r++)
            for (int c = 0; c < 8; c++)
                grid[r][c]->piece = '.';
        turn = 'w';
    }

    BoardNode *at(int r, int c) const
    {
        if (r < 0 || r > 7 || c < 0 || c > 7)
            return NULL;
        return grid[r][c];
    }

    static std::string rcToAlg(int r, int c)
    {
        std::string s;
        s += (char)('a' + c);
        s += (char)('8' - r);
        return s;
    }

    bool pathClear(BoardNode *from, BoardNode *to) const
    {
        int dr = 0, dc = 0;
        if (to->row > from->row)
            dr = 1;
        if (to->row < from->row)
            dr = -1;
        if (to->col > from->col)
            dc = 1;
        if (to->col < from->col)
            dc = -1;

        BoardNode *cur = from;

        while (true)
        {
            if (dr == -1 && dc == 0)
                cur = cur->up;
            else if (dr == 1 && dc == 0)
                cur = cur->down;
            else if (dr == 0 && dc == -1)
                cur = cur->left;
            else if (dr == 0 && dc == 1)
                cur = cur->right;
            else if (dr == -1 && dc == -1)
                cur = (cur->up) ? cur->up->left : NULL;
            else if (dr == -1 && dc == 1)
                cur = (cur->up) ? cur->up->right : NULL;
            else if (dr == 1 && dc == -1)
                cur = (cur->down) ? cur->down->left : NULL;
            else if (dr == 1 && dc == 1)
                cur = (cur->down) ? cur->down->right : NULL;
            else
                return false;

            if (cur == NULL)
                return false;
            if (cur == to)
                return true;
            if (!cur->isEmpty())
                return false;
        }
    }

    bool pseudoLegal(BoardNode *from, BoardNode *to) const
    {
        if (from == NULL || to == NULL)
            return false;
        if (from->isEmpty())
            return false;
        if (to->sameColor(from->piece))
            return false;

        int dr = to->row - from->row;
        int dc = to->col - from->col;
        char t = from->type();

        if (t == 'p')
        {
            int dir = from->isWhite() ? -1 : 1;
            int startR = from->isWhite() ? 6 : 1;

            if (dc == 0 && dr == dir && to->isEmpty())
                return true;

            if (dc == 0 && dr == 2 * dir && from->row == startR && to->isEmpty())
            {
                BoardNode *mid = (dir == -1) ? from->up : from->down;
                if (mid != NULL && mid->isEmpty())
                    return true;
            }

            if (abs(dc) == 1 && dr == dir && !to->isEmpty())
                return true;

            return false;
        }

        if (t == 'n')
            return (abs(dr) == 2 && abs(dc) == 1) || (abs(dr) == 1 && abs(dc) == 2);

        if (t == 'k')
            return abs(dr) <= 1 && abs(dc) <= 1;

        if (t == 'r')
        {
            if (dr != 0 && dc != 0)
                return false;
            return pathClear(from, to);
        }

        if (t == 'b')
        {
            if (abs(dr) != abs(dc))
                return false;
            return pathClear(from, to);
        }

        if (t == 'q')
        {
            bool straight = (dr == 0 || dc == 0);
            bool diagonal = (abs(dr) == abs(dc));
            if (!straight && !diagonal)
                return false;
            return pathClear(from, to);
        }

        return false;
    }

    bool isLegal(int fromR, int fromC, int toR, int toC) const
    {
        return pseudoLegal(at(fromR, fromC), at(toR, toC));
    }

    bool validateMove(const std::string &move) const
    {
        if (move.size() < 4)
            return false;
        int fc = move[0] - 'a', fr = '8' - move[1];
        int tc = move[2] - 'a', tr = '8' - move[3];

        BoardNode *from = at(fr, fc);
        BoardNode *to = at(tr, tc);
        if (from == NULL || to == NULL || from->isEmpty())
            return false;

        if (turn == 'w' && !from->isWhite())
            return false;
        if (turn == 'b' && !from->isBlack())
            return false;

        return pseudoLegal(from, to);
    }

    void applyMove(const std::string &move)
    {
        if (move.size() < 4)
            return;
        int fc = move[0] - 'a', fr = '8' - move[1];
        int tc = move[2] - 'a', tr = '8' - move[3];

        BoardNode *from = at(fr, fc);
        BoardNode *dest = at(tr, tc);
        if (from == NULL || dest == NULL)
            return;

        dest->piece = from->piece;
        from->piece = '.';

        if (dest->piece == 'P' && dest->row == 0)
            dest->piece = 'Q';
        if (dest->piece == 'p' && dest->row == 7)
            dest->piece = 'q';

        turn = (turn == 'w') ? 'b' : 'w';
    }

    std::string serialize() const
    {
        std::string s;
        for (int r = 0; r < 8; r++)
        {
            for (int c = 0; c < 8; c++)
                s += grid[r][c]->piece;
            if (r < 7)
                s += '/';
        }
        s += ' ';
        s += turn;
        return s;
    }

    void deserialize(const std::string &s)
    {
        int r = 0, c = 0;
        for (size_t i = 0; i < s.size(); i++)
        {
            char ch = s[i];
            if (ch == '/')
            {
                r++;
                c = 0;
            }
            else if (ch == ' ')
            {
                if (i + 1 < s.size())
                    turn = s[i + 1];
                break;
            }
            else if (r < 8 && c < 8)
            {
                grid[r][c]->piece = ch;
                c++;
            }
        }
    }
};
