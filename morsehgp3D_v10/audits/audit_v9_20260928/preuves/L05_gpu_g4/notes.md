# L05 GPU/G4 - notes de travail (lecture seule, HEAD ce8a649dd)
R22 00/K5: GPU chain 0.926 wall 1.718 digest .192 catdigest .286 ctx 121 pinned 39; moteur chain 2.770 wall 3.325; cpu_s 17.3 vs 114.9
R22 00/K10: GPU chain 2.961 wall 6.029 digest .999 catdigest 1.347 ctx 164 pinned 190; moteur 8.405 / 10.939; RSS 6.2 vs 4.4 GB
chain vs wall 00/K5 GPU: R16 1.532/2.118 R17 1.395/2.021 R18 1.251/1.872 R19 1.175/1.780 R20 1.109/1.728 R21 .972/1.722 R22 .926/1.718
R13 CPU batch filter arm (only one on G4): chain 3.841 vs engine 3.266 (filter 1576 ms vs GPU 91)
s4b local W8 (indicatif, host twin serial warps): K5 s4a 39.3 s vs s4b 46.5 s; K10 117.9 vs 341.6
Nsight R2: kernels 207.9 ms/pass (cert 90.1, rect 33.9, pairs 28.9, lanes tasks 37.9, lanes prep 15.3), copies 5.2; no overlap
Certificate slabs: 65536 sites x 52 B = 3.4 MB/warp x 3008 warps = 10.25 GB per call (cudaMalloc per call)
Amas 8k/16k/32k: E 2.09/7.79/30.70 M (x3.72/x3.94), P 28/113/450 M
