#pragma once
#include <string>
#include "ChessBoard.h"
#include "Protocol.h"
#include "Logger.h"

class ChessValidator
{
    private:
        Logger *logs;
        ChessBoard board;
        std::string pendingPromoMove;

    public:
        ChessValidator(Logger *logger) : logs(logger) {}

        char getBoardTurn() const
        {
            return board.turn;
        }

        bool isAwaitingPromotion() const
        {
            return board.isAwaitingPromotion();
        }

        const std::string &getPendingPromoMove() const
        {
            return pendingPromoMove;
        }

        bool isCheckmate(char color) const
        {
            return board.isCheckmate(color);
        }

        bool isStalemate(char color) const
        {
            return board.isStalemate(color);
        }

        bool isKingInCheck(char color) const
        {
            return board.isKingInCheck(color);
        }

        bool hasInsufficientMaterial() const
        {
            return board.hasInsufficientMaterial();
        }

        bool validate(const std::string &rawMsg, std::string &outMove, std::string &outBoardState, bool &needsPromotion)
        {
            needsPromotion = false;
            std::string move = Protocol::getData(rawMsg);

            while (!move.empty() && (move.back() == '\n' || move.back() == '\r'))
            {
                move.pop_back();
            }

            if (move.size() < 4)
            {
                logs->warning("Chess Validator:- move too short: " + move);
                outMove = "INVALID";
                return false;
            }

            bool ok = board.validateMove(move);

            if (ok)
            {
                if (board.needsPromotionChoice(move))
                {
                    board.applyMove(move);
                    pendingPromoMove = move;
                    outMove = move;
                    outBoardState = board.serialize();
                    needsPromotion = true;

                    logs->info("Chess Validator:- promotion required for " + move);
                    return true;
                }

                board.applyMove(move);
                pendingPromoMove = "";
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

        bool completePromotion(const std::string &rawMsg,nstd::string &outMove, std::string &outBoardState)
        {
            std::string data = Protocol::getData(rawMsg);

            while (!data.empty() && (data.back() == '\n' || data.back() == '\r'))
            {
                data.pop_back();
            }

            if (!board.isAwaitingPromotion() || pendingPromoMove.empty())
            {
                return false;
            }

            std::string move = data;
            if (move.size() == 1 && ChessBoard::isValidPromoChar(move[0]))
            {
                move = pendingPromoMove + move[0];
            }

            if (move.size() != 5)
            {
                return false;
            }

            if (move.substr(0, 4) != pendingPromoMove)
            {
                return false;
            }

            if (!ChessBoard::isValidPromoChar(move[4]))
            {
                return false;
            }

            if (!board.isPromotionChoiceLegal(move[4]))
            {
                return false;
            }

            if (!board.finishPromotion(move[4]))
            {
                return false;
            }

            pendingPromoMove = "";
            outMove = move;
            outBoardState = board.serialize();

            logs->info("Chess Validator:- promotion completed " + move);

            return true;
        }
};