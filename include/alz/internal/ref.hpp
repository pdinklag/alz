/**
 * approximate_lz77.hpp
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

#ifndef _ALZ_REF_HPP
#define _ALZ_REF_HPP

#include <concepts>
#include <memory>
#include <vector>

namespace alz::internal {

template<std::unsigned_integral Index>
struct Ref {
    Index beg, src, len;
    Index end() const { return beg + len - 1; }

    class ListIterator {
    private:
        std::unique_ptr<std::vector<Ref>> const* refs_;
        Index num_threads_;

        Index th_, pos_;

        Ref const& get() const {
            return (*refs_[th_])[pos_];
        }

        void advance() {
            if(++pos_ >= refs_[th_]->size()) {
                pos_ = 0;

                ++th_;
                while(th_ < num_threads_ && refs_[th_]->empty()) {
                    ++th_;
                }
            }
        }

    public:
        ListIterator() : num_threads_(0), th_(0) {
        }

        ListIterator(std::unique_ptr<std::vector<Ref>> const* refs, Index const num_threads) : refs_(refs), num_threads_(num_threads), th_(0), pos_(0) {
            while(th_ < num_threads_ && refs_[th_]->empty()) {
                ++th_;
            }
        }

        ListIterator(ListIterator&&) = default;
        ListIterator& operator=(ListIterator&&) = default;

        ListIterator(ListIterator const&) = default;
        ListIterator& operator=(ListIterator const&) = default;

        operator bool() const { return th_ < num_threads_; }
        Ref const& operator*() { return get(); }
        Ref const* operator->() { return &get(); }

        ListIterator& operator++() {
            advance();
            return *this;
        }

        ListIterator operator++(int) {
            auto copy = *this;
            advance();
            return copy;
        }
    };
} __attribute__((packed));

}

#endif
