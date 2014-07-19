#include "random.hpp"
#include "util.hpp"

SampleGenerator::SampleGenerator (int n2d, int n_samples)
    : samples(n_samples), n2d(n2d), count_2d(0), index(0), n_samples(n_samples)
{ }

int SampleGenerator::allocate_2d ()
{
    return count_2d++;
}

void SampleGenerator::generate (std::mt19937& rng)
{
    this->rng = &rng;
    distribution = std::uniform_real_distribution<float>(0,1);
    auto randf = std::bind(distribution, rng);

    for (int i = 0; i < n_samples; i++) {
        samples[i] = Sample(this);
        samples[i].samples_2d.resize(n2d);
        for (unsigned int j = 0; j < n2d; j++) {
            samples[i].samples_2d[j] = vec2(randf(), randf());
            // sample.samples_2d[i] = vec2(.5,.5);
            // sample.samples_2d[i] = vec2(.5,(randf()>=.5)?.1:.6);
        }
    }

    index = 0;
}

Sample SampleGenerator::next ()
{
    return samples[index++];
    // std::uniform_real_distribution<float> distribution(0,1);
    // // auto randf = std::bind(distribution, *rng);
    // Sample sample;
    // sample.samples_2d.resize(n2d);
    // for (int j = 0; j < n2d; j++) {
    //     // sample.samples_2d[j] = vec2(randf(), randf());
    //     sample.samples_2d[j] = vec2(distribution(*rng), distribution(*rng));
    //     sample.samples_2d[j] = vec2(.5,.5);
    // }
    // return sample;
}