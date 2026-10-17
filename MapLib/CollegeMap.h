#ifndef COLLEGEMAP_H
#define COLLEGEMAP_H

#include <iostream>
#include <string>

class CollegeMap
{
    std::string mapName;
    int nodesTotal;     // 0 = no map loaded
    int** weights;      // 2D array of weights

    void allocate(int nodes);
    void release();
    void copyFrom(const CollegeMap& ob);

public:
    CollegeMap();
    CollegeMap(std::string fileName);
    CollegeMap(const CollegeMap& ob);   // copy constructor
    ~CollegeMap();

    CollegeMap& operator=(const CollegeMap& ob);
    bool operator==(const CollegeMap& ob) const;
    bool operator!=(const CollegeMap& ob) const;
    friend std::ostream& operator<<(std::ostream& stream, const CollegeMap& ob);

    bool load(std::string fileName);

    // const = doesnt change the map
    bool isLoaded() const;
    std::string getName() const;
    int getNodesTotal() const;
    bool isValidNode(int node) const;

    int getWeight(int from, int to) const;
    bool setWeight(int from, int to, int weight);
};

#endif