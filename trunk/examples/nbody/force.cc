/*
 * force.cc
 */
#include "def.h"

inline vect_t newton_acceleration (t_real mass, vect_t cg, vect_t pos)
{
  t_real x0 = VX(pos), y0 = VY(pos), z0 = VZ(pos);
  t_real x1 = VX(cg), y1 = VY(cg), z1 = VZ(cg);
  t_real dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
  t_real l2 = dx * dx + dy * dy + dz * dz;
  t_real k = mass / (l2 * rsqrt (l2));
  return make_vect(k * dx, k * dy, k * dz);
}    

vect_t space::calc_accel(vect_t pos)
{
  switch (state) {
    case ONE_PARTICLE: {
      return make_vect(0.0, 0.0, 0.0);
    } /* ONE_PARTICLE */
    case MULTIPLE_PARTICLES: {
      int idx = select_covering_rectangle(pos, area);
      t_real resultx = 0.0, resulty = 0.0, resultz = 0.0;
      for (int i = 0; i < N_CHILDREN; i++) {
        space * s = subspaces[i];
        if (i == idx) {
          vect_t sv = s->calc_accel(pos);
          resultx += VX(sv);
          resulty += VY(sv);
          resultz += VZ(sv);
        } else {
          if (s) {
            vect_t sv = s->calc_accel1(pos);
            resultx += VX(sv);
            resulty += VY(sv);
            resultz += VZ(sv);
          }
        }
      }
      return make_vect(resultx, resulty, resultz);
    } /* MULTIPLE_PARTICLES */
    default: {
      printf("NO_PARTICLES!\n"); 
      exit(1); // FIXME: pthread_exit
      return make_vect(0.0, 0.0, 0.0); /* never reach */
    } /* default */
  }
}

#if 0
const t_real approx_theta = 0.67 * 0.67; /* 1.05263 */
#else
const t_real approx_theta = 1.0 * 1.0; /* 1.05263 */
#endif

vect_t space::calc_accel1(vect_t pos)
{
  switch (state) {
  case ONE_PARTICLE: {
    return newton_acceleration(mass, cg, pos);
  } /* ONE_PARTICLE */
  case MULTIPLE_PARTICLES: {
    t_real x0 = VX(pos), y0 = VY(pos), z0 = VZ(pos);
    t_real x1 = VX(cg), y1 = VY(cg), z1 = VZ(cg);
    t_real dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    t_real l2 = dx * dx + dy * dy + dz * dz;
    if (diameter2 < l2 * approx_theta) {
	    t_real k = mass / (l2 * rsqrt (l2));
	    return make_vect(k * dx, k * dy, k * dz);
    } else {
	    t_real resultx = 0.0, resulty = 0.0, resultz = 0.0;
	    for (int i = 0; i < N_CHILDREN; i++) {
        space * s = subspaces[i];
        if (s) {
          vect_t sv = s->calc_accel1(pos);
          resultx += VX(sv);
          resulty += VY(sv);
          resultz += VZ(sv);
        }
	    }
	    return make_vect(resultx, resulty, resultz);
    }
  } /* MULTIPLE_PARTICLES */
  default: {
      printf("NO_PARTICLES!\n"); 
      exit(1); // FIXME
      return make_vect(0.0, 0.0, 0.0); /* never reach */
    }
  } /* default */
}

void particle::set_accel(space * sp)
{
  vect_t a = sp->calc_accel(pos);
  accel = a;
}

void set_accels_dac(particle ** particles, int begin, int end, space * sp)
{
  if (end - begin < 5) {
    for (int i = begin; i <= end; i++) {
      particles[i]->set_accel(sp);
    }
  } else {
    int c = (begin + end - 1) / 2;
	  /* SEQUENTIAL */
    set_accels_dac(particles, begin, c, sp);
    set_accels_dac(particles, c + 1, end, sp);
  }
}

void set_accels(particle ** particles, int n_particles, space * sp)
{
  set_accels_dac(particles, 0, n_particles - 1, sp);
}

void particle::move(t_real dt)
{
  vect_t vel_prime = v_plus_v (vel, k_times_v(dt, accel));
  pos = v_plus_v (pos, k_times_v(dt, vel_prime));
  vel = vel_prime;
}

void move_particles(particle ** particles, int n_particles, t_real dt)
{
  for (int i = 0; i < n_particles; i++) {
    particles[i]->move(dt);
  }
}
