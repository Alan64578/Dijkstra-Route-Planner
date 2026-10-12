#include "pch.h"
#include <cstdio>
#include <fstream>
#include <string>
#include "CollegeMap.h"
#include "Dijkstra.h"
using namespace std;

// ---------- helpers ----------

// write a test file
void writeFile(string fileName, string contents)
{
    ofstream fout(fileName.c_str());
    fout << contents;
    fout.close();
}

// read a file back as words separated by single spaces
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

// ---------- fixtures ----------

// Appendix 4 fixture: 0 -5- 1 -2- 2
class ThreeNodeMapTest : public ::testing::Test
{
protected:
    const string fileName = "test_three_node.txt";
    const string outName = "test_three_out.txt";

    void SetUp() override
    {
        writeFile(fileName, "Tiny_Map\n3\n0 5 0\n5 0 2\n0 2 0\n");
    }

    void TearDown() override
    {
        remove(fileName.c_str());
        remove(outName.c_str());
    }
};

// MTU map from Appendix 3
class MtuMapTest : public ::testing::Test
{
protected:
    const string fileName = "test_mtu.txt";
    const string outName = "test_mtu_out.txt";

    void SetUp() override
    {
        ofstream fout(fileName.c_str());
        fout << "MTU_Map\n9\n"
             << "0 4 0 0 0 0 0 8 0\n"
             << "4 0 8 0 0 0 0 11 0\n"
             << "0 8 0 7 0 4 0 0 2\n"
             << "0 0 7 0 9 14 0 0 0\n"
             << "0 0 0 9 0 10 0 0 0\n"
             << "0 0 4 0 10 0 2 0 0\n"
             << "0 0 0 14 0 2 0 1 6\n"
             << "8 11 0 0 0 0 1 0 7\n"
             << "0 0 2 0 0 0 6 7 0\n";
        fout.close();
    }

    void TearDown() override
    {
        remove(fileName.c_str());
        remove(outName.c_str());
    }
};

// two separate parts: 0-1 and 2-3, no edge between them
class UnreachableMapTest : public ::testing::Test
{
protected:
    const string fileName = "test_unreachable.txt";
    const string outName = "test_unreachable_out.txt";

    void SetUp() override
    {
        ofstream fout(fileName.c_str());
        fout << "Split_Map\n4\n"
             << "0 3 0 0\n"
             << "3 0 0 0\n"
             << "0 0 0 4\n"
             << "0 0 4 0\n";
        fout.close();
    }

    void TearDown() override
    {
        remove(fileName.c_str());
        remove(outName.c_str());
    }
};

// bad files, each test writes its own
class MalformedFileTest : public ::testing::Test
{
protected:
    const string fileName = "test_bad.txt";

    void TearDown() override
    {
        remove(fileName.c_str());
    }
};

// ================= CollegeMap =================

// CM1
TEST(CollegeMapTest, DefaultMapIsEmpty)
{
    CollegeMap map;
    EXPECT_FALSE(map.isLoaded());
    EXPECT_EQ(0, map.getNodesTotal());
    EXPECT_EQ("None", map.getName());
}

// CM1
TEST(CollegeMapTest, EmptyMapCanBeCopiedAndAssigned)
{
    CollegeMap empty;
    CollegeMap copy = empty;
    CollegeMap assigned;
    assigned = empty;
    EXPECT_FALSE(copy.isLoaded());
    EXPECT_FALSE(assigned.isLoaded());
    EXPECT_TRUE(copy == empty);
}

// CM1, CM6
TEST(CollegeMapTest, EmptyMapDisplaysNoMapMessage)
{
    CollegeMap map;
    ofstream fout("test_empty_out.txt");
    fout << map;
    fout.close();
    EXPECT_EQ("Map: (no map loaded)", readWords("test_empty_out.txt"));
    remove("test_empty_out.txt");
}

