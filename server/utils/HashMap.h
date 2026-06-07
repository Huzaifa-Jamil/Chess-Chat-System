#pragma once
#include <QTcpSocket>
#include <string>
#include "Logger.h"

class HashMap
{
private:
    static const int SIZE = 100;

    struct Node
    {
        int key; // by userId
        QTcpSocket *socket;
        Node *next;

        Node(int k, QTcpSocket *s)
        {
            key = k;
            socket = s;
            next = nullptr;
        }
    };

    Node *table[SIZE];
    Logger *logs;

    int hash(int key)
    {
        return (key * 2654435761) % SIZE;
    }

public:
    HashMap(Logger *logger)
    {
        logs = logger;

        for (int i = 0; i < SIZE; i++)
        {
            table[i] = nullptr;
        }
    }

    void insert(int key, QTcpSocket *socket)
    {
        int index = hash(key);

        Node *n = new Node(key, socket);

        // insert at head
        n->next = table[index];
        table[index] = n;

        logs->info("HashMap, inserted user " + std::to_string(key));
    }

    QTcpSocket *get(int key)
    {
        int index = hash(key);

        Node *temp = table[index];

        while (temp != nullptr)
        {
            if (temp->key == key)
            {
                return temp->socket;
            }
            temp = temp->next;
        }

        return nullptr;
    }

    bool exists(int key)
    {
        return get(key) != nullptr;
    }

    void remove(int key)
    {
        int index = hash(key);

        Node *temp = table[index];
        Node *prev = nullptr;

        while (temp != nullptr)
        {
            if (temp->key == key)
            {
                if (prev == nullptr)
                {
                    table[index] = temp->next;
                }
                else
                {
                    prev->next = temp->next;
                }

                delete temp;
                logs->info("HashMap, removed user " + std::to_string(key));
                return;
            }

            prev = temp;
            temp = temp->next;
        }
    }

    void debug()
    {
        std::string out = "HashMap: ";

        for (int i = 0; i < SIZE; i++)
        {
            Node *temp = table[i];

            if (temp)
            {
                out += "[" + std::to_string(i) + ": ";

                while (temp)
                {
                    out += std::to_string(temp->key) + " -> ";
                    temp = temp->next;
                }

                out += "NULL] ";
            }
        }

        logs->info(out);
    }
};