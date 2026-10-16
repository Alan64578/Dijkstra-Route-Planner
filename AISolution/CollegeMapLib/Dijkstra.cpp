// Dijkstra.cpp

#include "Dijkstra.h"
#include "CollegeMap.h"

#include <climits>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <stdexcept>

const int Dijkstra::NoRoute = -1;

namespace
{
    const int Infinity = INT_MAX;

    // Unprocessed node with the smallest finite distance, or -1 if every
    // remaining node is unreachable.
    // Fix for reference-code defect 2: the original used "<=" against
    // INT_MAX, so once only unreachable nodes were left it picked one of
    // them and dist[u] + weight overflowed to a negative distance.
    // (It also returned an uninitialised index if nothing matched.)
    int closestUnprocessed(const int* distance, const bool* processed, int n)
    {
        int best = -1;
        int bestDistance = Infinity;
        for (int v = 0; v < n; ++v)
        {
            if (!processed[v] && distance[v] < bestDistance)
            {
                bestDistance = distance[v];
                best = v;
            }
        }
        return best;
    }
}

Dijkstra::Dijkstra(const CollegeMap& map)
    : m_map(map), m_nodes(0), m_source(-1), m_revision(0),
      m_distance(nullptr), m_processed(nullptr), m_parent(nullptr)
{
}

Dijkstra::~Dijkstra()
{
    release();   // only our own arrays; the map is not ours to free
}

void Dijkstra::release() noexcept
{
    delete[] m_distance;
    delete[] m_processed;
    delete[] m_parent;
    m_distance = nullptr;
    m_processed = nullptr;
    m_parent = nullptr;
    m_nodes = 0;
}

void Dijkstra::run(int source)
{
    if (!m_map.isLoaded())
        throw std::logic_error("Dijkstra::run: the map has no data loaded");

    const int n = m_map.nodes();
    if (source < 0 || source >= n)
        throw std::out_of_range("Dijkstra::run: source " + std::to_string(source)
                                + " is outside 0.." + std::to_string(n - 1));

    // D2: arrays are sized to the map as it is now, on every run, so a
    // map that has been reloaded with a different size is handled.
    // Build into locals first: if an allocation fails, the previous
    // results are untouched and nothing leaks.
    const std::size_t size = static_cast<std::size_t>(n);
    int*  distance  = new int[size];
    bool* processed = nullptr;
    int*  parent    = nullptr;
    try
    {
        processed = new bool[size];
        parent    = new int[size];
    }
    catch (...)
    {
        delete[] processed;
        delete[] distance;
        throw;
    }

    // Fix for reference-code defect 1: the original set parent[0] = -1
    // instead of initialising every entry (including parent[src]). With
    // any source other than 0, parent[src] was garbage and the route
    // printer walked off into uninitialised memory.
    for (int i = 0; i < n; ++i)
    {
        distance[i]  = Infinity;
        processed[i] = false;
        parent[i]    = -1;
    }
    distance[source] = 0;

    for (int count = 0; count < n; ++count)
    {
        const int u = closestUnprocessed(distance, processed, n);
        if (u == -1)
            break;                    // everything left is unreachable
        processed[u] = true;

        for (int v = 0; v < n; ++v)
        {
            const int w = m_map(u, v);   // element access only (CM8)
            // distance[u] is finite here, and weights are capped by
            // CollegeMap::MaxWeight, so the sum cannot overflow.
            if (!processed[v] && w > 0 && distance[u] + w < distance[v])
            {
                distance[v] = distance[u] + w;
                parent[v] = u;
            }
        }
    }

    release();                         // old results, if any
    m_distance  = distance;
    m_processed = processed;
    m_parent    = parent;
    m_nodes     = n;
    m_source    = source;
    m_revision  = m_map.revision();
}

bool Dijkstra::hasRun() const { return m_distance != nullptr; }

bool Dijkstra::isStale() const
{
    return hasRun() && m_map.revision() != m_revision;
}

int Dijkstra::source() const              { return m_source; }
const CollegeMap& Dijkstra::map() const   { return m_map; }

void Dijkstra::checkResults() const
{
    if (!hasRun())
        throw std::logic_error("Dijkstra: no results yet, call run() first");
    if (isStale())
        throw std::logic_error("Dijkstra: the map has changed since run(), call run() again");
}

void Dijkstra::checkDestination(int destination) const
{
    checkResults();
    if (destination < 0 || destination >= m_nodes)
        throw std::out_of_range("Dijkstra: destination " + std::to_string(destination)
                                + " is outside 0.." + std::to_string(m_nodes - 1));
}

bool Dijkstra::isReachable(int destination) const
{
    checkDestination(destination);
    return m_distance[destination] != Infinity;
}

int Dijkstra::distanceTo(int destination) const
{
    checkDestination(destination);
    return m_distance[destination] == Infinity ? NoRoute : m_distance[destination];
}

std::string Dijkstra::routeTo(int destination) const
{
    checkDestination(destination);
    if (m_distance[destination] == Infinity)
        return "unreachable";

    // Walk back through the parent array (replaces the recursive printPath).
    std::string route = std::to_string(destination);
    for (int node = m_parent[destination]; node != -1; node = m_parent[node])
        route = std::to_string(node) + " -> " + route;
    return route;
}

void Dijkstra::writeResults(std::ostream& os) const
{
    checkResults();

    os << "Map: " << m_map.name() << "   Nodes: " << m_nodes << '\n'
       << "Shortest routes from node " << m_source << "\n\n"
       << "Destination   Distance   Route\n";

    for (int dest = 0; dest < m_nodes; ++dest)
    {
        if (dest == m_source)
            continue;
        os << std::setw(6) << dest << "     ";
        if (m_distance[dest] == Infinity)
            os << std::setw(10) << "-" << "      unreachable\n";
        else
            os << std::setw(10) << m_distance[dest] << "      " << routeTo(dest) << '\n';
    }
}

bool Dijkstra::saveResults(const std::string& fileName) const
{
    checkResults();
    std::ofstream file(fileName);
    if (!file)
        return false;
    writeResults(file);
    return static_cast<bool>(file);
}
