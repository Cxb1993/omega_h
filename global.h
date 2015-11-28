#ifndef GLOBAL_H
#define GLOBAL_H

struct mesh;

void mesh_number_simply(struct mesh* m);
unsigned long* globalize_offsets(unsigned* local, unsigned n);
void global_to_linpart(unsigned long const* global, unsigned n,
    unsigned long total, unsigned nparts,
    unsigned** p_part, unsigned** p_local);
unsigned linpart_size(unsigned long total, unsigned nparts, unsigned part);

unsigned* sort_uints_by_category(
    unsigned const* a,
    unsigned width,
    unsigned n,
    unsigned const* cats,
    unsigned const* indices,
    unsigned const* cat_offsets);

double* sort_doubles_by_category(
    double const* a,
    unsigned width,
    unsigned n,
    unsigned const* cats,
    unsigned const* indices,
    unsigned const* cat_offsets);

unsigned* unsort_uints_by_category(
    unsigned const* a,
    unsigned width,
    unsigned n,
    unsigned const* cats,
    unsigned const* indices,
    unsigned const* cat_offsets);

double* unsort_doubles_by_category(
    double const* a,
    unsigned width,
    unsigned n,
    unsigned const* cats,
    unsigned const* indices,
    unsigned const* cat_offsets);

unsigned long* unsort_ulongs_by_category(
    unsigned long const* a,
    unsigned width,
    unsigned n,
    unsigned const* cats,
    unsigned const* indices,
    unsigned const* cat_offsets);

#endif
