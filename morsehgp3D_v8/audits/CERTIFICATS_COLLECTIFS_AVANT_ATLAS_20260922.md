# Rejeter des arêtes avant l’atlas : certificats collectifs

22 septembre 2026. Audit indépendant, base `a74e90f22167105cdba90b6850f0597f1a01a329`.
`exploration_v8_hors_registre`, prototype CPU entier 18 bits, `public_status=not_claimed`.
**Aucun moteur modifié, aucun build/CTest HGP ni benchmark LiDAR/GPU exécuté.**
Sources et résultats reproductibles : [collective_edge_20260922/](collective_edge_20260922/).

## Décision utile

**Tester un filtre collectif autonome avant la construction des covers/atlas, sans
l’activer partout.** Il peut rejeter des arêtes qu’aucune recherche de témoins
individuels ne peut rejeter. Le prototype démontre ce pouvoir supplémentaire,
pas un gain de temps du pipeline. La recherche des groupes et son acquisition
peuvent coûter plus que l’aval évité ; les mesures ci-dessous rendent ce risque concret.

Point de raccord : `Engine::edge`, après le filtre actuel et avant
`Q34EdgeCover::make`, dans [wspd_q34.cpp](../src/pipeline/wspd_q34.cpp).
Le [filtre actuel](../src/lanes/q34_witness_search.cpp) admet des témoins
universels individuels, éventuellement par blocs. Le
[collectif existant](../src/lanes/q34_collective.hpp) traite une famille de
**graine** `abx` : le présent certificat concerne une **arête** `ab`, avant les graines.
Les propositions d’atlas à la demande et de census par blocs restent intéressantes,
mais ce lot ne leur attribue aucun résultat expérimental nouveau.

## 1. Certificat exact et largeur arithmétique

Pour `d=b-a`, poser

\[
H_z=(z-a)\cdot(b-z),\qquad V_z=d\times(z-a).
\]

Pour un groupe fixe `S`, sommer `H_S` et `V_S`. Pour **toute** boule à support
positif d’arité `q`, dont `ab` est une plus longue arête,

\[
H_S>0,\qquad \alpha H_S^2>\|V_S\|^2,
\qquad \alpha=3\ (q3),\quad \alpha=2\ (q4)
\]

certifie **au moins un** point strictement intérieur dans `S`.
Ce point peut changer avec la boule. Un groupe n’apporte pas `|S|` crédits.

Preuve. Avec `D=|d|²`, `m=(a+b)/2`, `c=m+t`, la positivité donne des poids
barycentriques `lambda_i>0`. L’identité de variance implique
`R² <= D(1-sum lambda_i²)/2 <= D(q-1)/(2q)` ; donc
`|t|²=R²-D/4 <= D(q-2)/(4q)`. Puis

\[
\sum_{z\in S}(\|z-c\|^2-R^2)
\le -H_S+\|V_S\|/\sqrt\alpha<0.
\]

Au moins un terme est strictement négatif. Cette preuve nécessite la positivité
et l’arête maximale, pas l’acceptation préalable d’une boule q2/q3.

Le prototype n’évalue que des singletons et paires. Avec `M=262143`,
`|H_S|<=6M²`, `|(V_S)_i|<=4M²` ; `alpha H_S²<=108M⁴<2^79` et
`|V_S|²<=48M⁴<2^78`. **i64 pour les sommes, i128 avant les produits** suffisent.
Pas de racine, flottant ou multiprécision. Ne pas extrapoler ces bornes à des
sommes de taille arbitraire. Les coordonnées sont vérifiées avant calcul.

## 2. Nouveau résultat : un triangle de certificats donne deux crédits

Former un petit graphe de témoins : une arête `ij` signifie que la paire
`{z_i,z_j}` satisfait le certificat. Pour chaque boule admissible, ses points
intérieurs couvrent toutes les arêtes de ce graphe.

Un appariement de paires disjointes donne un crédit par paire. **Trois témoins
dont les trois paires sont certifiées donnent deux crédits**, car deux témoins
extérieurs laisseraient leur paire sans intérieur. Des triangles disjoints et
les paires restantes se cumulent ; retirer d’abord les témoins individuellement
certifiés. Le prototype conserve le meilleur des deux groupements gloutons
(appariement seul ; triangles puis appariement). Aucun optimum combinatoire revendiqué.

