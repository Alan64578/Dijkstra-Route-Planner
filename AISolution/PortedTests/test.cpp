#include "pch.h"
#include <cstdio>
#include <fstream>
#include <stdexcept>
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

// load ok
bool loadOk(CollegeMap& map, string fileName)
{
    return map.load(fileName) == CollegeMap::LoadStatus::Ok;
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
    EXPECT_EQ(0, empty.nodes());
    EXPECT_EQ("", empty.name());

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
    EXPECT_FALSE(loadOk(map, "missing.txt"));

    writeFile("bad.txt", "Bad_Map\nthree\n0 5\n5 0\n");     // non-numeric count
    EXPECT_FALSE(loadOk(map, "bad.txt"));

    writeFile("bad.txt", "Bad_Map\n3\n0 5 0\n5 0 2\n0 2\n"); // too few values
    EXPECT_FALSE(loadOk(map, "bad.txt"));

    writeFile("bad.txt", "Bad_Map\n2\n0 -5\n-5 0\n");        // negative weight
    EXPECT_FALSE(loadOk(map, "bad.txt"));

    writeFile("bad.txt", "Bad_Map\n2\n1 5\n5 0\n");          // non-zero diagonal
    EXPECT_FALSE(loadOk(map, "bad.txt"));

    EXPECT_FALSE(map.isLoaded());   // left empty
    remove("bad.txt");
}

// CM3, CM4
TEST_F(TinyMapTest, LoadsNameNodesAndWeights)
{
    CollegeMap map("tiny.txt");
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ("Tiny_Map", map.name());
    EXPECT_EQ(3, map.nodes());
    EXPECT_EQ(5, map.weight(0, 1));
    EXPECT_EQ(2, map.weight(2, 1));
    EXPECT_EQ(0, map.weight(0, 2));
}

// CM5
TEST_F(TinyMapTest, InvalidIndicesAndWeightsRejected)
{
    CollegeMap map("tiny.txt");
    EXPECT_NO_THROW(map.weight(2, 0));
    EXPECT_THROW(map.weight(3, 0), std::out_of_range);
    EXPECT_THROW(map.weight(-1, 0), std::out_of_range);
    EXPECT_THROW(map.weight(0, 3), std::out_of_range);

    EXPECT_THROW(map.setWeight(3, 0, 1), std::out_of_range);      // no node 3
    EXPECT_THROW(map.setWeight(0, 1, -4), std::invalid_argument); // negative
    EXPECT_THROW(map.setWeight(1, 1, 3), std::invalid_argument);  // same node
    EXPECT_EQ(5, map.weight(0, 1));                               // unchanged
}

// CM5
TEST_F(TinyMapTest, SetWeightChangesBothDirections)
{
    CollegeMap map("tiny.txt");
    EXPECT_NO_THROW(map.setWeight(0, 2, 9));
    EXPECT_EQ(9, map.weight(0, 2));
    EXPECT_EQ(9, map.weight(2, 0));
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
    EXPECT_EQ(5, original.weight(0, 1));
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
    EXPECT_EQ(2, assigned.weight(1, 2));
}

// CM7
TEST_F(TinyMapTest, SelfAssignmentKeepsData)
{
    CollegeMap map("tiny.txt");
    map = map;
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(5, map.weight(0, 1));
}

// CM9
TEST_F(TinyMapTest, ReloadReplacesOldMap)
{
    CollegeMap map("tiny.txt");
    writeFile("two.txt", "Two_Map\n2\n0 1\n1 0\n");
    EXPECT_TRUE(loadOk(map, "two.txt"));
    EXPECT_EQ(2, map.nodes());
    EXPECT_EQ("Two_Map", map.name());
    remove("two.txt");
}

// ---------- Dijkstra ----------

// D3
TEST(DijkstraTest, OneNodeMapRuns)
{
    writeFile("one.txt", "One_Map\n1\n0\n");
    CollegeMap map("one.txt");
    Dijkstra planner(map);
    EXPECT_NO_THROW(planner.run(0));
    EXPECT_EQ(0, planner.distanceTo(0));
    remove("one.txt");
}

