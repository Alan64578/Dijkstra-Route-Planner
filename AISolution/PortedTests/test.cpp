#include "pch.h"
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include "CollegeMap.h"
#include "Dijkstra.h"
using namespace std;

// My 34 tests from MapTests/test.cpp, ported to the AI solution's interface.
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

// PORT: load() returns a LoadStatus, so "loaded OK" means status == Ok
bool loadOk(CollegeMap& map, string fileName)
{
    return map.load(fileName) == CollegeMap::LoadStatus::Ok;
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

// MTU map from appendix 3, exactly as supplied
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

// PORT: the AI's empty map has the name "" where mine uses "None".
TEST(CollegeMapTest, DefaultMapIsEmpty)
{
    CollegeMap map;
    EXPECT_FALSE(map.isLoaded());
    EXPECT_EQ(0, map.nodes());
    EXPECT_EQ("", map.name());
}

// PORT: none. Expected text kept, so this checks the AI's empty display.
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

// PORT: load() returns a LoadStatus.
TEST(CollegeMapTest, MissingFileFailsToLoad)
{
    CollegeMap map;
    EXPECT_FALSE(loadOk(map, "does_not_exist.txt"));
    EXPECT_FALSE(map.isLoaded());
}

// PORT: load() returns a LoadStatus.
TEST(CollegeMapTest, BadFilesFailToLoad)
{
    CollegeMap map;

    writeFile("bad.txt", "Bad_Map\nthree\n0 5\n5 0\n");
    EXPECT_FALSE(loadOk(map, "bad.txt"));

    writeFile("bad.txt", "Bad_Map\n3\n0 5 0\n5 0 2\n0 2\n");
    EXPECT_FALSE(loadOk(map, "bad.txt"));

    writeFile("bad.txt", "Bad_Map\n2\n0 -5\n-5 0\n");
    EXPECT_FALSE(loadOk(map, "bad.txt"));

    writeFile("bad.txt", "Bad_Map\n2\n1 5\n5 0\n");
    EXPECT_FALSE(loadOk(map, "bad.txt"));

    remove("bad.txt");
}

// PORT: renamed getters.
TEST_F(ThreeNodeMapTest, LoadingValidFileReportsThreeNodes)
{
    CollegeMap map("tiny.txt");
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(3, map.nodes());
    EXPECT_EQ("Tiny_Map", map.name());
}

// PORT: getWeight() -> weight().
TEST_F(ThreeNodeMapTest, GetWeightReturnsValuesFromFile)
{
    CollegeMap map("tiny.txt");
    EXPECT_EQ(5, map.weight(0, 1));
    EXPECT_EQ(2, map.weight(2, 1));
    EXPECT_EQ(0, map.weight(0, 2));
}

// PORT: the AI has no isValidNode(), so each check is done through weight():
// a valid node reads without throwing, a bad one throws std::out_of_range
// instead of returning -1.
TEST_F(ThreeNodeMapTest, OutOfRangeNodesAreRejected)
{
    CollegeMap map("tiny.txt");
    EXPECT_NO_THROW(map.weight(2, 0));
    EXPECT_THROW(map.weight(3, 0), std::out_of_range);
    EXPECT_THROW(map.weight(-1, 0), std::out_of_range);
    EXPECT_THROW(map.weight(0, 3), std::out_of_range);
}

// PORT: renamed getters. The AI rejects non-symmetric files, so this is expected to fail.
TEST_F(MtuMapTest, NonSymmetricMapStillLoads)
{
    CollegeMap map("mtu.txt");
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(9, map.nodes());
    if (map.isLoaded())
        EXPECT_EQ(14, map.weight(3, 5));
}

// PORT: setWeight() is void and throws on error.
TEST_F(ThreeNodeMapTest, SetWeightChangesBothDirections)
{
    CollegeMap map("tiny.txt");
    EXPECT_NO_THROW(map.setWeight(0, 2, 9));
    EXPECT_EQ(9, map.weight(0, 2));
    EXPECT_EQ(9, map.weight(2, 0));
}

// PORT: bad index throws out_of_range; negative and self-loop throw invalid_argument.
TEST_F(ThreeNodeMapTest, SetWeightRejectsBadValues)
{
    CollegeMap map("tiny.txt");
    EXPECT_THROW(map.setWeight(3, 0, 1), std::out_of_range);
    EXPECT_THROW(map.setWeight(0, 1, -4), std::invalid_argument);
    EXPECT_THROW(map.setWeight(1, 1, 3), std::invalid_argument);
    EXPECT_EQ(5, map.weight(0, 1));
}

// PORT: none. Expected text kept, so this checks the AI's display layout.
TEST_F(ThreeNodeMapTest, DisplayShowsNameNodesAndMatrix)
{
    CollegeMap map("tiny.txt");
    ofstream fout("out.txt");
    fout << map;
    fout.close();
    EXPECT_EQ("Map: Tiny_Map Nodes: 3 0 1 2 0 0 5 0 1 5 0 2 2 0 2 0", readWords("out.txt"));
}

// PORT: setWeight() is void, renamed getter.
TEST_F(ThreeNodeMapTest, CopyConstructorMakesIndependentCopy)
{
    CollegeMap original("tiny.txt");
    CollegeMap copy = original;
    EXPECT_TRUE(copy == original);

    copy.setWeight(0, 1, 99);
    EXPECT_EQ(5, original.weight(0, 1));
}

// PORT: setWeight() is void, renamed getter.
TEST_F(ThreeNodeMapTest, AssignmentMakesIndependentCopy)
{
    CollegeMap original("tiny.txt");
    CollegeMap assigned;
    assigned = original;
    EXPECT_TRUE(assigned == original);

    original.setWeight(1, 2, 50);
    EXPECT_EQ(2, assigned.weight(1, 2));
}

// PORT: renamed getter.
TEST_F(ThreeNodeMapTest, SelfAssignmentKeepsData)
{
    CollegeMap map("tiny.txt");
    map = map;
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(5, map.weight(0, 1));
}

// PORT: setWeight() is void.
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

// PORT: load() returns a LoadStatus, renamed getters.
TEST_F(ThreeNodeMapTest, ReloadingReplacesOldMap)
{
    CollegeMap map("tiny.txt");
    writeFile("two.txt", "Two_Map\n2\n0 1\n1 0\n");
    EXPECT_TRUE(loadOk(map, "two.txt"));
    EXPECT_EQ(2, map.nodes());
    EXPECT_EQ("Two_Map", map.name());
    remove("two.txt");
}

// Dijkstra tests

// PORT: run() is void (EXPECT_NO_THROW), getDistance() -> distanceTo().
TEST(DijkstraTest, OneNodeMapRunsFromNodeZero)
{
    writeFile("one.txt", "One_Map\n1\n0\n");
    CollegeMap map("one.txt");
    Dijkstra planner(map);
    EXPECT_NO_THROW(planner.run(0));
    EXPECT_EQ(0, planner.distanceTo(0));
    remove("one.txt");
}

// PORT: run() is void, getDistance() -> distanceTo().
TEST_F(ThreeNodeMapTest, DistancesFromNodeZero)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    EXPECT_NO_THROW(planner.run(0));
    EXPECT_EQ(0, planner.distanceTo(0));
    EXPECT_EQ(5, planner.distanceTo(1));
    EXPECT_EQ(7, planner.distanceTo(2));
}

