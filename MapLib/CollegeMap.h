#ifndef COLLEGEMAP_H
#define COLLEGEMAP_H

#include <iostream>
#include <string>

class CollegeMap
{
    std::string mapName;
    int nodesTotal;     // 0 = no map
    int** weights;      // 2D weight array

    void allocate(int nodes);
    void release();
    void initialize();
    void copyFrom(const CollegeMap& ob);
    bool toNumber(std::string word, int& value) const;
    bool sameAs(const CollegeMap& ob) const;

public:
    CollegeMap();
    CollegeMap(std::string fileName);
    CollegeMap(const CollegeMap& ob);   // copy constructor
    ~CollegeMap();

    CollegeMap& operator=(const CollegeMap& ob);
    bool operator==(const CollegeMap& ob) const;
    friend std::ostream& operator<<(std::ostream& stream, const CollegeMap& ob);

    bool load(std::string fileName);

    bool isLoaded() const;
    std::string getName() const;
    int getNodesTotal() const;
    bool isValidNode(int node) const;

    int getWeight(int from, int to) const;
    bool setWeight(int from, int to, int weight);
};

#endif