# Contre-audit — localité, arités et dimensionnement sparse au snapshot `8c00ab0`

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Provenance

Snapshot audité : `8c00ab07695ef353e673ab73a778a6f260c87509`.

| objet | SHA-256 |
| --- | --- |
| `prototype/certified_locality_probe.cpp` | `87f7e77d914f59fbf846a0d7a8476a420f328a9604b32cc648606d2c072ca43a` |
| binaire Release reconstruit | `a54c2971e7908ae21d283b60d3a1342e20652358c9a124891d0983e834798bf1` |
| `CMakeLists.txt` | `899de73aedff6da6f68c10ed6b8821483b4dc2a91bdfdba56892dbe4c66a5534` |

Le snapshot déclare 28 CTests `mhgp3v_locality_*`, parmi 440 CTests
configurés. Aucun ne lance le nouveau mode `sparse`. Les six portes d'arité et
de fenêtre `431--436` passent `6/6` en `1,16 s` sur la machine partagée. Ce
temps n'est pas un benchmark.

Le fichier a évolué rapidement avant ce snapshot. Les audits de `154c107` et
`0e982bf` restent historiques : ils ne décrivent pas le code présent. Les
conclusions ci-dessous sont pincées aux octets du tableau.

## Verdict

Le lemme de localité par calottes reste un filtre mathématique sûr. La
fermeture par cône et le différentiel q2 par identités sont des améliorations
utiles. Le prototype n'est toutefois ni une source directe complète, ni un
dimensionnement reçu de `directes + gateways`, ni une route industrielle :

- q3/q4 reposent encore sur une fenêtre de supports non certifiée ;
- le code peut réussir avec une fenêtre connue incomplète ;
- le nouveau mode `sparse` dimensionne les facettes d'un sous-univers
  heuristique, appelle ces tuples « cofaces directes » sans fermer leur shell,
  puis balaie le nuage pour chaque facette ;
- aucun record, `BallKey`, owner rejouable, reçu ou fold n'est produit ;
- aucune pente 12,5/25/50 k ni borne mémoire ne reçoit la voie.

Le snapshot est un falsificateur et un instrument de masse CPU borné. Il ne
justifie ni un port G4, ni le contrat une seconde.

## 1. Faux vert permanent de saturation q3/q4

Commande rejouée sur les octets pincés :

```bash
build/v3/mhgp3v_certified_locality_probe --mode=arity --points=70 --family=uniform --grid=4 --seed=1 --kmin=4 --max-neighbours=69 --support-window=20 --threads=2
```

Résultat : code `0`, `q2/q3/q4=681/795/174`. Le juge exhaustif avec une
fenêtre suffisante donne `681/884/202`. Le probe imprime seulement que la
mesure est recevable si la fenêtre sature ; il ne la refuse pas.

`unclosed` ferme le balayage d'intériorité grâce à Jung. Il ne prouve pas que
les partenaires de support absents de la fenêtre ont été exclus. Seul
`--judge-census=1` compare l'arité locale à l'exhaustif, et ce juge refuse
`n>400`. Les tableaux à `n=1 500` ou `4 000` ne peuvent donc pas être dits
exacts ou saturés avec ce mécanisme.

Le CTest `mhgp3v_locality_fenetre_non_saturee` est vert parce qu'il active le
juge qui détecte le défaut. Il ne tue pas le mutant opérationnel « publier la
même fenêtre sans juge » reproduit ci-dessus.

Porte requise : toute exécution de mesure q3/q4 doit porter un certificat de
saturation indépendant de `n`, ou rendre un code non nul. Une comparaison de
deux fenêtres finies n'est qu'un diagnostic, pas une fermeture.

## 2. Portée réelle des juges

Le juge q2 directionnel du successeur `6693639` compare désormais des
identités de paires et corrige une faiblesse des snapshots antérieurs. Le juge
des trois arités continue à comparer seulement trois cardinalités et partage
`q3_ball`, `inside_q3`, `q4_ball` et le déterminant InSphere avec le sujet. Il
ne compare pas :

- l'ensemble des supports ;
- les identités des intérieurs stricts ;
- le shell fermé ;
- le support minimal retenu ;
- la `BallKey`, le niveau réduit ou l'owner ;
- les cofaces directes et leurs bras.

