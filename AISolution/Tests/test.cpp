// test.cpp - Google Test suite for CollegeMap and Dijkstra.
// Test names start with the requirement they trace to (CM1..CM9, D1..D7).
// If your Google Test project template uses precompiled headers, add
//     #include "pch.h"
// as the very first line of this file.

#include "pch.h"  //============AI Forgot this line and tests wouldnt run============
#include "gtest/gtest.h"

#include "CollegeMap.h"
#include "Dijkstra.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// ==================================================================
// new[] / delete[] tracking (Fixed Constraint 5, CM9, D7)
//
// Replacing the global array operators in the test executable lets the
// tests count every new[] and delete[] the classes perform, and make a
// chosen new[] throw std::bad_alloc to exercise the failure paths.
// ==================================================================
namespace alloc
{
    long newCalls = 0;
    long deleteCalls = 0;
    int  failCountdown = 0;   // > 0: the Nth new[] from now throws

    void failOnCall(int n) { failCountdown = n; }
}

void* operator new[](std::size_t size)
{
    if (alloc::failCountdown > 0 && --alloc::failCountdown == 0)
        throw std::bad_alloc();
    void* p = std::malloc(size == 0 ? 1 : size);
    if (p == nullptr)
        throw std::bad_alloc();
    ++alloc::newCalls;
    return p;
}

void operator delete[](void* p) noexcept
{
    if (p != nullptr)
    {
        ++alloc::deleteCalls;
        std::free(p);
    }
}

void operator delete[](void* p, std::size_t) noexcept
{
    operator delete[](p);
}

namespace
{
    // Records the counters on construction; live() is the number of
    // new[] blocks allocated since then that have not been released.
    class AllocationTracker
    {
    public:
        AllocationTracker() : m_new(alloc::newCalls), m_delete(alloc::deleteCalls) {}
        long allocations() const { return alloc::newCalls - m_new; }
        long releases() const    { return alloc::deleteCalls - m_delete; }
        long live() const        { return allocations() - releases(); }
    private:
        long m_new;
        long m_delete;
    };

    // ------------------------------------------------------------------
    // Test data
    // ------------------------------------------------------------------
    // Symmetric version of the Appendix 3 map (single 3-5 edge of 14).
    const char* const MtuMap =
        "MTU_Map\n"
        "9\n"
        "0   4   0   0   0   0   0   8   0\n"
        "4   0   8   0   0   0   0   11  0\n"
        "0   8   0   7   0   4   0   0   2\n"
        "0   0   7   0   9   14  0   0   0\n"
        "0   0   0   9   0   10  0   0   0\n"
        "0   0   4   14  10  0   2   0   0\n"
        "0   0   0   0   0   2   0   1   6\n"
        "8   11  0   0   0   0   1   0   7\n"
        "0   0   2   0   0   0   6   7   0\n";

    // The file exactly as printed in Appendix 3 (not symmetric).
    const char* const MtuMapAsSupplied =
        "MTU_Map\n"
        "9\n"
        "0   4   0   0   0   0   0   8   0\n"
        "4   0   8   0   0   0   0   11  0\n"
        "0   8   0   7   0   4   0   0   2\n"
        "0   0   7   0   9   14  0   0   0\n"
        "0   0   0   9   0   10  0   0   0\n"
        "0   0   4   0   10  0   2   0   0\n"
        "0   0   0   14  0   2   0   1   6\n"
        "8   11  0   0   0   0   1   0   7\n"
        "0   0   2   0   0   0   6   7   0\n";

    // Two separate islands: {0,1} and {2,3}.
    const char* const TwoIslands =
        "Islands\n"
        "4\n"
        "0 5 0 0\n"
        "5 0 0 0\n"
        "0 0 0 3\n"
        "0 0 3 0\n";

    const char* const Triangle =
        "Triangle\n"
        "3\n"
        "0 1 4\n"
        "1 0 2\n"
        "4 2 0\n";

