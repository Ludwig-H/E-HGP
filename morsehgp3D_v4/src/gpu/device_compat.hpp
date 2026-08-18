// MorseHGP3D v4 — compatibilite device (plan GPU :
// audits/NOTE_CLAUDE_PLAN_GPU_20260817.md). MHGP4_HD marque les
// primitives arithmetiques exactes (U192/U320, comparaisons de
// quotients, cœur de seed) comme compilables en code device : macro
// VIDE hors nvcc — strictement aucun effet sur le build CPU, verifie
// par les 109 portes existantes. Les kernels eux-memes ne sont PAS
// ecrits ici : sans nvcc local ils seraient invérifiables — ils
// naitront lors d'une session G4, valides par egalite bit-exacte
// post-RLE contre le chemin CPU et par le selftest arithmetique en
// device (temoins __int128), conformement au plan.
#pragma once

#if defined(__CUDACC__)
#define MHGP4_HD __host__ __device__
#else
#define MHGP4_HD
#endif
