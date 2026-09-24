# Palette bornée de blocs pour le certificat de moments sur rectangles

Audit B, 24 septembre 2026. **Résultat négatif local**, sans modification du
moteur ni GCP. On garde exactement le quart physique sans sol de 08/000200
(11 461 sites, SHA-256 `825d005ea9c3e1b8509e26bd5adda746a80779f1e12e8a7e05c1b01bc9f7e7ae`),
K5/s8 et les 120 rectangles ouverts de masse `|A||B|≥1024` du
[premier shadow](../moments_rectangle_shadow_20260924/README.md). Cette
fois, au lieu d'un seul bloc spatial, on teste jusqu'à **128 nœuds BVH**
par rectangle, classés par distance de leur boîte au milieu des deux
boîtes du produit. Le bloc est fixe pour chaque certificat, contient de
3 à 64 sites distincts du même nuage (au moins 4 pour qu'une fermeture
q3 à K5 soit possible), et doit passer les 64 couples de
coins exacts. Les voies q3/q4 sont créditées séparément ; deux blocs
différents peuvent fermer les deux voies indépendamment.

| taille minimale du bloc | rectangles du front / ouverts / sélectionnés | masse sélectionnée | tests de certificats | fermetures q3 / q4 / complètes parmi les palettes 1, 8, 32 et 128 |
| ---: | ---: | ---: | ---: | ---: |
| 3 (seuil q4) | 520 210 / 245 818 / 120 | 897 149 | 15 360 | 0 / 0 / 0 à chaque largeur |
| 16 | 520 210 / 245 818 / 120 | 897 149 | 15 360 | 0 / 0 / 0 à chaque largeur |
| 32 | 520 210 / 245 818 / 120 | 897 149 | 15 360 | 0 / 0 / 0 à chaque largeur |

Les trois recherches lisent chacune 2 750 520 boîtes d'index pour le
classement. Durées exploratoires sur l'hôte partagé : 19,52 / 19,18 /
19,64 s pour **front + filtre + palette** ; ce ne sont ni des temps de
chaîne FULL, ni des comparaisons de performance G4. Le premier essai à
un seul bloc avait déjà trouvé zéro fermeture et seulement quelques
paires individuelles positives. La palette indique que **changer
simplement le bloc voisin ne sauve pas ce certificat uniforme de grosse
boîte sur cet échantillon**. Elle ne borne ni un choix de garde plus
intelligent, ni d'autres rectangles/scènes, ni la sélectivité par arête
après S2. Elle ne prouve pas une borne sous-quadratique.

Le code audit-only [`palette.cpp`](palette.cpp) réutilise l'arithmétique
entière de `shadow.cpp` SHA-256
`7a74a0946ae3dbc7089061c4abe25c4d8eb3495e330ddd73f717748ef398f1d3`.
SHA-256 de cette version de `palette.cpp` :
`5e40def341db4704a6c78be08956c29fe552f85e1613d000fa3dfc3fdb7edc2a`.
Le `mhgp9_gen` a été reconstruit en Release depuis le checkout
`605f39be4` dans un build neuf ; archive SHA-256
`e17932e499905ae2ae389b243b1fdf973303091719d55514379abce978f395b0`.
Exécutable local SHA-256
`1654c505ac3b615e490b365a55724792dde9346f8b32ba09866f7e514b141fd6`.
Les deux derniers fichiers sont des témoins locaux non versionnés, pas des
artefacts autonomes du dépôt. Compilation :

```sh
cmake -S morsehgp3D_v9 -B /tmp/mhgp9-audit-palette-build-20260924 \
  -DCMAKE_BUILD_TYPE=Release \
  -DMHGP9_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include
cmake --build /tmp/mhgp9-audit-palette-build-20260924 --target mhgp9_gen -j4
g++ -std=c++20 -O2 -Wall -Wextra -Werror -I morsehgp3D_v9/src \
  -I morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/b_moments_palette_20260924/palette.cpp \
  /tmp/mhgp9-audit-palette-build-20260924/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9-audit-palette-20260924
```

