# Audit courant de MorseHGP3D v3

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur

`HEAD` audité : `041169150f07f88118084c7ac5556a790ed814b7`, sujet
`deliver the exact closed-depth terminal filter with three isolated modes`. Il
inclut le différentiel q2 de `d705bcde` et le sidecar de `9f6ea3c`. Au moment du
pincement, le code du worktree est identique au `HEAD`; les seules modifications
de l'auditeur concernent la documentation de ce verdict.

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `d0d5dfa9330b84262879d8df54229720e05176c36c38a88b7b44dbf56942c7ac` |
| `prototype/cloud_families.hpp` | `1a3e3027c2e0880e6ff381fc80b707b9ec88dbf573579aac535cfc80bb307b54` |
| `prototype/pair_selfjoin_probe.cpp` | `510c8306c7c99aa65b01506f7d2d3eac7317ff4e6f7de2f94f3ad60b19e583ac` |
| `prototype/pair_anchor_probe.cpp` | `e214280550e8cce986282a6150e3e4e675ed122b8d89d023b8cb9d02fe026520` |
| `prototype/validated_hybrid_sidecar.hpp` | `4df79198794c64c824abc04525a753dac0855d8e3bb43f6eb87f8f9ff2efbda7` |
| `prototype/sealed_source.hpp` | `98277903b46f93ec8cba85e54f212952942344363757851550427cd5fa489603` |
| `prototype/sidecar_factory_gate.cpp` | `c5e43ee6bda923eac29e4e77f9269b7196f108c6b065f6f5043d5cd761b327d2` |
| `prototype/sidecar_sha256.hpp` | `401df9cccd0cd0a5dc99d06e8836f01797dd37095e8aaafa9b68a59d43f3cb3e` |
| binaire Release q2 | `719b1ce1e628814807f72110de2ab3bae44da9f93f613329415db0b14f03c9b7` |
| binaire Release ancres | `ac87a36ec24eae2114c701a8bf4577dd120df8808e7b5316ea555e6c49cac442` |
| binaire Release sidecar | `b2dbdd2cf3aa46755f13c093cff0b3d779b8d69b1f18d9c88eb0ffd3f21b6a4a` |
| binaire ASan/UBSan sidecar | `887d722b876632d302f8805538f364e085a201413f7d1bf4983ba34d4d3bd18e` |

Une suite lancée avant la dernière modification ou un binaire construit sur une
autre empreinte ne reçoit pas ce worktree. Cet audit est la seule autorité
mutable du statut v3; les autres audits sont des preuves ou snapshots épinglés.

## Verdict

Le contrat n'est pas rempli. À 50 000 points et `K=10`, la cible principale
est un p95 `warm_e2e<100 ms`; `warm_e2e<1 s` est le jalon secondaire demandé.
Aucun backend public exact n'est qualifié.

Le verrou reste une source q2/q3/q4 exacte, complète et parcimonieuse, suivie
du census fermé, des `BallActivation`, du resolver, du fold horizontal et du
payload. Les sondes actuelles sont des falsificateurs bornés avec budgets. Le
chemin candidat ne streame aucun support q3/q4, aucune activation et aucune
hiérarchie bout en bout; l'oracle borné d'ancres, lui, énumère bien des tuples.

Gamma/v2 exhaustif et `hgp_reduced_normalized_h0_v3` restent deux sorties
distinctes. Une tombstone H0 ne prouve ni l'absence d'un support, ni celle
d'une incidence Gamma ou d'une verticale. Les verticales sont hors du contrat
horizontal et gardent une porte séparée.

## État des tests

Le registre CTest du `HEAD` compte 275 tests, dont 34 q2, 34 ancres et 7
sidecar. Sur les binaires épinglés ci-dessus, trois rejeux ciblés passent : q2
`34/34` en 0,93 s, ancres `34/34` en 3,38 s et sidecar `7/7` en 0,32 s, soit
`75/75` ciblés. Les réserves ci-dessous montrent pourquoi ce vert n'est pas une
réception générale.

```bash
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_pair_selfjoin_'
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_pair_anchor_'
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_sidecar_'
```

