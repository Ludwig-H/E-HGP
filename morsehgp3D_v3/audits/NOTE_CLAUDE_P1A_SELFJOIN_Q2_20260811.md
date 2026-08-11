# Note de Claude — P1a v1 : la machine de blocs du self-join et la lane q2, ledger fermé

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=mass_only_falsifier`,
`public_status=not_claimed`.

Première tranche du jalon P1a (PROPOSITION §7, audit S10) :
`prototype/pair_selfjoin_probe.cpp`. C'est un falsificateur mass-only —
aucune ancre formée, aucun census, aucune BallActivation.

## Ce qui est implémenté

- **La machine de blocs** : décomposition récursive `L,L / L×R / R,R`,
  division des blocs croisés sur UN SEUL côté (le plus gros, règle
  déterministe), microtuiles aux feuilles. **L'identité du ledger est une
  porte** : `pruned + microtile = C(n,2)` exactement, vérifiée à chaque run
  (CTest permanent).
- **La lane q2** : témoin strictement intérieur ⟺ `(w−x)·(w−y) < 0` ; le SUP
  de cette forme sur un triple de boîtes est **séparable par axe et atteint
  sur les 8 combinaisons d'extrêmes** — 24 produits entiers, aucun
  intervalle approché, aucun flottant. Dix témoins pris dans des plages de
  feuilles disjointes des deux extrémités certifient `p ≥ 10` donc
  `p+q ≥ 12 = K+2` : bloc entier H0-inerte (théorème 4.2). Les feuilles
  recouvrant une extrémité sont testées point à point en excluant les
  positions d'extrémités.
- **Soundness gravée** : `--verify-bruteforce` recompte l'inertie exacte de
  chaque paire par balayage et vérifie qu'aucune paire non inerte n'a été
  prunée (CTest sur `terrain` et `scanline_overlap_multiecho`).
- Budget d'états à refus (`--max-states`, code 3) : la sonde refuse, elle ne
  tronque pas.

## Mesures (1 thread, seed 20260810, feuilles ≤ 8, seuil 10)

| famille, n | états | prunées | microtuiles | part | temps chaud |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain 400 | 1 846 | 57 985 | 21 815 | 27,34 % | 0,009 s |
| terrain 2 400 | 24 186 | 2 733 814 | 144 986 | 5,04 % | 0,92 s |
| terrain 12 000 | 127 102 | 70 913 035 | 1 080 965 | 1,50 % | 35,7 s |
| scanline_single_pass 2 400 | — | 2 752 284 | 126 516 | 4,39 % | 1,50 s |
| uniform 2 400 | — | — | — | 14,15 % | 5,95 s |

La PART de microtuiles décroît avec n sur les trois familles mesurées — la
parcimonie de la lane q2 tient sur ces campagnes. À n=400, la vérité par
balayage compte 7 120 paires non inertes pour 21 815 microtuiles : le
certificat par blocs est conservateur d'un facteur ~3 à cette taille, ce qui
laisse une marge de resserrement (feuilles plus fines, raffinement par paire
dans les microtuiles).

## L'expérience d'héritage de frontière : essayée, mesurée, retirée

Le coût dominant est la recherche de témoins DEPUIS LA RACINE par bloc
(~n^1,9 en visites ; 35,7 s à 12 000 sur un thread). J'ai implémenté
l'héritage d'antichaîne (le sup est monotone par restriction de bloc, un
nœud certifié négatif le reste pour les enfants) : la version à frontière
plafonnée s'effondre — **une troncature de frontière est IRRÉVERSIBLE dans
le sous-arbre** (le nœud perdu ne revient jamais), et le recouvrement d'une
extrémité n'est PAS monotone (un recouvrant redevient disjoint chez
l'enfant). Mesuré : 52 puis 82 % de microtuiles à 2400/12000, contre 5,0 et
1,5 % depuis la racine. Retiré du code, gravé en commentaire. La question
d'ingénierie ouverte pour vous : une frontière SANS PERTE (budget par nœud,
compression par ancêtre commun, ou ordonnancement dual-tree) — ou la
parallélisation seule (les blocs sont indépendants ; 48 threads G4 ramènent
la référence racine vers ~1 s à 12 000, à mesurer).

## Ce que cette tranche ne fait pas

Les lanes q3/q4 (patches du médiateur et de la borne de Jung, §7.3) ne sont
pas implémentées ; les compteurs `a`, `M`, `c_e`, `ΣZ_e` des ancres
diamètre (§8) non plus. La prochaine tranche les ajoute sur la même machine
de blocs, puis la mesure 50 k passe sur G4 (48 threads) avec le verdict
NO-GO de la §7.4 aux seuils que vous avez gravés.

GCP non utilisé.
