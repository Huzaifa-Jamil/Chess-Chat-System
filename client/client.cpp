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

QTcpSocket socket;
QMutex ioMutex;

std::atomic<bool> authenticated(false);
std::atomic<bool> running(true);

GameState game;
ChatState chat;
char myColor = 'w';

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

    out += "  -----------------------------------------\n";

    if (myColor == 'b')
    {
        // you are black flipp Rows (7 to 0), Flipped Columns (7 to 0)
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

        out += "    a    b    c    d    e    f    g    h";
    }
    else
    {
        // White's board has standard Rows (0 to 7), Standard Columns (0 to 7)
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

        if (line.empty())
            continue;

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

        // Its a chess move   if it ends with '/'
        if (line.back() == '/')
        {
            std::string mv = line.substr(0, line.size() - 1);
            if (mv.empty())
            {
                safePrint("[System] Empty move command");
                continue;
            }

            if (game.validateMove(mv))
            {
                game.applyMove(mv);
                safePrint("[System] Move sent: " + mv);
                sendMsg(TAG_GAME, mv);
                printBoard();
            }
            else
            {
                safePrint("[System] Illegal move");
            }
            continue;
        }

        // treat everything else as a direct chat message
        safePrint("[You] " + line);
        sendMsg(TAG_CHAT, line);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // this code is only used for testing purpose on local host

    std::string targetServerIp = "127.0.0.1";
    std::string localBindIp = "";

    if (argc > 1)
    {
        std::string arg = argv[1];
        if (arg.rfind("127.", 0) == 0 || arg == "::1")
        {
            // If loopback IP is provided, bind local client endpoint to it
            // so the server sees the connection originating from this distinct IP
            localBindIp = arg;
            targetServerIp = "127.0.0.1";
        }
        else
        {
            // Otherwise, treat as target remote server IP (GCP public deployment)
            targetServerIp = arg;
        }
    }

    if (!localBindIp.empty())
    {
        socket.bind(QHostAddress(QString::fromStdString(localBindIp)), 0);
    }

    // I can change the targetserverip to my public gcp servers ip

    socket.connectToHost(QString::fromStdString(targetServerIp), 8080);

    QObject::connect(&socket, &QTcpSocket::connected, []()
                     { safePrint("[Server] Connection Successfull"); });

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
                    safePrint("[System] Server is asking for AUTHENTACTION TOKEN");
                    sendMsg(TAG_AUTH, "CHESS123456PLEASEAUTH");
                    safePrint("[System] AUTHENTACTION TOKEN sent to Server");
                }
                else if (data.find("OK") != std::string::npos)
                {
                    authenticated = true;
                    safePrint("[System] Authentication Successfull");
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
                    safePrint("[System] Move rejected by server!");
                }
                else
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
                        game.deserialize(boardState);

                        if (myColor == 'w')
                        {
                            safePrint("[System] Game Started against User " + oppId + "! You are playing as " + "White");
                        }
                        else 
                        {
                            safePrint("[System] Game Started against User " + oppId + "! You are playing as " + "Black");
                        }
                    }
                    else
                    {
                        safePrint("[System] Game Started");
                        game.reset();
                    }
                }
                else
                {
                    safePrint("[System] Game Started");
                    game.reset();
                }
                printBoard();
            }
            else if (tag == TAG_WIN)
                safePrint("[System] *** You WIN ***");
            else if (tag == TAG_LOSS)
                safePrint("[System] *** You LOSE ***");
            else if (tag == TAG_TIE)
                safePrint("[System] *** Its a DRAW ***");
            else if (tag == TAG_WAIT)
                safePrint("[Server] " + data);
            else if (tag == TAG_END)
                safePrint("[System] Match ended");
        } });

    QObject::connect(&socket, &QTcpSocket::disconnected, []()
                     {
        safePrint("[Server] Server Disconnected during mid session");
        running = false;
        QCoreApplication::quit(); });

    QThread *t = QThread::create(inputLoop);
    t->start();

    return a.exec();
}