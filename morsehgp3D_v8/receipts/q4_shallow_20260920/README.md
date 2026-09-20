# q4 : couches duales et balayages des seuls sites retenus — tranche29

20 septembre2026, aprèsc051bdb0. `cpu_reference`, u16,
`public_status=not_claimed`. Une arête fournie, **q4 seulement**, pas une
tour FULL ou un générateur global. GCP non utilisé.
[Preuve, API et limites](../../docs/Q4_COUCHES_DUALES_20260920.md).

## Ce qui change

Une sélection commune à toutes les faces d'une arête retire les sites
qui ne peuvent toucher aucune boule q4 de profondeur admissible.
Les couches convexes sont calculées exactement sur deux groupes duaux,
en conservant tous les contacts, IDs coïncidents et constantes nulles.
Les sites retirés ne sont plus ni des témoins ni des seeds.
Le compte réduit repart de zéro ; faible profondeur réduite certifie
la profondeur exacte et la coquille complète. Ce n'est pas un crédit
à ajouter au census. Positivité et propriété certifient ensuite le nuage.

L'entrée `run_q4_shallow_edge_candidates` est explicite. Les autres voies
et défauts restent inchangés ; pas d'empilement avec le pool26 ou l'atlas28.
Géométrie/ensemble immuables partagés, buffers privés réutilisés par arête.

## Qualification propre

Toutes les captures suivantes sont closes PASS, sources184 avant/après :

| Capture | Commandes | Contenu |
| --- | ---: | --- |
| [smoke_254ekmp2](smoke_254ekmp2/COMPLETION.json) | 18 | 8 gates Release,10 mesures32 sites |
| [smoke_us_t5o8a](smoke_us_t5o8a/COMPLETION.json) | 18 | 8 gates Clang ASan/UBSan,10 mesures32 sites |
| [scale__b3aquyb](scale__b3aquyb/COMPLETION.json) | 40 | 8 gates Release,32 mesures dont24 à8k/16k/32k |
| [regression_rnihy_44](regression_rnihy_44/COMPLETION.json) | 1 | 90 CTests Release, aucun ignoré |

Soit52 mesures principales. La nouvelle gate passe3686 contrôles :
23 sélections contre un oracle rationnel de droites d'appui (pas les
chaînes monotones du produit),1942 centres contrôlés,2085 occurrences
de site retiré non positif et12501 tests de témoins stricts.
37 appels par arête et135 par seed sont comparés à la référence et
aux complétions rationnelles indépendantes :1894 complétions,
7543 tests de sites,85 candidats q4. Coquille30, contacts retirés,
couches dégénérées, colinéaires non sommets, duaux coïncidents des deux
signes, extrêmes u16, permutations et les dix arêtes d'une fixture.
Quatre allocations fautives ciblées, callback levant, invalides et quatre
appels concurrents complètent les tests. Ce n'est ni une injection
exhaustive ni une nouvelle qualification TSan ou d'un ordonnanceur q4 ;
les90 CTests ne sont pas tous exécutés sous sanitizer.

Trois [mutants compilés](mutants/README_MUTANTS.md) sont tués par le
désaccord géométrique de l'ensemble retenu, pas par un compteur :
enlever les colinéaires de frontière, omettre une couche, omettre c=0.
Vingt [différentiels contre le binaire28](mutants/README_DIFFERENTIAL.md)
conservent tout le JSON hors temps de l'ancienne voie locale.
Le lecteur principal refuse41 corruptions de reçus ; lectures normales
et `-O`, commandes et hashes de fermeture sont vérifiés séparément.
La [fermeture des dix lectures](READBACK.md) conserve leurs résultats
identiques,184 sources,87 entrées et74 artefacts avant/après.

Builds désormais épinglés : `build/v8_q4_shallow_20260920` et
`build/v8_q4_shallow_sanitize_20260920`. Ne pas les réutiliser pour la suite.
Les deux JSON sous `preflight/` sont des sorties exploratoires antérieures
au gel, conservées comme telles, pas l'autorité des mesures ci-dessous.

## Mesures appariées : sélection, véritables balayages et sorties

Un thread, campagne scale sur CPU0 ; un essai par configuration sur
l'hôte partagé, donc **pas de gain de temps stable revendiqué**.
La référence28 est réellement exécutée en q4 seul, domaine positif,
profondeur7,4096 nœuds, Z512, feuille32, clipping actif. Le nouveau run
comprend géométrie, sélection, génération des seeds, balayages et callback.
Les préparations communes nuage/index/cover sont séparées ; les deux
sommes préparation+run les partagent, ce ne sont pas deux temps muraux
indépendants. La validation et la libération sont comptées à part.

Chaque ligne compare physiquement tous les enregistrements normalisés
(support, clé, profondeur, coquille), pas seulement un digest. Le juge
refait un census rationnel global par boule publiée distincte : validité
des émissions, **pas preuve autonome de complétude sur le grand dense**.
La complétude repose sur les petits oracles et le certificat séparé.

K10, nuages denses, sorties identiques entre moteurs :

