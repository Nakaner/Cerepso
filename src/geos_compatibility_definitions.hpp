/*
 * geos_compatibility_definitions.hpp
 *
 *  Created on:  2019-08-26
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef GEOS_COMPATIBILITY_DEFINITIONS_HPP_
#define GEOS_COMPATIBILITY_DEFINITIONS_HPP_

#include <geos/version.h>
#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && (GEOS_VERSION_MAJOR > 3 || (GEOS_VERSION_MAJOR == 3 && GEOS_VERSION_MINOR >= 6))
#define GEOS_36
#endif

#include <memory>
#include <geos/geom/GeometryFactory.h>

// GEOS_36 is defined by osmium_geos_factory
#ifdef GEOS_36
/**
 * The destructor of geos::geom::GeometryFactory is protected
 * since GEOS 3.6 and was replaced by the method destroy().
 */
struct GEOSGeometryFactoryDeleter {
    void operator()(geos::geom::GeometryFactory* f) {
        f->destroy();
    }
};
#endif

#ifdef GEOS_36
    using geos_factory_type = std::unique_ptr<geos::geom::GeometryFactory, GEOSGeometryFactoryDeleter>;
#else
    using geos_factory_type = std::unique_ptr<geos::geom::GeometryFactory>;
#endif


#endif /* GEOS_COMPATIBILITY_DEFINITIONS_HPP_ */