    // Writes a temporary file and deletes it when the object goes away.
    class TempFile
    {
    public:
        explicit TempFile(const std::string& content)
            : m_name("cm_test_" + std::to_string(++s_count) + ".txt")
        {
            std::ofstream out(m_name, std::ios::binary);
            out << content;
        }
        ~TempFile() { std::remove(m_name.c_str()); }
        TempFile(const TempFile&) = delete;
        TempFile& operator=(const TempFile&) = delete;
        const std::string& name() const { return m_name; }
    private:
        std::string m_name;
        static int s_count;
    };
    int TempFile::s_count = 0;

    CollegeMap loadFrom(const char* content)
    {
        TempFile file(content);
        return CollegeMap(file.name());
    }

    CollegeMap::LoadStatus statusOf(const std::string& content)
    {
        TempFile file(content);
        CollegeMap map;
        return map.load(file.name());
    }

    std::string text(const CollegeMap& map)
    {
        std::ostringstream os;
        os << map;
        return os.str();
    }

    std::string readFile(const std::string& name)
    {
        std::ifstream in(name);
        std::stringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    // Sum of edge weights along a route string such as "0 -> 7 -> 6".
    // Returns -1 if two consecutive nodes are not joined by an edge.
    int routeLength(const CollegeMap& map, const std::string& route)
    {
        std::istringstream in(route);
        int previous = 0;
        in >> previous;
        int total = 0;
        std::string arrow;
        int next = 0;
        while (in >> arrow >> next)
        {
            const int w = map(previous, next);
            if (w == 0)
                return -1;
            total += w;
            previous = next;
        }
        return total;
    }
}

// ==================================================================
// CM1 Construction
// ==================================================================
TEST(CM1_Construction, DefaultMapIsEmpty)
{
    CollegeMap map;
    EXPECT_FALSE(map.isLoaded());
    EXPECT_EQ(map.nodes(), 0);
    EXPECT_EQ(map.name(), "");
    EXPECT_EQ(map.status(), CollegeMap::LoadStatus::NotLoaded);
}

TEST(CM1_Construction, FromFileLoadsMap)
{
    CollegeMap map = loadFrom(MtuMap);
    EXPECT_TRUE(map.isLoaded());
    EXPECT_EQ(map.status(), CollegeMap::LoadStatus::Ok);
}

TEST(CM1_Construction, EmptyMapIsSafeToDisplayCopyAssignAndDestroy)
{
    CollegeMap empty;
    EXPECT_NE(text(empty).find("no map loaded"), std::string::npos);

    CollegeMap copy(empty);
    EXPECT_FALSE(copy.isLoaded());
    EXPECT_TRUE(copy == empty);

    CollegeMap assigned = loadFrom(Triangle);
    assigned = empty;
    EXPECT_FALSE(assigned.isLoaded());
    EXPECT_EQ(assigned.nodes(), 0);
}   // all three destroyed here without a crash

TEST(CM1_Construction, FailedFileIsSafeToDisplayCopyAssignAndDestroy)
{
    CollegeMap bad("this file does not exist.txt");
    EXPECT_FALSE(bad.isLoaded());
    EXPECT_NO_THROW(text(bad));

    CollegeMap copy(bad);
    EXPECT_FALSE(copy.isLoaded());

    CollegeMap good = loadFrom(Triangle);
    good = bad;
    EXPECT_FALSE(good.isLoaded());
}

// ==================================================================
// CM2 Loading and validation
// ==================================================================
TEST(CM2_Loading, MissingFileIsReported)
{
    CollegeMap map("this file does not exist.txt");
    EXPECT_EQ(map.status(), CollegeMap::LoadStatus::FileNotFound);
    EXPECT_NE(map.statusMessage().find("this file does not exist.txt"), std::string::npos);
}

TEST(CM2_Loading, EmptyFileIsMissingName)
{
    EXPECT_EQ(statusOf(""), CollegeMap::LoadStatus::MissingName);
    EXPECT_EQ(statusOf("\n  \n"), CollegeMap::LoadStatus::MissingName);
}

TEST(CM2_Loading, MissingNodeCount)
{
    EXPECT_EQ(statusOf("NameOnly\n"), CollegeMap::LoadStatus::BadNodeCount);
}

TEST(CM2_Loading, NonNumericNodeCount)
{
    EXPECT_EQ(statusOf("M\nnine\n0\n"), CollegeMap::LoadStatus::BadNodeCount);
    EXPECT_EQ(statusOf("M\n3x\n0 1 1\n1 0 1\n1 1 0\n"), CollegeMap::LoadStatus::BadNodeCount);
    EXPECT_EQ(statusOf("M\n2.5\n0 1\n1 0\n"), CollegeMap::LoadStatus::BadNodeCount);
}

TEST(CM2_Loading, NodeCountOutOfRange)
{
    EXPECT_EQ(statusOf("M\n0\n"), CollegeMap::LoadStatus::BadNodeCount);
    EXPECT_EQ(statusOf("M\n-3\n"), CollegeMap::LoadStatus::BadNodeCount);
    EXPECT_EQ(statusOf("M\n1001\n"), CollegeMap::LoadStatus::BadNodeCount);
}

TEST(CM2_Loading, TooFewRows)
{
    EXPECT_EQ(statusOf("M\n3\n0 1 4\n1 0 2\n"), CollegeMap::LoadStatus::TooFewRows);
}

TEST(CM2_Loading, RowWithTooFewOrTooManyValues)
{
    EXPECT_EQ(statusOf("M\n3\n0 1 4\n1 0\n4 2 0\n"), CollegeMap::LoadStatus::WrongRowLength);
    EXPECT_EQ(statusOf("M\n3\n0 1 4 9\n1 0 2\n4 2 0\n"), CollegeMap::LoadStatus::WrongRowLength);
}

TEST(CM2_Loading, NonNumericWeight)
{
    EXPECT_EQ(statusOf("M\n3\n0 1 4\n1 0 x\n4 2 0\n"), CollegeMap::LoadStatus::NonNumericValue);
    EXPECT_EQ(statusOf("M\n3\n0 1 4\n1 0 2.5\n4 2 0\n"), CollegeMap::LoadStatus::NonNumericValue);
}

TEST(CM2_Loading, NegativeOrHugeWeight)
{
    EXPECT_EQ(statusOf("M\n2\n0 -1\n-1 0\n"), CollegeMap::LoadStatus::WeightOutOfRange);
    EXPECT_EQ(statusOf("M\n2\n0 1000001\n1000001 0\n"), CollegeMap::LoadStatus::WeightOutOfRange);
}

TEST(CM2_Loading, NonZeroDiagonal)
{
    EXPECT_EQ(statusOf("M\n2\n5 1\n1 0\n"), CollegeMap::LoadStatus::NonZeroDiagonal);
}

TEST(CM2_Loading, AsymmetricMatrix)
{
    EXPECT_EQ(statusOf("M\n2\n0 1\n2 0\n"), CollegeMap::LoadStatus::NotSymmetric);
}

TEST(CM2_Loading, SuppliedAppendix3FileIsRejectedAsAsymmetric)
{
    TempFile file(MtuMapAsSupplied);
    CollegeMap map(file.name());
    EXPECT_EQ(map.status(), CollegeMap::LoadStatus::NotSymmetric);
    EXPECT_NE(map.statusMessage().find("3->5"), std::string::npos);
}

TEST(CM2_Loading, TrailingData)
{
    EXPECT_EQ(statusOf("M\n2\n0 1\n1 0\n7 7\n"), CollegeMap::LoadStatus::TrailingData);
}

TEST(CM2_Loading, WindowsLineEndingsAndBlankLinesAccepted)
{
    EXPECT_EQ(statusOf("M\r\n2\r\n\r\n0 1\r\n1 0\r\n\r\n"), CollegeMap::LoadStatus::Ok);
}

TEST(CM2_Loading, FailedLoadLeavesFreshObjectEmpty)
{
    TempFile file("M\n2\n0 1\n2 0\n");
    CollegeMap map(file.name());
    EXPECT_FALSE(map.isLoaded());
    EXPECT_EQ(map.nodes(), 0);
}

TEST(CM2_Loading, FailedReloadKeepsPreviousMap)
{
    CollegeMap map = loadFrom(MtuMap);
    const CollegeMap before(map);

    TempFile bad("M\n2\n0 -1\n-1 0\n");
    EXPECT_EQ(map.load(bad.name()), CollegeMap::LoadStatus::WeightOutOfRange);
    EXPECT_TRUE(map.isLoaded());
    EXPECT_TRUE(map == before);
    EXPECT_EQ(map.status(), CollegeMap::LoadStatus::WeightOutOfRange);
}

TEST(CM2_Loading, SuccessfulReloadReplacesMap)
{
    CollegeMap map = loadFrom(MtuMap);
    TempFile tri(Triangle);
    EXPECT_EQ(map.load(tri.name()), CollegeMap::LoadStatus::Ok);
    EXPECT_EQ(map.name(), "Triangle");
    EXPECT_EQ(map.nodes(), 3);
    EXPECT_EQ(map(0, 2), 4);
}

// ==================================================================
// CM3 Storage / CM4 Metadata
// ==================================================================
TEST(CM3_Storage, AllWeightsStoredInRightPlace)
{
    CollegeMap map = loadFrom(MtuMap);
    EXPECT_EQ(map(0, 1), 4);
    EXPECT_EQ(map(0, 7), 8);
    EXPECT_EQ(map(1, 7), 11);
    EXPECT_EQ(map(3, 5), 14);
    EXPECT_EQ(map(8, 7), 7);
    EXPECT_EQ(map(8, 8), 0);
    int total = 0;
    for (int r = 0; r < map.nodes(); ++r)
        for (int c = 0; c < map.nodes(); ++c)
            total += map(r, c);
    EXPECT_EQ(total, 2 * (4 + 8 + 8 + 11 + 7 + 4 + 2 + 9 + 14 + 10 + 2 + 1 + 6 + 7));
}

TEST(CM4_Metadata, NameAndNodeCount)
{
    CollegeMap map = loadFrom(MtuMap);
    EXPECT_EQ(map.name(), "MTU_Map");
    EXPECT_EQ(map.nodes(), 9);
}

TEST(CM4_Metadata, MetadataIsDisplayable)
{
    CollegeMap map = loadFrom(MtuMap);
    std::ostringstream os;
    map.writeMetadata(os);
    EXPECT_NE(os.str().find("MTU_Map"), std::string::npos);
    EXPECT_NE(os.str().find("Nodes: 9"), std::string::npos);
}

// ==================================================================
// CM5 Weight access and modification
// ==================================================================
TEST(CM5_Weights, ElementOperatorMatchesWeight)
{
    CollegeMap map = loadFrom(MtuMap);
    for (int r = 0; r < map.nodes(); ++r)
        for (int c = 0; c < map.nodes(); ++c)
            EXPECT_EQ(map(r, c), map.weight(r, c));
}

TEST(CM5_Weights, SetWeightIsUndirected)
{
    CollegeMap map = loadFrom(MtuMap);
    map.setWeight(2, 8, 20);
    EXPECT_EQ(map(2, 8), 20);
    EXPECT_EQ(map(8, 2), 20);
}

TEST(CM5_Weights, SetWeightZeroRemovesEdge)
{
    CollegeMap map = loadFrom(MtuMap);
    map.setWeight(0, 1, 0);
    EXPECT_EQ(map(0, 1), 0);
    EXPECT_EQ(map(1, 0), 0);
}

TEST(CM5_Weights, OutOfRangeIndexThrows)
{
    CollegeMap map = loadFrom(MtuMap);
    EXPECT_THROW(map(-1, 0), std::out_of_range);
    EXPECT_THROW(map(0, 9), std::out_of_range);
    EXPECT_THROW(map.setWeight(9, 0, 1), std::out_of_range);
    EXPECT_THROW(map.setWeight(0, -1, 1), std::out_of_range);
}

TEST(CM5_Weights, InvalidWeightThrowsAndLeavesMapUnchanged)
{
    CollegeMap map = loadFrom(MtuMap);
    const CollegeMap before(map);
    EXPECT_THROW(map.setWeight(0, 1, -5), std::invalid_argument);
    EXPECT_THROW(map.setWeight(0, 1, CollegeMap::MaxWeight + 1), std::invalid_argument);
    EXPECT_THROW(map.setWeight(3, 3, 2), std::invalid_argument);
    EXPECT_TRUE(map == before);
}

TEST(CM5_Weights, AccessOnEmptyMapThrows)
{
    CollegeMap map;
    EXPECT_THROW(map(0, 0), std::logic_error);
    EXPECT_THROW(map.setWeight(0, 0, 0), std::logic_error);
}

// ==================================================================
// CM6 Display
// ==================================================================
TEST(CM6_Display, StreamShowsMetadataAndMatrix)
{
    const std::string out = text(loadFrom(Triangle));
    EXPECT_NE(out.find("Triangle"), std::string::npos);
    EXPECT_NE(out.find("Nodes: 3"), std::string::npos);
    EXPECT_NE(out.find("    0    1    4"), std::string::npos);   // row 0
    EXPECT_NE(out.find("    4    2    0"), std::string::npos);   // row 2
}

// ==================================================================
// CM7 Copying
// ==================================================================
TEST(CM7_Copying, CopyConstructorMakesEqualMap)
{
    CollegeMap original = loadFrom(MtuMap);
    CollegeMap copy(original);
    EXPECT_TRUE(copy == original);
}

TEST(CM7_Copying, CopyConstructorIsDeep)
{
    CollegeMap original = loadFrom(MtuMap);
    CollegeMap copy(original);
    copy.setWeight(0, 1, 99);
    EXPECT_EQ(original(0, 1), 4);
    EXPECT_TRUE(copy != original);
}

TEST(CM7_Copying, AssignmentIsDeep)
{
    CollegeMap original = loadFrom(MtuMap);
    CollegeMap assigned = loadFrom(Triangle);
    assigned = original;
    EXPECT_TRUE(assigned == original);
    assigned.setWeight(2, 8, 50);
    EXPECT_EQ(original(2, 8), 2);
}

TEST(CM7_Copying, ModifyingOriginalDoesNotAffectCopy)
{
    CollegeMap original = loadFrom(MtuMap);
    CollegeMap copy;
    copy = original;
    original.setWeight(0, 7, 1);
    EXPECT_EQ(copy(0, 7), 8);
}

TEST(CM7_Copying, SelfAssignmentIsSafe)
{
    CollegeMap map = loadFrom(MtuMap);
    const CollegeMap before(map);
    CollegeMap& alias = map;
    map = alias;
    EXPECT_TRUE(map == before);
}

TEST(CM7_Copying, ChainedAssignment)
{
    CollegeMap a, b;
    const CollegeMap c = loadFrom(Triangle);
    a = b = c;
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(b == c);
}

TEST(CM7_Copying, FailedCopyDuringAssignmentLeavesTargetUnchanged)
{
    CollegeMap target = loadFrom(Triangle);
    const CollegeMap before(target);
    const CollegeMap source = loadFrom(MtuMap);

    AllocationTracker tracker;
    alloc::failOnCall(1);
    EXPECT_THROW(target = source, std::bad_alloc);
    EXPECT_TRUE(target == before);
    EXPECT_EQ(tracker.live(), 0);
}

// ==================================================================
// CM8 Supplying weights to other objects
// ==================================================================
TEST(CM8_SupplyingWeights, DijkstraSeesCurrentWeightsWithoutOwningThem)
{
    CollegeMap map = loadFrom(Triangle);
    Dijkstra d(map);
    d.run(0);
    EXPECT_EQ(d.distanceTo(2), 3);       // 0 -> 1 -> 2
    map.setWeight(0, 2, 1);
    d.run(0);
    EXPECT_EQ(d.distanceTo(2), 1);       // direct edge now cheaper
}

// ==================================================================
// CM9 Release
// ==================================================================
TEST(CM9_Release, EveryAllocationReleasedOnNormalUse)
{
    TempFile file(MtuMap);
    TempFile tri(Triangle);
    AllocationTracker tracker;
    {
        CollegeMap a(file.name());
        CollegeMap b(a);
        CollegeMap c;
        c = a;               // replaces empty storage
        c = b;               // replaces existing storage
        b.load(tri.name());  // reload replaces storage
        a.clear();
        CollegeMap moved(std::move(b));
    }
    EXPECT_GT(tracker.allocations(), 0);
    EXPECT_EQ(tracker.live(), 0);
}

TEST(CM9_Release, NothingLeaksWhenAFileFailsToLoad)
{
    const char* badFiles[] = {
        "M\n3\n0 1 4\n1 0 2\n",            // too few rows
        "M\n3\n0 1 4\n1 0\n4 2 0\n",        // short row
        "M\n2\n0 x\nx 0\n",                // non-numeric
        "M\n2\n0 -1\n-1 0\n",              // negative
        "M\n2\n5 1\n1 0\n",                // diagonal
        "M\n2\n0 1\n2 0\n",                // asymmetric
        "M\n2\n0 1\n1 0\n7\n",             // trailing data
    };
    CollegeMap map = loadFrom(Triangle);

    for (const char* content : badFiles)
    {
        TempFile file(content);
        AllocationTracker tracker;
        {
            CollegeMap fresh(file.name());     // failed construction
            map.load(file.name());             // failed reload of a loaded map
        }
        EXPECT_EQ(tracker.live(), 0) << content;
    }
}

// ==================================================================
// D1 Association (aggregation)
// ==================================================================
TEST(D1_Association, CannotBeBuiltFromATemporaryMap)
{
    static_assert(!std::is_constructible<Dijkstra, CollegeMap>::value,
                  "a Dijkstra must not bind to a temporary CollegeMap");
    static_assert(std::is_constructible<Dijkstra, const CollegeMap&>::value,
                  "a Dijkstra is built from an existing CollegeMap");
    SUCCEED();
}

TEST(D1_Association, RefersToTheSameMapObject)
{
    CollegeMap map = loadFrom(Triangle);
    Dijkstra d(map);
    EXPECT_EQ(&d.map(), &map);
}

// ==================================================================
// D2 Working storage
// ==================================================================
TEST(D2_WorkingStorage, ArraysFollowTheMapSizeOnEachRun)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);
    EXPECT_EQ(d.distanceTo(8), 14);

    TempFile tri(Triangle);
    map.load(tri.name());                 // 9 nodes -> 3 nodes
    EXPECT_TRUE(d.isStale());
    EXPECT_THROW(d.distanceTo(8), std::logic_error);   // never reads old arrays
    d.run(2);
    EXPECT_EQ(d.distanceTo(0), 3);
    EXPECT_THROW(d.distanceTo(8), std::out_of_range);
}

