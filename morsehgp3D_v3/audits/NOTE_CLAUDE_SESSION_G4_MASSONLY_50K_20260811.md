# Note de Claude — session G4 mass-only : les premiers reçus 50 k de la source par cellules et du masque fast

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Sous-portée mesurée : `K=10`, `smax=11`, `cpu_reference_sur_g4`,
`mass_only_diagnostic`.

## Provenance de session

VM `ehgp-blackwell-spot-ai1a` (europe-west4-ai1a, g4-standard-48 SPOT, 48
vCPU, RTX PRO 6000 — GPU non utilisé : session mass-only conformément à
l'ordre « la première mesure G4 reste mass-only »). Génération
`2026-08-11T03:33:26.248-07:00`, `maxRunDuration=3600 s` certifié avant
démarrage, arrêt invité 50 min armé, **arrêt ciblé certifié TERMINATED** sur
cette génération, clés OS Login de session révoquées et détruites. Deux
échecs de démarrage documentés avant succès : STOCKOUT SPOT en
europe-west4-a (deux fois), et un premier armement invité refusé sur la
jumelle ai1a (systemd `degraded` au boot) — fail-closed correct du script,
la VM a été arrêtée et recertifiée avant le second essai. Binaires
construits sur la VM depuis le tar du worktree au niveau du commit
`84ba459` ; sorties brutes rapatriées avant fermeture.

## 1. Sonde de masse de la source par cellules, 50 000 points

`mhgp3v_cell_source_mass_probe --points 50000 --smax 11 --seed 20260810
--threads 48`, trois familles, pas 6 et 10, prune convexe d'axe publié.
Extraits décisifs (pas 6 ; R = tuples avant prune, R' = après) :

| famille | cellules | R_2 | R'_2 | R_3 | R'_3 | R_4 | R'_4 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| terrain | 2 377 892 | 8,4e9 | **2,9e9** | 1,1e12 | 9,7e10 | 3,3e14 | 2,4e12 |
| scanline_single_pass | 1 293 280 | 2,9e10 | **4,8e8** | 2,9e12 | 3,5e10 | 2,5e14 | 2,6e12 |
| scanline_overlap_multiecho | 846 768 | 6,8e9 | **5,3e8** | 1,2e12 | 1,5e10 | 1,9e14 | 3,3e11 |

Le prune d'axe élague environ 46,9--91,7 % des cellules. Les réductions
observées atteignent 136,3x sur R_4 terrain au pas 6 et 584,3x sur R_4
multiecho au pas 6. Les quatre ratios R_3 scanline sont 49,4x, 55,0x, 79,2x
et 83,3x; aucun reçu ne porte 750x. Chaque lane count-only se mesure entre
0,174 et 29,153 s sur 48 threads. Ces temps sont déjà à budgéter; ils ne
mesurent ni formation de tuples, ni certification, ni fold.

**Verdict mass-only** : aucune lane n'est admise. Aucun tuple n'a été formé
et aucun budget d'octets, de fill, de certification ou de consommation n'a
été mesuré. Après prune, q2 porte encore 4,65e8--2,86e9 tuples, q3
1,47e10--1,32e11 et q4 3,30e11--9,97e12 selon la famille et le pas. q2 est
seulement la lane la moins rouge. Toute route qui énumère ces masses est
incompatible avec `warm_e2e < 1 s`; le pas 10 ne change pas ce verdict.

## 2. Masque hybrid-fast à l'échelle (catalogues parallèles 48 threads)

`mhgp3v_prefix_mass_probe --mask hybrid-fast --max-order 3`, seed 20260810 :

| famille, n | générateurs | catalogue | requêtes k=1 | k=2 | k=3 |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain 2 400 | 166 064 | 60,9 s | 42 816 (26 %) | 345 (0,21 %) | 93 (0,06 %) |
| scanline 2 400 | 158 587 | 77,1 s | 101 732 (64 %) | 9 131 (5,8 %) | 2 489 (1,6 %) |
| terrain 6 250 | 444 570 | 675,4 s | 166 821 (37 %) | **1 421 (0,32 %)** | 283 (0,06 %) |

Aux ordres k >= 2, le fast principal multi-lot réduit fortement le masque
fallback, mais pas uniformément à une fraction de pour cent ni à quelques
dizaines de millisecondes : le scanline n=2400 conserve 5,8 % / 0,548 s à
k=2 et 1,6 % / 0,117 s à k=3; terrain n=6250 atteint 0,32 % / 0,153 s à
k=2. Le préflight `predicted == hits` est exact partout. Deux verrous restent
visibles dans ces mêmes chiffres :

1. **k=1** : 26--64 % de requêtes — les `q > k+1` des lots multiples au
   fallback (théorème 2 reçu en solo seulement). Ma recommandation : router
   k=1 par l'EMST device au contrat normalisé — `k=1 == single-linkage` est
   déjà prouvé et gravé par partitions, les d² tiennent en entiers < 2^35,
   un Borůvka device est exact tel quel — plutôt que d'étendre le
   théorème 2. Ton arbitrage est demandé.
2. **le catalogue lui-même** : 675 s à n=6250 sur 48 threads (récolte
   séquentielle comprise) — c'est le poste que la source par cellules doit
   remplacer ; aucune extrapolation 50 k de cette voie n'est demandée.

La course scanline n=6250 a été coupée par la fenêtre de session (55 min) ;
son absence est un trou de mesure, pas un refus.

## 3. Ce que cette session ne dit pas

Aucun kernel n'a tourné, aucun fold n'a été rejoué à 50 k, aucun statut
n'avance. Les nombres ci-dessus sont des autorités d'admission au sens des
notes : ils disent où le budget mourra si on l'ignore. Les prochaines
mesures G4 utiles, dans ton ordre : le prune au plan général et l'anisotropie
dans la même sonde (CPU 48 threads suffit), puis `BallActivation` +
tombstones H0, puis seulement le producteur CUDA.

GCP : session unique, cible certifiée TERMINATED, aucune autre VM du label
`project=e-hgp` active.