Une omission et une émission surnuméraire peuvent donc se compenser. Le texte
« génération locale exacte sur les trois arités » doit se lire comme « mêmes
comptes sur cette fixture avec géométrie partagée ». Une gate de source compare
des records canoniques complets à l'oracle rationnel indépendant.

## 3. La mesure d'extra-shell ne mesure pas les supports multiples

Le commit `a329554` a d'abord interprété « un point hors support est sur la
sphère » comme « support minimal non unique ». Cette implication est fausse.
Pour `A=(0,1,0)`, `B=(2,1,0)` et `C=(1,2,0)`, `AB` est l'unique support
minimal positif de la boule diamétrale, tandis que `C` est une extra-shell; le
triangle droit n'est pas un autre support propre positif.

La note a été corrigée en
[`NOTE_CLAUDE_MESURE_PORTE_REGULIERE_20260812.md`](NOTE_CLAUDE_MESURE_PORTE_REGULIERE_20260812.md).
Les taux `4,172/11,478/11,390 %` restent des fractions de tuples émis avec une
extra-shell observée, pas des taux de boules ou de supports multiples. Leur
campagne ne conserve ni commande et seed complets, ni empreintes, ni log brut;
sa fenêtre de support 48 n'est pas certifiée à `n=1 500`. Ils constituent un
signal diagnostique de l'échec possible de `shell=support`, pas une mesure
reçue de la porte régulière.

Le texte du binaire pincé imprime encore « donc support minimal non unique ».
Cette sortie est mathématiquement fausse même si le compteur d'extra-shell est
utile.

## 4. Audit du nouveau mode `sparse`

### Reproduction et provenance manquante

Le message du commit annonce, sur `terrain n=600`, `38 391` cofaces,
`107 233` bras et `54 811` facettes. La commande naturelle avec les paramètres
publiés et la seed par défaut donne sur les mêmes octets :

```bash
build/v3/mhgp3v_certified_locality_probe --mode=sparse --points=600 --family=terrain --grid=4 --seed=20260812 --kmin=10 --support-window=48 --threads=2
```

Elle rend le code `0` et publie `38 641` tuples appelés cofaces, `108 226`
bras et `54 900` facettes, puis `34 058/15 358/5 484` dans les branches
`|J_F|=0/1/>=2`. La différence n'est pas une réfutation des valeurs du commit;
elle montre que la CLI, la seed et les octets de leur campagne ne sont pas
reconstructibles depuis le texte. Aucun reçu brut ne les lie.

### Le numérateur n'est pas la source directe

Le mode prend, pour chaque owner, les 48 premiers partenaires admissibles puis
énumère tous leurs couples et triples. Il ne prouve pas qu'un support q3/q4
hors fenêtre est absent. Il ferme les intérieurs d'un tuple conservé, mais ne
ferme pas son shell et ne réunit pas les supports par boule. Un tuple avec une
extra-shell pertinente ne satisfait pas la porte régulière de la source
directe normalisée.

Les quantités affichées sont donc, au mieux, celles des tuples `U union I` et
de leurs suppressions de support dans le sous-univers de fenêtre. Elles ne sont
pas le nombre des cofaces directes terminales, de leurs `BallKey` ou des bras
du produit. L'égalité théorique `Q=U union I` devient une coface directe reçue
seulement après fermeture de `I`, du shell, de l'owner et de la déduplication
de boule.

Le mode inclut aussi les paires sans intérieur, dont les suppressions sont les
singletons de la lane `k=1`. Cette lane possède une autorité EMST séparée dans
l'architecture candidate; l'inclure gonfle la branche zéro et mélange deux
contrats de source.

### Les branches `J_F` sont relatives et le coût dominant est caché

Pour chaque facette effectivement produite, la miniboule du petit ensemble est
calculée exactement et le scan global classe correctement `J_F` en zéro, un ou
au moins deux. Ce résultat positif est relatif à la liste de facettes
heuristique.