TEST(D2_WorkingStorage, ModifiedMapMakesResultsStale)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);
    EXPECT_FALSE(d.isStale());
    map.setWeight(6, 7, 20);
    EXPECT_TRUE(d.isStale());
    EXPECT_THROW(d.routeTo(6), std::logic_error);
    d.run(0);
    EXPECT_FALSE(d.isStale());
    EXPECT_EQ(d.routeTo(6), "0 -> 1 -> 2 -> 5 -> 6");
    EXPECT_EQ(d.distanceTo(6), 18);
}

TEST(D2_WorkingStorage, FailedAllocationKeepsPreviousResultsAndLeaksNothing)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);

    for (int failing = 1; failing <= 3; ++failing)   // distance, processed, parent
    {
        AllocationTracker tracker;
        alloc::failOnCall(failing);
        EXPECT_THROW(d.run(3), std::bad_alloc);
        EXPECT_EQ(tracker.live(), 0) << "failing allocation " << failing;
        EXPECT_EQ(d.source(), 0);
        EXPECT_EQ(d.distanceTo(4), 21);
    }
}

// ==================================================================
// D3 Execution
// ==================================================================
TEST(D3_Execution, MatchesSampleOutputFromNode0)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);
    const int distance[] = { 0, 4, 12, 19, 21, 11, 9, 8, 14 };
    const char* route[] = { "0", "0 -> 1", "0 -> 1 -> 2", "0 -> 1 -> 2 -> 3",
                            "0 -> 7 -> 6 -> 5 -> 4", "0 -> 7 -> 6 -> 5",
                            "0 -> 7 -> 6", "0 -> 7", "0 -> 1 -> 2 -> 8" };
    for (int node = 0; node < 9; ++node)
    {
        EXPECT_EQ(d.distanceTo(node), distance[node]) << "node " << node;
        EXPECT_EQ(d.routeTo(node), route[node]) << "node " << node;
    }
}

