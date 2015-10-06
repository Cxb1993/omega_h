#include "recover_by_volume.h"

#include "loop.h"  // for malloc, free
#include "mesh.h"    // for mesh_ask_up, mesh_dim, mesh_find_field
#include "size.h"    // for mesh_element_sizes
#include "tag.h"

double* recover_by_volume(
    unsigned nverts,
    unsigned const* elems_of_verts_offsets,
    unsigned const* elems_of_verts,
    double const* size_of_elems,
    unsigned ncomps,
    double const* comps_of_elems)
{
  double* comps_of_verts = loop_malloc(sizeof(double) * ncomps * nverts);
  for (unsigned i = 0; i < nverts; ++i) {
    unsigned first_use = elems_of_verts_offsets[i];
    unsigned end_use = elems_of_verts_offsets[i + 1];
    double* comps_of_vert = comps_of_verts + i * ncomps;
    for (unsigned j = 0; j < ncomps; ++j)
      comps_of_vert[j] = 0;
    double size_sum = 0;
    for (unsigned j = first_use; j < end_use; ++j) {
      unsigned elem = elems_of_verts[j];
      double elem_size = size_of_elems[elem];
      size_sum += elem_size;
      double const* comps_of_elem = comps_of_elems + elem * ncomps;
      for (unsigned k = 0; k < ncomps; ++k)
        comps_of_vert[k] += elem_size * comps_of_elem[k];
    }
    for (unsigned j = 0; j < ncomps; ++j)
      comps_of_vert[j] /= size_sum;
  }
  return comps_of_verts;
}

struct const_tag* mesh_recover_by_volume(
    struct mesh* m, char const* name)
{
  mesh_element_sizes(m);
  double const* elem_sizes = mesh_find_tag(
      m, mesh_dim(m), "elem_size")->data;
  struct const_tag* t = mesh_find_tag(m, mesh_dim(m), name);
  double* data = recover_by_volume(mesh_count(m, 0),
      mesh_ask_up(m, 0, mesh_dim(m))->offsets,
      mesh_ask_up(m, 0, mesh_dim(m))->adj,
      elem_sizes,
      t->ncomps, t->data);
  mesh_free_tag(m, mesh_dim(m), "elem_size");
  struct const_tag* out = mesh_add_tag(m, 0, TAG_F64, name, t->ncomps, data);
  return out;
}
