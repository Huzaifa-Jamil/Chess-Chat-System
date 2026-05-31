#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include "Logger.h"
#include "AVLTree.h"
#include "Queue.h"
#include "UserIdManager.h"

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
    player2->socket->write("Your Match Found\n");

    logs.info("Chat started between user " + to_string(idPlayer1) + " and user " + to_string(idPlayer2));

    QObject::connect(player1->socket, &QTcpSocket::readyRead, [=]()
                     {
        QByteArray data = player1->socket->readAll();
        logs.info("User " + to_string(idPlayer1) + " says: " + string(data.constData()));
        player2->socket->write(data + "\n"); });

    QObject::connect(player2->socket, &QTcpSocket::readyRead, [=]()
                     {
        QByteArray data = player2->socket->readAll();
        logs.info("User " + to_string(idPlayer2) + " says: " + string(data.constData()));
        player1->socket->write(data + "\n"); });
}

void handleNewConnection(QTcpSocket *socket)
{
    string ip = socket->peerAddress().toString().toStdString();

    logs.info("New User connected from " + ip);

    Node *user = users.findByIp(ip);

    if (user != NULL)
    {
        // if user is stored in to the avl just reconnect them
        users.reconnectUser(ip, socket);
        socket->write("You Have been successfully Reconnected to the server");

        logs.info("User " + to_string(user->userId) + " reconnected");
        socket->write("Welcome Back User !!\n");
        waitingQueue.enqueue(user->userId, socket);
    }
    else
    {
        // new user congratulations add them to avl and queue
        int id = idManager.assignNextUserId();

        users.insertUser(id, socket, ip);
        socket->write("Welcome to Chess Chat System hope you enjoy it \n");

        logs.info("New user " + to_string(id) + " added");

        waitingQueue.enqueue(id, socket);
    }

    handleQueue();

    QObject::connect(socket, &QTcpSocket::disconnected, [=]()
                     {
                         string disconnectedIp = socket->peerAddress().toString().toStdString();
                         Node *node = users.findByIp(disconnectedIp);

                         if (node != NULL)
                         {
                             node->isConnected = false;
                             node->isPlaying = false;
                             logs.info("User " + to_string(node->userId) + " disconnected \n");
                         }
                     });
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