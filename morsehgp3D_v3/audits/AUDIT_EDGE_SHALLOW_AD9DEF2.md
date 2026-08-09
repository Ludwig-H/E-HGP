# Audit intégral de `edge_shallow` au commit `ad9def2`

Date de l'audit : 2026-08-09  
Périmètre principal : commit `ad9def2258c91e61f26a674de10f1850017f1a35`  
Mode : audit seul ; builds, mutations hostiles et probes exclusivement sous `/tmp` ; aucune mutation Git ou GCP.

## Verdict exécutif

**NO-GO pour qualifier M2.2, Gate E, PEL-2 ou le contrat 50 000 points / ordre 10 / une seconde à partir de ce commit.**

Le résultat positif important est plus étroit : je n'ai pas trouvé de contre-exemple à l'identité algébrique `rang = 4 + c + profondeur` pour un support quatre en `RelevantGP`. Le signe strict, le retournement lorsque le déterminant est négatif, les formes véritablement constantes sur le plan entier et le rejet des parallèles sont cohérents. Une campagne complémentaire non permanente allant jusqu'au rang 6 est également verte contre l'oracle indépendant.

En revanche, les affirmations « dictionnaire vérifié », « A2e » et « coût de parcours shallow » ne sont pas établies par le code ou le CTest livré :

1. le CTest annoncé comme 20 nuages d'ordre 3 ne contient que **cinq** tirages d'ordre 3, tous avec `s_max=4`, `constant_inside=0` et donc aucun support émis de profondeur positive ;
2. le code ne vérifie ni que l'ancre est une arête de longueur maximale, ni le clipping par l'ellipse de Jung, ni l'owner diamétral ; il émet le même support depuis les six arêtes puis déduplique après le sink ;
3. `constant_inside`, `lines_active`, `vertices_shallow`, `edges_retained` et `emitted_arity_four` ne mesurent pas les quantités A2e définies par la proposition ; des probes exacts produisent des constantes clippées classées actives, des faux sommets hors ellipse et des arêtes non diamétrales dites retenues ;
4. le chemin reste dense : il forme toutes les paires de droites, balaie les témoins et les membres, puis exécute en plus le catalogue ancré exhaustif jusqu'à l'arité quatre avant d'en jeter les résultats d'arité quatre ;
5. le reçu du commit étiquette `edge_shallow` comme `anchored_catalogue`, omet toutes les statistiques de profondeur et est sérialisé avant la postcondition `dictionary_refuted`. Un probe hostile donne un reçu apparemment vert (`identity_closed=true`, `failures=0`) alors que le processus sort avec le code 1.

Le vert du catalogue final sur les échantillons testés demeure réel : l'oracle compare supports, rangs, membres, centres et niveaux exacts. Les findings ci-dessus portent sur la portée de la qualification, l'objet A2e réellement construit, ses compteurs, son reçu et son coût ; ils ne doivent pas être reformulés comme un contre-exemple au catalogue final en position générale.

## 1. Snapshot et empreintes

À l'ouverture de l'audit, `HEAD` valait exactement :

```text
ad9def2258c91e61f26a674de10f1850017f1a35
```

L'arbre de travail était concurrent : `edge_shallow.hpp` et `CMakeLists.txt` étaient propres au commit, tandis que `oracle_main.cpp` était déjà modifié. Son SHA-256 de travail capturé était `c93e313f3611079eaaec84cf91394fca19eccf184d30de5609e32d9d19558c85`. L'audit normatif ci-dessous emploie donc un `git archive` **explicite de `ad9def2258c91e61f26a674de10f1850017f1a35`**, jamais le `HEAD` mouvant ni l'oracle concurrent.

