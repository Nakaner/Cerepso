pg-array-hstore-parser
======================

pg-array-hstore-parser is a header-only C++ 11 library to parse the string representation of arrays and hstore
fields of a PostgreSQL database. If you don't want to run the unit tests, you don't have to care for any dependencies.

Dependencies
============

* Catch testing framework (included in this repository)
* cmake (to build the tests)
* Doxygen (to build the documentation)
* no other dependencies  except the standard library and a C++11 capable compiler.


Unit Tests
==========

Unit tests are located in the `test/` directory. The Catch framework is used, all
necessary dependencies are included in this repository.

Build the tests:
```mkdir build
cd build
cmake ..
make```

Run the tests:
```
make test
```

Usage Examples
==============

This is a simple usage example using libpq for the database access.

Hstore Parser
------------

```
PGConn *database_connection = PGconnectdb("dbname=mytestdb");
PGresult *result = PQexec(database_connection, "SELECT foo from testtable"); # assume that foo is a hstore column
int tuple_count = PQntuples(result);
for (int i = 0; i < tuple_count; i++) {
    std::string hstore_content = PQgetvalue(result, i, 0);
    pg_array_hstore_parser::HStoreParser hstore (hstore_content);
    while (hstore.has_next()) {
        pg_array_hstore_parser::StringPair kv_pair = hstore.get_next(); # StringPair is an alias for std::pair<std::string, std::string>
        std::cout << kv_pair.first << "->" << kv_pair.second << '\n';
    }
}
```

Array Parser
------------

The array parser is a template class which receives an implementation for the conversion from string to the output type you want.
The library currently provides output type converters for following types:

* `int64_t`
* `std::string`
* `char` (using the first character of the string representing an array element)

The library defines following aliases to reduce the amount of typing:
```
using StringConversion = TypeConversion<StringConversionImpl>;
using CharConversion = TypeConversion<CharConversionImpl>;
using Int64Conversion = TypeConversion<Int64ConversionImpl>;
```

This means you can instanciate an array parser which returns `int64_t` using:
```
pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> array_parser(array_content);
```
instead of
```
pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::TypeConversion<pg_array_hstore_parser::Int64Conversion>> array_parser(array_content);
```

You are free to implement your own conversion implementation for your user-defined type.

```
PGConn *database_connection = PGconnectdb("dbname=mytestdb");
PGresult *result = PQexec(database_connection, "SELECT foo from testtable"); # assume that foo is a `bigint[]` column
int tuple_count = PQntuples(result);
for (int i = 0; i < tuple_count; i++) {
    std::string array_content = PQgetvalue(result, i, 0);
    pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> array_parser(array_content);
    while (array_parser.has_next()) {
        int64_t mybigint = array_parser.get_next() + 1;
        // Do what you want to do.
    }
}
```

License
=======
see COPYING.md


To Do
=====
* performance improvements
* add an example who to write your own type converter

