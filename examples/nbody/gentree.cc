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
#if USE_MALLOC
  rectangle * r = (rectangle *) malloc(sizeof(rectangle));
  r->ll = make_vect(px, py, pz);
  r->ur = make_vect(qx, qy, qz);
  return r;
#else
  return new rectangle(make_vect(px, py, pz), make_vect(qx, qy, qz));
#endif
}

space ** make_new_spaces(rectangle * area)
{
#if USE_MALLOC
  space ** s = (space **) malloc(N_CHILDREN * sizeof(space*));
#else
  space ** s = new space* [N_CHILDREN];
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

void space::divide()
{
  state = MULTIPLE_PARTICLES;
  subspaces = make_new_spaces(area);
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

/* Parallel version of tree build */
struct bt_thread_dat {
  space * tree;
  particle ** particles;
  int * subset_arr;
  int subset_size;
};

bt_thread_dat * particles_in_tree(bt_thread_dat * orig)
{
#if USE_MALLOC
  bt_thread_dat * p = (bt_thread_dat *) malloc(sizeof(bt_thread_dat));
#else
  bt_thread_dat * p = new bt_thread_dat;
#endif
  
  p->tree = orig->tree;
  p->particles = orig->particles;
#if USE_MALLOC
  int * arr = (int *) malloc(sizeof(int) * orig->subset_size);
#else
  int * arr = new int[orig->subset_size];
#endif
  p->subset_size = 0;
  for (int i = 0; i < orig->subset_size; i++) {
    if (cover_p(p->tree->area, p->particles[orig->subset_arr[i]]->pos)) {
      arr[i] = orig->subset_arr[i]; 
      p->subset_size++; 
    } else {
      arr[i] = -1;
    }
  }
  if (p->subset_size > 0) {
#if USE_MALLOC
    p->subset_arr = (int *) malloc(sizeof(int) * p->subset_size);
#else
    p->subset_arr = new int [p->subset_size];
#endif
    int j = 0;
    for (int i = 0; i < orig->subset_size; i++) {
      if (arr[i] != -1) {
        p->subset_arr[j] = arr[i];
        j++;
      }
    }
  } else {
    p->subset_arr = NULL;
    p->subset_size = 0;
  }
  return p;
}

void * build_tree_rec(void * args)
{
  int t0, t1;
#if USE_MALLOC
  pthread_t ths[N_CHILDREN];
  bt_thread_dat parr[N_CHILDREN];
#endif
  bt_thread_dat * p = (bt_thread_dat *) args;
  t0 = current_real_time_micro();
  bt_thread_dat * np = particles_in_tree(p);
  t1 = current_real_time_micro();
//  printf("%d %d %d\n", p->subset_size, np->subset_size, t1-t0);
  if (np->subset_size == 1) {
    particle * par = np->particles[np->subset_arr[0]];
    np->tree->add_particle(par->mass, par->pos);
  } else if (np->subset_size > 1) {
    void * ret;
    int n_threads = N_CHILDREN - 1;
    np->tree->divide();
#if !USE_MALLOC
    bt_thread_dat * parr = new bt_thread_dat[N_CHILDREN];
    pthread_t * ths = new pthread_t[n_threads];
#endif
    for (int i = 0; i < N_CHILDREN; i++) {
      parr[i].tree = np->tree->subspaces[i];
      parr[i].particles = np->particles;
      parr[i].subset_arr = np->subset_arr;
      parr[i].subset_size = np->subset_size;
      if (i < n_threads) {
        pthread_create(&ths[i], NULL, build_tree_rec, (void *) &parr[i]);
      } else {
        build_tree_rec(&parr[i]);
      }
    }
    for (int i = 0; i < n_threads; i++) 
      pthread_join(ths[i], (void **) ret);
  }
}

void particles_in_tree2(bt_thread_dat * orig,
  bt_thread_dat * parr)
{
  int t0, t1, t2, t3, t4, t5, t6;
  
  t0 = current_real_time_micro();
#if USE_MALLOC
  int * arr = (int *) malloc(sizeof(int) * orig->subset_size);
#else
  int * arr = new int[orig->subset_size];
#endif
  t1 = current_real_time_micro();
  
  orig->tree->divide();
  t2 = current_real_time_micro();
  
  for (int i = 0; i < N_CHILDREN; i++) {
    parr[i].tree = orig->tree->subspaces[i];
    parr[i].particles = orig->particles;
    parr[i].subset_size = 0;
  }
  
  t3 = current_real_time_micro();
  for (int i = 0; i < orig->subset_size; i++) {
    int idx = select_covering_rectangle( 
      orig->particles[orig->subset_arr[i]]->pos, orig->tree->area);
    parr[idx].subset_size++;
    arr[i] = idx;
  }
  
  t4 = current_real_time_micro();
  int indices[N_CHILDREN];
  for (int i = 0; i < N_CHILDREN; i++) {
    if (parr[i].subset_size > 0) {
#if USE_MALLOC
      parr[i].subset_arr = (int *) malloc(sizeof(int) * parr[i].subset_size);
#else
      parr[i].subset_arr = new int [parr[i].subset_size];
#endif
    } else {
      parr[i].subset_arr = NULL;
    }
    indices[i] = 0;
  }
  
  t5 = current_real_time_micro();
  for (int i = 0; i < orig->subset_size; i++) {
    int idx = arr[i];
    parr[idx].subset_arr[indices[idx]] = orig->subset_arr[i];
    indices[idx]++;
  }
  
  t6 = current_real_time_micro();
  printf("%d %d %d %d %d %d %d %d\n", orig->subset_size, t6-t0, 
  t1-t0, t2-t1, t3-t2, t4-t3, t5-t4, t6-t5);
}

void * build_tree_rec2(void * args)
{
  pthread_t ths[N_CHILDREN];
  bt_thread_dat parr[N_CHILDREN];
  bt_thread_dat * p = (bt_thread_dat *) args;
 
  if (p->subset_size == 1) {
    particle * par = p->particles[p->subset_arr[0]];
    p->tree->add_particle(par->mass, par->pos);
  } else if (p->subset_size > 1) {
    particles_in_tree2(p, parr);
    void * ret;
    int n_threads = N_CHILDREN - 1;
    for (int i = 0; i < N_CHILDREN; i++) {
      if (i < n_threads) {
        pthread_create(&ths[i], NULL, build_tree_rec2, (void *) &parr[i]);
      } else {
        build_tree_rec2(&parr[i]);
      }
    }
    for (int i = 0; i < n_threads; i++) 
      pthread_join(ths[i], (void **) ret);
  }
}

space * build_tree(particle ** particles, int n_particles)
{
  bt_thread_dat dat;
  t_real limit = calc_limit(particles, n_particles);
  rectangle * rec = make_entire_rectangle(limit);
  space * tree = make_empty_space(rec);
  dat.tree = tree;
  dat.particles = particles;
  dat.subset_size = n_particles;
#if USE_MALLOC
  dat.subset_arr = (int *) malloc(sizeof(int) * dat.subset_size);
#else
  dat.subset_arr = new int[dat.subset_size];
#endif
  for (int i = 0; i < dat.subset_size; i++) dat.subset_arr[i] = i;
//  build_tree_rec(&dat);
  build_tree_rec2(&dat);
  return tree;
}

struct btup_thd_dat {
  space * tree;
  rectangle * area;
  particle ** particles;
  int * subset_arr;
  int subset_size;
};

btup_thd_dat * particles_in_tree(btup_thd_dat * orig)
{
#if USE_MALLOC
  btup_thd_dat * p = (btup_thd_dat *) malloc(sizeof(btup_thd_dat));
#else
  btup_thd_dat * p = new btup_thd_dat;
#endif
  p->tree = orig->tree; 
  p->area = orig->area;
  p->particles = orig->particles;
#if USE_MALLOC
  int * arr = (int *) malloc(sizeof(int) * orig->subset_size);
#else
  int * arr = new int[orig->subset_size];
#endif
  p->subset_size = 0;
  for (int i = 0; i < orig->subset_size; i++) {
    if (cover_p(p->area, p->particles[orig->subset_arr[i]]->pos)) {
      arr[i] = orig->subset_arr[i]; 
      p->subset_size++; 
    } else {
      arr[i] = -1;
    }
  }
  if (p->subset_size > 0) {
#if USE_MALLOC
    p->subset_arr = (int *) malloc(sizeof(int) * p->subset_size);
#else
    p->subset_arr = new int [p->subset_size];
#endif
    int j = 0;
    for (int i = 0; i < orig->subset_size; i++) {
      if (arr[i] != -1) {
        p->subset_arr[j] = arr[i];
        j++;
      }
    }
  } else {
    p->subset_arr = NULL;
    p->subset_size = 0;
  }
  return p;
}

void * build_tree_bottomup_rec(void *args)
{
#if USE_MALLOC
  btup_thd_dat parr[N_CHILDREN];
  pthread_t ths[N_CHILDREN];
#endif
  btup_thd_dat * p = (btup_thd_dat *) args;
  btup_thd_dat * np = particles_in_tree(p);
  p->tree = make_empty_space(np->area);
  if (np->subset_size == 1) {
    particle * par = p->particles[np->subset_arr[0]];
    p->tree->add_particle(par->mass, par->pos);
  } else if (np->subset_size > 1) {
    int n_threads = N_CHILDREN - 1;
    void * ret;
#if !USE_MALLOC
    btup_thd_dat * parr = new btup_thd_dat[N_CHILDREN];
    pthread_t * ths = new pthread_t[n_threads];
#endif
    for (int i = 0; i < N_CHILDREN; i++) {
      parr[i].tree = NULL;
      parr[i].area = make_sub_rectangle(np->area, i);
      parr[i].particles = np->particles;
      parr[i].subset_arr = np->subset_arr;
      parr[i].subset_size = np->subset_size;
      if (i < n_threads) {
        pthread_create(&ths[i], NULL, build_tree_bottomup_rec, 
          (void *) &parr[i]);
      } else {
        build_tree_bottomup_rec(&parr[i]);
     }
    }
    p->tree->state = MULTIPLE_PARTICLES;
#if USE_MALLOC
    p->tree->subspaces = (space **) malloc(sizeof(space *) * N_CHILDREN);
#else
    p->tree->subspaces = new space * [N_CHILDREN];
#endif
    for (int i = 0; i < N_CHILDREN; i++) {
      if (i < n_threads) pthread_join(ths[i], (void **) ret);
      p->tree->subspaces[i] = parr[i].tree;
    }
  }
}

space * build_tree_bottomup(particle ** particles, int n_particles)
{
  btup_thd_dat dat;
	t_real limit = calc_limit(particles, n_particles);
  dat.tree = NULL;
  dat.area = make_entire_rectangle(limit);
  dat.particles = particles;
  dat.subset_size = n_particles;
#if USE_MALLOC
  dat.subset_arr = (int *) malloc(sizeof(int) * dat.subset_size);
#else
  dat.subset_arr = new int[dat.subset_size];
#endif
  for (int i = 0; i < dat.subset_size; i++) dat.subset_arr[i] = i;
  build_tree_bottomup_rec(&dat);
  return dat.tree;
}

/* Serial version of tree generation */
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
