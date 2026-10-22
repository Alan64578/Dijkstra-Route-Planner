#include <iostream>
#include <string>
#include <cstdlib>
#include "CollegeMap.h"
#include "Dijkstra.h"
using namespace std;

int main()
{
    // load and show map
    CollegeMap mtu("MTU Map.txt");
    if (!mtu.isLoaded())
    {
        cout << "Could not load map" << endl;
        return 1;
    }
    cout << mtu << endl;

    // atoi gives 0 if bad
    string input = "";
    cout << "Enter source node (0-" << mtu.getNodesTotal() - 1 << "): ";
    cin >> input;
    int src = atoi(input.c_str());
    if (src == 0 && !(input == "0"))
        cout << "Not a number - using node 0" << endl;
    cout << endl;

    // new so we can delete
    Dijkstra* planner = new Dijkstra(&mtu);
    if (planner->run(src))
    {
        cout << *planner;
        if (planner->saveResults("Routes.txt"))
            cout << "Results saved to Routes.txt" << endl;
    }
    delete planner;     // map survives (aggregation)

    cout << endl << "Dijkstra object destroyed - map still usable: "
        << mtu.getName() << ", " << mtu.getNodesTotal() << " nodes" << endl << endl;

    // change weight and rerun
    Dijkstra planner2(&mtu);
    planner2.run(0);
    cout << "Before change: ";
    planner2.printRoute(4);

    mtu.setWeight(0, 4, 3);
    planner2.run(0);
    cout << "After adding edge 0-4: ";
    planner2.printRoute(4);
    cout << endl;

    // copy and assign
    CollegeMap copyMap = mtu;
    CollegeMap assigned;
    assigned = mtu;

    copyMap.setWeight(0, 1, 99);
    cout << "After changing copyMap(0,1) to 99:" << endl;
    cout << "  copyMap(0,1)  = " << copyMap.getWeight(0, 1) << endl;
    cout << "  mtu(0,1)      = " << mtu.getWeight(0, 1) << endl;
    cout << "  assigned(0,1) = " << assigned.getWeight(0, 1) << endl;

    if (copyMap == mtu)
        cout << "  copyMap == mtu ? yes" << endl;
    else
        cout << "  copyMap == mtu ? no" << endl;

    if (assigned == mtu)
        cout << "  assigned == mtu ? yes" << endl;
    else
        cout << "  assigned == mtu ? no" << endl;

    assigned = assigned;    // self assignment
    if (assigned == mtu)
        cout << "  after self-assignment, assigned == mtu ? yes" << endl;
    else
        cout << "  after self-assignment, assigned == mtu ? no" << endl;

    return 0;
}