#pragma once
#include <QDebug>
#include <fstream>
#include <string>
#include <iostream>
#include <ctime>
using namespace std;

class Logger
{
    ofstream file;

public:
    Logger(string filename = "server.log")
    {
        file.open(filename, ios::app);
    }

    void info(string msg)
    {
        logs("INFO", msg);
    }

    void warning(string msg)
    {
        logs("WARNING", msg);
    }

    void error(string msg)
    {
        logs("ERROR", msg);
    }

private:

    string getTime()
    {
        time_t now = time(0);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return string(buf);
    }

    void logs(string level, string msg)
    {
        if (!file.is_open())
        {
            return;
        }

        string line = getTime() + " - " + level + " -> [" + msg + "]";
        file << line << "\n";
        file.flush();

        qDebug() << QString::fromStdString(line);
    }
};