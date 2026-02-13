/**
 * alz.cpp
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
#include <iopp/file_output_stream.hpp>

#include <alz/approximate_lz77.hpp>
#include "vbyte.hpp"

class ALZ : public cmdline::Program {
private:
    static constexpr size_t MAX_SIZE_32BIT = 1ULL << 31 - 1;

    static constexpr char const* MAGIC = "ALZ";
    static constexpr size_t MAGIC_LEN = 3;

    static constexpr char MODE_VBYTE = 'V';

    std::string filename;
    std::string output_filename;
    bool decompress = false;
    
    size_t prefix = SIZE_MAX;

    uint64_t sampling = 4;
    uint64_t fp_window = 10;
    uint64_t window = 0;

    bool flag_collapse_gaps = false;

    struct Flags {
        bool collapse_gaps : 1;
        uint8_t __reserved : 7;
    };
    static_assert(sizeof(Flags) == sizeof(char));

    std::string load_input(size_t const n) {
        pm::MemoryTimePhase t;
        std::cout << "load file " << filename << " (n=" << n << ") ... "; std::cout.flush();
        t.start();
        auto s = iopp::load_file_str(filename, n);
        t.stop();
        std::cout << "(" << (size_t)t.get_metric<pm::Stopwatch::ElapsedTimeMillisMetric>() << "ms)" << std::endl;
        return s;
    }

    template<typename Factorizer>
    void factorize_vbyte(Factorizer& factorizer, size_t const n, lz77::EmitFunction emit_literal, lz77::EmitFunction emit_copy) {
        if(window > 0) {
            iopp::FileInputStream in(filename);
            factorizer.factorize(in, n, window, emit_literal, emit_copy);
        } else {
            auto s = load_input(n);
            factorizer.factorize(s.begin(), s.end(), emit_literal, emit_copy);
        }
    }

    template<std::unsigned_integral Index>
    size_t compress(size_t const n) {
        alz::ApproximateLZ77<Index> alz(sampling, fp_window);

        iopp::FileOutputStream fout(output_filename);
        fout.write(MAGIC, MAGIC_LEN);

        Flags flags { flag_collapse_gaps, 0 };
        fout.put(*((char*)&flags));
        alz::internal::encode_vbyte(fout, n);

        size_t z = 0;
        if(flag_collapse_gaps) {
            std::string gap;
            auto flush_gap = [&](){
                alz::internal::encode_vbyte(fout, gap.length());
                if(!gap.empty()) {
                    fout.write(gap.data(), gap.length());
                }
                gap.clear();
            };
            auto emit_literal = [&](lz77::Factor f){
                gap.push_back(f.literal());
                ++z;
            };
            auto emit_copy = [&](lz77::Factor f){
                flush_gap();
                alz::internal::encode_vbyte(fout, f.len);
                alz::internal::encode_vbyte(fout, f.src);
                ++z;
            };
            factorize_vbyte(alz, n, emit_literal, emit_copy);
            flush_gap();
        } else {
            auto emit_literal = [&](lz77::Factor f){
                alz::internal::encode_vbyte(fout, 0);
                fout.put(f.literal());
                ++z;
            };
            auto emit_copy = [&](lz77::Factor f){
                alz::internal::encode_vbyte(fout, f.len);
                alz::internal::encode_vbyte(fout, f.src);
                ++z;
            };
            factorize_vbyte(alz, n, emit_literal, emit_copy);
        }
        return z;
    }

public:
    ALZ() : cmdline::Program("alz", "Compute and encode an approximate LZ77 factorization") {
        required_arg("file", filename, "The input file.");
        option('s', "sampling", sampling, "The sampling rate (2^value).");
        option('l', "len", fp_window, "The fingerprint window size.");
        option('p', "prefix", prefix, "Process only this prefix of the input file.");
        option('w', "window", window, "The input window size; leave at 0 to load entire input into RAM.");
        option('o', "out", output_filename, "The output filename.");
        option('d', "decompress", decompress, "Decompress the input file rather than compressing it.");
        option("collapse-gaps", flag_collapse_gaps, "Collapse gaps to length-string representation.");
    }

    virtual int main() override {
        if(decompress) {
            if(output_filename.empty()) {
                output_filename = filename + ".dec";
            }

            std::string s;
            {
                iopp::FileInputStream fin(filename);

                // check magic
                {
                    char magic[MAGIC_LEN];
                    fin.read(magic, MAGIC_LEN);
                    for(size_t i = 0; i < MAGIC_LEN; i++) {
                        if(magic[i] != MAGIC[i]) {
                            std::cerr << "wrong magic" << std::endl;
                            std::abort();
                        }
                    }
                }

                auto const flags_char = fin.get();
                Flags flags = *((Flags*)&flags_char);

                auto const n = alz::internal::decode_vbyte(fin);
                if(flags.collapse_gaps) {
                    while(s.length() < n) {
                        std::string gap;
                        auto const gap_len = alz::internal::decode_vbyte(fin);
                        if(gap_len > 0) {
                            gap.resize(gap_len);
                            fin.read(gap.data(), gap_len);
                            s.append(gap);
                        }

                        if(s.length() < n) {
                            auto const len = alz::internal::decode_vbyte(fin);
                            auto const src = s.length() - alz::internal::decode_vbyte(fin);
                            for(size_t i = 0; i < len; i++) {
                                s.push_back(s[src + i]);
                            }
                        }
                    }
                } else {
                    while(s.length() < n) {
                        auto const len = alz::internal::decode_vbyte(fin);
                        if(len == 0) {
                            s.push_back(fin.get());
                        } else {
                            auto const src = s.length() - alz::internal::decode_vbyte(fin);
                            for(size_t i = 0; i < len; i++) {
                                s.push_back(s[src + i]);
                            }
                        }
                    }
                }
            }

            iopp::FileOutputStream fout(output_filename);
            fout.write(s.data(), s.length());
        } else {
            pm::MemoryTimePhase t;

            size_t const n = std::min(std::filesystem::file_size(filename), prefix);

            if(window > n) {
                window = 0; // load whole file
            }

            if(output_filename.empty()) {
                output_filename = filename + ".alz";
            }

            size_t z = 0;
            {
                t.start();
                if(n <= MAX_SIZE_32BIT) {
                    z = compress<uint32_t>(n);
                } else {
                    z = compress<uint64_t>(n);
                }
                t.stop();
            }
            
            auto const nout = std::filesystem::file_size(output_filename);
            std::cout << "n=" << n << ", z=" << z << ", nout=" << nout << ", t=" << t.get_metric<pm::Stopwatch::ElapsedTimeMillisMetric>() << ", m=" << t.get_metric<pm::MallocCounter::MemoryPeakMetric>() << std::endl;
        }
        return 0;
    }
};

int main(int argc, char** argv) {
    return ALZ().run(argc, argv);
}