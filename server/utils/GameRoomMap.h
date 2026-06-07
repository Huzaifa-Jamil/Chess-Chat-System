#pragma once
#include <string>
#include "Logger.h"
#include "GameRoom.h"
#include "FriendGraph.h"


static const int ROOM_MAP_SIZE = 100;

class GameRoomMap
{
private:

    struct Node
    {
        int       key;
        GameRoom *room;
        Node     *next;

        Node(int k, GameRoom *r)
        {
            key  = k;
            room = r;
            next = NULL;
        }
    };

    Node *table[ROOM_MAP_SIZE];

    int nextRoomId;

    FriendGraph *graph;

    Logger *logs;

    int hash(int key) const
    {
        return (key * 2654435761u) % ROOM_MAP_SIZE;
    }

public:

    GameRoomMap(FriendGraph *friendGraph, Logger *logger)
    {
        graph      = friendGraph;
        logs       = logger;
        nextRoomId = 1;

        for (int i = 0; i < ROOM_MAP_SIZE; i++)
        {
            table[i] = NULL;
        }
    }

    ~GameRoomMap()
    {
        for (int i = 0; i < ROOM_MAP_SIZE; i++)
        {
            Node *current = table[i];

            while (current != NULL)
            {
                Node *next = current->next;
                delete current->room;
                delete current;
                current = next;
            }
        }
    }


    int createRoom(QTcpSocket *p1, int userId1,
                   QTcpSocket *p2, int userId2)
    {
        int id = nextRoomId++;

        GameRoom *room = new GameRoom(id, p1, userId1, p2, userId2, logs);

        // Insert into the hash map
        int index = hash(id);
        Node *n   = new Node(id, room);
        n->next   = table[index];
        table[index] = n;

        logs->info("GameRoomMap: room " + std::to_string(id) +
                   " created for users " + std::to_string(userId1) +
                   " and " + std::to_string(userId2));


        graph->addUser(userId1);
        graph->addUser(userId2);

        // Start the room — sends START| to both players
        room->start();

        return id;
    }

    GameRoom *getRoom(int roomId) const
    {
        int index = hash(roomId);
        Node *temp = table[index];

        while (temp != NULL)
        {
            if (temp->key == roomId) return temp->room;
            temp = temp->next;
        }

        return NULL;
    }

    GameRoom *findRoomBySocket(QTcpSocket *socket) const
    {
        for (int i = 0; i < ROOM_MAP_SIZE; i++)
        {
            Node *temp = table[i];

            while (temp != NULL)
            {
                ChatSession *chat = temp->room->getChat();

                // if this socket is player1 or player2 in this room
                if (chat->getPlayer1() == socket ||
                    chat->getPlayer2() == socket)
                {
                    return temp->room;
                }

                temp = temp->next;
            }
        }

        return NULL;
    }

    void closeRoom(int roomId)
    {
        int index  = hash(roomId);
        Node *temp = table[index];
        Node *prev = NULL;

        while (temp != NULL)
        {
            if (temp->key == roomId)
            {
                ChatSession *chat = temp->room->getChat();
                graph->addEdge(chat->getId1(), chat->getId2());
                graph->displayConnections(chat->getId1());

                if (prev == NULL)
                {
                    table[index] = temp->next;
                }
                else
                {
                    prev->next = temp->next;
                }

                delete temp->room;
                delete temp;

                logs->info("GameRoomMap: room " + std::to_string(roomId) + " closed");
                return;
            }

            prev = temp;
            temp = temp->next;
        }
    }

    // print all active rooms to the log
    void display() const
    {
        std::string out = "GameRoomMap active rooms Hashmap ";
        bool any = false;

        for (int i = 0; i < ROOM_MAP_SIZE; i++)
        {
            Node *temp = table[i];

            while (temp != NULL)
            {
                out += "[room " + std::to_string(temp->key) + "] ";
                any  = true;
                temp = temp->next;
            }
        }

        if (!any) out += "NULL";
        logs->info(out);
    }
};
