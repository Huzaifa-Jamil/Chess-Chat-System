#pragma once

struct BoardNode
{
    // Piece data

    char piece;
    int row;
    int col;

    // Four directional pointers
    BoardNode *up;
    BoardNode *down;
    BoardNode *left;
    BoardNode *right;

    // Constructor

    BoardNode(int r, int c, char p = '.')
    {
        row = r;
        col = c;
        piece = p;

        up = NULL;
        down = NULL;
        left = NULL;
        right = NULL;
    }

    bool isEmpty() const { return piece == '.'; }
    bool isWhite() const { return piece >= 'A' && piece <= 'Z'; }
    bool isBlack() const { return piece >= 'a' && piece <= 'z'; }

    char type() const
    {
        if (piece == '.')
            return '.';
        if (piece >= 'A' && piece <= 'Z')
            return piece - 'A' + 'a';
        return piece;
    }

    bool sameColor(char other) const
    {
        bool thisWhite = isWhite();
        bool otherWhite = (other >= 'A' && other <= 'Z');
        return !isEmpty() && (thisWhite == otherWhite);
    }
};
