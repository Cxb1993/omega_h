#ifndef COARSEN_BY_SIZE_H
#define COARSEN_BY_SIZE_H

#include "mesh.h"

unsigned coarsen_by_size(
    struct mesh** p_m,
    double quality_floor,
    double size_ratio_floor);

#endif