// Reference-code defect 1: parent[0] = -1 instead of parent[src] = -1.
// With source 3 the original read an uninitialised parent[3] while
// printing the route and crashed (segfault) or printed garbage.
TEST(D3_Execution, Defect1_RoutesFromANonZeroSourceAreCorrect)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(3);
    EXPECT_EQ(d.routeTo(4), "3 -> 4");
    EXPECT_EQ(d.distanceTo(4), 9);
    EXPECT_EQ(d.routeTo(0), "3 -> 2 -> 1 -> 0");
    EXPECT_EQ(d.distanceTo(0), 19);
    EXPECT_EQ(d.routeTo(3), "3");
    EXPECT_EQ(d.distanceTo(3), 0);
}

TEST(D3_Execution, RepeatedRunsOnSameObjectGiveFreshResults)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);
    EXPECT_EQ(d.distanceTo(4), 21);
    d.run(4);
    EXPECT_EQ(d.source(), 4);
    EXPECT_EQ(d.distanceTo(4), 0);
    EXPECT_EQ(d.distanceTo(0), 21);
    EXPECT_EQ(d.routeTo(0), "4 -> 5 -> 6 -> 7 -> 0");
    d.run(0);
    EXPECT_EQ(d.routeTo(4), "0 -> 7 -> 6 -> 5 -> 4");
}

