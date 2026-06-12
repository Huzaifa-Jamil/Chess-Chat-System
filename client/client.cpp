#include <QCoreApplication>
#include <QTcpSocket>
#include <QHostAddress>
#include <QThread>
#include <QMetaObject>
#include <QMutex>
#include <QTextStream>
#include <iostream>
#include <atomic>
#include "Protocol.h"
#include "Chess.h"

QTcpSocket *pSocket = nullptr;
QMutex ioMutex;
QMutex gameMutex;

std::atomic<bool> authenticated(false);
std::atomic<bool> running(true);
std::atomic<bool> awaitingPromotion(false);
std::string pendingPromoMove;

GameState game;
ChatState chat;
char myColor = 'w';

static void safePrint(const std::string &s)
{
    QMutexLocker locker(&ioMutex);
    std::cout << "\n" << s << std::endl;
    std::cout << "> " << std::flush;
}

static void sendMsg(const std::string &tag, const std::string &data)
{
    if (!pSocket)
    {
        return;
    }

    QMetaObject::invokeMethod(pSocket, [=]()
    {
        pSocket->write(Protocol::build(tag, data).c_str());
        pSocket->flush(); }, Qt::QueuedConnection);
}

static void printBoard()
{
    std::string out;
    {
        QMutexLocker lock(&gameMutex);
        out += "\n";
        out += "  -----------------------------------------\n";

        if (myColor == 'b')
        {
            for (int r = 7; r >= 0; r--)
            {
                out += std::to_string(8 - r) + " |";
                for (int c = 7; c >= 0; c--)
                {
                    if (game.board[r][c].empty())
                    {
                        out += " .. |";
                    }
                    else
                    {
                        char p = game.board[r][c].type;
                        if (p >= 'A' && p <= 'Z')
                        {
                            out += " w";
                            out += p;
                            out += " |";
                        }
                        else
                        {
                            out += " b";
                            out += (char)(p - 'a' + 'A');
                            out += " |";
                        }
                    }
                }
                out += "\n";
                out += "  -----------------------------------------\n";
            }
            out += "    h    g    f    e    d    c    b    a";
        }
        else
        {
            for (int r = 0; r < 8; r++)
            {
                out += std::to_string(8 - r) + " |";
                for (int c = 0; c < 8; c++)
                {
                    if (game.board[r][c].empty())
                    {
                        out += " .. |";
                    }
                    else
                    {
                        char p = game.board[r][c].type;

                        if (p >= 'A' && p <= 'Z')
                        {
                            out += " w";
                            out += p;
                            out += " |";
                        }
                        else
                        {
                            out += " b";
                            out += (char)(p - 'a' + 'A');
                            out += " |";
                        }
                    }
                }
                out += "\n";
                out += "  -----------------------------------------\n";
            }
            out += "    a    b    c    d    e    f    g    h";
        }
    } // gameMutex released here before calling safePrint

    safePrint(out);
}

