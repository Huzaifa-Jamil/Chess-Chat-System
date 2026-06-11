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

        void broadcastMove(const std::string &outMove, const std::string &outBoard)
        {
            std::string relay = Protocol::build(TAG_GAME, outMove + "|" + outBoard);
            chat->sendToPlayer1(relay);
            chat->sendToPlayer2(relay);
        }

        void checkGameEndAfterMove()
        {
            if (game->isFinished())
            {
                chat->broadcast(Protocol::build(TAG_TIE, ""));
                chat->broadcast(Protocol::build(TAG_END, ""));
                return;
            }

            char nextTurn = validator->getBoardTurn();

            if (validator->isCheckmate(nextTurn))
            {
                GameStatus winner = (nextTurn == 'w')
                                        ? GameStatus::BLACK_WIN
                                        : GameStatus::WHITE_WIN;
                game->setResult(winner);

                if (winner == GameStatus::WHITE_WIN)
                {
                    chat->sendToPlayer1(Protocol::build(TAG_WIN, ""));
                    chat->sendToPlayer2(Protocol::build(TAG_LOSS, ""));
                    logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                            "] Checkmate — White wins");
                }
                else
                {
                    chat->sendToPlayer2(Protocol::build(TAG_WIN, ""));
                    chat->sendToPlayer1(Protocol::build(TAG_LOSS, ""));
                    logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                            "] Checkmate — Black wins");
                }
                chat->broadcast(Protocol::build(TAG_END, ""));
                game->displayMoves();
            }
            else if (validator->isStalemate(nextTurn))
            {
                game->setResult(GameStatus::TIE);
                chat->broadcast(Protocol::build(TAG_TIE, ""));
                chat->broadcast(Protocol::build(TAG_END, ""));
                logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                        "] Stalemate — draw");
                game->displayMoves();
            }
            else if (validator->hasInsufficientMaterial())
            {
                game->setResult(GameStatus::TIE);
                chat->broadcast(Protocol::build(TAG_TIE, ""));
                chat->broadcast(Protocol::build(TAG_END, ""));
                logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                        "] Insufficient material — draw");
                game->displayMoves();
            }
            else if (validator->isKingInCheck(nextTurn))
            {
                if (nextTurn == 'w')
                {
                    chat->sendToPlayer1(Protocol::build(TAG_GAME, "CHECK"));
                }
                else
                {
                    chat->sendToPlayer2(Protocol::build(TAG_GAME, "CHECK"));
                }
                logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                        "] King in CHECK for " +
                        (nextTurn == 'w' ? std::string("White") : std::string("Black")));
            }
        }

        bool isCorrectPlayer(QTcpSocket *from, char turn)
        {
            if (turn == 'w' && from != chat->getPlayer1())
            {
                return false;
            }

            if (turn == 'b' && from != chat->getPlayer2())
            {
                return false;
            }
            
            return true;
        }

        void rejectMove(QTcpSocket *from)
        {
            from->write(Protocol::build(TAG_GAME, "INVALID").c_str());
            from->flush();
        }

        void handleResign(QTcpSocket *from)
        {
            if (game->isFinished())
            {
                return;
            }

            if (from == chat->getPlayer1())
            {
                game->setResult(GameStatus::BLACK_WIN);
                chat->sendToPlayer1(Protocol::build(TAG_LOSS, ""));
                chat->sendToPlayer2(Protocol::build(TAG_WIN, ""));
                logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                        "] White resigned, Black wins");
            }
            else if (from == chat->getPlayer2())
            {
                game->setResult(GameStatus::WHITE_WIN);
                chat->sendToPlayer2(Protocol::build(TAG_LOSS, ""));
                chat->sendToPlayer1(Protocol::build(TAG_WIN, ""));
                logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                        "] Black resigned, White wins");
            }

            chat->broadcast(Protocol::build(TAG_END, ""));
            game->displayMoves();
        }

        void handlePromotion(QTcpSocket *from, const std::string &msg)
        {
            char turn = validator->getBoardTurn();
            if (!isCorrectPlayer(from, turn))
            {
                logs->warning("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                            "] Rejected promotion from wrong player");
                rejectMove(from);
                return;
            }

            std::string outMove;
            std::string outBoard;
            bool ok = validator->completePromotion(msg, outMove, outBoard);

            if (!ok)
            {
                rejectMove(from);
                logs->warning("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                            "] rejected promotion");
                return;
            }

            game->recordMove(outMove);
            broadcastMove(outMove, outBoard);
            logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                    "] promotion accepted: " + outMove);
            checkGameEndAfterMove();
        }

        void handleMove(QTcpSocket *from, const std::string &msg)
        {
            if (game->isFinished())
                return;

            if (validator->isAwaitingPromotion())
            {
                handlePromotion(from, msg);
                return;
            }

            char turn = validator->getBoardTurn();

            if (!isCorrectPlayer(from, turn))
            {
                logs->warning("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                            "] Rejected move from wrong player");
                rejectMove(from);
                return;
            }

            std::string outMove;
            std::string outBoard;
            bool needsPromotion = false;

            bool ok = validator->validate(msg, outMove, outBoard, needsPromotion);

            if (ok)
            {
                game->recordMove(outMove);
                broadcastMove(outMove, outBoard);

                logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                        "] accepted move: " + outMove);

                if (needsPromotion)
                {
                    std::string promoReq = Protocol::build(TAG_GAME, "PROMOTE|" + outMove);
                    from->write(promoReq.c_str());
                    from->flush();
                    logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                            "] promotion required for " + outMove);
                    return;
                }

                checkGameEndAfterMove();
            }
            else
            {
                rejectMove(from);
                logs->warning("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                            "] rejected move");
            }
        }

    public:
        GameRoom(int id, QTcpSocket *p1, int userId1, QTcpSocket *p2, int userId2, Logger *logger)
        {
            roomId = id;
            logs = logger;
            chat = new ChatSession(p1, userId1, p2, userId2, logger);
            game = new GameSession(userId1, userId2, logger);
            validator = new ChessValidator(logger);

            logs->info("GameRoom (Hash Map):- game room " + std::to_string(roomId) +
                    " created for user " + std::to_string(userId1) +
                    " vs user " + std::to_string(userId2));
        }

        ~GameRoom()
        {
            delete chat;
            delete game;
            delete validator;
        }

        void start()
        {
            game->start();
            ChessBoard startBoard;
            std::string boardState = startBoard.serialize();
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
                logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                        "] White disconnected, Black wins");
            }
            else if (who == chat->getPlayer2())
            {
                game->setResult(GameStatus::WHITE_WIN);
                chat->sendToPlayer1(Protocol::build(TAG_WIN, ""));
                chat->sendToPlayer1(Protocol::build(TAG_END, ""));
                logs->info("GameRoom (Hash Map):- game room [" + std::to_string(roomId) +
                        "] Black disconnected, White wins");
            }
        }

        void handleMessage(QTcpSocket *from, const std::string &rawMsg)
        {
            std::string msg = Protocol::clean(rawMsg);
            std::string tag = Protocol::getTag(msg);
            std::string data = Protocol::getData(msg);

            if (tag == TAG_CHAT)
            {
                chat->relayChat(from, data);
            }
            else if (tag == TAG_GAME)
            {
                if (data == "RESIGN")
                {
                    handleResign(from);
                }
                else
                {
                    handleMove(from, msg);
                }
            }
            else if (tag == TAG_WIN || tag == TAG_LOSS || tag == TAG_TIE)
            {
                logs->warning("GameRoom [" + std::to_string(roomId) +
                            "] ignoring client result claim: " + tag);
            }
            else
            {
                logs->warning("GameRoom (Hash Map):- game room [" +
                            std::to_string(roomId) + "] unknown tag: " + tag);
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
};