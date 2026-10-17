# MapTests

Google Test project with 36 tests covering CM1 to CM9 and D1 to D7 from the brief, plus one test for each bug in the reference code. D6 has no runtime test because copying a Dijkstra object is blocked with `= delete`, so `Dijkstra b = a;` won't compile.

Each fixture writes its own test map in `SetUp()` and deletes it in `TearDown()`, so the tests don't depend on `MTU Map.txt` or on which folder they run from. The comment above each test says which requirement it covers.

Run them from Test > Test Explorer > Run All.
