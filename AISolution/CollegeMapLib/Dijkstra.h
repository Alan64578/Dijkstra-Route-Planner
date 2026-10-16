// Dijkstra.h
// Shortest routes over a CollegeMap.
//
// Aggregation: a Dijkstra object refers to a CollegeMap it does not own.
// The map must outlive the Dijkstra object; destroying the Dijkstra
// object never touches the map's data.

#pragma once

#include <iosfwd>
#include <string>

class CollegeMap;

class Dijkstra
{
public:
    static const int NoRoute;   // distanceTo() result for an unreachable node

    explicit Dijkstra(const CollegeMap& map);

    // Binding to a temporary map would leave a dangling reference,
    // so it is rejected at compile time: Dijkstra d(CollegeMap("x.txt"));
    Dijkstra(const CollegeMap&&) = delete;

    // D6: not copyable or assignable (see README / Part B).
    Dijkstra(const Dijkstra&) = delete;
    Dijkstra& operator=(const Dijkstra&) = delete;

    ~Dijkstra();

    // D3: computes routes from source. Can be called any number of times.
    // Throws std::logic_error if the map has no data loaded and
    // std::out_of_range if source is not a node of the map.
    void run(int source);

    bool hasRun() const;        // results exist (they may be stale)
    bool isStale() const;       // map changed since the last run()
    int  source() const;        // -1 before the first run
    const CollegeMap& map() const;

    // D4: queries on the last run. All throw std::logic_error if there are
    // no results or they are stale, and std::out_of_range for a bad node.
    bool        isReachable(int destination) const;
    int         distanceTo(int destination) const;   // NoRoute if unreachable
    std::string routeTo(int destination) const;      // "0 -> 7 -> 6" or "unreachable"

    // D4(b) / D5: table of results for every other node.
    void writeResults(std::ostream& os) const;
    bool saveResults(const std::string& fileName) const;   // false if file can't be written

private:
    const CollegeMap& m_map;      // not owned
    int               m_nodes;    // size of the arrays below
    int               m_source;
    unsigned long     m_revision; // map revision the results belong to
    int*              m_distance;
    bool*             m_processed;
    int*              m_parent;

    void release() noexcept;
    void checkResults() const;
    void checkDestination(int destination) const;
};
