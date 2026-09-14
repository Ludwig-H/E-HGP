# Census q2 conjoint — qualification corrigée r2

14 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
GCP non utilisé. Aucun contrat de tour FULL/G4.

## Objet et provenance

Trois modes sur les mêmes entrées, le même front et les mêmes candidates :
`anchors` (référence), `joint` (division du plus large facteur A/B),
`joint-a` (A seul jusqu'au relais singleton). Compte, curseur, phase et
B original sont transmis ensemble. A reste dans les témoins tant qu'il
est un groupe. Le [contrat conjoint](../../docs/P0_CENSUS_CONJOINT_Q2.md)
donne la preuve et distingue les vingt compteurs de cette étape.
Le défaut reste Individual/Global/Disabled.

La [capture initiale](../q2_joint_20260914/README.md) est conservée :
47 CTests Release passent, 46/47 sous Clang, défaut réel du collecteur
Python à l'interruption. Ses 36 mesures ne sont pas réattribuées à r2.
La correction protège la lecture des deux flux contre les exceptions
asynchrones, puis rejoue les signaux hors de cette lecture. L'annulation
reste ciblée sur le processus possédé ; TERM puis KILL restent disponibles.
Ce changement ne limite pas la durée d'un benchmark.

Builds neufs : `build/v8_joint_r2_20260914` (GCC 13.3 Release) et
`build/v8_joint_sanitize_r2_20260914` (Clang 18.1.3 Debug ASan/UBSan,
détection des fuites conservée). Boost 1.83 sert aux oracles seulement.
Le code C++ ne change pas entre les deux captures ; le binaire Release
de la sonde est même identique octet pour octet, SHA256
`587719db78c7e63dc9c7b0b998abc95506fc998fc544ed71a66bb94cf3a8e1bd`.
La qualification r2 ne dispense pas des empreintes des scripts corrigés.

Les gates nouvelles comparent 5 336 appels et 124 698 supports complets
à l'oracle scalaire indépendant, avec 285 376 incidences de coquille,
K1/2/5/10, s8/10/12, transformations, rejets et exceptions. Le juge
des bornes vérifie 3 190 cas par calcul rationnel indépendant et grille
exhaustive, y compris les maxima intérieurs et les extrêmes u16.
Les cinq mutants de parcours et neuf mutants arithmétiques sont des
contre-modèles annoncés, pas des mutations de binaires produit.

L'[audit A publié à 7e315009](../../audits/q2_product_20260914/README.md)
confirme le relais et prouve la limite de raffinement : aucune admission
ni seconde phase conjointe accessible avant singleton avec cette règle.
Ces deux compteurs sont rapportés sans plancher artificiel. Son modèle
FIFO/LIFO reste une preuve séparée, pas une implémentation distribuée C++.

La porte des reçus WSPD compare 352 petites captures v1/v2/v3/v4,
176 configurations et 118 mutants. Les anciens formats gardent leurs
contrôles ; les nouveaux adaptent seulement les racines/tâches et les
comptages dont le périmètre a changé. Les six nouvelles vérifications
du collecteur complètent les six contrôles de lancement : deux flux,
SIGINT/SIGTERM, second signal pendant annulation, absence de signal,
handler non levant, groupes ciblés, codes, octets/base64 et restauration.

Le [pilote exact](qualification/record.py.snapshot), SHA256
`d0588d2659105cdd7ad5b55e89485c20b20c4fda7b2978d9fa6f5708b7e16541`,
teste les binaires sans reconstruire. Chaque tentative conserve commandes,
flux bruts et codes, puis vérifie sources et artefacts à la fermeture.
`release` et `sanitize` exécutent 47 CTests et les contrôles CLI ;
`readers` compare les lecteurs normal/−O et contrôle les documents.
Les builds r2 sont désormais épinglés ; toute révision
suivante devra employer de nouveaux chemins.

Les **47 CTests passent** en [Release](qualification/release_z5qr9wi7/RESULT.json)
et sous [Clang ASan/UBSan](qualification/sanitize_87mwoi3n/RESULT.json),
détection des fuites conservée. Chaque capture contrôle 27 commandes,
avec sources et artefacts inchangés à la fermeture. Les tests historiques
de lancement et les nouveaux tests de lecture à l'interruption passent
dans les deux modes Python. Aucun contrôle n'est désactivé pour r2.
Les [lecteurs normal/−O](qualification/readers_01yuz13n/RESULT.json)
passent avec sorties exactement identiques, 44 mesures/configurations,
empreintes des sources, binaires et campagnes stables à la fermeture.
Ces résultats sont locaux ; ils ne représentent pas une CI GPU.

## Protocole

Comparaison à 8k sur les quatre familles et s8/10/12, puis croissance
du bras `joint-a` à 16k/32k et s8. Kmax10, seed3, Samples/Shared,
Complement/sibling sont communs aux bras. Aucun optimum de ces options
n'est présumé. Le terrain est synthétique, pas une nouvelle capture LiDAR.

Un thread par mesure, une seule répétition, sans warmup ni CPU isolé,
sur hôte partagé avec des qualifications et audits concurrents. Les
temps sont exploratoires, sans p95 ni attribution statistique. Le total
comprend génération, propriétaire, index, front q2, census, collecte
complète, copie/tri/hash du callback, validations et destructions.
Il ne faut pas lui retrancher un ancien front à trois voies pour inventer
un temps census isolé. Les sous-chronomètres peuvent être imbriqués.

Une borne conjointe paie douze couples de constantes, une borne à
ancre fixe six ; leurs compteurs ne sont pas des instructions de même
coût. Publier séparément tests conjoints, tests après relais, tâches,
opérations structurelles, certificats frère et collecte. Une reprise
figure dans les tâches conjointes et dans les tâches individuelles :
ne pas interpréter leur somme comme un nombre de jobs disjoints.

## Comparaison appariée à 8k

La [campagne à 8k](paired_8k/MANIFEST.json) ferme 36 appels réussis.
Temps totaux q2 en secondes, s8, même révision :

| Famille | anchors | joint A/B | joint-a |
| --- | ---: | ---: | ---: |
| Uniforme | 5,009 | 5,074 | 4,988 |
| Terrain synthétique | 0,925 | 0,943 | 0,910 |
| Amas | 12,088 | 13,287 | 12,121 |
| Deux rangées | 0,242 | 1,888 | 0,237 |

Le partage équilibré A/B est un **contre-résultat de performance**,
particulièrement sur les rangées. Il réduit certains parcours communs,
mais fragmente B avant que le certificat frère puisse agir. Sur amas/s8,
les 1 291 731 ancres descriptives deviennent 15 375 375 relais,
avec 31 052 280 appels conjoints puis 24 918 445 appels à ancre fixe.
Les visites après relais baissent de 711 921 875 à 639 862 100,
mais 84 152 949 bornes conjointes plus coûteuses s'ajoutent. Les
rejets du certificat frère passent de 2 138 020 paires à seulement 10.
Sur les rangées il ne rejette plus rien, contre 7 846 336 paires en
référence. Le nombre inférieur de racines ne constituait donc pas le
bon critère de décision.

`joint-a` conserve le certificat frère et n'émet au plus qu'un relais
par ancre initiale. Sur amas/s8 : 1 290 987 relais, 710 217 109 visites
après relais et 1 148 784 bornes conjointes. L'économie est trop faible
pour conclure à un gain net ; la comparaison à s10/12 conduit à la même
décision. Les faibles écarts temporels des trois autres familles ne
justifient pas non plus une promotion dans ce protocole partagé.
Les trois modes restent comparables ; Individual reste le défaut.

Les [contrôles indépendants B publiés à e931d8f6](../../audits/chaine_q2_20260914/CHAINE_Q2_JOINT_CHECKS.json)
confrontent aussi les trois modes conjoints aux paires exhaustives sur
leur instantané de sources. Ce reçu complète la contrelecture, sans
remplacer la qualification r2 du collecteur ni nos mesures de croissance.

La croissance 16k/32k porte donc sur `joint-a` à s8. Le bras A/B
équilibré n'est pas étendu à ces tailles : son coût et sa fragmentation
à 8k justifient de conserver ce contre-résultat sans dépenser une
nouvelle matrice complète sur lui. Les données initiales pré-correctif
restent séparées et ne fournissent aucune ligne de ce tableau.

## Croissance et verdict

Les [8 mesures de croissance](growth_s8/MANIFEST.json) sont closes,
soit **44 mesures et 44 configurations** avec la campagne appariée.
Temps totaux de cette révision, `joint-a`, s8 :

| Famille | 8k (s) | 16k (s) | 32k (s) | Multiplicateurs des visites après relais |
| --- | ---: | ---: | ---: | --- |
| Uniforme | 4,988 | 12,026 | 28,469 | ×2,405 puis ×2,408 |
| Terrain synthétique | 0,910 | 1,889 | 4,423 | ×2,095 puis ×2,222 |
| Amas | 12,121 | 45,934 | 182,155 | ×4,107 puis ×4,231 |
| Deux rangées | 0,237 | 0,460 | 0,941 | ×2,158 puis ×2,104 |

Les nouvelles mesures 16k/32k ne sont pas appariées à de nouveaux temps
`anchors` de même taille. Les ratios portent sur le travail du même
mode, pas sur une comparaison de latence entre révisions différentes.
Les bornes conjointes restent supplémentaires :

| Famille | Visites après relais 8k / 16k / 32k | Bornes conjointes 8k / 16k / 32k |
| --- | --- | --- |
| Uniforme | 166 373 951 / 400 066 578 / 963 195 648 | 3 738 744 / 9 097 650 / 25 592 897 |
| Terrain | 18 831 356 / 39 459 967 / 87 664 005 | 1 101 028 / 2 230 751 / 4 939 771 |
| Amas | 710 217 109 / 2 916 766 695 / 12 339 594 540 | 1 148 784 / 4 109 647 / 13 490 452 |
| Rangées | 5 670 659 / 12 237 704 / 25 748 070 | 66 366 / 147 068 / 314 473 |

Sur amas, les tâches après relais sont 37 635 957 / 143 560 728 /
541 294 802 ; les appels conjoints 1 372 354 / 3 625 606 / 8 875 393.
Les supports croissent beaucoup moins vite : 245 733 / 520 208 /
1 088 696. Les descentes structurelles après relais valent 22 066 320 /
56 167 425 / 150 459 173, les descentes conjointes 249 804 / 843 882 /
2 723 862. Les certificats frère restent payés en plus, jusqu'à
51 593 995 tests à 32k. Le volume dominant est toujours celui des
recherches sur les candidates, pas celui des seuls supports émis.

**P0 n'est pas clos : la croissance demandée échoue encore sur les amas.**
Le mode A seul évite l'explosion supplémentaire du partage A/B, mais
ne retire pas le travail quasi quadratique dominant. Les trois autres
familles ont une croissance observée plus favorable sur ces tailles,
pas une preuve asymptotique générale. Aucun mode n'est promu par défaut.

À 32k, les capacités déclarées restent 192 000 octets d'entrée,
960 000 de propriétaire et 3 926 016 d'index ; les buffers du callback
valent 96 ou 104 octets selon famille/ordre d'émission. Ce ne sont pas
un pic RSS ni une mesure des piles, tampons internes transitoires ou
allocations du processus. Aucun tableau global de toutes les candidates
ou de toutes les sorties n'est matérialisé. Cela ne qualifie pas la
résidence à plusieurs dizaines de millions de points.

La [prochaine intégration Pool/paires](../../docs/P0_POOL_TERMINAL_RACCORD.md)
est motivée par le gain q2 complet du prototype A à fbbecc01 : il
filtre les gros rectangles avant ce coût résiduel. Sa preuve et ses
mesures restent indépendantes, non héritées par le produit. Après ce
filtre, front et petits rectangles deviennent prioritaires. q3/q4,
catalogue canonique, FULL, multi-CPU, GPU et contrats G4 restent ouverts.
