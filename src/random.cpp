#include "random.hpp"
#include "util.hpp"
#include <exception>

SampleGenerator::SampleGenerator (int n2d, int samples_per_pixel)
    : spp(samples_per_pixel),
    samples(samples_per_pixel),
    n2d(n2d) 
{ }


SampleGeneratorRandom::SampleGeneratorRandom (int n2d, int samples_per_pixel)
    : SampleGenerator(n2d, samples_per_pixel)
{ }

void SampleGeneratorRandom::generate (std::mt19937* rng)
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


SampleGeneratorStratified::SampleGeneratorStratified (int n2d, int samples_per_pixel)
    : SampleGenerator(n2d, samples_per_pixel)
{
    dim = (int)sqrtf(spp);
    if (spp != dim*dim) {
        throw std::range_error("Stratified sampler need spp to be a square.");
    }
}

void SampleGeneratorStratified::generate (std::mt19937* rng)
{
    this->rng = rng;
    distribution = std::uniform_real_distribution<float>(0,1);

    for (int i = 0; i < spp; i++) {
        samples[i] = Sample(rng, n2d);
    }

    std::vector<int> indices(spp);
    for (unsigned int i = 0; i < spp; i++) {
        indices[i] = i;
    }
    double dimrcp = 1.0 / dim;
    for (unsigned int j = 0; j < n2d; j++) {
        std::shuffle(indices.begin(), indices.end(), *rng);
        for (int v = 0; v < dim; v++) {
            for (int u = 0; u < dim; u++) {
                int i = u + v*dim;
                float uf = std::uniform_real_distribution<float>(u*dimrcp, (u+1)*dimrcp)(*rng);
                float vf = std::uniform_real_distribution<float>(v*dimrcp, (v+1)*dimrcp)(*rng);
                // float uf = std::uniform_real_distribution<float>(0, 1)(*rng);
                // float vf = std::uniform_real_distribution<float>(0, 1)(*rng);
                samples[indices[i]].samples_2d[j] = vec2(uf, vf);
            }
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

