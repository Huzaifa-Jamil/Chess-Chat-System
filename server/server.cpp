#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTcpServer server;
    server.listen(QHostAddress::Any, 8080);
    qDebug() << "Server Waiting for clients";

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket *client = server.nextPendingConnection();

        QObject::connect(client, &QTcpSocket::readyRead, [=]() {
            QByteArray data = client->readAll();
            qDebug() << "Client Says" << data;

            client->write ("Hello Client !!");
        });
    });

    return a.exec();
}
