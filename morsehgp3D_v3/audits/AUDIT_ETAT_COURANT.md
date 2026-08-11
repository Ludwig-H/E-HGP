# Audit courant de MorseHGP3D v3

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur et autorité

`HEAD` audité : `40050c4309f624655ab32ac8bd4687f35f364d65`.
Ce commit stabilise le sidecar borné et le différentiel q2. Le worktree
postérieur est concurrent : il ajoute à la sonde q2 la borne `L4`, l'héritage
de témoins et une pile réutilisée, ajuste CMake et consolide les documents.
Ce delta n'est pas confondu avec le reçu de `40050c4`.

Empreintes du rejeu Release :

| objet | SHA-256 |
| --- | --- |
| `prototype/pair_selfjoin_probe.cpp` | `7eaa9c34c684ad9d9dc27e0082726efb47a04274a2d9c5a2c55e30a49963dd38` |
| `CMakeLists.txt` | `0d259d6c35fdbabfaf667eabaddeca4078d858aee05ef5c77d58e04d4cc6c7a5` |
| binaire Release q2 local | `a447be4fc02dcd53da460eaed257cfd74b505eae83e862c4eb335032def668fe` |
| `prototype/validated_hybrid_sidecar.hpp` | `41eee48a4aad0393e1692f295322fcdd3004d951053bec4dc2defdd8462556a1` |
| `prototype/sealed_source.hpp` | `7156c090f0e0690784842579fd9a104e8c1e1279c4d6092e47eed60dfc3b9cee` |
| `prototype/sidecar_factory_gate.cpp` | `97b9443610edaa8cae0e889780d6e2edc066e519c37878bf8e3322ed0b9d3491` |
| `prototype/sidecar_sha256.hpp` | `401df9cccd0cd0a5dc99d06e8836f01797dd37095e8aaafa9b68a59d43f3cb3e` |
| binaire Release sidecar local | `82a38438410e9444eacede515d16474df67e069f2d67f6be354c9d3e485a55fb` |

Empreintes du delta q2 non reçu :

| objet | SHA-256 |
| --- | --- |
| `prototype/pair_selfjoin_probe.cpp` du worktree | `ce65d427b8df6d1feab582cba3350b946516d2b5f3357db80abc65b6ad1f19b8` |
| `prototype/cloud_families.hpp` | `7850a61e7a7e8c1ca366383ef7e21500a17b154f00c9c238c0c310231dd9f7f4` |
| `CMakeLists.txt` du worktree | `08e998216afbb2836329a0a2eadb3dd4848473007cfc9b5b8746aae5fb67b3d0` |
| binaire Release q2 du delta | `23098dc6dbe739b5b5ae213aaebf926190efc2c1b020d5b7182a0afa96469c1b` |

Une modification d'un objet invalide son rejeu. Ce fichier est l'unique
autorité mutable du statut courant; les audits épinglés ne décrivent que leur
snapshot.

## Verdict

Le contrat n'est pas rempli. À 50 000 points et `K=10`, la cible principale
est un p95 `warm_e2e<100 ms`; `warm_e2e<1 s` est le jalon secondaire demandé.
Aucun backend public exact n'est qualifié.

Le commit fait passer la porte locale q2 sur ses 22 tests ciblés : sort paire
par paire, inclusion non compensable, fixtures, mutants et budgets. Un
contre-exemple postérieur montre toutefois que le contrat de cardinalité du
générateur multi-écho et du driver n'est pas reçu. Le commit ferme aussi les
contre-exemples sidecar initiaux sur son périmètre d'oracle borné; ses `6/6`
CTests ciblés passent. Ces résultats n'approchent pas à eux seuls la latence
50 k.

Le verrou produit reste une source q2/q3/q4 exacte, complète et parcimonieuse.
q2 ne produit encore ni census fermé ni `BallActivation`; q3/q4 n'ont aucun
producteur complet mesuré à 50 k; le resolver et le fold bout en bout restent
ouverts. Les reçus G4 existants sont mass-only.

Deux sorties doivent rester séparées : Gamma/v2 exhaustif, avec incidences et
verticales, et le candidat horizontal
`hgp_reduced_normalized_h0_v3`. Une tombstone H0 ne prouve ni l'absence d'un
support, ni celle d'une incidence Gamma ou d'une verticale. Le candidat reste
`public_status=not_claimed`.

