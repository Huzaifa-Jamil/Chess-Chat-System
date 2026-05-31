#include "Logger.h"
using namespace std;

template <typename T>
class Stack
{

    private:
        struct Node
        {
            int userId;
            T data;
            Node *next;

            Node(int Id, T value)
            {
                userId = Id;
                data = value;
                next = NULL;
            }
        };

        Node *top;
        int size;
        Logger *logs;

    public:
        Stack(Logger *log)
        {
            this->logs = log;
            top = NULL;
            size = 0;
        }

        ~Stack()
        {
            clear();
        }

        bool isEmpty()
        {
            return top == NULL;
        }

        int getSize()
        {
            return size;
        }

        void display()
        {
            if (isEmpty())
            {
                logs->warning("Stack is Empty");
                return;
            }

            string stack = "Stack (top to bottom): ";
            Node *current = top;

            while (current != NULL)
            {
                stack += "[" + to_string(current->userId) + "] -> ";
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
            Node *newNode = new Node(id, value);
            newNode->next = top;
            top = newNode;

            size++;
            logs->info("User [" + to_string(id) + "] pushed onto stack");
            display();
        }

        void pop()
        {
            if (isEmpty())
            {
                logs->warning("Stack is Empty");
                return;
            }

            Node *temp = top;
            top = top->next;

            size--;
            logs->info("User [" + to_string(temp->userId) + "] popped from stack");
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
            return top->data;
        }
};