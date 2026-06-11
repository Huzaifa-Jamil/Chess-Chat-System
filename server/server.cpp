#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <string>
#include "Logger.h"
#include "AVLTree.h"
#include "Queue.h"
#include "UserIdManager.h"
#include "LinkedList.h"
#include "HashMap.h"
#include "Protocol.h"
#include "FriendGraph.h"
#include "GameSession.h"
#include "ChatSession.h"
#include "GameRoom.h"
#include "GameRoomMap.h"
#include <QTimer>

// variables
Logger logs;
UserIdManager idManager;
AVLTree users(&logs);
Queue<QTcpSocket *> waitingQueue(&logs);
LinkedList pendingUsers(&logs);
HashMap activeUsers(&logs);
FriendGraph friendGraph(&logs);
GameRoomMap activeRooms(&friendGraph, &logs);

// constants
static const std::string AUTH_TOKEN = "CHESS123456PLEASEAUTH";
static const int MAX_TOKEN_SIZE = 21;

// methods
void handleQueue();
void promoteToServer(QTcpSocket *socket, std::string ip);
void handleNewConnection(QTcpSocket *socket);

void handleQueue()
{
    if (waitingQueue.getSize() < 2)
    {
        return;
    }

    int idPlayer1 = waitingQueue.getFrontId();
    waitingQueue.dequeue();

    int idPlayer2 = waitingQueue.getFrontId();
    waitingQueue.dequeue();

    Node *player1 = users.findById(idPlayer1);
    Node *player2 = users.findById(idPlayer2);

    if (player1 == NULL || player2 == NULL)
    {
        logs.info("System:- Match failed, one of the users not found");
        return;
    }

    // Change player's status
    player1->isPlaying = true;
    player2->isPlaying = true;

    // Disconnect old signals
    QObject::disconnect(player1->socket, &QTcpSocket::readyRead, nullptr, nullptr);
    QObject::disconnect(player1->socket, &QTcpSocket::disconnected, nullptr, nullptr);
    QObject::disconnect(player2->socket, &QTcpSocket::readyRead, nullptr, nullptr);
    QObject::disconnect(player2->socket, &QTcpSocket::disconnected, nullptr, nullptr);

    // Create room
    int roomId = activeRooms.createRoom(player1->socket, idPlayer1, player2->socket, idPlayer2);

    activeRooms.display();

    QObject::connect(player1->socket, &QTcpSocket::readyRead, [=]()
    {
        while (player1->socket->canReadLine())
        {
            QByteArray raw  = player1->socket->readLine();
            std::string msg = raw.toStdString();

            GameRoom *room = activeRooms.getRoom(roomId);

            if (room == NULL) 
            {
                return;
            }

            room->handleMessage(player1->socket, msg);

            if (room->isFinished())
            {
                activeRooms.closeRoom(roomId);
                friendGraph.displayAll();

                // Reenqueue player1
                Node *n1 = users.findById(idPlayer1);

                if (n1 != NULL && n1->isConnected)
                {
                    n1->isPlaying = false;
                    waitingQueue.enqueue(idPlayer1, n1->socket);
                    n1->socket->write(Protocol::build(TAG_WAIT, "Game over! Looking for next opponent...").c_str());
                    n1->socket->flush();
                    
                    QObject::connect(n1->socket, &QTcpSocket::disconnected, [=]()
                    {
                        Node *node = users.findById(idPlayer1);

                        if (node != NULL) {
                            node->isConnected = false;
                            node->isPlaying = false;
                        }

                        activeUsers.remove(idPlayer1);
                        waitingQueue.remove(idPlayer1);
                        logs.info("Server:- User " + std::to_string(idPlayer1) + " disconnected while waiting in queue");
                    });
                }

                // Reenqueue player2
                Node *n2 = users.findById(idPlayer2);

                if (n2 != NULL && n2->isConnected)
                {
                    n2->isPlaying = false;
                    waitingQueue.enqueue(idPlayer2, n2->socket);
                    n2->socket->write(Protocol::build(TAG_WAIT, "Game over! Looking for next opponent...").c_str());
                    n2->socket->flush();
                    
                    QObject::connect(n2->socket, &QTcpSocket::disconnected, [=]()
                    {
                        Node *node = users.findById(idPlayer2);

                        if (node != NULL) {
                            node->isConnected = false;
                            node->isPlaying = false;
                        }

                        activeUsers.remove(idPlayer2);
                        waitingQueue.remove(idPlayer2);
                        logs.info("Server:- User " + std::to_string(idPlayer2) + " disconnected while waiting in queue");
                    });
                }

                handleQueue();
                return;
            }
        } 
    });

    QObject::connect(player2->socket, &QTcpSocket::readyRead, [=]()
    {
        while (player2->socket->canReadLine())
        {
            QByteArray raw  = player2->socket->readLine();
            std::string msg = raw.toStdString();

            GameRoom *room = activeRooms.getRoom(roomId);
            if (room == NULL)
            {
                return;
            }

            room->handleMessage(player2->socket, msg);

            if (room->isFinished())
            {
                activeRooms.closeRoom(roomId);
                friendGraph.displayAll();

                // Reenqueue player1
                Node *n1 = users.findById(idPlayer1);

                if (n1 != NULL && n1->isConnected)
                {
                    n1->isPlaying = false;
                    waitingQueue.enqueue(idPlayer1, n1->socket);

                    n1->socket->write(Protocol::build(TAG_WAIT, "Game over! Looking for next opponent...").c_str());
                    n1->socket->flush();
                    
                    QObject::connect(n1->socket, &QTcpSocket::disconnected, [=]()
                    {
                        Node *node = users.findById(idPlayer1);

                        if (node != NULL) {
                            node->isConnected = false;
                            node->isPlaying = false;
                        }

                        activeUsers.remove(idPlayer1);
                        waitingQueue.remove(idPlayer1);

                        logs.info("Server:- User " + std::to_string(idPlayer1) + " disconnected while waiting in queue");
                    });
                }

                // Reenqueue player2
                Node *n2 = users.findById(idPlayer2);

                if (n2 != NULL && n2->isConnected)
                {
                    n2->isPlaying = false;
                    waitingQueue.enqueue(idPlayer2, n2->socket);
                    n2->socket->write(Protocol::build(TAG_WAIT, "Game over! Looking for next opponent...").c_str());
                    n2->socket->flush();
                    
                    QObject::connect(n2->socket, &QTcpSocket::disconnected, [=]()
                    {
                        Node *node = users.findById(idPlayer2);

                        if (node != NULL) {
                            node->isConnected = false;
                            node->isPlaying = false;
                        }

                        activeUsers.remove(idPlayer2);
                        waitingQueue.remove(idPlayer2);
                        logs.info("Server:- User " + std::to_string(idPlayer2) + " disconnected while waiting in queue");
                    });
                }

                handleQueue();
                break;
            }
        } 
    });

    QObject::connect(player1->socket, &QTcpSocket::disconnected, [=]()
    {
        Node *node = users.findById(idPlayer1);

        if (node != NULL)
        {
            node->isConnected = false;
            node->isPlaying   = false;

            activeUsers.remove(idPlayer1);
            waitingQueue.remove(idPlayer1);

            logs.info("System:- User " + std::to_string(idPlayer1) + " disconnected during game");
        }

        // Disconnect readyRead on the disconnected player
        QObject::disconnect(player1->socket, &QTcpSocket::readyRead, nullptr, nullptr);

        GameRoom *room = activeRooms.getRoom(roomId);
        if (room != NULL && !room->isFinished())
        {
            room->notifyDisconnect(player1->socket);
            activeRooms.closeRoom(roomId);
            friendGraph.displayAll();

            // Disconnect game signals from survivor before reenqueue
            QObject::disconnect(player2->socket, &QTcpSocket::readyRead, nullptr, nullptr);
            QObject::disconnect(player2->socket, &QTcpSocket::disconnected, nullptr, nullptr);

            // Reenqueue the other player
            Node *survivor = users.findById(idPlayer2);
            if (survivor != NULL && survivor->isConnected)
            {
                survivor->isPlaying = false;
                waitingQueue.enqueue(idPlayer2, survivor->socket);
                survivor->socket->write(Protocol::build(TAG_WAIT, "Opponent disconnected! Looking for next opponent...").c_str());
                survivor->socket->flush();
                
                QObject::connect(survivor->socket, &QTcpSocket::disconnected, [=]()
                {
                    Node *node = users.findById(idPlayer2);
                    if (node != NULL) {
                        node->isConnected = false;
                        node->isPlaying = false;
                    }

                    activeUsers.remove(idPlayer2);
                    waitingQueue.remove(idPlayer2);

                    logs.info("Server:- User " + std::to_string(idPlayer2) + " disconnected while waiting in queue");
                });
                
                handleQueue();
            }
        } 
    });

    QObject::connect(player2->socket, &QTcpSocket::disconnected, [=]()
    {
        Node *node = users.findById(idPlayer2);
        if (node != NULL)
        {
            node->isConnected = false;
            node->isPlaying   = false;
            activeUsers.remove(idPlayer2);
            waitingQueue.remove(idPlayer2);

            logs.info("System:- User " + std::to_string(idPlayer2) + " disconnected during game");
        }

        // Disconnect readyRead on the disconnected player to prevent stale lambda firing
        QObject::disconnect(player2->socket, &QTcpSocket::readyRead, nullptr, nullptr);

        GameRoom *room = activeRooms.getRoom(roomId);
        if (room != NULL && !room->isFinished())
        {
            room->notifyDisconnect(player2->socket);
            activeRooms.closeRoom(roomId);
            friendGraph.displayAll();

            // Disconnect game signals from survivor before reenqueue
            QObject::disconnect(player1->socket, &QTcpSocket::readyRead, nullptr, nullptr);
            QObject::disconnect(player1->socket, &QTcpSocket::disconnected, nullptr, nullptr);

            // Reenqueue the other player
            Node *survivor = users.findById(idPlayer1);

            if (survivor != NULL && survivor->isConnected)
            {
                survivor->isPlaying = false;
                waitingQueue.enqueue(idPlayer1, survivor->socket);
                survivor->socket->write(Protocol::build(TAG_WAIT, "Opponent disconnected! Looking for next opponent...").c_str());
                survivor->socket->flush();
                
                QObject::connect(survivor->socket, &QTcpSocket::disconnected, [=]()
                {
                    Node *node = users.findById(idPlayer1);

                    if (node != NULL) {
                        node->isConnected = false;
                        node->isPlaying = false;
                    }

                    activeUsers.remove(idPlayer1);
                    waitingQueue.remove(idPlayer1);
                    logs.info("Server:- User " + std::to_string(idPlayer1) + " disconnected while waiting in queue");
                });
                
                handleQueue();
            }
        } 
    });
}

