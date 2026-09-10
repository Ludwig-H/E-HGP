# 1 "fixture.cu"
#include "strict_include.hpp"
#define MHGP7_VENDOR_LOCAL 13
static_assert(mhgp7_vendor_included() + MHGP7_VENDOR_LOCAL + MHGP7_COMMAND_DEFINE == 31);
int main() { return mhgp7_vendor_included() + MHGP7_VENDOR_LOCAL + MHGP7_COMMAND_DEFINE - 31; }
