There are multiple ways to build geometries for PostGIS and to write them to the database.


Building Geometries
===================

* WKT via Osmium's WKTFactory
* WKB via Osmium's WKBFactory
* WKB (hex) via Osmiums' WKBFactory
* for simple geometries: building WKT manually

Following time is consumed using libpq to write 39709 nodes and surrounding it by "BEGIN" and "COMMIT":

| method | time | comments
|--------+------+---------
| WKB hex | 5.3 s | `INSERT into nodes (osm_id, geom) VALUES (1347868146, 'SRID=4326;01010000000C4CB8A173BC2040DA5E6633E27F4840');`
| WKT (wktFactory) | 6.45 s | `INSERT into nodes (osm_id, geom) VALUES (365874, ST_GeometryFromText('WKT', 4326));`
| WKT (manually)   | 6.43 s | `INSERT into nodes (osm_id, geom) VALUES (365874, ST_GeometryFromText('WKT', 4326));`

WKB hex variant needs 0.25 s if it is in COPY mode.