## Tests du commit

Après régénération CMake et build Release, la commande ciblée donne :

```bash
ctest --test-dir build/v3 --output-on-failure -R '^(mhgp3v_pair_selfjoin_|mhgp3v_sidecar_)'
```

Résultat : `28/28`, dont q2 `22/22` et sidecar `6/6`.

La suite enregistrée contient 228 tests selon `ctest -N`. Aucun résultat de
suite complète n'est revendiqué ici. La suite en cours a démarré avant le build
du delta q2 et ne peut donc pas le recevoir.

`python tools/check_docs.py` ne parcourt pas le périmètre v3. Les liens et les
règles LaTeX de ce dossier sont donc vérifiés séparément.

## Self-join q2 : prune exact, contrat du driver ouvert

L'audit de la baseline est
[`AUDIT_Q2_SELFJOIN_8A39C53.md`](AUDIT_Q2_SELFJOIN_8A39C53.md).

Le prune est exact : pour une paire `x,y`, le signe de
`(w-x) dot (w-y)` classe l'intérieur strict de sa boule diamétrale. Le sup
AABB séparable est exact, un contact descend et dix `PointId` distincts hors
des extrémités donnent `p+q>=12`. Cela autorise uniquement une tombstone q2 du
quotient horizontal jusqu'à `K=10`.

Le commit `8a39c53` ne comparait que des comptes compensables. `40050c4` ajoute
un sort triangulaire par paire à petit `n`, refuse omission et double
affectation et vérifie que toute paire avec moins de dix témoins atteint une
microtuile. Les portes couvrent :

- terrain et multi-écho avec balayage indépendant;
- contact, neuvième/dixième témoin, coordonnées dupliquées et portée q2/q3;
- omissions d'un enfant croisé, de `R,R` et de la dernière microtuile;
- seuil 9, contact compté strict et duplication compensée;
- frontière exacte du budget, planchers anti-vacuité et codes de sortie.

Le ledger quadratique est limité à `n<=3000`; c'est un juge borné, jamais une
structure produit.

### Contre-exemple de cardinalité multi-écho

Dans `scanline_pass`, la condition `size<n` n'est vérifiée qu'avant un pixel;
après le retour sol, un ou deux échos peuvent encore être ajoutés. La sonde
refuse seulement `size<n`, construit donc parfois l'arbre sur `n+1` ou `n+2`
points, puis dimensionne `C(n,2)`, le fate ledger et l'oracle avec le `n`
demandé. À `n=12500`, `coord=707`, seed `20260810`, famille
`scanline_overlap_multiecho`, cela produit l'échec nominal gravé plus bas.

Avec `--verify-bruteforce`, un identifiant au-delà de `n-1` peut en outre
indexer le tableau de sorts hors bornes. La réparation doit borner chaque push
ou normaliser explicitement le nuage, puis exiger `pts.size()==n` avant la
construction de l'arbre. Une fixture permanente reproduit l'overshoot, tue le
mutant `size>=n` et passe sous sanitizer. Ce contre-exemple vise le contrat
générateur--driver; il ne réfute ni le sup q2 ni `L4`.

### Une phrase de fixture reste fausse

La fixture `q2-vs-q3-scope` démontre correctement que `ab` possède dix
intérieurs q2 et reste l'arête maximale d'un support q3 propre dont le cercle
exclut ces témoins. Elle n'impose plus le sort de parcours, ce qui est juste.
Mais elle imprime encore « ab prunee q2 » alors que le même reçu annonce
`prunes=0` et place les 78 paires en microtuiles.

Une paire H0-inerte n'est pas nécessairement prunée par une borne de bloc
conservatrice. Le texte doit dire « ab a exactement dix témoins q2 et est
H0-inerte q2 », sans revendiquer un sort `PRUNED` absent. Ce défaut de sortie
ne change pas le calcul, mais contredit actuellement le reçu affiché.

## Self-join q2 : coût

Mesures Release locales, un thread, seed `20260810`, feuilles de taille 8,
sans bruteforce. Les compteurs sont déterministes sur cette empreinte; les
temps sont des phases uniques sans chauffe ni p95, et ne sont pas des mesures
G4.