Aucun résultat global `275/275` n'est revendiqué. Un journal antérieur consigne
`254/254`, mais sa suite a traversé des reconstructions concurrentes et précède
les deux derniers commits; il ne reçoit donc pas ce `HEAD`. Le ciblé vert ne
ferme pas les contre-résultats statiques ci-dessous.
`python tools/check_docs.py` ne parcourt pas le dossier v3; ses liens et règles
LaTeX demandent un contrôle séparé.

## Cardinalité du générateur : contre-exemple q2 fermé

Le contre-exemple permanent est
`scanline_overlap_multiecho`, `n=12500`, `coord=707`, seed `20260810` :
l'ancien générateur rendait 12 501 points. L'ancien q2 construisait son arbre
sur ces 12 501 points mais comparait la couverture à `C(12500,2)` et
dimensionnait le sort par paire avec 12 500. Le ledger échouait; sous
`--verify-bruteforce`, un accès hors bornes du tableau de sorts était possible.
Ce défaut visait le contrat générateur--driver, pas les lemmes q2 ou Jung.

Le `HEAD` borne désormais chaque `push` multi-écho par `n`. q2 exige ensuite
`pts.size()==n` avant l'arbre et possède un mutant qui rétablit l'overshoot;
CMake grave un nominal à 1 100 points et le mutant associé. Le falsificateur
d'ancres exige lui aussi l'égalité. Cette direction réalise le contrat durable
« exactement `n`, ou refus fermé ».

Les portes q2 nominale et mutante passent sur le binaire reconstruit; le
contre-exemple q2 est donc fermé. Comme `make_family_cloud` est partagé, une
gate directe du générateur doit encore
couvrir les quatre familles, insuffisance de complétion et consommateurs qui
n'emploient pas les deux probes. Aucun consommateur ne doit réinterpréter
`--points` comme un minimum.

Le journal historique
[`scale_counters_raw.txt`](../receipts/selfjoin_q2_20260811/scale_counters_raw.txt),
SHA-256 `2685ceb387f46cb0be2f0a04f7b1ad8afbcaa41c521dad20328c7a4cb5332bc5`,
conserve 16 runs de l'ancien binaire : 15 codes nuls et l'échec 12 500. Le
journal correctif séparé, SHA-256
`3ade1bc74dd2f129a9c26079fe8c52195946e8ccd479c587e462e2d40144149d`,
démontre seulement qu'un autre binaire normalisait la taille réelle; il ne
transforme pas le journal rouge en reçu vert et il est remplacé
conceptuellement par le contrat exact du générateur.

## Self-join q2

La preuve locale est exacte. Pour une paire `x,y`, un point `w` est strictement
intérieur à la boule diamétrale exactement lorsque
`(w-x) dot (w-y)<0`. Le supremum `U4` sur trois AABB est séparable et exact;
dix `PointId` distincts hors extrémités autorisent seulement une tombstone H0
q2 jusqu'à `K=10`. Le sort triangulaire de petit `n` ferme omission, doublon et
compensation paire par paire.

L'infimum `L4` committé est l'infimum exact sur les AABB continues. `L4>=0`
exclut correctement tout témoin strict. Hériter au plus neuf positions est
inductivement sûr tant que `tree.order` est immuable. Le format publié actuel
n'engage toutefois pas complètement cet ordre, les points et la topologie.
Pour une future frontière persistante, le frère d'extrémité libéré par chaque
split doit être réintroduit puis reclassifié; hériter seulement l'ancienne
antichaîne perdrait des témoins nouvellement admissibles.

Le commit `d705bcde`, inclus dans le `HEAD`, reçoit une préservation
sémantique bornée : sa référence interne désactive `L4` et tous les crédits
hérités, puis compare au candidat les cinq masses structurantes et chaque sort
de paire. Le balayage exact vérifie séparément qu'aucune paire non inerte n'est
prunée. Les deux campagnes nominales sont `terrain, n=400` et
`uniform, n=1000`; cinq mutants terrain meurent à code 4 et le plancher
`9+nouveau` est non vacant. Cette baseline partage arbre, partition, `U4` et
plomberie et récolte encore des handles : c'est une référence de décisions,
pas une seconde implémentation ni une baseline de coût.

