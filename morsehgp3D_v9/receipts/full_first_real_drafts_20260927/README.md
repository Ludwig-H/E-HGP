# Première occurrence sur vrais drafts — capture locale close

27 septembre 2026, base moteur `b9fcc3d63`. Autorité `r1/` ; première
qualification réussie, aucun essai en échec dans cette génération.
Voir la [méthode et les limites](../../audits/b_full_first_real_drafts_20260927/README.md).

Six commandes GCC Release / Clang ASan/UBSan/LSan, puis quatre processus
de mesure Release : tous clos, stderr vide. Les cinq lecteurs LIVE normal
et `-O` passent ; ils vérifient 117 pins sources/bibliothèque pour la
qualification, 139 avec l'entrée LiDAR et 118 pour chaque synthétique,
ainsi que l'inventaire complet des huit pins de builds.

Tous les tableaux des vingt ordres sont comparés aux reconstructions
natives et aux forêts effectivement publiées, trois paires par ordre.
Les petites portes exercent les deux formes d'entrée et le pass-through,
avec les trois digests identiques. Aucun moteur modifié, aucune donnée
KITTI recopiée, aucun GPU/GCP utilisé.

| capture | sites | somme médianes natif ms | somme médianes prototype ms | scratch demandé total, octets |
| --- | ---: | ---: | ---: | ---: |
| `r1/ng00` sans sol entier | 39 885 | 168,118 | 171,082 | 24 668 000 |
| `r1/uniform_8000` synthétique | 8 000 | 63,779 | 62,383 | 10 070 464 |
| `r1/uniform_16000` synthétique | 16 000 | 133,109 | 131,788 | 20 828 704 |
| `r1/uniform_32000` synthétique | 32 000 | 308,247 | 282,465 | 42 564 992 |

Les sommes de médianes isolées ne sont pas des murs de tour. Machine
partagée avec une campagne arène q34 W1/W4 : observations appariées
conservées, **pas de gain stable LiDAR acquis**. Allocations/écritures
comprises ; comparaison et destruction des résultats exclues. Aucune
répétition favorable n'a remplacé les essais d'origine.

Toutes les entrées ont zéro continuation : chemin linéaire effectivement
exercé. Les trois digests, tailles et capacités sont égaux à la capture
historique générale `full_real_drafts_20260927/r3`, mais les chronos entre
ces deux processus ne sont pas appariés. Le scratch demandé LiDAR est
24,668 Mo contre 98,672 Mo pour l'ancien prototype ; les sorties n'ont pas
été réduites. Ces sommes ne sont pas des pics de mémoire.

Chaînes instrumentées intégralement payées : LiDAR 28,313 s, synthétiques
6,212 / 12,857 / 27,979 s. Les copies des drafts perturbent la fin de chaîne.
Pas de contrat GPU/FULL ni d'extrapolation de croissance LiDAR, de s10/s12,
de K10 ou de plusieurs séquences dans ce lot.

Builds LIVE épinglés, ne pas reconstruire :
`/workspaces/E-HGP/build/v9-audit-full-first-real-drafts-20260927-r1/{release,sanitize}`.
Chaque `capture.json` conserve les inventaires et leurs hashes ; les
lecteurs dépendent encore des builds et entrées originales non versionnées.
