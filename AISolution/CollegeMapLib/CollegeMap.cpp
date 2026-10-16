// CollegeMap.cpp

#include "CollegeMap.h"

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

const int CollegeMap::MaxNodes  = 1000;
const int CollegeMap::MaxWeight = 1000000;   // 1000 nodes * 1e6 < INT_MAX

// ------------------------------------------------------------------
// File-parsing helpers (private to this file)
// ------------------------------------------------------------------
namespace
{
    std::string trim(const std::string& text)
    {
        const char* space = " \t\r\n";
        const std::size_t first = text.find_first_not_of(space);
        if (first == std::string::npos)
            return "";
        const std::size_t last = text.find_last_not_of(space);
        return text.substr(first, last - first + 1);
    }

    // Reads the next line that is not blank. Returns false at end of file.
    bool nextContentLine(std::istream& in, std::string& line)
    {
        while (std::getline(in, line))
        {
            if (!trim(line).empty())
                return true;
        }
        return false;
    }

    // Whole-token integer parse: "12" ok, "12x", "4.5", "abc" rejected.
    bool parseInt(const std::string& token, int& value)
    {
        std::istringstream ss(token);
        char extra = '\0';
        return (ss >> value) && !(ss >> extra);
    }

    std::string at(int row, int col)
    {
        return "row " + std::to_string(row) + ", column " + std::to_string(col);
    }

    // Reads and validates an n x n matrix into buffer (already allocated).
    CollegeMap::LoadStatus parseMatrix(std::istream& in, int n, int* buffer,
                                       std::string& detail)
    {
        using S = CollegeMap::LoadStatus;
        std::string line;

        for (int row = 0; row < n; ++row)
        {
            if (!nextContentLine(in, line))
            {
                detail = "expected " + std::to_string(n) + " rows of weights, found "
                       + std::to_string(row);
                return S::TooFewRows;
            }

            std::istringstream values(line);
            std::string token;
            int col = 0;
            while (values >> token)
            {
                if (col == n)
                {
                    detail = "row " + std::to_string(row) + " has more than "
                           + std::to_string(n) + " values";
                    return S::WrongRowLength;
                }
                int value = 0;
                if (!parseInt(token, value))
                {
                    detail = at(row, col) + ": '" + token + "' is not a whole number";
                    return S::NonNumericValue;
                }
                if (value < 0 || value > CollegeMap::MaxWeight)
                {
                    detail = at(row, col) + ": weight " + token + " is outside 0.."
                           + std::to_string(CollegeMap::MaxWeight);
                    return S::WeightOutOfRange;
                }
                buffer[row * n + col] = value;
                ++col;
            }
            if (col < n)
            {
                detail = "row " + std::to_string(row) + " has " + std::to_string(col)
                       + " values, expected " + std::to_string(n);
                return S::WrongRowLength;
            }
        }

        for (int i = 0; i < n; ++i)
        {
            if (buffer[i * n + i] != 0)
            {
                detail = at(i, i) + ": a node's distance to itself must be 0";
                return S::NonZeroDiagonal;
            }
        }

        for (int row = 0; row < n; ++row)
        {
            for (int col = row + 1; col < n; ++col)
            {
                if (buffer[row * n + col] != buffer[col * n + row])
                {
                    detail = "weight " + std::to_string(row) + "->" + std::to_string(col)
                           + " differs from " + std::to_string(col) + "->"
                           + std::to_string(row) + " (map must be undirected)";
                    return S::NotSymmetric;
                }
            }
        }

        if (nextContentLine(in, line))
        {
            detail = "unexpected data after the " + std::to_string(n) + " rows: '"
                   + trim(line) + "'";
            return S::TrailingData;
        }
        return S::Ok;
    }
}

// ------------------------------------------------------------------
// Construction, copying, release
// ------------------------------------------------------------------
CollegeMap::CollegeMap()
    : m_name(), m_nodes(0), m_weights(nullptr),
      m_status(LoadStatus::NotLoaded), m_message("no map loaded"),
      m_revision(nextRevision())
{
}

CollegeMap::CollegeMap(const std::string& fileName)
    : CollegeMap()
{
    load(fileName);
}