La réception générale et industrielle reste refusée pour les motifs suivants :

- le CTest q2 scanline n'a pas de double imposant le code zéro; sa seule
  `PASS_REGULAR_EXPRESSION` peut ignorer un code non nul après avoir vu
  `FERME`;
- aucune porte n'impose que `L4` morde : le remplacer par une borne toujours
  négative ou le désactiver peut conserver tous les sorts et masses. Le mutant
  `l4-sign` substitue `U4` et ne couvre ni axe omis, ni clip, ni frontière
  `L4=0`;
- `--min-nine-plus-one` est silencieusement ignoré hors mode différentiel;
  cette combinaison doit refuser;
- une défaillance interne de la baseline passe actuellement par `fail()` et
  peut être transformée en faux code 4 lorsqu'un mutant sujet est actif; le
  juge doit échouer indépendamment des injections du sujet et vérifier aussi
  qu'aucun sort baseline ne reste non assigné;
- le SHA-256 de l'ordre n'est imprimé que sur 16 hexadécimaux et n'engage ni
  points ni topologie; cela ne suffit pas à identifier un reçu de handles;
- la fixture u16 extrême n'active pas le différentiel et aucune configuration
  sanitizer n'est enregistrée pour q2. Manquent encore parité grille,
  `L4<0`, `L4=0`, `L4>0`, clips des deux côtés et demi-entiers;
- les campagnes CMake n'utilisent qu'un seed et les mutants seulement terrain.
  Elles prouvent une équivalence bornée, jamais un gain de coût ni une route
  vers 50 k.

Sous u16, un infimum grille plus fort est disponible au même coût : minimiser
sur l'entier `w0=clip(floor((x+y)/2),[w_min,w_max])` pour chaque couple de
coins. Il élimine notamment le faux négatif continu `x=0,y=1`, dont l'infimum
grille est zéro. Ce changement exige son propre différentiel exhaustif avant
adoption.

### Coût q2

Les segments 50 k du journal historique ont des ledgers fermés, mais leurs
temps sont sous charge et leur binaire n'est pas le worktree actuel :

| famille | états | visites `L4` | tests ponctuels | paires terminales |
| --- | ---: | ---: | ---: | ---: |
| terrain | 710 396 | 240 347 699 | 495 522 203 | 6 205 971 |
| scanline simple | 367 890 | 53 240 637 | 86 172 879 | 3 598 676 |
| multi-écho | 950 500 | 393 107 357 | 801 949 159 | 8 109 344 |
| uniforme | 1 580 440 | 723 579 105 | 1 364 858 170 | 14 851 373 |

Chaque visite `L4` et test ponctuel réalise au moins douze produits entiers;
les évaluations `U4` ne sont pas comptées. Cela représente déjà environ 1,67
milliard de produits pour scanline simple et 8,83 milliards pour terrain. q2
seul reste donc très loin d'une seconde, avant census, q3/q4 et fold. La route
à comparer reste Yao48/LBVH avec classifieur terminal et census fermé; le
self-join reste oracle, falsificateur ou second prune.

## Falsificateur d'ancres q3/q4 `core`

Le théorème est sain. Pour une paire distincte certifiée arête maximale d'un
support propre positif, les prédicats `g>0` puis `3g^2>4Q` en q3 ou
`g^2>2Q` en q4 certifient un témoin strict pour tout le disque de Jung. Neuf
ou huit `PointId` distincts autorisent seulement la tombstone H0 correspondante.
Le test par huit coins est sûr par convexité; la partition nominale couvre les
paires.

Le code committé n'implémente que `--mode core`. `depth` est explicitement
refusé et le center-cover est absent. La réception logicielle est refusée pour
les motifs suivants :

1. en q4, `15*maxU2<=4*minD2` accepte `D^2=U^2=0`. Sur des `PointId`
   colocalisés, les prétendus huit témoins ont `g=0` : le prédicat ponctuel les
   refuse mais le pré-prune fabrique un certificat faux. Exiger `minD2>0`,
   sinon descendre jusqu'au prédicat exact;
