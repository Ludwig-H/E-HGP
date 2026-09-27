# Capture des bandes q3/q4 — 27 septembre 2026

**Autorité : `r2/capture.json`, statut `completed`, 23 commandes.**
Code et preuve : [prototype](../../audits/b_q34_bands_20260927/README.md).
Cadre : représentation CPU d'audit, grille entière 1 mm, hors registre,
`not_claimed`. Aucun moteur modifié ; GCP non utilisé.

La capture à la racine est **failed** et conservée : 11 commandes, arrêt
de `gate_sanitize` car LeakSanitizer refuse ptrace dans la sandbox. Le gate
Release et les deux mutations Release avaient réussi. Aucune erreur
géométrique ni mémoire du prototype n'est déduite de cet arrêt d'outil.
R2 reprend exactement les mêmes sources dans de nouveaux builds, hors
sandbox, avec `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` et
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

## Correction de la représentation

Release et Clang ASan/UBSan/LSan passent, compteurs identiques :

- 18 928 cas, dont 1 440 combinaisons K1..10/masques/classes fabriquées ;
- 4 457 784 paires et 1 439 204 cellules comparées ;
- 96 fronts natifs, 17 488 vrais rectangles, huit refus ;
- deux mutations tuées dans **chaque** build : bande dupliquée (cause
  `bands_overlap_duplicate`) et masque 6 abusif (`bands_mask_differs`).

Les mutations sont des variantes natives explicites exécutées en code 1,
pas des crashes ni des erreurs du sanitizer. Les portes portent sur la
couverture et les masques des crédits déjà fournis par le Pool, pas sur une
nouvelle preuve géométrique indépendante du Pool.

Les sources complètes du moteur, les sources du prototype et de l'ancien
plan réutilisé, les archives générateur, les entrées, les commandes et leurs
sorties sont hachés avant/après capture. Les binaires ne sont pas reconstruits
par le lecteur. Le lecteur vérifie la recette et les identités d'entrée ;
chaque mesure doit retrouver **exactement** P, E, E3, E4, F et le nombre
d'anciennes cellules de la capture Pool publiée.

## Mesures closes

Une observation locale par cas, CPU mono partagé ; pas de mesure de gain
net ni de chronométrage GPU. Tous les tests sont K5/s8/min-factor2. Les neuf
synthétiques reprennent seed3, 8k/16k/32k. LiDAR est la **trame entière**
08/000000 sans sol, 39 885 sites, grille 1 mm ; les sept morceaux existants
et leurs IDs sont contrôlés mais seul le morceau entier est chronométré ici.

| entrée | anciennes cellules | bandes | capacité cellules → bandes | ajout CPU bandes |
| --- | ---: | ---: | ---: | ---: |
| uniforme 8k | 463 | 273 | 20 480 → 3 408 octets | 0,131 ms |
| uniforme 16k | 1 064 | 664 | 45 440 → 8 196 octets | 0,337 ms |
| uniforme 32k | 2 018 | 1 279 | 86 320 → 15 900 octets | 0,710 ms |
| terrain 8k | 672 | 386 | 30 280 → 4 920 octets | 0,139 ms |
| terrain 16k | 972 | 577 | 42 840 → 7 224 octets | 0,205 ms |
| terrain 32k | 1 228 | 751 | 54 160 → 9 480 octets | 0,351 ms |
| amas 8k | 2 427 | 1 208 | 140 760 → 21 552 octets | 0,250 ms |
| amas 16k | 3 124 | 1 585 | 163 720 → 25 524 octets | 0,537 ms |
| amas 32k | 3 986 | 2 146 | 193 280 → 31 380 octets | 1,036 ms |
| sans-sol 08/000000 | 659 165 | 331 733 | 34,810 → 4,913 Mo | 49,377 ms |

Les capacités sont celles des **seuls tableaux de descripteurs**, sommées
sur tous les plans préparés ; pas la RAM/VRAM de pic ni toute la mémoire.
Sur la trame LiDAR, les anciens plans complets représentent encore
125,200 Mo de capacités cumulées et les objets bandes ajoutent 20,697 Mo,
dont 4,913 Mo de tableaux. Les facteurs restent possédés par l'ancien plan.
L'absence de copies supplémentaires de rangs/crédits ne rend pas les
originaux gratuits. Cette sonde conserve effectivement les deux objets.

Sur cette trame, les descripteurs passent de 659 165 à 331 733 (−49,7 %) et
leur capacité de 34,810 à 4,913 Mo (−85,9 %). Le prix local est réellement
mesuré : **49,4 ms ajoutées** pour les bandes, validation des 1 820 907 rangs
incluse ; les anciens plans coûtent 640,6 ms et la vérification différentielle
46,5 ms. Le front/filtre CPU porte la durée totale à 18,294 s. Ces durées ne
doivent pas être rapprochées directement du GPU G4 ni présentées comme un
gain S2 : les vieux blocs ne sont pas encore remplacés et le stockage
collectif GPU n'est pas implémenté.

## Croissance : ce qui ne change pas

E est **strictement identique** au Pool précédent, sur chaque mesure. Les
résidus uniformes restent 435 709 / 908 050 / 1 876 820 ; terrains
140 079 / 285 967 / 591 278. Sur amas, E reste 2 091 410 / 7 787 691 /
30 699 080, soit ×3,724 puis ×3,942 au doublement. Le problème de croissance
quasi quadratique de ce régime n'est donc **pas corrigé par les bandes**.
LiDAR conserve P=23 686 751 et E=9 122 704, sans nouvelle pente spatiale
mesurée dans cette capture. La construction des bandes n'explore pas E.

Suite utile : remplacer réellement les anciennes cellules dans une arène
collective et mesurer le coût total, restauration d'ordre incluse ; réduire
en parallèle le travail géométrique résiduel et l'aval FULL. Ni FULL, ni
GPU, ni contrat 1 s/100 ms ne sont qualifiés par ce reçu.

## Builds et lecture

Les deux paires de builds sont désormais épinglées et ne doivent plus être
modifiées :

- `build/v9-audit-q34-bands-20260927_{release,sanitize}` : tentative initiale ;
- `build/v9-audit-q34-bands-r2-20260927_{release,sanitize}` : autorité r2.

Les archives générateur du 26 septembre sont liées en lecture seule, pas
reconstruites. Les lecteurs r2 sont LIVE et requièrent les chemins absolus
de la capture ; aucun octet KITTI n'est versionné ici. Les sorties des
lectures normal/−O et huit mutations du lecteur sont conservées sous
`r2/checks/` après exécution de `check_capture.py`.
