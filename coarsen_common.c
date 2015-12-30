#include "coarsen_common.h"

#include "arrays.h"
#include "check_collapse_class.h"
#include "coarsen_qualities.h"
#include "coarsen_topology.h"
#include "collapse_codes.h"
#include "collapses_to_ents.h"
#include "collapses_to_verts.h"
#include "concat_topology.h"
#include "graph.h"
#include "indset.h"
#include "ints.h"
#include "loop.h"
#include "mesh.h"
#include "quality.h"
#include "subset.h"
#include "tables.h"
#include "tag.h"

static void coarsen_ents(
    struct mesh* m,
    struct mesh* m_out,
    unsigned ent_dim,
    unsigned const* gen_offset_of_verts,
    unsigned const* gen_vert_of_verts,
    unsigned const* offset_of_same_verts,
    unsigned** p_dead_sides)
{
  unsigned nents = mesh_count(m, ent_dim);
  unsigned const* verts_of_ents = mesh_ask_down(m, ent_dim, 0);
  unsigned* gen_offset_of_ents;
  unsigned* gen_vert_of_ents;
  unsigned* gen_direction_of_ents;
  unsigned* offset_of_same_ents;
  collapses_to_ents(m, ent_dim,
      gen_offset_of_verts, gen_vert_of_verts,
      &gen_offset_of_ents, &gen_vert_of_ents,
      &gen_direction_of_ents, &offset_of_same_ents, p_dead_sides);
  unsigned ngen_ents;
  unsigned* verts_of_gen_ents;
  coarsen_topology(ent_dim, nents, verts_of_ents, gen_offset_of_ents,
      gen_vert_of_ents, gen_direction_of_ents, &ngen_ents,
      &verts_of_gen_ents);
  loop_free(gen_offset_of_ents);
  loop_free(gen_vert_of_ents);
  loop_free(gen_direction_of_ents);
  unsigned nents_out;
  unsigned* verts_of_ents_out;
  concat_verts_of_elems(ent_dim, nents, ngen_ents, verts_of_ents,
      offset_of_same_ents, verts_of_gen_ents,
      &nents_out, &verts_of_ents_out);
  loop_free(offset_of_same_ents);
  loop_free(verts_of_gen_ents);
  /* remap new connectivity to account for vertex removal */
  unsigned verts_per_ent = the_down_degrees[ent_dim][0];
  for (unsigned i = 0; i < nents_out * verts_per_ent; ++i)
    verts_of_ents_out[i] = offset_of_same_verts[verts_of_ents_out[i]];
  mesh_set_ents(m_out, ent_dim, nents_out, verts_of_ents_out);
}

static void coarsen_reduced_ents(
    struct mesh* m,
    struct mesh* m_out,
    unsigned const* gen_offset_of_verts,
    unsigned const* gen_vert_of_verts,
    unsigned const* offset_of_same_verts)
{
  coarsen_ents(m, m_out, mesh_dim(m), gen_offset_of_verts,
      gen_vert_of_verts, offset_of_same_verts, 0);
}

unsigned coarsen_common(
    struct mesh** p_m,
    unsigned* col_codes,
    double quality_floor,
    unsigned require_better)
{
  struct mesh* m = *p_m;
  unsigned nedges = mesh_count(m, 1);
  if (uints_max(col_codes, nedges) == DONT_COLLAPSE)
    return 0;
  unsigned const* verts_of_edges = mesh_ask_down(m, 1, 0);
  double const* coords = mesh_find_tag(m, 0, "coordinates")->d.f64;
  unsigned nverts = mesh_count(m, 0);
  unsigned elem_dim = mesh_dim(m);
  unsigned nelems = mesh_count(m, elem_dim);
  unsigned const* verts_of_elems = mesh_ask_down(m, elem_dim, 0);
  check_collapse_class(m, col_codes);
  unsigned const* elems_of_verts_offsets = mesh_ask_up(m, 0, elem_dim)->offsets;
  unsigned const* elems_of_verts = mesh_ask_up(m, 0, elem_dim)->adj;
  unsigned const* elems_of_verts_directions = mesh_ask_up(m, 0, elem_dim)->directions;
  double* elem_quals = 0;
  if (require_better)
    elem_quals = element_qualities(elem_dim, nelems, verts_of_elems, coords);
  double* quals_of_edges = coarsen_qualities(elem_dim, nedges, col_codes,
      verts_of_elems, verts_of_edges, elems_of_verts_offsets,
      elems_of_verts, elems_of_verts_directions, coords, quality_floor,
      elem_quals, require_better);
  loop_free(elem_quals);
  if (uints_max(col_codes, nedges) == DONT_COLLAPSE) {
    /* early return #2: all small edges failed their classif/quality checks */
    loop_free(quals_of_edges);
    return 0;
  }
  /* from this point forward, some edges will definitely collapse */
  unsigned* candidates;
  unsigned* gen_vert_of_verts;
  double* qual_of_verts;
  unsigned const* edges_of_verts_offsets = mesh_ask_up(m, 0, 1)->offsets;
  unsigned const* edges_of_verts = mesh_ask_up(m, 0, 1)->adj;
  unsigned const* edges_of_verts_directions = mesh_ask_up(m, 0, 1)->directions;
  collapses_to_verts(nverts, verts_of_edges, edges_of_verts_offsets,
      edges_of_verts, edges_of_verts_directions, col_codes, quals_of_edges,
      &candidates, &gen_vert_of_verts, &qual_of_verts);
  loop_free(quals_of_edges);
  unsigned* gen_offset_of_verts = mesh_indset_offsets(m, 0, candidates,
      qual_of_verts);
  loop_free(candidates);
  loop_free(qual_of_verts);
  unsigned* offset_of_same_verts = uints_negate_offsets(
      gen_offset_of_verts, nverts);
  unsigned nverts_out = offset_of_same_verts[nverts];
  struct mesh* m_out = new_mesh(elem_dim);
  mesh_set_ents(m_out, 0, nverts_out, 0);
  tags_subset(m, m_out, 0, offset_of_same_verts);
  coarsen_reduced_ents(m, m_out, gen_offset_of_verts, gen_vert_of_verts,
      offset_of_same_verts);
  loop_free(gen_vert_of_verts);
  loop_free(gen_offset_of_verts);
  loop_free(offset_of_same_verts);
  free_mesh(m);
  *p_m = m_out;
  return 1;
}
