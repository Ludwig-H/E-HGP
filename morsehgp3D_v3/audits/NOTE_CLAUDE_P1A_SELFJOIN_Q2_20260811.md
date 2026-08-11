# Note de livraison Claude — P1a v1 q2, contre-audit non reçu

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Sous-portée de la livraison : `mass_only_falsifier`.

Première tranche du jalon P1a (PROPOSITION §7, audit S10) :
`prototype/pair_selfjoin_probe.cpp` au commit
`8a39c53f41c1964b12d38b0129d7e8a0a5cc94e7`, SHA-256
`5f2b160e7ff58a6b017f8c9c351353686a8d7a61b6115ebca88e5894a432a688`.
C'est un falsificateur mass-only : aucune ancre formée, aucun census, aucune
`BallActivation`. La livraison compile et ses CTests passent, mais elle n'est
pas reçue : son différentiel dit « soundness » tout en comparant seulement
deux comptes compensables.

## Ce qui est implémenté

- **La machine de blocs** : décomposition récursive `L,L / L×R / R,R`,
  division des blocs croisés sur UN SEUL côté (le plus gros, règle
  déterministe), microtuiles aux feuilles. L'identité agrégée
  `pruned + microtile = C(n,2)` est vérifiée à chaque run et par CTest. Cette
  somme ne prouve pas à elle seule la multiplicité un : une omission et un
  doublon de même masse peuvent se compenser.
- **La lane q2** : témoin strictement intérieur ⟺ `(w−x)·(w−y) < 0` ; le SUP
  de cette forme sur un triple de boîtes est **séparable par axe et atteint
  sur les 8 combinaisons d'extrêmes** — 24 produits entiers, aucun
  intervalle approché, aucun flottant. Dix témoins pris dans des plages de
  feuilles disjointes des deux extrémités certifient `p ≥ 10` donc
  `p+q ≥ 12 = K+2` : bloc entier H0-inerte (théorème 4.2). Les feuilles
  recouvrant une extrémité sont testées point à point en excluant les
  positions d'extrémités.
- **Différentiel insuffisant** : `--verify-bruteforce` recompte le nombre de
  paires non inertes, puis vérifie seulement
  `non_inert <= microtile_pairs`. Une paire non inerte prunée peut être
  compensée par une paire inerte conservée. Les CTests sur `terrain` et
  `scanline_overlap_multiecho` rendent cette inégalité verte; ils ne ferment
  pas la soundness.
- Budget d'états à refus (`--max-states`, code 3) : la sonde refuse, elle ne
  tronque pas.

Le budget d'états est fail-closed, mais la recherche de témoins possède aussi
un `kFrontierCap=96` : les nœuds ambigus excédentaires sont écartés de la
recherche du bloc. Ce cap est conservateur pour l'exactitude, mais change les
masses; `frontier_truncations` n'est pas publié. Les CTests n'imposent aucun
plancher de prune : le mutant « tout en microtuiles » resterait vert.

## Mesures (1 thread, seed 20260810, feuilles ≤ 8, seuil 10)

| famille, n | états | prunées | microtuiles | part | temps chaud |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain 400 | 1 846 | 57 985 | 21 815 | 27,34 % | 0,009 s |
| terrain 2 400 | 24 186 | 2 733 814 | 144 986 | 5,04 % | 0,92 s |
| terrain 12 000 | 127 102 | 70 913 035 | 1 080 965 | 1,50 % | 35,7 s |
| scanline_single_pass 2 400 | — | 2 752 284 | 126 516 | 4,39 % | 1,50 s |
| uniform 2 400 | — | — | — | 14,15 % | 5,95 s |

La part de microtuiles décroît avec `n` seulement sur la série `terrain`
publiée; les autres familles n'ont qu'une taille dans ce tableau. À
`n=2400`, les parts déclarées vont de 4,39 % sur scanline à 14,15 % sur
uniforme. Ce sont des mesures de masse, pas une admission. À `n=400`, le
balayage compte 7 120 paires non inertes pour 21 815 microtuiles, mais cette
comparaison agrégée ne localise pas les paires prunées à tort.

## L'expérience d'héritage de frontière : essayée, mesurée, retirée

Le coût dominant observé est la recherche de témoins depuis la racine par
bloc, avec 35,7 s déclarées à 12 000 points sur un thread. Trois tailles et
des pentes variables ne justifient aucun exposant asymptotique. Claude a
implémenté
l'héritage d'antichaîne (le sup est monotone par restriction de bloc, un
nœud certifié négatif le reste pour les enfants) : la version à frontière
plafonnée s'effondre — **une troncature de frontière est IRRÉVERSIBLE dans
le sous-arbre** (le nœud perdu ne revient jamais), et le recouvrement d'une
extrémité n'est PAS monotone (un recouvrant redevient disjoint chez
l'enfant). Mesuré : 52 puis 82 % de microtuiles à 2400/12000, contre 5,0 et
1,5 % depuis la racine. Retiré du code, gravé en commentaire. La question
d'ingénierie ouverte est une antichaîne sans perte construite par la topologie
du self-join, avec compte durable séparé de la frontière ambiguë. La
parallélisation ne reçoit aucun facteur idéal : le code livré est mono-thread
et aucune mesure 48 threads ne permet d'annoncer une seconde.

## Ce que cette tranche ne fait pas

Les lanes q3/q4 ne sont pas implémentées. Elles ne peuvent pas être ajoutées
sur le résiduel q2 : dix témoins dans une boule diamétrale n'excluent pas une
activation q3/q4 décalée dans le plan médiateur. Il faut prouver séparément une
source sparse et complète des ancres supérieures. Pour q2, la prochaine porte
est un bitmap ou ledger de fate à petit `n` qui vérifie, pour chaque paire,
multiplicité un et inclusion de toutes les non-inertes dans les microtuiles,
avec mutants d'omission et de duplication.

GCP non utilisé.