TEST(D3_Execution, EveryRouteIsARealPathOfTheReportedLength)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    for (int s = 0; s < map.nodes(); ++s)
    {
        d.run(s);
        for (int t = 0; t < map.nodes(); ++t)
            EXPECT_EQ(routeLength(map, d.routeTo(t)), d.distanceTo(t))
                << "from " << s << " to " << t;
    }
}

TEST(D3_Execution, AgreesWithFloydWarshallForAllPairs)
{
    CollegeMap map = loadFrom(MtuMap);
    const int n = 9;
    const int inf = 1000000000;
    int best[n][n];
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            best[i][j] = (i == j) ? 0 : (map(i, j) > 0 ? map(i, j) : inf);
    for (int k = 0; k < n; ++k)
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                if (best[i][k] + best[k][j] < best[i][j])
                    best[i][j] = best[i][k] + best[k][j];

    Dijkstra d(map);
    for (int s = 0; s < n; ++s)
    {
        d.run(s);
        for (int t = 0; t < n; ++t)
            EXPECT_EQ(d.distanceTo(t), best[s][t]) << "from " << s << " to " << t;
    }
}

TEST(D3_Execution, InvalidSourceThrows)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    EXPECT_THROW(d.run(-1), std::out_of_range);
    EXPECT_THROW(d.run(9), std::out_of_range);
    EXPECT_FALSE(d.hasRun());
}

