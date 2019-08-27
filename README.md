Cerepso
=======

Cerepso is a tool to import OSM data into a PostgreSQL database (with
PostGIS and hstore extension) and to keep the database up to date with
minutely, hourly or daily diff files provided by OSM itself or third parties
like Geofabrik.

Cerepso is inspired by osm2pgsql and tries to be as compatible as possible
with it but removes some "dirty hacks". See differences-from-osm2pgsql for details.
Small parts of osm2pgsql source code and almost all database optimization techniques
have been reused by Cerepso.

Cerepso makes much use of the Osmium library for reading OSM data and building
geometries.

Status
------

**This programme is experimental.** A larger refactoring has to be done to
ease maintanance and extensibility in future. Please expect bugs if you use this
programme.

The most important tasks to be done are:

* `PostgresHandler::prepare_query` and similar methods should go into a separate
  class or collection of functions because they are also required by
  RelationCollector and DatabaseLocationHandler
* many (mostly static) methods of `PostgresHandler` should be moved
  `PostgresTable`
* `PostgresTable` should be split up into one class per table type, i.e.
  `NodesTable`, `WaysTable`, `AreasTable`, `RelationsTable`, `NodeWaysTable`,
  `NodeRelationsTable`, `WayRelationsTable. `postgres_drivers::Table` should be a
  member of these classes. These classes should be class interfaces and the
  database implementation (i.e. `postgres_drivers::Table`) should be a template
  parameter.

Lacking features and minor issues can be found in the bugtracker of this
project. In addition, some "TODO" comments in the code give help where to start.

License
-------

Its written in C++11 and available under the terms of GNU General Public License 2
(GPLv2). See [COPYING.txt](COPYING.txt) for the full legal code. This program contains
code from osm2pgsql for escaping strings before inserting them into the database.

Dependencies
------------
* [Cerepso-Postgres-Backend](https://github.com/Nakaner/Cerepso-Postgres-Backend) (included in this repository)
* [Wkbhpp](https://github.com/Nakaner/wkbhpp) (included in this repository)
* GEOS library and its C++ API
* Osmium library v2 aka libosmium
* [osmium_geos_factory](https://github.com/Nakaner/osmium_geos_factory) (included in this repository)
* Catch2 (for testing only, included in this repository)
* Doxygen (to build the documentation)

Building
--------

    mkdir build
    cmake ..
    make

Unit Tests
----------

Unit tests are located in the test subdirectory. The Catch framework is used for unit tests.
Run `make test` in the `build/` directory to run the tests.