| fichier au commit audité | blob Git | SHA-256 |
| --- | --- | --- |
| `morsehgp3D_v3/prototype/edge_shallow.hpp` | `b9007610e6d20b5f2a0909063d5e20ba9d1ebb46` | `badc8100a700669b56accd68f3362fdef2517d8b4e63ce0b3a303ff29b0f0627` |
| `morsehgp3D_v3/prototype/anchored_catalogue.hpp` | `3ba83a0ce9cdec6cc916ecd33692601f6f311ca6` | `28b18507fc702cabedf0194dd0db26da23c385d9ffa70338b4bb28875cbd52cf` |
| `morsehgp3D_v3/oracle/oracle_main.cpp` | `a59b2bf4d7efa39ac9c228a0dcfd9658c629f6f1` | `0fe9a4de4633562de0e207b6c66855239e8c07ba12e97dc652d56c273a0b03c9` |
| `morsehgp3D_v3/CMakeLists.txt` | `02c81fc270285dd6958b3ac3ea663d193e700c40` | `720984024e19566b7dfbacb4a6a09d71d9a4ee276cf880ff417869990858c269` |
| `morsehgp3D_v3/README.md` | `a2dc083bccc46c824443d008750ae02c6fa17842` | `90272a87591f0bb244a3370051344ee9a80cdcbb7bd2e33606efdf900cac945a` |
| `morsehgp3D_v3/PROPOSITION.md` | `9dd1edc6943e2c5b3c5c437b92435c82bee410c9` | `615935ad798ce5afb3eb3280a54a3bfd8306eed9d7570ff474866c7a3255d912` |
| `morsehgp3D_v2/include/mhgp/exact.hpp` | `de345e25676642ffa00a179a98f0b301a30d1f43` | `72b93c0c11ad80326265d43f7692e40ed0cfbfaf61d52e3f3d344c721bb74796` |
| `morsehgp3D_v2/include/mhgp/sphere.hpp` | `a8e3b7b84169d629b6cbdb02d6913ca0355d445a` | `cdc6e98735833286a460a8d04c738a9059621fa9c6ca373493424e322164584a` |
| `morsehgp3D_v2/include/mhgp/mhgp.hpp` | `504f3e35db13a8223bd0cff1b7186758d0c34309` | `4c788f0a6d087859e8910cde0b3f32d4815ff69a0f8ab63a853d32cbb69e292a` |

Le §2 bis sur 50 000 points a été ajouté après ce snapshot. La réponse de la section 8 se rapporte au texte observé séparément au commit `7094d04cafb592dcf891b4cba8a92a2b5b7cb293`, `README.md` SHA-256 `e33f1020026ab091ad00105bac6acd6bd2cbbfbdb3aca1f7614e66301573a86a`. Il ne change pas les sources auditées ci-dessus.

## 2. Finding bloquant : le CTest ne vérifie pas le dictionnaire général annoncé

Références au snapshot : `CMakeLists.txt:86-91`, `oracle_main.cpp:1272-1277`, `oracle_main.cpp:1305`, `edge_shallow.hpp:128-161`, `README.md:66-80`.

Le test livré passe :

```text
arete[profondeur] : aretes=924 dont retenues=48 | droites actives=7848 constantes interieures=0 | sommets examines=3996 dont peu profonds=420 | arite4 emise=66 | tests de profondeur=9432 | DICTIONNAIRE REFUTE=0
sujet=edge_shallow | attempted=20 decided=20 rejected_domain=0 | spheres=948 forets=39 noeuds=956 | largeur max=157 bits | grille=[0,65535]
OK : campagne fermee, structure complete comparee sur la grille declaree
```

Mais `--max-order 3` n'impose pas l'ordre 3. L'oracle tire `order = 1 + rng() % maximum_order`. La suite exacte des vingt ordres est :

```text
3 2 1 3 2 1 1 2 2 3 1 2 2 1 3 2 2 3 1 2
```

Il n'y a donc que **cinq** essais A2e effectifs. Pour les quinze autres, `s_max <= 3`, le fichier construit encore les lignes mais quitte à `depth_budget < 0` avant toute intersection.

