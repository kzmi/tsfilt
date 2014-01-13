#include <stdio.h>
#include <vector>
#include "accessor.h"

using namespace BitFieldAccessor;

int failCount = 0;

void assert_(const char *expr, bool cond) {
  const char *result = cond ? "PASS" : "FAIL";
  printf("%s ...... %s\n", expr, result);
  if (!cond)
    failCount++;
}

#define EQUALS(expr, expected) assert_(#expr, (expr) == (expected))

struct Bytes : std::vector<u_int8_t> {
  Bytes(std::initializer_list<u_int8_t> args) : std::vector<u_int8_t>(args) { }
  operator const u_int8_t *() { return data(); }
};
  

int main() {

  EQUALS(BYTEMASK(0,1), 0x80);
  EQUALS(BYTEMASK(0,2), 0xc0);
  EQUALS(BYTEMASK(1,2), 0x60);
  EQUALS(BYTEMASK(7,1), 0x01);
  EQUALS(BYTEMASK(6,2), 0x03);
  EQUALS(BYTEMASK(0,8), 0xff);
  EQUALS(BYTEMASK(8,2), 0x00);
  EQUALS(BYTEMASK(7,3), 0x01);

  EQUALS(BIT<0>::get(Bytes{0x80, 0x00, 0x00}), true);
  EQUALS(BIT<0>::get(Bytes{0x7f, 0xff, 0xff}), false);
  EQUALS(BIT<7>::get(Bytes{0x01, 0x00, 0x00}), true);
  EQUALS(BIT<7>::get(Bytes{0xfe, 0xff, 0xff}), false);
  EQUALS(BIT<8>::get(Bytes{0x00, 0x80, 0x00}), true);
  EQUALS(BIT<8>::get(Bytes{0xff, 0x7f, 0xff}), false);
  EQUALS(BIT<21>::get(Bytes{0x00, 0x00, 0x04}), true);
  EQUALS(BIT<21>::get(Bytes{0xff, 0xff, 0xfb}), false);

  EQUALS((INT<0,1>::get(Bytes{0xa0, 0x00, 0x00})), 0x01);
  EQUALS((INT<0,2>::get(Bytes{0xa0, 0x00, 0x00})), 0x02);
  EQUALS((INT<0,3>::get(Bytes{0xa0, 0x00, 0x00})), 0x05);
  EQUALS((INT<0,8>::get(Bytes{0x81, 0x00, 0x00})), 0x81);
  EQUALS((INT<1,1>::get(Bytes{0x50, 0x00, 0x00})), 0x01);
  EQUALS((INT<1,2>::get(Bytes{0x50, 0x00, 0x00})), 0x02);
  EQUALS((INT<1,3>::get(Bytes{0x50, 0x00, 0x00})), 0x05);
  EQUALS((INT<1,7>::get(Bytes{0x41, 0x00, 0x00})), 0x41);
  EQUALS((INT<9,1>::get(Bytes{0x00, 0x50, 0x00})), 0x01);
  EQUALS((INT<9,2>::get(Bytes{0x00, 0x50, 0x00})), 0x02);
  EQUALS((INT<9,3>::get(Bytes{0x00, 0x50, 0x00})), 0x05);
  EQUALS((INT<9,7>::get(Bytes{0x00, 0x41, 0x00})), 0x41);

  EQUALS((INT<0,9>::get(Bytes{0x81, 0x80, 0x00})), 0x103);
  EQUALS((INT<0,16>::get(Bytes{0x80, 0x01, 0x00})), 0x8001);
  //EQUALS((INT<0,17>::get(Bytes{0x80, 0x01, 0x00})), 0x8001); // should fail to compile
  EQUALS((INT<1,8>::get(Bytes{0x41, 0x80, 0x00})), 0x83);
  EQUALS((INT<1,15>::get(Bytes{0x40, 0x01, 0x00})), 0x4001);
  EQUALS((INT<7,2>::get(Bytes{0x01, 0x80, 0x00})), 0x03);
  EQUALS((INT<15,2>::get(Bytes{0x00, 0x01, 0x80})), 0x03);
  EQUALS((INT<15,3>::get(Bytes{0x00, 0x01, 0x80})), 0x06);
  //EQUALS((INT<7,10>::get(Bytes{0x01, 0xff, 0x80})), 0x3ff); // should fail to compile
  
  return failCount;
}


