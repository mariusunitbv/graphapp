#pragma once

#include <execution>
#include <fstream>
#include <ranges>
#include <queue>
#include <stack>

#include <QtOpenGLWidgets>
#include <QtConcurrent>
#include <QtWidgets>

// External
#include <osmium/geom/haversine.hpp>
#include <osmium/geom/mercator_projection.hpp>

#include <osmium/io/pbf_input.hpp>
#include <osmium/visitor.hpp>

#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>

#include <simdjson.h>
