# Paires de gardes pondérées : preuve exacte et ablation pré-cœur

23 septembre 2026. Cette expérience prolonge le
[certificat de paires de B](../CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md)
avec des **poids rationnels positifs fixes par paire**. C'est un shadow
d'audit CPU, sans modification du moteur. Les preuves positives sur les
15 nouveaux rectangles S2-positifs sont archivées avec leurs boîtes,
indices de sites, coordonnées et rapports ; le [lecteur](verify_certs.py)
refait leurs 64 coins et vérifie les coordonnées contre l'entrée épinglée.
Les autres résultats sont des agrégats de source et de replay, pas un
chrono de chaîne FULL/G4 ni une loi de croissance.

## Lemme

Pour l'arête propriétaire `ab`, poser `d=b−a`, `D=|d|²`,
`w_g=2g−a−b`, `h_g=D−|w_g|²` et `C_g=d×w_g`, de même pour `h`.
Pour des entiers `λ,μ>0`, le certificat exact de la **même paire de sites
distincts** est :

| Voie | Test strict |
| --- | --- |
| q3 | `H=λh_g+μh_h>0` et `3H²>4|λC_g+μC_h|²` |
| q4 | `H>0` et `H²>2|λC_g+μC_h|²` |

En effet, la somme `λP_g(t)+μP_h(t)` a pour minimum sur le disque de
centres admissibles `H/4−ρ|proj_(d⊥)(λw_g+μw_h)|`. Si elle est
strictement positive, au moins un des deux sites est intérieur à tout
centre. À K5, quatre paires disjointes en IDs ferment q3 et trois
ferment q4 ; les deux voies ne cumulent jamais leurs crédits. La
propriété « `ab` est la plus longue arête propriétaire » reste
indispensable aux disques q3/q4. Les égalités ne créditent rien.

Pour un rectangle `A×B`, **le même rapport `λ:μ`** est testé aux
64 couples de coins de `box(A)×box(B)`. À poids fixés, `H` et `C`
sont affines séparément en `a,b`; `H>α|C|` définit un cône convexe
ouvert. Ces 64 tests certifient donc chaque arête réelle du produit.
Choisir un rapport différent à chaque coin serait invalide. Les poids
de la petite palette ci-dessous totalisent au plus cinq ; les bornes
u18/i128 du certificat de groupes de B s'appliquent après promotion
avant produit. L'optimisation *libre* des poids ne bénéficie pas de
cette borne i128 sans une nouvelle analyse de bit-width.

La [fixture arithmétique](verify_math.py) a
`A=[(3,10,10),(4,10,10)]`, `B=[(17,10,10),(18,10,10)]`,
`g=(10,5,13)`, `h=(15,14,8)`. Les deux gardes sont strictement dans
les disques diamétraux aux quatre couples distincts de coins et
aucun singleton ne passe q3. Au coin `a=(4,10,10),b=(17,10,10)`,
la paire 1:1 échoue q3/q4 (`H=40`, `F3=−608`, `F4=−1104`). Le
rapport **2:3** passe q4 sur les 64 tests, avec minima `H=88` et
`F4=2336`; il passe donc aussi q3. La pondération augmente réellement
le pouvoir d'une paire uniforme sur boîte.

Pour une arête *fixe*, normaliser `λ:μ=t:(1−t)` :
`F_q(t)=κ(h_h+t(h_g−h_h))²−β|C_h+t(C_g−C_h)|²` est un polynôme
quadratique rationnel. Sur `0≤t≤1` et `H(t)≥0`, son maximum est à
une borne ou à son sommet si la parabole est concave. Un maximum
strictement positif donne un rapport rationnel **positif** par
continuité, même si la borne gagnante a un poids nul. Le lecteur exact
avec `Fraction` trouve **zéro paire pondérable** parmi les 66 paires
de la [fixture des triples de B](../CERTIFICAT_B_GROUPES_GARDES_Q34_20260923.md) :
les meilleurs `F3` et `F4` sont respectivement
`−89220768/5191` et `−6598732/649`. La preuve des triples conserve
donc un pouvoir propre, y compris face aux paires pondérées libres.
Pour une boîte, les 64 ensembles de rapports admissibles sont des
intervalles ouverts dont il faut trouver l'intersection **commune** ;
cette recherche exacte n'est pas évaluée ici.

## Ablation sur les rectangles LiDAR réels

Le [probe B épinglé](../rect_pair_shadow_b_20260923/README.md) fournit
la même palette : quatre recherches indexées par représentant de
rectangle, chacune bornée à 128 nœuds et huit gardes au plus. On garde
son appariement maximum 1:1. Pour chaque paire encore non certifiée
dans les deux voies, on essaie dans l'ordre `1:2`, `2:1`, `2:3`,
`3:2` au représentant puis, si une voie reste possible, aux 64 coins.
Le repli après échec est inchangé. Le [patch](probe.patch) archive
exactement ce delta sur la source B de SHA
`7f4f2c4e50d0b9e4e3261288ad582abaffa3d9e971466d30a9878bb56c6c589c`.

