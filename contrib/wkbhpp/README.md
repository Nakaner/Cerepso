# WKBHPP

WKBHPP is a header-only C++ library to encode geometries as [Well Known
Binary](https://en.wikipedia.org/wiki/Well-known_text#Well-known_binary) (WKB).

The code of this library originates from the [Libosmium](https://osmcode.org/libosmium/) library but
has been slightly adapted to be useful as a generic library to create WKB.


## License and Authors

This library was developed by Geofabrik GmbH mainly using code by Jochen Topf.

WKBHPP is available under the Boost Software License. See LICENSE.txt.


## Dependencies

Only a C++11 compiler and the standard library are required.

Building and running the unit tests requires the Catch library which is shipped with this library.


## Building

To build the unit tets, run:

```sh
mkdir build
cd build
cmake ..
make
```

Execute the unit tests using `make test`.