Pour les cinq essais d'ordre 3, `s_max=4`. Tout support quatre accepté impose alors exactement `constant_inside=0` et `depth=0`. Les 3 996 sommets examinés sont exactement la somme générique des `C(n,2) C(n-2,2)` de ces cinq nuages : aucune branche parallèle ou concurrente n'est exercée. La sortie confirme également `constant_inside=0`. Les 66 émissions avant déduplication correspondent à onze supports génériques vus depuis leurs six arêtes.

Le test exerce des signes positifs pour rejeter des sommets trop profonds, mais il ne compare jamais un **rang émis** 5 ou supérieur obtenu par profondeur. Il ne vérifie aucune constante intérieure positive et aucune égalité. `--min-nodes 200` porte sur toute la forêt, pas sur un plancher d'arité quatre ou de profondeur positive. Aucun plancher ne protège `emitted_arity_four`, `vertices_shallow`, `constant_inside` ou un histogramme des rangs.

Conclusion exacte : ce CTest est non vide pour les supports quatre de rang 4, mais son vert ne qualifie pas le dictionnaire général annoncé par le README.

### Probe complémentaire favorable, non permanent

Une campagne isolée sous `/tmp`, `n=8`, trente nuages, `maximum_order=5`, est verte contre l'oracle :

```text
arete[profondeur] : aretes=840 dont retenues=178 | droites actives=5040 constantes interieures=0 | sommets examines=9240 dont peu profonds=3522 | arite4 emise=234 | tests de profondeur=27468 | DICTIONNAIRE REFUTE=0
sujet=edge_shallow | attempted=30 decided=30 rejected_domain=0 | spheres=1322 forets=94 noeuds=1675 | largeur max=159 bits | grille=[0,65535]
OK : campagne fermee, structure complete comparee sur la grille declaree
```

La distribution des supports quatre uniques du sujet sur exactement ces nuages est `rang4=18`, `rang5=14`, `rang6=7`. Cela renforce l'identité pour les profondeurs actives positives ; ce n'est toujours ni une preuve exhaustive de la grille ni une fixture permanente, et `constant_inside` y reste nul.

## 3. Finding bloquant : le fichier n'implémente pas l'objet A2e décrit

Références : `edge_shallow.hpp:105-124`, `edge_shallow.hpp:132-199`, `edge_shallow.hpp:203-242`, `PROPOSITION.md:284-323`, `PROPOSITION.md:522-535`.

### 3.1 Aucune condition de diamètre maximal et aucun clipping de Jung

La proposition exige, pour le support quatre, une paire de longueur maximale et le clipping exact par l'ellipse de Jung. Le fichier :

- exécute toutes les `C(n,2)` paires (`edge_shallow.hpp:227-232`) ;
- ne compare jamais la longueur de l'ancre aux cinq autres arêtes du support ;
- ne calcule jamais la condition de Jung ;
- traite tout le plan médiateur et toutes les intersections de droites ;
- ne rejette les faux centres que tardivement par `build_sphere` et `well_centered4`.

Fixture exacte :

```text
p0=(4,3,3) p1=(3,1,1) p2=(1,3,1) p3=(1,1,3)
centre=(23/10,23/10,23/10)
barycentriques=(19/50,2/25,27/100,27/100)
d2(01)=9 d2(02)=13 d2(03)=13 d2(12)=8 d2(13)=8 d2(23)=8
```

Le tétraèdre est strictement bien centré. Les arêtes maximales sont `02` et `03`, donc l'owner requis est `02`. Le résultat du probe est :

```text
edge=01 maximal=0 target_emitted=1 jung4=0
edge=02 maximal=1 target_emitted=1 jung4=1
edge=03 maximal=1 target_emitted=1 jung4=1
edge=12 maximal=0 target_emitted=1 jung4=0
edge=13 maximal=0 target_emitted=1 jung4=0
edge=23 maximal=0 target_emitted=1 jung4=0
catalogue edges_retained=6 emitted_raw=6 unique4=1
```

Le catalogue final contient bien l'unique support valide, mais le chemin local a accepté quatre ancres non maximales hors ellipse et a payé six sinks. `edges_retained` ne signifie donc pas « arêtes diamétrales retenues », et `emitted_arity_four` n'est pas une cardinalité de sortie.

