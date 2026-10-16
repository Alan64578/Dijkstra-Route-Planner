#include "pch.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "CollegeMap.h"
#include "Dijkstra.h"
using namespace std;

// My 46 tests from MapTests/test.cpp, ported to the AI solution's interface.
// Names and the behaviour each test checks are unchanged.
// "PORT:" comments say what had to change and why.
//
// Interface changes used throughout:
//   getNodesTotal() -> nodes()        getName() -> name()
//   getWeight()     -> weight()       load() returns LoadStatus, not bool
//   setWeight()     -> void, throws instead of returning false
//   run()           -> void, throws instead of returning false
//   getDistance()   -> distanceTo()   getSource() -> source()
//   getRoute(d, int*) -> routeTo(d) returning a string such as "3 -> 2"
//   operator<< for Dijkstra -> writeResults(ostream&)

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

bool loadOk(CollegeMap& map, string fileName)
{
    return map.load(fileName) == CollegeMap::LoadStatus::Ok;
}

// The MTU map data. MTU_DATA is the file exactly as printed in Appendix 3
// unless the build defines USE_AI_CORRECTED_MAP (second run, for comparison).
#ifdef USE_AI_CORRECTED_MAP
static const char* MTU_ROW5 = "0 0 4 14 10 0 2 0 0\n";
static const char* MTU_ROW6 = "0 0 0 0 0 2 0 1 6\n";
#else
static const char* MTU_ROW5 = "0 0 4 0 10 0 2 0 0\n";
static const char* MTU_ROW6 = "0 0 0 14 0 2 0 1 6\n";
#endif

// ---------- fixtures ----------

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
            << MTU_ROW5
            << MTU_ROW6
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
// PORT: the AI's empty map has the name "" where mine uses "None".
TEST(CollegeMapTest, DefaultMapIsEmpty)
{
    CollegeMap map;
    EXPECT_FALSE(map.isLoaded());
    EXPECT_EQ(0, map.nodes());
    EXPECT_EQ("", map.name());
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
// PORT: none. Expected text kept, so this checks the AI's empty display.
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
    EXPECT_FALSE(loadOk(map, "does_not_exist.txt"));
    EXPECT_FALSE(map.isLoaded());
}

// CM3, CM4
TEST_F(ThreeNodeMapTest, LoadingValidFileReportsThreeNodes)
{
    CollegeMap map(fileName);
    ASSERT_TRUE(map.isLoaded());
    EXPECT_EQ(3, map.nodes());
    EXPECT_EQ("Tiny_Map", map.name());
}

// CM4, CM5 - getters work through a const reference
// PORT: the AI has no isValidNode(); that line is replaced by a check
// that weight() accepts node 2 through the const reference.
TEST_F(ThreeNodeMapTest, ConstMapCanStillBeRead)
{
    CollegeMap map(fileName);
    const CollegeMap& constMap = map;
    EXPECT_TRUE(constMap.isLoaded());
    EXPECT_EQ("Tiny_Map", constMap.name());
    EXPECT_EQ(3, constMap.nodes());
    EXPECT_EQ(5, constMap.weight(0, 1));
    EXPECT_NO_THROW(constMap.weight(2, 2));
    EXPECT_TRUE(constMap == map);
}

// CM3, CM5
TEST_F(ThreeNodeMapTest, GetWeightReturnsValuesFromFile)
{
    CollegeMap map(fileName);
    EXPECT_EQ(5, map.weight(0, 1));
    EXPECT_EQ(2, map.weight(2, 1));
    EXPECT_EQ(0, map.weight(0, 2));
    EXPECT_EQ(0, map.weight(1, 1));
}

// CM2
TEST_F(MalformedFileTest, NonNumericNodeCountFails)
{
    writeFile(fileName, "Bad_Map\nthree\n0 5\n5 0\n");
    CollegeMap map;
    EXPECT_FALSE(loadOk(map, fileName));
    EXPECT_FALSE(map.isLoaded());
}

// CM2
TEST_F(MalformedFileTest, ZeroNodeCountFails)
{
    writeFile(fileName, "Bad_Map\n0\n");
    CollegeMap map;
    EXPECT_FALSE(loadOk(map, fileName));
}