2. le rejeu appelle sa variable `distinct` mais ne déduplique pas
   défensivement les handles. Le producteur nominal parcourt actuellement une
   partition et ne pousse chaque position qu'une fois; c'est un durcissement
   du juge et du futur format de reçu, pas un contre-exemple nominal établi;
3. les certificats internes portent des positions dans `tree.order`, non des
   `PointId` persistants. Tout reçu exporté devra engager explicitement l'arbre
   et sa permutation ou convertir les positions en identifiants stables;
4. l'« oracle exhaustif » partage `mhgp::miniball_of` et `sphere_side` avec la
   dépendance v2, et le rejeu partage `universal_witness` avec le sujet. C'est
   un différentiel relatif et un oracle exhaustif du parcours, jamais une
   autorité mathématique indépendante;
5. manquent une fixture q4 colocalisée qui mord le point 1, l'égalité sûre du
   sous-test rationnel q4, les extrêmes u16 et des planchers séparés pour
   blocs, coins et terminaux. La campagne CMake `uniform, n=24` et les sept
   fixtures ont toutes `blocs-prunes=0`; elles ne rejouent aucun certificat de
   bloc.

La campagne bornée `--points 32 --seed 13 --family terrain --leaf-size 2
--oracle 1` ferme l'oracle avec deux blocs q3 et un bloc q4 prunés. Elle doit
remplacer ou compléter la campagne CMake vacue. Cent autres campagnes
`n=16`, quatre familles, cinq tailles de feuille et cinq seeds n'ont révélé
aucun désaccord; ce diagnostic ne remplace ni la fixture dégénérée ni un juge
indépendant.

Deux mutants demandés par la note initiale sont non observables dans ce
count-only. Accepter un tuple non positif ne peut faire disparaître une ancre
active ici : si une paire est prunée, ses 9/8 témoins universels sont déjà
strictement dans la miniboule du tuple et le code la classe inerte; sinon la
paire reste résiduelle. De même, toutes les arêtes maximales d'un support non
inerte survivent, donc choisir un autre maximum ex æquo ne change pas le set de
paires. Positivité et ownership canonique restent obligatoires, mais leurs
mutants appartiennent au constructeur de supports/activations aval.

Les commentaires/compteurs demandent aussi correction : le contact q3 n'est
frontalier que pour la paire choisie, pas chaque arête du support; le tétraèdre
gravé a un rayon carré 108, pas 3; `corner_tests` compte les nœuds soumis au
test et non les une à huit évaluations réellement exécutées; « exactement »
dans le préambule vaut sur le disque entier de Jung, tandis que le prédicat est
seulement suffisant pour le sous-ensemble des centres réalisables.

### Campagne d'ancres

Le journal
[`anchor_core_counters_raw.txt`](../receipts/selfjoin_q2_20260811/anchor_core_counters_raw.txt)
est count-only, mono-thread et hors `warm_e2e`. Son en-tête annonce encore
`40050c4+delta` et ne donne ni SHA source, ni CMake, ni générateur; le binaire
est identifié mais la provenance n'est donc pas un manifeste complet. Le
message de commit « linear residuals » décrit au mieux l'observation
1 200--2 400, jamais une borne.

À seulement 2 400 points :

| famille | q3 visites / tests / résiduelles | q4 visites / tests / résiduelles |
| --- | ---: | ---: |
| terrain | 369 M / 858 M / 83 072 | 420 M / 977 M / 85 626 |
| scanline simple | 309 M / 718 M / 86 581 | 344 M / 800 M / 89 338 |
| multi-écho | 461 M / 1,071 Md / 109 339 | 507 M / 1,180 Md / 115 537 |
| uniforme | 977 M / 2,273 Md / 210 661 | 1,082 Md / 2,516 Md / 232 105 |

