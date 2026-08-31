// MorseHGP3D v6 — compatibilite device. MHGP6_HD marque les primitives
// arithmetiques exactes (entiers larges, comparaisons de quotients, formes
// q2/q3/q4) compilables en code device. Macro VIDE hors nvcc : aucun effet
// sur le build CPU. Les kernels naissent en session G4, valides par egalite
// bit-exacte post-RLE contre le chemin CPU ; l'oracle n'est jamais porte.
#pragma once

#if defined(__CUDACC__)
#define MHGP6_HD __host__ __device__
#else
#define MHGP6_HD
#endif
