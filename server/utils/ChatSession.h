#pragma once
#include <QTcpSocket>
#include <string>
#include "Logger.h"
#include "Protocol.h"

class ChatSession
{
    private:
        QTcpSocket *player1;
        QTcpSocket *player2;
        int id1; // userId of player1
        int id2; // userId of player2
        Logger *logs;

    public:
        ChatSession(QTcpSocket *p1, int userId1, QTcpSocket *p2, int userId2, Logger *logger)
        {
            player1 = p1;
            player2 = p2;
            id1 = userId1;
            id2 = userId2;
            logs = logger;
        }

        // Send a message to player1
        void sendToPlayer1(const std::string &msg)
        {
            if (player1 == NULL || player1->state() != QAbstractSocket::ConnectedState)
            {
                return;
            }

            player1->write(msg.c_str());
            player1->flush();
        }

        // Send a message to player2
        void sendToPlayer2(const std::string &msg)
        {
            if (player2 == NULL || player2->state() != QAbstractSocket::ConnectedState)
            {
                return;
            }

            player2->write(msg.c_str());
            player2->flush();
        }

        // Send the same message to both players
        void broadcast(const std::string &msg)
        {
            sendToPlayer1(msg);
            sendToPlayer2(msg);
        }

        void relayChat(QTcpSocket *from, const std::string &text)
        {
            if (from == player1)
            {
                std::string out = Protocol::build(TAG_CHAT, text);
                sendToPlayer2(out);
                logs->info("Chat:- user " + std::to_string(id1) + " -> user " + std::to_string(id2) + " | " + text);
            }
            else if (from == player2)
            {
                std::string out = Protocol::build(TAG_CHAT, text);
                sendToPlayer1(out);
                logs->info("Chat:- user " + std::to_string(id2) + " -> user " + std::to_string(id1) + " | " + text);
            }
        }

        QTcpSocket *getPlayer1()
        {
            return player1;
        }

        QTcpSocket *getPlayer2()
        {
            return player2;
        }

        int getId1()
        {
            return id1;
        }

        int getId2()
        {
            return id2;
        }
};