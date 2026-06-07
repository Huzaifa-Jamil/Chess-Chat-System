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

Logger logs;
UserIdManager idManager;
AVLTree users(&logs);
Queue<QTcpSocket *> waitingQueue(&logs);
LinkedList pendingUsers(&logs);
HashMap activeUsers(&logs);
FriendGraph friendGraph(&logs);
GameRoomMap activeRooms(&friendGraph, &logs);

static const std::string AUTH_TOKEN = "CHESS123456PLEASEAUTH";
static const int MAX_TOKEN_SIZE = 21;

void handleQueue();
void promoteToServer(QTcpSocket *socket, std::string ip);
void handleNewConnection(QTcpSocket *socket);

void handleQueue()
{
    if (waitingQueue.getSize() < 2)
        return;

    int idPlayer1 = waitingQueue.getFrontId();
    waitingQueue.dequeue();
    int idPlayer2 = waitingQueue.getFrontId();
    waitingQueue.dequeue();

    Node *player1 = users.findById(idPlayer1);
    Node *player2 = users.findById(idPlayer2);

    if (player1 == NULL || player2 == NULL)
    {
        logs.info("Match failed, one of the users not found");
        return;
    }

    player1->isPlaying = true;
    player2->isPlaying = true;

    // Create room
    int roomId = activeRooms.createRoom(
        player1->socket, idPlayer1,
        player2->socket, idPlayer2);

    activeRooms.display();

    QObject::connect(player1->socket, &QTcpSocket::readyRead, [=]()
                     {
        QByteArray raw  = player1->socket->readAll();
        std::string msg = raw.toStdString();

        GameRoom *room = activeRooms.getRoom(roomId);
        if (room == NULL) return;

        room->handleMessage(player1->socket, msg);

        if (room->isFinished())
        {
            activeRooms.closeRoom(roomId);
            friendGraph.displayAll();
        } });

    QObject::connect(player2->socket, &QTcpSocket::readyRead, [=]()
                     {
        QByteArray raw  = player2->socket->readAll();
        std::string msg = raw.toStdString();

        GameRoom *room = activeRooms.getRoom(roomId);
        if (room == NULL) return;

        room->handleMessage(player2->socket, msg);

        if (room->isFinished())
        {
            activeRooms.closeRoom(roomId);
            friendGraph.displayAll();
        } });

    QObject::connect(player1->socket, &QTcpSocket::disconnected, [=]()
                     {
        Node *node = users.findById(idPlayer1);
        if (node != NULL)
        {
            node->isConnected = false;
            node->isPlaying   = false;
            activeUsers.remove(idPlayer1);
            logs.info("User " + std::to_string(idPlayer1) + " disconnected during game");
        }

        GameRoom *room = activeRooms.getRoom(roomId);
        if (room != NULL && !room->isFinished())
        {
            room->notifyDisconnect(player1->socket);
            activeRooms.closeRoom(roomId);
            friendGraph.displayAll();

            // Re-enqueue the other player
            Node *survivor = users.findById(idPlayer2);
            if (survivor != NULL && survivor->isConnected)
            {
                survivor->isPlaying = false;
                waitingQueue.enqueue(idPlayer2, survivor->socket);
                handleQueue();
            }
        } });

    QObject::connect(player2->socket, &QTcpSocket::disconnected, [=]()
                     {
        Node *node = users.findById(idPlayer2);
        if (node != NULL)
        {
            node->isConnected = false;
            node->isPlaying   = false;
            activeUsers.remove(idPlayer2);
            logs.info("User " + std::to_string(idPlayer2) + " disconnected during game");
        }

        GameRoom *room = activeRooms.getRoom(roomId);
        if (room != NULL && !room->isFinished())
        {
            room->notifyDisconnect(player2->socket);
            activeRooms.closeRoom(roomId);
            friendGraph.displayAll();

            // Re-enqueue the other player
            Node *survivor = users.findById(idPlayer1);
            if (survivor != NULL && survivor->isConnected)
            {
                survivor->isPlaying = false;
                waitingQueue.enqueue(idPlayer1, survivor->socket);
                handleQueue();
            }
        } });
}

void promoteToServer(QTcpSocket *socket, std::string ip)
{
    Node *existing = users.findByIp(ip);

    if (existing != NULL)
    {
        if (activeUsers.exists(existing->userId))
        {
            logs.error("Duplicate login attempt for active user " +
                       std::to_string(existing->userId));
            socket->write(Protocol::build(TAG_AUTH, "DUPLICATE").c_str());
            socket->flush();
            socket->disconnectFromHost();
            return;
        }

        users.reconnectUser(ip, socket);
        activeUsers.insert(existing->userId, socket);

        socket->write(Protocol::build(TAG_AUTH, "RECONNECT").c_str());
        socket->flush();

        logs.info("User " + std::to_string(existing->userId) +
                  " reconnected from " + ip);

        waitingQueue.enqueue(existing->userId, socket);
    }
    else
    {
        int id = idManager.assignNextUserId();
        users.insertUser(id, socket, ip);
        activeUsers.insert(id, socket);

        socket->write(Protocol::build(TAG_AUTH, "OK|" + std::to_string(id)).c_str());
        socket->flush();

        logs.info("New user " + std::to_string(id) + " connected from " + ip);

        waitingQueue.enqueue(id, socket);
    }

    socket->write(Protocol::build(TAG_WAIT, "Looking for opponent...").c_str());
    socket->flush();

    handleQueue();
}

void handleNewConnection(QTcpSocket *socket)
{
    std::string ip = socket->peerAddress().toString().toStdString();
    logs.info("New connection from " + ip + ", waiting for auth");

    pendingUsers.insert(socket, ip);

    socket->write(Protocol::build(TAG_AUTH, "SEND_TOKEN").c_str());
    socket->flush();

    QObject::connect(socket, &QTcpSocket::readyRead, [=]()
                     {
        LinkedListNode *node = pendingUsers.find(socket);
        if (node == NULL) return;

        std::string msg = socket->readAll().toStdString();
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
            msg.pop_back();

        node->tokenBuffer += msg;

        std::string tag  = Protocol::getTag(node->tokenBuffer);
        std::string data = Protocol::getData(node->tokenBuffer);

        if (tag == TAG_AUTH && data == AUTH_TOKEN)
        {
            logs.info("Auth success from " + ip);
            std::string nodeIp = node->ip;
            pendingUsers.remove(socket);
            promoteToServer(socket, nodeIp);
            
        }
        else if (node->tokenBuffer.size() > (size_t)(MAX_TOKEN_SIZE + 5))
        {
            socket->write(Protocol::build(TAG_AUTH, "FAIL").c_str());
            socket->flush();
            logs.info("Auth failed from " + ip);
            pendingUsers.remove(socket);
            socket->disconnectFromHost();
        } });

    QObject::connect(socket, &QTcpSocket::disconnected, [=]()
                     {
        LinkedListNode *node = pendingUsers.find(socket);
        if (node != NULL)
        {
            logs.info("Unauthenticated user dropped " + ip);
            pendingUsers.remove(socket);
        } });
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
        handleNewConnection(socket); });

    return a.exec();
}
