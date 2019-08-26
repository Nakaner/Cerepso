Cerepso-Postgres-Backend
========================

Cerepso-Postgres-Backend is the collection of classes used by both [Cerepso](https://github.com/Nakaner/Cerepso) and
[Cerepso2vt](https://github.com/Nakaner/Cerepso2vt) for database access. It is header-only C++ 11 library for
read-only access to a PostgreSQL database (with PostGIS and hstore).

Dependencies
============

* libpq-dev (libpq)
* libboost-dev (format.hpp)
* C++11 capable compiler.

Optional, necessary for building the documentation:

* cmake 
* doxygen

License
=======
This code is available under the terms of GNU General Public License version 2 (GPLv2). See [COPYING.txt](COPYING.txt)
for the full legal code. This project contains code from the osm2pgsql project which is available under the terms
of GPLv2.


Documentation
=============
You need Doxygen to build the documentation. It is build using:

    mkdir build
    cmake ..
    make doc
