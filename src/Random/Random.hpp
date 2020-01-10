#pragma once
#include <stdint.h>
#include <cmath>

/**
BEGIN minimalist PCG code
*/

typedef struct pcg_state_setseq_64 {    // Internals are *Private*.
    uint64_t state;             // RNG state.  All values are possible.
    uint64_t inc;               // Controls which RNG sequence (stream) is
    // selected. Must *always* be odd.
} pcg32_random_t;

static pcg32_random_t pcg32_global = { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL };

static inline uint32_t pcg32_random_r(pcg32_random_t* rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static inline uint32_t random() {
    return pcg32_random_r(&pcg32_global);
}

/**
END minimalist PCG code
*/

// map random value to [0,range) with slight bias
static inline uint32_t random(uint32_t range) {
    uint64_t random32bit, multiresult;
    random32bit =  random();
    multiresult = random32bit * range;
    return multiresult >> 32; // [0, range)
}
// map random value to [0,range) with slight bias
static inline double randomf() {
    return random() / (double)(0xffff'ffff);
}

static inline double randomnorm(double u, double s) noexcept {
    return std::sqrt(-2 * std::log(randomf())) * std::cos(2 * 3.14159265359 * randomf());
}

template<typename T>
void shuffle(T* ptr, size_t size) noexcept {
	size_t i;
	for (i=size; i>1; i--) {
		size_t nextpos = random(i);
		auto tmp = ptr[i-1];// likely in cache
		auto val = ptr[nextpos]; // could be costly
		ptr[i - 1] = val;
		ptr[nextpos] = tmp; // you might have to read this store later
	}
}