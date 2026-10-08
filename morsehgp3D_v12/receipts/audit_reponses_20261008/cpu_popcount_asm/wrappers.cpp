#include "catalogue/simt.hpp"
extern "C" mhgp12::u32 audit_popc32(mhgp12::u32 x) { return mhgp12::simt::popc(x); }
extern "C" mhgp12::u32 audit_popc256(const mhgp12::simt::Bits<4>& x) { return mhgp12::simt::popc(x); }
