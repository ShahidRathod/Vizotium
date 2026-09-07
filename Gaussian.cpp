#include <random>
#include <functional>
#include <cmath>
#include <numbers>
#include <complex>
#include <iostream>
#include <algorithm>
#include <initializer_list>

using ComplexT = std::complex<float>;
constexpr float pi = std::numbers::pi_v<float>;


constexpr int arrsz(int k, int d) { return 1 << (k * d); }

inline int pow2(int n) { return 1 << n; };

template <int k>
struct inverseFFT {
    static constexpr int sz = 1 << k;

    static ComplexT omega[sz];
    ComplexT* output = nullptr;
    ComplexT* input = nullptr;

    inverseFFT() = default;

    inverseFFT(ComplexT* inp, ComplexT* out) {
        input = inp;
        output = out;
    }

    void set_omega() {
        for (int i = 0; i < sz; i++) {
            omega[i] = std::polar(1.f, 2 * (pi * i) / sz);
        }
    }

    void eval_fft() {
        for (int i = 0; i < sz;i++) fft(sz, 1, input);
    }

    void fft(int n, int s, ComplexT* write) {

        if (n == 1) {
            write[0] = input[s];
            return;
        }
        int n2 = n / 2;
        ComplexT* odd = write + n2;
        ComplexT* even = odd + n2 / 2;
        fft(n2, s, odd);
        fft(n2, s + 1, even);
        for (int i = 0; i < n2; i++) {
            ComplexT ei = even[i] * omega[(i * sz / n2) % sz];
            ComplexT oi = odd[i];
            write[i] = oi + ei;
            write[n2 + i] = oi - ei;
        }
    }

};

template <int k>
struct ComplexNoise {
    static constexpr int sz = 1 << k;
    static constexpr int arr_sz = arrsz(k, 2);

    ComplexT noise[arr_sz];
    float spectral_bias[arr_sz];
    static ComplexT fx[sz], fy[sz], fft_buffer[3 * (sz * sz)];

    inverseFFT<k> fftx{ noise,fx };
    inverseFFT<k> ffty{ fx,fy };

    std::random_device seed_gen;
    std::normal_distribution<float> normal{ 0, 1 };
    int seed;

    void set_seed(int val) { seed = val; }
    void gen_seed() {
        seed = seed_gen();
    }



    void inverse_fft() {
        fftx.eval_fft();
        ffty.eval_fft();
    }

    void init_arr() {
        auto normal_gen = std::bind(normal, std::default_random_engine(seed));
        int z = sz / 2;
        for (int i = 0; i < z; i++) {
            for (int j = 0; j < z; j++) {
                ComplexT c(normal_gen(), normal_gen());
                noise[i * sz + j] = noise[(i + z) * sz + (j + z)] = c;
                noise[i * sz + j + z] = noise[(i + z) * sz + j] = std::conj(c);
            }
        }

        auto fun = [this](std::initializer_list<int> lst) {
            for (int x : lst) noise[x].imag(0.0);
            };

        fun({ z, sz * z, z + sz * sz, 0 });
    }

    ComplexNoise() {
        gen_seed();
        init_arr();
        std::fill(spectral_bias, spectral_bias + arr_sz, 1);
    }
};

int main() {
    ComplexNoise<4> cn;
    cn.inverse_fft();

}