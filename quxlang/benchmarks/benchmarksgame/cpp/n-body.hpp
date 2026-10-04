#ifndef QUXLANG_BENCHMARKSGAME_CPP_N_BODY_HPP
#define QUXLANG_BENCHMARKSGAME_CPP_N_BODY_HPP
#include <array>

/** Position, velocity, and mass of one body. */
struct planet
{
    double x, y, z, vx, vy, vz, mass;
};

/** Initializes the five prescribed solar-system bodies. */
void initialize_planets(std::array< planet, 5 >& bodies);

/** Advances one step of the simple symplectic integrator. */
void advance(std::array< planet, 5 >& bodies, double dt);

/** Sums kinetic and gravitational potential energy. */
double energy(std::array< planet, 5 > const& bodies);

#endif
