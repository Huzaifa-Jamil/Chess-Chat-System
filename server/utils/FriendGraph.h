#pragma once
#include <string>
#include "Logger.h"

static const int MAX_GRAPH_USERS = 1000;

class FriendGraph
{
    private:
        // The adjacency matrix
        int matrix[MAX_GRAPH_USERS][MAX_GRAPH_USERS];

        bool activeNodes[MAX_GRAPH_USERS];
        int userCount;
        Logger *logs;

        // Convert userId to matrix index
        int toIndex(int userId) const
        {
            return userId - 1;
        }

        // Check if a userId is within valid range
        bool validId(int userId) const
        {
            return userId >= 1 && userId <= MAX_GRAPH_USERS;
        }

    public:
        FriendGraph(Logger *logger)
        {
            logs = logger;
            userCount = 0;

            for (int i = 0; i < MAX_GRAPH_USERS; i++)
            {
                activeNodes[i] = false;
                
                for (int j = 0; j < MAX_GRAPH_USERS; j++)
                {
                    matrix[i][j] = 0;
                }
            }
        }

        void addUser(int userId)
        {
            if (!validId(userId))
            {
                return;
            }

            if (!activeNodes[toIndex(userId)])
            {
                activeNodes[toIndex(userId)] = true;
                userCount++;

                logs->info("Friends Graph(Graph):- user " + std::to_string(userId) + " added as node");
            }
        }

        void addEdge(int userId1, int userId2)
        {
            if (!validId(userId1) || !validId(userId2))
            {
                return;
            }

            if (userId1 == userId2)
            {
                return;
            }

            int i = toIndex(userId1);
            int j = toIndex(userId2);

            matrix[i][j]++;
            matrix[j][i]++;

            logs->info("Friends Graph(Graph):- Edge updated user " + std::to_string(userId1) +
                    " <-> user " + std::to_string(userId2) +
                    " | games played: " + std::to_string(matrix[i][j]));
        }

        int getGameCount(int userId1, int userId2) const
        {
            if (!validId(userId1) || !validId(userId2))
            {
                return 0;
            }

            return matrix[toIndex(userId1)][toIndex(userId2)];
        }

        bool havePlayedTogether(int userId1, int userId2) const
        {
            return getGameCount(userId1, userId2) > 0;
        }

        void displayConnections(int userId) const
        {
            if (!validId(userId))
            {
                return;
            }

            int i = toIndex(userId);

            std::string out = "Friends Graph(Graph):- connections for user " +
                            std::to_string(userId) + ": ";

            bool any = false;

            for (int j = 0; j < MAX_GRAPH_USERS; j++)
            {
                if (matrix[i][j] > 0)
                {
                    out += "user " + std::to_string(j + 1) +
                        "(" + std::to_string(matrix[i][j]) + " games) ";
                    any = true;
                }
            }

            if (!any)
            {
                out += "NULL";
            }

            logs->info(out);
        }

        void displayAll() const
        {
            logs->info("Friends Graph(Graph):- connection matrix of users ");

            for (int i = 0; i < MAX_GRAPH_USERS; i++)
            {
                bool hasAny = false;

                for (int j = 0; j < MAX_GRAPH_USERS; j++)
                {
                    if (matrix[i][j] > 0)
                    {
                        hasAny = true;
                        break;
                    }
                }

                if (!hasAny)
                {
                    continue;
                }

                std::string row = "Friends Graph(Graph):-   user " + std::to_string(i + 1) + ": ";

                for (int j = 0; j < MAX_GRAPH_USERS; j++)
                {
                    if (matrix[i][j] > 0)
                    {
                        row += "user" + std::to_string(j + 1) +
                            "=" + std::to_string(matrix[i][j]) + " ";
                    }
                }

                logs->info(row);
            }
        }

        void logMST() const
        {
            std::string prefix = "friends graph (Prims Alog, MST):- ";
            logs->info(prefix);

            bool visited[MAX_GRAPH_USERS];
            int parent[MAX_GRAPH_USERS];
            int key[MAX_GRAPH_USERS];
            bool hasActive = false;

            for (int i = 0; i < MAX_GRAPH_USERS; i++)
            {
                visited[i] = false;
                parent[i] = -1;
                key[i] = 999999999;
                
                if (activeNodes[i])
                {
                    hasActive = true;
                }
            }

            if (!hasActive)
            {
                logs->info(prefix + "NULL");
                return;
            }

            for (int start = 0; start < MAX_GRAPH_USERS; start++)
            {
                if (!activeNodes[start] || visited[start])
                {
                    continue;
                }

                key[start] = 0;

                for (int count = 0; count < MAX_GRAPH_USERS; count++)
                {
                    int u = -1;
                    int minKey = 999999999;

                    for (int i = 0; i < MAX_GRAPH_USERS; i++)
                    {
                        if (activeNodes[i] && !visited[i] && key[i] < minKey)
                        {
                            minKey = key[i];
                            u = i;
                        }
                    }

                    if (u == -1)
                    {
                        break;
                    }

                    visited[u] = true;

                    if (parent[u] != -1)
                    {
                        logs->info(prefix + "MST Edge: User " + std::to_string(parent[u] + 1) +
                                " <-> User " + std::to_string(u + 1) +
                                " (Games: " + std::to_string(matrix[parent[u]][u]) + ")");
                    }

                    for (int v = 0; v < MAX_GRAPH_USERS; v++)
                    {
                        if (activeNodes[v] && matrix[u][v] > 0 && !visited[v])
                        {
                            if (matrix[u][v] < key[v])
                            {
                                parent[v] = u;
                                key[v] = matrix[u][v];
                            }
                        }
                    }
                }
            }

            logs->info(prefix);
        }
};