// CM2
TEST_F(MalformedFileTest, TooFewValuesFails)
{
    writeFile(fileName, "Bad_Map\n3\n0 5 0\n5 0 2\n0 2\n");
    CollegeMap map;
    EXPECT_FALSE(loadOk(map, fileName));
    EXPECT_EQ(0, map.nodes());
}

// CM2
TEST_F(MalformedFileTest, NegativeWeightFails)
{
    writeFile(fileName, "Bad_Map\n2\n0 -5\n-5 0\n");
    CollegeMap map;
    EXPECT_FALSE(loadOk(map, fileName));
}

// CM2
TEST_F(MalformedFileTest, NonZeroDiagonalFails)
{
    writeFile(fileName, "Bad_Map\n2\n1 5\n5 0\n");
    CollegeMap map;
    EXPECT_FALSE(loadOk(map, fileName));
}

// CM2, CM9 - a failed reload must not leave the old map behind
// PORT: an invalid index now throws instead of returning -1.
TEST_F(ThreeNodeMapTest, FailedReloadLeavesMapEmpty)
{
    CollegeMap map(fileName);
    ASSERT_TRUE(map.isLoaded());
    EXPECT_FALSE(loadOk(map, "does_not_exist.txt"));
    EXPECT_FALSE(map.isLoaded());
    EXPECT_THROW(map.weight(0, 1), std::logic_error);
}

// CM2 - decision: not symmetric is only a warning (MTU map isn't symmetric)
TEST_F(MtuMapTest, NonSymmetricMtuMapStillLoads)
{
    CollegeMap map(fileName);
    EXPECT_TRUE(map.isLoaded());
    ASSERT_EQ(9, map.nodes());
    EXPECT_EQ(14, map.weight(3, 5));
    EXPECT_EQ(0, map.weight(5, 3));
}

// CM5
// PORT: setWeight() is void and throws on error.
TEST_F(ThreeNodeMapTest, SetWeightChangesBothDirections)
{
    CollegeMap map(fileName);
    EXPECT_NO_THROW(map.setWeight(0, 2, 9));
    EXPECT_EQ(9, map.weight(0, 2));
    EXPECT_EQ(9, map.weight(2, 0));
}

// CM5 - boundary indices
// PORT: bad indices throw std::out_of_range instead of returning -1/false.
TEST_F(ThreeNodeMapTest, OutOfRangeIndexIsRejected)
{
    CollegeMap map(fileName);
    EXPECT_THROW(map.weight(-1, 0), std::out_of_range);
    EXPECT_THROW(map.weight(0, 3), std::out_of_range);
    EXPECT_THROW(map.setWeight(3, 0, 1), std::out_of_range);
    EXPECT_THROW(map.setWeight(0, -1, 1), std::out_of_range);
    EXPECT_EQ(5, map.weight(0, 1)); // nothing changed
}

// CM5
// PORT: a negative weight throws std::invalid_argument.
TEST_F(ThreeNodeMapTest, NegativeWeightIsRejected)
{
    CollegeMap map(fileName);
    EXPECT_THROW(map.setWeight(0, 1, -4), std::invalid_argument);
    EXPECT_EQ(5, map.weight(0, 1));
}

// CM5
// PORT: a self-loop weight throws std::invalid_argument.
TEST_F(ThreeNodeMapTest, SelfLoopWeightIsRejected)
{
    CollegeMap map(fileName);
    EXPECT_THROW(map.setWeight(1, 1, 3), std::invalid_argument);
    EXPECT_EQ(0, map.weight(1, 1));
}

// CM6 - checks << output word by word
// PORT: none. Expected text kept, so this checks the AI's display layout.
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
    EXPECT_EQ(99, copy.weight(0, 1));
    EXPECT_EQ(5, original.weight(0, 1));
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
    EXPECT_EQ(2, assigned.weight(1, 2));
}