| famille, n | états | visites nœuds | tests ponctuels | paires terminales | phase locale observée |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain, 2 400 | 24 186 | 20 855 916 | 48 301 083 | 144 986 | 0,789--1,129 s |
| scanline simple, 2 400 | 20 600 | 17 667 775 | 40 919 884 | 126 516 | 0,727--0,758 s |
| multi-écho, 2 400 | 32 984 | 29 024 422 | 67 314 546 | 204 657 | 1,056--1,253 s |
| uniforme, 2 400 | 67 668 | 60 454 402 | 140 290 100 | 407 313 | 2,419--2,628 s |
| terrain, 5 000 | 52 198 | 89 691 896 | 217 489 879 | 323 749 | 3,756 s |

À 2 400 points, chaque état visite encore environ 84 à 87 % des 1 023 nœuds
témoins et teste environ 83 à 86 % du nuage. À 5 000, terrain visite environ
84 % des 2 047 nœuds par état. La sortie au dixième témoin ne réduit les
visites que d'environ 0,23 à 0,39 % : le dixième témoin est trouvé tard.

Avec des feuilles de 64, la phase mass-only tombe localement à
`0,022--0,058 s`, mais 20,32 à 56,59 % des paires atteignent les microtuiles;
la famille uniforme rend `NO-GO`. Surtout, le classifieur terminal n'existe
pas encore : profondeur, coquille fermée, owner, `BallKey` et activation ne
sont pas inclus. Ce réglage ne constitue pas une accélération bout en bout.

Le pire cas reste cubique pour recherche et census. Le cap `--max-states`
classe l'exécutable comme falsificateur censuré, même lorsqu'il n'est pas
atteint; il ne peut qualifier un temps produit.

## Delta q2 `L4` et héritage : non reçu

Le worktree implémente l'infimum exact du même triple AABB. Par axe, pour chaque
couple d'extrémités `x,y`, poser `t=clip(x+y,[2*w_min,2*w_max])`; le minimum
de quatre fois `(w-x)(w-y)` vaut `(t-2*x)(t-2*y)`. Le minimum sur les quatre
couples puis la somme des axes donne `L4`; les coins donnent `U4`.

- `U4<0` crédite tout le nœud comme témoin strict;
- `L4>=0` prouve qu'il ne contient aucun témoin strict;
- `L4<0<=U4` descend.

Sous raffinement des blocs d'extrémités, `L4` augmente et `U4` diminue : les
deux décisions sont héritables. `L4=0` permet d'écarter le nœud de la recherche
d'intérieurs, jamais du census fermé qui doit retrouver les contacts. Sous
u16, les expressions doublées tiennent en `int64` signé.

La preuve statique de ces deux transformations est saine. `L4` est l'infimum
continu exact sur le produit des trois AABB et augmente sous restriction. Une
position témoin stricte pour tout `A*B`, hors `A` et `B`, reste stricte et hors
des sous-plages enfants. Ces positions sont toutefois des handles locaux dans
`tree.order`, pas des `PointId` persistants; le reçu doit engager l'arbre et
cet ordre immuables.

La campagne nominale réfute déjà la livraison courante. Avec le binaire
`23098dc6...`, `n=12500`, seed `20260810`, feuilles 8 et la famille
`scanline_overlap_multiecho`, elle termine par :

```text
ECHEC l'identite du ledger : prunees + microtuiles != C(n,2)
exit=1
```

Le budget configuré vaut 900 millions d'états et n'est pas le motif affiché.
La cause est l'overshoot du générateur décrit ci-dessus, pas le lemme `L4`.
Le reçu terminé contient 16 runs : 15 codes nuls et ce code 1. Son SHA-256 est
`2685ceb387f46cb0be2f0a04f7b1ad8afbcaa41c521dad20328c7a4cb5332bc5` :
[`scale_counters_raw.txt`](../receipts/selfjoin_q2_20260811/scale_counters_raw.txt).

Indépendamment de cette réfutation, aucune porte ne compare encore le mode
optimisé à une baseline sur tous les sorts et masses. Les budgets terrain et
l'inclusion unilatérale peuvent survivre à une perte de prune. Aucun mutant ne
vise la formule ou le signe de `L4`, un mauvais handle hérité, son double
comptage, sa propagation au mauvais frère ou le cas « neuf hérités plus un
nouveau »; la fixture aux extrêmes u16 manque aussi.

