Logged table, Baden-Württemberg, no indexes: 7:36 min

Unlogged table, same area, no indexes: 4:08 min

Import and indexes on all tables (including untagged nodes): 15:56 min on Baden-Württemberg

Import indexes on all tables except untagged nodes: 6:39 min on Baden-Württemberg

Ordering by ST_GeoHash, except untagged nodes, and creation of an geometry indes: 11:46 min on Baden-Württemberg

osm2pgsql
=========

Comparison osm2pgsql, default settings with `--slim`: 38:55 min, 686 MB RAM

Comparison osm2pgsql, default settings with `--slim --multi-geometry`: 35:42 min, 683 MB RAM

Comparison osm2pgsql, default settings with `--slim --multi-geometry --exclude-invalid-polygon`: 39:27, 688 MB RAM

osm2pgsql, globe, planet 2016-08-29, `--create --number-processes 5 --style (OpenRailwayMap) --merc --exclude-invalid-polygon --unlogged --cache-strategy dense --hstore --hstore-match-only --cache 54000` 2:40, almost all RAM


Buffered vs. Unbuffered Writing to Database
===========================================
1024 Byte
---------
globe, Thüringen, 86 MB PBF, 458512 kB RAM, 1:04
globe, Thüringen, 86 MB PBF, 476024 kB RAM, 1:08
globe, Thüringen, 86 MB PBF, 451940 kB RAM, 1:13
hatano, Thüringen, 86 MB PBF, 351760 kB RAM, 1:36
hatano, Thüringen, 86 MB PBF, 346672 kB RAM, 1:33
hatano, Thüringen, 86 MB PBF, 354212 kB RAM, 1:39

Table objects were created before Handler was intialized:
globe, planet, 67700592 kB RAM, 11:06:51 (no order by ST_GeoHash, no import of usernames), no BEGIN/COMMIT before COPY

2048 Byte
---------
hatano, Thüringen 86 MB PBF, 346808 kB RAM, 1:31
globe, Thüringen 86 MB PBF, 469040 kB RAM, 1:03
globe, Thüringen 86 MB PBF, 456792 kB RAM, 1:06
hatano, Thüringen 86 MB PBF, 353472 kB RAM, 1:33

Table objects were created before Handler was intialized:
globe, Bayern 507 MB PBF, 1400304 kB RAM, 7:18
globe, Germany 2850 MB PBF , 6040100 kB RAM, 39:24
globe, Germany 2850 MB PBF, 6035596 kB RAM, 39:34

Table objects were created by the Handler (1cd289f0dc2ed440799d072b2d73fa2289730733):
globe, Germany 2850 MB PBF, kB RAM, 42:09


Unbuffered
----------
globe, Europe, 41640924 kB RAM (Cerepso)+, 1:29:08,
  details europe-4455683967af64ab9aacf39cafbb77acbd046fdd.log

globe, planet, no BEGIN/COMMIT before COPY, 67812140 kB RAM, 10:50, (no order by ST_GeoHash, no import of usernames)

globe, planet original from planet.osm.org of 2016-08-29, 7:06 (no order by ST_GeoHash, no import of usernames), 47800808 kB RAM (only Cerepso!), 7:06

globe, planet original from planet.osm.org of 2016-08-29, 7:06 (no order by ST_GeoHash, no import of usernames, no geometry index on table untagged_nodes); Cerepso needed 47828968 kB RAM (+ Postgres usage)

similar but on commit d4e4627, needs 8:34, 68549560 kB RAM

similar but on commit 801f47b, needs 7:11, 68440016 kB RAM


Diff Import
==========
Baden-Württemberg, diff, 2016-08-02, 728 kB gzip compressed XML, 22.1 sec., 46668 kB RAM
Baden-Württemberg, diff, 2016-08-02–2016-09-26, 9:13, 39.0 kB RAM, diff handler with two passes, unbuffered writing, prepared statements
Baden-Württemberg, dto., 11:04, 60.2 kB RAM, diff handler with only one pass, caching of all inserts via COPY until whole diff is read, prepared statements


