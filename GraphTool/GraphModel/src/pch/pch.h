#pragma once

// C/C++ standard library headers
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <numeric>
#include <vector>
#include <string>
#include <thread>
#include <deque>
#include <queue>
#include <array>
#include <cmath>
#include <span>

#include "../../include/graph_model.h"

// External library headers
#include <lz4frame.h>

#ifndef __EMSCRIPTEN__
#include <parallel_hashmap/phmap.h>

#include <osmium/geom/haversine.hpp>
#include <osmium/geom/mercator_projection.hpp>

#include <osmium/io/pbf_input.hpp>
#include <osmium/io/xml_input.hpp>
#endif
