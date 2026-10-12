#include "Dijkstra.h"
#include <fstream>
#include <iomanip>
#include <climits>
using namespace std;

// just keep a pointer to the map
Dijkstra::Dijkstra(CollegeMap &aMap)
    : map(&aMap), nodes(0), source(-1), dist(0), processed(0), parent(0)
{
}

// only frees our arrays, not the map
Dijkstra::~Dijkstra()
{
    release();
}

void Dijkstra::allocate(int size)
{
    nodes = size;
    dist = new int[nodes];
    processed = new bool[nodes];
    parent = new int[nodes];
}

void Dijkstra::release()
{
    if (dist != 0)
        delete[] dist;
    if (processed != 0)
        delete[] processed;
    if (parent != 0)
        delete[] parent;
    dist = 0;
    processed = 0;
    parent = 0;
    nodes = 0;
}

// closest node not processed yet, -1 if none left
int Dijkstra::minDistance() const
{
    int min = INT_MAX;
    int minIndex = -1;

    for (int v = 0; v < nodes; v++)
    {
        if (!processed[v] && dist[v] < min)
        {
            min = dist[v];
            minIndex = v;
        }
    }
    return minIndex;
}

bool Dijkstra::run(int src)
{
    if (!map->isLoaded())
    {
        cout << "Dijkstra: no map loaded" << endl;
        source = -1;
        return false;
    }
    if (!map->isValidNode(src))
    {
        cout << "Dijkstra: invalid source node " << src << endl;
        source = -1;
        return false;
    }

    // allocate each run in case the map size changed
    release();
    allocate(map->getNodesTotal());

    for (int i = 0; i < nodes; i++)
    {
        dist[i] = INT_MAX;
        processed[i] = false;
        parent[i] = -1;
    }
    dist[src] = 0;

    for (int count = 0; count < nodes; count++)
    {
        int u = minDistance();
        if (u == -1)
            break;      // rest are unreachable

        processed[u] = true;

        for (int v = 0; v < nodes; v++)
        {
            int w = map->getWeight(u, v);
            if (!processed[v] && w > 0 && dist[u] + w < dist[v])
            {
                parent[v] = u;
                dist[v] = dist[u] + w;
            }
        }
    }

    source = src;
    return true;
}

int Dijkstra::getSource() const
{
    return source;
}

bool Dijkstra::isReachable(int dest) const
{
    if (source == -1 || dest < 0 || dest >= nodes)
        return false;
    return dist[dest] != INT_MAX;
}

// -1 if unreachable
int Dijkstra::getDistance(int dest) const
{
    if (!isReachable(dest))
        return -1;
    return dist[dest];
}

// puts the route source..dest into route[], returns how many nodes (0 if none)
// route[] needs room for the number of nodes in the map
int Dijkstra::getRoute(int dest, int *route) const
{
    if (!isReachable(dest))
        return 0;

    // count the nodes back to the source
    int count = 0;
    for (int node = dest; node != -1; node = parent[node])
        count++;

    // fill in backwards so route[0] is the source
    int i = count - 1;
    for (int node = dest; node != -1; node = parent[node])
    {
        route[i] = node;
        i--;
    }
    return count;
}

// one destination to the screen
void Dijkstra::printRoute(int dest) const
{
    if (source == -1)
    {
        cout << "No results - call run() first" << endl;
        return;
    }
    if (dest < 0 || dest >= nodes)
    {
        cout << "Invalid destination " << dest << endl;
        return;
    }
    if (!isReachable(dest))
    {
        cout << "Node " << dest << " is unreachable from node " << source << endl;
        return;
    }

    int *route = new int[nodes];
    int count = getRoute(dest, route);

    cout << "Node " << source << " to " << dest << ", distance " << dist[dest] << ": ";
    for (int i = 0; i < count; i++)
    {
        if (i > 0)
            cout << " -> ";
        cout << route[i];
    }
    cout << endl;

    delete[] route;
}

// all routes, same code for cout and files
ostream &operator<<(ostream &stream, const Dijkstra &ob)
{
    if (ob.source == -1)
    {
        stream << "No results - call run() first" << endl;
        return stream;
    }

    stream << "Shortest routes from node " << ob.source << endl;
    stream << "Destination  Distance    Route" << endl;

    int *route = new int[ob.nodes];
    for (int dest = 0; dest < ob.nodes; dest++)
    {
        if (dest != ob.source)
        {
            stream << setw(11) << dest;
            if (!ob.isReachable(dest))
            {
                stream << setw(10) << "-" << "    unreachable" << endl;
            }
            else
            {
                stream << setw(10) << ob.dist[dest] << "    ";
                int count = ob.getRoute(dest, route);
                for (int i = 0; i < count; i++)
                {
                    if (i > 0)
                        stream << " -> ";
                    stream << route[i];
                }
                stream << endl;
            }
        }
    }
    delete[] route;

    return stream;
}

// writes map and routes to a text file
bool Dijkstra::saveResults(string fileName)
{
    if (source == -1)
    {
        cout << "Nothing to save - call run() first" << endl;
        return false;
    }

    ofstream fout(fileName.c_str());
    if (!fout)
    {
        cout << "Cannot open output file: " << fileName << endl;
        return false;
    }

    fout << *map << endl;
    fout << *this;
    fout.close();
    return true;
}
