#include "refine_by_size.h"
#include "tables.h"
#include "algebra.h"
#include "vtk.h"
#include "element_qualities.h"
#include "doubles.h"
#include <stdlib.h>
#include <stdio.h>

static double simple(double const x[])
{
  double coarse = 0.5;
  double fine = 0.025;
  double radius = vector_norm(x, 3);
  double d = fabs(radius - 0.5);
  return coarse * d + fine * (1 - d);
}

int main()
{
  struct mesh* m = new_box_mesh(3);
  char fname[64];
  for (unsigned it = 0; 1; ++it) {
    if (!refine_by_size(&m, simple))
      break;
    printf("%u elements, %u vertices\n", mesh_count(m, mesh_dim(m)), mesh_count(m, 0));
    double* quals = element_qualities(mesh_dim(m), mesh_count(m, mesh_dim(m)),
        mesh_ask_down(m, mesh_dim(m), 0),
        mesh_find_nodal_field(m, "coordinates")->data);
    double minqual = doubles_min(quals, mesh_count(m, mesh_dim(m)));
    free(quals);
    printf("min quality %f\n", minqual);
    sprintf(fname, "out_%u.vtu", it);
    write_vtk(m, fname);
  }
  free_mesh(m);
}