Fixture constructive : `a=(400,400,400)`, `b=(600,600,600)`, `m=(500,500,500)`.
Pour chacun des cinq `r=80..84`, prendre
`m+r(1,-1,0)`, `m+r(0,1,-1)`, `m+r(-1,0,1)`.
Ajouter les graines `(650,350,500)` et `(650,500,350)` : 19 sites au total.

| Sur cette arête, pour q3 et q4 | Crédits certifiés |
|---|---:|
| Tous les témoins individuels du nuage | 0 |
| Meilleur appariement possible des 15 témoins proposés | au plus 7 |
| Cinq triangles disjoints trouvés automatiquement | **10** |

À K10, les seuils sont 9 et 8 : les triangles rejettent les deux voies,
les paires seules ne suffisent pas sur ce pool. L’oracle trouve effectivement
2 supports q3 et 1 support q4 positifs possédés ; profondeur minimale 15.
Ce n’est pas un rejet démontré seulement sur une famille vide.
80 requêtes après permutation du pool et translation/agrandissement dans la
plage haute des 18 bits retrouvent 10 crédits.

## 3. Ne pas payer un graphe qui ne pourra pas rejeter

Soit `s` le nombre de singletons déjà certifiés dans le **même pool**, `m` le
nombre de sites restants, `h` le nombre total de sites du pool avec `H_z>0`,
et `T=K-1` ou `K-2`. Le groupement ci-dessus ne peut atteindre `T` si

\[
\min\{h,\ s+\lfloor2m/3\rfloor\}<T.
\]

Une paire certifiée contient au moins un `H_z>0` ; un triangle certifié en
contient au moins deux. Cela justifie la première borne ; le nombre de sites
consommés par les groupes justifie la seconde.
**Ces gardes abandonnent le filtre, pas l’arête ni le census exact.** Elles ne
prétendent pas majorer la profondeur réelle de la boule.

Sur les 8 736 requêtes du corpus exact : 7 271 recherches de groupes deviennent
inutiles ; **499 270 tests de paires passent à 62 117**, sans changer un seul
bit de rejet. C’est une réduction de travail de ce prototype, pas du pipeline.

## 4. Extension WSPD : mêmes groupes sur tous les coins, impérativement

Pour un groupe fixe, `(H_S,V_S)` est affine séparément en `a` et en `b`.
Le cône `H>0, |V|<sqrt(alpha)H` est convexe. Vérifier **ce même groupe** sur
les 64 couples de coins suffit donc pour tout `A x B`, puis pour ses enfants.
Le groupement et ses IDs doivent rester communs : un appariement différent
pour chaque coin ne donne pas cette preuve.

Contre-exemple exact à ce raccourci : `b=(300,200,200)`, `A` est le segment
entre `(200,100,200)` et `(200,300,200)`, pool `{(250,150,200),(250,250,200)}`.
Chaque extrémité de `A` dispose d’un témoin individuel universel, mais au milieu
`a=(200,200,200)`, les deux témoins sont **sur la coquille**, sans intérieur,
pour le q3 de troisième sommet `(250,200,260)` et le q4 de sommets supplémentaires
`(250,230,270),(250,170,270)`. Supports positifs et arête maximale vérifiés.
Le rejet par les bornes indépendantes aux coins serait faux à K2/q3 et K3/q4.
L’extension correcte est aussi testée : 1 152 tests de coins et
13 122 tests de couples d’extrémités intérieures à deux boîtes non ponctuelles.

## 5. Résultats, portée et coût

C++ autonome GCC 14.2 `-O3`, puis Clang 17 ASan/UBSan, fuites activées ; chacun
rejoué avec le vérificateur Python normal et `-O`. **Quatre sorties identiques,
aucun diagnostic sanitizer.** Oracle indépendant en `Fraction`, résolution de
Gram, énumération des supports, sans appel au prédicat collectif pour décider
les boules ou leurs intérieurs.

| Corpus exact : 24 petits nuages, K5/K10, q3/q4 | Résultat |
|---|---:|
| Boules positives possédées / puissances rationnelles | 4 891 / 68 474 |
| Requêtes de filtre / avec au moins un support admissible | 8 736 / 3 956 |
| Rejets individuels exhaustifs sur les autres sites du nuage | 280 |
| Rejets avec paires / avec paires et triangles | 521 / 522 |
| Rejets supplémentaires / dont familles non vides | 242 / **123** |
| Faux minorants de profondeur | **0** |

