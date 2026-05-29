#include <QCoreApplication>
#include <QTcpSocket>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QTcpSocket socket;

    socket.connectToHost("127.0.0.1", 8080);
    qDebug() << "Client Starts";

    if (socket.waitForConnected(10000))
    {
        qDebug() << "Connected to server !! ";

        socket.write("Hello Server !!");
        socket.waitForReadyRead();

        qDebug() << "server Says : " << socket.readAll();
    }
    else
    {
        qDebug() << "Timeout for connection !! bad day ";
    }

    return 0;
}
