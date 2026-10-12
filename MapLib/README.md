# MapLib

Static library with the two classes. MapDemo and MapTests both link against it.

- `CollegeMap.h` / `.cpp` loads the map file, checks it and stores the weights. It also has the copy constructor, `=`, `==`, `!=` and the `<<` operator for printing.
- `Dijkstra.h` / `.cpp` runs the algorithm on a `CollegeMap`. You can get the route to one node with `getRoute()`, print every route with `<<`, or save them to a file.