// CM2
TEST(CollegeMapTest, MissingFileFailsToLoad)
{
    CollegeMap map;
    EXPECT_FALSE(map.load("does_not_exist.txt"));
    EXPECT_FALSE(map.isLoaded());
}

// CM3, CM4
TEST_F(ThreeNodeMapTest, LoadingValidFileReportsThreeNodes)
{
    CollegeMap map(fileName);
    ASSERT_TRUE(map.isLoaded());
    EXPECT_EQ(3, map.getNodesTotal());
    EXPECT_EQ("Tiny_Map", map.getName());
}

// CM4, CM5 - getters work through a const reference
TEST_F(ThreeNodeMapTest, ConstMapCanStillBeRead)
{
    CollegeMap map(fileName);
    const CollegeMap &constMap = map;
    EXPECT_TRUE(constMap.isLoaded());
    EXPECT_EQ("Tiny_Map", constMap.getName());
    EXPECT_EQ(3, constMap.getNodesTotal());
    EXPECT_EQ(5, constMap.getWeight(0, 1));
    EXPECT_TRUE(constMap.isValidNode(2));
    EXPECT_TRUE(constMap == map);
}

// CM3, CM5
TEST_F(ThreeNodeMapTest, GetWeightReturnsValuesFromFile)
{
    CollegeMap map(fileName);
    EXPECT_EQ(5, map.getWeight(0, 1));
    EXPECT_EQ(2, map.getWeight(2, 1));
    EXPECT_EQ(0, map.getWeight(0, 2));
    EXPECT_EQ(0, map.getWeight(1, 1));
}

// CM2
TEST_F(MalformedFileTest, NonNumericNodeCountFails)
{
    writeFile(fileName, "Bad_Map\nthree\n0 5\n5 0\n");
    CollegeMap map;
    EXPECT_FALSE(map.load(fileName));
    EXPECT_FALSE(map.isLoaded());
}

// CM2
TEST_F(MalformedFileTest, ZeroNodeCountFails)
{
    writeFile(fileName, "Bad_Map\n0\n");
    CollegeMap map;
    EXPECT_FALSE(map.load(fileName));
}

// CM2
TEST_F(MalformedFileTest, TooFewValuesFails)
{
    writeFile(fileName, "Bad_Map\n3\n0 5 0\n5 0 2\n0 2\n");
    CollegeMap map;
    EXPECT_FALSE(map.load(fileName));
    EXPECT_EQ(0, map.getNodesTotal());
}

// CM2
TEST_F(MalformedFileTest, NegativeWeightFails)
{
    writeFile(fileName, "Bad_Map\n2\n0 -5\n-5 0\n");
    CollegeMap map;
    EXPECT_FALSE(map.load(fileName));
}

// CM2
TEST_F(MalformedFileTest, NonZeroDiagonalFails)
{
    writeFile(fileName, "Bad_Map\n2\n1 5\n5 0\n");
    CollegeMap map;
    EXPECT_FALSE(map.load(fileName));
}

// CM2, CM9 - a failed reload must not leave the old map behind
TEST_F(ThreeNodeMapTest, FailedReloadLeavesMapEmpty)
{
    CollegeMap map(fileName);
    ASSERT_TRUE(map.isLoaded());
    EXPECT_FALSE(map.load("does_not_exist.txt"));
    EXPECT_FALSE(map.isLoaded());
    EXPECT_EQ(-1, map.getWeight(0, 1));
}

// CM2 - decision: not symmetric is only a warning (MTU map isn't symmetric)
TEST_F(MtuMapTest, NonSymmetricMtuMapStillLoads)
{
    CollegeMap map(fileName);
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(9, map.getNodesTotal());
    EXPECT_EQ(14, map.getWeight(3, 5));
    EXPECT_EQ(0, map.getWeight(5, 3));
}

// CM5
TEST_F(ThreeNodeMapTest, SetWeightChangesBothDirections)
{
    CollegeMap map(fileName);
    EXPECT_TRUE(map.setWeight(0, 2, 9));
    EXPECT_EQ(9, map.getWeight(0, 2));
    EXPECT_EQ(9, map.getWeight(2, 0));
}