CollegeMap::CollegeMap(const CollegeMap& other)
    : m_name(other.m_name), m_nodes(other.m_nodes), m_weights(nullptr),
      m_status(other.m_status), m_message(other.m_message),
      m_revision(nextRevision())
{
    if (other.m_weights != nullptr)
    {
        const int count = m_nodes * m_nodes;
        m_weights = new int[static_cast<std::size_t>(count)];
        for (int i = 0; i < count; ++i)
            m_weights[i] = other.m_weights[i];
    }
}

CollegeMap::CollegeMap(CollegeMap&& other) noexcept
    : CollegeMap()
{
    swap(*this, other);
}

// Parameter taken by value: the copy (or move) is made by the copy
// constructor before we get here, so the copying logic exists once,
// self-assignment is safe, and if the copy throws *this is untouched.
CollegeMap& CollegeMap::operator=(CollegeMap other) noexcept
{
    swap(*this, other);
    return *this;
}   // other (holding the old data) is destroyed here -> delete[]

CollegeMap::~CollegeMap()
{
    delete[] m_weights;
}

void swap(CollegeMap& a, CollegeMap& b) noexcept
{
    using std::swap;
    swap(a.m_name, b.m_name);
    swap(a.m_nodes, b.m_nodes);
    swap(a.m_weights, b.m_weights);
    swap(a.m_status, b.m_status);
    swap(a.m_message, b.m_message);
    // Both objects now hold different contents.
    a.m_revision = CollegeMap::nextRevision();
    b.m_revision = CollegeMap::nextRevision();
}

unsigned long CollegeMap::nextRevision() noexcept
{
    static unsigned long counter = 0;
    return ++counter;
}

// ------------------------------------------------------------------
// Loading
// ------------------------------------------------------------------
CollegeMap::LoadStatus CollegeMap::fail(LoadStatus status, const std::string& message)
{
    m_status = status;
    m_message = message;
    return status;
}

CollegeMap::LoadStatus CollegeMap::load(const std::string& fileName)
{
    std::ifstream file(fileName);
    if (!file)
        return fail(LoadStatus::FileNotFound, "cannot open '" + fileName + "'");

    std::string line;
    if (!nextContentLine(file, line))
        return fail(LoadStatus::MissingName, "'" + fileName + "' is empty");
    std::string newName = trim(line);

    if (!nextContentLine(file, line))
        return fail(LoadStatus::BadNodeCount, "node count is missing");

    int n = 0;
    if (!parseInt(trim(line), n))
        return fail(LoadStatus::BadNodeCount,
                    "node count '" + trim(line) + "' is not a single whole number");
    if (n < 1 || n > MaxNodes)
        return fail(LoadStatus::BadNodeCount,
                    "node count " + std::to_string(n) + " is outside 1.."
                    + std::to_string(MaxNodes));

    // Parse into a temporary block so a bad file never damages the
    // map we already hold. This block is released exactly once on
    // every path: by the catch, by the failure branch, or by this
    // object's destructor after it has been committed.
    int* buffer = new int[static_cast<std::size_t>(n) * static_cast<std::size_t>(n)];
    LoadStatus result = LoadStatus::Ok;
    std::string detail;
    try
    {
        result = parseMatrix(file, n, buffer, detail);
    }
    catch (...)
    {
        delete[] buffer;
        throw;
    }

    if (result != LoadStatus::Ok)
    {
        delete[] buffer;
        return fail(result, detail);
    }

    // Commit (nothing below can throw before the old block is freed
    // and the new one adopted).
    delete[] m_weights;
    m_weights = buffer;
    m_nodes = n;
    m_name.swap(newName);
    m_status = LoadStatus::Ok;
    m_revision = nextRevision();
    m_message = "loaded '" + fileName + "'";
    return LoadStatus::Ok;
}

void CollegeMap::clear()
{
    delete[] m_weights;
    m_weights = nullptr;
    m_nodes = 0;
    m_name.clear();
    m_status = LoadStatus::NotLoaded;
    m_message = "no map loaded";
    m_revision = nextRevision();
}

