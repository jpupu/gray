#include <random>
#include "mymath.hpp"
#include "util.hpp"

class Sample;


class SampleGenerator
{
public:
    /// @par n2d    Number of 2D samples per sample set. This tells e.g. how many
    ///             bounces can be made with stratified samples. If more samples
    ///             are accessed, the rest will be totally random.
    /// @par samples_per_pixel  Number of sample sets to generate.
    SampleGenerator (int n2d, int samples_per_pixel);

    /// Generates sample sets for a new pixel.
    void generate(std::mt19937* rng);

    Sample& get (int index) { return samples[index]; }

    float randf () { return distribution(*rng); }
    vec2 rand2f () { return vec2(distribution(*rng), distribution(*rng)); }

private:
    int spp;
    std::vector<Sample> samples;
    unsigned int n2d;
    std::mt19937* rng;
    std::uniform_real_distribution<float> distribution;
};


class Sample
{
public:
    Sample ();
    Sample (std::mt19937* rng, int n2d);

    vec2 get2d ()
    {
        if (index_2d < samples_2d.size()) {
            return samples_2d[index_2d++];
        }
        return rand2f();
    }

    float randf () { return distribution(*rng); }
    vec2 rand2f () { return vec2(distribution(*rng), distribution(*rng)); }

private:
    friend class SampleGenerator;
    std::vector<vec2> samples_2d;
    unsigned int index_2d;
    std::mt19937* rng;
    std::uniform_real_distribution<float> distribution;
};