// PORT: getDistance() -> distanceTo().
TEST_F(MtuMapTest, DistancesFromNodeZeroMatchBrief)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(4, planner.distanceTo(1));
    EXPECT_EQ(12, planner.distanceTo(2));
    EXPECT_EQ(19, planner.distanceTo(3));
    EXPECT_EQ(21, planner.distanceTo(4));
    EXPECT_EQ(11, planner.distanceTo(5));
    EXPECT_EQ(9, planner.distanceTo(6));
    EXPECT_EQ(8, planner.distanceTo(7));
    EXPECT_EQ(14, planner.distanceTo(8));
}

// PORT: getDistance() -> distanceTo().
TEST_F(MtuMapTest, DistancesFromNodeThree)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(3);
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

// PORT: getSource() -> source(), getDistance() -> distanceTo().
TEST_F(MtuMapTest, SecondRunUsesNewSource)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(0);
    planner.run(3);
    EXPECT_EQ(3, planner.source());
    EXPECT_EQ(0, planner.distanceTo(3));
    EXPECT_EQ(19, planner.distanceTo(0));
    EXPECT_EQ(14, planner.distanceTo(7));
}

// PORT: the route comes back as a string, not an int array and a count.
TEST_F(MtuMapTest, RouteFromNodeThreeToSeven)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(3);
    EXPECT_EQ("3 -> 2 -> 5 -> 6 -> 7", planner.routeTo(7));
}