TEST(D3_Execution, RunOnEmptyMapThrows)
{
    CollegeMap empty;
    Dijkstra d(empty);
    EXPECT_THROW(d.run(0), std::logic_error);
}

TEST(D3_Execution, SingleNodeMap)
{
    CollegeMap map = loadFrom("Solo\n1\n0\n");
    Dijkstra d(map);
    d.run(0);
    EXPECT_EQ(d.distanceTo(0), 0);
    EXPECT_EQ(d.routeTo(0), "0");
}

// ==================================================================
// D4 Results
// ==================================================================
// Reference-code defect 2: minDistance() used "<=" against INT_MAX, so
// once only unreachable nodes were left it picked one and
// dist[u] + weight overflowed. The original gave node 2 of this map a
// distance of -2147483646 and the route "0 2".
TEST(D4_Results, Defect2_UnreachableNodesAreNotGivenADistance)
{
    CollegeMap map = loadFrom(TwoIslands);
    Dijkstra d(map);
    d.run(0);
    EXPECT_TRUE(d.isReachable(1));
    EXPECT_EQ(d.distanceTo(1), 5);
    for (int node = 2; node <= 3; ++node)
    {
        EXPECT_FALSE(d.isReachable(node)) << "node " << node;
        EXPECT_EQ(d.distanceTo(node), Dijkstra::NoRoute) << "node " << node;
        EXPECT_EQ(d.routeTo(node), "unreachable") << "node " << node;
    }
}

