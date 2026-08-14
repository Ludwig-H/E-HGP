# Contre-audit — `Q4SeedAxisTopR4`

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot et contrat

Snapshot logiciel relu :
`HEAD=3507b5ee4c7bcf1531f263737c406aedf0a6a100`, commit
`name the kernel after its lane and make it carry real identities`.

| fichier | SHA-256 au `3507b5e` |
| --- | --- |
| `prototype/q4seed_axis_topr4.hpp` | `e818b99960e4d90e0c511ef28c023e7793bda31da6608a9518c1f549d8d992be` |
| `prototype/q4seed_axis_topr4_probe.cpp` | `b41f7714f2df96bee467f820fe3990e85483d1e4499e207ab27882c672675f9a` |
| `CMakeLists.txt` | `d809d02f43dd231844102e1dd1087ae73ef7c236f284c32ea566353ded5e4ca3` |

Historique utile : `069acf7` introduisait un `AxisTop8` fixé à huit malgré son
CLI ; `840a2e2` l'a paramétré par le seuil de mort. Le commit courant remplace
entièrement ce prototype par le nom et l'API contractuels.

L'auditeur n'a modifié aucun fichier de code. Le noyau est interne à `Lane4` :
son entrée `Q4Seed3` est créée par `Lane4`, jamais lue depuis `Lane3`. Il n'est
ni un support q3, ni une sortie q3. `Lane2`, `Lane3` et `Lane4` restent trois
énumérations indépendantes ; aucune Delaunay de quelque ordre que ce soit n'est
admise.

## Verdict court

Le commit ferme cinq défauts importants de la première révision :

- `r4=smax-3` paramètre sélection, mort et borne ;
- les vrais IDs permanents et le shell persistant sont transportés ;
- le juge classe `insphere_j<0`, `==0` et `>0`, puis compare `I_B/U_B` ;
- `MORT_GAP` est un verdict du sujet, confronté à un oracle indépendant ;
- un mode exact-once compare le multiensemble global q4 au brute force et
  mesure des `Q4Seed3` déjà morts pour q3, sans en dépendre.

La réception reste cependant bloquée par une troncature silencieuse exacte du
shell : le buffer `Census.shell` peut perdre un vrai `PointId` sans rendre de
fate d'overflow. La gate accepte en outre encore une dégénérescence en ne faisant
que la compter. Ce P0 précède toute nouvelle session G4.

## Rejeux frais

Commandes :

```text
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --target mhgp3v_q4seed_axis_topr4_probe --parallel
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_q4axis'
```

Résultat : `36/36` CTests passent en `38,24 s`, dont les seuils non nominaux
`smax=7/14`, cinq familles non vides, la contre-famille `two_lines`, trois
exact-once et neuf mutants. Cela reçoit utilement la branche régulière bornée ;
cela ne mord pas la capacité réelle du shell ci-dessous.

Contre-fixture indépendante compilée seulement dans `/tmp` :

```text
verdict=OUVERT entrants=48 sortants=49 n_shell=99 attendu=100 range=97
```

## P0 — `U_B` est tronqué sans fate

Prendre le `Q4Seed3` aigu owner
`a=(96,108,100)`, `b=(108,96,100)`, `x=(92,96,100)`. Ajouter 49 vrais
`PointId` distincts de position `(100,100,110)` et 48 vrais `PointId` distincts
de position `(100,100,92)`. Tous les 97 sites ont la même racine axiale ; la
sélection reste `OUVERT`, sans overflow, avec 48 entrants et 49 sortants. Pour
l'un de ces apex, le shell exact contient les trois IDs du seed et les 97 IDs
égaux, soit 100 IDs.

Le buffer courant a pour capacité `32+64+3=99`. `census_replay` continue son
range-report, incrémente `range_reports=97`, mais cesse silencieusement
d'écrire au 99e slot. Il rend donc une liste fausse avec `degenere=true`, sans
`required_count`, overflow ou verdict non consommable.

Réparation minimale : capacité prouvée
`3+kCapShell+2*kCapRacines=163`, compte requis et fate typé en cas de dépassement.
Réparation préférable : pour un apex déjà prouvé shallow, tout site de racine
égale appartient nécessairement aux groupes complets retenus ; reconstruire le
shell depuis `entrants/sortants` évite le second scan global. Dans les deux cas,
un overflow ou un shell extra termine en `PENDING_CAP` ou
`unsupported_degeneracy`, jamais en liste partielle.

La preuve du replay porte seulement sur un apex retenu avec profondeur
strictement inférieure à `r4`. L'API doit imposer ces préconditions avant de
publier `I_B/U_B` ; appelée sur un apex profond ou après un verdict
`DEBORDEMENT`, elle peut légitimement omettre des non-extrêmes et ne doit pas
prétendre rendre un census exact.

