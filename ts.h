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

#ifndef TS_H_
#define TS_H_

#include <stdio.h>
#include <sys/types.h>

#include <iterator>
#include <vector>

#include "accessor.h"

namespace TS {

using namespace BitFieldAccessor;

//
// ISO/IEC 13818-1 Predefined PID values
// 
namespace PID {
  constexpr int PAT = 0x0000;
  constexpr int CAT = 0x0001;
  constexpr int TDT = 0x0002;
  constexpr int Null = 0x1fff;
};


//
// Payload part in a transport packet
//
struct Payload {
  const u_int8_t * const data;
  const size_t size;

  Payload(const u_int8_t *data_, size_t size_)
   : data(data_), size(size_) {}
};

//
// ISO/IEC 13818-1 Transport packet
//
class Packet {
public:
  static constexpr size_t SIZE = 188;
  static constexpr u_int8_t SYNCBYTE = 0x47;

  Packet() : data() {}

  // returns true if SIZE bytes were read in
  bool read(FILE *fin);

  void write(FILE *fout);

  int  syncByte()                   const { return INT<0,8>::get(data); }
  bool transportErrorIndicator()    const { return BIT<8>::get(data); }
  bool payloadUnitStartIndicator()  const { return BIT<9>::get(data); }
  bool transportPriority()          const { return BIT<10>::get(data); }
  int  pid()                        const { return INT<11,13>::get(data); }
  int  transportScramblingControl() const { return INT<24,2>::get(data); }
  bool hasAdaptationField()         const { return BIT<26>::get(data); }
  bool hasPayload()                 const { return BIT<27>::get(data); }
  int  continuityCounter()          const { return INT<28,4>::get(data); }

  // Adaptation field
  // These values are valid only if hasAdaptationField() returns true.
  int  adaptationFieldLength()      const { return INT<32,8>::get(data); }
  bool discontinuityIndicator()     const { return BIT<40>::get(data); }
  bool randomAccessIndicator()      const { return BIT<41>::get(data); }
  bool elementaryStreamPriorityIndicator() const { return BIT<42>::get(data); }
  bool pcrFlag()                    const { return BIT<43>::get(data); }
  bool opcrFlag()                   const { return BIT<44>::get(data); }
  bool splicingPointFlag()          const { return BIT<45>::get(data); }
  bool transportPrivateDataFlag()   const { return BIT<46>::get(data); }
  bool adaptationFieldExtensionFlag() const { return BIT<47>::get(data); }
  int64_t pcr() const {
    const u_int8_t *p = &data[6];
    return ((int64_t)p[0] << 40)
         | ((int64_t)p[1] << 32)
         | ((int64_t)p[2] << 24)
         | ((int64_t)p[3] << 16)
         | ((int64_t)p[4] << 8)
         | ((int64_t)p[5]);
  }
  int64_t opcr() const {
    const u_int8_t *p = &data[6 + pcrFlag() * 6];
    return ((int64_t)p[0] << 40)
         | ((int64_t)p[1] << 32)
         | ((int64_t)p[2] << 24)
         | ((int64_t)p[3] << 16)
         | ((int64_t)p[4] << 8)
         | ((int64_t)p[5]);
  }
  int spliceCountdown() const {
    const u_int8_t *p = &data[6 + (pcrFlag() + opcrFlag()) * 6];
    return *reinterpret_cast<const int8_t *>(p);
  }

  // Payload
  // The returned value is valid only if hasPayload() returns true.
  Payload payload() const {
    const int index = hasAdaptationField() ? (4 + 1 + adaptationFieldLength()) : 4;
    return Payload(&data[index], SIZE - index);
  }

private:
  u_int8_t data[SIZE];
};

// forward declaration
class PSISection;

//
// ISO/IEC 13818-1 Program Specific Information
// 
class PSI {
public:
  PSI();

  // returns true if all sections were read in
  bool feed(const Packet &packet);

  int pointerField() const { return data[0]; }

