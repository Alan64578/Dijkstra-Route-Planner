#include "pch.h"
#include <cstdio>
#include <fstream>
#include <string>
#include "CollegeMap.h"
#include "Dijkstra.h"
using namespace std;

// makes a text file for the tests
void writeFile(string fileName, string contents)
{
    ofstream fout(fileName.c_str());
    fout << contents;
    fout.close();
}

// reads a file back word by word so spacing doesnt matter
string readWords(string fileName)
{
    ifstream fin(fileName.c_str());
    string word, text;
    while (fin >> word)
    {
        if (text != "")
            text += " ";
        text += word;
    }
    return text;
}

// small 3 node map from appendix 4
class ThreeNodeMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        writeFile("tiny.txt", "Tiny_Map\n3\n0 5 0\n5 0 2\n0 2 0\n");
    }

    void TearDown() override
    {
        remove("tiny.txt");
        remove("out.txt");
    }
};

// MTU map from appendix 3
class MtuMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        writeFile("mtu.txt", "MTU_Map\n9\n"
            "0 4 0 0 0 0 0 8 0\n"
            "4 0 8 0 0 0 0 11 0\n"
            "0 8 0 7 0 4 0 0 2\n"
            "0 0 7 0 9 14 0 0 0\n"
            "0 0 0 9 0 10 0 0 0\n"
            "0 0 4 0 10 0 2 0 0\n"
            "0 0 0 14 0 2 0 1 6\n"
            "8 11 0 0 0 0 1 0 7\n"
            "0 0 2 0 0 0 6 7 0\n");
    }

    void TearDown() override
    {
        remove("mtu.txt");
    }
};

// map in two halves (0-1 and 2-3) so some nodes cant be reached
class SplitMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        writeFile("split.txt", "Split_Map\n4\n0 3 0 0\n3 0 0 0\n0 0 0 4\n0 0 4 0\n");
    }

    void TearDown() override
    {
        remove("split.txt");
    }
};

// CollegeMap tests

// CM1: a new map should be empty
TEST(CollegeMapTest, DefaultMapIsEmpty)
{
    CollegeMap map;
    EXPECT_FALSE(map.isLoaded());
    EXPECT_EQ(0, map.getNodesTotal());
    EXPECT_EQ("None", map.getName());
}

// CM1: an empty map still has to copy, assign and print ok
TEST(CollegeMapTest, EmptyMapCanBeCopiedAndPrinted)
{
    CollegeMap empty;
    CollegeMap copy = empty;
    CollegeMap assigned;
    assigned = empty;
    EXPECT_FALSE(copy.isLoaded());
    EXPECT_FALSE(assigned.isLoaded());

    ofstream fout("empty_out.txt");
    fout << empty;
    fout.close();
    EXPECT_EQ("Map: (no map loaded)", readWords("empty_out.txt"));
    remove("empty_out.txt");
}

// CM2: file that doesnt exist
TEST(CollegeMapTest, MissingFileFailsToLoad)
{
    CollegeMap map;
    EXPECT_FALSE(map.load("does_not_exist.txt"));
    EXPECT_FALSE(map.isLoaded());
}

// CM2: none of these bad files should load
TEST(CollegeMapTest, BadFilesFailToLoad)
{
    CollegeMap map;

    writeFile("bad.txt", "Bad_Map\nthree\n0 5\n5 0\n");     // word instead of number
    EXPECT_FALSE(map.load("bad.txt"));

    writeFile("bad.txt", "Bad_Map\n3\n0 5 0\n5 0 2\n0 2\n"); // number missing
    EXPECT_FALSE(map.load("bad.txt"));

    writeFile("bad.txt", "Bad_Map\n2\n0 -5\n-5 0\n");        // negative
    EXPECT_FALSE(map.load("bad.txt"));

    writeFile("bad.txt", "Bad_Map\n2\n1 5\n5 0\n");          // diagonal should be 0
    EXPECT_FALSE(map.load("bad.txt"));

    remove("bad.txt");
}