| Régime | n | Sites retenus | Lectures29 / lectures28 | Tri29 / tri28 | Run29 / run28, ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Préfixe | 8 000 | 867 | 749 955 /5 310 748 | 8 655 544 /10 271 563 | 230,188 /457,538 |
| Préfixe | 16 000 | 1 024 | 1 046 528 /13 798 714 | 12 422 702 /20 005 755 | 331,304 /971,820 |
| Préfixe | 32 000 | 1 242 | 1 540 080 /15 791 402 | 18 456 967 /21 069 568 | 496,755 /1108,951 |
| Permuté | 8 000 | 408 | 165 648 /912 689 | 1 675 241 /893 368 | 53,235 /64,416 |
| Permuté | 16 000 | 708 | 499 848 /3 579 207 | 5 445 436 /4 077 292 | 167,345 /260,515 |
| Permuté | 32 000 | 1 256 | 1 575 024 /14 973 733 | 18 920 906 /19 803 576 | 562,050 /1105,682 |

Lectures29 préfixe : ×1,395/×1,472 ; permuté : ×3,018/×3,151.
Tris29 préfixe : ×1,435/×1,486 ; permuté : ×3,251/×3,475.
**À8k permuté, le nouveau trie plus que28**, malgré moins de lectures ;
à32k le tri baisse beaucoup moins que les lectures. Les événements
profonds entre survivants sont toujours construits avant rejet.
Sorties K10 :25/25/25 au préfixe et36/37/30 à la permutation.

K5 : W préfixe96720/127448/199808 (×1,318/×1,568), permuté
25599/65535/192720 (×2,560/×2,941). Tris permutés219127/585768/1871245
(×2,673/×3,195). Runs32k56,754ms préfixe et62,641ms permuté,
contre1103,596 et1123,373ms. Sorties6/6/6 et6/6/8 respectivement.
Tous les autres ratios, y compris supérieurs à4, figurent dans les
lectures de la campagne, sans masquer préparation, mémoire ou validation.

Le prétraitement n'est pas gratuit : permuté32k/K10,619777 comparaisons
lexicographiques,525760 orientations,263252 copies d'indices d'enveloppe,
130960 déplacements de groupes. Le pic dynamique propre est1861533octets,
contre267672 pour28 dans ce cas : mémoire plus élevée, capacités et non RSS,
hors nuage/index/cover, objets fixes et consommateurs.
Préparation commune+run :577,233ms contre1120,864ms à32k/K10 permuté.
Ce n'est pas le contrat de tour50k/G4.

## Contre-régimes et croissance non résolue

- **AdversaireK10 : aucun site retiré.** n32/64/128/256 donne
  W960/3968/16128/65024 (×4,133/×4,065/×4,032), tris
  4943/25374/133728/623717 (×5,133/×5,270/×4,664).
  Toujours36 sorties q4, donc pas un carré imposé par la sortie.
  À256, run18,302ms contre1,783ms pour28, régression×10,27.
- AdversaireK5 : r32/64/102/110, W960/3968/10200/11880 ;
  le premier doublement dépasse×4, puis baisse. Run à256 :2,855ms
  contre1,238ms ; ne pas sélectionner seulement les derniers ratios.
- **Cap : réduction réelle mais mauvais choix de méthode.** À32k/K10,
  r1149, deux seeds, W2298 contre22 pour28 ; run17,468 contre0,158ms.
  Sommes préparation+run30,886 contre13,577ms. Les blocs28 savent déjà
  rejeter en masse : le tri commun et les balayages29 ajoutent du travail.
- Far : six sites retenus, deux seeds, douze lectures ; préparation
  globale dominante. Ce fond n'étudie pas une croissance des seeds.
- Petit préfixe32 colinéaire : r32/S30,960 lectures malgré zéro sortie ;
  le repli dégénéré sûr ne promet aucune réduction.

La sélection coûte O(m log(1+m)+Km), sous-quadratique à K fixé.
Le producteur aval reste O(Sr log(1+r)), hors travail ajouté par le
consommateur. r et S peuvent être de l'ordre de m. **Aucune borne
générale sous-quadratique**, ni promotion de29 comme défaut universel.
Prochaine question : ne visiter que les événements peu profonds et/ou
composer cette sélection avec les partitions28, contacts et coût total
payés. WSPD multivoie, q3 global, catalogue/intérieurs, FULL, GPU/G4 et
massif restent ouverts. s8/10/12 se compare au futur raccord global,
pas dans cette primitive sans paramètre s.

## Relecture

```bash
python morsehgp3D_v8/bench/run_q4_shallow_checks.py read morsehgp3D_v8/receipts/q4_shallow_20260920/scale__b3aquyb --check-live
python -O morsehgp3D_v8/bench/run_q4_shallow_checks.py read morsehgp3D_v8/receipts/q4_shallow_20260920/scale__b3aquyb --check-live
python morsehgp3D_v8/bench/run_q4_shallow_checks.py selftest morsehgp3D_v8/receipts/q4_shallow_20260920/smoke_254ekmp2
```