void promoteToServer(QTcpSocket *socket, std::string ip)
{
    Node *existing = users.findByIp(ip);
    int assignedId = 0;

    if (existing != NULL)
    {
        if (activeUsers.exists(existing->userId))
        {
            logs.error("Server:- Duplicate login attempt for active user " + std::to_string(existing->userId));
            socket->write(Protocol::build(TAG_AUTH, "DUPLICATE").c_str());
            socket->flush();
            socket->disconnectFromHost();
            return;
        }

        users.reconnectUser(ip, socket);
        activeUsers.insert(existing->userId, socket);
        assignedId = existing->userId;

        // simple Welcome back message to old user
        socket->write(Protocol::build(TAG_WAIT, "Welcome Back Player " + std::to_string(assignedId) + " To Chess and Chat System Made by Huzaifa Jamil").c_str());
        socket->flush();

        socket->write(Protocol::build(TAG_AUTH, "RECONNECT").c_str());
        socket->flush();

        logs.info("Server:- User " + std::to_string(existing->userId) + " reconnected from " + ip);

        waitingQueue.enqueue(existing->userId, socket);
    }
    else
    {
        int id = idManager.assignNextUserId();
        users.insertUser(id, socket, ip);
        activeUsers.insert(id, socket);
        assignedId = id;

        socket->write(Protocol::build(TAG_AUTH, "OK|" + std::to_string(id)).c_str());
        socket->flush();

        logs.info("Server:- New user " + std::to_string(id) + " connected from " + ip);

        // simple Welcome message to new user
        socket->write(Protocol::build(TAG_WAIT, "Welcome To Chess and Chat System Made by Huzaifa Jamil. Hope you will enjoy it, Player " + std::to_string(id)).c_str());
        socket->flush();

        waitingQueue.enqueue(id, socket);
    }

    QObject::connect(socket, &QTcpSocket::disconnected, [=]()
    {
        Node *node = users.findById(assignedId);

        if (node != NULL)
        {
            node->isConnected = false;
            node->isPlaying   = false;
        }

        activeUsers.remove(assignedId);
        waitingQueue.remove(assignedId);

        logs.info("Server:- User " + std::to_string(assignedId) + " disconnected while waiting in queue"); 
    });

    socket->write(Protocol::build(TAG_WAIT, "Looking for opponent...").c_str());
    socket->flush();

    handleQueue();
}

