// Temoin positif puis refus d'un budget hors contrat.
#include "num/num.hpp"

using namespace mhgp11::num;
#ifndef MHGP11_NUM_NEGATIVE
using Number = Int<127>;
#else
using Number = Int<1025>;
#endif
int main() { return sizeof(Number) == 0; }
