Logged table, Baden-Württemberg, no indexes: 7:36 min

Unlogged table, same area, no indexes: 4:08 min

Import and indexes on all tables (including untagged nodes): 15:56 min on Baden-Württemberg

Import indexes on all tables except untagged nodes: 6:39 min on Baden-Württemberg

Ordering by ST_GeoHash, except untagged nodes, and creation of an geometry indes: 11:46 min on Baden-Württemberg

Comparison osm2pgsql, default settings with `--slim`: 38:55 min, 686 MB RAM

Comparison osm2pgsql, default settings with `--slim --multi-geometry`: 35:42 min, 683 MB RAM

Comparison osm2pgsql, default settings with `--slim --multi-geometry --exclude-invalid-polygon`: 39:27, 688 MB RAM


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

