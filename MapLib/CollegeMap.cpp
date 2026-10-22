#include "CollegeMap.h"
#include <fstream>
#include <iomanip>
#include <cstdlib>
using namespace std;

// initialize member variables
void CollegeMap::initialize()
{
    mapName = "None";
    nodesTotal = 0;
    weights = 0;
}

// empty map
CollegeMap::CollegeMap()
{
    initialize();
}

// load from file
CollegeMap::CollegeMap(string fileName)
{
    initialize();
    load(fileName);
}

// copy constructor
CollegeMap::CollegeMap(const CollegeMap& ob)
{
    initialize();
    copyFrom(ob);
}

CollegeMap::~CollegeMap()
{
    release();
}

// make array, all zeros
void CollegeMap::allocate(int nodes)
{
    nodesTotal = nodes;
    weights = new int* [nodesTotal];
    if (!weights)
    {
        cout << "Allocation failed." << endl;
        exit(1);
    }
    for (int row = 0; row < nodesTotal; ++row)
    {
        weights[row] = new int[nodesTotal];
        if (!weights[row])
        {
            cout << "Allocation failed." << endl;
            exit(1);
        }
        for (int col = 0; col < nodesTotal; col++)
            weights[row][col] = 0;
    }
}

// free the array
void CollegeMap::release()
{
    if (weights)
    {
        for (int row = 0; row < nodesTotal; row++)
            delete[] weights[row];
        delete[] weights;
    }
    weights = 0;
    nodesTotal = 0;
    mapName = "None";
}

// deep copy
void CollegeMap::copyFrom(const CollegeMap& ob)
{
    int n = ob.nodesTotal;
    string name = ob.mapName;
    int** temp = 0;

    if (n > 0)
    {
        temp = new int* [n];
        if (!temp)
        {
            cout << "Allocation failed." << endl;
            exit(1);
        }
        for (int row = 0; row < n; ++row)
        {
            temp[row] = new int[n];
            if (!temp[row])
            {
                cout << "Allocation failed." << endl;
                exit(1);
            }
            for (int col = 0; col < n; col++)
                temp[row][col] = ob.weights[row][col];
        }
    }

    release();      // free old map
    weights = temp;
    nodesTotal = n;
    mapName = name;
}

// word to int, false if bad
bool CollegeMap::toNumber(string word, int& value) const
{
    int length = (int)word.length();
    if (length == 0)
        return false;

    int start = 0;
    if (word[0] == '-')
        start = 1;
    if (start == length)
        return false;

    for (int i = start; i < length; i++)
    {
        if (!isdigit(word[i]))
            return false;
    }

    value = atoi(word.c_str());
    return true;
}

// same name, size, weights
bool CollegeMap::sameAs(const CollegeMap& ob) const
{
    if (!(mapName == ob.mapName) || nodesTotal != ob.nodesTotal)
        return false;

    for (int row = 0; row < nodesTotal; row++)
        for (int col = 0; col < nodesTotal; col++)
            if (weights[row][col] != ob.weights[row][col])
                return false;

    return true;
}

CollegeMap& CollegeMap::operator=(const CollegeMap& ob)
{
    copyFrom(ob);
    return *this;
}

bool CollegeMap::operator==(const CollegeMap& ob) const
{
    return sameAs(ob);
}

// print name and matrix
ostream& operator<<(ostream& stream, const CollegeMap& ob)
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

// read file, false if bad
bool CollegeMap::load(string fileName)
{
    release();

    ifstream fin(fileName);
    if (!fin)
    {
        cout << "Cannot open input file: " << fileName << endl;
        return false;
    }

    string name = "";
    string word = "";
    int nodes = 0;
    fin >> name >> word;
    if (!toNumber(word, nodes) || nodes <= 0)
    {
        cout << "Missing or invalid node count in: " << fileName << endl;
        return false;
    }

    allocate(nodes);

    for (int row = 0; row < nodesTotal; row++)
    {
        for (int col = 0; col < nodesTotal; col++)
        {
            word = "";
            fin >> word;
            if (!toNumber(word, weights[row][col]))
            {
                cout << "Too few or non-numeric weights in: " << fileName << endl;
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

    // diagonal 0
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