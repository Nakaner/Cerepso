# Osmium GEOS Factory

Osmium GEOS Factory is a plugin for the [Osmium library version
2.x](http://osmcode.org/libosmium) (also known as Libosmium).

Osmium contained a factory for GEOS geometry objects originally but it was
removed because it used the GEOS C++ API which [is not considered to be
stable](https://lists.osgeo.org/pipermail/geos-devel/2017-January/007653.html).
This repository contains the factory and all modifications to make it work with
both GEOS 3.5 and GEOS 3.6.

## Usage

This library is a drop-in replacement for the `geos::geom::GEOSFactory` class of Libosmium.
The usage is equal to the other geometry factories (WKT, WKB, OGR) offered by Libosmium. Only the namespace is different.


``` c++
#include <osmium_geos_factory/geos_factory.hpp>
osmium_geos_factory::GEOSFactory<> factory;
```

You can also set the SRID used by GEOS (default is -1, unset):

``` c++
osmium_geos_factory::GEOSFactory<> factory{4326};
```

If this is not flexible enough for your case, you can also create a GEOS
factory yourself and then the libosmium factory from it:

``` c++
geos::geom::PrecisionModel geos_pm;
geos::geom::GeometryFactory geos_factory{&pm, 4326};
osmium_geos_factory::GEOSFactory<> factory{geos_factory};
```

Note: GEOS keeps a pointer to the factory it was created from in each geometry.
You have to make sure the factory is not destroyed before all the geometries
created from it have been destroyed!

All creation functions return a `unique_ptr` to the GEOS geometry:

``` c++
std::unique_ptr<geos::geom::Point> point = factory.create_point(node);
std::unique_ptr<geos::geom::LineString> line = factory.create_linestring(way);
std::unique_ptr<geos::geom::Polygon> polygon = factory.create_polygon(way);
std::unique_ptr<geos::geom::MultiPolygon> multipolygon = factory.create_multipolygon(area);
...
```

If you want to create GEOS geometries in a projection other than unprojected WGS84 (EPSG:4326),
you will have to add a template parameter:

```c++
#include <osmium/geom/projection.hpp>

osmium_geos_factory::GEOSFactory<osmium::geom::Projection> factory {osmium::geom::Projection(25832)};
```

If you want to transform to Web Mercator (EPSG:3857), you can use the faster Web Mercator projection
provided by Libosmium:

```c++
#include <osmium/geom/mercator_projection.hpp>

osmium_geos_factory::GEOSFactory<osmium::geom::MercatorProjection> factory {osmium::geom::MercatorProjection()};
```


## License

Osmium GEOS Factory is available under the Boost Software License. See
[LICENSE.txt](LICENSE.txt).

## Authors

The GEOSFactoryImpl class of Libosmium was written by Jochen Topf
(jochen@topf.org). Adaption to GEOS 3.6 was added by Michael Reichert
(michael.reichert@geofabrik.de).  See the git commit log for other authors.

