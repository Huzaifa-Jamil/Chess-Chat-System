#pragma once
#include <QTcpSocket>
#include <string>
#include "Logger.h"

class HashMap
{
    private:
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
        
        static const int SIZE = 100;

        Node *table[SIZE];
        Logger *logs;

        int hash(int key) const
        {
            return (int)(((unsigned int)key * 2654435761u) % SIZE);
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

        ~HashMap()
        {
            for (int i = 0; i < SIZE; i++)
            {
                Node *current = table[i];

                while (current != nullptr)
                {
                    Node *next = current->next;
                    delete current;
                    current = next;
                }

                table[i] = nullptr;
            }
        }

        void insert(int key, QTcpSocket *socket)
        {
            int index = hash(key);

            Node *existing = table[index];

            while (existing != nullptr)
            {
                if (existing->key == key)
                {
                    existing->socket = socket;
                    logs->info("Active users (Hash map):- Updated user " + std::to_string(key));
                    debug();
                    return;
                }

                existing = existing->next;
            }

            Node *n = new Node(key, socket);

            // insert at head
            n->next = table[index];
            table[index] = n;

            logs->info("Active users (Hash map):- Inserted user " + std::to_string(key));
            debug();
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
                    logs->info("Active users (Hash map):- Removed user " + std::to_string(key));
                    debug();
                    return;
                }

                prev = temp;
                temp = temp->next;
            }
        }

        void debug()
        {
            std::string out = "Active users (Hash map):- ";

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