// CollegeMap.h
// Holds a named, undirected, weighted map loaded from a text file.
// The edge weights live in one contiguous block allocated with new[]
// and indexed as row * nodes + col.

#pragma once

#include <iosfwd>
#include <string>

class CollegeMap
{
public:
    // Result of the most recent load attempt (CM2).
    enum class LoadStatus
    {
        NotLoaded,          // no load attempted yet
        Ok,
        FileNotFound,
        MissingName,
        BadNodeCount,       // missing, non-numeric, < 1 or > MaxNodes
        TooFewRows,
        WrongRowLength,     // a row with too few or too many values
        NonNumericValue,
        WeightOutOfRange,   // negative, or larger than MaxWeight
        NonZeroDiagonal,
        NotSymmetric,
        TrailingData        // extra non-blank lines after the matrix
    };

    static const int MaxNodes;   // largest map accepted
    static const int MaxWeight;  // keeps every path sum well inside int range

    // ---- CM1 construction / CM7 copying / CM9 release ----
    CollegeMap();                                   // empty, no map loaded
    explicit CollegeMap(const std::string& fileName);
    CollegeMap(const CollegeMap& other);            // deep copy
    CollegeMap(CollegeMap&& other) noexcept;
    CollegeMap& operator=(CollegeMap other) noexcept;  // copy-and-swap
    ~CollegeMap();

    friend void swap(CollegeMap& a, CollegeMap& b) noexcept;

    // ---- CM2 loading ----
    // On success the old map is replaced. On failure the object keeps
    // whatever map it held before (strong guarantee) and status() /
    // statusMessage() say what was wrong.
    LoadStatus load(const std::string& fileName);
    void clear();                                   // back to "no map loaded"

    bool isLoaded() const;
    LoadStatus status() const;                      // result of last load()
    const std::string& statusMessage() const;

    // ---- CM4 metadata ----
    const std::string& name() const;
    int nodes() const;

    // ---- CM5 / CM8 weight access ----
    // Both throw std::logic_error if no map is loaded and
    // std::out_of_range if an index is outside 0..nodes()-1.
    int weight(int from, int to) const;
    int operator()(int from, int to) const;         // same as weight()

    // Undirected: sets a-b and b-a together. 0 removes the edge.
    // Throws std::invalid_argument for a weight outside 0..MaxWeight,
    // or a non-zero weight from a node to itself.
    void setWeight(int a, int b, int newWeight);

    // Changes every time the map's contents change. Lets a Dijkstra
    // object detect that its results are out of date.
    unsigned long revision() const;

    // ---- comparison: same name, size and weights ----
    bool operator==(const CollegeMap& other) const;
    bool operator!=(const CollegeMap& other) const;

    // ---- CM6 display ----
    void writeMetadata(std::ostream& os) const;
    void writeMatrix(std::ostream& os) const;
    friend std::ostream& operator<<(std::ostream& os, const CollegeMap& map);

private:
    std::string   m_name;
    int           m_nodes;
    int*          m_weights;     // m_nodes * m_nodes values, or nullptr
    LoadStatus    m_status;
    std::string   m_message;
    unsigned long m_revision;

    void checkIndex(int node, const char* caller) const;
    int  cell(int row, int col) const { return row * m_nodes + col; }
    LoadStatus fail(LoadStatus status, const std::string& message);
    static unsigned long nextRevision() noexcept;
};

std::string toString(CollegeMap::LoadStatus status);
