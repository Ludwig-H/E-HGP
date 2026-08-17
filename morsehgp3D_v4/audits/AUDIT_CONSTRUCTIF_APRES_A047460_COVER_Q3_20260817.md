# Audit constructif après `a047460` — q3 exact, cover partagé et prochaine primitive

Date : 17 août 2026.  
Pin de code audité : **`a047460083331c9998609a5f22d64e71257d49a3` inclus**.  
Dernier pin déjà couvert par l'harmonisation précédente : `dac7085ed0e43d2b98f6e534e26424243e7a9e59`.

Commits nouveaux examinés :

| commit | nature | objet |
|---|---|---|
| `e45a683` | contre-audit | contrats q3, petits n, vrais IDs, point fixe médiateur, porte de bord |
| `2d26e7a` | Claude | IDs dans q3, exact-once, `h_a/h_b`, mode exact |
| `79dc862` | reçu | gain de `h_a/h_b` sur l'instruction q3 |
| `a047460` | Claude + reçu | cover partagé `B(m,√3D/2)`, census trié, gain 10× |

Cadre : `phase=exploration_v4_hors_registre`, `public_status=not_claimed`.
Cette réception est mathématique et statique, confrontée aux reçus versionnés.
Aucun statut CI GitHub n'est publié au pin ; je ne transforme donc pas les
`20/20 CTest` annoncés par Claude en rejeu indépendant.

---

## 0. Verdict exécutif

Le cours pris est excellent. Claude a appliqué les raccords demandés, puis a
fait tomber le coût q3 de `eight_clusters,n=2000` de **475 s à 24,2 s** en deux
étapes conservant la même géométrie :

1. `h_a/h_b` élimine 17 % des ancres, mais 47 % du temps ;
2. un cover partagé et ordonné remplace une descente d'arbre par porteur,
   donnant encore un facteur 10,5.

Ce résultat reçoit pleinement l'intuition architecturale : les longues ancres
inter-amas coûtaient beaucoup, puis chaque porteur repayait le même domaine de
sites. Les deux redondances sont désormais visibles et attaquées dans le bon
ordre.

### Reçu mathématiquement

1. Le déplacement de `universal_over_corners` dans `spindle.hpp` et son usage
   q3 sont corrects : à `s,z` fixés, le lieu des partenaires est un cône
   convexe ouvert ; tous les coins distincts d'une boîte dans le cône
   impliquent toute la boîte dans le cône.
2. Le filtre `h_cœur+h_a+h_b` branché avant la lentille est fail-open et ne
   peut supprimer aucun support q3 pertinent.
3. Le mode cover de `a047460` est exact : le cover fermé

   ```text
   |2z-a-b|² <= 3 |a-b|²
   ```

   contient tout porteur, tout intérieur et toute coquille de toute
   circum-boule q3 pertinente possédée par l'ancre.
4. L'early-exit à `h_3` est compatible avec le contrat
   `regular_up_to_Kmax` : une boule ayant déjà `h_3` intérieurs ne sert aucune
   forêt demandée, donc sa coquille n'a pas à être certifiée par ce pipeline
   peu profond.
5. La voie `tree` et la voie `cover` utilisent la même autorité ponctuelle
   `q3_power`; leur égalité observée est cohérente avec la preuve du cover.

### Deux qualifications de vocabulaire à conserver

- Le code a maintenant des IDs tirés des buckets, mais l'API amont fabrique
  encore ces IDs depuis l'ordre du vecteur. Ce ne sont donc pas encore des
  **IDs externes stables**.
- Le probe reste un énumérateur de **supports q3 réguliers peu profonds**. Il
  ne publie pas encore un événement HGP complet avec `BallKey`,
  `ExactLevel`, `InteriorIds` et hyperincidence.

### Prochaine primitive recommandée

Avant un arrangement de droites complet, réutiliser dans le census les
**témoins déjà certifiés** par `h_cœur/h_a/h_b`, puis transposer le scan :

```text
actuel      : pour chaque porteur x, parcourir le cover z ;
prochaine   : pour chaque site z, mettre à jour tous les porteurs actifs.
```

