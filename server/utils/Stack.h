#pragma once
#include "Logger.h"
#include <iostream>
#include <string>

template <typename T>
struct StackNode
{
    int userId;
    T data;
    StackNode *next;

    StackNode(int id, T d)
    {
        userId = id;
        data = d;
        next = NULL;
    }
};

template <typename T>
class Stack
{

private:
    StackNode<T> *topNode;
    int stackSize;
    Logger *logs;

public:
    Stack(Logger *log)
    {
        this->logs = log;
        topNode = NULL;
        stackSize = 0;
    }

    ~Stack()
    {
        clear();
    }

    bool isEmpty()
    {
        return topNode == NULL;
    }

    int getSize()
    {
        return stackSize;
    }

    void display()
    {
        if (isEmpty())
        {
            logs->warning("Stack is Empty");
            return;
        }

        std::string stack = "Stack (top to bottom): ";
        StackNode<T> *current = topNode;

        while (current != NULL)
        {
            stack += "[" + std::to_string(current->userId) + "] -> ";
            current = current->next;
        }

        stack += "END";
        logs->info(stack);
    }

    void clear()
    {
        while (!isEmpty())
        {
            pop();
        }
    }

    void push(int id, T value)
    {
        StackNode<T> *newNode = new StackNode<T>(id, value);
        newNode->next = topNode;
        topNode = newNode;

        stackSize++;
        logs->info("User [" + std::to_string(id) + "] pushed onto stack");
        display();
    }

    void pop()
    {
        if (isEmpty())
        {
            logs->warning("Stack is Empty");
            return;
        }

        StackNode<T> *temp = topNode;
        topNode = topNode->next;

        stackSize--;
        logs->info("User [" + std::to_string(temp->userId) + "] popped from stack");
        delete temp;
        display();
    }

    T peek()
    {
        if (isEmpty())
        {
            logs->warning("Stack is Empty");
            return T{};
        }
        return topNode->data;
    }
};