  PSISection firstSection() const;

private:
  std::vector<u_int8_t> data;
  int nextCounter;
};

//
// Section in PSI
//
class PSISection {
public:
  PSISection(const u_int8_t *data_, size_t size_)
    : data(data_), size(size_) { }

  PSISection(const PSISection &s)
    : data(s.data), size(s.size) { }

  bool isComplete() const {
    return canDetermineSectionSize() && size >= sectionSize();
  }

  bool isLastSection() const {
    return sectionNumber() == lastSectionNumber();
  }

  PSISection nextSection() const {
    const int sectSize = sectionSize();
    return PSISection(&data[sectSize], size - sectSize);
  }

  int  tableId()                const { return INT<0,8>::get(data); }
  bool sectionSyntaxIndicator() const { return BIT<8>::get(data); }
  int  sectionLength()          const { return INT<12,12>::get(data); }

  int  versionNumber()          const { return INT<42,5>::get(data); }
  bool currentNextIndicator()   const { return BIT<47>::get(data); }
  int  sectionNumber()          const { return INT<48,8>::get(data); }
  int  lastSectionNumber()      const { return INT<56,8>::get(data); }

  u_int32_t crc() const {
    const u_int8_t * const p = &data[sectionSize() - 4];
    return ((u_int32_t)p[0] << 24)
         | ((u_int32_t)p[1] << 16)
         | ((u_int32_t)p[2] << 8)
         | ((u_int32_t)p[3]);
  }

protected:
  const u_int8_t *data;
  size_t size;

  bool canDetermineSectionSize() const { return size >= 3; }
  int sectionSize() const { return 3 + sectionLength(); }
};


class PATSection : public PSISection {
public:
  class Entry {
  public:
    Entry(const u_int8_t *data_) : data(data_) { }
    Entry(const Entry &e) : data(e.data) { }

    int programNumber() const { return INT<0,16>::get(data); }
    int pid() const { return INT<19,13>::get(data); }
  private:
    const u_int8_t *data;
  };

  class Iterator {
  public:
    Iterator(const u_int8_t *ptr, size_t size) : nextPtr(ptr), endPtr(ptr + size) {}
    Iterator(const Iterator &it) : nextPtr(it.nextPtr), endPtr(it.endPtr) {}

    bool hasNext() const { return nextPtr < endPtr; }

    Entry next() {
      const u_int8_t * const p = nextPtr;
      nextPtr += 4;
      return Entry(p);
    }

  private:
    const u_int8_t *nextPtr;
    const u_int8_t *endPtr;
  };

  PATSection(const PSISection &s) : PSISection(s) {}

  Iterator iterator() {
    const int entryPos = 8;
    return Iterator(data + entryPos, sectionSize() - entryPos - 4);
  }
};


class PMTSection : public PSISection {
public:
  class Entry {
  public:
    Entry(const u_int8_t *data_) : data(data_) { }
    Entry(const Entry &e) : data(e.data) { }

    int streamType() const { return INT<0,8>::get(data); }
    int elementaryPid() const { return INT<11,13>::get(data); }
    int esInfoLength() const { return INT<28,12>::get(data); }
  private:
    const u_int8_t *data;
  };

  class Iterator {
  public:
    Iterator(const u_int8_t *ptr, size_t size) : nextPtr(ptr), endPtr(ptr + size) {}
    Iterator(const Iterator &it) : nextPtr(it.nextPtr), endPtr(it.endPtr) {}

    bool hasNext() const { return nextPtr < endPtr; }

    Entry next() {
      Entry ent(nextPtr);
      nextPtr += 5 + ent.esInfoLength();
      return ent;
    }

  private:
    const u_int8_t *nextPtr;
    const u_int8_t *endPtr;
  };

  PMTSection(const PSISection &s) : PSISection(s) {}

  int pcrPid() const { return INT<67,13>::get(data); }
  int programInfoLength() const { return INT<84,12>::get(data); }

  Iterator iterator() {
    const int entryPos = 12 + programInfoLength();
    return Iterator(data + entryPos, sectionSize() - entryPos - 4);
  }
};

} // namespace

#endif // TS_H_
