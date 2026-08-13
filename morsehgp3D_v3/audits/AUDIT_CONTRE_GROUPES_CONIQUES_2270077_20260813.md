# Contre-audit du probe de groupes coniques

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le théorème ponctuel est correct : trois sites satisfaisant H2 et dont le cône
positif contient `d` fournissent, ensemble, au moins un intérieur strict pour
toute sphère passant par les endpoints. Le probe montre utilement que des
groupes peuvent fermer des candidatures que **tout ensemble de témoins
universels individuels** laisse ouvertes.

Il ne reçoit pas encore la deuxième voie collective : packing glouton incomplet,
génération `O(n^3 log n)`, aucun résiduel factorisé, `smax` ignoré, juge H2
partagé avec le sujet, mutants inertes et aucun cap/octet/pente. Verdict :
**microprobe mathématique borné, NO-GO 50 k/G4**.

## Pin et rejeu

Le pin est `HEAD=22700778af0d14bd4e25c614bf901ccf427946f2`, commit
`three witnesses cover what none of them covers alone`.

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `57fc7435657b89fa8ea846152b55ea5db45d58970f2de8ac7e1ae0cd4e2b8ef3` |
| `prototype/conic_groups.hpp` | `75f3ab9067be8240eef167fab6ca084616d83f16317f6bbb1fa977ec79e861a3` |
| `prototype/conic_groups_probe.cpp` | `974d14d312b2733a4e258ab46b11170ab42f5d91b3aec9b9c50db875cda6b266` |
| ELF Release | `928043ebaf9a4fefe4e1eb14fefd8760b8c4da7fc7fe6172bb2e029a25dca771` |

L'ELF a `74 864` octets et le Build ID
`2b0b1cc1f77dead377b67e9de875741cfb45269e`. Configuration Release, CUDA
désactivé. La commande ciblée :

```text
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_groupes_'
```

rend `11/11` en `5,61 s`. Ces verts sont diagnostiques et soumis aux limites
ci-dessous.

## Théorème admis, packing non admis

Pour `d=b-a`, `s_i=z_i-a` et tout centre admissible paramétré par `t dot d=0`,
poser `mu_i(t)=d dot s_i-||s_i||^2+2s_i dot t`. Si
`d=sum alpha_i s_i`, `alpha_i>=0`, et chaque marge H2 est strictement positive,
normaliser les coefficients donne une combinaison convexe des `mu_i(t)` dont
le terme en `t` s'annule et dont la partie constante est positive. Pour chaque
sphère, au moins un membre du groupe est donc intérieur ; ce membre peut varier
avec la sphère. Des groupes disjoints donnent des intérieurs distincts.

Le test Cramer plein rang reçoit correctement un triple. En revanche, former
les triples angulaires à pas régulier est un greedy explicitement incomplet. Il
ne prouve aucune absence et ne produit pas le packing maximum. Cela n'impose
pas un packing maximal au fast path : tout packing canonique de `h` crédits
vérifiés est exact, et son échec reste fail-open. Une voie cellulaire plus
simple généralise le crédit à une taille quelconque, construit l'enveloppe
convexe projective 2D des témoins actifs, extrait au plus neuf IDs couvrant les
trois rayons de la cellule, retire ces IDs et recommence. Elle évite à la fois
`C(m,3)` et le 3-set-packing ; le packing maximal reste seulement un oracle de
rappel à petit `m`.

Le probe omet aussi les groupes de taille un et deux. Un témoin universel est
un singleton conique ; deux projections antipodales forment une paire. Fixture
q4 : `d=(100,0,0)`, sept singletons `(10+k,0,0)`, `k=0,...,6`, puis
`p=(10,20,0)`, `q=(10,-20,0)`. Les sept singletons donnent sept crédits ; ni
`p` ni `q` n'est ponctuellement q4, mais `d=5(p+q)` et H2 est stricte, donc la
paire donne le huitième crédit. Tous les points sont coplanaires et le probe de
triples plein rang rend zéro groupe. Un générateur complet doit donc traiter
singleton, paire et triple ; sa colonne `ponctuel_seul` devrait être vide par
inclusion des singletons.