Les triangles n’ajoutent qu’un rejet aux paires dans ce corpus aléatoire ; leur
intérêt à K10 est démontré par la fixture, pas par un gain général. Rangées et
slabs ne fournissent aucun q4 positif dans ce corpus : leurs rejets q4 ne sont
pas comptés comme des preuves non vides. 160 requêtes supplémentaires portent
sur les 64 points de `{0,1,M-1,M}³` ; neuf entrées invalides sont refusées.
Cinq variantes erronées du noyau, gardes désactivées, produisent effectivement
de faux comptes : chevauchement des paires, deux crédits par paire, `>=`,
absence de `H>0`, constante q3 appliquée à q4. Ce ne sont pas des crashs.

Microbenchmark du prototype, **par requête (arête, voie, K)** :

| Pool de 32 sites, famille synthétique | Survivantes du test individuel du pool | Rejets collectifs | Individuel / collectif gardé, microsecondes |
|---|---:|---:|---:|
| Volume | 225 | 72 | 0,175 / 1,513 |
| Deux plans | 645 | 425 | 0,190 / 3,463 |
| Huit amas | 240 | 75 | 0,180 / 2,318 |
| Sphère | 408 | 41 | 0,188 / 2,871 |

Cinq répétitions avec ordre tournant, CPU partagé AMD EPYC 9V74. Les pools sont
présélectionnés par un scan Python des 512 sites, **hors chrono** : recette de
diagnostic optimiste, pas proposeur de production. Les groupes de preuve sont
alloués par le prototype. Ces requêtes ne sont pas les résidus WSPD du moteur,
et les gains de rejet ne sont pas ceux du filtre global. Le collectif coûte
plus cher que l’individuel sur un même petit pool ; il doit économiser l’aval.

## 6. Port minimal à essayer, avec critère d’abandon

Récupérer un petit pool d’IDs uniques pendant la recherche de témoins existante,
plutôt qu’ajouter un scan du nuage par arête. Des blocs exclus du **témoignage
individuel** peuvent contenir les bons partenaires : leur exclusion ne réfute
pas le collectif. Tester les paires, puis les triangles seulement si nécessaire,
avec les deux gardes ; garder le tout privé à l’arête/worker. La taille du pool
borne les **propositions**, jamais l’exploration ni les sorties du moteur.

Le résultat est un masque de rejet autonome, séparé par voie. Ne pas additionner
ses crédits aux comptes partiels de `filter_q34_witnesses` ni d’un census repris
à la racine. Réutiliser les bilans existants de masses rejetées exactement une
fois, avec une ventilation collective distincte pour la mesure. Échec du pool,
limite de propositions ou seuil non atteint : chemin exact existant inchangé.

Comparer avec/sans sur les **mêmes arêtes survivantes et mêmes entrées 1 mm** :
acquisition + filtre + cover/atlas + graines + census + sorties. Commencer par
les arêtes dont l’aval est cher ; mesurer le coût réellement évité, pas seulement
le pourcentage d’arêtes rejetées. Retenir le port uniquement si
`probabilité de rejet supplémentaire x coût aval conditionnel évité > coût total du filtre`.
**Aucune accélération LiDAR ni cible sub-seconde n’est démontrée par ce lot.**

## Rejouer sans toucher aux builds ni reçus du constructeur

Depuis la racine du dépôt, dans un répertoire temporaire neuf :

```bash
A=morsehgp3D_v8/audits/collective_edge_20260922
T=$(mktemp -d)
g++ -std=c++20 -O3 -Wall -Wextra -Werror "$A/collective_probe.cpp" -o "$T/probe"
clang++ -std=c++20 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-sanitize-recover=all -fno-omit-frame-pointer "$A/collective_probe.cpp" -o "$T/probe_san"
python3 -B "$A/proof_checks.py" --binary "$T/probe" > "$T/release.json"
ASAN_OPTIONS=detect_leaks=1 python3 -B -O "$A/proof_checks.py" --binary "$T/probe_san" > "$T/san.json"
cmp "$T/release.json" "$T/san.json"
python3 -B "$A/microbench.py" --binary "$T/probe" --output "$T/bench" --rounds 100
```

[RESULTS.json](collective_edge_20260922/RESULTS.json) contient les sorties de preuve,
les captures chronométriques, les empreintes et les limites. Les chronos varient ;
les réponses discrètes doivent coïncider. Ce répertoire n’est pas lié au CMake v8.
