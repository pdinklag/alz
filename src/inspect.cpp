/**
 * inspect.cpp
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

#include <cmdline/program.hpp>
#include <iopp/load_file.hpp>

#include <alz/approximate_lz77.hpp>

class ALZInspect : public cmdline::Program {
private:
    std::string filename;
    
    size_t prefix = SIZE_MAX;

    uint64_t sampling = 4;
    uint64_t fp_window = 10;
    uint64_t window = 16 * 1024 * 1024;

    template<std::unsigned_integral Index>
    void parse(size_t const n) {
        iopp::FileInputStream in(filename, 0, n);
        alz::ApproximateLZ77<Index> approx(sampling, fp_window);
        auto [size, est_cardinality] = approx.inspect(in, n, window);
        double const ratio = 100.0 * double(est_cardinality) / double(size);
        std::cout << "n=" << n << ", parsing=" << size << ", est_cardinality=" << est_cardinality << " (" << ratio << "%)" << std::endl;
    }

public:
    ALZInspect() : cmdline::Program("alz", "Compute and encode an approximate LZ77 factorization") {
        required_arg("file", filename, "The input file.");
        option('s', "sampling", sampling, "The sampling rate (2^value).");
        option('l', "len", fp_window, "The fingerprint window size.");
        option('p', "prefix", prefix, "Process only this prefix of the input file.");
        option('w', "window", window, "The input window size; leave at 0 to load entire input into RAM.");
    }

    virtual int main() override {
        size_t const n = std::min(std::filesystem::file_size(filename), prefix);
        window = (window == 0) ? n : std::min(window, n);
        
        if(n >= (1ULL << 31)) {
            parse<uint64_t>(n);
        } else {
            parse<uint32_t>(n);
        }
        return 0;
    }
};

int main(int argc, char** argv) {
    return ALZInspect().run(argc, argv);
}
