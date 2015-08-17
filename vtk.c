#include "vtk.h"
#include "tables.h"
#include <stdio.h>

static unsigned const vtk_types[4] = {
  1,
  3,
  5,
  10
};

/* this function can be a time hog,
 * no fault of our own really, just printf and friends
 * are fairly slow.
 * if you're so inclined, add binary functionality
 * (the VTK format supports it)
 */

void write_vtk(struct mesh* m, char const* filename)
{
  unsigned elem_dim = mesh_dim(m);
  unsigned nverts = mesh_count(m, 0);
  unsigned nelems = mesh_count(m, elem_dim);
  unsigned const* verts_of_elems = mesh_ask_down(m, elem_dim, 0);
  double const* coords = mesh_find_nodal_field(m, "coordinates")->data;
  FILE* f = fopen(filename, "w");
  fprintf(f, "<VTKFile type=\"UnstructuredGrid\">\n");
  fprintf(f, "<UnstructuredGrid>\n");
  fprintf(f, "<Piece NumberOfPoints=\"%u\" NumberOfCells=\"%u\">\n", nverts, nelems);
  fprintf(f, "<Points>\n");
  fprintf(f, "<DataArray type=\"Float64\" Name=\"coordinates\""
             " NumberOfComponents=\"3\" format=\"ascii\">\n");
  for (unsigned i = 0; i < nverts; ++i) {
    double const* p = coords + i * 3;
    fprintf(f, "%f %f %f\n", p[0], p[1], p[2]);
  }
  fprintf(f, "</DataArray>\n");
  fprintf(f, "</Points>\n");
  fprintf(f, "<Cells>\n");
  fprintf(f, "<DataArray type=\"UInt32\" Name=\"connectivity\" format=\"ascii\">\n");
  unsigned down_degree = the_down_degrees[elem_dim][0];
  for (unsigned i = 0; i < nelems; ++i) {
    unsigned const* p = verts_of_elems + i * down_degree;
    for (unsigned j = 0; j < down_degree; ++j)
      fprintf(f, " %u", p[j]);
    fprintf(f, "\n");
  }
  fprintf(f, "</DataArray>\n");
  fprintf(f, "<DataArray type=\"UInt32\" Name=\"offsets\" format=\"ascii\">\n");
  for (unsigned i = 0; i < nelems; ++i)
    fprintf(f, "%u\n", (i + 1) * down_degree);
  fprintf(f, "</DataArray>\n");
  fprintf(f, "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n");
  unsigned type = vtk_types[elem_dim];
  for (unsigned i = 0; i < nelems; ++i)
    fprintf(f, "%u\n", type);
  fprintf(f, "</DataArray>\n");
  fprintf(f, "</Cells>\n");
  fprintf(f, "<PointData>\n");
  if (mesh_find_nodal_label(m, "class_dim")) {
    unsigned const* class_dim = mesh_find_nodal_label(m, "class_dim")->data;
    fprintf(f, "<DataArray type=\"UInt32\" Name=\"class_dim\""
               " NumberOfComponents=\"1\" format=\"ascii\">\n");
    for (unsigned i = 0; i < nverts; ++i)
      fprintf(f, "%u\n", class_dim[i]);
    fprintf(f, "</DataArray>\n");
  }
  fprintf(f, "</PointData>\n");
  fprintf(f, "<CellData>\n");
  fprintf(f, "</CellData>\n");
  fprintf(f, "</Piece>\n");
  fprintf(f, "</UnstructuredGrid>\n");
  fprintf(f, "</VTKFile>\n");
  fclose(f);
}
