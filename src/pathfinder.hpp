#ifndef PATHFINDER_HPP
#define PATHFINDER_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <cmath>
#include "graph.hpp"

template <typename Key, typename Value>
using umap = std::unordered_map<Key, Value>;
using string = std::string;

class Pathfinder
{
private:
    string m_StartNode, m_EndNode;
    umap<string, double> m_Dist;
    umap<string, string> m_Prev;
    umap<string, Node> m_Graph;
    int m_NodesExplored = 0;
    double m_TimeTaken = 0.0;
    string m_Algorithm;

public:
    Pathfinder(const umap<string, Node> &graph, const string &startNode, const string &endNode, const string& algo = "dijkstra")
        : m_StartNode(startNode), m_EndNode(endNode), m_Graph(graph), m_Algorithm(algo)
    {
        if (m_Graph.find(m_StartNode) == m_Graph.end())
            throw std::runtime_error("Start node not found in graph\n");

        for (const auto &[id, _] : m_Graph)
            m_Dist[id] = std::numeric_limits<double>::infinity();

        auto start = std::chrono::high_resolution_clock::now();

        if (algo == "astar")
            astar();
        else
            dijkstra();

        auto end = std::chrono::high_resolution_clock::now();
        m_TimeTaken = std::chrono::duration<double, std::milli>(end - start).count();
    }

    inline const umap<string, double> GetDistances() const { return m_Dist; }
    inline int GetNodesExplored() const { return m_NodesExplored; }
    inline double GetTimeTaken() const { return m_TimeTaken; }
    inline string GetAlgorithm() const { return m_Algorithm; }

    inline std::vector<string> GetPath() const
    {
        std::vector<string> path;
        for (string at = m_EndNode; !at.empty(); at = m_Prev.count(at) ? m_Prev.at(at) : "")
        {
            path.push_back(at);
            if (at == m_StartNode) break;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

private:
    double heuristic(const string& a, const string& b)
    {
        const double R = 6371000;
        double lat1 = m_Graph.at(a).lat * M_PI / 180.0;
        double lat2 = m_Graph.at(b).lat * M_PI / 180.0;
        double dLat = (m_Graph.at(b).lat - m_Graph.at(a).lat) * M_PI / 180.0;
        double dLon = (m_Graph.at(b).lon - m_Graph.at(a).lon) * M_PI / 180.0;
        double x = sin(dLat/2)*sin(dLat/2) + cos(lat1)*cos(lat2)*sin(dLon/2)*sin(dLon/2);
        return R * 2 * atan2(sqrt(x), sqrt(1-x));
    }

    void dijkstra()
    {
        using Pair = std::pair<double, string>;
        std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> pq;
        m_Dist[m_StartNode] = 0;
        pq.emplace(0, m_StartNode);

        while (!pq.empty())
        {
            auto [cost, u] = pq.top(); pq.pop();
            if (cost > m_Dist[u]) continue;
            m_NodesExplored++;
            if (u == m_EndNode) break;
            for (auto &[v, weight] : m_Graph[u].neighbours)
            {
                double alt = m_Dist[u] + weight;
                if (alt < m_Dist[v])
                {
                    m_Dist[v] = alt;
                    m_Prev[v] = u;
                    pq.emplace(alt, v);
                }
            }
        }
    }

    void astar()
    {
        using Pair = std::pair<double, string>;
        std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> pq;
        m_Dist[m_StartNode] = 0;
        pq.emplace(0, m_StartNode);

        while (!pq.empty())
        {
            auto [cost, u] = pq.top(); pq.pop();
            if (cost > m_Dist[u] + heuristic(u, m_EndNode)) continue;
            m_NodesExplored++;
            if (u == m_EndNode) break;
            for (auto &[v, weight] : m_Graph[u].neighbours)
            {
                double alt = m_Dist[u] + weight;
                if (alt < m_Dist[v])
                {
                    m_Dist[v] = alt;
                    m_Prev[v] = u;
                    pq.emplace(alt + heuristic(v, m_EndNode), v);
                }
            }
        }
    }
};

#endif