Le commentaire « une seule allocation par sonde » est trop fort : le `vector`
est réutilisé, mais sans capacité réservée il peut réallouer pendant sa première
croissance. `l4_skipped_points` et `inherited_credits` sont des multiplicités
de travail par recherche, pas des nombres de points ou IDs uniques.
Le commentaire CMake annonce par ailleurs cinq mutants alors que six sont
enregistrés.

La gate minimale exécute les deux modes sur les mêmes petits nuages aléatoires
et adversariaux, exige l'égalité de tous les sorts et masses, puis tue les
mutants ci-dessus. Aucun gain de performance du delta n'est reçu avant cette
gate. Les temps de la campagne actuelle sont de toute façon contaminés par une
suite complète et d'autres sondes concurrentes.

Les compteurs des quatre runs à 50 000 dont le ledger ferme restent des
diagnostics de travail, pas une réception du delta :

| famille | états | visites `L4` | tests ponctuels | paires terminales |
| --- | ---: | ---: | ---: | ---: |
| terrain | 710 396 | 240 347 699 | 495 522 203 | 6 205 971 (0,50 %) |
| scanline simple | 367 890 | 53 240 637 | 86 172 879 | 3 598 676 (0,29 %) |
| multi-écho | 950 500 | 393 107 357 | 801 949 159 | 8 109 344 (0,65 %) |
| uniforme | 1 580 440 | 723 579 105 | 1 364 858 170 | 14 851 373 (1,19 %) |

Chaque visite `L4` et chaque test ponctuel effectuent au moins douze produits
entiers. Même sans compter les évaluations `U4`, cela représente environ 1,67
milliard de produits pour scanline simple et 8,83 milliards pour terrain. Le
rescan racine reste donc le verrou concret. Le prochain reçu doit publier
séparément `L4`, `U4`, feuilles, crédits, allocations et high-water; la route
produit à comparer reste Yao48/LBVH avec census terminal, pas une extrapolation
du faible pourcentage de microtuiles.

La comparaison produit reste Morton/LBVH + Yao48 strict + classifieur terminal
et census fermé. Le self-join est un oracle, un falsificateur ou un second
prune tant que ses masses ne justifient pas davantage.

## Sidecar : périmètre reçu et limite restante

Les contre-exemples connus sont fermés dans `40050c4` :

- jeton et reçu à constructeurs privés, non trivialement copiables;
- index trié par centre puis niveau exact, qui refuse `[r1,r2,r1]`;
- points, `Sphere.base`, numérateurs et dénominateur bornés avant géométrie;
- pgcd sur magnitudes non signées;
- support canonique reconstruit et toute autre déclaration refusée, de sorte
  que evidence, digest et fold utilisent une seule convention;
- sérialisation SHA-256 little-endian taggée et versionnée, digest final du
  certificat complet et self-test FIPS exécuté dans la factory;
- reçu déplacé invalidé et identité contractuelle du producteur ajoutée.

Les `6/6` CTests ciblés passent. Une campagne sanitizer fraîche reste utile
pour recevoir l'absence d'UB sur l'empreinte finale, indépendamment de la note
de livraison.

Le champ `producer_code_digest` ne hache toutefois pas le source, l'ELF ou un
manifeste de build. `SealedSourceProducer` hache seulement la chaîne littérale
`mhgp3v_sealed_source_flat_catalogue_v1`, et `claims_complete_family()` ne
compare pas ce digest à une valeur attendue. C'est un identifiant de version
sémantique, pas un SHA du code exécuté. Il faut le renommer comme tel, ou lier
et vérifier un vrai digest de producteur avant de le présenter comme identité
de code.

Les nouvelles métadonnées n'ont pas encore de mutants de mauvais contrat,
profil, schéma, statut terminal ou digest. La gate appelle aussi le self-test
SHA directement avant de construire le sidecar; elle ne prouve donc pas à elle
seule que l'appel interne de la factory resterait obligatoire. Une injection
ciblée doit faire échouer la factory sans prétest externe. La fixture de grille
couvre la borne basse hostile, pas encore la borne haute au-delà de 65535.