TEST(D4_Results, UnreachableShownMeaningfullyInTable)
{
    CollegeMap map = loadFrom(TwoIslands);
    Dijkstra d(map);
    d.run(2);
    std::ostringstream os;
    d.writeResults(os);
    EXPECT_NE(os.str().find("unreachable"), std::string::npos);
    EXPECT_EQ(os.str().find("2147483647"), std::string::npos);
    EXPECT_EQ(os.str().find("-2147483"), std::string::npos);
}

TEST(D4_Results, TableListsEveryOtherNode)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);
    std::ostringstream os;
    d.writeResults(os);
    const std::string out = os.str();
    EXPECT_NE(out.find("Shortest routes from node 0"), std::string::npos);
    EXPECT_NE(out.find("0 -> 7 -> 6 -> 5 -> 4"), std::string::npos);
    EXPECT_NE(out.find("0 -> 1 -> 2 -> 8"), std::string::npos);
}

TEST(D4_Results, QueryBeforeRunThrows)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    EXPECT_FALSE(d.hasRun());
    EXPECT_THROW(d.distanceTo(1), std::logic_error);
    EXPECT_THROW(d.routeTo(1), std::logic_error);
    std::ostringstream os;
    EXPECT_THROW(d.writeResults(os), std::logic_error);
}

