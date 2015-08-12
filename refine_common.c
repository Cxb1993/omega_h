#include "refine_common.h"
#include "up_from_down.h"
#include "refine_qualities.h"
#include "reflect_down.h"
#include "refine_qualities.h"
#include "star.h"
#include "indset.h"
#include "ints.h"
#include "splits_to_elements.h"
#include "refine_topology.h"
#include "refine_nodal.h"
#include "concat.h"
#include <stdlib.h>

void refine_common(
    unsigned elem_dim,
    unsigned* p_nelems,
    unsigned* p_nverts,
    unsigned** p_verts_of_elems,
    double** p_coords,
    unsigned src_dim,
    unsigned nsrcs,
    unsigned const* verts_of_srcs,
    unsigned const* candidates)
{
  unsigned nelems = *p_nelems;
  unsigned nverts = *p_nverts;
  unsigned const* verts_of_elems = *p_verts_of_elems;
  double const* coords = *p_coords;
  unsigned* srcs_of_verts_offsets;
  unsigned* srcs_of_verts;
  up_from_down(src_dim, 0, nsrcs, nverts, verts_of_srcs,
      &srcs_of_verts_offsets, &srcs_of_verts, 0);
  unsigned* srcs_of_elems = reflect_down(elem_dim, src_dim, nelems, nsrcs,
      verts_of_elems, srcs_of_verts_offsets, srcs_of_verts);
  free(srcs_of_verts_offsets);
  free(srcs_of_verts);
  unsigned* elems_of_srcs_offsets;
  unsigned* elems_of_srcs;
  unsigned* elems_of_srcs_directions;
  up_from_down(elem_dim, src_dim, nelems, nsrcs, srcs_of_elems,
      &elems_of_srcs_offsets, &elems_of_srcs, &elems_of_srcs_directions);
  double* src_quals = refine_qualities(elem_dim, src_dim, nsrcs, verts_of_srcs,
      verts_of_elems, elems_of_srcs_offsets, elems_of_srcs,
      elems_of_srcs_directions, candidates, coords);
  free(elems_of_srcs_directions);
  unsigned* srcs_of_srcs_offsets;
  unsigned* srcs_of_srcs;
  get_star(src_dim, elem_dim, nsrcs, elems_of_srcs_offsets, elems_of_srcs,
      srcs_of_elems, &srcs_of_srcs_offsets, &srcs_of_srcs);
  unsigned* indset = find_indset(nsrcs, srcs_of_srcs_offsets, srcs_of_srcs,
      candidates, src_quals);
  free(elems_of_srcs_offsets);
  free(elems_of_srcs);
  free(srcs_of_srcs_offsets);
  free(srcs_of_srcs);
  free(src_quals);
  unsigned* gen_offset_of_srcs = ints_exscan(indset, nsrcs);
  free(indset);
  unsigned nsplit_srcs = gen_offset_of_srcs[nsrcs];
  unsigned* gen_vert_of_srcs = malloc(sizeof(unsigned) * nsrcs);
  for (unsigned i = 0; i < nsrcs; ++i)
    if (gen_offset_of_srcs[i] != gen_offset_of_srcs[i + src_dim])
      gen_vert_of_srcs[i] = nverts + gen_offset_of_srcs[i];
  unsigned* gen_offset_of_elems;
  unsigned* gen_direction_of_elems;
  unsigned* gen_vert_of_elems;
  project_splits_to_elements(elem_dim, src_dim, nelems,
      srcs_of_elems, gen_offset_of_srcs, gen_vert_of_srcs,
      &gen_offset_of_elems, &gen_direction_of_elems, &gen_vert_of_elems);
  free(srcs_of_elems);
  free(gen_vert_of_srcs);
  unsigned ngen_elems;
  unsigned* verts_of_gen_elems;
  refine_topology(elem_dim, src_dim, elem_dim, nelems, verts_of_elems,
      gen_offset_of_elems, gen_vert_of_elems, gen_direction_of_elems,
      &ngen_elems, &verts_of_gen_elems);
  free(gen_vert_of_elems);
  free(gen_direction_of_elems);
  double* gen_coords = refine_nodal(src_dim, nsrcs, verts_of_srcs,
      gen_offset_of_srcs, 3, coords);
  free(gen_offset_of_srcs);
  unsigned concat_sizes[2] = {nverts, nsplit_srcs};
  unsigned nverts_out = nverts + nsplit_srcs;
  double const* concat_coords[2] = {coords, gen_coords};
  double* coords_out = concat_doubles(2, 3, concat_sizes, concat_coords);
  free(gen_coords);
  unsigned* offset_of_same_elems = ints_negate_offsets(
      gen_offset_of_elems, nelems);
  free(gen_offset_of_elems);
  unsigned nelems_out;
  unsigned* verts_of_elems_out;
  concat_verts_of_elems(elem_dim, nelems, ngen_elems, verts_of_elems,
      offset_of_same_elems, verts_of_gen_elems,
      &nelems_out, &verts_of_elems_out);
  free(offset_of_same_elems);
  free(verts_of_gen_elems);
  *p_nelems = nelems_out;
  *p_nverts = nverts_out;
  free(*p_verts_of_elems);
  *p_verts_of_elems = verts_of_elems_out;
  free(*p_coords);
  *p_coords = coords_out;
}