Enfin, le probe trie puis applique `unique` au shell sujet avant comparaison.
Cette normalisation masque un doublon accidentel d'ID. Il faut vérifier
séparément l'unicité brute, puis comparer la liste canonique.

## P0 — groupes bornés ne signifie pas IDs bornés

Le théorème borne `2*(r4-p)` **groupes de racines orientés**, jamais le nombre
de sites. Pourtant la fin de `campagne` rejette `cand_max>2*r4`. Cette assertion
contredit le théorème et peut transformer un plateau valide en « désaccord » au
lieu d'un fate de dégénérescence/capacité. Le reçu de mort tient en
`2*r4-p` groupes ou en RLE avec multiplicités, pas nécessairement en autant
d'IDs.

La gate compte `c.degenere`, mais un run nominal peut rester vert avec ce compte
non nul. Sous le profil `RelevantGP`, toute égalité hors support ou shell
persistant doit rendre `unsupported_degeneracy`; hors de ce profil, une
politique de plateau reçue est nécessaire. Un compteur n'est pas un fate.

## `MORT_GAP` — formule réparée, fixture permanente encore requise

Une première version non suivie, SHA-256 commençant par `df99`, évaluait
seulement l'intervalle après chaque racine. Un groupe contenant à la même racine
des entrants et des sortants peut être profond des deux côtés mais shallow à
la racine elle-même, où tout le groupe est shell ; cette version rendait alors
un faux `MORT_GAP`.

Le commit `3507b5e` corrige la formule : bout gauche fermé, racine exacte puis
intervalle suivant, avec comparaisons strictes à la racine. La fixture u16 à
graver est : seed
`(100,100,100)`, `(100,110,110)`, `(110,100,110)` ; sept permanents
`(100,101,101)` à `(100,105,105)`, puis `(101,100,101)` et `(102,100,102)` ;
apex `(110,110,100)` et shell opposé `(98,100,104)`. Les deux racines sont
égales au bout droit, la vraie profondeur minimale vaut sept et le verdict doit
rester `OUVERT`. Elle falsifie l'ancien snapshot et protège simultanément
égalité, shell et asymétrie du bout droit.

Le mode exact-once courant ne saute pas `kMortGap` avant de parcourir les
candidats. Il peut donc rester vert même si une future régression ferme à tort
un seed : ce mode teste le primary, mais pas encore l'usage causal de la mort.
Ajouter une variante où `kMortGap` supprime réellement la branche, confrontée
au brute force global.

## P1 — exact-once et options

Les trois gates exact-once régulières passent et le primary choisit bien le plus
petit vrai `PointId` parmi les deux `Q4Seed3` aigus. Leur oracle accepte toutefois
les supports par positivité et profondeur sans classifier le shell complet ;
elles ne reçoivent donc pas un plateau ni le fate `RelevantGP`.

Le CLI emploie toujours `atoll` : suffixes et overflows sont acceptés, plusieurs
modes s'écrasent, des options étrangères à une fixture sont ignorées,
`coord` n'est pas prévalidé u16, les planchers négatifs passent et `threads`
n'a pas de cap. Réception : parse entier strict, options admissibles par mode,
`min_*>=0`, domaine u16 et `1<=threads<=cap`.

Deux mutants restent sémantiquement corrélés : `a8-signe-b-inverse` et
`a8-abs-avant-tri` réalisent la même inversion sur les sortants. Les deux tests
verts ne prouvent donc pas deux fautes indépendantes.

## Portée industrielle

Le sujet rescane tous les sites pour chaque racine : coût quadratique par
`Q4Seed3`. La campagne ajoute l'énumération des seeds et l'oracle apex, avec un
pire coût en puissance cinq. Elle reçoit un oracle CPU borné, ni un générateur
WSPD/Morton q4, ni une range-query best-first, ni des blocs, continuations,
octets/HWM ou une route 50k.

Le raccord industriel reste exclusivement dans `Lane4` :
`NeutralPairPartition -> PairAnchor4 -> Q4Seed3Block -> AxisTopR4 -> Positive4`.
Il doit mesurer `seed_blocks/splits`, visites Morton, comparaisons larges,
ties/overflows, morts par cause, octets et HWM. Aucun résultat, cap ou verdict
de `Lane3` ne figure dans cette chaîne.

## G4

Une tentative concurrente, non lancée par l'auditeur, a échoué avant build car
le `terminationTimestamp` GCE n'a pas pu être certifié. Le garde interne a
arrêté la génération exacte `2026-08-14T13:00:56.283-07:00` et certifié la cible
`TERMINATED`. Elle ne produit aucune mesure CPU/GPU du noyau.

La cible d'une seconde pour 50k reste entièrement ouverte. GCP non utilisé par
l'auditeur.
