#include "random.hpp"
#include "util.hpp"

SampleGenerator::SampleGenerator (int n2d, int samples_per_pixel)
    : spp(samples_per_pixel),
    samples(samples_per_pixel),
    n2d(n2d) 
{ }

void SampleGenerator::generate (std::mt19937* rng)
{
    this->rng = rng;
    distribution = std::uniform_real_distribution<float>(0,1);

    for (int i = 0; i < spp; i++) {
        samples[i] = Sample(rng, n2d);

        for (unsigned int j = 0; j < n2d; j++) {
            samples[i].samples_2d[j] = rand2f();
        }
    }
}

Sample::Sample ()
    : samples_2d(0),
    index_2d(0),
    rng(nullptr)
{ }

Sample::Sample (std::mt19937* rng, int n2d)
    : samples_2d(n2d),
    index_2d(0),
    rng(rng),
    distribution(0,1)
{ }

