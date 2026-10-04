#include "n-body.hpp"
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

/** Advances one step of the simple symplectic integrator. */
void advance(std::array< planet, 5 >& bodies, double dt)
{
    for (std::size_t i = 0; i < 5; ++i)
    {
        for (std::size_t j = i + 1; j < 5; ++j)
        {
            double dx = bodies[i].x - bodies[j].x;
            double dy = bodies[i].y - bodies[j].y;
            double dz = bodies[i].z - bodies[j].z;
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            double magnitude = dt / (distance * distance * distance);
            bodies[i].vx -= dx * bodies[j].mass * magnitude;
            bodies[i].vy -= dy * bodies[j].mass * magnitude;
            bodies[i].vz -= dz * bodies[j].mass * magnitude;
            bodies[j].vx += dx * bodies[i].mass * magnitude;
            bodies[j].vy += dy * bodies[i].mass * magnitude;
            bodies[j].vz += dz * bodies[i].mass * magnitude;
        }
    }
    for (std::size_t i = 0; i < 5; ++i)
    {
        bodies[i].x += dt * bodies[i].vx;
        bodies[i].y += dt * bodies[i].vy;
        bodies[i].z += dt * bodies[i].vz;
    }
}

/** Sums kinetic and gravitational potential energy. */
double energy(std::array< planet, 5 > const& bodies)
{
    double result = 0;
    for (std::size_t i = 0; i < 5; ++i)
    {
        double speed_squared = bodies[i].vx * bodies[i].vx + bodies[i].vy * bodies[i].vy + bodies[i].vz * bodies[i].vz;
        result += 0.5 * bodies[i].mass * speed_squared;
        for (std::size_t j = i + 1; j < 5; ++j)
        {
            double dx = bodies[i].x - bodies[j].x;
            double dy = bodies[i].y - bodies[j].y;
            double dz = bodies[i].z - bodies[j].z;
            result -= bodies[i].mass * bodies[j].mass / std::sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    return result;
}

/** Offsets momentum, advances the system, and prints its initial and final energy. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    std::size_t n = std::strtoul(argv[1], nullptr, 10);
    std::array< planet, 5 > bodies{};
    initialize_planets(bodies);
    double px = 0, py = 0, pz = 0;
    for (std::size_t i = 0; i < 5; ++i)
    {
        px += bodies[i].vx * bodies[i].mass;
        py += bodies[i].vy * bodies[i].mass;
        pz += bodies[i].vz * bodies[i].mass;
    }
    bodies[0].vx = -px / bodies[0].mass;
    bodies[0].vy = -py / bodies[0].mass;
    bodies[0].vz = -pz / bodies[0].mass;
    std::printf("%.9f\n", energy(bodies));
    for (std::size_t iteration = 0; iteration < n; ++iteration)
    {
        advance(bodies, 0.01);
    }
    std::printf("%.9f\n", energy(bodies));
    return 0;
}

/* Initial conditions specified by the Benchmarks Game n-body workload. */
void initialize_planets(std::array< planet, 5 >& bodies)
{
    double pi = 3.141592653589793;
    double solar = 4.0 * pi * pi;
    double year = 365.24;
    bodies[0].mass = solar;
    bodies[1].x = 4.84143144246472090e+00;
    bodies[1].y = -1.16032004402742839e+00;
    bodies[1].z = -1.03622044471123109e-01;
    bodies[1].vx = 1.66007664274403694e-03 * year;
    bodies[1].vy = 7.69901118419740425e-03 * year;
    bodies[1].vz = -6.90460016972063023e-05 * year;
    bodies[1].mass = 9.54791938424326609e-04 * solar;
    bodies[2].x = 8.34336671824457987e+00;
    bodies[2].y = 4.12479856412430479e+00;
    bodies[2].z = -4.03523417114321381e-01;
    bodies[2].vx = -2.76742510726862411e-03 * year;
    bodies[2].vy = 4.99852801234917238e-03 * year;
    bodies[2].vz = 2.30417297573763929e-05 * year;
    bodies[2].mass = 2.85885980666130812e-04 * solar;
    bodies[3].x = 1.28943695621391310e+01;
    bodies[3].y = -1.51111514016986312e+01;
    bodies[3].z = -2.23307578892655734e-01;
    bodies[3].vx = 2.96460137564761618e-03 * year;
    bodies[3].vy = 2.37847173959480950e-03 * year;
    bodies[3].vz = -2.96589568540237556e-05 * year;
    bodies[3].mass = 4.36624404335156298e-05 * solar;
    bodies[4].x = 1.53796971148509165e+01;
    bodies[4].y = -2.59193146099879641e+01;
    bodies[4].z = 1.79258772950371181e-01;
    bodies[4].vx = 2.68067772490389322e-03 * year;
    bodies[4].vy = 1.62824170038242295e-03 * year;
    bodies[4].vz = -9.51592254519715870e-05 * year;
    bodies[4].mass = 5.15138902046611451e-05 * solar;
}
