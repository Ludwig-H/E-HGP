# Pages de 2 Mio (THP) pour les grands tampons : exploration G4, piste fermée

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudethp1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudethp1/receipt.json`). Source `new` : `c9c7373f0`. Variante `thp` : la même source, dont
`buffer_acquire` passait en pages de 2 Mio (archive `sha256 c011b694…b4c7`, code non commité :
`claudethp1/variante_thp_buffer.cpp.txt` et `claudethp1/thp_complet.patch`). Trames LiDAR réelles ng00, ng01 et ng02,
W48. Aucune mesure ne promeut un statut public.

## Objet

Sous Linux, la variante alignait sur 2 Mio tout `Buffer` d'au moins 2 Mio (`posix_memalign`) et conseillait
`MADV_HUGEPAGE` sur ses octets exacts. Le budget restait inchangé. Constat local avant la session (codespace, 8 cœurs,
W8, ng00 et ng01, quatre passes) : les fautes de page mineures tombaient d'environ 517 000 à 84 000–96 000. L'étage des
forêts perdait environ 17 %, et la destruction des grands tampons passait de 18–21 ms à 2 ms en sortie de
`generate_single`, et de 14–16 ms à 1,5 ms après l'assemblage. Les vidages étaient identiques, et la suite rapide
locale passait (917 sur 921, les quatre autres échecs étant connus en local). La variante avait sa porte unitaire et
trois mutants tués.

## Règle écrite dans le plan avant la session

Exactitude : bancs conformes, vidages identiques entre variantes et égaux aux empreintes. Si la moyenne géométrique des
6 rapports thp/new du mur à chaud (médiane des passes 2 à 10 d'un processus, K5 CPU feuilles 16 en `278523` et K5 GPU
feuilles 24 en `344059:400`, trois trames) est ≤ 0,95, l'implémentation est commitée, qualifiée et jugée dans une
session suivante. Sinon, la piste est notée et fermée.

## Résultat

Les trois bancs sont `conforme`, avec les vidages des empreintes (K5 et K10). Politique de la VM, relevée par
`gpu_ab` : THP `[madvise]`, défragmentation `[madvise]`, noyau 6.8.0-1070-gcp, 185 Go de mémoire dont 176 à 182 Go
disponibles.

| Mode | Trame | Mur à chaud new → thp (ms) | Rapport | `domain` à chaud | `forest` à chaud |
| --- | --- | --- | ---: | --- | --- |
| CPU, 16 | ng00 | 310,3 → 348,1 | 1,122 | 208,8 → 220,4 | 100,0 → 126,1 |
| CPU, 16 | ng01 | 266,2 → 309,3 | 1,162 | 169,9 → 190,6 | 93,3 → 118,2 |
| CPU, 16 | ng02 | 321,0 → 354,8 | 1,105 | 205,9 → 224,3 | 114,8 → 129,3 |
| GPU 400 ‰, 24 | ng00 | 288,0 → 290,5 | 1,009 | 162,7 → 163,2 | 125,2 → 126,5 |
| GPU 400 ‰, 24 | ng01 | 240,3 → 248,5 | 1,034 | 136,9 → 140,9 | 102,6 → 107,7 |
| GPU 400 ‰, 24 | ng02 | 280,9 → 271,4 | 0,966 | 161,4 → 156,0 | 117,7 → 114,1 |

**Moyenne géométrique : 1,064 (seuil 0,95). La piste est fermée** ; rien n'est commité dans le moteur.

Lecture (descriptive) :
- À froid, la variante perd partout : mur 1,03 à 1,14 fois celui de `new`. À K10, elle perd aussi (mur à chaud 1,007
  à 1,070), surtout sur l'étage `domain` (+32 à +94 ms).
- Le gain local ne se transpose pas : sur 48 fils, les pages de 2 Mio coûtent plus qu'elles ne rapportent. Deux causes
  plausibles, non mesurées ici. D'une part, une page de 2 Mio est remise à zéro en entier à la première écriture,
  même si le tampon n'en remplit qu'une partie : arènes, réservoirs et capacités bornées par excès. D'autre part, 48
  fils se disputent l'allocation des pages d'ordre 9, avec compactage direct sous la politique `madvise`.
- Les coûts de fautes de page et de restitution des grands tampons restent réels. Une piste différente s'y attaquerait
  sans pages de 2 Mio : réutiliser les blocs d'une passe à l'autre, en régime à chaud. Elle demanderait sa propre
  règle, jugée sur G4.

## Pièces

`claudethp1/` : `plan.json`, `launch.json`, `receipt.json`, `gpu_ab_report_ab_k5_16_cpu.json`,
`gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`, `variante_thp_buffer.cpp.txt` (le
`buffer.cpp` de la variante), `thp_complet.patch` (variante, porte unitaire, mutants et paragraphe d'architecture,
jamais commités). Aucune donnée ni coordonnée LiDAR. `SHA256SUMS` couvre tous les fichiers sauf lui-même.