// CM5 - boundary indices
TEST_F(ThreeNodeMapTest, OutOfRangeIndexIsRejected)
{
    CollegeMap map(fileName);
    EXPECT_EQ(-1, map.getWeight(-1, 0));
    EXPECT_EQ(-1, map.getWeight(0, 3));
    EXPECT_FALSE(map.setWeight(3, 0, 1));
    EXPECT_FALSE(map.setWeight(0, -1, 1));
    EXPECT_EQ(5, map.getWeight(0, 1));      // nothing changed
}

// CM5
TEST_F(ThreeNodeMapTest, NegativeWeightIsRejected)
{
    CollegeMap map(fileName);
    EXPECT_FALSE(map.setWeight(0, 1, -4));
    EXPECT_EQ(5, map.getWeight(0, 1));
}

// CM5
TEST_F(ThreeNodeMapTest, SelfLoopWeightIsRejected)
{
    CollegeMap map(fileName);
    EXPECT_FALSE(map.setWeight(1, 1, 3));
    EXPECT_EQ(0, map.getWeight(1, 1));
}

// CM6 - checks << output word by word
TEST_F(ThreeNodeMapTest, DisplayShowsNameNodesAndMatrix)
{
    CollegeMap map(fileName);
    ofstream fout(outName.c_str());
    fout << map;
    fout.close();
    EXPECT_EQ("Map: Tiny_Map Nodes: 3 0 1 2 0 0 5 0 1 5 0 2 2 0 2 0", readWords(outName));
}

// CM7
TEST_F(ThreeNodeMapTest, CopyConstructorMakesIndependentCopy)
{
    CollegeMap original(fileName);
    CollegeMap copy = original;
    EXPECT_TRUE(copy == original);

    copy.setWeight(0, 1, 99);
    EXPECT_EQ(99, copy.getWeight(0, 1));
    EXPECT_EQ(5, original.getWeight(0, 1));
    EXPECT_TRUE(copy != original);
}

// CM7
TEST_F(ThreeNodeMapTest, AssignmentMakesIndependentCopy)
{
    CollegeMap original(fileName);
    CollegeMap assigned;
    assigned = original;
    EXPECT_TRUE(assigned == original);

    original.setWeight(1, 2, 50);
    EXPECT_EQ(2, assigned.getWeight(1, 2));
}

// CM7
TEST_F(ThreeNodeMapTest, SelfAssignmentKeepsData)
{
    CollegeMap map(fileName);
    CollegeMap &sameMap = map;
    map = sameMap;
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(3, map.getNodesTotal());
    EXPECT_EQ(5, map.getWeight(0, 1));
}

// CM7 - a = b = c works because = returns a reference
TEST_F(ThreeNodeMapTest, ChainedAssignmentCopiesToAll)
{
    CollegeMap original(fileName);
    CollegeMap a, b;
    a = b = original;
    EXPECT_TRUE(a == original);
    EXPECT_TRUE(b == original);
}

// CM7, CM9 - assigning over a loaded map replaces it
TEST_F(MtuMapTest, AssignmentReplacesLargerMap)
{
    CollegeMap big(fileName);
    CollegeMap small;
    small = big;
    EXPECT_EQ(9, small.getNodesTotal());

    CollegeMap empty;
    small = empty;
    EXPECT_FALSE(small.isLoaded());
}

// == and != operators
TEST_F(ThreeNodeMapTest, MapsWithDifferentWeightsAreNotEqual)
{
    CollegeMap a(fileName);
    CollegeMap b(fileName);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);

    b.setWeight(0, 1, 6);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

// CM9 - reload a different size
TEST_F(ThreeNodeMapTest, ReloadingReplacesOldMap)
{
    CollegeMap map(fileName);
    writeFile("test_two_node.txt", "Two_Map\n2\n0 1\n1 0\n");
    EXPECT_TRUE(map.load("test_two_node.txt"));
    EXPECT_EQ(2, map.getNodesTotal());
    EXPECT_EQ("Two_Map", map.getName());
    remove("test_two_node.txt");
}

