@mainpage

pgimporter is a tool to import OSM data into a PostgreSQL database (with
PostGIS and hstore extension) and to keep the database up to date with
minutely, hourly or daily diff files provided by OSM itself or third parties
like Geofabrik.

pgimporter is inspired by osm2pgsql and tries to be as compatible as possible
with it but removes some "dirty hacks". See differences-from-osm2pgsql for details.
Small parts of osm2pgsql source code and almost all database optimization techniques
have been reused by pgimporter.

pgimporter makes much use of the Osmium library for reading OSM data and building
geometries.

Its written in C++11 and available under GPLv2.

