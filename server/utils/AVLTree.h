#pragma once
#include <string>
#include <QTcpSocket>
#include "Logger.h"
using namespace std;

struct Node
{
    int userId;
    QTcpSocket *socket;
    bool isConnected;
    string ipAddress;
    bool isPlaying;
    Node *left;
    Node *right;
    int height;

    Node(int id, QTcpSocket *sock, string ip)
    {
        userId = id;
        socket = sock;
        isConnected = true;
        ipAddress = ip;
        isPlaying = false;
        left = NULL;
        right = NULL;
        height = 1;
    }
};

class AVLTree
{
    private:
        Node *root;
        Logger *logs;

        void inorder(Node *root, string &result)
        {
            if (root == NULL)
            {
                return;
            }

            inorder(root->left, result);

            result += "[" + to_string(root->userId) + "] ";
            result += root->ipAddress + " ";
            result += (root->isConnected ? "connected " : "offline ");
            result += (root->isPlaying ? "playing, " : "idle, ");

            inorder(root->right, result);
        }

        int getHeight(Node *node)
        {
            if (node == NULL)
            {
                return 0;
            }

            return node->height;
        }

        int maxNode(int a, int b)
        {
            if (a > b)
            {
                return a;
            }
            else
            {
                return b;
            }
        }

        int getBF(Node *node)
        {
            if (node == NULL)
            {
                return 0;
            }

            return getHeight(node->left) - getHeight(node->right);
        }

        Node *rightRotate(Node *y)
        {
            Node *x = y->left;
            Node *T2 = x->right;

            x->right = y;
            y->left = T2;

            y->height = maxNode(getHeight(y->left), getHeight(y->right)) + 1;
            x->height = maxNode(getHeight(x->left), getHeight(x->right)) + 1;

            logs->info("Users AVL:- Right Rotation happens in AVL Tree");
            return x;
        }

        Node *leftRotate(Node *x)
        {
            Node *y = x->right;
            Node *T2 = y->left;

            y->left = x;
            x->right = T2;

            x->height = maxNode(getHeight(x->left), getHeight(x->right)) + 1;
            y->height = maxNode(getHeight(y->left), getHeight(y->right)) + 1;

            logs->info("Users AVL:- Left Rotation happens in AVL Tree");
            return y;
        }

        Node *minNode(Node *root)
        {
            while (root->left != NULL)
            {
                root = root->left;
            }

            return root;
        }

        Node *insertUser(Node *root, int id, QTcpSocket *sock, string ip)
        {
            if (root == NULL)
            {
                return new Node(id, sock, ip);
            }

            if (id < root->userId)
            {
                root->left = insertUser(root->left, id, sock, ip);
            }
            else if (id > root->userId)
            {
                root->right = insertUser(root->right, id, sock, ip);
            }
            else
            {
                return root;
            }

            root->height = 1 + maxNode(getHeight(root->left), getHeight(root->right));

            int balance = getBF(root);

            // LL
            if (balance > 1 && id < root->left->userId)
            {
                return rightRotate(root);
            }

            // RR
            if (balance < -1 && id > root->right->userId)
            {
                return leftRotate(root);
            }

            // LR
            if (balance > 1 && id > root->left->userId)
            {
                root->left = leftRotate(root->left);
                return rightRotate(root);
            }

            // RL
            if (balance < -1 && id < root->right->userId)
            {
                root->right = rightRotate(root->right);
                return leftRotate(root);
            }

            return root;
        }

        Node *deleteUser(Node *root, int id)
        {
            if (root == NULL)
            {
                return root;
            }

            if (id < root->userId)
            {
                root->left = deleteUser(root->left, id);
            }
            else if (id > root->userId)
            {
                root->right = deleteUser(root->right, id);
            }
            else
            {
                if (root->left == NULL && root->right == NULL)
                {
                    delete root;
                    return NULL;
                }
                else if (root->left == NULL)
                {
                    Node *temp = root->right;
                    delete root;
                    return temp;
                }
                else if (root->right == NULL)
                {
                    Node *temp = root->left;
                    delete root;
                    return temp;
                }

                Node *temp = minNode(root->right);
                root->userId = temp->userId;
                root->socket = temp->socket;
                root->isConnected = temp->isConnected;
                root->ipAddress = temp->ipAddress;
                root->isPlaying = temp->isPlaying;
                root->right = deleteUser(root->right, temp->userId);
            }

            if (root == NULL)
            {
                return root;
            }

            root->height = maxNode(getHeight(root->left), getHeight(root->right)) + 1;

            int balance = getBF(root);

            // LL
            if (balance > 1 && getBF(root->left) >= 0)
            {
                return rightRotate(root);
            }

            // LR
            if (balance > 1 && getBF(root->left) < 0)
            {
                root->left = leftRotate(root->left);
                return rightRotate(root);
            }

            // RR
            if (balance < -1 && getBF(root->right) <= 0)
            {
                return leftRotate(root);
            }

            // RL
            if (balance < -1 && getBF(root->right) > 0)
            {
                root->right = rightRotate(root->right);
                return leftRotate(root);
            }

            return root;
        }

        Node *findById(Node *root, int id)
        {
            if (root == NULL)
            {
                return NULL;
            }

            if (id == root->userId)
            {
                return root;
            }
            else if (id < root->userId)
            {
                return findById(root->left, id);
            }
            else
            {
                return findById(root->right, id);
            }
        }

        Node *findByIp(Node *root, string ip)
        {
            if (root == NULL)
            {
                return NULL;
            }

            Node *left = findByIp(root->left, ip);

            if (left != NULL)
            {
                return left;
            }

            if (root->ipAddress == ip)
            {
                return root;
            }

            return findByIp(root->right, ip);
        }

        void destroyTree(Node *node)
        {
            if (node == NULL)
            {
                return;
            }
            destroyTree(node->left);
            destroyTree(node->right);
            delete node;
        }

    public:
        ~AVLTree()
        {
            destroyTree(root);
        }

        void display()
        {
            if (root == NULL)
            {
                logs->info("No Users Found");
                return;
            }

            string usersDisplay = "Users (AVL):- ";
            inorder(root, usersDisplay);
            usersDisplay += "END";
            logs->info(usersDisplay);
        }

        AVLTree(Logger *logger)
        {
            root = NULL;
            logs = logger;
        }

        void insertUser(int id, QTcpSocket *sock, string ip)
        {
            root = insertUser(root, id, sock, ip);
            display();
        }

        void deleteUser(int id)
        {
            root = deleteUser(root, id);
            display();
        }

        Node *findById(int id)
        {
            return findById(root, id);
        }

        Node *findByIp(string ip)
        {
            return findByIp(root, ip);
        }

        bool reconnectUser(string ip, QTcpSocket *newSocket)
        {
            Node *node = findByIp(root, ip);

            if (node == NULL)
            {
                return false;
            }

            node->socket = newSocket;
            node->isConnected = true;
            node->isPlaying = false;

            logs->info("Server:- User " + to_string(node->userId) + " reconnected in to server");
            display();
            
            return true;
        }
};