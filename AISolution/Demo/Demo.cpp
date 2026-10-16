// main.cpp - demonstration program for CollegeMap and Dijkstra.
// Lives in its own project; the classes come from the CollegeMapLib
// static library.

#include "CollegeMap.h"
#include "Dijkstra.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    void heading(const std::string& title)
    {
        std::cout << "\n========== " << title << " ==========\n";
    }

    // Keeps asking until the user enters a valid node number.
    int askForNode(int nodes)
    {
        while (true)
        {
            std::cout << "Enter a source node (0-" << nodes - 1 << "): ";
            std::string line;
            if (!std::getline(std::cin, line))
            {
                std::cout << "\nNo input, using node 0.\n";
                return 0;
            }
            try
            {
                std::size_t used = 0;
                const int node = std::stoi(line, &used);
                if (line.find_first_not_of(" \t\r", used) == std::string::npos
                    && node >= 0 && node < nodes)
                    return node;
            }
            catch (const std::exception&)
            {
                // not a number - fall through and ask again
            }
            std::cout << "'" << line << "' is not a node of this map.\n";
        }
    }

    void showRoute(const Dijkstra& d, int destination)
    {
        std::cout << "  " << d.source() << " to " << destination << ": "
            << d.routeTo(destination) << "  (distance "
            << d.distanceTo(destination) << ")\n";
    }
}

int main()
{
    try
    {
        // ---------------------------------------------------------------
        heading("1. Load 'MTU Map.txt' and display it");
        CollegeMap map("MTU Map.txt");
        if (!map.isLoaded())
        {
            // The supplied file has 3->5 = 14 but 5->3 = 0 (and 6->3 = 14
            // but 3->6 = 0), so validation rejects it. Fall back to a
            // corrected copy with the single 3-5 edge of the original graph.
            std::cout << "'MTU Map.txt' rejected (" << toString(map.status()) << "): "
                << map.statusMessage() << '\n'
                << "Loading the corrected copy instead.\n\n";
            if (map.load("MTU Map (corrected).txt") != CollegeMap::LoadStatus::Ok)
            {
                std::cout << "Could not load the corrected map either: "
                    << map.statusMessage() << '\n'
                    << "(Are the map files in the working directory?)\n";
                return 1;
            }
        }
        std::cout << map;

        std::cout << "\nLoading a file that does not exist:\n";
        CollegeMap missing("No Such Map.txt");
        std::cout << "  status: " << toString(missing.status()) << " - "
            << missing.statusMessage() << "\n  display: ";
        missing.writeMetadata(std::cout);   // still safe to use

        // ---------------------------------------------------------------
        heading("2. Change an edge weight and see the route change");
        Dijkstra router(map);
        router.run(0);
        std::cout << "Before:\n";
        showRoute(router, 6);

        std::cout << "Setting edge 6-7 from " << map(6, 7) << " to 20...\n";
        map.setWeight(6, 7, 20);
        std::cout << "  map(6,7) = " << map(6, 7) << ", map(7,6) = " << map(7, 6)
            << " (undirected)\n";

        std::cout << "  Old results stale? " << (router.isStale() ? "yes" : "no") << '\n';
        try
        {
            router.distanceTo(6);
        }
        catch (const std::logic_error& e)
        {
            std::cout << "  Querying stale results is refused: " << e.what() << '\n';
        }

        router.run(0);
        std::cout << "After re-running:\n";
        showRoute(router, 6);

        map.setWeight(6, 7, 1);   // put it back
        std::cout << "Edge 6-7 restored to " << map(6, 7) << ".\n";

        std::cout << "Misuse is rejected:\n";
        try { map.setWeight(0, 9, 5); }
        catch (const std::out_of_range& e) { std::cout << "  " << e.what() << '\n'; }
        try { map.setWeight(0, 1, -3); }
        catch (const std::invalid_argument& e) { std::cout << "  " << e.what() << '\n'; }

        // ---------------------------------------------------------------
        heading("3. Copying and assigning maps");
        CollegeMap copied(map);                    // copy constructor
        std::cout << "copied == map? " << (copied == map ? "yes" : "no") << '\n';
        copied.setWeight(0, 1, 99);
        std::cout << "After copied.setWeight(0,1,99): map(0,1) = " << map(0, 1)
            << ", copied(0,1) = " << copied(0, 1)
            << ", copied == map? " << (copied == map ? "yes" : "no") << '\n';

        CollegeMap assigned;                       // starts empty
        assigned = map;                            // copy assignment
        assigned.setWeight(2, 8, 50);
        std::cout << "After assigned = map; assigned.setWeight(2,8,50): map(2,8) = "
            << map(2, 8) << ", assigned(2,8) = " << assigned(2, 8) << '\n';

        CollegeMap& alias = assigned;
        assigned = alias;                          // self-assignment
        std::cout << "After self-assignment: assigned(2,8) = " << assigned(2, 8)
            << ", still " << assigned.nodes() << " nodes\n";

        router.run(0);            // map changed (edge restored) since last run
        Dijkstra onCopy(copied);
        onCopy.run(0);
        std::cout << "Route 0 to 2 on the original vs the modified copy:\n";
        showRoute(router, 2);
        showRoute(onCopy, 2);

        // ---------------------------------------------------------------
        heading("4. Routes from a node of your choice");
        const int source = askForNode(map.nodes());
        router.run(source);
        std::cout << '\n';
        router.writeResults(std::cout);

        const std::string outFile = "Routes from node " + std::to_string(source) + ".txt";
        if (router.saveResults(outFile))
            std::cout << "\nSaved to '" << outFile << "'.\n";
        else
            std::cout << "\nCould not write '" << outFile << "'.\n";

        // ---------------------------------------------------------------
        heading("5. Aggregation: destroy a Dijkstra object, keep the map");
        {
            Dijkstra temporary(map);
            temporary.run(4);
            std::cout << "Temporary Dijkstra object:\n";
            showRoute(temporary, 0);
            std::cout << "Leaving scope: temporary's destructor runs now.\n";
        }
        std::cout << "The map is still valid and usable: ";
        map.writeMetadata(std::cout);
        std::cout << "  map(0,1) = " << map(0, 1) << '\n';

        Dijkstra another(map);
        another.run(4);
        std::cout << "A new Dijkstra object on the same map:\n";
        showRoute(another, 0);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
