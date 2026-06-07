#pragma once
#include <string>
#include "ChessBoard.h"
#include "Protocol.h"
#include "Logger.h"

class ChessValidator
{
private:
    Logger *logs;
    ChessBoard board; // reused for every validation call

public:
    ChessValidator(Logger *logger) : logs(logger) {}

    char getBoardTurn() const
    {
        return board.turn;
    }

    bool validate(const std::string &rawMsg,
                  std::string &outMove,
                  std::string &outBoardState)
    {
        std::string move = Protocol::getData(rawMsg);

        while (!move.empty() && (move.back() == '\n' || move.back() == '\r'))
            move.pop_back();

        if (move.size() < 4)
        {
            logs->warning("Chess Validator:- move too short: " + move);
            outMove = "INVALID";
            return false;
        }

        bool ok = board.validateMove(move);

        if (ok)
        {
            board.applyMove(move);
            outMove = move;
            outBoardState = board.serialize();
            logs->info("Chess Validator:- VALID Move " + move);
        }
        else
        {
            outMove = "INVALID";
            outBoardState = board.serialize();
            logs->warning("Chess Validator:- INVALID Move " + move);
        }

        return ok;
    }
};
