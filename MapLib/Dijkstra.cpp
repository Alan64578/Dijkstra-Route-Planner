#include "Dijkstra.h"
#include <fstream>
#include <iomanip>
#include <cstdlib>
using namespace std;

// means not reached yet
const int INFINITE_DIST = 2147483647;

// store map pointer only
Dijkstra::Dijkstra(CollegeMap* aMap)
{
    pMap = aMap;
    nodes = 0;
    source = -1;
    dist = 0;
    processed = 0;
    parent = 0;
}

// frees arrays, not map
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

    if (!dist || !processed || !parent)
    {
        cout << "Allocation failed." << endl;
        exit(1);
    }
}

void Dijkstra::release()
{
    if (dist)
        delete[] dist;
    if (processed)
        delete[] processed;
    if (parent)
        delete[] parent;
    dist = 0;
    processed = 0;
    parent = 0;
    nodes = 0;
}

// closest unprocessed node, -1 if none
int Dijkstra::minDistance() const
{
    int min = INFINITE_DIST;
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
    if (!pMap || !pMap->isLoaded())
    {
        cout << "Dijkstra: no map loaded" << endl;
        source = -1;
        return false;
    }
    if (!pMap->isValidNode(src))
    {
        cout << "Dijkstra: invalid source node " << src << endl;
        source = -1;
        return false;
    }

    // map size may change
    release();
    allocate(pMap->getNodesTotal());

    for (int i = 0; i < nodes; i++)
    {
        dist[i] = INFINITE_DIST;
        processed[i] = false;
        parent[i] = -1;
    }
    dist[src] = 0;

    // stop when none reachable
    int u = minDistance();
    while (u != -1)
    {
        processed[u] = true;

        for (int v = 0; v < nodes; v++)
        {
            int w = pMap->getWeight(u, v);
            if (!processed[v] && w > 0 && dist[u] + w < dist[v])
            {
                parent[v] = u;
                dist[v] = dist[u] + w;
            }
        }

        u = minDistance();
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
    return dist[dest] != INFINITE_DIST;
}

// -1 if unreachable
int Dijkstra::getDistance(int dest) const
{
    if (!isReachable(dest))
        return -1;
    return dist[dest];
}

// fills route, returns length
int Dijkstra::getRoute(int dest, int* route) const
{
    if (!isReachable(dest))
        return 0;

    // count back to source
    int count = 0;
    int node = dest;
    while (node != -1)
    {
        count++;
        node = parent[node];
    }

    // fill in backwards
    int i = count - 1;
    node = dest;
    while (node != -1)
    {
        route[i] = node;
        i--;
        node = parent[node];
    }
    return count;
}

// print path from source to destination
void Dijkstra::printPath(ostream& stream, int j) const
{
    // base case: j is source
    if (parent[j] == -1)
    {
        stream << j;
        return;
    }
    printPath(stream, parent[j]);
    stream << " -> " << j;
}

// one route to screen
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

    cout << "Node " << source << " to " << dest << ", distance " << dist[dest] << ": ";
    printPath(cout, dest);
    cout << endl;
}

// all routes, any stream
ostream& operator<<(ostream& stream, const Dijkstra& ob)
{
    if (ob.source == -1)
    {
        stream << "No results - call run() first" << endl;
        return stream;
    }

    stream << "Shortest routes from node " << ob.source << endl;
    stream << "Destination  Distance    Route" << endl;

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
                ob.printPath(stream, dest);
                stream << endl;
            }
        }
    }

    return stream;
}

// map and routes to file
bool Dijkstra::saveResults(string fileName) const
{
    if (source == -1)
    {
        cout << "Nothing to save - call run() first" << endl;
        return false;
    }

    ofstream fout(fileName);
    if (!fout)
    {
        cout << "Cannot open output file: " << fileName << endl;
        return false;
    }

    fout << *pMap << endl;
    fout << *this;
    fout.close();
    return true;
}