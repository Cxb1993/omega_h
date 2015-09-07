#include "vtk.h"
#include "mesh.h"

int main()
{
  struct mesh* m = new_box_mesh(3);
  write_vtk(m, "test.vtu");
  free_mesh(m);
}
