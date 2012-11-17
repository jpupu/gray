#ifndef FILM_HPP
#define FILM_HPP

#include "gray.hpp"
#include <vector>

struct Pixel
{
    Spectrum L;
    float weight;
    Spectrum tonemapped;

    Pixel () : L(0.f), weight(0.0f), tonemapped(0.f) { }

    void add (const Spectrum& Ln, float wn)
    {
        L += Ln;
        weight += wn;
    }

    Spectrum normalized () const
    {
        return L / weight;
    }

    float luminosity () const
    {

        return L[0] * 0.27 + L[1] * 0.67 + L[2] * 0.06;
    }
};

class Film
{
public:
    Film (int xres, int yres);

    /// #x and #y are in range 0..1
    void add_sample (float x, float y, const Spectrum& s);

    void save (const char* filename);

public:
    int xres, yres;

private:
    std::vector<Pixel> data;

    void save_png (const char* filename);

    void tone_mapping ();
};

#endif /* FILM_HPP */