// CM7
TEST_F(ThreeNodeMapTest, SelfAssignmentKeepsData)
{
    CollegeMap map(fileName);
    CollegeMap& sameMap = map;
    map = sameMap;
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(3, map.nodes());
    EXPECT_EQ(5, map.weight(0, 1));
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
    EXPECT_EQ(9, small.nodes());

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
    EXPECT_TRUE(loadOk(map, "test_two_node.txt"));
    EXPECT_EQ(2, map.nodes());
    EXPECT_EQ("Two_Map", map.name());
    remove("test_two_node.txt");
}

// ================= Dijkstra =================

// D3 - one node map, nothing else to reach
TEST(DijkstraTest, OneNodeMapRunsFromNodeZero)
{
    writeFile("test_one_node.txt", "One_Map\n1\n0\n");
    CollegeMap map("test_one_node.txt");
    Dijkstra planner(map);
    EXPECT_NO_THROW(planner.run(0));
    EXPECT_EQ(0, planner.distanceTo(0));
    remove("test_one_node.txt");
}

// D3, CM8 - hand worked: 0->1 = 5, 0->1->2 = 5+2 = 7
TEST_F(ThreeNodeMapTest, DistancesFromNodeZero)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_NO_THROW(planner.run(0));
    EXPECT_EQ(0, planner.distanceTo(0));
    EXPECT_EQ(5, planner.distanceTo(1));
    EXPECT_EQ(7, planner.distanceTo(2));
}

// D3 - matches the sample output in the brief
TEST_F(MtuMapTest, DistancesFromNodeZeroMatchBrief)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_NO_THROW(planner.run(0));
    EXPECT_EQ(4, planner.distanceTo(1));
    EXPECT_EQ(12, planner.distanceTo(2));
    EXPECT_EQ(19, planner.distanceTo(3));
    EXPECT_EQ(21, planner.distanceTo(4));
    EXPECT_EQ(11, planner.distanceTo(5));
    EXPECT_EQ(9, planner.distanceTo(6));
    EXPECT_EQ(8, planner.distanceTo(7));
    EXPECT_EQ(14, planner.distanceTo(8));
}

// D3, CM8 - source 3, worked out by hand (see report)
TEST_F(MtuMapTest, DistancesFromNodeThree)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_NO_THROW(planner.run(3));
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

// D4 - full route from node 3 to 7: 3 -> 2 -> 5 -> 6 -> 7
// PORT: the route comes back as a string, not an int array.
TEST_F(MtuMapTest, RouteFromNodeThreeToSeven)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_NO_THROW(planner.run(3));
    EXPECT_EQ("3 -> 2 -> 5 -> 6 -> 7", planner.routeTo(7));
    EXPECT_EQ(14, planner.distanceTo(7));
}

// D3 - running again with a new source gives the same as a new object
TEST_F(MtuMapTest, RepeatedRunsMatchFreshObject)
{
    CollegeMap map(fileName);
    Dijkstra reused(map);
    ASSERT_NO_THROW(reused.run(0));
    reused.run(3);

    Dijkstra fresh(map);
    fresh.run(3);

    for (int i = 0; i < 9; i++)
        EXPECT_EQ(fresh.distanceTo(i), reused.distanceTo(i));
    EXPECT_EQ(3, reused.source());
}

// D3 - invalid source nodes
// PORT: run() throws std::out_of_range instead of returning false, and
// distanceTo() throws when there are no results instead of returning -1.
TEST_F(ThreeNodeMapTest, InvalidSourceIsRejected)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    EXPECT_THROW(planner.run(-1), std::out_of_range);
    EXPECT_THROW(planner.run(3), std::out_of_range);
    EXPECT_EQ(-1, planner.source());
    EXPECT_THROW(planner.distanceTo(1), std::logic_error);
}

// D3 - no map loaded
// PORT: run() throws std::logic_error instead of returning false.
TEST(DijkstraTest, RunOnEmptyMapFails)
{
    CollegeMap empty;
    Dijkstra planner(empty);
    EXPECT_THROW(planner.run(0), std::logic_error);
}

