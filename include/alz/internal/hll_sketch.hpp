/**
 * hll_sketch.hpp
 * part of pdinklag/alz
 * 
 * MIT License
 * 
 * Copyright (c) Patrick Dinklage
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _ALZ_HLL_SKETCH_HPP
#define _ALZ_HLL_SKETCH_HPP

#include <array>
#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>

#include "murmur_hash3.hpp"

namespace alz::internal {
template<uint8_t b_ = 6> // 2^b = number of registers
class HLLSketch {
private:
    using Hash = uint64_t;
    using Register = Hash; // nb: we could potentially use less bits if b_ >= bitsizeof(Hash)/2, but for 64-bit hashes, that is impractical

    static constexpr size_t num_registers_ = 1ULL << b_;
    static constexpr Hash reg_mask_ = num_registers_ - 1;

    static constexpr double alpha() {
        if constexpr(num_registers_ == 16) {
            return 0.673; 
        } else if constexpr(num_registers_ == 32) {
            return 0.697;
        } else if constexpr(num_registers_ == 64) {
            return 0.709;
        } else {
            return 0.7213 / (1.0 + 1.079/num_registers_);
        }
    }

    std::array<Register, num_registers_> reg_;

public:
    HLLSketch() {
        for(size_t i = 0; i < num_registers_; i++) {
            reg_[i] = 0;
        }
    }

    template<typename Value>
    void push(Value const v) {
        Hash h = MurmurHash3()(v);
        size_t const i = h & reg_mask_;
        size_t const rho = std::countl_zero(h) + 1;
        if(rho > reg_[i]) {
            reg_[i] = rho;
        }
    }

    void merge(HLLSketch const& other) {
        for(size_t i = 0; i < num_registers_; ++i) {
            reg_[i] = std::max(reg_[i], other.reg_[i]);
        }
    }

    double estimate() const {
        double E = 0;
        int V = 0;

        for(size_t i = 0; i < num_registers_; ++i) {
            auto const r = reg_[i];
            V += (r == 0);
            E += 1.0 / (1ULL << r);
        }
        E = alpha() * num_registers_ * num_registers_ / E;

        if(E <= 5.0 / 2.0 * num_registers_ && V != 0) {
            return num_registers_*log(static_cast<double>(num_registers_)/V);
        } else if (E <= (1ULL << 32)/30) {
            return E;
        } else {
            return -(1LL << 32) * log(1 - E/(1LL << 32));
        }
    }
};

}

#endif