Les phases observées vont de 10 à 113 secondes, sous charge et sans protocole
p95. Entre 1 200 et 2 400, les exposants des visites sont déjà compris entre
environ 2,1 et 2,6 selon lane et famille, donc tous au-dessus de la gate choisie
à 1,35. Les compteurs suffisent : le rescan depuis la racine est `NO-GO` comme
route produit et n'a aucune trajectoire mesurée vers 50 k sous une seconde.

Avant tout port device, réutiliser `L4` comme rejet de nœuds témoins
(`g>0` implique l'intérieur diamétral), hériter sans perte les 9/8 `PointId`
déjà universels et conserver une frontière exacte sans cap, en réintroduisant
les sous-arbres d'extrémités libérés par chaque split. Si les exposants restent
au-dessus de la gate, abandonner ce self-join comme route produit au profit du
LBVH/range-report; le probe demeure oracle ou second prune. La profondeur
terminale vient ensuite, avec mesures `core`, `depth` et `combined` séparées.

## Sidecar borné

Le successeur de `cbac109` ferme les contre-exemples initiaux sur son périmètre :
construction privée et anti-forge, doublon `[r1,r2,r1]`, bornes u16 avant
géométrie, magnitudes sans UB, support canonique unique, SHA-256 canonique et
reçu déplacé invalidé. `9f6ea3c` ajoute les bornes hautes u16, quatre mauvaises
identités producteur et un mutant `sha-fault`; les identités erronées laissent
bien la fermeture inconnue. Les 7 CTests Release ciblés passent en 0,32 s.

Le même sous-ensemble passe `7/7` en 0,57 s dans un build `RelWithDebInfo`
`-fsanitize=address,undefined -fno-sanitize-recover=all
-fno-omit-frame-pointer`, avec `ASAN_OPTIONS=detect_leaks=1` et
`UBSAN_OPTIONS=halt_on_error=1`. Ce rejeu borne l'absence d'erreur sanitizer à
ces fixtures; il ne qualifie ni provenance ni complétude du producteur.

Trois limites restent ouvertes :

- `producer_version_digest` est désormais honnêtement nommé, mais reste le
  hash d'un littéral. `claims_complete_family()` ne le compare pas à une valeur
  attendue et aucun manifeste ne lie source, ELF ou options de build;
- `sha-fault` ajoute directement une branche de refus à côté du vrai résultat
  du self-test; il ne sabote pas `sidecar_sha256_selftest()`. Supprimer l'appel
  réel laisserait ce mutant vert, d'autant que la gate préteste SHA en dehors
  de la factory. L'obligation du contrôle interne n'est donc pas reçue;
- le pipeline exige `smax>=n`, refuse `smax>32`, double l'énumération et rescane
  par générateur. Il demeure à jamais un oracle CPU `n<=32`, jamais la source
  50 k.

## Ordre des portes

1. Corriger la garde dégénérée q4 et graver la campagne de blocs non vide;
   compléter en parallèle la porte de cardinalité du générateur partagé et le
   code zéro du CTest q2 scanline.
2. Durcir la gate q2 : juge de référence indépendant des injections, sort
   baseline totalement assigné, plancher de non-vacuité `L4`, mutants de
   formule/clip et identité complète de l'arbre. Comparer ensuite à
   Yao48/LBVH+census; ne pas extrapoler le différentiel borné vers 50 k.
3. Ajouter au cœur `L4`, héritage et frontière lossless; appliquer la gate
   d'exposant avant toute campagne 12,5 k ou tout port G4.
4. Construire l'owner et la positivité dans la source q3/q4 aval, puis
   implémenter séparément profondeur et center-cover; ne jamais utiliser le
   résiduel q2 comme univers supérieur.
5. Recevoir `BallActivation`, census fermé, resolver latent et fold horizontal
   contre Gamma exhaustif borné.
6. Porter seulement les primitives admises sur G4, puis mesurer build, source,
   certification, census, resolver, fold et payload dans le même p95
   `warm_e2e` à 12,5 k, 25 k et 50 k.

Une insuffisance de ressource refuse atomiquement. Aucun tableau global de
paires, tuples, cellules, faces, cofaces ou incidences n'entre dans le chemin
produit.

GCP non utilisé.