// CM3, CM4
TEST_F(ThreeNodeMapTest, LoadingValidFileReportsThreeNodes)
{
    CollegeMap map("tiny.txt");
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(3, map.getNodesTotal());
    EXPECT_EQ("Tiny_Map", map.getName());
}

// CM3, CM5: weights should match the file
TEST_F(ThreeNodeMapTest, GetWeightReturnsValuesFromFile)
{
    CollegeMap map("tiny.txt");
    EXPECT_EQ(5, map.getWeight(0, 1));
    EXPECT_EQ(2, map.getWeight(2, 1));
    EXPECT_EQ(0, map.getWeight(0, 2));
}

// CM5: nodes outside the map
TEST_F(ThreeNodeMapTest, OutOfRangeNodesAreRejected)
{
    CollegeMap map("tiny.txt");
    EXPECT_TRUE(map.isValidNode(2));
    EXPECT_FALSE(map.isValidNode(3));
    EXPECT_FALSE(map.isValidNode(-1));
    EXPECT_EQ(-1, map.getWeight(0, 3));
    EXPECT_EQ(-1, map.getWeight(-1, 0));
}

// CM2: MTU map isnt symmetric (3 to 5 is 14, 5 to 3 is 0) but it should still load
TEST_F(MtuMapTest, NonSymmetricMapStillLoads)
{
    CollegeMap map("mtu.txt");
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(9, map.getNodesTotal());
    EXPECT_EQ(14, map.getWeight(3, 5));
}

// CM5: setting a weight changes both ways
TEST_F(ThreeNodeMapTest, SetWeightChangesBothDirections)
{
    CollegeMap map("tiny.txt");
    EXPECT_TRUE(map.setWeight(0, 2, 9));
    EXPECT_EQ(9, map.getWeight(0, 2));
    EXPECT_EQ(9, map.getWeight(2, 0));
}

// CM5
TEST_F(ThreeNodeMapTest, SetWeightRejectsBadValues)
{
    CollegeMap map("tiny.txt");
    EXPECT_FALSE(map.setWeight(3, 0, 1));    // no node 3
    EXPECT_FALSE(map.setWeight(0, 1, -4));   // negative
    EXPECT_FALSE(map.setWeight(1, 1, 3));    // same node
    EXPECT_EQ(5, map.getWeight(0, 1));       // still the same
}

// CM4, CM6: print the map to a file and check it
TEST_F(ThreeNodeMapTest, DisplayShowsNameNodesAndMatrix)
{
    CollegeMap map("tiny.txt");
    ofstream fout("out.txt");
    fout << map;
    fout.close();
    EXPECT_EQ("Map: Tiny_Map Nodes: 3 0 1 2 0 0 5 0 1 5 0 2 2 0 2 0", readWords("out.txt"));
}

// CM7 copy constructor
TEST_F(ThreeNodeMapTest, CopyConstructorMakesIndependentCopy)
{
    CollegeMap original("tiny.txt");
    CollegeMap copy = original;
    EXPECT_TRUE(copy == original);

    // changing the copy shouldnt change the original
    copy.setWeight(0, 1, 99);
    EXPECT_EQ(5, original.getWeight(0, 1));
}

// CM7 assignment
TEST_F(ThreeNodeMapTest, AssignmentMakesIndependentCopy)
{
    CollegeMap original("tiny.txt");
    CollegeMap assigned;
    assigned = original;
    EXPECT_TRUE(assigned == original);

    // changing the original shouldnt change the assigned one
    original.setWeight(1, 2, 50);
    EXPECT_EQ(2, assigned.getWeight(1, 2));
}

// CM7: map = map
TEST_F(ThreeNodeMapTest, SelfAssignmentKeepsData)
{
    CollegeMap map("tiny.txt");
    map = map;
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(5, map.getWeight(0, 1));
}

// == and !=
TEST_F(ThreeNodeMapTest, MapsWithDifferentWeightsAreNotEqual)
{
    CollegeMap a("tiny.txt");
    CollegeMap b("tiny.txt");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);

    b.setWeight(0, 1, 6);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