bool CollegeMap::isLoaded() const                  { return m_weights != nullptr; }
CollegeMap::LoadStatus CollegeMap::status() const  { return m_status; }
const std::string& CollegeMap::statusMessage() const { return m_message; }
const std::string& CollegeMap::name() const        { return m_name; }
int CollegeMap::nodes() const                      { return m_nodes; }
unsigned long CollegeMap::revision() const         { return m_revision; }

// ------------------------------------------------------------------
// Weight access
// ------------------------------------------------------------------
void CollegeMap::checkIndex(int node, const char* caller) const
{
    if (!isLoaded())
        throw std::logic_error(std::string(caller) + ": no map is loaded");
    if (node < 0 || node >= m_nodes)
        throw std::out_of_range(std::string(caller) + ": node " + std::to_string(node)
                                + " is outside 0.." + std::to_string(m_nodes - 1));
}

int CollegeMap::weight(int from, int to) const
{
    checkIndex(from, "CollegeMap::weight");
    checkIndex(to, "CollegeMap::weight");
    return m_weights[cell(from, to)];
}

int CollegeMap::operator()(int from, int to) const
{
    return weight(from, to);
}

void CollegeMap::setWeight(int a, int b, int newWeight)
{
    checkIndex(a, "CollegeMap::setWeight");
    checkIndex(b, "CollegeMap::setWeight");
    if (newWeight < 0 || newWeight > MaxWeight)
        throw std::invalid_argument("CollegeMap::setWeight: weight "
                                    + std::to_string(newWeight) + " is outside 0.."
                                    + std::to_string(MaxWeight));
    if (a == b && newWeight != 0)
        throw std::invalid_argument("CollegeMap::setWeight: a node's distance to itself must be 0");

    m_weights[cell(a, b)] = newWeight;
    m_weights[cell(b, a)] = newWeight;
    m_revision = nextRevision();
}

// ------------------------------------------------------------------
// Comparison
// ------------------------------------------------------------------
bool CollegeMap::operator==(const CollegeMap& other) const
{
    if (m_name != other.m_name || m_nodes != other.m_nodes
        || isLoaded() != other.isLoaded())
        return false;

    const int count = m_nodes * m_nodes;
    for (int i = 0; i < count; ++i)
    {
        if (m_weights[i] != other.m_weights[i])
            return false;
    }
    return true;
}

bool CollegeMap::operator!=(const CollegeMap& other) const
{
    return !(*this == other);
}

// ------------------------------------------------------------------
// Display
// ------------------------------------------------------------------
void CollegeMap::writeMetadata(std::ostream& os) const
{
    os << "Map: " << (isLoaded() ? m_name : std::string("(no map loaded)"))
       << "   Nodes: " << m_nodes << '\n';
}

void CollegeMap::writeMatrix(std::ostream& os) const
{
    if (!isLoaded())
    {
        os << "(no weights)\n";
        return;
    }

    os << "      ";
    for (int col = 0; col < m_nodes; ++col)
        os << std::setw(5) << col;
    os << '\n';

    for (int row = 0; row < m_nodes; ++row)
    {
        os << std::setw(5) << row << ':';
        for (int col = 0; col < m_nodes; ++col)
            os << std::setw(5) << m_weights[cell(row, col)];
        os << '\n';
    }
    os << "(0 = no direct edge)\n";
}

std::ostream& operator<<(std::ostream& os, const CollegeMap& map)
{
    map.writeMetadata(os);
    map.writeMatrix(os);
    return os;
}

std::string toString(CollegeMap::LoadStatus status)
{
    using S = CollegeMap::LoadStatus;
    switch (status)
    {
    case S::NotLoaded:        return "NotLoaded";
    case S::Ok:               return "Ok";
    case S::FileNotFound:     return "FileNotFound";
    case S::MissingName:      return "MissingName";
    case S::BadNodeCount:     return "BadNodeCount";
    case S::TooFewRows:       return "TooFewRows";
    case S::WrongRowLength:   return "WrongRowLength";
    case S::NonNumericValue:  return "NonNumericValue";
    case S::WeightOutOfRange: return "WeightOutOfRange";
    case S::NonZeroDiagonal:  return "NonZeroDiagonal";
    case S::NotSymmetric:     return "NotSymmetric";
    case S::TrailingData:     return "TrailingData";
    }
    return "Unknown";
}
