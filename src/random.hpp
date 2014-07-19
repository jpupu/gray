#include <random>
#include "mymath.hpp"
#include "util.hpp"

class Sample;


class SampleGenerator
{
public:
    SampleGenerator (int n2d, int n_samples);

    int allocate_2d ();

    void generate(std::mt19937& rng);

    Sample next ();

    std::vector<Sample> samples;

    float randf () { return distribution(*rng); }

private:
    unsigned int n2d;
    int count_2d;
    int index;
    int n_samples;
    std::mt19937* rng;
    std::uniform_real_distribution<float> distribution;
};


class Sample
{
public:
    Sample () : sgen(nullptr), index_2d(0) {}
    Sample (SampleGenerator* sgen) : sgen(sgen), index_2d(0) {}

    vec2 get2d ()
    {
        if (index_2d < samples_2d.size()) return samples_2d[index_2d++];
        // if (index_2d == samples_2d.size()) { index_2d = 0; }
        return vec2(sgen->randf(), sgen->randf());
        // return samples_2d[index_2d++];
    }

    SampleGenerator* sgen;
    std::vector<vec2> samples_2d;
    unsigned int n2d;
    unsigned int index_2d;

};

