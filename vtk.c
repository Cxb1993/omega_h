#include "vtk.h"
#include <stdio.h>   // for fprintf, FILE, fclose, fopen, printf
#include "field.h"   // for const_field
#include "label.h"   // for const_label
#include "mesh.h"    // for mesh_count, mesh_dim, mesh_find_nodal_label
#include "tables.h"  // for the_down_degrees
#include "loop.h"    // for loop_host_malloc
#include <string.h>  // for strlen
#include <assert.h>  // for assert
#include <stdlib.h>  // for atoi

static unsigned const vtk_types[4] = {
  1,
  3,
  5,
  10
};

static void write_nodal_field(FILE* file, struct mesh* m, struct const_field* field)
{
  fprintf(file, "<DataArray type=\"Float64\" Name=\"%s\""
             " NumberOfComponents=\"%u\" format=\"ascii\">\n",
             field->name, field->ncomps);
  unsigned nverts = mesh_count(m, 0);
  double const* p = field->data;
  for (unsigned i = 0; i < nverts; ++i) {
    for (unsigned j = 0; j < field->ncomps; ++j)
      fprintf(file, " %e", *p++);
    fprintf(file, "\n");
  }
  fprintf(file, "</DataArray>\n");
}

static void write_nodal_label(FILE* file, struct mesh* m, struct const_label* label)
{
  fprintf(file, "<DataArray type=\"UInt32\" Name=\"%s\""
             " NumberOfComponents=\"1\" format=\"ascii\">\n",
             label->name);
  unsigned nverts = mesh_count(m, 0);
  unsigned const* p = label->data;
  for (unsigned i = 0; i < nverts; ++i)
    fprintf(file, " %u\n", p[i]);
  fprintf(file, "</DataArray>\n");
}

static void write_elem_field(FILE* file, struct mesh* m, struct const_field* field)
{
  fprintf(file, "<DataArray type=\"Float64\" Name=\"%s\""
             " NumberOfComponents=\"%u\" format=\"ascii\">\n",
             field->name, field->ncomps);
  unsigned nverts = mesh_count(m, mesh_dim(m));
  double const* p = field->data;
  for (unsigned i = 0; i < nverts; ++i) {
    for (unsigned j = 0; j < field->ncomps; ++j)
      fprintf(file, " %e", *p++);
    fprintf(file, "\n");
  }
  fprintf(file, "</DataArray>\n");
}

static char const* types_header =
"<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">";

/* this function can be a time hog,
 * no fault of our own really, just printf and friends
 * are fairly slow.
 * if you're so inclined, add binary functionality
 * (the VTK format supports it)
 */

void write_vtk(struct mesh* m, char const* filename)
{
  printf("writing %s\n", filename);
  unsigned elem_dim = mesh_dim(m);
  unsigned nverts = mesh_count(m, 0);
  unsigned nelems = mesh_count(m, elem_dim);
  unsigned const* verts_of_elems = mesh_ask_down(m, elem_dim, 0);
  FILE* file = fopen(filename, "w");
  fprintf(file, "<VTKFile type=\"UnstructuredGrid\">\n");
  fprintf(file, "<UnstructuredGrid>\n");
  fprintf(file, "<Piece NumberOfPoints=\"%u\" NumberOfCells=\"%u\">\n", nverts, nelems);
  fprintf(file, "<Points>\n");
  struct const_field* coord_field = mesh_find_nodal_field(m, "coordinates");
  write_nodal_field(file, m, coord_field);
  fprintf(file, "</Points>\n");
  fprintf(file, "<Cells>\n");
  fprintf(file, "<DataArray type=\"UInt32\" Name=\"connectivity\" format=\"ascii\">\n");
  unsigned down_degree = the_down_degrees[elem_dim][0];
  for (unsigned i = 0; i < nelems; ++i) {
    unsigned const* p = verts_of_elems + i * down_degree;
    for (unsigned j = 0; j < down_degree; ++j)
      fprintf(file, " %u", p[j]);
    fprintf(file, "\n");
  }
  fprintf(file, "</DataArray>\n");
  fprintf(file, "<DataArray type=\"UInt32\" Name=\"offsets\" format=\"ascii\">\n");
  for (unsigned i = 0; i < nelems; ++i)
    fprintf(file, "%u\n", (i + 1) * down_degree);
  fprintf(file, "</DataArray>\n");
  fprintf(file, "%s\n", types_header);
  unsigned type = vtk_types[elem_dim];
  for (unsigned i = 0; i < nelems; ++i)
    fprintf(file, "%u\n", type);
  fprintf(file, "</DataArray>\n");
  fprintf(file, "</Cells>\n");
  fprintf(file, "<PointData>\n");
  for (unsigned i = 0; i < mesh_count_nodal_labels(m); ++i) {
    struct const_label* label = mesh_get_nodal_label(m, i);
    write_nodal_label(file, m, label);
  }
  for (unsigned i = 0; i < mesh_count_nodal_fields(m); ++i) {
    struct const_field* field = mesh_get_nodal_field(m, i);
    if (field != coord_field)
      write_nodal_field(file, m, field);
  }
  fprintf(file, "</PointData>\n");
  fprintf(file, "<CellData>\n");
  for (unsigned i = 0; i < mesh_count_elem_fields(m); ++i) {
    struct const_field* field = mesh_get_elem_field(m, i);
    if (field != coord_field)
      write_elem_field(file, m, field);
  }
  fprintf(file, "</CellData>\n");
  fprintf(file, "</Piece>\n");
  fprintf(file, "</UnstructuredGrid>\n");
  fprintf(file, "</VTKFile>\n");
  fclose(file);
}

