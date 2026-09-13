# Une coquille complète, cinq supports : préparer le catalogue sans perdre les incidences

13 septembre 2026, census en cours après `256957a5`. Cadre :
`exploration_v8_hors_registre / cpu_reference / quantized_u16_input_only /
audit_independant_math_and_architecture / not_claimed`.
Fixture bornée exécutée sur le véritable census, sans modification moteur.

## Une répétition qui traverse les préconditions actuelles

Le [juge C++](q2_repeated_shell_probe.cpp) construit 398 sites sur une
même sphère de rayon 1105 et de centre (2000,2000,2000). Relativement
au centre, prendre les cinq sites de A :

```text
(1105,0,0), (1104,47,0), (1104,-47,0), (1104,0,47), (1104,0,-47)
```

B contient leurs antipodes. Les autres sites sont les solutions entières
de x²+y²+z²=65², multipliées par 17 ; l'union déduplique les pôles déjà
présents. La construction utilise une table de carrés entiers, sans arrondi.
Les coordonnées translatées restent u16. Les boîtes des deux facteurs
ont une diagonale carrée de 17 673 et un écart minimal carré de 4 875 264,
supérieur à 12²·17 673 : les trois séparations s8/10/12 sont admissibles.
Tous les autres sites appartiennent au même propriétaire et restent
hors A∪B ; le census doit pourtant les retrouver.

Le filtre additif conserve les 25 paires. Pour chacun des seuils 1, 5
et 10, le census garde exactement les cinq diamètres antipodaux. Ils ont
tous zéro intérieur, la même clé entière et **398 IDs de coquille**.
Les 20 autres paires ont assez d'intérieurs pour être rejetées, d'après
le calcul indépendant de H sur tout le produit. Cette fixture est un
plateau sphérique volontairement non régulier, pas un échantillon de
performance ni une qualification Gabriel stricte ou FULL.

| Objet ou travail par appel | Quantité |
| --- | --- |
| Candidates réellement consommées | 25 |
| Incidences de supports conservées | 5 |
| Géométries de boule distinctes | 1 |
| Intérieurs | 0 |
| IDs de coquille distincts pour cette boule | 398 |
| IDs de coquille émis dans le flux actuel | 1 990 |

Les deux modes du moteur produisent les mêmes ensembles exacts d'IDs.
La répétition n'est pas un bug : le contrat est un flux de supports,
avec une seconde collecte complète pour chaque support. Sur les 36
appels, le reçu compte 143 100 visites de collecte et 71 640 IDs de
coquille émis. Ces dépenses ne sont pas supprimées par le partage des
préfixes du seul comptage, puisque les cinq supports ont des ancres A
différentes.

## Le prochain objet peut partager le payload et conserver les cinq incidences

Dans la portée d'un même nuage immuable et du même espace d'IDs,
normaliser le résultat en une table de boules et une table d'incidences :

- une entrée par clé exacte `(a+b, |a-b|²)`, avec intérieur et coquille ;
- une incidence `(a_id,b_id,ball_id)` pour chaque support reçu.

Le [contrat entre seuils et supports](P0_CENSUS_PARTAGE_ET_SEUILS.md)
précise la portée de cette clé. Le juge normalise effectivement les
payloads copiés, puis reconstruit le flux original et vérifie son égalité
exacte. Dans cette fixture, une coquille de 398 IDs et cinq références
conservent toute l'information des 1 990 IDs répétés. Sur les 36 appels
indépendants, cela représente 14 328 entrées de coquille canonique contre
71 640 entrées dans les incidences développées. Les références et clés
ont leur propre stockage ; ces cardinalités ne mesurent pas la RAM.

Un mutant qui supprime aussi les quatre incidences répétées échoue :
dédupliquer la géométrie ne permet pas de perdre ses supports. Le
round-trip du juge distingue ces deux décisions. Il ne normalise pas
silencieusement le flux produit et ne propose pas un cache entre des
propriétaires de rectangles différents.

Normaliser **après** les callbacks réduit le stockage ultérieur éventuel,
mais ne rembourse aucune visite de collecte déjà exécutée. Pour éviter
ces recherches répétées, un futur catalogue pourrait regrouper les
demandes par clé avant collecte, ou réutiliser une coquille déjà certifiée.
Le tri, les recherches de clé, les collisions résolues par égalité exacte,
la résidence du catalogue et les incidences doivent alors être payés.
Il faut aussi distinguer un consommateur qui lit chaque coquille complète
par support d'un consommateur qui lit une boule puis ses références :
seul le second évite ces lectures répétées en sortie.

Cette fixture prépare le raccord de catalogue déjà annoncé. Elle ne
demande pas de remplacer le flux courant avant sa qualification. Aucun
gain global n'est déduit d'une sphère commune ; le nombre et la somme
des coquilles de boules distinctes restent à maîtriser.

## Preuve exécutable et limites

Le [runner](q2_repeated_shell_checks.py) conserve neuf sources produit
et le juge dans le [reçu](Q2_REPEATED_SHELL_CHECKS.json). Le cpp census
`3c513cc4…` et le hpp `2116ac4b…` délimitent les octets exécutés.
Les deux ordres d'IDs, les trois seuils et s8/10/12 donnent 18 propriétaires
et 36 appels. Le juge indépendant réalise 179 100 tests de H ; il compare
les payloads complets, les supports, les clés et les comptes de sortie,
pas seulement un digest entre les deux modes du même moteur.

C++20 strict, GCC/O2/UBSan sans récupération, normal et rejeu −O concordants.
Le mutant d'audit perdant des incidences est réfuté avec code 1 ; aucune
mutation du produit n'est prétendue dans ce dernier contrôle.

```bash
python3 -B audits/morsehgp3D_v8_complementaire/q2_repeated_shell_checks.py --selftest
python3 -B -O audits/morsehgp3D_v8_complementaire/q2_repeated_shell_checks.py --replay audits/morsehgp3D_v8_complementaire/Q2_REPEATED_SHELL_CHECKS.json
```

Aucune durée, campagne lourde ou implémentation de catalogue produit.
P0 et FULL restent ouverts. GCP non utilisé.
