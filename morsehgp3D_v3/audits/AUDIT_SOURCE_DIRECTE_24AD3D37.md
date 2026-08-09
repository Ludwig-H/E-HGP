# Audit continu de la source directe `24ad3d37` à `9edf150d`

Date du snapshot : 9 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_and_gpu_candidate_under_audit`,
`profile=quantized_u16_input_only`, `mode=audit_differentiel_borne`,
`public_status=not_claimed`.

Cet audit est strictement limité à `morsehgp3D_v3`. Il n'a modifié aucun
prototype. Le fichier audité est un delta non committé posé sur le HEAD
`c0df579ca0f5a6a6330294b8aa4fd80d372fb6ce`. Pendant la clôture, Claude a
remplacé `24ad3d37...` par `2b47247e...`; ce second palier n'ajoute que la
dispersion de `Q` par feuille et a été réaudité séparément. Il a ensuite ajouté
quatre CTests dans `CMakeLists.txt`, puis retiré transitoirement
`direct_source.cpp` du worktree. Le palier `9edf150d...` restaure le source,
ferme la plupart de ces findings et porte sept CTests dans le CMake
`4530a8c...`. C'est l'état live courant de ce rapport.

| objet | empreinte SHA-256 |
| --- | --- |
| `prototype/direct_source.cpp`, premier palier | `24ad3d37aedbf74c4b126fae30453a74d1f2a675eea572e6b92678b27c27258e` |
| `prototype/direct_source.cpp`, palier dispersion | `2b47247e9d1ecd6e1a8a573f4597bab9bb19e10a4a3d2ab4295c524d2d1ee68c` |
| `prototype/direct_source.cpp`, palier courant restauré | `9edf150de3f9b75cf931df405d0885f7644f05b622016b78fdb22bc3658216f0` |
| `CMakeLists.txt`, sans CTests directs | `beeb06c0399c038b6718d0ab7d48d8d4eec2ca666f86a3fb5e221bc405912c07` |
| `CMakeLists.txt`, palier courant avec quatre CTests | `1f06ad8b7d3f28ea4b4a89da945fe47ccf82dd05fbb9873ec633bd95a032f9b1` |
| `CMakeLists.txt`, palier courant avec sept CTests | `4530a8c4817fbfc1e399f0dff628f374b89d2d1d2117fa964c448ce72efec431` |
| binaire Release GCC 13.3 du premier palier | `d724f33c16f676804ed381190a9e6dadc2257ba9978635667aa7191fa7bd6a4e` |
| binaire Release GCC 13.3 du palier courant | `e33f045c26b1bf0f8ea54bdb31929b6317b4c0f55375172f2cb06b31f063de7f` |
| binaire Release GCC 13.3 restauré | `73dd077a75467c97182ca6d273295a309dd11448d6efb098e80d25e45c279006` |

## Verdict

**GO fonctionnel borné et relatif pour le lemme banque--rayon, les voisinages,
le payload membres et l'unicité sur les campagnes reçues; NO-GO maintenu pour
la totalité CLI aux petits `s_max`, la porte de coût/mémoire 50 k et toute
promotion en source produit ouverte ou certifiée.**

Le progrès architectural est réel. La partie source n'énumère aucun sommet
d'arrangement et ne construit aucune mosaïque d'ordre supérieur. Les candidats
de supports sont streamés depuis l'ancre de plus petit `PointId`; les voisinages
contiennent support, intérieur et coquille complets sous le certificat. Le
chemin de jugement par défaut appelle toutefois encore `flat_catalogue` avant
la source et matérialise une vérité exhaustive : l'absence de sommets vaut pour
la partie candidate, pas pour l'exécution complète avec `--judge 1`.

Le fichier est un bon falsificateur CPU borné. Le palier courant représente et
compare maintenant les membres, reçoit les doubles émissions, sépare ses trois
modes, élargit les compteurs de preuve et applique les replis racine. Il ne
constitue toujours pas la source Gabriel ouverte streamée ni la porte de coût
et de mémoire 50 k exigées par la note GPU.

## Résultats positifs indépendants

Le cœur de preuve a été réaudité sans réutiliser le tri ni le balayage du
prototype.

- La banque est non circulaire : si ses `t_q` témoins sont tous strictement
  intérieurs, le candidat dépasse la fenêtre; sinon un témoin non intérieur
  prouve le rayon carré strictement inférieur à `Q_q` avant le census.
- Le fallback racine est sûr par une preuve globale distincte : centre et point
  sont dans la boîte, donc leur distance carrée est au plus la somme des carrés
  des spans, strictement sous `Q_root`.
- La localisation rationnelle par division avec plancher est exacte pour les
  supports bien centrés.
- Avec `a*a<=Q_q<4*a*a`, les `9^3` cellules couvrent tout point à distance
  carrée strictement inférieure à `4*Q_q`; la frontière égale est correctement
  exclue.
- Les lanes d'arité un à quatre, l'ancre minimale et la canonicalisation
  concordent avec le catalogue partagé sur les campagnes exercées.

Trois oracles temporaires indépendants ont donné :

| oracle | couverture | résultat | SHA-256 du binaire |
| --- | ---: | ---: | --- |
| voisinage CSR contre scan quadratique | 33 914 listes, frontières `dist2=4Q` incluses | 0 écart | `8a5f267ecba25bf0348c2c21985acc07185c5099dd2e794730eb0b1273bedbee` |
| cover/rayon | 15 360 supports propres, 11 325 dans la fenêtre, 92 fallbacks racine | 0 écart | `f32ba21fe58890c7c93c9b5527d8a926597342ed7acc6b7d9ae7872944c872bd` |
| dernière feuille/fallback | témoin entier ciblé | preuve reproduite | `f62db5fc49c53e01330b3e7064a0c59c9ceea74d4205cfaf683adc0da1cb6f15` |

Sept côtés de feuille de 1 à 65 536 sur huit nuages chacun donnent aussi zéro
désaccord. GCC 13.3 et Clang 18 compilent le target Release avec
`-Wall -Wextra -Werror`; les campagnes ASan/UBSan ciblées sont vertes. Ces
artefacts temporaires n'ont pas de source ni de sidecar persistants et ne
remplacent donc pas une fixture permanente.

### Réaudit du delta de dispersion `2b47247e`

Le delta ajoute `q_leaf_min`, une médiane haute des `Q` locaux, leur vecteur et
leur tri, puis les imprime en `--cover-only`. Il ne change ni le lemme, ni la
décision, ni le voisinage. Sur un seul nuage sans fallback, ces trois
statistiques décrivent correctement les valeurs calculées; c'est un diagnostic
positif de l'hétérogénéité des feuilles. Le vecteur et son tri ajoutent cependant
un coût et une mémoire en nombre de feuilles qui ne figurent dans aucun
high-water.

Le reçu est faux dans les deux autres cas. Sur plusieurs nuages, la valeur
nommée `médiane` est le **maximum des médianes par nuage**, et non la médiane de
toutes les feuilles. En fallback, min et médiane viennent des coins nominaux
avant repli, tandis que le champ nommé `max` est le `Q_root` effectif après
repli. La commande :

```sh
mhgp3v_direct_source --clouds 1 --points 8 --coord 20 --smax 5 --seed 7 --cover-only 1
```

imprime pour `q=2` `Q=843`, `min=631`, `mediane=991`, `max=843` : la médiane
dépasse le maximum. Avec `n=10`, `coord=100`, `smax=4`, `seed=7` et
`leaf=65536`, elle atteint `12864596746` contre un `Q/max` effectif de 20642.
Il faut séparer `Q_grid_raw` et `Q_effectif`, ou agréger les valeurs effectives
avec un libellé exact. Une médiane de rayons ne dit par ailleurs pas
« exactement » le gain d'un cover adaptatif : degrés, masses et ordre de
construction restent inconnus. Aucun finding antérieur n'est corrigé par ce
delta.

### Réaudit de la gate CMake `1f06ad8`, puis source retirée

L'ajout de quatre CTests est un progrès positif. Sur le build configuré et
reconstruit pendant que `2b47247e...` était encore présent, les quatre passent :

```text
mhgp3v_direct_source_generic          PASS
mhgp3v_direct_source_degenerate       PASS
mhgp3v_direct_source_reject_unknown   PASS
mhgp3v_direct_source_reject_floor     PASS
```

La campagne générique reçoit 612 300 candidats, 10 524 refus de fenêtre,
12 124 émissions et zéro désaccord sur six nuages. Ses planchers sont sensibles
à la suppression d'une lane. La campagne dégénérée reçoit 9 372 candidats,
61 refus de fenêtre, 2 072 émissions, 36 fallbacks racine et zéro désaccord sur
douze nuages. Les tests négatifs protègent l'argument inconnu et un plancher
d'émissions hostile. L'absence totale de CTest du premier palier est donc
corrigée **dans CMake**.

Cette gate reste relative et incomplète. Ses commentaires promettent
« l'égalité EXACTE des deux catalogues », mais elle compare toujours une map par
coquille sans pool de membres ni multiplicité; le mutant 126/56 reste invisible.
CMake ne passe pas explicitement `--judge 1` : muter sa valeur par défaut à zéro
laisse les mêmes planchers verts sans oracle. Les deux campagnes ont par ailleurs
des voisinages complets, donc ne protègent pas la sélectivité spatiale. Aucun
test ne tue le faux succès `--judge 0`, le bypass `--cover-only`, `n<t_q`, le cap
de cellules, l'overflow 64 bits ou les quantiles incohérents. Le seul négatif de
plancher vise `min-emitted`.

Surtout, après ces passages, `prototype/direct_source.cpp` a disparu tandis que
`CMakeLists.txt` continue de le déclarer. Une configuration fraîche échoue :

```text
CMake Error at CMakeLists.txt:94 (add_executable):
  Cannot find source file: prototype/direct_source.cpp
