#include "film.hpp"
#include "lodepng.h"
// #include <cmath>
#include <cstdint>
#include <memory>
using std::auto_ptr;

Film::Film (int xres, int yres)
    : xres(xres), yres(yres), data(xres*yres)
{ }

void Film::add_sample (float x, float y, const Spectrum& s)
{
    int xi = clamp((int)(x*xres), 0, xres-1);
    int yi = clamp((int)(y*yres), 0, yres-1);
    data[xi + yi*xres].add(s, 1.0);
}


void Film::save (const char* filename)
{
    save_png(filename);
}

void Film::save_png (const char* filename)
{
    tone_mapping();
    std::vector<uint8_t> rgb(xres*yres*3);

    for (int i = 0; i < xres*yres; i++) {
//         Spectrum L = data[i].normalized();
        Spectrum L = data[i].tonemapped;
        L = clamp(L*255.0f, Spectrum(0), Spectrum(255));
        for (int k = 0; k < 3; k++) {
            rgb[i*3+k] = L[k];
        }
    }

    lodepng_encode24_file(filename, &rgb[0], xres, yres);
}

void Film::tone_mapping ()
{
//     // Calculate root mean square.
//     float lum_sq_sum = 0;
//     for (int i = 0; i < xres*yres; i++) {
//         float l = data[i].luminosity();
//         lum_sq_sum += l*l;
//     }
//     float rms = sqrtf(lum_sq_sum / (xres*yres));

    /*
     * Erik Reinhard:
     * Parameter Estimation for Photographic Tone Reproduction
     */

    float Lmax = 0;
    float Lmin = 20000;

    const float delta = 0.01; // small constant
    // log average luminance
    float Lav = 0;
    float Lmean = 0;
    for (int i = 0; i < xres*yres; i++) {
        float Lw = data[i].luminosity();
        if (Lw > Lmax) Lmax = Lw;
        if (Lw < Lmin) Lmin = Lw;
        Lav += log(delta + Lw);
        Lmean += Lw;
    }
    Lav = exp(Lav / (xres*yres));
    Lmean /= xres*yres;
    Lmin += delta;

    const float alpha = 0.18 * powf(4, (2*log2(Lav)-log2(Lmin)-log2(Lmax)) / (log2(Lmax)-log2(Lmin)));

    const float Lwhite = 1.2 * powf(2, log2(Lmax) - log2(Lmin) - 5);

    float Lwhite2 = Lwhite*Lwhite;
    for (int i = 0; i < xres*yres; i++) {
        float L = alpha / Lav * data[i].luminosity();
//         float Ld = L * (1 + L/Lwhite2) / (1 + L);
        float Ld = L / (1 + L);
        data[i].tonemapped = data[i].normalized() * Ld;
    }

    printf("Lmin %f\n", Lmin);
    printf("Lmax %f\n", Lmax);
    printf("Lmean %f\n", Lmean);
    printf("Lav %f\n", Lav);
    printf("alpha %f\n", alpha);
    printf("Lwhite %f\n", Lwhite);
}