// D4 - asking for results before run()
// PORT: each query throws std::logic_error instead of returning -1/false/0.
TEST_F(ThreeNodeMapTest, NoResultsBeforeRun)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    EXPECT_THROW(planner.distanceTo(1), std::logic_error);
    EXPECT_THROW(planner.isReachable(1), std::logic_error);
    EXPECT_THROW(planner.routeTo(1), std::logic_error);
}

// D4 - unreachable node is reported, not 2147483647
TEST_F(UnreachableMapTest, UnreachableNodeHasNoDistance)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_NO_THROW(planner.run(0));
    EXPECT_EQ(3, planner.distanceTo(1));
    EXPECT_FALSE(planner.isReachable(2));
    EXPECT_FALSE(planner.isReachable(3));
    EXPECT_EQ(-1, planner.distanceTo(3));
}

// D4 - unreachable nodes get no route and print as unreachable
// PORT: routeTo() gives "unreachable" instead of a count of 0, and the table
// comes from writeResults(). Expected text kept, so this checks the layout.
TEST_F(UnreachableMapTest, UnreachableNodePrintedAsUnreachable)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ("unreachable", planner.routeTo(3));

    ofstream fout(outName.c_str());
    planner.writeResults(fout);
    fout.close();
    EXPECT_EQ("Shortest routes from node 0 Destination Distance Route 1 3 0 -> 1 2 - unreachable 3 - unreachable",
        readWords(outName));
}

// D4 - all routes, hand worked from node 2: 2->1 = 2, 2->1->0 = 7
// PORT: table comes from writeResults(). Expected text kept.
TEST_F(ThreeNodeMapTest, PrintAllFromNodeTwo)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(2);
    ofstream fout(outName.c_str());
    planner.writeResults(fout);
    fout.close();
    EXPECT_EQ("Shortest routes from node 2 Destination Distance Route 0 7 2 -> 1 -> 0 1 2 2 -> 1",
        readWords(outName));
}

// D5
// PORT: none apart from run(). Expected text kept.
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
// PORT: saveResults() before run() throws std::logic_error instead of returning false.
TEST_F(ThreeNodeMapTest, SaveBeforeRunFails)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    EXPECT_THROW(planner.saveResults(outName), std::logic_error);
}

// D1, CM8 - Dijkstra uses the map itself, not a copy
TEST_F(ThreeNodeMapTest, DijkstraSeesChangesToMap)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(7, planner.distanceTo(2));

    map.setWeight(0, 2, 1); // add a shortcut
    planner.run(0);
    EXPECT_EQ(1, planner.distanceTo(2));
}

// D2 - map reloaded at a different size after Dijkstra was made
TEST_F(ThreeNodeMapTest, RunAfterMapReloadedToNewSize)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    planner.run(0);

    writeFile("test_big.txt", "Big_Map\n4\n0 1 0 0\n1 0 1 0\n0 1 0 1\n0 0 1 0\n");
    ASSERT_TRUE(loadOk(map, "test_big.txt"));
    ASSERT_NO_THROW(planner.run(0));
    EXPECT_EQ(3, planner.distanceTo(3));
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
    EXPECT_EQ(3, map.nodes());
    EXPECT_EQ(5, map.weight(0, 1));
}

// ================= Reference code defects =================

// Defect: parent[0] = -1 instead of parent[src] (only works when src is 0)
// PORT: route is a string.
TEST_F(MtuMapTest, RouteFromNonZeroSourceStartsAtSource)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_NO_THROW(planner.run(3));
    EXPECT_EQ("3 -> 2", planner.routeTo(2));
}

// Defect: printSolution always prints source as 0
// PORT: table comes from writeResults(). Same word positions checked.
TEST_F(MtuMapTest, PrintAllShowsRealSource)
{
    CollegeMap map(fileName);
    Dijkstra planner(map);
    ASSERT_NO_THROW(planner.run(6));
    ofstream fout(outName.c_str());
    planner.writeResults(fout);
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
    EXPECT_EQ(-1, planner.distanceTo(2));
    EXPECT_EQ(-1, planner.distanceTo(3));

    planner.run(2); // other side works too
    EXPECT_EQ(4, planner.distanceTo(3));
    EXPECT_EQ(-1, planner.distanceTo(0));
}
