#include <QCoreApplication>
#include <QTcpSocket>
#include <QTextStream>
#include <QThread>
#include <QDebug>
#include <QMetaObject>

QTcpSocket socket;

const QString AUTH_TOKEN = "CHESS123456PLEASEAUTH";
bool authenticated = false;

void inputLoop()
{
    QTextStream in(stdin);

    // wait only once for auth
    while (!authenticated)
    {
        QThread::msleep(100);
    }

    qDebug() << "You can start chatting now.";

    while (true)
    {
        QString msg = in.readLine();

        if (msg == "/exit")
        {
            QMetaObject::invokeMethod(&socket, []()
                                      { socket.disconnectFromHost(); }, Qt::QueuedConnection);
            break;
        }

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

    socket.connectToHost("104.198.200.12", 8080);
    // socket.connectToHost("127.0.0.1", 8080);

    QObject::connect(&socket, &QTcpSocket::connected, []()
                     { qDebug() << "[Server]: Connected"; });

    QObject::connect(&socket, &QTcpSocket::readyRead, [&]()
                     {
        while (socket.canReadLine())
        {
            QString msg = socket.readLine().trimmed();

            // one-time auth handshake
            if (msg.contains("Send auth token"))
            {
                qDebug() << "[Server]:" << msg;

                socket.write(AUTH_TOKEN.toUtf8() + "\n");
                socket.flush();

                qDebug() << "[You]: Auth token sent";
            }
            else if (msg == "AUTH OK")
            {
                authenticated = true;
                qDebug() << "[Server]: Authentication  Successful";
            }
            else if (msg == "AUTH FAILED")
            {
                qDebug() << "[Server]: Authentication failed";
                socket.disconnectFromHost();
            }
            else
            {
                qDebug().noquote() << "[Other]:" << msg;
            }
        } });

    QObject::connect(&socket, &QTcpSocket::disconnected, []()
                     { qDebug() << "[Server]: Disconnected"; });

    QThread *t = QThread::create(inputLoop);
    t->start();
    return a.exec();
}