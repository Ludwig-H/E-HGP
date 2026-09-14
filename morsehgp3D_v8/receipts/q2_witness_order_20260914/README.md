# Ordre des témoins q2 — dixième tranche

14 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
GCP non utilisé. Cette capture ne qualifie ni FULL ni le GPU.

## Objet et portée

L'option `ComplementFirst` examine le complément du B original, sans
l'ancre a, puis B original. Les enfants conservent contexte, phase,
curseur et compte. Aucune liste de témoins ni préparation de facteur
n'est introduite ; l'ancre reste dans la coquille collectée. Le
[contrat](../../docs/P0_ORDRE_TEMOINS_Q2.md) précise preuve, coûts et
limites de la continuation actuelle, encore synchrone.

Quatre variantes sur le même front : Global/none, Complement/none,
Global/sibling, Complement/sibling. Le défaut reste Global/none.
Le temps total englobe génération, propriétaire, index, front q2,
census, collecte, callback canonique et destructions. Il ne faut pas
retrancher un ancien temps de front à trois voies pour isoler ce census.

## Qualification et provenance

Builds propres : `build/v8_witness_order_20260914` (GCC 13.3 Release)
et `build/v8_witness_order_sanitize_20260914` (Clang 18.1.3 Debug,
ASan/UBSan, détection des fuites conservée). Boost 1.83 du répertoire
`build/v7_boost_gate/extracted/usr` sert aux oracles, pas au produit.
Les builds deviennent épinglés après cette capture : ne pas les écraser.

La nouvelle gate compare 1 798 appels, 48 922 supports complets et
138 352 incidences de coquille, avec 436 117 contrôles. Le juge scalaire
indépendant utilise les trois produits i64 de H ; il vérifie aussi les
clés, chaque intérieur et toute la frontière, K1/2/5/10, s8/10/12,
réflexions/permutations, exceptions et requêtes invalides. Les 256
comparaisons implicite/Global vérifient le défaut. Les cinq mutants
modèles sont annoncés comme tels, pas comme mutations du code produit.

La fixture 77 sites expose douze témoins universels. Sur toute la WSPD
Pure/s12 et les deux modes frère, les divisions B passent de 1 638 à
1 384. Ce compteur agrégé ne prétend pas instrumenter une seule ancre.
L'[audit A publié à a1ee8cb0](../../audits/q2_complement_20260914/README.md)
prouve et teste séparément les reprises ; sa qualification modèle ne
remplace pas ces tests C++.

Les 45 CTests passent en [Release](qualification/release_m7o4dawg/RESULT.json)
et sous [Clang ASan/UBSan](qualification/sanitize_nc4ydh9i/RESULT.json),
avec 23 commandes contrôlées par capture, hashes de fermeture stables
et détection des fuites conservée. Les [lecteurs normal/−O](qualification/readers_ossbi31a/RESULT.json)
passent avec sorties identiques, 56 mesures et 56 configurations ; leurs
hashes de fermeture et celui du binaire restent stables. Les portes de
reçus exercent 256 petites captures v1/v2/v3 et 89 mutants dans chaque
mode Python. Le [préflight invalidé](preflight/README.md) reste conservé :
le constructeur avait modifié CMake pendant une capture avant gel.
Aucun contrôle de provenance n'a été désactivé pour le faire passer.

Le pilote exact est conservé dans `qualification/record.py.snapshot`
(SHA256 `482a258be6e8d662053f3969f2f9d9f6a07a6d20054548e94d1dea219b8cfb70`).
Ses chemins de build sont explicites ; `release`, `sanitize` et `readers`
testent des binaires existants, sans compilation, et créent un nouveau
répertoire de tentative. Pour une nouvelle révision, adapter une copie
à de nouveaux builds avant toute capture, jamais modifier ces preuves.
`--execution-context` enregistre une déclaration, pas une élévation.

## Comparaison appariée à 8k

[paired_8k](paired_8k/MANIFEST.json) : 48 mesures, quatre familles,
s8/10/12 et quatre variantes, Kmax10/seed3/Samples/Shared, un thread.
[growth_s8](growth_s8/MANIFEST.json) poursuit à 16k/32k les quatre
familles avec Complement/sibling, choisi pour mesurer sa croissance
après la comparaison 8k. Les nouveaux temps 16k/32k ne sont pas appariés
à de nouvelles exécutions Global. Aucune nouvelle mesure LiDAR ici ;
le terrain est un slab synthétique. Les schémas de capture/sonde sont v3.

Une seule répétition, sans warmup ni CPU isolé. Des qualifications et
audits indépendants coexistent sur l'hôte ; les temps sont exploratoires,
pas des p95 ni des différences statistiquement attribuées. Les compteurs
et les sorties sont déterministes et comparés par le lecteur.

Temps total q2 en secondes à n8k/s8, nouvelles sources seulement :