L'implémentation parcourt ensuite jusqu'à `n` points pour chacune des `54 900`
facettes. Elle ne compte ni ces tests, ni les recherches de support de la
miniboule, ni les octets et le high-water; le chrono `generation` est arrêté
**avant** ce second étage. À 50 k, ce schéma `facettes * n` est précisément le
scan global que la route industrielle doit remplacer par une requête LBVH
fermée. Le fait `beta(F)<beta(Q)` n'implique aucune inclusion de boules aux
centres différents et ne permet pas de réutiliser la fenêtre de `Q`.

Pour `|J_F|=0/1`, un nouveau certificat local ancré sur le support de `F` peut
fermer la requête. Pour `|J_F|>=2`, deux témoins authentifiés suffisent à
choisir la branche, mais leur découverte doit elle-même être complète pour le
tie-break canonique demandé. Budget épuisé signifie `unresolved`.

### Aucun test ni reçu du nouveau mode

Les 28 CTests de localité n'invoquent jamais `--mode=sparse`. Aucun oracle ne
compare facettes, miniboules, `J_F`, déduplication ou branches. Le mode ne
construit ni `M(F)`, ni gateway, ni carrier, ni quotient, ni fold. Son succès
ne valide donc aucune des hypothèses du théorème sparse.

## 5. Fermeture par cône : progrès mécanique, pas borne de travail

La route `--closure=cone` évite le `full_scan` explicite dans le chemin
directionnel. Chaque cellule ouverte repart néanmoins de la racine du LBVH et
peut visiter tout l'arbre; le census d'une paire repart lui aussi de la racine
et une activation à moins de dix intérieurs exige une fermeture complète.

Sur un snapshot `6693639` isolé, le différentiel `terrain n=900` retrouve les
mêmes `17 023` identités en scan et en cône, mais le cône compte environ
`10,14` millions de visites et prend `4,649 s`, contre `1,386 s` pour le scan
sur la machine partagée. Entre `n=600` et `1 200`, ses visites passent de
`5,41` à `15,96` millions, pente locale voisine de `1,56`. Ces secondes ne
sont pas des benchmarks; les compteurs suffisent à interdire le claim
« travail proportionnel au contenu du cône, jamais au nuage ».

La fermeture est un oracle utile et peut gagner sur d'autres distributions.
Elle ne fournit aucune pente industrielle ni garantie sous-linéaire.

## 6. Autres faux verts et surclaims

1. Le parseur `from_chars` ne vérifie pas que toute la chaîne a été consommée.
   `--points=5junk` est accepté comme `n=5` et rend le code `0`.
2. Le mode `profile` affirme un coût `O(sum M*) états`, mais omet les visites
   kNN, cellules, heaps, allocations et recherches de fermeture. Sur la
   commande précédente à cinq points il publie même zéro état malgré 180 tests
   de sommets.
3. Le facteur `8*48=384` compare deux volumes idéalisés; il ne démontre ni le
   rapport au travail réel, ni la cause de la superlinéarité mesurée.
4. Le triangle/croissant cité dans la note de solution est un commentaire, pas
   une fixture permanente exécutée avec les dix témoins nécessaires.
5. Le mode normal reste count-only : pas de census fermé matérialisé, coquille,
   rang, `BallKey`, `BallActivation` ou payload contractuel.

## 7. Gates avant consommation par la route Gabriel/gateway

1. Graver la contre-fixture entière q3 : triangle u16 plus dix témoins qui
   tombstonent une arête sans toucher sa boule circonscrite.
2. Comparer des records `(BallKey,U,I,E,beta,owner)` par identités à un oracle
   indépendant, avec omission et doublon injectés séparément.
3. Certifier toute fenêtre q3/q4 sans le juge exhaustif; toute fenêtre ouverte
   rend un code non nul.
4. Séparer extra-shell, support multiple, représentation multiple de boule et
   bloc saturé haut-rang dans les compteurs et reçus.
5. Exclure la lane EMST du dimensionnement des gateways d'ordre supérieur.
6. Remplacer le scan `facettes*n` de `J_F` par une recherche exacte reçue;
   compter nœuds, feuilles, tests, profondeurs, octets et high-water.
7. Produire `M(F)`, la confluence, le resolver terminal et le quotient de lot,
   puis comparer le journal normalisé à Gamma sur E5 et les plateaux.
8. Mesurer les quatre familles à `12 500/25 000/50 000` avec deux pentes
   successives, avant seulement un port G4.

GCP non utilisé.