CMake Error at CMakeLists.txt:94 (add_executable):
  No SOURCES given to target: mhgp3v_direct_source
```

Les résultats précédents restent valides pour le couple figé
`2b47247e.../1f06ad8...`, mais la porte live n'est plus reproductible. Claude
doit restaurer ou retirer atomiquement source, cible et tests; l'état
intermédiaire actuel est un P0 de build.

Ce P0 transitoire est fermé par la restauration `9edf150d...`; il reste conservé
ici pour la chronologie, pas comme verdict live.

## Réaudit du palier courant `9edf150d` / `4530a8c`

Le nouveau palier répond précisément à l'audit, et plusieurs fermetures sont
substantielles :

- CMake et stdout disent désormais **prototype** et **accord relatif**, jamais
  certification ni voie exclusive;
- les modes jugé, mesure et cover sont exclusifs; les deux derniers disent
  explicitement qu'aucune exactitude n'est affirmée, et les planchers restent
  actifs;
- chaque sortie transporte son vecteur ordonné complet de membres; le
  différentiel le compare à la référence;
- `emplace` reçoit l'unicité, un compteur sépare tentatives et clefs, et le
  mutant `--force-both-directions` rougit sur 314 doublons pour un nuage ciblé;
- `n<t_q` et le cap de cellules ont des statuts nommés et appliquent le fallback
  racine exact au lieu de sortir;
- les `Q` effectifs de toutes les feuilles et de tous les nuages sont agrégés;
  min, médiane et max redeviennent ordonnés après fallback;
- `C_q`, `T_q`, `H_q`, census et émissions passent en `u128`, et le mode cover
  calcule les masses avant de sauter l'énumération;
- un clamp du locator est désormais une faute finale, et les bornes de la
  dernière feuille distinguent correctement valeur brute et racine;
- sept CTests passent sur un build Release frais; quatre ciblés passent sous
  ASan/UBSan. La campagne générique reçoit les trois lanes; la dégénérée exerce
  36 replis racine; small-cloud, conflit de modes et double émission sont
  permanents.

Les commandes directes confirment aussi zéro désaccord pour le petit nuage
`n<t_q` et pour `--cell-cap 1`. Le mode mesure publie `reference=0` puis
`AUCUNE EXACTITUDE N'EST AFFIRMEE`; le mode cover refuse un plancher de
candidats puisqu'il n'en évalue aucun. C'est un net GO fonctionnel borné.

