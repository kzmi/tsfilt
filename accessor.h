/*
 * Copyright (c) 2014, Iwasa Kazmi
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ACCESSOR_H_
#define ACCESSOR_H_

#include <sys/types.h>

namespace BitFieldAccessor {

//----------------------------------
// Accessors
//----------------------------------

static constexpr int BYTEMASK(int start, int bits) {
  return ((0xff >> bits) ^ 0xff) >> start;
}

static constexpr int BYTEOFFSET(int bitpos) {
  return bitpos / 8;
}

static constexpr int BITOFFSET(int bitpos) {
  return bitpos % 8;
}

template<int bitpos>
  class BIT {
  public:
    static bool get(const u_int8_t *p) {
      return (p[BYTEOFFSET(bitpos)] & BYTEMASK(BITOFFSET(bitpos), 1)) != 0;
    }
  };

template<int bitpos, int bits, int bytes>
  class INT_ {
    // INT_::get(const u_int8_t *) is not implemented.
  };

template<int bitpos, int bits>
  class INT_<bitpos, bits, 1> {
    static constexpr int SHIFT = 8 - BITOFFSET(bitpos) - bits;
  public:
    static int get(const u_int8_t *p) {
      return (p[BYTEOFFSET(bitpos)] & BYTEMASK(BITOFFSET(bitpos), bits)) >> SHIFT;
    }
  };

template<int bitpos, int bits>
  class INT_<bitpos, bits, 2> {
    static constexpr int BITPOS1 = bitpos;
    static constexpr int BITS1 = 8 - BITOFFSET(bitpos);
    static constexpr int BITPOS2 = bitpos + BITS1;
    static constexpr int BITS2 = bits - BITS1;
  public:
    static int get(const u_int8_t *p) {
      return (INT_<BITPOS1, BITS1, 1>::get(p) << BITS2) | INT_<BITPOS2, BITS2, 1>::get(p);
    }
  };

template<int bitpos, int bits>
  class INT {
    static constexpr int BYTES = BYTEOFFSET(bitpos + bits - 1) - BYTEOFFSET(bitpos) + 1;
  public:
    static int get(const u_int8_t *p) {
      return INT_<bitpos, bits, BYTES>::get(p);
    }
  };

} // namespace

#endif // ACCESSOR_H_
