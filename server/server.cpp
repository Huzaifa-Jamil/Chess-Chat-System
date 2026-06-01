#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include "Logger.h"
#include "AVLTree.h"
#include "Queue.h"
#include "UserIdManager.h"
#include <string>

// variables declaration
Logger logs;
UserIdManager idManager;
AVLTree users(&logs);
Queue<QTcpSocket *> waitingQueue(&logs);

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
        logs.info("Match failed: one of the users not found");
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

void handleNewConnection(QTcpSocket *socket)
{
    std::string ip = socket->peerAddress().toString().toStdString();

    logs.info("New User connected from " + ip);

    Node *user = users.findByIp(ip);

    if (user != NULL)
    {
        users.reconnectUser(ip, socket);
        socket->write("You Have been successfully Reconnected to the server \n");

        logs.info("User " + std::to_string(user->userId) + " reconnected\n");
        socket->write("Welcome Back User !!\n");
        socket->flush();
        waitingQueue.enqueue(user->userId, socket);
    }
    else
    {
        int id = idManager.assignNextUserId();
        users.insertUser(id, socket, ip);
        socket->write("Welcome to Chess Chat System hope you enjoy it \n");
        socket->flush();
        logs.info("New user " + std::to_string(id) + " added");
        waitingQueue.enqueue(id, socket);
    }

    handleQueue();

    QObject::connect(socket, &QTcpSocket::disconnected, [=]()
                     {
        std::string disconnectedIp = socket->peerAddress().toString().toStdString();
        Node *node = users.findByIp(disconnectedIp);

        if (node != NULL)
        {
            node->isConnected = false;
            node->isPlaying   = false;
            logs.info("User " + std::to_string(node->userId) + " disconnected \n");
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