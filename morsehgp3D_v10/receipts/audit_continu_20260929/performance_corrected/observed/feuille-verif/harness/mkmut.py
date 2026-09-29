#!/usr/bin/env python3
"""Construit un harnais cathash sur une copie mutante de leaf.hpp (verificateur). usage : mkmut.py NOM"""
import os
import shutil
import subprocess
import sys

V = '/workspaces/E-HGP/build/v10-perf/feuille-verif'
M = {
    'M3_bas_strict': ('(t0 - mn(n0, yk0) >= 0)', '(t0 - mn(n0, yk0) > 0)'),
    'B4_A_large': ('(A > 0) & (B > 0)', '(A >= 0) & (B > 0)'),
    'B4_somme_large': ('(A + B + Gm < D2)', '(A + B + Gm <= D2)'),
    'Small_seuil_2p22': ('constexpr i64 kLim = i64(1) << 18;', 'constexpr i64 kLim = i64(1) << 22;'),
    'D_large': ('static_cast<u64>(base - 2 * cmin < 0)', 'static_cast<u64>(base - 2 * cmin <= 0)'),
    # mutants du verificateur
    'aigu_large': ('(g > 0) & (g < nj) & (g < NN[k])', '(g > 0) & (g <= nj) & (g < NN[k])'),
    'Small_sites_2p22': ('ok = ok && y < kLim && -y < kLim;', 'ok = ok && y < (kLim << 4) && -y < (kLim << 4);'),
    'Z_i64_partout': ('if (S.small) triples_impl<true>', 'if (true) triples_impl<true>'),
    'B4_i64_partout': ('if (S.small) quads_impl<true>', 'if (true) quads_impl<true>'),
    'M3_haut_strict': ('(t0 - mx(x0, yk0) < H0)', '(t0 - mx(x0, yk0) < H0 - 1)'),
}
name = sys.argv[1]
d = '/tmp/j3verif_mut/' + name
shutil.rmtree(d, ignore_errors=True)
shutil.copytree(V + '/patched/morsehgp3D_v10/src', d + '/src')
L = d + '/src/catalogue/leaf.hpp'
s = open(L).read()
old, new = M[name]
if s.count(old) != 1:
    sys.exit('motif absent %s %d' % (name, s.count(old)))
open(L, 'w').write(s.replace(old, new))
flags = ['-O3', '-DNDEBUG', '-std=c++20', '-I' + d + '/src']
subprocess.run(['g++'] + flags + ['-c', d + '/src/catalogue/generator.cpp', '-o', d + '/generator.cpp.o'], check=True)
objs = [os.path.join('/tmp/j3verif_mut/objs', o) for o in sorted(os.listdir('/tmp/j3verif_mut/objs'))
        if o.endswith('.o') and o != 'generator.cpp.o']
subprocess.run(['g++'] + flags + [V + '/harness/cathash.cpp', d + '/generator.cpp.o'] + objs + ['-lpthread', '-o',
               V + '/harness/cathash_mut_' + name], check=True)
print('mutant', name, 'ok')