Rejouer avec le fichier d'entrée épinglé, `K=5`, `min_product=1024`, puis
`min_group_sites` omis, 16 et 32. Le programme refuse un autre domaine K,
les entrées mal formées, moins de 128 blocs admissibles par rectangle et
plus de 10 000 rectangles sélectionnés. L'entrée `K=4294967301`, qui
pouvait autrefois se replier sur 5 par rétrécissement entier, est
maintenant refusée avec code 1. Le [stdout figé](RUNS.txt) affiche le
ledger exact du front pour vérifier l'identité de l'expérience. Les 128
nœuds les plus proches peuvent inclure des ancêtres et descendants
recouvrants : ce ne sont pas 128 régions indépendantes.

## Première jointure par arête S2→S3, K10 sur 1 288 sites

Le [sidecar S2→S3](../b_s2_trace_20260924/README.md) a rendu possible une
vraie jointure ordinale sur **un quartier distinct**, sans relancer le
générateur complet. [`moment_trace.cpp`](moment_trace.cpp) choisit pour
chaque arête survivante un nœud BVH de ≤64 sites proche du milieu, applique
le certificat ponctuel entier aux seules voies ouvertes et publie
`(s2_ordinal, proved_mask)`. Les variantes palette 8/32 essaient les
8/32 nœuds admissibles les plus proches, chacun **indépendamment** ; leurs
preuves de voies peuvent s'unir, jamais leurs nombres de sites. Chaque
variante a été jointe par le lecteur du sidecar, qui refuse les bits non
demandés. Le source ne fait pas partie du moteur et le coût de sa sélection
n'est pas un coût produit/GPU.

| Palette par arête | Certificats testés | Arêtes entièrement fermées | Masse `F` éligible à éviter avant le cœur | q3 / q4 prouvés |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 55 657 | 1 | 66 / 1 151 766 | 1 / 0 |
| 8 | 445 249 | 1 | 66 / 1 151 766 | 1 / 0 |
| 32 | 1 780 993 | 1 | 66 / 1 151 766 | 1 / 0 |

La seule arête fermée est l'ordinal 35 017 (rectangle 35 040, sites locaux
571 et 14, retours bruts 11 808 et 2 435), voie q3 seule, `F=66` ; le
prover du cœur publié la fermait déjà. Le potentiel identifié par ce
**shadow avant cœur** représente seulement **0,0057 %** de `ΣF` sur ce
quartier K10. Un autre choix de garde ou le régime K5 peut être différent ;
ce résultat ne condamne pas la famille des certificats par moments, mais
**écarte ces palettes naïves** sur ce cas. Aucune économie de temps ni
sortie FULL n'a été mesurée, et ces trois essais ne disent rien d'une
croissance 8k/16k/32k ou d'une trame entière.

Le source `moment_trace.cpp` SHA-256
`3c658d4dea7bfe9567c22b201728d7caafc46368c4166f8b6dba5ce33292a9a5`
et l'exécutable local SHA-256
`9f2e104ea872bfc21099f91a6a40a0ca633746c7e5662de7b800bc238c261fc6`
utilisent l'archive `mhgp9_gen` mentionnée plus haut. Le fichier
[`quarter_1288.moment1.tsv`](quarter_1288.moment1.tsv) (SHA-256
`212e6746f62badf4e6c1ffd7a4a4f02813dde52390c42962a92c9eb3a7d6a0d0`)
conserve les 55 657 décisions de la palette 1 ; les palettes 8/32 ont
produit **le même SHA-256** et leurs doublons n'ont pas été conservés.
Les [sorties courtes](MOMENT_TRACE_RUNS.txt) et la commande de reproduction
sont publiées ; les trois jointures rendent
`status=eligibility_only_not_measured_savings`.