### 3.2 L'owner documenté n'existe pas

Le propriétaire exigé par `PROPOSITION.md:524-527` est la plus petite paire lexicographique parmi les arêtes de longueur maximale. Le commentaire `edge_shallow.hpp:203-205` parle au contraire du plus petit identifiant du support, puis le code ne teste aucun owner : il trie les occurrences par support et appelle `unique` **après** toute la géométrie, les rescans et les allocations (`edge_shallow.hpp:234-242`).

Cette déduplication tardive est suffisante pour le catalogue CPU séquentiel actuel, car une quadruple affinement indépendante définit la même sphère depuis ses six arêtes. Elle ne valide ni l'owner A2e, ni le coût, ni une émission concurrente déterministe avant sink.

### 3.3 `constant_inside` et `lines_active` ne sont pas les quantités clippées de la proposition

La proposition classe les formes **sur l'ellipse de Jung** : une droite qui manque l'ellipse peut être intérieure constante ou extérieure constante. Le fichier ne déclare constante qu'une forme algébriquement constante sur le plan entier, `a=b=0`, c'est-à-dire un point collinéaire à l'ancre.

Fixture exacte :

```text
p=(0,0,0) q=(4,0,0) x=(2,1,0)
(a,b,c)=(0,-32,-12)
```

Dans les coordonnées `s` du fichier, la frontière vaut `s2=3/8`. Son énergie de Gram est 36, alors que la borne de Jung vaut 32 : la droite manque l'ellipse et son demi-plan positif contient toute l'ellipse. `x` doit donc contribuer à la constante clippée `c_e`, mais le code le range dans `active`.

Avec deux tels points, `z=(2,1,0)` et `w=(2,0,1)`, `s_max=4`, la vraie classification clippée a `c_e=2` et élimine l'ancre avant arrangement. Le fichier rend :

```text
FALSE_VERTEX clipped_constant_expected=2 code_constant=0 active=2 vertices_shallow=1 emitted=0
```

Le faux sommet hors ellipse est finalement rejeté parce que le tétraèdre n'est pas bien centré. Il reste néanmoins compté comme sommet shallow. Les statistiques `c_e`, `m_e` et `Z_e` du fichier ne peuvent donc pas alimenter Gate D ou une extrapolation PEL-2.

## 4. Finding bloquant : le prototype reste la cascade dense, deux fois

Références : `edge_shallow.hpp:108-126`, `edge_shallow.hpp:132-158`, `edge_shallow.hpp:163-195`, `edge_shallow.hpp:212-232`, `anchored_catalogue.hpp:186-251`.

Le cœur arité quatre :

1. balaie les `n-2` points pour chaque paire d'ancrage ;
2. forme toutes les paires de lignes ;
3. balaie les lignes actives pour chaque intersection jusqu'au dépassement ;
4. alloue un support, reconstruit la sphère et rescane les `n` points pour chaque candidat shallow centré ;
5. déduplique seulement à la fin.

Le chemin hybride appelle auparavant `anchored_catalogue(points, s_max, n, exhaustive, ...)`. Or ce catalogue énumère les tailles **1 à 4**, compte les membres de chaque boule, puis `edge_shallow_catalogue` jette seulement après coup toutes ses sphères d'arité quatre. La phrase « les arités 1 à 3 empruntent le chemin ancré » ne décrit pas le travail réellement payé : l'arité quatre exhaustive est également payée.

Pour un nuage générique de taille `n`, les masses du seul chemin arête sont :

$$A=inom{n}{2},qquad L=inom{n}{2}(n-2),qquad V=inom{n}{2}inom{n-2}{2}=6inom{n}{4}.$$

Le pire cas des tests de profondeur vaut `V(n-4)`, sans compter le second rescan des membres. Le chemin ancré ajoute `sum_{m=1..4} m C(n,m)` candidats et jusqu'à `n` témoins par candidat.

À `n=50 000`, cela donne exactement :

