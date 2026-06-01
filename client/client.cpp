#include <QCoreApplication>
#include <QTcpSocket>
#include <QTextStream>
#include <QThread>
#include <QDebug>
#include <QMetaObject>

QTcpSocket socket;

void inputLoop()
{
    QTextStream in(stdin);

    while (true)
    {
        QString msg = in.readLine();

        if (msg == "/exit")
            break;

        qDebug().noquote() << "[You]:" << msg;

        QMetaObject::invokeMethod(&socket, [msg]()
        {
            socket.write(msg.toUtf8() + "\n");
            socket.flush(); }, Qt::QueuedConnection);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    socket.connectToHost("127.0.0.1", 8080);

    QObject::connect(&socket, &QTcpSocket::connected, []()
    { 
        qDebug() << "Connected to server";
    });

    QObject::connect(&socket, &QTcpSocket::readyRead, [&]()
    {
        while (socket.canReadLine())
        {
            QString msg = socket.readLine().trimmed();
            qDebug().noquote() << "[Other]:" << msg;
        }
    });

    QObject::connect(&socket, &QTcpSocket::disconnected, []()
    {
        qDebug() << "[Server]: Disconnected";
    });

    QThread *t = QThread::create(inputLoop);
    t->start();

    return a.exec();
}