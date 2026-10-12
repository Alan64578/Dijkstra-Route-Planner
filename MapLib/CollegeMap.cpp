#include "CollegeMap.h"
#include <fstream>
#include <iomanip>
using namespace std;

// empty map
CollegeMap::CollegeMap() : mapName("None"), nodesTotal(0), weights(0)
{
}

// load map from file
CollegeMap::CollegeMap(string fileName) : mapName("None"), nodesTotal(0), weights(0)
{
    load(fileName);
}

// copy constructor
CollegeMap::CollegeMap(const CollegeMap &ob) : mapName("None"), nodesTotal(0), weights(0)
{
    copyFrom(ob);
}

CollegeMap::~CollegeMap()
{
    release();
}

// make the 2D array, all set to 0
void CollegeMap::allocate(int nodes)
{
    nodesTotal = nodes;
    weights = new int*[nodesTotal];
    for (int row = 0; row < nodesTotal; row++)
    {
        weights[row] = new int[nodesTotal];
        for (int col = 0; col < nodesTotal; col++)
            weights[row][col] = 0;
    }
}

// free the 2D array
void CollegeMap::release()
{
    if (weights != 0)
    {
        for (int row = 0; row < nodesTotal; row++)
            delete[] weights[row];
        delete[] weights;
    }
    weights = 0;
    nodesTotal = 0;
    mapName = "None";
}

// deep copy, used by copy constructor and =
// copies into a new array first and frees the old one after,
// so a = a still works
void CollegeMap::copyFrom(const CollegeMap &ob)
{
    int n = ob.nodesTotal;
    string name = ob.mapName;
    int **temp = 0;

    if (n > 0)
    {
        temp = new int*[n];
        for (int row = 0; row < n; row++)
        {
            temp[row] = new int[n];
            for (int col = 0; col < n; col++)
                temp[row][col] = ob.weights[row][col];
        }
    }

    release();      // free the old map
    weights = temp;
    nodesTotal = n;
    mapName = name;
}

CollegeMap &CollegeMap::operator=(const CollegeMap &ob)
{
    copyFrom(ob);
    return *this;
}

bool CollegeMap::operator==(const CollegeMap &ob) const
{
    if (mapName != ob.mapName || nodesTotal != ob.nodesTotal)
        return false;

    for (int row = 0; row < nodesTotal; row++)
        for (int col = 0; col < nodesTotal; col++)
            if (weights[row][col] != ob.weights[row][col])
                return false;

    return true;
}

bool CollegeMap::operator!=(const CollegeMap &ob) const
{
    return !(*this == ob);
}

// print name, nodes and weight matrix
ostream &operator<<(ostream &stream, const CollegeMap &ob)
{
    if (ob.nodesTotal == 0)
    {
        stream << "Map: (no map loaded)" << endl;
        return stream;
    }

    stream << "Map: " << ob.mapName << "   Nodes: " << ob.nodesTotal << endl;

    stream << "    ";
    for (int col = 0; col < ob.nodesTotal; col++)
        stream << setw(4) << col;
    stream << endl;

    for (int row = 0; row < ob.nodesTotal; row++)
    {
        stream << setw(4) << row;
        for (int col = 0; col < ob.nodesTotal; col++)
            stream << setw(4) << ob.weights[row][col];
        stream << endl;
    }
    return stream;
}

// read map file, returns false if anything is wrong
bool CollegeMap::load(string fileName)
{
    release();

    ifstream fin(fileName.c_str());
    if (!fin)
    {
        cout << "Cannot open input file: " << fileName << endl;
        return false;
    }

    string name;
    int nodes = 0;
    fin >> name >> nodes;
    if (!fin || nodes <= 0)
    {
        cout << "Missing or invalid node count in " << fileName << endl;
        return false;
    }

    allocate(nodes);

    for (int row = 0; row < nodesTotal; row++)
    {
        for (int col = 0; col < nodesTotal; col++)
        {
            fin >> weights[row][col];
            if (!fin)
            {
                cout << "Too few or non-numeric weights in " << fileName << endl;
                release();
                return false;
            }
            if (weights[row][col] < 0)
            {
                cout << "Negative weight at [" << row << "][" << col << "]" << endl;
                release();
                return false;
            }
        }
    }

    // diagonal must be 0, not symmetric is just a warning for now
    for (int row = 0; row < nodesTotal; row++)
    {
        if (weights[row][row] != 0)
        {
            cout << "Non-zero diagonal at node " << row << endl;
            release();
            return false;
        }
        for (int col = row + 1; col < nodesTotal; col++)
        {
            if (weights[row][col] != weights[col][row])
                cout << "Warning: matrix not symmetric at [" << row << "][" << col << "]" << endl;
        }
    }

    mapName = name;
    fin.close();
    return true;
}

bool CollegeMap::isLoaded() const
{
    return nodesTotal > 0;
}

string CollegeMap::getName() const
{
    return mapName;
}

int CollegeMap::getNodesTotal() const
{
    return nodesTotal;
}

bool CollegeMap::isValidNode(int node) const
{
    return node >= 0 && node < nodesTotal;
}

// -1 if out of range
int CollegeMap::getWeight(int from, int to) const
{
    if (!isValidNode(from) || !isValidNode(to))
        return -1;
    return weights[from][to];
}

// sets both directions
bool CollegeMap::setWeight(int from, int to, int weight)
{
    if (!isValidNode(from) || !isValidNode(to))
    {
        cout << "setWeight: node index out of range" << endl;
        return false;
    }
    if (weight < 0)
    {
        cout << "setWeight: weight cannot be negative" << endl;
        return false;
    }
    if (from == to && weight != 0)
    {
        cout << "setWeight: a node's weight to itself must be 0" << endl;
        return false;
    }

    weights[from][to] = weight;
    weights[to][from] = weight;
    return true;
}
