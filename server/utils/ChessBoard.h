#pragma once
#include <string>
#include "BoardNode.h"

class ChessBoard
{
public:
    BoardNode *grid[8][8];
    char turn;
    int enPassantCol;
    int enPassantRow;
    bool castleWK;
    bool castleWQ;
    bool castleBK;
    bool castleBQ;
    int pendingPromoRow;
    int pendingPromoCol;

    ChessBoard()
    {
        turn = 'w';
        pendingPromoRow = -1;
        pendingPromoCol = -1;

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
        {
            for (int c = 0; c < 8; c++)
            {
                delete grid[r][c];
            }
        }
    }

    void reset()
    {
        const char back[] = {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'};
        for (int c = 0; c < 8; c++)
        {
            grid[0][c]->piece = back[c];
            grid[1][c]->piece = 'p';
            grid[6][c]->piece = 'P';
            grid[7][c]->piece = (char)(back[c] - 32);
        }

        for (int r = 2; r <= 5; r++)
        {
            for (int c = 0; c < 8; c++)
            {
                grid[r][c]->piece = '.';
            }
        }

        turn = 'w';
        enPassantCol = -1;
        enPassantRow = -1;
        castleWK = castleWQ = castleBK = castleBQ = true;
        pendingPromoRow = -1;
        pendingPromoCol = -1;
    }

    BoardNode *at(int r, int c) const
    {
        if (r < 0 || r > 7 || c < 0 || c > 7)
        {
            return NULL;
        }

        return grid[r][c];
    }

    static std::string rcToAlg(int r, int c)
    {
        std::string s;
        s += (char)('a' + c);
        s += (char)('8' - r);
        return s;
    }

    bool isAwaitingPromotion() const
    {
        return pendingPromoRow >= 0;
    }

    bool pathClear(BoardNode *from, BoardNode *to) const
    {
        int dr = 0, dc = 0;

        if (to->row > from->row)
        {
            dr = 1;
        }

        if (to->row < from->row)
        {
            dr = -1;
        }

        if (to->col > from->col)
        {
            dc = 1;
        }

        if (to->col < from->col)
        {
            dc = -1;
        }

        BoardNode *cur = from;

        while (true)
        {
            if (dr == -1 && dc == 0)
            {
                cur = cur->up;
            }
            else if (dr == 1 && dc == 0)
            {
                cur = cur->down;
            }
            else if (dr == 0 && dc == -1)
            {
                cur = cur->left;
            }
            else if (dr == 0 && dc == 1)
            {
                cur = cur->right;
            }
            else if (dr == -1 && dc == -1)
            {
                cur = (cur->up) ? cur->up->left : NULL;
            }
            else if (dr == -1 && dc == 1)
            {
                cur = (cur->up) ? cur->up->right : NULL;
            }
            else if (dr == 1 && dc == -1)
            {
                cur = (cur->down) ? cur->down->left : NULL;
            }
            else if (dr == 1 && dc == 1)
            {
                cur = (cur->down) ? cur->down->right : NULL;
            }
            else
            {
                return false;
            }

            if (cur == NULL)
            {
                return false;
            }

            if (cur == to)
            {
                return true;
            }

            if (!cur->isEmpty())
            {
                return false;
            }
        }
    }

    bool isCastleMove(BoardNode *from, BoardNode *to) const
    {
        if (from->type() != 'k')
        {
            return false;
        }

        int dr = to->row - from->row;
        int dc = to->col - from->col;

        if (dr != 0 || abs(dc) != 2)
        {
            return false;
        }

        int row = from->row;

        if (from->isWhite() && row != 7)
        {
            return false;
        }

        if (from->isBlack() && row != 0)
        {
            return false;
        }

        if (from->isWhite())
        {
            if (dc == 2 && !castleWK)
            {
                return false;
            }

            if (dc == -2 && !castleWQ)
            {
                return false;
            }

            if (dc == 2)
            {
                if (grid[row][5]->piece != '.' || grid[row][6]->piece != '.')
                {
                    return false;
                }

                if (grid[row][7]->piece != 'R')
                {
                    return false;
                }
            }
            else
            {
                if (grid[row][1]->piece != '.' || grid[row][2]->piece != '.' || grid[row][3]->piece != '.')
                {
                    return false;
                }

                if (grid[row][0]->piece != 'R')
                {
                    return false;
                }
            }
        }
        else
        {
            if (dc == 2 && !castleBK)
            {
                return false;
            }

            if (dc == -2 && !castleBQ)
            {
                return false;
            }

            if (dc == 2)
            {
                if (grid[row][5]->piece != '.' || grid[row][6]->piece != '.')
                {
                    return false;
                }

                if (grid[row][7]->piece != 'r')
                {
                    return false;
                }
            }
            else
            {
                if (grid[row][1]->piece != '.' || grid[row][2]->piece != '.' || grid[row][3]->piece != '.')
                {
                    return false;
                }

                if (grid[row][0]->piece != 'r')
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool pseudoLegal(BoardNode *from, BoardNode *to, bool forAttack = false) const
    {
        if (from == NULL || to == NULL)
        {
            return false;
        }

        if (from->isEmpty())
        {
            return false;
        }

        if (to->sameColor(from->piece))
        {
            return false;
        }

        if (!forAttack && to->type() == 'k')
        {
            return false;
        }

        int dr = to->row - from->row;
        int dc = to->col - from->col;
        char t = from->type();

        if (t == 'p')
        {
            int dir = from->isWhite() ? -1 : 1;
            int startR = from->isWhite() ? 6 : 1;

            if (dc == 0 && dr == dir && to->isEmpty())
            {
                return true;
            }

            if (dc == 0 && dr == 2 * dir && from->row == startR && to->isEmpty())
            {
                BoardNode *mid = (dir == -1) ? from->up : from->down;
                if (mid != NULL && mid->isEmpty())
                    return true;
            }

            if (abs(dc) == 1 && dr == dir && !to->isEmpty())
            {
                return true;
            }

            if (abs(dc) == 1 && dr == dir && to->isEmpty() && to->row == enPassantRow && to->col == enPassantCol)
            {
                return true;
            }

            return false;
        }

        if (t == 'n')
        {
            return (abs(dr) == 2 && abs(dc) == 1) || (abs(dr) == 1 && abs(dc) == 2);
        }

        if (t == 'k')
        {
            if (abs(dr) <= 1 && abs(dc) <= 1)
            {
                return true;
            }

            return isCastleMove(from, to);
        }

        if (t == 'r')
        {
            if (dr != 0 && dc != 0)
            {
                return false;
            }

            return pathClear(from, to);
        }

        if (t == 'b')
        {
            if (abs(dr) != abs(dc))
            {
                return false;
            }

            return pathClear(from, to);
        }

        if (t == 'q')
        {
            bool straight = (dr == 0 || dc == 0);
            bool diagonal = (abs(dr) == abs(dc));
            
            if (!straight && !diagonal)
            {
                return false;
            }

            return pathClear(from, to);
        }

        return false;
    }

    bool isSquareAttacked(int r, int c, char byColor) const
    {
        for (int rr = 0; rr < 8; rr++)
        {
            for (int cc = 0; cc < 8; cc++)
            {
                BoardNode *attacker = grid[rr][cc];
                if (attacker->isEmpty())
                {
                    continue;
                }

                bool isAttackerWhite = attacker->isWhite();

                if (byColor == 'w' && !isAttackerWhite)
                {
                    continue;
                }

                if (byColor == 'b' && isAttackerWhite)
                {
                    continue;
                }

                if (pseudoLegal(attacker, grid[r][c], true))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool isKingInCheck(char color) const
    {
        char kingPiece = (color == 'w') ? 'K' : 'k';
        int kr = -1, kc = -1;

        for (int r = 0; r < 8; r++)
        {
            for (int c = 0; c < 8; c++)
            {
                if (grid[r][c]->piece == kingPiece)
                {
                    kr = r;
                    kc = c;
                }
            }
        }

        if (kr == -1)
        {
            return false;
        }

        char opponent = (color == 'w') ? 'b' : 'w';

        return isSquareAttacked(kr, kc, opponent);
    }

    static bool isValidPromoChar(char p)
    {
        return p == 'q' || p == 'r' || p == 'b' || p == 'n';
    }

    bool isPromotionMove(const std::string &move) const
    {
        if (move.size() < 4)
        {
            return false;
        }

        int fc = move[0] - 'a', fr = '8' - move[1];
        int tc = move[2] - 'a', tr = '8' - move[3];

        BoardNode *from = at(fr, fc);

        if (from == NULL || from->type() != 'p')
        {
            return false;
        }

        if (from->isWhite() && tr == 0)
        {
            return true;
        }

        if (from->isBlack() && tr == 7)
        {
            return true;
        }

        return false;
    }

    bool needsPromotionChoice(const std::string &move) const
    {
        return isPromotionMove(move) && move.size() < 5;
    }

    bool simulateMove(const std::string &move, char promoChar) const
    {
        int fc = move[0] - 'a', fr = '8' - move[1];
        int tc = move[2] - 'a', tr = '8' - move[3];

        BoardNode *from = at(fr, fc);
        BoardNode *to = at(tr, tc);

        if (from == NULL || to == NULL)
        {
            return false;
        }

        char savedDest = to->piece;
        char savedSrc = from->piece;
        bool isEp = (from->type() == 'p' && fc != tc && savedDest == '.');
        char savedEpPawn = '.';

        if (isEp)
        {
            savedEpPawn = grid[fr][tc]->piece;
            grid[fr][tc]->piece = '.';
        }

        bool isCastle = isCastleMove(from, to);
        char savedRook = '.';
        int rookFromC = -1, rookToC = -1;

        if (isCastle)
        {
            int dc = to->col - from->col;
            rookFromC = (dc == 2) ? 7 : 0;
            rookToC = (dc == 2) ? 5 : 3;
            savedRook = grid[fr][rookFromC]->piece;
            grid[fr][rookFromC]->piece = '.';
            grid[fr][rookToC]->piece = savedRook;
        }

        to->piece = from->piece;
        from->piece = '.';

        if (savedSrc == 'P' || savedSrc == 'p')
        {
            if (savedSrc == 'P' && tr == 0)
            {
                if (isValidPromoChar(promoChar))
                {
                    to->piece = (char)(promoChar - 32);
                }
                else
                {
                    to->piece = 'Q';
                }
            }

            if (savedSrc == 'p' && tr == 7)
            {
                if (isValidPromoChar(promoChar))
                {
                    to->piece = promoChar;
                }

                else
                {
                    to->piece = 'q';
                }
            }
        }

        bool inCheck = isKingInCheck(turn);

        from->piece = savedSrc;
        to->piece = savedDest;

        if (isEp)
        {
            grid[fr][tc]->piece = savedEpPawn;
        }

        if (isCastle)
        {
            grid[fr][rookFromC]->piece = savedRook;
            grid[fr][rookToC]->piece = '.';
        }

        return !inCheck;
    }

    bool isMoveLegal(const std::string &move) const
    {
        if (move.size() < 4)
        {
            return false;
        }

        int fc = move[0] - 'a', fr = '8' - move[1];
        int tc = move[2] - 'a', tr = '8' - move[3];

        BoardNode *from = at(fr, fc);
        BoardNode *to = at(tr, tc);

        if (from == NULL || to == NULL || from->isEmpty())
        {
            return false;
        }

        if (turn == 'w' && !from->isWhite())
        {
            return false;
        }

        if (turn == 'b' && !from->isBlack())
        {
            return false;
        }

        if (!pseudoLegal(from, to))
        {
            return false;
        }

        if (isCastleMove(from, to))
        {
            if (isKingInCheck(turn))
            {
                return false;
            }
            
            int dc = to->col - from->col;
            int passCol = (dc == 2) ? from->col + 1 : from->col - 1;
            char opponent = (turn == 'w') ? 'b' : 'w';

            if (isSquareAttacked(fr, passCol, opponent))
            {
                return false;
            }

            if (isSquareAttacked(to->row, to->col, opponent))
            {
                return false;
            }
        }

        if (isPromotionMove(move))
        {
            if (move.size() >= 5 && isValidPromoChar(move[4]))
            {
                return simulateMove(move, move[4]);
            }

            const char promos[] = {'q', 'r', 'b', 'n'};

            for (int i = 0; i < 4; i++)
            {
                if (simulateMove(move, promos[i]))
                {
                    return true;
                }
            }

            return false;
        }

        return simulateMove(move, 'q');
    }

    bool hasAnyLegalMove(char color) const
    {
        for (int fr = 0; fr < 8; fr++)
        {
            for (int fc = 0; fc < 8; fc++)
            {
                BoardNode *from = grid[fr][fc];
                
                if (from->isEmpty())
                {
                    continue;
                }

                if (color == 'w' && !from->isWhite())
                {
                    continue;
                }

                if (color == 'b' && !from->isBlack())
                {
                    continue;
                }

                for (int tr = 0; tr < 8; tr++)
                {
                    for (int tc = 0; tc < 8; tc++)
                    {
                        std::string mv;
                        mv += (char)('a' + fc);
                        mv += (char)('8' - fr);
                        mv += (char)('a' + tc);
                        mv += (char)('8' - tr);

                        if (isMoveLegal(mv))
                        {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    bool isCheckmate(char color) const
    {
        return isKingInCheck(color) && !hasAnyLegalMove(color);
    }

    bool isStalemate(char color) const
    {
        return !isKingInCheck(color) && !hasAnyLegalMove(color);
    }

    bool hasInsufficientMaterial() const
    {
        int white = 0, black = 0;
        bool whiteMinor = false, blackMinor = false;
        bool whiteBishop = false, blackBishop = false;
        bool whiteKnight = false, blackKnight = false;

        for (int r = 0; r < 8; r++)
        {
            for (int c = 0; c < 8; c++)
            {
                char p = grid[r][c]->piece;
                if (p == '.' || p == 'K' || p == 'k')
                {
                    continue;
                }

                if (p >= 'A' && p <= 'Z')
                {
                    white++;

                    if (p == 'Q' || p == 'R' || p == 'P')
                    {
                        return false;
                    }

                    if (p == 'B')
                    {
                        whiteBishop = true;
                    }

                    if (p == 'N')
                    {
                        whiteKnight = true;
                    }

                    whiteMinor = true;
                }
                else
                {
                    black++;
                    if (p == 'q' || p == 'r' || p == 'p')
                    {
                        return false;
                    }

                    if (p == 'b')
                    {
                        blackBishop = true;
                    }

                    if (p == 'n')
                    {
                        blackKnight = true;
                    }

                    blackMinor = true;
                }
            }
        }

        if (white == 0 && black == 0)
        {
            return true;
        }

        if (white == 1 && black == 0 && (whiteBishop || whiteKnight))
        {
            return true;
        }

        if (black == 1 && white == 0 && (blackBishop || blackKnight))
        {
            return true;
        }

        if (white == 1 && black == 1 && whiteBishop && blackBishop)
        {
            return true;
        }

        return false;
    }

    bool validateMove(const std::string &move) const
    {
        return isMoveLegal(move);
    }

    void updateCastlingRights(int fr, int fc, int tr, int tc, char movingType, char captured)
    {
        if (movingType == 'k')
        {
            if (turn == 'w')
            {
                castleWK = false;
                castleWQ = false;
            }
            else
            {
                castleBK = false;
                castleBQ = false;
            }
        }

        if (movingType == 'r')
        {
            if (fr == 7 && fc == 0)
            {
                castleWQ = false;
            }

            if (fr == 7 && fc == 7)
            {
                castleWK = false;
            }

            if (fr == 0 && fc == 0)
            {
                castleBQ = false;
            }

            if (fr == 0 && fc == 7)
            {
                castleBK = false;
            }
        }

        if (captured == 'R')
        {
            if (tr == 7 && tc == 0)
            {
                castleWQ = false;
            }

            if (tr == 7 && tc == 7)
            {
                castleWK = false;
            }
        }

        if (captured == 'r')
        {
            if (tr == 0 && tc == 0)
            {
                castleBQ = false;
            }

            if (tr == 0 && tc == 7)
            {
                castleBK = false;
            }
        }
    }

    void applyMove(const std::string &move)
    {
        if (move.size() < 4)
        {
            return;
        }

        int fc = move[0] - 'a', fr = '8' - move[1];
        int tc = move[2] - 'a', tr = '8' - move[3];

        BoardNode *from = at(fr, fc);
        BoardNode *dest = at(tr, tc);

        if (from == NULL || dest == NULL)
        {
            return;
        }

        if (dest->type() == 'k')
        {
            return;
        }

        char movingType = from->type();
        char savedDest = dest->piece;
        bool isCastle = isCastleMove(from, dest);

        if (needsPromotionChoice(move))
        {
            dest->piece = from->piece;
            from->piece = '.';
            pendingPromoRow = tr;
            pendingPromoCol = tc;

            int newEnPassantCol = -1;
            int newEnPassantRow = -1;
            enPassantCol = newEnPassantCol;
            enPassantRow = newEnPassantRow;
            updateCastlingRights(fr, fc, tr, tc, movingType, savedDest);
            return;
        }

        dest->piece = from->piece;
        from->piece = '.';

        if (isCastle)
        {
            int dc = tc - fc;
            int rookFromC = (dc == 2) ? 7 : 0;
            int rookToC = (dc == 2) ? 5 : 3;
            grid[fr][rookToC]->piece = grid[fr][rookFromC]->piece;
            grid[fr][rookFromC]->piece = '.';
        }

        int newEnPassantCol = -1;
        int newEnPassantRow = -1;

        if (movingType == 'p' && abs(fr - tr) == 2)
        {
            newEnPassantCol = tc;
            newEnPassantRow = (fr + tr) / 2;
        }

        if (movingType == 'p' && fc != tc && savedDest == '.')
        {
            grid[fr][tc]->piece = '.';
        }

        enPassantCol = newEnPassantCol;
        enPassantRow = newEnPassantRow;
        updateCastlingRights(fr, fc, tr, tc, movingType, savedDest);

        if (dest->piece == 'P' && dest->row == 0)
        {
            if (move.size() >= 5 && isValidPromoChar(move[4]))
            {
                dest->piece = (char)(move[4] - 32);
            }
            else
            {
                dest->piece = 'Q';
            }
        }

        if (dest->piece == 'p' && dest->row == 7)
        {
            if (move.size() >= 5 && isValidPromoChar(move[4]))
            {
                dest->piece = move[4];
            }
            else
            {
                dest->piece = 'q';
            }
        }

        pendingPromoRow = -1;
        pendingPromoCol = -1;
        turn = (turn == 'w') ? 'b' : 'w';
    }

    bool isPromotionChoiceLegal(char promo) const
    {
        if (pendingPromoRow < 0)
        {
            return false;
        }

        if (!isValidPromoChar(promo))
        {
            return false;
        }

        BoardNode *pawn = at(pendingPromoRow, pendingPromoCol);

        if (pawn == NULL || pawn->type() != 'p')
        {
            return false;
        }

        char saved = pawn->piece;

        if (pawn->isWhite())
        {
            pawn->piece = (char)(promo - 32);
        }
        else
        {
            pawn->piece = promo;
        }

        bool inCheck = isKingInCheck(turn);
        pawn->piece = saved;

        return !inCheck;
    }

    bool finishPromotion(char promo)
    {
        if (!isPromotionChoiceLegal(promo))
        {
            return false;
        }

        BoardNode *pawn = at(pendingPromoRow, pendingPromoCol);

        if (pawn->isWhite())
        {
            pawn->piece = (char)(promo - 32);
        }
        else
        {
            pawn->piece = promo;
        }

        pendingPromoRow = -1;
        pendingPromoCol = -1;
        turn = (turn == 'w') ? 'b' : 'w';

        return true;
    }

    std::string serialize() const
    {
        std::string s;
        for (int r = 0; r < 8; r++)
        {
            for (int c = 0; c < 8; c++)
            {
                s += grid[r][c]->piece;
            }
            if (r < 7)
            {
                s += '/';
            }
        }

        s += ' ';
        s += turn;
        s += ' ';

        if (castleWK)
        {
            s += 'K';
        }

        if (castleWQ)
        {
            s += 'Q';
        }

        if (castleBK)
        {
            s += 'k';
        }

        if (castleBQ)
        {
            s += 'q';
        }

        if (!castleWK && !castleWQ && !castleBK && !castleBQ)
        {
            s += '-';
        }

        s += ' ';

        if (enPassantRow >= 0 && enPassantCol >= 0)
        {
            s += rcToAlg(enPassantRow, enPassantCol);
        }
        else
        {
            s += '-';
        }

        return s;
    }

    void deserialize(const std::string &s)
    {
        int r = 0, c = 0;
        int phase = 0;
        std::string castling;
        std::string ep;

        for (size_t i = 0; i < s.size(); i++)
        {
            char ch = s[i];

            if (phase == 0)
            {
                if (ch == '/')
                {
                    r++;
                    c = 0;
                }
                else if (ch == ' ')
                {
                    phase = 1;
                }
                else if (r < 8 && c < 8)
                {
                    grid[r][c]->piece = ch;
                    c++;
                }
            }
            else if (phase == 1)
            {
                if (ch != ' ')
                {
                    turn = ch;
                    phase = 2;
                }
            }
            else if (phase == 2)
            {
                if (ch == ' ')
                {
                    if (!castling.empty())
                    {
                        phase = 3;
                        castleWK = (castling.find('K') != std::string::npos);
                        castleWQ = (castling.find('Q') != std::string::npos);
                        castleBK = (castling.find('k') != std::string::npos);
                        castleBQ = (castling.find('q') != std::string::npos);
                    }
                }
                else
                {
                    castling += ch;
                }
            }
            else if (phase == 3)
            {
                if (ch != ' ' && ch != '\n' && ch != '\r')
                {
                    ep += ch;
                }
            }
        }

        if (phase == 2 && !castling.empty())
        {
            castleWK = (castling.find('K') != std::string::npos);
            castleWQ = (castling.find('Q') != std::string::npos);
            castleBK = (castling.find('k') != std::string::npos);
            castleBQ = (castling.find('q') != std::string::npos);
        }

        if (ep == "-" || ep.empty())
        {
            enPassantCol = -1;
            enPassantRow = -1;
        }
        else if (ep.size() >= 2)
        {
            enPassantCol = ep[0] - 'a';
            enPassantRow = '8' - ep[1];
        }

        pendingPromoRow = -1;
        pendingPromoCol = -1;
    }
};
