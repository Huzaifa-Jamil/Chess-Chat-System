#pragma once
#include <string>
#include "Logger.h"

static const int MAX_GRAPH_USERS = 100;

class FriendGraph
{
private:

    // The adjacency matrix
    
    int matrix[MAX_GRAPH_USERS][MAX_GRAPH_USERS];

    // How many users are currently registered in the graph
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
        logs      = logger;
        userCount = 0;

        for (int i = 0; i < MAX_GRAPH_USERS; i++)
        {
            for (int j = 0; j < MAX_GRAPH_USERS; j++)
            {
                matrix[i][j] = 0;
            }
        }
    }

    void addUser(int userId)
    {
        if (!validId(userId)) return;

        userCount++;
        logs->info("FriendGraph: user " + std::to_string(userId) + " added as node");
    }

    void addEdge(int userId1, int userId2)
    {
        if (!validId(userId1) || !validId(userId2)) return;
        if (userId1 == userId2) return;

        int i = toIndex(userId1);
        int j = toIndex(userId2);

        matrix[i][j]++;
        matrix[j][i]++;

        logs->info("FriendGraph: edge updated user " + std::to_string(userId1) +
                   " <-> user " + std::to_string(userId2) +
                   " | games played: " + std::to_string(matrix[i][j]));
    }

    int getGameCount(int userId1, int userId2) const
    {
        if (!validId(userId1) || !validId(userId2)) return 0;

        return matrix[toIndex(userId1)][toIndex(userId2)];
    }

    bool havePlayedTogether(int userId1, int userId2) const
    {
        return getGameCount(userId1, userId2) > 0;
    }

    void displayConnections(int userId) const
    {
        if (!validId(userId)) return;

        int i = toIndex(userId);
        std::string out = "FriendGraph connections for user " +
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

        if (!any) out += "none";

        logs->info(out);
    }

    void displayAll() const
    {
        logs->info("FriendGraph connections matrix of users ");

        for (int i = 0; i < MAX_GRAPH_USERS; i++)
        {
            bool hasAny = false;

            for (int j = 0; j < MAX_GRAPH_USERS; j++)
            {
                if (matrix[i][j] > 0) { hasAny = true; break; }
            }

            if (!hasAny) continue; // skip users with no connections

            std::string row = "  user " + std::to_string(i + 1) + ": ";

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
};
