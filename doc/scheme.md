Database Schema
===============

pgimporter uses a database schema which is inspired by osm2pgsql.

There are following tables:
* osm_nodes
* osm_ways_linear
* osm_ways_area
* osm_relation_multipolygon
* osm_relation_other

All tables have at least following columns:

| name     | type     | description
| ---------+----------+-----------
| osm_id   | bigint   | OSM object ID
| tags     | hstore   | tags as hstore
| osm_user | text     | OSM username
| osm_uid  | bigint   | OSM user ID
| osm_version | bigint | OSM object version
| osm_lastmodified | | last_modified timestamp
| osm_changeset    | | OSM changeset ID

All geometry columns are called way, they have following types:
* `osm_nodes`: Point(2)
* `osm_ways_linear`: LineString(2)
* `osm_ways_area`: Polygon(2)
* `osm_relation_multipolygon`: Geometry(2)
* `osm_relation_other`: GeometryCollection(2)

osm_ways_* tables have additional columns about their members:
* `way_nodes` bigint[]

osm_relation_* tables have additional columns about their members:
* `relation_members` COMPOSITE relmembers (`member_id bigint, `member_type` char(1))

All tables can have additional columns for tags which you want to have in a separate column instead of the hstore colum