Ensuite seulement, ajouter l'arbre temporaire des centres et les `range-add`.
Cette progression donne une baseline exacte, vectorisable et GPU-friendly à
chaque étape, au lieu de sauter directement vers une structure plus savante
parce que les humains aiment beaucoup résoudre le problème suivant avant de
réutiliser le résultat précédent.

---

## 1. Contre-audit du complément `e45a683`

Le complément `COMPLEMENT_CONTRE_AUDIT_DAC7085_20260817.md` est globalement
juste et utile.

### 1.1 Régularité pertinente pour `K_max`

Je confirme la bonne portée : pour une boule-support d'arité `q`, profondeur
`d`, l'ordre est

```text
K = q + d - 1.
```

Elle est pertinente pour la sortie tronquée à `K_max` exactement lorsque

```text
d < h_q = K_max-q+2.
```

Le pipeline peu profond peut donc certifier `regular_up_to_Kmax`, pas la
position générale globale du nuage. Une boule atteignant `h_q` peut être
abandonnée sans chercher ses ex æquo ; une boule restant sous le seuil doit
finir son census et refuser tout extra-shell.

### 1.2 Petits nuages

La critique reste ouverte au pin `a047460` : `q3_events_probe` exige encore
`n>=4` et `smax_eff>=5`, alors qu'un triangle aigu isolé à `n=3` est déjà un
support q3 de profondeur zéro, avec `h_3=1`.

Il faut passer à un masque dynamique :

```text
lane q active <=> q <= smax_eff,
h_q = smax_eff-q+1.
```

Les fixtures minimales `n=2/3/4` sont prioritaires, car les petits oracles sont
précisément ceux censés porter la vérité.

### 1.3 API d'identités

Le complément a raison :

```text
pid(u)=bucket_ids[bucket_start[u]]
```

ne suffit que si le bucket contient déjà un ID externe. L'overload actuel
`build_cloud_index(vector<P3>)` crée `PointId=i`, donc une permutation physique
des records renomme le nuage.

La vraie API doit être :

```cpp
struct InputPoint {
  PointId id;
  P3 position;
};
```

avec unicité des IDs vérifiée, tri spatial sans réécriture de `id`, et
l'overload `vector<P3>` explicitement réservé aux benchmarks sous le statut
`generated_ids_not_stable`.

### 1.4 Point fixe pour le census médiateur

La construction proposée est saine. Pour une ancre `d=b-a`, un porteur q3
produit

```text
T = 2c-a-b = (W-Gd)/G = N/G.
```

Avec `S=2^32`, les intervalles dirigés

```text
Tlo_i=floor(SN_i/G),
Thi_i=ceil (SN_i/G)
```

permettent de classifier un site par une forme affine sur une AABB de centres.
La borne de largeur doit toutefois invoquer explicitement la géométrie, pas
seulement les numérateurs : sous arête maximale et triangle aigu,

```text
|T| <= |a-b|/sqrt(3),
```

si bien que les coordonnées fixes tiennent en moins de 49 bits. En parallèle,
`|N_i|<2^86` et `S|N_i|<2^118`, donc la division dirigée tient en i128 avant
le cast contrôlé. La forme

```text
E_z(Ts)=S(|u_z|²-D²)-2u_z·Ts
```

tient largement en i128.

Conclusion : le chemin **point fixe dirigé** peut viser i128 ; des
comparaisons rationnelles non normalisées pourront demander i192. Le reçu de
`a047460` ne doit donc pas annoncer « l'arithmétique i192 des `T_x` prouvée »
comme une décision déjà close. Il faut nommer le prédicat exact visé, puis
prouver sa largeur.

### 1.5 Porte de bord pondérée

La pondération des ancres dont la boule diamétrale est entièrement dans le
cube est correcte. À longueur `r` fixée, les milieux admissibles ont volume
`(L-r)^3`; le poids `(L/(L-r))^3` annule ce facteur et, puisque
`W_q⊂B(m,r/2)`, aucun fuseau n'est tronqué. C'est une excellente porte proche
du générateur cubique actuel.

---

## 2. Audit du commit `2d26e7a`

### 2.1 `h_a/h_b` : correction reçue

