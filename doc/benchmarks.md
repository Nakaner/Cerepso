Logged table, Baden-Württemberg, no indexes: 7:36 min

Unlogged table, same area, no indexes: 4:08 min

Import and indexes on all tables (including untagged nodes): 15:56 min on Baden-Württemberg

Import indexes on all tables except untagged nodes: 6:39 min on Baden-Württemberg

Ordering by ST_GeoHash, except untagged nodes, and creation of an geometry indes: 11:46 min on Baden-Württemberg

Comparison osm2pgsql, default settings with `--slim`: 38:55 min, 686 MB RAM

Comparison osm2pgsql, default settings with `--slim --multi-geometry`: 35:42 min, 683 MB RAM

Comparison osm2pgsql, default settings with `--slim --multi-geometry --exclude-invalid-polygon`: 39:27, 688 MB RAM

