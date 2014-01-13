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

#include <stdio.h>

#include "ts.h"

namespace TS {

bool Packet::read(FILE *fin) {
  size_t len = fread(data, 1, SIZE, fin);
  return len == SIZE;
}

void Packet::write(FILE *fout) {
  fwrite(data, SIZE, 1, fout);
}

PSI::PSI()
  : data(),
    nextCounter(-1) {
}

bool PSI::feed(const Packet &packet) {
  if (!packet.hasPayload())
    return false;
  const bool start = packet.payloadUnitStartIndicator();
  const int counter = packet.continuityCounter();
  
  if (!start) {
    if (counter != nextCounter) {
      nextCounter = -1;
      return false;
    }
  } else {
    data.clear();
  }
  nextCounter = (counter + 1) % 16;

  Payload payload = packet.payload();
  data.insert(data.end(), payload.data, payload.data + payload.size);

  PSISection section = firstSection();
  for (;;) {
    if (!section.isComplete())
      return false;
    if (section.isLastSection())
      return true;
    section = section.nextSection();
  }
}

PSISection PSI::firstSection() const {
  const size_t dataSize = data.size();
  if (dataSize > 0) {
    const int pos = pointerField() + 1;
    if (pos < dataSize) {
      return PSISection(&data[pos], dataSize - pos);
    }
  }

  // return incomplete section
  return PSISection(&data[0], 0);
}

} // namespace