### Finding live P1 : domaine `s_max` contradictoire

La CLI accepte désormais `smax>=2`, mais la boucle appelle toujours les lanes
`q=2,3,4`. `build_cover` refuse logiquement `t_q=s_max-q+1<1`. Les deux commandes
valides selon le parseur échouent donc :

```sh
mhgp3v_direct_source --clouds 1 --points 5 --coord 10 --smax 2 --seed 1
mhgp3v_direct_source --clouds 1 --points 5 --coord 10 --smax 3 --seed 1
```

La première sort 3 à l'arité trois, la seconde à l'arité quatre, avec un faux
diagnostic d'arithmétique hors domaine. Il faut sauter les lanes `q>s_max` et
ne pas les inclure au reçu, ou rétablir explicitement `s_max>=4`; un CTest bas
ordre doit garder le choix.

### Finding live P0 : gates positives jugeables comme simple mesure

Les CTests positifs reposent sur la valeur **implicite** `judge=1`. La commande
générique avec exactement les mêmes planchers et `--judge 0` rend encore zéro,
avec 612 300 candidats, 10 524 refus de fenêtre et 12 124 émissions, mais
`reference=0`. Le stdout est honnête; c'est CMake qui serait fail-open si la
valeur par défaut du juge mutait. Chaque gate de jugement doit passer
`--judge 1` explicitement et recevoir une preuve non nulle de comparaison.
Le test small-cloud est alors particulièrement vacuable : son plancher de 30
émissions est exactement satisfait par les six fois cinq singletons, sans
aucune lane supérieure.

