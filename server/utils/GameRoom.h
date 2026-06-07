#pragma once
#include <QTcpSocket>
#include <string>
#include "Logger.h"
#include "Protocol.h"
#include "ChatSession.h"
#include "GameSession.h"
#include "ChessValidator.h"

class GameRoom
{
private:
    int roomId;
    ChatSession *chat;
    GameSession *game;
    ChessValidator *validator;
    Logger *logs;

public:
    GameRoom(int id,
             QTcpSocket *p1, int userId1,
             QTcpSocket *p2, int userId2,
             Logger *logger)
    {
        roomId = id;
        logs = logger;
        chat = new ChatSession(p1, userId1, p2, userId2, logger);
        game = new GameSession(userId1, userId2, logger);
        validator = new ChessValidator(logger);

        logs->info("GameRoom (Hash Map):- game room " + std::to_string(roomId) + "created for user " +
                   std::to_string(userId1) + " vs user " + std::to_string(userId2));
    }

    ~GameRoom()
    {
        delete chat;
        delete game;
        delete validator;
    }

    // Send START to both players with board sates
    void start()
    {
        game->start();

        ChessBoard startBoard;
        std::string boardState = startBoard.serialize();

        // START|opponentId|color|boardState
        chat->sendToPlayer1(Protocol::build(TAG_START, std::to_string(chat->getId2()) + "|w|" + boardState));

        chat->sendToPlayer2(Protocol::build(TAG_START, std::to_string(chat->getId1()) + "|b|" + boardState));

        logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) + "] started");
    }

    void notifyDisconnect(QTcpSocket *who)
    {
        if (game->isFinished())
        {
            return;
        }

        if (who == chat->getPlayer1())
        {
            game->setResult(GameStatus::BLACK_WIN);
            chat->sendToPlayer2(Protocol::build(TAG_WIN, ""));
            chat->sendToPlayer2(Protocol::build(TAG_END, ""));
        }

        else if (who == chat->getPlayer2())
        {
            game->setResult(GameStatus::WHITE_WIN);
            chat->sendToPlayer1(Protocol::build(TAG_WIN, ""));
            chat->sendToPlayer1(Protocol::build(TAG_END, ""));
        }

        logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) + "] ended by disconnect");
    }

    void handleMessage(QTcpSocket *from, const std::string &rawMsg)
    {
        std::string msg = Protocol::clean(rawMsg);
        std::string tag = Protocol::getTag(msg);

        if (tag == TAG_CHAT)
        {
            chat->relayChat(from, Protocol::getData(msg));
        }
        else if (tag == TAG_GAME)
        {
            handleMove(from, msg);
        }
        else if (tag == TAG_WIN)
        {
            handleResult(from, GameStatus::WHITE_WIN, GameStatus::BLACK_WIN);
        }
        else if (tag == TAG_LOSS)
        {
            handleResult(from, GameStatus::BLACK_WIN, GameStatus::WHITE_WIN);
        }
        else if (tag == TAG_TIE)
        {
            game->setResult(GameStatus::TIE);
            chat->broadcast(Protocol::build(TAG_TIE, ""));
            chat->broadcast(Protocol::build(TAG_END, ""));
        }
        else
        {
            logs->warning("GameRoom (Hash Map):- game room [" + std::to_string(roomId) + "] unknown tag, " + tag);
        }
    }

    int getRoomId()
    {
        return roomId;
    }

    GameStatus getStatus()
    {
        return game->getStatus();
    }

    bool isFinished()
    {
        return game->isFinished();
    }

    ChatSession *getChat()
    {
        return chat;
    }

    GameSession *getGame()
    {
        return game;
    }

private:
    void handleMove(QTcpSocket *from, const std::string &msg)
    {
        char turn = validator->getBoardTurn();

        if (turn == 'w' && from != chat->getPlayer1())
        {
            logs->warning("GameRoom (Hash Map):- game room [" + std::to_string(roomId) + "] Rejected Black player move on White's turn");
            from->write(Protocol::build(TAG_GAME, "INVALID").c_str());
            from->flush();
            return;
        }

        if (turn == 'b' && from != chat->getPlayer2())
        {
            logs->warning("GameRoom (Hash Map):- game room [" + std::to_string(roomId) + "] Rejected White player move on Black's turn");
            from->write(Protocol::build(TAG_GAME, "INVALID").c_str());
            from->flush();
            return;
        }

        std::string outMove;
        std::string outBoard;

        bool ok = validator->validate(msg, outMove, outBoard);

        if (ok)
        {
            game->recordMove(outMove);

            std::string relay = Protocol::build(TAG_GAME, outMove + "|" + outBoard);

            if (from == chat->getPlayer1())
            {
                chat->sendToPlayer2(relay);
            }
            else
                chat->sendToPlayer1(relay);

            logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                       "] accept move, " + outMove);
        }
        else
        {
            from->write(Protocol::build(TAG_GAME, "INVALID").c_str());
            from->flush();

            logs->warning("GameRoom [" + std::to_string(roomId) +
                          "] reject move");
        }
    }

    void handleResult(QTcpSocket *from,
                      GameStatus senderWins,
                      GameStatus senderLoses)
    {
        if (from == chat->getPlayer1())
        {
            game->setResult(senderWins);
            chat->sendToPlayer1(Protocol::build(TAG_WIN, ""));
            chat->sendToPlayer2(Protocol::build(TAG_LOSS, ""));
        }
        else
        {
            game->setResult(senderLoses);
            chat->sendToPlayer2(Protocol::build(TAG_WIN, ""));
            chat->sendToPlayer1(Protocol::build(TAG_LOSS, ""));
        }

        chat->broadcast(Protocol::build(TAG_END, ""));
        game->displayMoves();

        logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) + "] finished");
    }
};