// PORT: the AI has no printRoute(); routeTo() returns the same text,
// so the output is taken from it instead of captured from cout.
TEST_F(MtuMapTest, PrintRouteShowsOneRoute)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(3);

    string output = planner.routeTo(7);

    EXPECT_NE(string::npos, output.find("3 -> 2 -> 5 -> 6 -> 7"));
}

// PORT: run() throws std::out_of_range instead of returning false; getSource() -> source().
TEST_F(ThreeNodeMapTest, InvalidSourceIsRejected)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    EXPECT_THROW(planner.run(-1), std::out_of_range);
    EXPECT_THROW(planner.run(3), std::out_of_range);
    EXPECT_EQ(-1, planner.source());
}

// PORT: run() throws std::logic_error instead of returning false.
TEST(DijkstraTest, RunOnEmptyMapFails)
{
    CollegeMap empty;
    Dijkstra planner(empty);
    EXPECT_THROW(planner.run(0), std::logic_error);
}

// PORT: getDistance() -> distanceTo(); an unreachable route is the string
// "unreachable" instead of a count of 0.
TEST_F(SplitMapTest, UnreachableNodeHasNoDistance)
{
    CollegeMap map("split.txt");
    Dijkstra planner(map);
    planner.run(0);

    EXPECT_EQ(3, planner.distanceTo(1));
    EXPECT_TRUE(planner.isReachable(1));
    EXPECT_FALSE(planner.isReachable(3));
    EXPECT_EQ(-1, planner.distanceTo(3));

    EXPECT_EQ("unreachable", planner.routeTo(3));
}

// PORT: the table comes from writeResults(). Expected text kept.
TEST_F(ThreeNodeMapTest, PrintAllFromNodeTwo)
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

// PORT: none apart from run(). Expected text kept.
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

// PORT: setWeight() is void, getDistance() -> distanceTo().
TEST_F(ThreeNodeMapTest, DijkstraSeesChangesToMap)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(7, planner.distanceTo(2));

    map.setWeight(0, 2, 1);
    planner.run(0);
    EXPECT_EQ(1, planner.distanceTo(2));
}

// PORT: run() is void, getDistance() -> distanceTo().
TEST_F(ThreeNodeMapTest, RunAfterMapReloadedToNewSize)
{
    CollegeMap map("tiny.txt");
    Dijkstra planner(map);

    writeFile("big.txt", "Big_Map\n4\n0 1 0 0\n1 0 1 0\n0 1 0 1\n0 0 1 0\n");
    map.load("big.txt");
    EXPECT_NO_THROW(planner.run(0));
    EXPECT_EQ(3, planner.distanceTo(3));
    remove("big.txt");
}

// PORT: renamed getter.
TEST_F(ThreeNodeMapTest, MapSurvivesDijkstraDestruction)
{
    CollegeMap map("tiny.txt");
    {
        Dijkstra planner(map);
        planner.run(0);
    }
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(5, map.weight(0, 1));
}

// tests for the bugs in the reference code

// PORT: the route comes back as a string.
TEST_F(MtuMapTest, RouteFromNonZeroSourceStartsAtSource)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(3);
    EXPECT_EQ("3 -> 2", planner.routeTo(2));
}

// PORT: getDistance() -> distanceTo().
TEST_F(SplitMapTest, UnreachableNodeDoesNotGoNegative)
{
    CollegeMap map("split.txt");
    Dijkstra planner(map);
    planner.run(0);
    EXPECT_EQ(-1, planner.distanceTo(2));
    EXPECT_EQ(-1, planner.distanceTo(3));

    planner.run(2);
    EXPECT_EQ(4, planner.distanceTo(3));
}

// PORT: the table comes from writeResults(). Same word position checked.
TEST_F(MtuMapTest, PrintAllShowsRealSource)
{
    CollegeMap map("mtu.txt");
    Dijkstra planner(map);
    planner.run(6);
    ofstream fout("mtu_out.txt");
    planner.writeResults(fout);
    fout.close();

    ifstream fin("mtu_out.txt");
    string word;
    fin >> word >> word >> word >> word >> word;
    fin.close();
    EXPECT_EQ("6", word);
    remove("mtu_out.txt");
}