### Réserves de payload et mutation-résistance

Le chemin membres est actif et compare bien une **liste triée par coquille**,
mais aucun `mhgp::Catalogue` source avec pool global, offsets `members_begin`,
ordre et sérialisation n'est assemblé. Les claims « mêmes pools » doivent donc
être ramenés à « mêmes listes de membres par coquille ». Aucun mutant permanent
ne substitue ou supprime un membre pour prouver que le comparateur lui-même
reste vivant; la map attendue suppose aussi l'unicité de la coquille de
l'autorité sans l'asserter.

L'égalité observée `candidates==C_q` n'est jamais exigée. Une mutation qui saute
ou duplique seulement des candidats non émissibles peut conserver sortie et
planchers. Aucun CTest n'épingle `C_q/T_q/H_q` ni n'exerce une valeur dépassant
64 bits. Les campagnes n'ont enfin ni digest attendu ni coordonnées sidecar.
Elles ont des voisinages complets sur leurs entrées positives : degrés 39/39 et
11/11. Les frontières `dist2=4Q`, offsets `9^3`, centre rationnel sur split et
voisinage réellement sélectif restent couverts seulement par les oracles
temporaires, pas par CTest.

### Coût, mémoire et architecture produit toujours NO-GO

La construction du cover rescane toujours les `n` points dans chacune des `F`
feuilles et applique un tri partiel : à densité fixe, son terme reste quasi
quadratique. Le CSR reste quadratique au pire. `--cell-cap` borne un nombre de
feuilles, pas les octets; jusqu'à cent millions de feuilles sont acceptées et
une allocation peut encore échouer sans statut de reprise.

Les nouveaux high-waters sont utiles mais partiels. `banque+dispersion` ne
compte ni le vecteur global `leaf_q` accumulé sur tous les nuages, ni le buffer
`ranked`; `CSR` ne compte pas les buckets et listes transitoires; vérité,
`produced`, maps et vecteurs de membres sont absents. Le target refuse toujours
`n>20 000`, aucune campagne ne reçoit 50 k, et aucune sortie brute/sidecar ne
scelle les octets ou le coût.

Les masses principales sont bien en `u128`, mais `duplicate_emissions`,
`locator_clamps` et `mismatches` restent en `long long`. Leur borne cumulée sur
le domaine CLI `clouds=2000,n=20000` dépasse `2^63`; le claim global « les
compteurs de preuve sont passés en 128 bits » doit être limité aux
`SourceCounters`.

