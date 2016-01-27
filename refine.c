#include "refine.h"

#include "arrays.h"
#include "loop.h"
#include "measure_edges.h"
#include "mesh.h"
#include "refine_common.h"
#include "tag.h"

unsigned refine_by_size(struct mesh** p_m, double qual_floor)
{
  struct mesh* m = *p_m;
  double const* coords = mesh_find_tag(m, 0, "coordinates")->d.f64;
  unsigned const* verts_of_edges = mesh_ask_down(m, 1, 0);
  unsigned nedges = mesh_count(m, 1);
  double const* sizes = mesh_find_tag(m, 0, "adapt_size")->d.f64;
  double* edge_sizes = measure_edges(nedges, verts_of_edges, coords, sizes);
  unsigned* candidates = LOOP_MALLOC(unsigned, nedges);
  for (unsigned i = 0; i < nedges; ++i)
    candidates[i] = edge_sizes[i] > 1.0;
  loop_free(edge_sizes);
  mesh_add_tag(m, 1, TAG_U32, "candidate", 1, candidates);
  unsigned ret = refine_common(p_m, 1, qual_floor, 0);
  return ret;
}

void uniformly_refine(struct mesh** p_m)
{
  struct mesh* m = *p_m;
  unsigned nedges = mesh_count(m, 1);
  mesh_add_tag(m, 1, TAG_U32, "candidate", 1, uints_filled(nedges, 1));
  refine_common(p_m, 1, 0.0, 0);
}