Pour `a,z` fixés, écrivons `e=z-a` et `t=b-z`. Les conditions q3/q4 sont

```text
H=e·t>0,
q3 : 4H²>|e|²|t|²,
q4 : 3H²>|e|²|t|².
```

Dans la variable `t`, ce sont les intérieurs de cônes de Lorentz strictement
convexes. Une AABB est l'enveloppe convexe de ses coins distincts ; la fonction
`universal_over_corners` est donc une autorité exacte sur l'enveloppe continue.
La symétrie en `(a,b)` donne `h_b`.

La disjonction est structurelle :

- cœur hors `A∪B` ;
- `h_a` dans `A\{a}` ;
- `h_b` dans `B\{b}` ;
- `A` et `B` sont disjoints dans un rectangle CK.

Le test

```text
h_a(a)+h_b(b) >= h_3-h_cœur
```

est donc sûr.

### 2.2 Gain des lanes de rayon nul

Dans `count_universal_witnesses_234(...,with_corners=false)`, la boule est la
seule voie q3/q4. Retirer du masque une lane de rayon nul évite un parcours qui
ne pouvait créditer aucun point. Le caller conserve la lane vivante puisque le
compteur rendu reste zéro : optimisation sûre.

### 2.3 Exact-once visible

`raw_events-events_unique` et la porte `doublons=0` sont une amélioration
réelle. Le `sort/unique` ne peut plus réparer silencieusement une double
émission.

### 2.4 Quatre raccords encore ouverts

#### A. `Key3` n'est pas encore un `SupportKey<PointId>`

`Key3` stocke trois `i32`, alors que `PointId` est `u32`. Les casts

```cpp
(i32)pid(u)
```

changent l'ordre canonique dès que le bit 31 est posé. Le projet vise certes
des dizaines de millions de points, mais son contrat est u32. Utiliser
`PointId u[3]` et trier en ordre non signé ; ajouter les IDs
`{0,0x80000000,0xffffffff}` comme fixture de codec.

#### B. L'oracle ne juge pas encore l'identité de l'owner

Dans la boucle brute, les arêtes candidates sont encore construites avec

```cpp
(PointId)i, (PointId)j, (PointId)k
```

c'est-à-dire les rangs Morton, alors que le sujet départage avec `pid(i)`.
La clé finale du support est convertie en vrais IDs, mais l'arête owner choisie
par le juge ne l'est pas.

Cela n'affecte généralement pas l'ensemble des triplets : deux owners égaux en
longueur donnent la même circum-boule et le même `SupportKey`. Le juge 0/0 peut
donc rester vert tout en ne testant pas la convention d'owner annoncée.

Il faut faire retourner au sujet et au juge :

```text
(SupportKey, OwnerEdgeKey)
```

sur les fixtures à égalités.

Fixture immédiate :

```text
a=(0,0,0), b=(1,1,0), x=(1,0,1)
```

triangle équilatéral entier en distances carrées, puis plusieurs permutations
d'IDs externes. La plus petite `EdgeKey` doit gagner dans le sujet **et** dans
l'oracle.

#### C. Le mode exact n'a pas de porte CTest

Les deux CTests q3 utilisent le défaut `regular_subset_diagnostic`; aucune
porte n'exécute `--exact`. Il faut une fixture cosphérique déterministe où un
support peu profond possède un extra-shell, avec :

- statut `unsupported_degeneracy` ;
- aucun payload ou reçu de succès ;
- `first_shell_witness` publié ;
- mutant « oublier la coquille » tué.

Le code de sortie `2` est aujourd'hui décrit ailleurs comme « refus avant
calcul », alors que la dégénérescence est découverte après instruction. Le
statut typé doit être l'autorité ; la convention numérique du probe doit être
clarifiée au lieu de faire semblant que le calcul n'a pas eu lieu.

#### D. Le filtre n'utilise pas encore son histogramme pour l'énumération

La décision est correcte, mais le probe parcourt encore tout `A×B` et teste la
somme. Comme chaque ancre survivante doit de toute façon être instruite, ce
n'est pas le verrou actuel. Pour garder le contrat annoncé
`O(|A|+|B|+#survivantes)`, ranger néanmoins les `b` par valeur saturée de
`h_b` et n'émettre que les seaux admissibles pour chaque `a`.

