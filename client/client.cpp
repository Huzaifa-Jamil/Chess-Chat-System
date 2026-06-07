#include <QCoreApplication>
#include <QTcpSocket>
#include <QThread>
#include <QMetaObject>
#include <QMutex>
#include <QTextStream>
#include <iostream>
#include <atomic>
#include "Protocol.h"
#include "Chess.h"

QTcpSocket socket;
QMutex ioMutex;

std::atomic<bool> authenticated(false);
std::atomic<bool> running(true);

GameState game;
ChatState chat;

static void safePrint(const std::string &s)
{
    QMutexLocker locker(&ioMutex);
    std::cout << "\n"
              << s << std::endl;
    std::cout << "> " << std::flush;
}

static void sendMsg(const std::string &tag, const std::string &data)
{
    QMetaObject::invokeMethod(&socket, [=]()
                              {
        socket.write(Protocol::build(tag, data).c_str());
        socket.flush(); }, Qt::QueuedConnection);
}

static void printBoard()
{
    std::string out;
    out += "\n";

    for (int r = 0; r < 8; r++)
    {
        out += std::to_string(8 - r) + " ";
        for (int c = 0; c < 8; c++)
        {
            char p = game.board[r][c].type;

            if (game.board[r][c].empty())
                out += ". ";
            else
                out += std::string(1, p) + " ";
        }
        out += "\n";
    }

    out += "  a b c d e f g h";
    safePrint(out);
}

static void inputLoop()
{
    QTextStream in(stdin);

    while (running)
    {
        std::string line = in.readLine().toStdString();

        if (!running)
            break;

        if (line == "quit")
        {
            running = false;
            QMetaObject::invokeMethod(&socket, []()
                                      { socket.disconnectFromHost(); }, Qt::QueuedConnection);
            break;
        }

        if (line == "board")
        {
            printBoard();
            continue;
        }

        if (line.rfind("chat ", 0) == 0)
        {
            std::string msg = line.substr(5);
            safePrint("[You] " + msg);
            sendMsg(TAG_CHAT, msg);
            continue;
        }

        if (line.rfind("move ", 0) == 0)
        {
            std::string mv = line.substr(5);

            if (game.validateMove(mv))
            {
                game.applyMove(mv);
                safePrint("[System] Move sent: " + mv);
                sendMsg(TAG_GAME, mv + "|" + game.serialize());
                printBoard();
            }
            else
            {
                safePrint("[System] Illegal move");
            }
            continue;
        }

        safePrint("[System] Commands: move e2e4 | chat hello | board | quit");
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    socket.connectToHost("127.0.0.1", 8080);

    QObject::connect(&socket, &QTcpSocket::connected, []()
                     { safePrint("[Server] Connected"); });

    QObject::connect(&socket, &QTcpSocket::readyRead, [&]()
                     {
        while (socket.canReadLine())
        {
            std::string msg = socket.readLine().toStdString();
            msg = Protocol::clean(msg);

            std::string tag = Protocol::getTag(msg);
            std::string data = Protocol::getData(msg);

            if (tag == TAG_AUTH)
            {
                if (data.find("SEND_TOKEN") != std::string::npos)
                {
                    sendMsg(TAG_AUTH, "CHESS123456PLEASEAUTH");
                }
                else if (data.find("OK") != std::string::npos)
                {
                    authenticated = true;
                    safePrint("[System] Authenticated");
                }
            }
            else if (tag == TAG_CHAT)
            {
                safePrint("[Opponent] " + data);
            }
            else if (tag == TAG_GAME)
            {
                size_t p = data.find('|');
                if (p != std::string::npos)
                {
                    std::string mv = data.substr(0, p);
                    std::string board = data.substr(p + 1);
                    game.deserialize(board);
                    safePrint("[System] Opponent move: " + mv);
                    printBoard();
                }
            }
            else if (tag == TAG_START)
            {
                safePrint("[System] Game Started");
                game.reset();
                printBoard();
            }
            else if (tag == TAG_WIN)
                safePrint("*** YOU WIN ***");
            else if (tag == TAG_LOSS)
                safePrint("*** YOU LOSE ***");
            else if (tag == TAG_TIE)
                safePrint("*** DRAW ***");
        } });

    QObject::connect(&socket, &QTcpSocket::disconnected, []()
                     {
        safePrint("[Server] Disconnected");
        running = false;
        QCoreApplication::quit(); });

    QThread *t = QThread::create(inputLoop);
    t->start();

    return a.exec();
}