| masse du prototype actuel | valeur |
| --- | ---: |
| ancres | 1 249 975 000 |
| classifications paire--point | 62 496 250 050 000 |
| intersections de lignes | 1 562 312 506 874 925 000 |
| tests de profondeur, pire cas | 78 109 376 093 718 750 300 000 |
| candidats du catalogue ancré redondant | 1 041 604 170 000 000 000 |
| témoins du catalogue ancré, majorant | 52 080 208 500 000 000 000 000 |

Les arrêts précoces réduisent certaines instances ; ils ne changent ni l'architecture ni le pire cas. Tout sommet réellement shallow doit en outre parcourir ses témoins et ses membres faute de transcript de conflits.

Le commentaire « sans allocation » ne vaut que pour la largeur du prédicat. L'implémentation alloue ou réalloue un `vector<Line>` par arête, un `vector<i32>` de support par intersection, un `vector<i32>` de membres par candidat et le flux global `found`. Elle ne fournit aucune mesure de high-water ou de débit réutilisable pour un GPU ou 48 cœurs.

Conclusion : ce fichier est un falsificateur géométrique borné, pas un constructeur shallow, et son temps ne permet aucune extrapolation vers le produit.

## 5. Finding élevé : reçu `edge_shallow` faux-vert et non attribuable

Références : `oracle_main.cpp:1448-1456`, `oracle_main.cpp:1464-1508`, `oracle_main.cpp:1490`, `oracle_main.cpp:1532-1538`, `CMakeLists.txt:89-91`.

Le CTest ne demande aucun reçu. Le seul `receipts/oracle_campaign_20260808.json` commité à ce snapshot porte sur `morsehgp3D_v2`, pas sur `edge_shallow`.

Un reçu propre généré dynamiquement avec `--subject edge_shallow` contient :

```json
{
  "subject": "mhgp3v anchored_catalogue",
  "identity_closed": true,
  "failures": 0
}
```

Il n'inclut ni `edges_examined`, ni `vertices_shallow`, ni `emitted_arity_four`, ni `dictionary_refuted`, ni statut ou code de sortie. Le ternaire de `oracle_main.cpp:1490` étiquette tout sujet non-v2 comme `mhgp3v anchored_catalogue`.

Plus grave, la sérialisation se termine à la ligne 1508, tandis que la postcondition du dictionnaire n'est évaluée qu'aux lignes 1532-1538. Un probe hostile sous `/tmp` a ajouté, après chaque émission valide, un incrément indépendant de `dictionary_refuted` sans modifier le catalogue. Le différentiel reste donc vert mais la postcondition doit rougir. Résultat exact :

```text
DICTIONNAIRE REFUTE=18
sujet=edge_shallow | attempted=1 decided=1 rejected_domain=0 | spheres=53 forets=3 noeuds=64
ECHEC : dictionnaire rang = 4 + c_e + delta_e REFUTE 18 fois
EXIT=1
```

Le reçu déjà écrit dit pourtant :

```json
{
  "subject": "mhgp3v anchored_catalogue",
  "attempted": 1,
  "decided": 1,
  "rejected_domain": 0,
  "identity_closed": true,
  "failures": 0
}
```

C'est un faux vert sérialisé démontré, pas une possibilité théorique. Une correction postérieure au snapshot ne rétroqualifie pas le commit `ad9def2` ni ses affirmations.

## 6. Égalités, parallèles, concurrences, shell et doublons

### 6.1 Parallèles : branche locale correcte

Pour une ancre fixée, le déterminant des deux formes est proportionnel au volume orienté de `(p,q,x,y)`. S'il est nul, les quatre points sont affinement dépendants et ne peuvent porter un support quatre. La branche `determinant == 0 -> continue` est donc correcte pour l'arité quatre.

Probe coplanaire :

```text
(0,0,0) (4,0,0) (0,1,0) (1,1,0)
PARALLEL active=2 vertices_examined=0 emitted=0
```

### 6.2 Concurrences : catalogue protégé, constructeur et compteurs non conformes

