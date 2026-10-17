# AISolution

The solution an AI tool generated for Part D of the assignment. None of this is my code apart from `PortedTests`. It's kept in its own solution so it can't affect `RoutePlanner.sln`.

Open `AISolution.slnx` in Visual Studio, then Build > Rebuild Solution and Test > Test Explorer > Run All.

| Project | What it is |
| --- | --- |
| `CollegeMapLib` | The AI's `CollegeMap` and `Dijkstra` classes, as a static library |
| `Demo` | The AI's demo program. It needs `MTU Map.txt` and `MTU Map (corrected).txt` in this folder |
| `Tests` | The AI's own 65 tests. All 65 pass |
| `PortedTests` | My 36 tests from `MapTests`, changed to call the AI's functions. 24 pass and 12 fail |

The AI rejects `MTU Map.txt` from the brief because the matrix isn't symmetric, and loads its own corrected copy instead. That causes 8 of the 12 failures. The rest are output layout and save-file differences. Section 3 of the report goes through each failure.