TEST(D4_Results, BadDestinationThrows)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);
    EXPECT_THROW(d.distanceTo(9), std::out_of_range);
    EXPECT_THROW(d.routeTo(-1), std::out_of_range);
}

// ==================================================================
// D5 Saving results
// ==================================================================
TEST(D5_Saving, WritesTheSameTableToTheNamedFile)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);

    const std::string name = "cm_test_results.txt";
    ASSERT_TRUE(d.saveResults(name));
    std::ostringstream expected;
    d.writeResults(expected);
    EXPECT_EQ(readFile(name), expected.str());
    std::remove(name.c_str());
}

TEST(D5_Saving, ReportsAFileThatCannotBeWritten)
{
    CollegeMap map = loadFrom(MtuMap);
    Dijkstra d(map);
    d.run(0);
    EXPECT_FALSE(d.saveResults("no_such_folder/results.txt"));
}

// ==================================================================
// D6 Copying
// ==================================================================
TEST(D6_Copying, DijkstraCannotBeCopiedOrAssigned)
{
    static_assert(!std::is_copy_constructible<Dijkstra>::value, "copying is disabled");
    static_assert(!std::is_copy_assignable<Dijkstra>::value, "assignment is disabled");
    SUCCEED();
}

// ==================================================================
// D7 Release
// ==================================================================
TEST(D7_Release, DestroyingDijkstraLeavesMapIntactAndUsable)
{
    CollegeMap map = loadFrom(MtuMap);
    const CollegeMap before(map);
    {
        Dijkstra d(map);
        d.run(0);
    }
    EXPECT_TRUE(map == before);
    map.setWeight(0, 1, 5);              // still usable
    Dijkstra again(map);
    again.run(0);
    EXPECT_EQ(again.distanceTo(1), 5);
}

TEST(D7_Release, DijkstraReleasesExactlyWhatItAllocated)
{
    CollegeMap map = loadFrom(MtuMap);
    AllocationTracker tracker;
    {
        Dijkstra d(map);
        d.run(0);                        // 3 arrays
        d.run(4);                        // 3 new arrays, old 3 released
    }
    EXPECT_EQ(tracker.allocations(), 6);
    EXPECT_EQ(tracker.releases(), 6);
}

TEST(D7_Release, UnusedDijkstraAllocatesNothing)
{
    CollegeMap map = loadFrom(Triangle);
    AllocationTracker tracker;
    {
        Dijkstra d(map);
    }
    EXPECT_EQ(tracker.allocations(), 0);
    EXPECT_EQ(tracker.live(), 0);
}
