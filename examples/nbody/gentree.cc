/* 
 * gentree.cc
 */

#include "def.h"

int select_covering_rectangle(vect_t pos, rectangle * rec)
{
  if (cover_p(rec, pos)) {
    vect_t ll = rec->ll; vect_t ur = rec->ur;
    t_real x = VX(pos), y = VY(pos), z = VZ(pos);
    t_real lx = VX(ll), ly = VY(ll), lz = VZ(ll);
    t_real rx = VX(ur), ry = VY(ur), rz = VZ(ur);
    t_real cx = (lx + rx) * 0.5, cy = (ly + ry) * 0.5, cz = (lz + rz) * 0.5;
    int r = 0;
    if (x >= cx) r += 4;
    if (y >= cy) r += 2;
    if (z >= cz) r += 1;
    return r;
  } else {
    printf("ERROR: select_covering_rectangle\nRectangle: ");
    print_rectangle(rec);
    printf("\nPOS: ");
    print_vect(pos);
    printf("\n");
    return 0;
  }
}

rectangle * make_sub_rectangle(rectangle * parent, int idx)
{
  vect_t ll = parent->ll; vect_t ur = parent->ur;
  t_real lx = VX(ll), ly = VY(ll), lz = VZ(ll);
  t_real rx = VX(ur), ry = VY(ur), rz = VZ(ur);
  t_real cx = 0.5 * (lx + rx), cy = 0.5 * (ly + ry), cz = 0.5 * (lz + rz); 
  t_real px = lx, py = ly, pz = lz;
  t_real qx = cx, qy = cy, qz = cz;
  if ((idx >> 2) % 2 == 1) { px = cx; qx = rx; }
  if ((idx >> 1) % 2 == 1) { py = cy; qy = ry; }
  if ((idx >> 0) % 2 == 1) { pz = cz; qz = rz; }
  return new rectangle(make_vect(px, py, pz), 
			    make_vect(qx, qy, qz));
}

space ** make_new_spaces(rectangle * area)
{
	space ** s = new space* [N_CHILDREN];
  // FIXME: Zero memory
  //MM_ZERO_CLEAR((void *)s, sizeof(space*) * N_CHILDREN);
#if 1/*endo debug*/
  if (VX(area->ll) == 0.0 && VX(area->ur) == 0.0)
		fprintf(stderr,"[make_new_spaces] AREA EMPTY??\n");
#endif
  for (int i = 0; i < N_CHILDREN; i++) 
    s[i] = make_empty_space(make_sub_rectangle(area, i));
  return s;
}

void space::add_particle(t_real m, vect_t p)
{
  switch (state) {
    case NO_PARTICLE: {
      state = ONE_PARTICLE;
      mass = m; cg = p;
      return;
    } /* NO_PARTICLE */
    case ONE_PARTICLE: {
      subspaces = make_new_spaces(area);
      int idx0 = select_covering_rectangle(cg, area);
      subspaces[idx0]->add_particle(mass, cg);
      int idx = select_covering_rectangle(p, area);
      subspaces[idx]->add_particle(m, p);
      state = MULTIPLE_PARTICLES;
      return;
    } /* ONE_PARTICLE */
    case MULTIPLE_PARTICLES: {
      int idx = select_covering_rectangle(p, area);
      subspaces[idx]->add_particle(m, p);
      return;
    } /* MULTIPLE_PARTICLES */
    default: {
      printf("ERROR: add_particle\n");
    } /* default */
  }
}

t_real particle::calc_limit()
{
  t_real x = VX(pos), y = VY(pos), z = VZ(pos);
  t_real xx = fabs(x), yy = fabs(y), zz = fabs(z);
  t_real r = xx;
  if (r < yy) r = yy;
  if (r < zz) r = zz;
  return r;
}

t_real calc_limit(particle ** particles, int n_particles)
{
  t_real r = 0.0;
  for (int i = 0; i < n_particles; i++) {
    t_real s = particles[i]->calc_limit();
    if (r < s) r = s;
  }
  return 1.01 * r;
}

space * generate_tree(particle ** particles, int n_particles)
{
	t_real limit = calc_limit(particles, n_particles);
	rectangle * rec = make_entire_rectangle(limit);
	space * tree = make_empty_space(rec);
	for (int i = 0; i < n_particles; i++)
		tree->add_particle(particles[i]->mass, particles[i]->pos);
	return tree;
}

mass_momentum space::set_mass_and_cg()
{
  switch (state) {
    case NO_PARTICLE: {
      mass = 0.0;
      return mass_momentum(0.0, make_vect(0.0, 0.0, 0.0), 0);
    } /* NO_PARTICL */
    case ONE_PARTICLE: {
      return mass_momentum(mass, k_times_v(mass, cg), 1);
    } /* ONE_PARTICLE */
    case MULTIPLE_PARTICLES: {
      vect_t total_momentum = make_vect(0.0, 0.0, 0.0);
      t_real total_mass = 0.0;
      int total_n_nodes = 1;
      for (int i = 0; i < N_CHILDREN; i++) {
	      space * subspace = subspaces[i];
	      mass_momentum mm = subspace->set_mass_and_cg();
	      if (mm.n_nodes == 0) {
	        subspaces[i] = 0;
        } else {
          total_mass += mm.mass;
          total_momentum = v_plus_v(total_momentum, mm.momentum);
          total_n_nodes += mm.n_nodes;
        }
      }
      mass = total_mass; 
      cg = v_div_k(total_mass, total_momentum);
      return mass_momentum(total_mass, total_momentum, total_n_nodes);
    } /* MULTIPLE_PARTICLES */
    default: {
      printf("invalid state tag = %d\n", state);
      return mass_momentum(0.0, make_vect(0.0, 0.0, 0.0), 0);
    } /* default */
  } 
}