// ================= Dijkstra =================

// D3 - one node map, nothing else to reach
TEST(DijkstraTest, OneNodeMapRunsFromNodeZero)
{
    writeFile("test_one_node.txt", "One_Map\n1\n0\n");
    CollegeMap map("test_one_node.txt");
    Dijkstra planner(map);
    EXPECT_TRUE(planner.run(0));
    EXPECT_EQ(0, planner.getDistance(0));
    remove("test_one_node.txt");
}

// D3 - hand worked: 0->1 = 5, 0->1->2 = 5+2 = 7
TEST_F(ThreeNodeMapTest, DistancesFromNodeZero)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_TRUE(planner.run(0));
    EXPECT_EQ(0, planner.getDistance(0));
    EXPECT_EQ(5, planner.getDistance(1));
    EXPECT_EQ(7, planner.getDistance(2));
}

// D3 - matches the sample output in the brief
TEST_F(MtuMapTest, DistancesFromNodeZeroMatchBrief)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
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

// D3 - source 3, worked out by hand (see report)
TEST_F(MtuMapTest, DistancesFromNodeThree)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_TRUE(planner.run(3));
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

// D4 - full route from node 3 to 7: 3 -> 2 -> 5 -> 6 -> 7
TEST_F(MtuMapTest, RouteFromNodeThreeToSeven)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_TRUE(planner.run(3));
    int route[9];
    ASSERT_EQ(5, planner.getRoute(7, route));
    EXPECT_EQ(3, route[0]);
    EXPECT_EQ(2, route[1]);
    EXPECT_EQ(5, route[2]);
    EXPECT_EQ(6, route[3]);
    EXPECT_EQ(7, route[4]);
    EXPECT_EQ(14, planner.getDistance(7));
}

// D3 - running again with a new source gives the same as a new object
TEST_F(MtuMapTest, RepeatedRunsMatchFreshObject)
{
    CollegeMap map(fileName);
    Dijkstra reused(map);
    reused.run(0);
    reused.run(3);

    Dijkstra fresh(map);
    fresh.run(3);

    for (int i = 0; i < 9; i++)
        EXPECT_EQ(fresh.getDistance(i), reused.getDistance(i));
    EXPECT_EQ(3, reused.getSource());
}

// D3 - invalid source nodes
TEST_F(ThreeNodeMapTest, InvalidSourceIsRejected)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    EXPECT_FALSE(planner.run(-1));
    EXPECT_FALSE(planner.run(3));
    EXPECT_EQ(-1, planner.getSource());
    EXPECT_EQ(-1, planner.getDistance(1));
}

// D3 - no map loaded
TEST(DijkstraTest, RunOnEmptyMapFails)
{
    CollegeMap empty;
    Dijkstra planner(empty);
    EXPECT_FALSE(planner.run(0));
}

// D4 - asking for results before run()
TEST_F(ThreeNodeMapTest, NoResultsBeforeRun)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    int route[3];
    EXPECT_EQ(-1, planner.getDistance(1));
    EXPECT_FALSE(planner.isReachable(1));
    EXPECT_EQ(0, planner.getRoute(1, route));
}

// D4 - unreachable node is reported, not 2147483647
TEST_F(UnreachableMapTest, UnreachableNodeHasNoDistance)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_TRUE(planner.run(0));
    EXPECT_EQ(3, planner.getDistance(1));
    EXPECT_FALSE(planner.isReachable(2));
    EXPECT_FALSE(planner.isReachable(3));
    EXPECT_EQ(-1, planner.getDistance(3));
}

