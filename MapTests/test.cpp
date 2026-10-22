#include "pch.h"
#include <cstdio>
#include <fstream>
#include <string>
#include "CollegeMap.h"
#include "Dijkstra.h"
using namespace std;

// write a test file
void writeFile(string fileName, string contents)
{
    ofstream fout(fileName);
    fout << contents;
    fout.close();
}

// read file as words
string readWords(string fileName)
{
    ifstream fin(fileName);
    string word, text;
    while (fin >> word)
    {
        if (text.length() > 0)
            text += " ";
        text += word;
    }
    return text;
}

// 3 node map, Appendix 4
class TinyMapTest : public ::testing::Test
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

// MTU map, Appendix 3
class MtuMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        string contents = "MTU_Map\n9\n";
        contents += "0 4 0 0 0 0 0 8 0\n";
        contents += "4 0 8 0 0 0 0 11 0\n";
        contents += "0 8 0 7 0 4 0 0 2\n";
        contents += "0 0 7 0 9 14 0 0 0\n";
        contents += "0 0 0 9 0 10 0 0 0\n";
        contents += "0 0 4 0 10 0 2 0 0\n";
        contents += "0 0 0 14 0 2 0 1 6\n";
        contents += "8 11 0 0 0 0 1 0 7\n";
        contents += "0 0 2 0 0 0 6 7 0\n";
        writeFile("mtu.txt", contents);
    }

    void TearDown() override
    {
        remove("mtu.txt");
    }
};

// two halves, some unreachable
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

// ---------- CollegeMap ----------

// CM1
TEST(CollegeMapTest, EmptyMapIsSafeToUse)
{
    CollegeMap empty;
    EXPECT_FALSE(empty.isLoaded());
    EXPECT_EQ(0, empty.getNodesTotal());
    EXPECT_EQ("None", empty.getName());

    CollegeMap copy = empty;
    CollegeMap assigned;
    assigned = empty;
    EXPECT_FALSE(copy.isLoaded());
    EXPECT_FALSE(assigned.isLoaded());

    ofstream fout("empty.txt");
    fout << empty;
    fout.close();
    EXPECT_EQ("Map: (no map loaded)", readWords("empty.txt"));
    remove("empty.txt");
}

// CM2
TEST(CollegeMapTest, MissingAndMalformedFilesFailToLoad)
{
    CollegeMap map;
    EXPECT_FALSE(map.load("missing.txt"));

    writeFile("bad.txt", "Bad_Map\nthree\n0 5\n5 0\n");     // non-numeric count
    EXPECT_FALSE(map.load("bad.txt"));

    writeFile("bad.txt", "Bad_Map\n3\n0 5 0\n5 0 2\n0 2\n"); // too few values
    EXPECT_FALSE(map.load("bad.txt"));

    writeFile("bad.txt", "Bad_Map\n2\n0 -5\n-5 0\n");        // negative weight
    EXPECT_FALSE(map.load("bad.txt"));

    writeFile("bad.txt", "Bad_Map\n2\n1 5\n5 0\n");          // non-zero diagonal
    EXPECT_FALSE(map.load("bad.txt"));

    EXPECT_FALSE(map.isLoaded());   // left empty
    remove("bad.txt");
}

// CM3, CM4
TEST_F(TinyMapTest, LoadsNameNodesAndWeights)
{
    CollegeMap map("tiny.txt");
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ("Tiny_Map", map.getName());
    EXPECT_EQ(3, map.getNodesTotal());
    EXPECT_EQ(5, map.getWeight(0, 1));
    EXPECT_EQ(2, map.getWeight(2, 1));
    EXPECT_EQ(0, map.getWeight(0, 2));
}

// CM5
TEST_F(TinyMapTest, InvalidIndicesAndWeightsRejected)
{
    CollegeMap map("tiny.txt");
    EXPECT_TRUE(map.isValidNode(2));
    EXPECT_FALSE(map.isValidNode(3));
    EXPECT_FALSE(map.isValidNode(-1));
    EXPECT_EQ(-1, map.getWeight(0, 3));

    EXPECT_FALSE(map.setWeight(3, 0, 1));    // no node 3
    EXPECT_FALSE(map.setWeight(0, 1, -4));   // negative
    EXPECT_FALSE(map.setWeight(1, 1, 3));    // same node
    EXPECT_EQ(5, map.getWeight(0, 1));       // unchanged
}

// CM5
TEST_F(TinyMapTest, SetWeightChangesBothDirections)
{
    CollegeMap map("tiny.txt");
    EXPECT_TRUE(map.setWeight(0, 2, 9));
    EXPECT_EQ(9, map.getWeight(0, 2));
    EXPECT_EQ(9, map.getWeight(2, 0));
}

// CM4, CM6
TEST_F(TinyMapTest, DisplayShowsNameNodesAndMatrix)
{
    CollegeMap map("tiny.txt");
    ofstream fout("out.txt");
    fout << map;
    fout.close();
    EXPECT_EQ("Map: Tiny_Map Nodes: 3 0 1 2 0 0 5 0 1 5 0 2 2 0 2 0", readWords("out.txt"));
}

// CM7
TEST_F(TinyMapTest, CopyIsIndependent)
{
    CollegeMap original("tiny.txt");
    CollegeMap copy = original;
    EXPECT_TRUE(copy == original);

    copy.setWeight(0, 1, 99);
    EXPECT_EQ(5, original.getWeight(0, 1));
    EXPECT_FALSE(copy == original);
}

// CM7
TEST_F(TinyMapTest, AssignmentIsIndependent)
{
    CollegeMap original("tiny.txt");
    CollegeMap assigned;
    assigned = original;
    EXPECT_TRUE(assigned == original);

    original.setWeight(1, 2, 50);
    EXPECT_EQ(2, assigned.getWeight(1, 2));
}

