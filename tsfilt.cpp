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

#include "ts.h"

int g_pmtPid = -1;

std::set<int> g_dropPidSet;

void printDebug(const char *format, ...) {
#ifdef DEBUG
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
#endif
}

void printError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

void feedPAT(const TS::Packet &packet) {
  static TS::PSI psi;
  bool completed = psi.feed(packet);
  if (!completed)
    return;

  printDebug("pasre PAT\n");

  TS::PATSection section = psi.firstSection();
  for (;;) {
    auto iterator = section.iterator();
    while(iterator.hasNext()) {
      const auto entry = iterator.next();

      int programNumber = entry.programNumber();
      int pid = entry.pid();
      printDebug("PAT: prog:%d  pid:%d\n", programNumber, pid);
      if (programNumber != 0) {
        g_pmtPid = pid;
        break;
      }
    }

    if (section.isLastSection())
      break;
    section = section.nextSection();
  }
}

void feedPMT(const TS::Packet &packet) {
  static TS::PSI psi;
  bool completed = psi.feed(packet);
  if (!completed)
    return;

  printDebug("pasre PMT\n");

  bool hasVideo = false;
  bool hasAudio = false;

  g_dropPidSet.clear();

  TS::PMTSection section = psi.firstSection();
  for (;;) {
    auto iterator = section.iterator();
    while(iterator.hasNext()) {
      const auto entry = iterator.next();

      int streamType = entry.streamType();
      int pid = entry.elementaryPid();
      printDebug("PMT: streamType:%d  pid:%d\n", streamType, pid);

      switch (streamType) {
        case 2: // ISO/IEC 13818-2
          if (hasVideo) {
            g_dropPidSet.insert(pid);
          } else {
            hasVideo = true;
          }
          break;

        case 0xf: // ISO/IEC 13818-7
          if (hasAudio) {
            g_dropPidSet.insert(pid);
          } else {
            hasAudio = true;
          }
          break;

        default:
          g_dropPidSet.insert(pid);
          break;
      } 
    }

    if (section.isLastSection())
      break;
    section = section.nextSection();
  }
}

bool checkPacket(const TS::Packet &packet) {
  printDebug("SI:%d PID:%d hasAF:%d hasPL:%d ct:%d\n",
    packet.payloadUnitStartIndicator(),
    packet.pid(),
    packet.hasAdaptationField(),
    packet.hasPayload(),
    packet.continuityCounter());

  if (!packet.hasPayload())
    return false;

  const auto pid = packet.pid();

  if (pid == TS::PID::PAT) {
    feedPAT(packet);
  } else if (pid == g_pmtPid) {
    feedPMT(packet);
  } else if (g_dropPidSet.find(pid) != g_dropPidSet.end()) {
    return true;
  }

  return false;
}

void filterTS(FILE *fin, FILE *fout) {
  TS::Packet packet;

  for(;;) {
    for(;;) {
      int c = fgetc(fin);
      if (c == EOF)
        return;
      if (c == TS::Packet::SYNCBYTE)
        break;
    }

    fseek(fin, ftell(fin) - 1, SEEK_SET);

    for(;;) {
      const auto readPos = ftell(fin);
      if (!packet.read(fin))
        return;

      if (packet.syncByte() != TS::Packet::SYNCBYTE) {
        printError("missing sync-byte\n");
        fseek(fin, readPos, SEEK_SET);
        break;
      }

      bool drop = checkPacket(packet);
      if (drop) {
        printDebug("--> drop\n");
      } else {
        printDebug("--> keep\n");
        packet.write(fout);
      }
    }
  }
}


int main(int argc, char **argv) {

  const char *inPath = nullptr;
  const char *outPath = nullptr;

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
  FILE *fin = nullptr;
  FILE *fout = nullptr;

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

  return result;
}

