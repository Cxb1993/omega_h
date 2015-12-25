#include "refine_topology.h"

#include <assert.h>

#include "loop.h"
#include "mesh.h"
#include "tables.h"

/* two more dimensions are introduced in the implementation:
 *   the "base" dimension is the product dimension minus one,
 *       product entities are generated by connecting a base entity
 *       to a generated vertex
 *   the "opp" dimension, the opposite of the base dimension.
 *       the bases of domains which generate products are those
 *       which are opposite to entities adjacent to the source entity.
 *
 * for example, when splitting an edge of a tet, the two vertices
 * of the edge cause the two new tets to be generated from
 * two base triangles, each base triangle being opposite from
 * an edge vertex.
 */

void refine_topology(
    unsigned dom_dim,
    unsigned src_dim,
    unsigned prod_dim,
    unsigned ndoms,
    unsigned const* verts_of_doms,
    unsigned const* offset_of_doms,
    unsigned const* direction_of_doms,
    unsigned const* vert_of_doms,
    unsigned* p_nprods,
    unsigned** p_verts_of_prods)
{
  assert(dom_dim <= 3);
  assert(dom_dim >= src_dim);
  assert(dom_dim >= prod_dim);
  assert(prod_dim > 0);
  *p_nprods = 0;
  *p_verts_of_prods = 0;
  unsigned base_dim = prod_dim - 1;
  unsigned opp_dim = get_opposite_dim(dom_dim, base_dim);
  if (src_dim < opp_dim)
    return;
  unsigned opps_per_src = the_down_degrees[src_dim][opp_dim];
  assert(opps_per_src);
  unsigned nsplit_doms = offset_of_doms[ndoms];
  if (!nsplit_doms)
    return;
  unsigned nprods = nsplit_doms * opps_per_src;
  unsigned verts_per_prod = the_down_degrees[prod_dim][0];
  unsigned verts_per_base = verts_per_prod - 1;
  unsigned verts_per_dom = the_down_degrees[dom_dim][0];
  unsigned* verts_of_prods = LOOP_MALLOC(unsigned,
      nsplit_doms * opps_per_src * verts_per_prod);
  unsigned const* const* dom_opps_of_srcs =
    the_canonical_orders[dom_dim][src_dim][opp_dim];
  unsigned const* const* dom_verts_of_bases =
    the_canonical_orders[dom_dim][base_dim][0];
  unsigned const* dom_base_of_opps = the_opposite_orders[dom_dim][opp_dim];
  for (unsigned i = 0; i < ndoms; ++i) {
    if (offset_of_doms[i] == offset_of_doms[i + 1])
      continue;
    unsigned direction = direction_of_doms[i];
    unsigned const* dom_opps_of_src = dom_opps_of_srcs[direction];
    unsigned const* verts_of_dom = verts_of_doms + i * verts_per_dom;
    unsigned vert = vert_of_doms[i];
    unsigned* verts_of_prod = verts_of_prods + 
      offset_of_doms[i] * opps_per_src * verts_per_prod;
    for (unsigned j = 0; j < opps_per_src; ++j) {
      unsigned opp = dom_opps_of_src[j];
      unsigned base = dom_base_of_opps[opp];
      unsigned const* dom_verts_of_base = dom_verts_of_bases[base];
      for (unsigned k = 0; k < verts_per_base; ++k)
        verts_of_prod[k] = verts_of_dom[dom_verts_of_base[k]];
      verts_of_prod[verts_per_base] = vert;
      verts_of_prod += verts_per_prod;
    }
  }
  *p_nprods = nprods;
  *p_verts_of_prods = verts_of_prods;
}

void mesh_refine_topology(struct mesh* m,
    unsigned dom_dim,
    unsigned src_dim,
    unsigned prod_dim,
    unsigned const* offset_of_doms,
    unsigned const* vert_of_doms,
    unsigned const* direction_of_doms,
    unsigned* p_nprods,
    unsigned** p_verts_of_prods)
{
  unsigned ndoms = mesh_count(m, dom_dim);
  unsigned const* verts_of_doms = mesh_ask_down(m, dom_dim, 0);
  refine_topology(dom_dim, src_dim, prod_dim, ndoms, verts_of_doms,
      offset_of_doms, vert_of_doms, direction_of_doms,
      p_nprods, p_verts_of_prods);
}
