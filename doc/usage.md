Command Line Options
====================

Indexing and Performance
------------------------

`-a, --all-geom-indexes` will create an geometry index on all tables. If this option is *not* given,
the untagged nodes' table will not get an geometry index. You will only need this option if you perform
data analysis and want to work with untagged orphaned nodes. If you just want to analys or render ways,
you will not need this option. Using this option increases the duration of the first import about 100%.

`-o, --no-order-by` will disable reordering tables after first import by `ST_Geohash`. This option reduces disk IO if your
request are similar to those of a tileserver. See a [talk](https://vimeo.com/115315282) at State of the Map 2014 by Christian Quest
([slides](https://pdf.yt/d/P0vxShtbGagwXg3Q)) and a [discussion](https://github.com/openstreetmap/osm2pgsql/issues/208) at
osm2pgsql issue tracker about this topic.