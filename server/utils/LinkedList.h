#pragma once
#include <string>
#include <QTcpSocket>
#include <QDebug>

struct LinkedListNode
{
    QTcpSocket *socket;
    std::string ip;
    bool authenticated;
    std::string tokenBuffer;
    LinkedListNode *prev;
    LinkedListNode *next;

    LinkedListNode(QTcpSocket *s, std::string i)
    {
        socket = s;
        ip = i;
        authenticated = false;
        tokenBuffer = "";
        prev = NULL;
        next = NULL;
    }
};

class LinkedList
{
private:
    Logger *logs;
    LinkedListNode *head;
    LinkedListNode *tail;

    void display()
    {
        LinkedListNode *temp = head;
        std::string result = "Unauth Pending list (LinkedList):- ";

        while (temp != NULL)
        {
            result += temp->ip + " ";
            result += (temp->authenticated ? "AUTH, " : "NOAUTH, ");
            temp = temp->next;
        }

        result += "END";
        logs->info(result);
    }

public:
    LinkedList(Logger *logger)
    {
        head = NULL;
        tail = NULL;
        logs = logger;
    }

    ~LinkedList()
    {
        LinkedListNode *current = head;
        while (current != NULL)
        {
            LinkedListNode *next = current->next;
            delete current;
            current = next;
        }
    }

    void insert(QTcpSocket *socket, std::string ip)
    {
        LinkedListNode *n = new LinkedListNode(socket, ip);

        if (head == NULL)
        {
            head = tail = n;
            display();
            return;
        }

        n->next = head;
        head->prev = n;
        head = n;
        display();
    }

    void remove(QTcpSocket *socket)
    {
        LinkedListNode *temp = find(socket);

        if (temp == NULL)
        {
            return;
        }

        if (temp->prev != NULL)
        {
            temp->prev->next = temp->next;
        }
        else
        {
            head = temp->next;
            if (head != NULL)
            {
                head->prev = NULL;
            }
        }

        if (temp->next != NULL)
        {
            temp->next->prev = temp->prev;
        }
        else
        {
            tail = temp->prev;
            if (tail != NULL)
            {
                tail->next = NULL;
            }
        }

        delete temp;
        display();
    }

    LinkedListNode *find(QTcpSocket *socket)
    {
        LinkedListNode *temp = head;

        while (temp != NULL)
        {
            if (temp->socket == socket)
            {
                return temp;
            }

            temp = temp->next;
        }

        return NULL;
    }

    LinkedListNode *findByIp(std::string ip)
    {
        LinkedListNode *temp = head;

        while (temp != NULL)
        {
            if (temp->ip == ip)
            {
                return temp;
            }

            temp = temp->next;
        }

        return NULL;
    }

    void markAuthenticated(QTcpSocket *socket)
    {
        LinkedListNode *n = find(socket);

        if (n != NULL)
        {
            n->authenticated = true;
            display();
        }
    }
};