| Famille | Global/none | Complement/none | Global/sibling | Complement/sibling |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | 4,794 | 4,807 | 4,723 | 4,803 |
| Terrain synthétique | 0,897 | 0,896 | 0,898 | 0,888 |
| Amas | 12,964 | 11,408 | 12,672 | 11,378 |
| Deux rangées | 1,428 | 1,077 | 0,229 | 0,201 |

Sur uniforme, la baisse 200,53→171,90 millions de visites géométriques
avec Complement ne donne **pas de gain de temps** : 26,56 millions de
divisions structurelles s'ajoutent, plus sauts et transitions. Sur amas,
le nouvel ordre améliore le total de cette capture mais ne supprime pas
le travail dominant. Sur les rangées, il complète le gain du frère.
Une baisse de quelques centièmes de seconde n'est pas une preuve de gain
robuste dans ce protocole partagé.

Temps Complement/sibling 8k pour s8 / s10 / s12 : uniforme
4,803 / 4,709 / 4,824 ; terrain 0,888 / 0,961 / 0,965 ; amas
11,378 / 11,360 / 11,093 ; rangées 0,201 / 0,204 / 0,219 s.
Changer s ne constitue pas une amélioration monotone générale ; les
faibles écarts temporels ne suffisent pas à choisir un optimum.

## Croissance et décision

Les 8 mesures de croissance sont closes, soit **56 configurations et
56 mesures** avec la comparaison appariée. Voici les temps totaux
Complement/sibling, s8, de cette révision uniquement :

| Famille | 8k (s) | 16k (s) | 32k (s) | Visites géométriques × à chaque doublement |
| --- | ---: | ---: | ---: | --- |
| Uniforme | 4,803 | 11,235 | 26,675 | 2,406 puis 2,421 |
| Terrain synthétique | 0,888 | 1,840 | 4,033 | 2,091 puis 2,223 |
| Amas | 11,378 | 43,750 | 173,471 | 4,106 puis 4,229 |
| Deux rangées | 0,201 | 0,437 | 0,942 | 2,159 puis 2,107 |

Travail géométrique et structurel, unités volontairement séparées :

| Famille | Visites géométriques 8k / 16k / 32k | Divisions structurelles 8k / 16k / 32k |
| --- | --- | --- |
| Uniforme | 171 895 354 / 413 553 244 / 1 001 201 993 | 26 562 215 / 65 188 415 / 154 325 015 |
| Terrain | 20 472 635 / 42 798 408 / 95 128 515 | 7 939 142 / 17 447 595 / 39 812 440 |
| Amas | 711 921 875 / 2 922 835 710 / 12 359 585 696 | 22 863 138 / 58 740 489 / 158 367 076 |
| Rangées | 5 742 485 / 12 397 590 / 26 120 016 | 1 740 354 / 3 946 378 / 8 738 137 |

Sur les amas, les tâches valent 37 636 701 / 143 561 382 / 541 296 330,
les propositions frère 36 344 970 / 140 166 132 / 533 068 354, et les
tests frère 3 611 796 / 13 670 971 / 51 593 995. Les supports ne sont
que 245 733 / 520 208 / 1 088 696. Le coût géométrique dominant reste
donc quasi quadratique malgré une sortie proche du linéaire sur ces
tailles : **P0 n'est pas clos et cette chaîne n'est pas globalement
sous-quadratique**. Les croissances plus favorables des trois autres
familles sont des observations à trois tailles, pas une preuve asymptotique.

Sur les rangées, les bornes frère supplémentaires sont 180 844 /
425 848 / 915 856 et les supports 123 860 / 247 860 / 495 860.
Les temps proches d'une seconde à 32k portent sur q2 CPU seulement,
pas sur une tour 50k G4. Les compteurs bruts conservent aussi les sauts,
phases et coûts du front. Ne pas sommer leurs unités différentes ou
les compteurs qui se recouvrent comme s'il s'agissait d'instructions.

Les compteurs mémoire décrivent les capacités retenues, pas le pic RSS
ni la pile récursive. Aucun buffer global de toutes les candidates ou
sorties n'est matérialisé par la sonde. Cela ne qualifie pas la résidence
à plusieurs dizaines de millions de points ni un export de tour FULL.
À 32k, capacités déclarées : entrée 192 000 octets, propriétaire
960 000, index 3 926 016, buffers du callback 96 ; ces postes ne
constituent pas un total de mémoire maximale du processus.

L'ordre reste optionnel. Le prochain axe partage A et B avant le passage
aux ancres uniques, avec bornes conjointes et reprise sans recommencer Z.
Ne pas omettre tout A du compte. Une racine B singleton peut aussi faire
l'objet d'une comparaison isolée avec l'ancien ordre ; aucun descendant
déjà crédité ne peut être réinitialisé pour cette optimisation.
P0, q3/q4, parents FULL, multi-CPU, GPU et les contrats G4 restent ouverts.
