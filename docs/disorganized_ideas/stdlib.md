# Stdlib

## Data Structures

### `std::dynarr#T`

Standard "Dynamic Array" data structure. Similar to `std::vector<T>` in C++.

Wasted Space: O(N)

### `std::geoarr#T`

Standard "Geometric Array" data structure. Contains a segment array with 1, 2, 4, 8... etc. elements in each segment.

Similar behavior to `std::dynarr#T`, but with O(log N) worst case push_back/pop_back and elements stay in-place.

Wasted Space: O(N)

### `std::dougarr#T`

Standard "Doubled Geometric Array". This structure can greatly reduce wasted space compared to `std::dynarr#T` or 
`std::geoarr#T` at the expense of being slower.

Wasted Space: O(root(N))