Cette omission casse aussi la précondition de `std::sort`. Une projection nulle
`V=0`, c'est-à-dire un singleton parallèle à `d`, entre dans le comparateur
angulaire ordinaire et peut créer un cycle. Exemple avec `d=(20,0,0)` :
`s_C=(10,1,5)`, `s_Z=(10,0,0)`, `s_B=(10,1,-5)` et des PointId dans l'ordre
`C<Z<B`. Dans la base courante, les projections sont
`C=(100,-400)`, `Z=(0,0)`, `B=(-100,-400)` ; le comparateur donne
`B<C`, `C<Z` et `Z<B`. Il n'est donc pas strict-weak-order et le tri n'a plus de
sémantique C++ garantie. Extraire tous les `V=0` comme singletons avant le tri,
puis donner une classe totale distincte aux seules projections non nulles.

## P0 — `smax` est imprimé mais ignoré

Le probe fige `kNeed={10,9,8}`, accepte `smax` dans `[4,34]`, puis n'emploie
jamais `opt.smax` pour les verdicts ponctuels ou par groupes. Le falsificateur
ne peut pas voir cette faute puisqu'il compare lui aussi le nombre de groupes
aux seuils figés.

Reproducteur sur l'ELF pincé :

```text
./build/v3/mhgp3v_conic_groups_probe --points=120 --family=eight_clusters --seed=3 --judge --smax=34
```

Il rend code `0`, `accord=OUI`, et publie encore `2 826` fermetures q4 avec
seulement huit groupes requis. Le vrai besoin est `h=34+1-4=31`; ces verdicts
ne sont donc pas autorisés. Refuser `smax!=11` ou dériver partout
`h=smax+1-q`, dimensionner le packing et juger ce même seuil. Graver
`smax=4,11,12,34`, pas seulement `LLONG_MAX`.

## Le juge n'est pas indépendant sur H2 ni sur les IDs crédités

Les membres soumis au falsificateur sont d'abord filtrés par le même
`member_admissible` que le sujet. Le replay Cramer rappelle ensuite le même
`group_covers` et le même `det3`. Il parcourt même des triples acceptables non
crédités après l'arrêt à dix groupes : un mutant peut mourir sans affecter le
verdict publié. Une faute primitive cohérente s'auto-confirme. Stocker les
groupes réellement sélectionnés, puis rejuger H2, déterminants, identité Cramer
et union des IDs par une seconde écriture. Ventiler les désaccords de sphère,
d'union créditée et de Cramer. Une faute `>=` sur H2 est aujourd'hui partagée.
Fixture à graver :
`d=(20,0,0)` et
`s1=(10,10,0), s2=(10,-6,8), s3=(10,-6,-8)`. Le triple est plein rang et
entoure l'axe, mais `d dot s_i=||s_i||^2=200` pour les trois : à la sphère
diamétrale ils sont shell, jamais intérieurs.

Le falsificateur échantillonne seulement les deux axes `k e1` et `k e2`, pas
leurs combinaisons ; c'est un stress, non un juge exhaustif du plan des centres.
Il compte en outre tous les intérieurs du nuage, pas seulement l'union des IDs
des groupes crédités. Des points étrangers peuvent donc masquer une faute de
disjonction. Le reçu doit stocker les trois IDs de chaque groupe et publier les
minima `all_cloud` et `credited_union`, ce dernier étant l'autorité du
certificat.

## Mutants et fixtures

- `groupe-pointid-reutilise` est rigoureusement inerte : les triples
  `(i,i+third,i+2third)` sont disjoints par construction, donc désactiver le
  test `used` ne réutilise aucun index. Le run générique rend code `3`, mutant
  survivant. Il faut une vraie liste de groupes concurrents partageant un ID.
- `groupe-egalite-puissance` partage le filtre H2 avec le juge et survit aussi
  en code `3`; la fixture d'égalité ci-dessus doit le tuer avec une seconde
  écriture stricte.
