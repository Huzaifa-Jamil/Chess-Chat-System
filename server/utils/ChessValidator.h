#pragma once
#include <string>
#include "ChessBoard.h"
#include "Protocol.h"
#include "Logger.h"

class ChessValidator
{
private:

    Logger    *logs;
    ChessBoard board; // reused for every validation call

public:

    ChessValidator(Logger *logger) : logs(logger) {}

    bool validate(const std::string &rawMsg,
                  std::string &outMove,
                  std::string &outBoardState)
    {
    
        std::string data = Protocol::getData(rawMsg);

        size_t sep = data.find('|');
        if (sep == std::string::npos)
        {
            logs->warning("ChessValidator, wrong form message: " + rawMsg);
            outMove = "INVALID";
            return false;
        }

        std::string move       = data.substr(0, sep);
        std::string boardState = data.substr(sep + 1);

        if (move.size() < 4)
        {
            logs->warning("ChessValidator, move too short: " + move);
            outMove = "INVALID";
            return false;
        }

        board.deserialize(boardState);

        bool ok = board.validateMove(move);

        if (ok)
        {
            board.applyMove(move);
            outMove       = move;
            outBoardState = board.serialize();
            logs->info("ChessValidator: VALID " + move);
        }
        else
        {
            outMove       = "INVALID";
            outBoardState = boardState;
            logs->warning("ChessValidator: INVALID " + move);
        }

        return ok;
    }
};