static char const* the_step_prefix = 0;
static unsigned the_step = 0;

void start_vtk_steps(char const* prefix)
{
  the_step_prefix = prefix;
  the_step = 0;
}

void write_vtk_step(struct mesh* m)
{
  char fname[64];
  sprintf(fname, "%s_%04u.vtu", the_step_prefix, the_step);
  write_vtk(m, fname);
  ++the_step;
}

static unsigned seek_prefix(FILE* f,
    char line[], unsigned line_size, char const prefix[])
{
  unsigned pl = (unsigned) strlen(prefix);
  while (fgets(line, (int) line_size, f))
    if (!strncmp(line, prefix, pl))
      return 1;
  return 0;
}

static char* read_attrib(char elem[], char const name[])
{
  char* pname = strstr(elem, name);
  assert(pname);
  assert(pname[strlen(name) + 1] == '\"');
  char* val = strtok(pname + strlen(name) + 2, "\"");
  assert(val && strlen(val));
  return val;
}

static char* read_array_name(char header[])
{
  return read_attrib(header, "Name");
}

enum array_type { FIELD, LABEL };

static enum array_type read_array_type(char header[])
{
  return strstr(read_attrib(header, "type"), "Float") ?
        FIELD : LABEL;
}

static unsigned read_array_ncomps(char header[])
{
  return (unsigned) atoi(read_attrib(header, "NumberOfComponents"));
}

static unsigned* read_ints(FILE* f, unsigned n)
{
  unsigned* out = loop_host_malloc(sizeof(unsigned) * n);
  for (unsigned i = 0; i < n; ++i)
    fscanf(f, "%u", &out[i]);
  return out;
}

static double* read_doubles(FILE* f, unsigned n)
{
  double* out = loop_host_malloc(sizeof(double) * n);
  for (unsigned i = 0; i < n; ++i)
    fscanf(f, "%lf", &out[i]);
  return out;
}

static void read_size(FILE* f, unsigned* nverts, unsigned* nelems)
{
  char line[256];
  assert(seek_prefix(f, line, sizeof(line), "<Piece"));
  *nverts = (unsigned) atoi(read_attrib(line, "NumberOfPoints"));
  *nelems = (unsigned) atoi(read_attrib(line, "NumberOfCells"));
}

static unsigned read_dimension(FILE* f, unsigned nelems)
{
  assert(nelems);
  char line[256];
  assert(seek_prefix(f, line, sizeof(line), types_header));
  unsigned* types = read_ints(f, nelems);
  unsigned dim;
  for (dim = 0; dim < 4; ++dim)
    if (types[0] == vtk_types[dim])
      break;
  assert(dim < 4);
  for (unsigned i = 1; i < nelems; ++i)
    assert(types[i] == vtk_types[dim]);
  loop_host_free(types);
  return dim;
}

static unsigned read_mesh_array(FILE* f, struct mesh* m,
    unsigned dim)
{
  char line[256];
  if (!seek_prefix(f, line, sizeof(line), "<DataArray"))
    return 0;
  enum array_type at = read_array_type(line);
  char* name = read_array_name(line);
  if (at == FIELD) {
    unsigned ncomps = read_array_ncomps(line);
    double* data = read_doubles(f, mesh_count(m, dim) * ncomps);
    if (dim == 0)
      mesh_add_nodal_field(m, name, ncomps, data);
    else
      mesh_add_elem_field(m, name, ncomps, data);
  } else {
    unsigned* data = read_ints(f, mesh_count(m, dim));
    mesh_add_nodal_label(m, name, data);
  }
  return 1;
}

static void read_verts(FILE* f, struct mesh* m)
{
  char line[256];
  seek_prefix(f, line, sizeof(line), "<Points");
  read_mesh_array(f, m, 0);
}

static void read_elems(FILE* f, struct mesh* m, unsigned nelems)
{
  char line[256];
  seek_prefix(f, line, sizeof(line), "<Cells");
  seek_prefix(f, line, sizeof(line), "<DataArray");
  assert(!strcmp("connectivity", read_array_name(line)));
  unsigned dim = mesh_dim(m);
  unsigned verts_per_elem = the_down_degrees[dim][0];
  unsigned* data = read_ints(f, nelems * verts_per_elem);
  mesh_set_ents(m, dim, nelems, data);
}

struct mesh* read_vtk(char const* filename)
{
  FILE* f = fopen(filename, "r");
  assert(f);
  unsigned nverts, nelems;
  read_size(f, &nverts, &nelems);
  if (!nelems) {
    fclose(f);
    return new_mesh(0);
  }
  unsigned dim = read_dimension(f, nelems);
  struct mesh* m = new_mesh(dim);
  mesh_set_ents(m, 0, nverts, 0);
  rewind(f);
  read_verts(f, m);
  read_elems(f, m, nelems);
  fclose(f);
  return m;
}