Enfin le prototype construit une map du catalogue **fermé** et son juge
matérialise `flat_catalogue`. Cela reste légitime comme falsificateur borné et
évite positivement les sommets dans la partie candidate. Ce n'est pas encore la
source Gabriel ouverte, agrégée par `SphereKey`, streamée avec statut de reprise
et sans catalogue global que décrit l'architecture cible.
Les formules « n'énumère aucun sommet » doivent rester qualifiées par « partie
candidate » : le mode par défaut matérialise l'oracle avant de lancer la source.

## Findings historiques des paliers `24ad3d37` / `2b47247e`

Les reproductions ci-dessous expliquent les corrections de `9edf150d...`. Les
sections 1 à 3 sont fermées au palier courant; la section 4 reste partiellement
ouverte pour la complexité et la mémoire.

### 1. Le mode de jugement était fail-open — fermé

La commande suivante sort avec le code zéro :

```sh
mhgp3v_direct_source --points 10 --coord 10 --smax 4 --clouds 1 --judge 0 --seed 7
```

Elle imprime d'abord `AUCUN JUGEMENT`, puis `OK : la source directe rend
exactement le catalogue ferme`. L'exactitude est ainsi annoncée en l'absence
d'oracle. La provenance omet en outre `--judge` et `--cover-only`.

`--cover-only` revient avant les planchers et avant le verdict des désaccords.
Avec `--judge 1`, dix points et la même graine, le binaire imprime 53 sphères
manquantes, ignore `--min-emitted 999999 --min-candidates 999999`, puis sort
zéro. Il affirme aussi que ni candidat ni sphère n'est produit alors que les
singletons d'arité un ont déjà été construits. Enfin ce mode appelle encore
`flat_catalogue` lorsque le juge est actif.

Le premier palier ne déclarait aucun CTest. Le palier intermédiaire `1f06ad8...`
en a ajouté quatre, puis la source a disparu transitoirement. Le palier courant
restaure le source, porte sept CTests et ferme ces sorties fail-open; la
chronologie reste ici pour expliquer les nouvelles portes.

### 2. Le payload et l'unicité étaient invisibles — fermé

`members` est calculé pour le rang puis jeté. `produced` ne contient qu'un
`CriticalSphere`; aucun pool de membres n'est construit et `members_begin`
reste nul. Le différentiel compare la coquille, le support, l'arité, le rang et
la valeur exacte de la sphère, jamais les identifiants de l'intérieur. Deux
payloads de même rang avec membres différents sont indiscernables.

Les deux maps emploient la coquille comme clef et l'affectation écrase les
doublons. Elles ne jugent ni multiplicité ni propriété « exactement une fois ».
Un mutant temporaire qui retire seulement la condition `z>p` émet 126 fois au
lieu de 56 sur `n=8, seed=7`; les doublons sont écrasés, le différentiel rend
toujours zéro écart et le faux `OK`. Il faut recevoir séparément unicité,
multiplicité et payload complet; `emitted` n'est actuellement comparé ni à la
taille de la map ni à un digest de séquence.

### 3. Le fallback annoncé n'était pas total — fermé

Deux entrées acceptées par la CLI échouent avant toute reprise :

```sh
mhgp3v_direct_source --points 5 --coord 10 --smax 32 --clouds 1 --judge 0 --seed 7
mhgp3v_direct_source --points 5 --coord 65536 --smax 4 --clouds 1 --leaf 1 --cover-only 1 --judge 0 --seed 7
```

La première a `n<t_2`; pourtant `s_max>n` rend toute sortie automatiquement
dans la fenêtre et un voisinage racine est exact. La seconde dépasse le cap de
quatre millions de cellules. Les deux sortent 3 avec le même message générique
`cover ... non construit`; aucun statut typé, resume token ou replay ne les
distingue. À l'inverse, `--leaf 65536` atteint bien le fallback racine, donne
trois replis et zéro désaccord : le mécanisme est correct lorsqu'il est atteint.

La dernière cellule nominale peut dépasser le maximum du nuage. La borne
commentée `3*65535^2<2^34` est donc fausse pour la distance brute au coin; une
borne sûre est inférieure à `3*(2*65535)^2<2^36`, toujours largement dans
`i64`. Sur le cube `[0,10]^3` avec côté 8, le maximum nominal vaut 548 alors que
`Q_root=301`. Le fallback reste exact grâce à la preuve globale boîte--boîte,
pas grâce au certificat de coin initial; le reçu doit dire quelle preuve est
active.

Le clamp de `locate_leaf` ne doit jamais servir sur un support bien centré,
puisque le centre appartient à l'enveloppe convexe du support. S'il se
déclenche, il masque une violation d'invariant au lieu d'échouer fermée.

### 4. La porte de coût ne mesure pas encore son coût complet — partiellement ouvert

`build_cover` rescane les `n` points dans chacune des `F` feuilles et applique
un tri partiel. Son coût est `F*n` tests plus le tri, et non `O(n)` par lane.
Avec le côté par défaut à densité fixe, `F` est proportionnel à
`n/(s_max+1)` : le terme est quasi quadratique. Une sonde à densité fixe passe
de 8 100 tests de cover à `n=100` à 300 000 à `n=800`, pour un facteur 37 quand
`n` est multiplié par huit. Sous le cap courant, une seule lane peut encore
faire 80 milliards de tests de distance.

Le voisinage peut être dense : son CSR atteint environ 400 millions d'IDs à
`n=20 000`, soit environ 1,6 Go pour les seuls IDs. La banque peut atteindre
124 millions d'IDs, environ 496 Mo, avant maps, listes, sorties et vérité. Aucun
cap en octets, high-water, échec d'allocation typé ou replay n'est publié. Le
target refuse en outre `n>20 000`, donc n'exerce pas le palier produit 50 k.

`9edf150d...` ajoute des high-waters structurels CSR/banque; ils restent
partiels comme détaillé dans le verdict courant et ne ferment ni allocation ni
50 k.

Le mode `--cover-only` matérialise déjà les voisinages, mais son `continue`
précède la boucle qui calcule `C_q`, `T_q` et `H_q`. Il ne publie donc pas les
masses combinadiques qu'il prétend pré-évaluer; les obtenir exige actuellement
l'énumération complète.

Ce point est fermé à `9edf150d...` : les masses sont calculées depuis les degrés
avant le `continue` du mode cover.

Enfin `bound_t` et `census_tests` sont des `unsigned long long`. Dans un graphe
complet à l'arité quatre, leur borne est `(n-1)*C(n,4)`. Elle dépasse `UINT64_MAX`
dès `n=13 468` et vaut `133286672333050005000` à `n=20 000`, soit 7,23 fois la
capacité. Le wrap est silencieux sur une entrée autorisée. Les compteurs de
preuve doivent être checked, saturés avec statut explicite ou élargis avant de
qualifier un coût.

Ce wrap précis est fermé par les `SourceCounters` en `u128`; quelques compteurs
de verdict restent toutefois en `long long`.

## Aide constructive à Claude

Le palier courant a déjà appliqué une grande partie de l'aide initiale. Avant
toute promotion, les prochaines corrections utiles sont maintenant :

1. fermer le domaine bas ordre en sautant `q>s_max` ou en rétablissant la
   précondition `s_max>=4`, avec CTests `s_max=2/3`;
2. passer `--judge 1` explicitement dans chaque gate différentielle et recevoir
   le mode/le nombre de comparaisons, puis ajouter des CTests positifs propres
   aux modes mesure et cover;
3. graver un mutant de membre, l'identité `candidates==C_q`, des digests attendus
   et une fixture réellement sélective aux frontières du voisinage;
4. distinguer listes de membres par coquille et vrai pool global sérialisé avant
   de parler de `Catalogue` ou de « mêmes pools »;
5. ajouter caps en octets, statuts d'allocation/reprise et high-waters complets,
   puis remplacer le cover `F*n` si sa mesure ferme la porte 50 k;
6. conserver `flat_catalogue` comme juge borné seulement et développer
   séparément la source Gabriel ouverte streamée, sans arrangement, mosaïque ni
   catalogue de vérité dans le chemin produit.

GCP non utilisé : aucune VM créée, démarrée, arrêtée ou modifiée pour cet audit.
