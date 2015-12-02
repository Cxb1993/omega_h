#include <assert.h>

#include "comm.h"
#include "global.h"
#include "mesh.h"
#include "migrate_mesh.h"
#include "vtk.h"

int main()
{
  comm_init();
  struct mesh* m = 0;
  assert(comm_size() == 2);
  if (comm_rank() == 0)
    m = new_box_mesh(1);
  else
    m = new_mesh(1);
  mesh_number_simply(m);
  write_parallel_vtu(m, "before.pvtu");
//if (comm_rank() == 0) {
//  unsigned n = 0;
//  unsigned recvd_elem_ranks[1];
//  unsigned recvd_elem_ids[1];
//  migrate_mesh(&m, n, recvd_elem_ranks, recvd_elem_ids);
//} else {
//  unsigned n = 1;
//  unsigned recvd_elem_ranks[1] = {0};
//  unsigned recvd_elem_ids[1] = {0};
//  migrate_mesh(&m, n, recvd_elem_ranks, recvd_elem_ids);
//}
//write_parallel_vtu(m, "after.pvtu");
  free_mesh(m);
  comm_fini();
}