void handleNewConnection(QTcpSocket *socket)
{
    std::string ip = socket->peerAddress().toString().toStdString();

    logs.info("Server:- New connection from " + ip + ", waiting for auth");

    pendingUsers.insert(socket, ip);

    socket->write(Protocol::build(TAG_AUTH, "SEND_TOKEN").c_str());
    socket->flush();

    QTimer *authTimer = new QTimer();
    authTimer->setSingleShot(true);

    QObject::connect(authTimer, &QTimer::timeout, [=]()
    {
        LinkedListNode *node = pendingUsers.find(socket);

        if (node != NULL)
        {
            logs.info("Server:- Auth timeout for " + ip);
            pendingUsers.remove(socket);
            socket->disconnectFromHost();
        }

        authTimer->deleteLater(); 
    });

    QObject::connect(socket, &QTcpSocket::readyRead, [=]()
    {
        LinkedListNode *node = pendingUsers.find(socket);

        if (node == NULL) 
        {
            return;
        }

        while (socket->canReadLine())
        {
            std::string msg = socket->readLine().toStdString();

            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
            {
                msg.pop_back();
            }

            std::string tag  = Protocol::getTag(msg);
            std::string data = Protocol::getData(msg);

            if (tag == TAG_AUTH && data == AUTH_TOKEN)
            {
                logs.info("Server:- Auth success from " + ip);
                std::string nodeIp = node->ip;
                pendingUsers.remove(socket);

                authTimer->stop();
                authTimer->deleteLater();
                QObject::disconnect(socket, &QTcpSocket::readyRead, nullptr, nullptr);
                QObject::disconnect(socket, &QTcpSocket::disconnected, nullptr, nullptr);

                promoteToServer(socket, nodeIp);
                return;
            }
            else
            {
                socket->write(Protocol::build(TAG_AUTH, "FAIL").c_str());
                socket->flush();
                logs.info("Server:- Auth failed from " + ip);
                pendingUsers.remove(socket);
                socket->disconnectFromHost();
                return;
            }
        } 
    });

    QObject::connect(socket, &QTcpSocket::disconnected, [=]()
    {
        authTimer->stop();
        authTimer->deleteLater();

        LinkedListNode *node = pendingUsers.find(socket);
                         
        if (node != NULL)
        {
            logs.info("Server:- Unauthenticated user dropped " + ip);
            pendingUsers.remove(socket);
        }

        socket->deleteLater(); // Socket should be deleted if disconnected BEFORE authentication
    });

    authTimer->start(10000); // 10s timeout
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTcpServer server;

    server.listen(QHostAddress::Any, 8080);

    logs.info("Server is waiting for clients all over the world on port 8080");

    QObject::connect(&server, &QTcpServer::newConnection, [&]()
    {
        QTcpSocket *socket = server.nextPendingConnection();
        handleNewConnection(socket); 
    });

    return a.exec();
}