// CM9: load a different size map into the same object
TEST_F(ThreeNodeMapTest, ReloadingReplacesOldMap)
{
    CollegeMap map("tiny.txt");
    writeFile("two.txt", "Two_Map\n2\n0 1\n1 0\n");
    EXPECT_TRUE(map.load("two.txt"));
    EXPECT_EQ(2, map.getNodesTotal());
    EXPECT_EQ("Two_Map", map.getName());
    remove("two.txt");
}

// Dijkstra tests

// D3: one node map, only node 0 to start from
TEST(DijkstraTest, OneNodeMapRunsFromNodeZero)
{
    writeFile("one.txt", "One_Map\n1\n0\n");
    CollegeMap map("one.txt");
    Dijkstra planner(map);
    EXPECT_TRUE(planner.run(0));
    EXPECT_EQ(0, planner.getDistance(0));
    remove("one.txt");
}

// D3, CM8: 0 to 1 is 5 and 0 to 2 is 5+2 = 7
TEST_F(ThreeNodeMapTest, DistancesFromNodeZero)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    EXPECT_TRUE(planner.run(0));
    EXPECT_EQ(0, planner.getDistance(0));
    EXPECT_EQ(5, planner.getDistance(1));
    EXPECT_EQ(7, planner.getDistance(2));
}

// D3: numbers from the sample output in the brief
TEST_F(MtuMapTest, DistancesFromNodeZeroMatchBrief)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(4, planner.getDistance(1));
    EXPECT_EQ(12, planner.getDistance(2));
    EXPECT_EQ(19, planner.getDistance(3));
    EXPECT_EQ(21, planner.getDistance(4));
    EXPECT_EQ(11, planner.getDistance(5));
    EXPECT_EQ(9, planner.getDistance(6));
    EXPECT_EQ(8, planner.getDistance(7));
    EXPECT_EQ(14, planner.getDistance(8));
}

// D3, CM8: distances from node 3
TEST_F(MtuMapTest, DistancesFromNodeThree)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(3);
    EXPECT_EQ(19, planner.getDistance(0));
    EXPECT_EQ(15, planner.getDistance(1));
    EXPECT_EQ(7, planner.getDistance(2));
    EXPECT_EQ(0, planner.getDistance(3));
    EXPECT_EQ(9, planner.getDistance(4));
    EXPECT_EQ(11, planner.getDistance(5));
    EXPECT_EQ(13, planner.getDistance(6));
    EXPECT_EQ(14, planner.getDistance(7));
    EXPECT_EQ(9, planner.getDistance(8));
}

// D3: run from 0 first, then 3, should give the node 3 answers
TEST_F(MtuMapTest, SecondRunUsesNewSource)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(0);
    planner.run(3);
    EXPECT_EQ(3, planner.getSource());
    EXPECT_EQ(0, planner.getDistance(3));
    EXPECT_EQ(19, planner.getDistance(0));
    EXPECT_EQ(14, planner.getDistance(7));
}

// D4: route from 3 to 7 should be 3 2 5 6 7
TEST_F(MtuMapTest, RouteFromNodeThreeToSeven)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(3);
    int route[9];
    EXPECT_EQ(5, planner.getRoute(7, route));
    EXPECT_EQ(3, route[0]);
    EXPECT_EQ(2, route[1]);
    EXPECT_EQ(5, route[2]);
    EXPECT_EQ(6, route[3]);
    EXPECT_EQ(7, route[4]);
}

// D4: printRoute prints to cout so CaptureStdout grabs it
TEST_F(MtuMapTest, PrintRouteShowsOneRoute)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(3);

    testing::internal::CaptureStdout();
    planner.printRoute(7);
    string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(string::npos, output.find("3 -> 2 -> 5 -> 6 -> 7"));
}

// D3: start node that doesnt exist
TEST_F(ThreeNodeMapTest, InvalidSourceIsRejected)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    EXPECT_FALSE(planner.run(-1));
    EXPECT_FALSE(planner.run(3));
    EXPECT_EQ(-1, planner.getSource());
}