Cas unique : **08/000000 brut entier**, 123 389 sites, grille
1 mm/u18, K5/s8. Le front laisse 1 747 rectangles ouverts de produit
au moins 1 024, dont 299 avec survivantes S2. Les comptes de base
retrouvent ceux de B. `F` est la somme des formes diamétrales après S2,
potentiellement évitables si le rejet est intégré avant le cœur.

| Sur les 299 rectangles S2-positifs | Paires 1:1 | Avec quatre rapports supplémentaires |
| --- | ---: | ---: |
| Rectangles entièrement fermés | 72 | **87** |
| Arêtes S2 dans ces rectangles | 2 175 | **3 887** |
| `F` dans ces rectangles | 5 059 809 | **10 693 497** |

Les **15** fermetures nouvelles représentent 1 712 arêtes et
**5 633 688 `F`**, soit **1,007 %** des 559 661 741 formes de la
trame. L'ensemble 1:1+pondéré représente **1,911 %** du `F`
global. Sur les 1 747 grands rectangles, 85 fermetures nouvelles
supplémentaires portent un segment S2 déjà vide : elles ne réduisent
pas ce `F`. Les [deux fermetures par triples de
B](../rect_guard_triples_b_20260923/RESULTS.json) se recoupent en
`1790368` (`F=44 046`) ; `2938279` (`F=9 477`) reste propre aux
triples. L'union des trois essais ferme ainsi 88 rectangles positifs
et porte `F=10 702 974` dans **cette palette et cette trame**.

Le coût des échecs est décisif. Sur **tous** les 1 747 grands
rectangles tentés avant de savoir s'ils survivent à S2, la phase
pondérée paie **2 242 967** rapports au représentant,
**58 085 846** tests de coins et **1,476 s** en somme d'intervalles
locaux `steady_clock` sur le thread mono, en sus de la sélection et
des paires de base. Si on la
déclenche seulement après l'échec de l'appariement 1:1 sur les
696 rectangles concernés, la somme de leurs mesures vaut encore
**1 088 739** rapports, **23 060 495** coins et **0,583 s**
d'intervalles locaux ;
ce dernier total est une comptabilité conditionnelle du même run,
pas un nouveau pipeline mesuré. Déclencher sur les seuls 227
rectangles S2-positifs serait un oracle aval, indisponible au front.
La phase teste aussi les voies inactives, donc ce coût n'est ni une
borne inférieure ni celui d'un port optimisé. Aucun gain net de
durée, aucun contrat G4 et aucun comportement sous-quadratique
ne découlent de ce shadow.

## Preuve conservée et rejeu

[`CERTIFICATES.tsv`](CERTIFICATES.tsv) conserve pour les 15 succès
nouveaux les boîtes, masques, **105 paires de matching** q3/q4,
indices dans le nuage préparé (identiques aux positions d'entrée),
coordonnées et rapports. Le [lecteur autonome](verify_certs.py)
recalcule **6 720 inégalités strictes** aux coins, exige 4/3 paires
disjointes par voie et vérifie que chaque garde est hors des deux
boîtes de facteurs. Il peut aussi recouper les indices, coordonnées
et IDs bruts avec les deux fichiers d'entrée. Les rapports de q3 et q4
peuvent différer pour une même paire ; le lecteur ne les additionne
pas. Cela établit les certificats géométriques de ces 15 fermetures,
pas l'exhaustivité des échecs ou la provenance WSPD des boîtes sans
rejeu du front. Une mutation d'une coordonnée de garde dans l'archive
fait échouer le lecteur sur l'inégalité stricte.

[`RESULTS.json`](RESULTS.json) et [`MATH.json`](MATH.json) sont lus
en modes normal et `python -O` par [`analyze.py`](analyze.py),
[`verify_math.py`](verify_math.py) et `verify_certs.py`, sans `assert`
désactivable. La sortie native complète reste dans `/tmp`; les
résultats non temporels et les certificats sont figés ici. Le reçu
[`INPUT_SHA256SUMS`](INPUT_SHA256SUMS) épingle les coordonnées,
IDs, huit traces S2 et l'archive produit ; ils ne sont pas versionnés
dans ce dossier. Le binaire local initial a SHA
`6a5634369b9e18e7695a535f03bc3439a62e5039522a3dab328d9dbd15ea933e`,
la source après patch
`b6f96ddad461ab49d9780cd67b64a177542b06eb3b3f994d93ceb50d705e3143`.
Le [replay complet](replay.sh) part d'un objet Git fixé, contrôle les
SHA, recompile, reconstruit le front et joint les traces S2 épinglées,
compare les comptes hors
durées, compare les certificats octet pour octet et relit leurs
coordonnées contre l'entrée. Il requiert les dépendances LIVE sous
`/tmp` et `build`, comme le probe B.

Une suite utile serait de tester un **rapport rationnel commun dans
l'intersection des 64 intervalles** avec coût numérique borné, puis
une porte conditionnelle sur les échecs de paires. Il faut mesurer
sélection, coins, matching, replis et chaîne de bout en bout sur les
moitiés/quarts physiques et densités emboîtées, brut et sans sol,
K5/K10. Une palette meilleure peut augmenter les fermetures ; le
coût sur **toutes** les tentatives doit rester inférieur à ce qu'elle
supprime réellement du cœur et de l'aval.
