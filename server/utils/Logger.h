#pragma once
#include <QDebug>
#include <fstream>
#include <string>
#include <iostream>
#include <ctime>

class Logger
{
    std::ofstream file;

    public:
        Logger(std::string filename = "server.log")
        {
            file.open(filename, std::ios::app);
        }

        ~Logger()
        {
            if (file.is_open())
            {
                file.close();
            }
        }

        void info(std::string msg)
        {
            logs("INFO", msg);
        }

        void warning(std::string msg)
        {
            logs("WARNING", msg);
        }

        void error(std::string msg)
        {
            logs("ERROR", msg);
        }

    private:
        std::string getTime()
        {
            time_t now = time(0);
            char buf[20];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
            return std::string(buf);
        }

        void logs(std::string level, std::string msg)
        {
            if (!file.is_open())
            {
                return;
            }

            std::string line = getTime() + " - " + level + " -> [" + msg + "]";
            file << line << "\n";
            file.flush();
            qDebug() << QString::fromStdString(line);
        }
};