// D4 - unreachable nodes get no route and print as unreachable
TEST_F(UnreachableMapTest, UnreachableNodePrintedAsUnreachable)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(0);
    int route[4];
    EXPECT_EQ(0, planner.getRoute(3, route));

    ofstream fout(outName.c_str());
    fout << planner;
    fout.close();
    EXPECT_EQ("Shortest routes from node 0 Destination Distance Route 1 3 0 -> 1 2 - unreachable 3 - unreachable",
              readWords(outName));
}

// D4 - all routes, hand worked from node 2: 2->1 = 2, 2->1->0 = 7
TEST_F(ThreeNodeMapTest, PrintAllFromNodeTwo)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(2);
    ofstream fout(outName.c_str());
    fout << planner;
    fout.close();
    EXPECT_EQ("Shortest routes from node 2 Destination Distance Route 0 7 2 -> 1 -> 0 1 2 2 -> 1",
              readWords(outName));
}

// D5
TEST_F(ThreeNodeMapTest, SaveResultsWritesMapAndRoutes)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_TRUE(planner.saveResults(outName));
    string expected = "Map: Tiny_Map Nodes: 3 0 1 2 0 0 5 0 1 5 0 2 2 0 2 0 Shortest routes from node 0 Destination Distance Route 1 5 0 -> 1 2 7 0 -> 1 -> 2";
    EXPECT_EQ(expected, readWords(outName));
}

// D5
TEST_F(ThreeNodeMapTest, SaveBeforeRunFails)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    EXPECT_FALSE(planner.saveResults(outName));
}

// D1 - Dijkstra uses the map itself, not a copy
TEST_F(ThreeNodeMapTest, DijkstraSeesChangesToMap)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(7, planner.getDistance(2));

    map.setWeight(0, 2, 1);     // add a shortcut
    planner.run(0);
    EXPECT_EQ(1, planner.getDistance(2));
}

// D2 - map reloaded at a different size after Dijkstra was made
TEST_F(ThreeNodeMapTest, RunAfterMapReloadedToNewSize)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(0);

    writeFile("test_big.txt", "Big_Map\n4\n0 1 0 0\n1 0 1 0\n0 1 0 1\n0 0 1 0\n");
    ASSERT_TRUE(map.load("test_big.txt"));
    ASSERT_TRUE(planner.run(0));
    EXPECT_EQ(3, planner.getDistance(3));
    remove("test_big.txt");
}

// D1, D7 - map still works after the Dijkstra object is destroyed
TEST_F(ThreeNodeMapTest, MapSurvivesDijkstraDestruction)
{
    CollegeMap map(fileName);
    {
        Dijkstra planner(map);
        planner.run(0);
    }
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(3, map.getNodesTotal());
    EXPECT_EQ(5, map.getWeight(0, 1));
}

// ================= Reference code defects =================

// Defect: parent[0] = -1 instead of parent[src] (only works when src is 0)
TEST_F(MtuMapTest, RouteFromNonZeroSourceStartsAtSource)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(3);
    int route[9];
    ASSERT_EQ(2, planner.getRoute(2, route));
    EXPECT_EQ(3, route[0]);
    EXPECT_EQ(2, route[1]);
}

// Defect: printSolution always prints source as 0
TEST_F(MtuMapTest, PrintAllShowsRealSource)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(6);
    ofstream fout(outName.c_str());
    fout << planner;
    fout.close();
    ifstream fin(outName.c_str());
    string w1, w2, w3, w4, w5;
    fin >> w1 >> w2 >> w3 >> w4 >> w5;
    EXPECT_EQ("node", w4);
    EXPECT_EQ("6", w5);
}

// Defect: minDistance uses <= so it picks unreachable node 2 (INT_MAX),
// then INT_MAX + 4 overflows and node 3 gets a negative distance
TEST_F(UnreachableMapTest, UnreachableNodeDoesNotGetOverflowDistance)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(-1, planner.getDistance(2));
    EXPECT_EQ(-1, planner.getDistance(3));

    planner.run(2);             // other side works too
    EXPECT_EQ(4, planner.getDistance(3));
    EXPECT_EQ(-1, planner.getDistance(0));
}
