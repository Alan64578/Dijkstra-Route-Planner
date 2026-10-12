#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <iostream>
#include <string>
#include "CollegeMap.h"
using namespace std;

class Dijkstra
{
    CollegeMap *map;    // not owned, never deleted here
    int nodes;
    int source;         // -1 = not run yet
    int *dist;
    bool *processed;
    int *parent;

    void allocate(int size);
    void release();
    int minDistance() const;

public:
    Dijkstra(CollegeMap &aMap);
    ~Dijkstra();

    // no copying allowed
    Dijkstra(const Dijkstra &ob) = delete;
    Dijkstra &operator=(const Dijkstra &ob) = delete;

    bool run(int src);
    int getSource() const;
    bool isReachable(int dest) const;
    int getDistance(int dest) const;
    int getRoute(int dest, int *route) const;

    void printRoute(int dest) const;
    bool saveResults(string fileName);

    // prints all routes, works for cout or a file
    friend ostream &operator<<(ostream &stream, const Dijkstra &ob);
};

#endif
