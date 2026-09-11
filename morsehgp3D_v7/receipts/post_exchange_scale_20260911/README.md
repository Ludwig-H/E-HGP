# Triplet mono : semis complets après échange

Trois vrais processus indépendants, sans baseline conservée dans le moteur :
uniformes u16, graine 3, WSPD s8, toute la tour K1..10, amont1/statique1.
Le créneau CPU a été coordonné explicitement avec ROOT. Aucun GCP, CUDA,
timeout algorithmique ou arrêt anticipé ; les trois sorties sont fermées.

| n | MEB après | MEB évitées H=T | Total pipeline (s) | FULL seul (s) | Pic RSS (Gio) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 3 947 627 | 237 557 | 141,366 | 58,977 | 2,039 |
| 16 000 | 8 278 207 | 501 258 | 318,968 | 134,329 | 4,223 |
| 32 000 | 17 199 233 | 1 045 620 | 694,459 | 292,341 | 8,687 |

Les MEB économisées valent environ 5,68 %, 5,71 % et 5,73 % du travail
statique antérieur. Les 35 champs de sortie, dont les digests d'entrée et de
payload, et les R/U/S par K sont strictement identiques aux reçus statiques
publiés. Q=intruder_queries ; H=T≤min(Q,U−S) ; M nouveau+H=M ancien ;
anchor_hits nouveau+H=U−S. Aucun faux anchor_hit n'est ajouté.

Pour 8k→16k puis 16k→32k, les MEB doublent par ×2,097 et ×2,078 ; les
supports testés par ×2,106 et ×2,084. Les exposants locaux log2 correspondants
des MEB sont 1,068 et 1,055. Les buffers retenus doublent par ×2,004 et ×2,003.
Ces observations concernent cette famille et ces tailles, sans garantie
universelle sous-quadratique. Le RSS est celui du processus complet : il ne
prouve aucun gain mémoire causé par cette optimisation.

## Lire et reconstruire les preuves

Exécuter `python3 -B verify.py`, puis `python3 -B -O verify.py`.
Le [lecteur](verify.py) vérifie les sources et commandes, les trois fermetures,
les 35 champs et digests publiés, les R/U/S par K, les comptes réellement payés,
les traces de progression et RSS, puis recalcule les ratios et exposants. Il réexerce
neuf fautes synthétiques du comparateur de format, sans exécuter le moteur.
Ces neuf fautes ne sont pas de nouveaux mutants C++ géométriques.
Les comparaisons physiques complètes des forêts relèvent des qualifications
bornées séparées, pas de ce triplet à grande échelle.

[storage_map.json](storage_map.json) reconstruit tous les chemins logiques
retenus à partir de contenus dédupliqués octet pour octet. Les anciens Markdown
sont archivés comme `.source`, sans faux liens vivants. [extract.py](extract.py)
peut reconstruire les fichiers dans un répertoire nouveau, sans rejouer les
scripts historiques. Le seul ELF est exclu ; son empreinte et sa provenance
CMake Release restent présentes. Aucun vendor n'est distribué.

Header `6763a877…`, ELF actif `ade5dbdd7b35fb1f66a8e9f7ea6f33c315458b1ce12897396686ce7782cd9512`,
entrée fermée `7ce9df73…`. Le build actif associé conserve 24 CTests, les sources
propres et les dépendances utilisées épinglées avant/après. Cela ne fournit pas
une chaîne native vendorizée ni une garantie de reconstruction bit-à-bit.

## Portée

Les références historiques utilisaient statique4 sur un hôte partagé : aucun
speedup temporel contre ces références n'est revendiqué. Les nouvelles durées
sont des observations mono-thread appariées en configuration, une exécution
par taille, sans intervalle de confiance. La proximité des exposants ne vaut
ni preuve de complétude WSPD universelle, ni qualification d'autres géométries,
de s10/s12, de 50k sous 1 s/100 ms, ou de dizaines de millions de points G4.
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`phase=exploration_v7_hors_registre`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Les témoins SAN/mutants de l'optimisation restent
des qualifications séparées ; aucun résultat GPU n'est transféré ici.
