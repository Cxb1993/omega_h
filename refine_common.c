#include "refine_common.h"
#include "loop.h"              // for free, malloc
#include "concat.h"              // for concat_doubles, concat_ints, concat_...
#include "field.h"               // for const_field
#include "graph.h"               // for const_graph
#include "indset.h"              // for find_indset
#include "ints.h"                // for ints_max, ints_exscan, ints_negate_o...
#include "label.h"               // for const_label
#include "mesh.h"                // for mesh_ask_down, mesh_ask_up, mesh_count
#include "quality.h"             // for mesh_qualities
#include "refine_class.h"        // for refine_class
#include "refine_nodal.h"        // for refine_nodal
#include "refine_qualities.h"    // for refine_qualities
#include "refine_topology.h"     // for refine_topology
#include "splits_to_elements.h"  // for project_splits_to_elements

unsigned refine_common(
    struct mesh** p_m,
    unsigned src_dim,
    unsigned* candidates,
    double qual_floor,
    unsigned require_better)
{
  struct mesh* m = *p_m;
  unsigned elem_dim = mesh_dim(m);
  unsigned nsrcs = mesh_count(m, src_dim);
  if (!ints_max(candidates, nsrcs))
    return 0;
  unsigned const* verts_of_srcs = mesh_ask_down(m, src_dim, 0);
  unsigned const* verts_of_elems = mesh_ask_down(m, elem_dim, 0);
  unsigned const* elems_of_srcs_offsets =
    mesh_ask_up(m, src_dim, elem_dim)->offsets;
  unsigned const* elems_of_srcs =
    mesh_ask_up(m, src_dim, elem_dim)->adj;
  unsigned const* elems_of_srcs_directions =
    mesh_ask_up(m, src_dim, elem_dim)->directions;
  double const* coords = mesh_find_field(m, 0, "coordinates")->data;
  double* elem_quals = 0;
  if (require_better)
    elem_quals = mesh_qualities(m);
  double* src_quals = refine_qualities(elem_dim, src_dim, nsrcs, verts_of_srcs,
      verts_of_elems, elems_of_srcs_offsets, elems_of_srcs,
      elems_of_srcs_directions, candidates, coords, qual_floor,
      elem_quals, require_better);
  loop_free(elem_quals);
  if (!ints_max(candidates, nsrcs)) {
    loop_free(src_quals);
    return 0;
  }
  unsigned const* srcs_of_srcs_offsets =
    mesh_ask_star(m, src_dim, elem_dim)->offsets;
  unsigned const* srcs_of_srcs =
    mesh_ask_star(m, src_dim, elem_dim)->adj;
  unsigned* indset = find_indset(nsrcs, srcs_of_srcs_offsets, srcs_of_srcs,
      candidates, src_quals);
  loop_free(src_quals);
  unsigned* gen_offset_of_srcs = ints_exscan(indset, nsrcs);
  loop_free(indset);
  unsigned nsplit_srcs = gen_offset_of_srcs[nsrcs];
  unsigned nverts = mesh_count(m, 0);
  unsigned* gen_vert_of_srcs = loop_malloc(sizeof(unsigned) * nsrcs);
  for (unsigned i = 0; i < nsrcs; ++i)
    if (gen_offset_of_srcs[i] != gen_offset_of_srcs[i + src_dim])
      gen_vert_of_srcs[i] = nverts + gen_offset_of_srcs[i];
  unsigned nelems = mesh_count(m, elem_dim);
  unsigned const* srcs_of_elems = mesh_ask_down(m, elem_dim, src_dim);
  unsigned* gen_offset_of_elems;
  unsigned* gen_direction_of_elems;
  unsigned* gen_vert_of_elems;
  project_splits_to_elements(elem_dim, src_dim, nelems,
      srcs_of_elems, gen_offset_of_srcs, gen_vert_of_srcs,
      &gen_offset_of_elems, &gen_direction_of_elems, &gen_vert_of_elems);
  loop_free(gen_vert_of_srcs);
  unsigned ngen_elems;
  unsigned* verts_of_gen_elems;
  refine_topology(elem_dim, src_dim, elem_dim, nelems, verts_of_elems,
      gen_offset_of_elems, gen_vert_of_elems, gen_direction_of_elems,
      &ngen_elems, &verts_of_gen_elems);
  loop_free(gen_vert_of_elems);
  loop_free(gen_direction_of_elems);
  struct mesh* m_out = new_mesh(elem_dim);
  unsigned nverts_out = nverts + nsplit_srcs;
  mesh_set_ents(m_out, 0, nverts_out, 0);
  for (unsigned i = 0; i < mesh_count_fields(m, 0); ++i) {
    struct const_field* f = mesh_get_field(m, 0, i);
    double* gen_vals = refine_nodal(src_dim, nsrcs, verts_of_srcs,
        gen_offset_of_srcs, f->ncomps, f->data);
    double* vals_out = concat_doubles(f->ncomps, f->data, nverts,
        gen_vals, nsplit_srcs);
    loop_free(gen_vals);
    mesh_add_field(m_out, 0, f->name, f->ncomps, vals_out);
  }
  if (mesh_find_label(m, 0, "class_dim")) {
    unsigned const* class_dim = mesh_find_label(m, 0, "class_dim")->data;
    unsigned* gen_class_dim = refine_class(src_dim, nsrcs, verts_of_srcs,
        gen_offset_of_srcs, class_dim);
    unsigned* class_dim_out = concat_ints(1, class_dim, nverts,
        gen_class_dim, nsplit_srcs);
    loop_free(gen_class_dim);
    mesh_add_label(m_out, 0, "class_dim", class_dim_out);
  }
  loop_free(gen_offset_of_srcs);
  unsigned* offset_of_same_elems = ints_negate_offsets(
      gen_offset_of_elems, nelems);
  loop_free(gen_offset_of_elems);
  unsigned nelems_out;
  unsigned* verts_of_elems_out;
  concat_verts_of_elems(elem_dim, nelems, ngen_elems, verts_of_elems,
      offset_of_same_elems, verts_of_gen_elems,
      &nelems_out, &verts_of_elems_out);
  mesh_set_ents(m_out, elem_dim, nelems_out, verts_of_elems_out);
  loop_free(offset_of_same_elems);
  loop_free(verts_of_gen_elems);
  free_mesh(m);
  *p_m = m_out;
  return 1;
}