- La fixture du mutant espace linéaire utilise actuellement un troisième site
  qui échoue H2 ; elle teste `cone_contains`, pas le chemin complet. Une fixture
  end-to-end est `d=(20,0,0)`, `s1=(2,3,0)`, `s2=(2,0,3)`, `s3=(2,3,3)` :
  H2 est stricte, le triple est plein rang, `d` appartient au span mais pas au
  cône positif.
- `groupe-axe-seul` ne teste aucun axe de cellule ; il saute simplement H1 sur
  la cible ponctuelle. Le renommer selon la faute réelle.
- `groupe-determinant-nul` exige une fixture dédiée et un H2 rejugé ; son passage
  sur une famille aléatoire n'explique pas la cause.

La fixture actuelle ne reçoit qu'un groupe, pas le passage de `7` à `8`.
Graver la fixture positive suivante de **huit groupes disjoints**, qui reçoit ce
seuil et tue aussi `smax=12` : prendre
`a=(1000,1000,1000)`, `b=(11000,1000,1000)` et, pour `k=0,...,7`,
`r=10+k`, les trois sites relatifs
`(100,r,0)`, `(100,-r,r)`, `(100,-r,-r)`. Les huit triples sont disjoints,
plein rang, H2 stricts et leur cône positif contient `d`. À un centre transverse
approprié, seuls huit intérieurs subsistent : q4 ferme à `smax=11` mais doit
rester ouvert à `smax=12`, qui en exige neuf.

Le greedy doit aussi publier sa perte contre un oracle petit `m`. Une fixture
de neuf vecteurs relatifs, tous de première coordonnée `50`, est :

```text
(50,491,95) (50,485,121) (50,483,129)
(50,187,464) (50,-195,460) (50,-377,328)
(50,121,-485) (50,335,-372) (50,347,-360)
```

Pour `d=(10000,0,0)`, le stride courant n'accepte qu'un triple, tandis que les
triples disjoints d'indices `(3,5,7)` et `(4,6,8)` sont valides. La gate publie
`packing_greedy=1`, `packing_oracle=2` et route la différence au résiduel.

## La comparaison publiée n'est pas dominance 432

Le compteur `ponctuel` appelle `cone_oracle::count_witnesses` pour chaque paire
et tous les sites. Il mesure le certificat ponctuel spindle exact, strictement
plus général que le cutoff dominance 432. Les colonnes `groupes_seuls` prouvent
donc que le groupe apporte quelque chose au-delà de tous les témoins universels
individuels ; elles ne mesurent pas la complémentarité avec l'ordonnance 432.
Cette dernière exige les deux bitsets/relations sur le même univers et le même
owner, puis les quatre masses intersection/différences.

Les nombres `n=150` du commentaire CMake n'ont ni transcript, ni hash d'ELF, ni
commande versionnée. Ils sont une observation, pas un reçu ni une pente.

Deux contrats CLI/reçu sont aussi ouverts. `--fixture` retourne avant les
validations : `--fixture --smax=LLONG_MAX` ou une injection incohérente rend
code `0`. Et sans `--judge`, stdout publie encore `juge accord=OUI` avec zéro
sphère testée ; le statut correct est `NON_EXECUTE`. Le receipt doit pincer ses
sources/ELF, publier caps/bytes et distinguer juge absent d'un accord.

## Complexité et prochaine gate

Le probe balaie les paires, rescane et trie jusqu'à `n` membres, puis appelle
un oracle `O(n)` par paire : son coût est `O(n^3 log n)` et son domaine est
borné à `n<=400`. Ce n'est ni un counter-only factorisé, ni une route 50 k.

La prochaine gate reçoit séparément : génération de crédits, packing disjoint
vérifié ou fail-open, ledger d'IDs, résiduel factorisé, caps de travail/pièces/
octets et HWM. Elle compare les fermetures contre dominance 432 sur le même
univers et exige deux pentes au plus `1,35` avant CUDA.

GCP non utilisé.