---

## 3. Audit du reçu `79dc862`

Le résultat est très positif :

```text
eight_clusters n=2000 :
  ancres supprimées      17,0 %,
  porteurs supprimés     32,9 %,
  temps supprimé         46,7 %.
```

Il confirme que `h_a/h_b` cible les longues ancres inter-amas, comme prédit.
La géométrie donne précisément ce comportement : le milieu est vide, mais les
amas proches des extrémités fournissent des témoins universels d'extrémité.

Pour en faire une porte durable, ajouter une ablation **dans le même binaire** :

```text
--ha=off | --ha=on | --ha=compare
```

et publier sur la même exécution :

```text
support_digest,
lens_queries,
lens_tree_nodes,
carriers_tested,
q3_depth_nodes,
q3_power_tests.
```

Le temps apparié est convaincant ; ces compteurs rendront l'explication
causale indépendante de la machine et des caches.

---

## 4. Audit du cover partagé `a047460`

### 4.1 Théorème du cover

Soit `D=|a-b|` et `C` l'angle du triangle opposé à l'arête maximale `ab`.
Pour un triangle aigu possédé par `ab`,

```text
pi/3 <= C < pi/2.
```

Son circumrayon et la distance du circumcentre au milieu `m` de `ab` valent

```text
R       = D/(2 sin C),
delta   = D/(2 tan C).
```

Tout point de la circum-boule fermée vérifie alors

```text
|z-m| <= R+delta
       = D(1+cos C)/(2 sin C)
       = (D/2) cot(C/2)
       <= (sqrt(3)/2) D.
```

L'égalité a lieu pour `C=pi/3`, donc la constante est sharp.

En unités entières :

```text
|2z-a-b|² <= 3 |a-b|².
```

Cette inclusion couvre :

- tous les porteurs de la lentille ;
- tous les intérieurs stricts ;
- toute coquille externe pertinente.

Le scan du cover est donc un census exact pour chaque porteur possédé.

### 4.2 Fixture de sharpness gratuite

Le triangle entier

```text
a=(0,0,0), b=(1,1,0), x=(1,0,1)
```

vérifie les six égalités d'un triangle équilatéral en distances carrées, et

```text
|2x-a-b|² = 3 |a-b|².
```

Le porteur `x` est exactement sur le bord du cover. Une porte doit tuer les
mutants :

```text
<= devient <,
bound=3D² devient 3D²-1.
```

### 4.3 Arithmétique du `cover_query`

Sous u16 :

```text
D² < 2^34,
3D² < 2^36,
|2z-a-b| <= 131070,
somme des trois carrés < 2^36.
```

Les `i64` employés sont donc sûrs. La borne de boîte est un minorant exact sur
l'enveloppe continue, éventuellement un peu lâche à cause de la parité ; cette
lâcheté ne peut qu'ajouter du travail.

### 4.4 Early-exit et coquilles

Pour une boule dont la profondeur atteint `h_3`, sortir tôt est correct : elle
est hors sortie demandée. Pour une boule survivante, le scan parcourt tout le
cover et voit donc tous les zéros de puissance. L'ordre radial n'influence que
le coût, jamais le verdict.

### 4.5 Ce que le reçu établit exactement

Le facteur 10,5 est un reçu de performance très fort. En revanche, la phrase
« le poste dominant devient la collecte du cover » reste une hypothèse
plausible, pas encore un résultat causal : le probe publie `cover_pts`, mais
pas le nombre de `q3_power` évalués ni les temps séparés requête/tri/scan.

Ajouter :

```text
cover_tree_nodes,
t_cover_query,
t_cover_sort,
power_tests,
carriers_rejected_at_h,
carriers_full_scan,
mean_tests_per_carrier,
t_power_scan.
```

Ces nombres décideront objectivement entre trois optimisations : seaux
radiaux, transposition du scan, arbre de centres.

### 4.6 Parité `tree|cover`

À petite taille, les deux voies jugées 0/0 contre la vérité donnent une parité
forte. À `n=2000`, « mêmes événements » doit être reçu par digest de la liste
triée, pas par cardinal seul. Ajouter un mode interne :

