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
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>

#include <set>

constexpr int TS_PACKET_SIZE = 188;
constexpr int PID_PAT = 0;

int g_pmtPid = -1;

std::set<int> dropPidSet;

void printDebug(const char *format, ...) {
/*
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
*/
}

void printError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}


class PSI {
public:
  PSI();
  const u_int8_t *header() { return &data[headerPos]; }
  const u_int8_t *syntaxSection() { return &data[syntaxSectionPos]; }
  const u_int8_t *tableData() { return &data[tableDataPos]; }
  int tableDataSize() { return tailPos - tableDataPos - 4; }

  bool feed(bool startInd, int contCounter, const u_int8_t *payload, int payloadSize);

private:
  u_int8_t data[1024];
  int nextCounter;
  int dataSize;
  int headerPos;
  int syntaxSectionPos;
  int tableDataPos;
  int tailPos;
};

PSI::PSI()
 : data(),
   nextCounter(-1),
   dataSize(0),
   headerPos(0),
   syntaxSectionPos(0),
   tableDataPos(0),
   tailPos(0)
{}

bool PSI::feed(bool startInd, int contCounter, const u_int8_t *payload, int payloadSize) {
  if (!startInd) {
    if (contCounter != nextCounter) {
      nextCounter = -1;
      return false;
    }
    nextCounter = (nextCounter + 1) % 16;
  } else {
    dataSize = 0;
    nextCounter = (contCounter + 1) % 16;
  }

  printDebug("feed\n");

  memcpy(&data[dataSize], payload, payloadSize);
  dataSize += payloadSize;

  if (startInd) {
    headerPos = data[0] + 1;
    syntaxSectionPos = headerPos + 3;
    tableDataPos = syntaxSectionPos + 5;
    const u_int8_t *hdr = header();
    int sectionLen = ((hdr[1] & 0xf) << 8) | hdr[2];
    tailPos = syntaxSectionPos + sectionLen;
  }

  return (tailPos <= dataSize);
}

void feedPAT(bool startInd, int contCounter, const u_int8_t *payload, int payloadSize) {
  static PSI psi;
  bool completed = psi.feed(startInd, contCounter, payload, payloadSize);
  if (!completed)
    return;

  const u_int8_t *p = psi.tableData();
  const u_int8_t *tail = p + psi.tableDataSize();
  while (p < tail) {
    int programNumber = (p[0] << 8) | p[1];
    int pid = ((p[2] & 0x1f) << 8) | p[3];
    printDebug("PAT: prog:%d  pid:%d\n", programNumber, pid);
    if (programNumber != 0) {
      g_pmtPid = pid;
      break;
    }
    p += 4;
  }
}

void feedPMT(bool startInd, int contCounter, const u_int8_t *payload, int payloadSize) {
  static PSI psi;
  bool completed = psi.feed(startInd, contCounter, payload, payloadSize);
  if (!completed)
    return;

  const u_int8_t *p = psi.tableData();
  const u_int8_t *tail = p + psi.tableDataSize();

  int programInfoLen = ((p[2] & 0xf) << 8) | p[3];
  p += programInfoLen + 4;

  bool hasVideo = false;
  bool hasAudio = false;

  dropPidSet.clear();

  while (p < tail) {
    int streamType = p[0];
    int pid = ((p[1] & 0x1f) << 8) | p[2];
    printDebug("PMT: streamType:%d  pid:%d\n", streamType, pid);
    int infoLen = ((p[3] & 0xf) << 8) | p[4];
    p += infoLen + 5;

    switch (streamType) {
      case 2: // ISO/IEC 13818-2
        if (hasVideo) {
          dropPidSet.insert(pid);
        } else {
          hasVideo = true;
        }
        break;

      case 0xf: // ISO/IEC 13818-7
        if (hasAudio) {
          dropPidSet.insert(pid);
        } else {
          hasAudio = true;
        }
        break;

      default:
        dropPidSet.insert(pid);
        break;
    }
  }
}

bool checkPacket(const u_int8_t *packet, int packetSize) {
  bool startInd = ((packet[1] & 0x40) != 0);
  int pid = ((packet[1] & 0x1f) << 8) | packet[2];
  bool hasAdaptationField = ((packet[3] & 0x20) != 0);
  bool hasPayload = ((packet[3] & 0x10) != 0);
  int contCounter = packet[3] & 0xf;

  printDebug("SI:%d PID:%d hasAF:%d hasPL:%d ct:%d\n",
    startInd, pid, hasAdaptationField, hasPayload, contCounter);

  if (!hasPayload)
    return false;

  int payloadPos = 4;
  if (hasAdaptationField)
    payloadPos += packet[4] + 1;

  if (pid == PID_PAT) {
    feedPAT(startInd, contCounter, &packet[payloadPos], packetSize - payloadPos);
  } else if (pid == g_pmtPid) {
    feedPMT(startInd, contCounter, &packet[payloadPos], packetSize - payloadPos);
  } else if (dropPidSet.find(pid) != dropPidSet.end()) {
    return true;
  }

  return false;
}

void filterTS(FILE *fin, FILE *fout) {
  constexpr u_int8_t SYNC_BYTE = 0x47;

  u_int8_t packet[TS_PACKET_SIZE];

  for(;;) {
    for(;;) {
      int c = fgetc(fin);
      if (c == EOF)
        return;
      if (c == SYNC_BYTE)
        break;
    }

    fseek(fin, ftell(fin) - 1, SEEK_SET);

    for(;;) {
      long readPos = ftell(fin);
      size_t len = fread(packet, 1, TS_PACKET_SIZE, fin);
      if (len != TS_PACKET_SIZE)
        return;

      if (packet[0] != SYNC_BYTE) {
        printError("missing sync-byte\n");
        fseek(fin, readPos, SEEK_SET);
        break;
      }

      bool drop = checkPacket(packet, TS_PACKET_SIZE);
      if (!drop) {
        fwrite(packet, TS_PACKET_SIZE, 1, fout);
      }
    }
  }
}


int main(int argc, char **argv) {

  const char *inPath = NULL;
  const char *outPath = NULL;

  for (int i = 1; i < argc; ++i) {
    if (!inPath) {
      inPath = argv[i];
      continue;
    }
    if (!outPath) {
      outPath = argv[i];
      continue;
    }
  }

  int result = 0;
  FILE *fin = NULL;
  FILE *fout = NULL;

  fin = inPath ? fopen(inPath, "rb") : stdin;
  if (!fin) {
    printError("cannot open : %s\n", inPath);
    result = 1;
    goto FINISH;
  }

  fout = outPath ? fopen(outPath, "wb") : stdout;
  if (!fout) {
    printError("cannot open : %s\n", outPath);
    result = 1;
    goto FINISH;
  }

  filterTS(fin, fout);

  fflush(fout);

FINISH:
  if (fin && fin != stdin)
    fclose(fin);

  if (fout && fout != stdout)
    fclose(fout);

  return 0;
}