De même, le schéma de tâches est un identifiant constant; aucun ledger de
counts prévus/remplis/consommés n'est porté. Pour le producteur séquentiel
borné, l'autorité reste la terminaison du code de confiance `flat_catalogue`
avec `smax>=n`. Un futur producteur streamé doit posséder un reçu de tâches
complet et ne peut hériter cette convention.

Le pipeline validé exige `smax>=n`, refuse `smax>32`, énumère deux fois le
catalogue et rescane le nuage par générateur. Il est donc classé à jamais
oracle CPU `n<=32`, jamais source chaude 50 k.

## Ancres q3/q4

La note
[`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md)
réexprime les filtres P0 exacts déjà documentés pour `JungChordCsrTile`. Pour
une paire réellement diamétrale d'un support propre positif, neuf témoins q3
ou huit q4 strictement intérieurs pour tout le disque de Jung rendent l'ancre
H0-inerte.

Deux autres certificats restent distincts : la profondeur fermée de demi-boule
est un filtre terminal sans hypothèse de diamètre, sous un support propre
positif q3/q4 certifié, et le schéma conditionnel de center-cover emploie une
banque dépendant de chacun de ses 64 patches. Son composant complet n'est pas
reçu. Aucun mécanisme ne doit être présenté comme l'unique prune.

q2 emploie le total des intérieurs diamétraux; q3/q4 peuvent employer la
profondeur. Leurs résiduels sont incomparables et chaque lane porte sur
l'univers original des paires. Les certificats prouvent la sûreté d'un rejet,
pas la parcimonie : le nombre d'ancres peut rester quadratique et une recherche
naïve de témoins cubique.

La première implémentation autorisée est un falsificateur count-only avec sort
par paire et oracle exhaustif `n<=32`, contacts q3/q4 exacts, puis histogrammes
`delta<9` et `delta<8`. Aucun port G4 ne précède l'admission de ces masses.

## Reçus G4 et budget

Les reçus
[`cell_50k_raw.txt`](../receipts/g4_massonly_20260811/cell_50k_raw.txt) et
[`mask_scale_raw.txt`](../receipts/g4_massonly_20260811/mask_scale_raw.txt)
sont des diagnostics CPU sur l'hôte G4; le GPU n'a pas été utilisé. Après le
prune d'axe, les masses atteignent 2,86 milliards en q2, 131,76 milliards en
q3 et 9,97 billions en q4. Les `0,174--29,153 s` count-only ne sont ni un
débit de source ni un temps `warm_e2e`.

Pour l'objectif secondaire d'une seconde, l'enveloppe provisoire est
`40/200/200/300/200/60 ms` pour transfert+LBVH, source+cover, cordes,
shallow+exact, reducer et réserve. Source+cover+cordes au-dessus de 400 ms
chaud classe la route no-go; passer ce sous-budget ne qualifie pas le pipeline
complet.

## Ordre des prochaines portes

1. Reproduire et graver l'échec nominal à 12 500, puis recevoir `L4` et
   l'héritage par différentiel baseline, mutants ciblés et extrêmes u16.
2. Corriger le libellé contradictoire de la fixture q2, mesurer seulement le
   delta reçu, puis construire le classifieur terminal fermé et comparer les
   mêmes masses à Yao48/LBVH.
3. Rejouer le sidecar sous sanitizers et corriger la sémantique du digest de
   version; conserver ce chemin seulement comme oracle `n<=32`.
4. Prototyper indépendamment cœur de Jung, profondeur fermée et center-cover
   sur toutes les paires, avec ledger de sort et oracle exhaustif borné.
5. Recevoir `BallActivation`, census fermé, resolver latent et fold horizontal
   contre Gamma exhaustif à petit `n`.
6. Porter sur G4 uniquement les primitives dont les masses sont admises, puis
   mesurer build, source, certification, census, resolver, fold, payload et
   p95 `warm_e2e` complet à 12,5 k, 25 k et 50 k.

Une insuffisance de ressource refuse atomiquement; elle ne publie jamais un
préfixe. Aucun tableau global de paires, tuples, cellules, faces, cofaces ou
incidences n'entre dans le chemin produit.

GCP non utilisé.