Avec cinq coins distincts du cube sur la sphère de centre `(2,2,2)` et rayon carré 3 :

```text
(3,3,3) (3,1,1) (1,3,1) (1,1,3) (3,3,1)
```

les trois droites restantes pour l'ancre `01` sont concourantes. Le fichier traite les trois paires comme trois sommets :

```text
CONCURRENT vertices_examined=3 vertices_shallow=3 emitted=0 dictionary_refuted=0
```

Le rescan du shell voit le cinquième point et rejette les trois émissions. Dans le raccord oracle, le catalogue ancré exhaustif signale par ailleurs la dégénérescence et le nuage est retiré du domaine. Il n'y a donc pas de faux support publié ici. Mais la concurrence n'est ni groupée en lot unique, ni signalée par `edge_shallow_supports`, et les compteurs gonflent de `C(t,2)` au lieu d'un sommet. Le CTest aléatoire n'exerce pas cette branche.

### 6.3 Doublons : l'intégration les détecte indirectement, pas l'arête seule

Avec un duplicata exact du premier sommet du tétraèdre régulier, la paire de longueur nulle est simplement quittée à `edge_shallow.hpp:99`. Le catalogue ancré auxiliaire détecte les shells supplémentaires :

```text
DUPLICATE degenerate_shells=40 zero_length_edges_skipped=1
```

Le raccord oracle supprime donc la publication. C'est sûr dans ce chemin hybride, mais ce comportement dépend précisément du passage exhaustif redondant ; `edge_shallow_supports` seul ne retourne aucun statut hors domaine. Aucun CTest `edge_shallow` permanent ne couvre doublons, shell supplémentaire ou concurrence.

### 6.4 Profondeur stricte : aucun défaut trouvé

Le test `>` avec inversion lorsque le déterminant est négatif est correct. Les deux carriers sélectionnés sont omis de la profondeur et comptés dans la base quatre. Toute autre égalité reste hors profondeur stricte et est reconstruite comme shell avant émission.

Fixture positive exacte : les quatre premiers points forment un tétraèdre régulier de centre `(2,2,2)` et rayon carré 3 ; le cinquième est strictement intérieur et milieu de `01` :

```text
(3,3,3) (3,1,1) (1,3,1) (1,1,3) (3,2,2)
```

À `s_max=5`, le support `0123` a le rang fermé 5. Le code rend :

```text
edge=01 constant_inside=1 target_rank=5 dictionary_refuted=0
edge=02 constant_inside=0 target_rank=5 dictionary_refuted=0
edge=03 constant_inside=0 target_rank=5 dictionary_refuted=0
edge=12 constant_inside=0 target_rank=5 dictionary_refuted=0
edge=13 constant_inside=0 target_rank=5 dictionary_refuted=0
edge=23 constant_inside=0 target_rank=5 dictionary_refuted=0
```

Sur `01`, le point intérieur est une constante du plan entier ; sur les cinq autres arêtes, il est une forme active strictement positive au centre. Cette fixture confirme exactement les deux termes du dictionnaire, mais elle manque au CMake.

## 7. Largeurs entières

La dérivation du fichier pour l'arité quatre est cohérente sur le profil `quantized_u16_input` :

- les produits internes des lignes tiennent très largement dans `i128` ;
- le déterminant est borné sous 87 bits ;
- le numérateur le plus large sous 88 bits ;
- le test de profondeur sous environ 123,6 bits, donc sous la limite signée de 127 bits de magnitude.

Les calculs intermédiaires stockés dans `P3` restent également sous `i64` : `b1` est de l'ordre de `d`, et `b2=d x b1` reste sous environ 33,6 bits. Je n'ai trouvé ni overflow signé ni erreur de facteur quatre dans le changement `t=s/4` sur le domaine déclaré.

Cette conclusion est un audit de bornes, pas une validation de toute coordonnée de la grille par le CTest aléatoire. Elle ne s'étend ni aux coordonnées hors `[0,65535]`, ni au profil dyadique, ni aux arités deux et trois.