// D3, CM8
TEST_F(MtuMapTest, DistancesFromNodeZeroMatchBrief)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    EXPECT_NO_THROW(planner.run(0));
    if (planner.hasRun())
    {
        EXPECT_EQ(4, planner.distanceTo(1));
        EXPECT_EQ(12, planner.distanceTo(2));
        EXPECT_EQ(19, planner.distanceTo(3));
        EXPECT_EQ(21, planner.distanceTo(4));
        EXPECT_EQ(11, planner.distanceTo(5));
        EXPECT_EQ(9, planner.distanceTo(6));
        EXPECT_EQ(8, planner.distanceTo(7));
        EXPECT_EQ(14, planner.distanceTo(8));
    }
}

// D3: rerun, new source
TEST_F(MtuMapTest, RerunFromNodeThree)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    EXPECT_NO_THROW(planner.run(0));
    EXPECT_NO_THROW(planner.run(3));
    EXPECT_EQ(3, planner.source());
    if (planner.hasRun())
    {
        EXPECT_EQ(19, planner.distanceTo(0));
        EXPECT_EQ(15, planner.distanceTo(1));
        EXPECT_EQ(7, planner.distanceTo(2));
        EXPECT_EQ(0, planner.distanceTo(3));
        EXPECT_EQ(9, planner.distanceTo(4));
        EXPECT_EQ(11, planner.distanceTo(5));
        EXPECT_EQ(13, planner.distanceTo(6));
        EXPECT_EQ(14, planner.distanceTo(7));
        EXPECT_EQ(9, planner.distanceTo(8));
    }
}

// D4, bug 1: wrong source parent
TEST_F(MtuMapTest, RouteFromNodeThreeToSeven)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    EXPECT_NO_THROW(planner.run(3));
    if (planner.hasRun())
    {
        EXPECT_EQ("3 -> 2 -> 5 -> 6 -> 7", planner.routeTo(7));

        EXPECT_THROW(planner.routeTo(9), std::out_of_range);    // invalid
        EXPECT_EQ(14, planner.distanceTo(7));
    }
}

// D3
TEST_F(TinyMapTest, InvalidSourceAndEmptyMapRejected)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    EXPECT_THROW(planner.run(-1), std::out_of_range);
    EXPECT_THROW(planner.run(3), std::out_of_range);
    EXPECT_EQ(-1, planner.source());

    CollegeMap empty;
    Dijkstra planner2(empty);
    EXPECT_THROW(planner2.run(0), std::logic_error);
}

// D4, bug 2: unreachable went negative
TEST_F(SplitMapTest, UnreachableNodeReported)
{
    CollegeMap map("split.txt");
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_TRUE(planner.isReachable(1));
    EXPECT_EQ(3, planner.distanceTo(1));
    EXPECT_FALSE(planner.isReachable(3));
    EXPECT_EQ(-1, planner.distanceTo(2));
    EXPECT_EQ(-1, planner.distanceTo(3));

    EXPECT_EQ("unreachable", planner.routeTo(3));

    planner.run(2);             // other half works
    EXPECT_EQ(4, planner.distanceTo(3));
}

// D4, bug 3: source printed as 0
TEST_F(TinyMapTest, PrintAllShowsRealSource)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    planner.run(2);
    ofstream fout("out.txt");
    planner.writeResults(fout);
    fout.close();
    EXPECT_EQ("Shortest routes from node 2 Destination Distance Route 0 7 2 -> 1 -> 0 1 2 2 -> 1",
        readWords("out.txt"));
}

// D5
TEST_F(TinyMapTest, SaveResultsWritesMapAndRoutes)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
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
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(7, planner.distanceTo(2));

    map.setWeight(0, 2, 1);     // direct 0-2 edge
    planner.run(0);
    EXPECT_EQ(1, planner.distanceTo(2));

    writeFile("big.txt", "Big_Map\n4\n0 1 0 0\n1 0 1 0\n0 1 0 1\n0 0 1 0\n");
    map.load("big.txt");        // bigger map
    EXPECT_NO_THROW(planner.run(0));
    EXPECT_EQ(3, planner.distanceTo(3));
    remove("big.txt");
}

// D1, D7
TEST_F(TinyMapTest, MapSurvivesDijkstraDestruction)
{
    CollegeMap map("tiny.txt");
    Dijkstra* planner = new Dijkstra(map);
    planner->run(0);
    delete planner;     // map untouched
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(5, map.weight(0, 1));
}