```text
--census=compare
```

qui construit les deux ensembles et exige :

```text
SupportKey_digest_tree == SupportKey_digest_cover,
duplicate_supports_tree == duplicate_supports_cover == 0.
```

Le CMake actuel n'exécute que la voie par défaut `cover`; garder au moins une
porte permanente `tree` et une porte `compare`.

---

## 5. Nouvelle optimisation prioritaire : consommer les témoins déjà prouvés

Le préfiltre et le census prouvent aujourd'hui deux fois la même chose.
Pour un rectangle vivant et une ancre survivante `(a,b)`, poser

```text
base(a,b)=h_cœur+h_a(a)+h_b(b) < h_3.
```

Chaque témoin ainsi compté appartient à `W_3(a,b)`, donc à l'intérieur de
**toute** circum-boule q3 admissible passant par `a,b`, notamment celle de
chaque porteur possédé `x`.

### 5.1 Paquets d'identités bornés

Comme `base<h_3<=9`, il suffit de conserver au plus huit IDs :

```cpp
struct Q3WitnessPacket {
  u8 count;
  PointId ids[8];
};
```

ou, durant la descente, de petits handles de plages `ALL` qui ne sont expansés
que pour un rectangle vivant.

Les trois paquets sont disjoints par théorème :

```text
core_ids  subset X\(A union B),
ha_ids(a) subset A\{a},
hb_ids(b) subset B\{b}.
```

### 5.2 Census initialisé

Pour chaque porteur :

```text
depth = base(a,b),
scan uniquement cover \ packet_ids,
arrêt à h_3.
```

Il n'y a ni double compte, ni risque de coquille cachée : les IDs du paquet
sont certifiés **strictement** intérieurs. Cette optimisation :

1. réduit le nombre d'intérieurs supplémentaires nécessaires ;
2. évite de rechercher les mêmes témoins pour chaque porteur ;
3. fournit déjà une partie de `InteriorIds` pour l'événement complet ;
4. raccorde enfin le préfiltre au payload, au lieu de jeter ses certificats
   après avoir pris une décision booléenne.

### 5.3 Porte causale

Comparer `packet=off|on` sur les mêmes ancres et exiger :

```text
mêmes SupportKey,
mêmes profondeurs exactes,
mêmes InteriorIds après tri,
aucun ID répété entre les trois paquets,
power_tests_packet_on <= power_tests_packet_off.
```

Mutants : oublier d'exclure un ID du scan, autoriser un shell dans le paquet,
ou fusionner deux identités de même position.

---

## 6. Deuxième étape : transposer le scan avant l'arbre de centres

Le code actuel est carrier-major :

```cpp
for (carrier x)
  for (site z : cover)
    q3_power(x,z);
```

Construire d'abord tous les porteurs possédés et leurs `Q3Form`, puis écrire :

```cpp
for (site z : cover_in_radial_order)
  for (carrier x encore actif)
    q3_power(x,z), saturation et masque;
```

Le nombre de prédicats ne peut qu'être identique à politique d'ordre égale,
mais :

- chaque site est chargé une fois ;
- les formes des porteurs sont en SoA ;
- les masques saturés éliminent immédiatement les lanes finies ;
- CPU SIMD et GPU « un CTA par ancre, lanes = porteurs » deviennent naturels ;
- la collecte des shells et intérieurs des survivants se branche directement.

C'est la baseline exacte à mesurer avant le BVH de centres.

### 6.1 Le tri total n'est pas contractuel

L'ordre radial est une heuristique de débit. Il peut être remplacé par 16 ou
32 seaux stables selon

```text
bin = floor(B * dist2q / (3D²+1)).
```

Un counting sort `O(|cover|+B)` évite `std::sort` par ancre. Aucune preuve
scientifique ne dépend de l'ordre ; comparer les événements bit à bit suffit.

### 6.2 Puis arbre de centres et `range-add`

Une fois la baseline site-major reçue, construire l'arbre temporaire des
centres fixes `T_x`. Pour un site `z`, la forme `ell_z(T)` est affine :