## 8. Réponse au §2 bis : 50 000 points, ordre 10, moins d'une seconde

### 8.1 Borne utile de l'A2e idéal

À `K=10`, `s_max=11`. Pour une ancre utile, le budget support quatre est `kappa_e = 7 - c_e`. Si `c_e <= 7`, la borne annoncée donne :

$$Z_eleq m_e(8-c_e),qquad sum_e Z_eleq8sum_e m_e.$$

En posant `M = sum_e m_e` et `Z = sum_e Z_e`, un vrai constructeur A2e aurait une cible de coût de la forme :

$$T=T_{mathrm{A1}}+T_{mathrm{range}}+O!left(sum_eleft[m_elog m_e+Z_eight]ight)+T_{mathrm{exact}}+T_{mathrm{transcript}}+T_{mathrm{tri}}+T_{mathrm{aval}}.$$

La borne `Z <= 8M` est utile, mais elle ne borne ni le nombre d'ancres `a`, ni `M`, ni le range-report, ni le tri, ni les incidences aval. Elle n'est pas un constructeur et ne donne aucune durée seule.

### 8.2 Conditions nécessaires pour une seconde sur 48 cœurs

Toutes les conditions suivantes doivent être satisfaites sur chaque famille sanctionnée, pas seulement en moyenne :

1. **A1-source complète et sparse sous le budget.** Le balayage exhaustif a 1 249 975 000 paires à 50 000 points. La mesure citée dans `PROPOSITION.md:338-344`, environ 1,92 microseconde par paire, donne environ 2 400 secondes avant A2e. Le RNG borné n'est pas complet. Il faut donc une autorité fail-open qui prouve en bloc la masse écartée et dont `T_A1` reste très inférieur à une seconde.
2. **Range-report proportionnel à sa sortie.** Aucun scan des 50 000 points par ancre n'est admissible. Il faut mesurer `a`, `M`, les `c_e`, les maxima de `m_e` et les visites LBVH. Une seule ancre très lourde peut déterminer le mur.
3. **Vrai préfixe shallow.** Le coût doit être proche de `sum(m log m + Z)`, jamais de `sum C(m,2)`, et transporter les conflits nécessaires. Le fichier audité ne satisfait pas cette condition.
4. **Décision exacte sans rescan global par sommet.** Shell, membres, owner, niveau et payload HGP doivent venir d'un transcript borné ou d'un range-report proportionnel à la sortie ; `Z*n` est déjà incompatible avec la cible.
5. **Volume terminal compatible avec le tri global exact.** Si `S` est le nombre de records supports, incidences actives et silencieuses, verticales et couverture, le réducteur doit payer au moins leur matérialisation bornée, un ordre exact global ou des runs externes, puis le groupement de toutes les égalités. `S`, les octets et le coût des comparaisons rationnelles doivent entrer dans la même seconde.
6. **Aval complet inclus.** La seconde demandée ne peut pas s'arrêter à la génération des sommets : lots atomiques, attaches, descendances, incidences silencieuses et verticales doivent être chiffrés et qualifiés.
7. **Débit mesuré.** Une limite falsifiable doit relier `M`, `Z` et `S` à des nanosecondes par classification, sommet, comparaison exacte et record. Le dépôt ne fournit actuellement aucune de ces constantes pour ce chemin CPU 48 cœurs.

### 8.3 Verdict d'atteignabilité

- **Prototype actuel : non atteignable de très loin.** Ses masses exactes de la section 4 suffisent à le conclure sans benchmark à 50 000 points.
- **Architecture A2e idéale : mathématiquement possible sous hypothèses de parcimonie, mais non démontrée et non mesurée.** Aucun résultat audité ne permet aujourd'hui de dire « plausible sur 48 cœurs ».
- **Statut documentaire honnête : `experimental_target` / obligation ouverte.** Pour promouvoir « une seconde plausible », il faut au minimum un census reproductible de `a`, `M`, `Z`, `S`, des quantiles et maxima, puis un prototype réellement output-sensitive incluant A1, tri exact et aval.

