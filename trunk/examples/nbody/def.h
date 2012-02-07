/*
 * def.h
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef REAL_TYPE
#define REAL_TYPE float
#endif
typedef REAL_TYPE t_real;

#ifndef UNBOX_VECT 
#define UNBOX_VECT 1
#endif

struct vect
{
#if UNBOX_VECT
  void * operator new(size_t sz) {
    printf("do not call vect::new\n");
	exit(1);
  }
#endif /* UNBOX_VECT */
  vect (t_real xx, t_real yy, t_real zz) {
    x = xx; y = yy; z = zz;
  }
  vect () { }
  t_real x, y, z;
};

#if UNBOX_VECT
#define vect_t vect
#define VX(v) (v).x
#define VY(v) (v).y
#define VZ(v) (v).z
static inline vect_t make_vect (t_real x, t_real y, t_real z)
{
    return vect (x, y, z);
}
#else
#define vect_t vect *
#define VX(v) (v)->x
#define VY(v) (v)->y
#define VZ(v) (v)->z
static inline vect_t make_vect (t_real x, t_real y, t_real z)
{
  vect_t p = new vect (x, y, z);
  return p;
}
#endif

static inline vect_t v_plus_v (vect_t v0, vect_t v1) 
{
  return make_vect(VX(v0) + VX(v1), VY(v0) + VY(v1), VZ(v0) + VZ(v1));
}

static inline vect_t v_minus_v (vect_t v0, vect_t v1) 
{
  return make_vect(VX(v0) - VX(v1), VY(v0) - VY(v1), VZ(v0) - VZ(v1));
}

void print_vect(vect_t);
static inline vect_t k_times_v(t_real k, vect_t v) {
  return make_vect (k * VX(v), k * VY(v), k * VZ(v));
}
static inline vect_t v_div_k (t_real k, vect_t v) {
  return make_vect (VX(v) / k, VY(v) / k, VZ(v) / k);
}

struct rectangle
{
  vect_t ll;
  vect_t ur;
  rectangle(vect_t ll_, vect_t ur_) {
    ll = ll_; ur = ur_;
  }
};

struct mass_momentum 
{
  void * operator new(size_t sz) {
    printf("do not call mass_momentum::new\n");
    //st_wg_die((void *)1);
  }
  mass_momentum (t_real ma, vect_t mo, int nn) { 
    mass = ma; momentum = mo; n_nodes = nn; 
  }
  t_real mass;
  vect_t momentum;
  int n_nodes;
};

enum space_state { NO_PARTICLE, ONE_PARTICLE, MULTIPLE_PARTICLES };

#define N_CHILDREN 8

struct particle;
struct space
{
  space_state state;
  space ** subspaces;
  
  t_real mass;
  vect_t cg;
  rectangle * area;
  t_real diameter2;
  
  int cached;
  space (space_state s, t_real m, vect_t c, rectangle * a, t_real d) {
    state = s; mass = m; cg = c; area = a; diameter2 = d;
    cached = 0;
  }
  void add_particle(t_real, vect_t);
  mass_momentum set_mass_and_cg();
  vect_t calc_accel(vect_t);
  vect_t calc_accel1(vect_t);
};

struct particle_data
{
  int id;
  t_real mass;
  t_real px, py, pz;
  t_real vx, vy, vz;
  particle_data (int id_, t_real mass_, t_real px_, t_real py_, t_real pz_, 
		 t_real vx_, t_real vy_, t_real vz_) {
    id = id_; mass = mass_; px = px_; py = py_; pz = pz_;
    vx = vx_; vy = vy_; vz = vz_;
  }
};

struct particle
{
  int id;
  t_real mass;
  vect_t accel;
  vect_t pos;
  vect_t vel;
  particle (int id_, t_real mass_, vect_t accel_, vect_t pos_, vect_t vel_) {
    id = id_; mass = mass_; accel = accel_;
    pos = pos_; vel = vel_;
  }
  void dump();		/* dump content of the particle */
  void set_accel(space *);
  void move(t_real);
  void centerlize(t_real, t_real, t_real, t_real, t_real, t_real);
  t_real calc_limit();
};


void print_rectangle(rectangle *);

static inline t_real edge(rectangle * rec)
{
  return (VX(rec->ur) - VX(rec->ll));
}

static inline t_real distance2(vect_t p0, vect_t p1)
{
  t_real dx = VX (p1) - VX (p0);
  t_real dy = VY (p1) - VY (p0);
  t_real dz = VZ (p1) - VZ (p0);
  return dx * dx + dy * dy + dz * dz;
}

static inline float rsqrt(float x)
{
  float y;
#if defined(sparc) && defined(__GNUC__)
  asm("fsqrts %1,%0" : "=f"(y) : "f"(x));
#else
  y = (float) sqrt((double)x);
#endif
  return y;
}

static inline double rsqrt(double x)
{
  double y;
#if defined(sparc) && defined(__GNUC__)
  asm("fsqrtd %1,%0" : "=f"(y) : "f"(x));
#else
  y = sqrt (x);
#endif
  return y;
}

static inline t_real rabs(t_real x)
{
  if (x < 0.0) return -x;
  else return x;
}

static inline t_real distance(vect_t p0, vect_t p1)
{
  return rsqrt(distance2(p0, p1));
}

particle ** generate_particles(int);
space * make_empty_space(rectangle *);
rectangle * make_entire_rectangle(t_real);
space * generate_tree(particle **, int);
void set_accels(particle **, int, space *);
void move_particles(particle **, int, t_real);

int cover_p(rectangle *, vect_t);
t_real distance2(vect_t, vect_t);
int select_covering_rectangle(vect_t, rectangle *);

int current_real_time_micro(void);
int current_real_time_milli(void);
