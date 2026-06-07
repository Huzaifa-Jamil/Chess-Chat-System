#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include "Logger.h"
#include "AVLTree.h"
#include "Queue.h"
#include "UserIdManager.h"
#include "LinkedList.h"
#include "HashMap.h"
#include <string>

// variables declaration
Logger logs;
UserIdManager idManager;
AVLTree users(&logs);
Queue<QTcpSocket *> waitingQueue(&logs);
LinkedList pendingUsers(&logs);
HashMap activeUsers(&logs);

// auth tocken info
static const std::string AUTH_TOKEN = "CHESS123456PLEASEAUTH";
static const int MAX_TOKEN_SIZE = 21;

// method declaration
void handleQueue();
void promoteToServer(QTcpSocket *socket, std::string ip);
void handleNewConnection(QTcpSocket *socket);

// some helper methods

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
        logs.info("Match failed, one of the users not found");
        return;
    }

    player1->isPlaying = true;
    player2->isPlaying = true;

    player1->socket->write("Your Match Found\n");
    player1->socket->flush();
    player2->socket->write("Your Match Found\n");
    player2->socket->flush();

    std::string msg = "Chat started between user " + std::to_string(idPlayer1) + " and user " + std::to_string(idPlayer2) + "\n";
    player1->socket->write(msg.c_str());
    player1->socket->flush();
    player2->socket->write(msg.c_str());
    player2->socket->flush();

    logs.info("Chat started between user " + std::to_string(idPlayer1) + " and user " + std::to_string(idPlayer2));

    QObject::connect(player1->socket, &QTcpSocket::readyRead, [=]()
                     {
        QByteArray data = player1->socket->readAll();
        logs.info("User " + std::to_string(idPlayer1) + " says: " + std::string(data.constData()));
        player2->socket->write(data + "\n");
        player2->socket->flush(); });

    QObject::connect(player2->socket, &QTcpSocket::readyRead, [=]()
                     {
        QByteArray data = player2->socket->readAll();
        logs.info("User " + std::to_string(idPlayer2) + " says: " + std::string(data.constData()));
        player1->socket->write(data + "\n");
        player1->socket->flush(); });
}

void promoteToServer(QTcpSocket *socket, std::string ip)
{
    Node *existing = users.findByIp(ip);

    if (existing != NULL)
    {
        // Check if user already has an active session in HashMap
        if (activeUsers.exists(existing->userId))
        {
            logs.error("Duplicate login attempt for active user " + std::to_string(existing->userId));

            // Reject the new connection and continue old one
            socket->write("Error: This user is already logged in elsewhere (from same ip)\n");
            socket->flush();
            socket->disconnectFromHost();

            return;
        }

        // Standard Reconnect Logic
        users.reconnectUser(ip, socket);
        activeUsers.insert(existing->userId, socket);

        socket->write("Welcome Back! You have been reconnected\n");
        socket->flush();
        logs.info("User " + std::to_string(existing->userId) + " reconnected from " + ip);
        waitingQueue.enqueue(existing->userId, socket);
    }
    else
    {
        // New User Registration
        int id = idManager.assignNextUserId();
        users.insertUser(id, socket, ip);
        activeUsers.insert(id, socket);

        socket->write("Welcome to Chess Chat System! Its a good Day lets Chat and play!\n");
        socket->flush();
        logs.info("New user " + std::to_string(id) + " connected from " + ip);
        waitingQueue.enqueue(id, socket);
    }

    // Handle Disconnects
    QObject::connect(socket, &QTcpSocket::disconnected, [=]()
                     {
        Node *node = users.findByIp(ip);
        if (node != NULL)
        {
            node->isConnected = false;
            node->isPlaying   = false;
            activeUsers.remove(node->userId);
            logs.info("User " + std::to_string(node->userId) + " disconnected");
        } });

    handleQueue();
}

void handleNewConnection(QTcpSocket *socket)
{
    std::string ip = socket->peerAddress().toString().toStdString();
    logs.info("New connection from " + ip + ", waiting for auth");

    pendingUsers.insert(socket, ip);
    socket->write("Send auth token to continue \n");
    socket->flush();

    QObject::connect(socket, &QTcpSocket::readyRead, [=]()
                     {
        LinkedListNode *node = pendingUsers.find(socket);
        if (node == NULL)
        {
            return;
        }

        std::string msg = socket->readAll().toStdString();
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
        {
            msg.pop_back();
        }

        node->tokenBuffer += msg;

        if (node->tokenBuffer == AUTH_TOKEN)
        {
            socket->write("AUTH OK\n");
            socket->flush();
            logs.info("Auth success from " + ip);

            std::string nodeIp = node->ip;
            pendingUsers.remove(socket);
            promoteToServer(socket, nodeIp);
        }

        // token too long will be rejected like GET / and other stuff
        else if (node->tokenBuffer.size() > MAX_TOKEN_SIZE)
        {
            socket->write("AUTH FAILED\n");
            socket->flush();
            logs.info("Auth failed from " + ip);
            pendingUsers.remove(socket);
            socket->disconnectFromHost();
        } });

    // disconnect before auth
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