La borne `Z <= 8M` est une bonne raison de poursuivre A2e. Elle n'est pas une preuve de SLO.

## 9. Commandes et artefacts de reproduction

### 9.1 Archive et build immuables

```bash
archive_dir=$(mktemp -d /tmp/edge-shallow-head.XXXXXX)
git archive --format=tar ad9def2258c91e61f26a674de10f1850017f1a35 | tar -xf - -C "$archive_dir"
cmake -S "$archive_dir/morsehgp3D_v3" -B "$archive_dir/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$archive_dir/build" -j2
ctest --test-dir "$archive_dir/build" -R '^mhgp3v_edge_shallow_depth_dictionary$' -V
```

Build observé : GCC 13.3.0, succès ; CTest ciblé : succès en 15,41 s. L'arbre CMake contient 23 tests.

### 9.2 Campagne complémentaire rangs 4 à 6

```bash
/tmp/edge-shallow-head.sQjHXK/build/mhgp3v_oracle \
  --subject edge_shallow --clouds 30 --seed 918273 \
  --min-points 8 --max-points 8 --max-order 5 \
  --min-decided 30 --min-nodes 1
```

### 9.3 Probe géométrique

Source temporaire : `/tmp/edge_shallow_geometry_probe.cpp`, SHA-256 `cd01c530b79999d6d2c1bba85c41e7e7a74bec443e759001f68367a3d91268ab`.  
Binaire : SHA-256 `0367eb9037295a2835ba8d76574718a8f32752d1deb599f8baa9d07a5ccce424`.

```bash
g++ -std=c++20 -O2 \
  -I/tmp/edge-shallow-head.sQjHXK/morsehgp3D_v3 \
  -I/tmp/edge-shallow-head.sQjHXK/morsehgp3D_v2/include \
  /tmp/edge_shallow_geometry_probe.cpp \
  -o /tmp/edge_shallow_geometry_probe
/tmp/edge_shallow_geometry_probe
```

Les coordonnées littérales et les sorties nécessaires pour recréer ce petit programme sont toutes données dans les sections 3 et 6.

### 9.4 Probe hostile du reçu au commit exact

Mutation locale unique, après `++statistics->emitted_arity_four` :

```cpp
++statistics->dictionary_refuted;
```

Puis :

```bash
/tmp/edge-shallow-ad9-receipt-probe.DzL8GR/build/mhgp3v_oracle \
  --subject edge_shallow --clouds 1 --seed 4242 \
  --min-points 8 --max-points 8 --max-order 3 \
  --min-decided 1 --min-nodes 1 \
  --receipt /tmp/edge-shallow-ad9-hostile-receipt.json
```

Reçu hostile SHA-256 : `854f0e29b7b21b778838e8ba6bff93499a85223c30d9b3aade031949a2bb7f22`.

## 10. Portes minimales avant toute nouvelle prétention

1. Remplacer le CTest par des fixtures permanentes séparant profondeur active positive, constante intérieure du plan, constante clippée par Jung, parallèles, concurrence, shell, doublons, owner unique et owner à égalité.
2. Imposer des planchers explicites par rang et par branche, et publier ces histogrammes dans un reçu attribuable et fail-closed.
3. Ne pas appeler `lines_active`, `vertices_shallow` ou `edges_retained` des mesures A2e tant que diamètre, ellipse, classification clippée, batch de concurrence et owner ne sont pas exécutés avant sink.
4. Supprimer du chemin de mesure le catalogue ancré arité quatre redondant ; un oracle peut rester exhaustif, le sujet de complexité ne le peut pas.
5. Construire réellement le préfixe shallow sans `sum C(m_e,2)`, avec transcript de conflits et sans rescan global par sortie.
6. Mesurer A1-source, `M`, `Z`, records aval, tri exact, octets et high-water sur les mêmes nuages à l'échelle avant toute estimation en secondes.

**GCP non utilisé.**