// D3: no map loaded
TEST(DijkstraTest, RunOnEmptyMapFails)
{
    CollegeMap empty;
    Dijkstra planner(empty);
    EXPECT_FALSE(planner.run(0));
}

// D4: 2 and 3 cant be reached from 0
TEST_F(SplitMapTest, UnreachableNodeHasNoDistance)
{
    CollegeMap map("split.txt");
    Dijkstra planner(map);
    planner.run(0);

    EXPECT_EQ(3, planner.getDistance(1));
    EXPECT_TRUE(planner.isReachable(1));
    EXPECT_FALSE(planner.isReachable(3));
    EXPECT_EQ(-1, planner.getDistance(3));

    int route[4];
    EXPECT_EQ(0, planner.getRoute(3, route));
}

// D4: all routes from node 2
TEST_F(ThreeNodeMapTest, PrintAllFromNodeTwo)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    planner.run(2);
    ofstream fout("out.txt");
    fout << planner;
    fout.close();
    EXPECT_EQ("Shortest routes from node 2 Destination Distance Route 0 7 2 -> 1 -> 0 1 2 2 -> 1",
        readWords("out.txt"));
}

// D5
TEST_F(ThreeNodeMapTest, SaveResultsWritesMapAndRoutes)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_TRUE(planner.saveResults("out.txt"));
    EXPECT_EQ("Map: Tiny_Map Nodes: 3 0 1 2 0 0 5 0 1 5 0 2 2 0 2 0 "
        "Shortest routes from node 0 Destination Distance Route 1 5 0 -> 1 2 7 0 -> 1 -> 2",
        readWords("out.txt"));
}

// D1, D2: change the map and Dijkstra should pick it up
TEST_F(ThreeNodeMapTest, DijkstraSeesChangesToMap)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(7, planner.getDistance(2));

    map.setWeight(0, 2, 1);     // direct path from 0 to 2
    planner.run(0);
    EXPECT_EQ(1, planner.getDistance(2));
}

// D2: load a bigger map after making the Dijkstra object
TEST_F(ThreeNodeMapTest, RunAfterMapReloadedToNewSize)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);

    writeFile("big.txt", "Big_Map\n4\n0 1 0 0\n1 0 1 0\n0 1 0 1\n0 0 1 0\n");
    map.load("big.txt");
    EXPECT_TRUE(planner.run(0));
    EXPECT_EQ(3, planner.getDistance(3));
    remove("big.txt");
}

// D1, D7: map still works after planner goes out of scope
TEST_F(ThreeNodeMapTest, MapSurvivesDijkstraDestruction)
{
    CollegeMap map("tiny.txt");
    {
        Dijkstra planner(map);
        planner.run(0);
    }
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(5, map.getWeight(0, 1));
}

// tests for the bugs in the reference code

// bug 1: routes were only right when starting from node 0
TEST_F(MtuMapTest, RouteFromNonZeroSourceStartsAtSource)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(3);
    int route[9];
    EXPECT_EQ(2, planner.getRoute(2, route));
    EXPECT_EQ(3, route[0]);
    EXPECT_EQ(2, route[1]);
}

// bug 2: nodes it couldnt reach got a weird negative distance
TEST_F(SplitMapTest, UnreachableNodeDoesNotGoNegative)
{
    CollegeMap map("split.txt");
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(-1, planner.getDistance(2));
    EXPECT_EQ(-1, planner.getDistance(3));

    planner.run(2);             // other half should still work
    EXPECT_EQ(4, planner.getDistance(3));
}

// bug 3: start node was always printed as 0
TEST_F(MtuMapTest, PrintAllShowsRealSource)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(6);
    ofstream fout("mtu_out.txt");
    fout << planner;
    fout.close();

    ifstream fin("mtu_out.txt");
    string word;
    fin >> word >> word >> word >> word >> word;   // 5th word is the node number
    fin.close();
    EXPECT_EQ("6", word);
    remove("mtu_out.txt");
}