// CM7
TEST_F(TinyMapTest, SelfAssignmentKeepsData)
{
    CollegeMap map("tiny.txt");
    map = map;
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(5, map.getWeight(0, 1));
}

// CM9
TEST_F(TinyMapTest, ReloadReplacesOldMap)
{
    CollegeMap map("tiny.txt");
    writeFile("two.txt", "Two_Map\n2\n0 1\n1 0\n");
    EXPECT_TRUE(map.load("two.txt"));
    EXPECT_EQ(2, map.getNodesTotal());
    EXPECT_EQ("Two_Map", map.getName());
    remove("two.txt");
}

// ---------- Dijkstra ----------

// D3
TEST(DijkstraTest, OneNodeMapRuns)
{
    writeFile("one.txt", "One_Map\n1\n0\n");
    CollegeMap map("one.txt");
    Dijkstra planner(&map);
    EXPECT_TRUE(planner.run(0));
    EXPECT_EQ(0, planner.getDistance(0));
    remove("one.txt");
}

// D3, CM8
TEST_F(MtuMapTest, DistancesFromNodeZeroMatchBrief)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(&map);
    ASSERT_TRUE(planner.run(0));
    EXPECT_EQ(4, planner.getDistance(1));
    EXPECT_EQ(12, planner.getDistance(2));
    EXPECT_EQ(19, planner.getDistance(3));
    EXPECT_EQ(21, planner.getDistance(4));
    EXPECT_EQ(11, planner.getDistance(5));
    EXPECT_EQ(9, planner.getDistance(6));
    EXPECT_EQ(8, planner.getDistance(7));
    EXPECT_EQ(14, planner.getDistance(8));
}

// D3: rerun, new source
TEST_F(MtuMapTest, RerunFromNodeThree)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(&map);
    planner.run(0);
    planner.run(3);
    EXPECT_EQ(3, planner.getSource());
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

// D4, bug 1: wrong source parent
TEST_F(MtuMapTest, RouteFromNodeThreeToSeven)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(&map);
    planner.run(3);
    int route[9];
    ASSERT_EQ(5, planner.getRoute(7, route));
    EXPECT_EQ(3, route[0]);
    EXPECT_EQ(2, route[1]);
    EXPECT_EQ(5, route[2]);
    EXPECT_EQ(6, route[3]);
    EXPECT_EQ(7, route[4]);

    planner.printRoute(7);      // valid
    planner.printRoute(9);      // invalid
    EXPECT_EQ(14, planner.getDistance(7));
}

// D3
TEST_F(TinyMapTest, InvalidSourceAndEmptyMapRejected)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(&map);
    EXPECT_FALSE(planner.run(-1));
    EXPECT_FALSE(planner.run(3));
    EXPECT_EQ(-1, planner.getSource());

    CollegeMap empty;
    Dijkstra planner2(&empty);
    EXPECT_FALSE(planner2.run(0));
}

// D4, bug 2: unreachable went negative
TEST_F(SplitMapTest, UnreachableNodeReported)
{
    CollegeMap map("split.txt");
    Dijkstra planner(&map);
    planner.run(0);
    EXPECT_TRUE(planner.isReachable(1));
    EXPECT_EQ(3, planner.getDistance(1));
    EXPECT_FALSE(planner.isReachable(3));
    EXPECT_EQ(-1, planner.getDistance(2));
    EXPECT_EQ(-1, planner.getDistance(3));

    int route[4];
    EXPECT_EQ(0, planner.getRoute(3, route));

    planner.run(2);             // other half works
    EXPECT_EQ(4, planner.getDistance(3));
}

// D4, bug 3: source printed as 0
TEST_F(TinyMapTest, PrintAllShowsRealSource)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(&map);
    planner.run(2);
    ofstream fout("out.txt");
    fout << planner;
    fout.close();
    EXPECT_EQ("Shortest routes from node 2 Destination Distance Route 0 7 2 -> 1 -> 0 1 2 2 -> 1",
        readWords("out.txt"));
}

// D5
TEST_F(TinyMapTest, SaveResultsWritesMapAndRoutes)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(&map);
    planner.run(0);
    EXPECT_TRUE(planner.saveResults("out.txt"));
    string expected = "Map: Tiny_Map Nodes: 3 0 1 2 0 0 5 0 1 5 0 2 2 0 2 0 ";
    expected += "Shortest routes from node 0 Destination Distance Route 1 5 0 -> 1 2 7 0 -> 1 -> 2";
    EXPECT_EQ(expected, readWords("out.txt"));
}

// D1, D2: sees map changes
TEST_F(TinyMapTest, DijkstraSeesMapChanges)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(&map);
    planner.run(0);
    EXPECT_EQ(7, planner.getDistance(2));

    map.setWeight(0, 2, 1);     // direct 0-2 edge
    planner.run(0);
    EXPECT_EQ(1, planner.getDistance(2));

    writeFile("big.txt", "Big_Map\n4\n0 1 0 0\n1 0 1 0\n0 1 0 1\n0 0 1 0\n");
    map.load("big.txt");        // bigger map
    EXPECT_TRUE(planner.run(0));
    EXPECT_EQ(3, planner.getDistance(3));
    remove("big.txt");
}

// D1, D7
TEST_F(TinyMapTest, MapSurvivesDijkstraDestruction)
{
    CollegeMap map("tiny.txt");
    Dijkstra* planner = new Dijkstra(&map);
    planner->run(0);
    delete planner;     // map untouched
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(5, map.getWeight(0, 1));
}