```text
max_box ell_z < 0  => range-add interior,
min_box ell_z > 0  => prune,
sinon              => split.
```

Le point corrélé `x` d'un porteur vérifie `ell_x(T_x)=0`; puisque l'AABB
contient `T_x`, aucun `ALL_INTERIOR` strict ne peut compter silencieusement le
porteur comme son propre témoin.

Commencer avec une liste plate pour les petites ancres et le BVH seulement
au-dessus d'un seuil mesuré. Construire un arbre pour trois porteurs serait une
façon assez humaine de rendre une optimisation plus chère que son problème.

---

## 7. Portes permanentes à ajouter maintenant

### Identités et owner

1. `q3_external_ids_permutation` : mêmes records permutés, IDs conservés,
   mêmes `(SupportKey,OwnerEdgeKey)`.
2. `q3_key_u32_highbit` : IDs `0`, `0x80000000`, `0xffffffff`.
3. `q3_owner_equal_edges` : triangle régulier entier, plusieurs affectations
   d'IDs.

### Cover

4. `q3_cover_sharp_regular_triangle` : égalité `dist2q=3D²`.
5. mutant `cover-strict` ou `cover-minus-one` tué.
6. `q3_census_tree_cover_compare` : digests d'identités égaux.
7. `q3_cover_shell_boundary` : une coquille exactement sur la frontière du
   cover reste détectée.

### Statuts

8. `q3_exact_extra_shell` : statut transactionnel sans publication.
9. `q3_small_n3` : triangle aigu isolé, profondeur zéro.
10. `q3_regular_scope` : une coquille sur une boule déjà profonde ne bloque
    pas `regular_up_to_Kmax`, mais bloque le vérificateur global séparé.

### Raccord préfiltre-census

11. `q3_witness_packet_disjoint`.
12. `q3_witness_packet_no_double_count`.
13. `q3_witness_packet_same_interior_ids`.

---

## 8. Ordre de travail recommandé à Claude

### P0 — fermer les contrats d'identité et les portes du nouveau chemin

1. `Key3<PointId>` non signé ;
2. oracle owner en vrais IDs, fixture égalitaire ;
3. `--census=compare` + digest ;
4. fixture exacte d'extra-shell ;
5. masque dynamique des petites tailles.

### P1 — réutiliser les certificats déjà payés

6. paquets bornés `core/ha/hb` ;
7. initialisation du census et collecte `InteriorIds` ;
8. compteurs causaux requête/tri/power-tests.

### P2 — baseline GPU-friendly avant structure sophistiquée

9. carriers en SoA ;
10. scan site-major avec masques saturés ;
11. seaux radiaux au lieu du tri total.

### P3 — census médiateur collectif

12. intervalles fixes dirigés de `T_x` ;
13. flat path pour petites ancres ;
14. LBVH de centres + `range-add` pour grandes ancres ;
15. parité avec le scan plat actuel.

### P4 — événement complet et vérité indépendante

16. `BallKey`, `ExactLevel`, profondeur et `InteriorIds` ;
17. micro-oracle rationnel indépendant avant q4 ;
18. hyperincidences et facettes `F_K^conn/F_K^render`.

La WSPD à cellules de préfixe exactes et la porte de bord restent importantes,
mais elles ne doivent pas interrompre cette séquence q3 : le verrou dominant
vient de passer de « une descente par porteur » à un problème collectif local
par ancre, et le code dispose maintenant de toutes les quantités nécessaires
pour le résoudre proprement.

---

## Conclusion

Les commits récents sont une réussite. Le filtre d'extrémité et le cover
partagé ne sont pas des micro-optimisations : ils retirent ensemble environ
95 % du temps q3 observé sur la famille adversariale, sans changer l'objet
selon les juges disponibles.

Le prochain gain le plus sûr consiste à **ne pas oublier les preuves déjà
calculées** : transporter les quelques IDs universels dans le census, puis
faire circuler chaque site devant tous les porteurs actifs. Ce raccord donne en
même temps la vitesse, les `InteriorIds` nécessaires au véritable événement
HGP et une architecture CUDA simple. L'arbre de centres viendra ensuite comme
accélérateur reçu contre cette baseline, pas comme nouvelle vérité.