static void inputLoop()
{
    while (running)
    {
        std::string line;
        if (!std::getline(std::cin, line))
        {
            break;
        }

        if (!running)
        {
            break;
        }

        if (line.empty())
        {
            continue;
        }

        if (line == "quit")
        {
            running = false;
            if (pSocket)
            {
                QMetaObject::invokeMethod(pSocket, []()
                {
                    pSocket->disconnectFromHost(); }, Qt::QueuedConnection);
            }

            break;
        }

        if (line == "resign")
        {
            safePrint("[System] You resigned");
            sendMsg(TAG_GAME, "RESIGN");
            continue;
        }

        if (line == "board")
        {
            printBoard();
            continue;
        }

        if (awaitingPromotion)
        {
            std::string promo = line;
            if (promo.size() == 1 && (promo[0] == 'q' || promo[0] == 'r' || promo[0] == 'b' || promo[0] == 'n'))
            {
                safePrint("[System] Promotion sent: " + pendingPromoMove + promo);
                sendMsg(TAG_GAME, pendingPromoMove + promo);
                awaitingPromotion = false;
            }
            else if (promo.size() == 5 && promo.substr(0, 4) == pendingPromoMove)
            {
                safePrint("[System] Promotion sent: " + promo);
                sendMsg(TAG_GAME, promo);
                awaitingPromotion = false;
            }
            else
            {
                safePrint("[System] Choose pawn promotion: q, r, b, or n (or full move like " + pendingPromoMove + "q)");
            }
            continue;
        }

        if (line.back() == '/')
        {
            std::string mv = line.substr(0, line.size() - 1);
            if (mv.empty())
            {
                safePrint("[System] Warning, Your move command is empty");
                continue;
            }

            bool ok = false;
            char turn = 'w';

            QMutexLocker lock(&gameMutex);
            turn = game.chessBoard.turn;

            if (turn != myColor)
            {
                safePrint("[System] Warning, Not your turn (you are " + std::string(myColor == 'w' ? "White" : "Black") + ")");
            }
            else
            {
                ok = game.validateMove(mv);
            }

            if (turn == myColor && ok)
            {
                safePrint("[System] Move sent: " + mv);
                sendMsg(TAG_GAME, mv);
            }
            else if (turn == myColor)
            {
                safePrint("[System] Illegal move");
            }
            continue;
        }

        // Everything else is a chat message
        safePrint("[You] " + line);
        sendMsg(TAG_CHAT, line);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    pSocket = new QTcpSocket();

    pSocket->connectToHost("127.0.0.1", 8080);

    QObject::connect(pSocket, &QTcpSocket::connected, []()
    {
        safePrint("[Server] Connection Successful");
    });

    QObject::connect(pSocket, &QTcpSocket::readyRead, [&]()
    {
        while (pSocket->canReadLine())
        {
            std::string msg = pSocket->readLine().toStdString();
            msg = Protocol::clean(msg);

            std::string tag  = Protocol::getTag(msg);
            std::string data = Protocol::getData(msg);

            if (tag == TAG_AUTH)
            {
                if (data.find("SEND_TOKEN") != std::string::npos)
                {
                    safePrint("[System] Server is asking for AUTHENTICATION TOKEN");
                    sendMsg(TAG_AUTH, "CHESS123456PLEASEAUTH");
                    safePrint("[System] AUTHENTICATION TOKEN sent to Server");
                }
                else if (data.find("OK") != std::string::npos)
                {
                    authenticated = true;
                    safePrint("[System] Authentication Successful");
                }
            }
            else if (tag == TAG_CHAT)
            {
                safePrint("[Opponent] " + data);
            }
            else if (tag == TAG_GAME)
            {
                if (data == "INVALID")
                {
                    // Server rejected our move
                    safePrint("[Server] Move rejected by server!");
                }
                else if (data == "CHECK")
                {
                    safePrint("[System] WARNING: Your King is in CHECK!");
                }
                else if (data.find("PROMOTE|") == 0)
                {
                    pendingPromoMove = data.substr(8);
                    awaitingPromotion = true;
                    safePrint("[System] Pawn PROMOTION REQUIRED for " + pendingPromoMove + " Please enter q, r, b, or n");
                }
                else
                {
                    size_t p = data.find('|');
                    if (p != std::string::npos)
                    {
                        std::string mv    = data.substr(0, p);
                        std::string board = data.substr(p + 1);
                        {
                            QMutexLocker lock(&gameMutex);
                            game.deserialize(board);
                        }
                        safePrint("[Server] Move Verified By Server : " + mv);
                        printBoard();
                    }
                }
            }
            else if (tag == TAG_START)
            {
                size_t p1 = data.find('|');

                if (p1 != std::string::npos)
                {
                    std::string oppId = data.substr(0, p1);
                    size_t p2 = data.find('|', p1 + 1);
                    
                    if (p2 != std::string::npos)
                    {
                        std::string colorStr = data.substr(p1 + 1, p2 - p1 - 1);
                        std::string boardState = data.substr(p2 + 1);

                        if (!colorStr.empty())
                        {
                            myColor = colorStr[0];
                        }

                        QMutexLocker lock(&gameMutex);
                        game.deserialize(boardState);

                        std::string out;

                        out += "\n";
                        out += "=========================================\n";
                        out += "          THE BOARD IS SET\n";
                        out += "=========================================\n";
                        out += "\n";

                        out += "In chess, as in life,\n";
                        out += "every move has consequences.\n";
                        out += "\n";

                        out += "Two kings enter.\n";
                        out += "Only one shall prevail.\n";
                        out += "\n";

                        out += "Opponent : User " + oppId + "\n";

                        if (myColor == 'w')
                        {
                            out += "You are WHITE\n";
                            out += "> Opponent is BLACK\n";
                            out += "\n";
                            out += "White Commands First Move\n";
                            out += "Black Awaits the Challenge\n";
                        }
                        else
                        {
                            out += "You are BLACK\n";
                            out += "Opponent is WHITE\n";
                            out += "\n";
                            out += "White Commands First Move\n";
                            out += "Black Awaits the Challenge\n";
                        }

                        out += "\n";
                        out += "Let the battle begin!\n";
                        out += "=========================================\n";

                        safePrint(out);
                    }
                    else
                    {
                        safePrint("[System] Game Started");
                        QMutexLocker lock(&gameMutex);
                        game.reset();
                    }
                }
                else
                {
                    safePrint("[System] Game Started");
                    QMutexLocker lock(&gameMutex);
                    game.reset();
                }

                printBoard();
            }
            else if (tag == TAG_WIN)
            {
                awaitingPromotion = false;
                safePrint("[System] *** You WIN ***");
            }
            else if (tag == TAG_LOSS)
            {
                awaitingPromotion = false;
                safePrint("[System] *** You LOSE ***");
            }
            else if (tag == TAG_TIE)
            {
                safePrint("[System] *** It's a DRAW ***");
            }
            else if (tag == TAG_WAIT)
            {
                safePrint("[Server] " + data);
            }
            else if (tag == TAG_END)
            {
                awaitingPromotion = false;
                safePrint("[System] Match ended");
            }
        } 
    });

    QObject::connect(pSocket, &QTcpSocket::disconnected, []()
    {
        safePrint("[Server] Server Disconnected during mid session");
        running = false;
        QCoreApplication::quit(); 
    });

    QThread *t = QThread::create(inputLoop);
    t->start();

    int ret = a.exec();
    running = false;
    t->quit();
    t->wait(2000);
    delete t;
    delete pSocket;
    return ret;
}