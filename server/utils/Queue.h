#pragma once
#include "Logger.h"
using namespace std;

template <typename T>
class Queue
{
private:
    struct Node
    {
        int userId;
        T socket;
        Node *next;

        Node(int Id, T value)
        {
            userId = Id;
            socket = value;
            next = NULL;
        }
    };

    Node *front;
    Node *rear;
    int size;
    Logger *logs;

public:
    Queue(Logger *Logger)
    {
        this->logs = Logger;
        front = rear = NULL;
        size = 0;
    }

    ~Queue()
    {
        clear();
    }

    bool isEmpty()
    {
        return front == NULL;
    }

    int getSize()
    {
        return size;
    }

    bool contains(int id)
    {
        if (isEmpty())
        {
            return false;
        }

        Node *current = front;

        do
        {
            if (current->userId == id)
            {
                return true;
            }

            current = current->next;
        } while (current != front);

        return false;
    }

    void dispaly()
    {
        if (isEmpty())
        {
            return;
        }

        string userQueue = "Users Queue: ";
        Node *current = front;

        do
        {
            userQueue += "[" + to_string(current->userId) + "], ";
            current = current->next;
        } while (current != front);

        userQueue += "END";
        logs->info(userQueue);
    }

    void clear()
    {
        while (!(isEmpty()))
        {
            dequeue();
        }
    }

    int getFrontId()
    {
        if (isEmpty())
        {
            return -1;
        }

        return front->userId;
    }

    void enqueue(int id, T value)
    {
        if (contains(id))
        {
            logs->info("Queue: user [" + to_string(id) + "] already in queue, skipping them");
            return;
        }

        Node *newNode = new Node(id, value);

        if (isEmpty())
        {
            front = rear = newNode;
            rear->next = front;
        }
        else
        {
            rear->next = newNode;
            rear = newNode;
            rear->next = front;
        }

        size++;
        dispaly();
    }

    void dequeue()
    {
        if (isEmpty())
        {
            return;
        }

        Node *temp = front;

        if (front == rear)
        {
            front = rear = NULL;
        }
        else
        {
            front = front->next;
            rear->next = front;
        }

        size--;

        delete temp;
        dispaly();
    }
};