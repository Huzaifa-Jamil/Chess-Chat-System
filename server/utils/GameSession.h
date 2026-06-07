#pragma once
#include <string>
#include "Logger.h"

static const int MAX_MOVES = 200;

// All the states a game room can be in
enum class GameStatus
{
    WAITING,   // room created not started yet
    IN_GAME,   // game is running
    WHITE_WIN, // white player won
    BLACK_WIN, // black player won
    TIE        // draw
};

class GameSession
{
private:
    std::string moves[MAX_MOVES];
    int moveCount;
    int whiteId;
    int blackId;
    GameStatus status;
    Logger *logs;

public:
    GameSession(int playerWhite, int playerBlack, Logger *logger)
    {
        whiteId = playerWhite;
        blackId = playerBlack;
        moveCount = 0;
        status = GameStatus::WAITING;
        logs = logger;

        for (int i = 0; i < MAX_MOVES; i++)
        {
            moves[i] = "";
        }
    }

    void start()
    {
        status = GameStatus::IN_GAME;
        logs->info("GameSession :- Started between white=" + std::to_string(whiteId) +
                   " black=" + std::to_string(blackId));
    }

    bool recordMove(const std::string &move)
    {
        if (moveCount >= MAX_MOVES)
        {
            logs->error("GameSession:- move array full");
            return false;
        }

        moves[moveCount] = move;
        moveCount++;

        logs->info("GameSession:- Move recorded, total moves " + std::to_string(moveCount) +
                   ", move added " + move);
        return true;
    }

    void setResult(GameStatus result)
    {
        status = result;

        std::string resultStr = "";
        if (result == GameStatus::WHITE_WIN)
            resultStr = "white wins";
        if (result == GameStatus::BLACK_WIN)
            resultStr = "black wins";
        if (result == GameStatus::TIE)
            resultStr = "tie";

        logs->info("GameSession:- Game Ended " + resultStr +
                   " after " + std::to_string(moveCount) + " moves");
    }

    void displayMoves()
    {
        std::string out = "GameSession:- move hidtory ";

        for (int i = 0; i < moveCount; i++)
        {
            out += std::to_string(i + 1) + "." + moves[i] + " ";
        }

        logs->info(out);
    }

    // Getters

    GameStatus getStatus()
    {
        return status;
    }

    int getWhiteId()
    {
        return whiteId;
    }

    int getBlackId()
    {
        return blackId;
    }

    int getMoveCount()
    {
        return moveCount;
    }

    bool isFinished()
    {
        return status == GameStatus::WHITE_WIN ||
               status == GameStatus::BLACK_WIN ||
               status == GameStatus::TIE;
    }
};
