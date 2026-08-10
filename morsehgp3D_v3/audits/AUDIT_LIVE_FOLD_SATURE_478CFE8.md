# Réception constructive — fold saturé de `478cfe8` à `23f12af`

Date : 10 août 2026 UTC.

Périmètre : nouveau candidat `prototype/saturated_fold.hpp` et branche
`--subject-fold 1` du juge Gamma, committés et poussés à
`23f12afb4f3896bfd16f8f1281d5f1a3d4f89f22`. Aucun fichier produit n'est
modifié par l'auditeur.

Cadre : `phase=exploration_v3_hors_registre`, backend CPU candidat jugé par
Gamma exhaustif, profil u16, `public_status=not_claimed`.

Snapshot produit pincé :

- `prototype/saturated_fold.hpp` SHA-256
  `1be6e58ba720b2b5cb95dc4c59bdb62a62e4ea197bdaa35cd78de95faaaf6931`;
- `oracle/gamma_forest_judge.cpp` SHA-256
  `40f62e3a767e5fbfb17c1eb723ef8a54f9176e0b084ab23c0bd706e294f1eadd`;
- `CMakeLists.txt` SHA-256
  `b599fee0d59d51a3f32c8077be28b932f0f1965515d6e85280e7eec051c2c041`.

Un configure/build Release frais en répertoire temporaire puis la sélection
`^mhgp3v_gamma_judge_` passent 9/9 en 7,97 s.
La suite complète stable affiche 97/97, zéro échec, en 466,25 s. La commande
Claude passe encore par `ctest -j2 2>&1 | tail -3`, donc son code shell est celui
de `tail`; le journal `LastTest.log`, SHA-256
`fb5bf552c9fcae8993fe189dab42a2b75e7a92665fc10c3816ce4009bc8b3315`,
contient toutefois les 97 entrées, aucune faute ni `Not Run`. Sources, CMake et
binaire sont restés stables pendant ce run; les builds mutants de l'auditeur
étaient isolés sous `/tmp`.

## Résultat positif majeur

Le choix mathématique est le bon. Pour chaque ordre `k`, le fold traite chaque
saturé complet `M` comme un générateur connexe, relie deux générateurs lorsque
`|M intersection N|>=k`, puis publie l'union de leurs membres par composante.
D'après S.4 de
[`TOUR_BOULES_SATUREES.md`](../../docs/math/TOUR_BOULES_SATUREES.md), ces
composantes sont exactement celles de Gamma **si la famille de générateurs
d'entrée est complète**.

Le prototype respecte déjà quatre points difficiles :

1. tri des niveaux par `sphere_cmp_beta`, sans `double`;
2. activation de toutes les sphères d'un niveau avant classification;
3. intersection exacte de listes triées et union DSU au seuil `k`;
4. couverture fermée calculée seulement après le commit du lot entier.

Sur le premier snapshot stable observé, la campagne générique passe de 87
ordres jugés avec la chaîne v2 à **90/90 ordres jugés**, dont 78 hors
dégénérescence et 12 dégénérés, tous en accord de couverture. La grille
saturée passe de 20 ordres `k=1` jugés et 40 censures `k=2,3` à **60/60 ordres
jugés et 60 accords de couverture**. Le juge compare aux deux coupes sur
l'union des niveaux; il compte 2 386 niveaux sur la campagne saturée, dont 682
niveaux sujet étrangers. C'est le premier signal positif direct que le quotient
par saturés répare les censures multiplicataires sur ces entrées.

Ce fold ne matérialise ni mosaïque de Delaunay d'ordre supérieur, ni graphes de
Johnson, ni tous les sous-simplexes de ses grands générateurs. C'est donc une
direction compatible avec l'invariant d'allègement de MorseHGP3D.

La sensibilité du cœur est elle aussi positive : un mutant hors dépôt remplace
le seuil d'intersection `>=k` par `>k`. Les deux portes fold deviennent rouges,
avec 90/90 écarts de compte sur la campagne générique et 60/60 sur la saturée.
Le poids exact `|M intersection N|`, les unions DSU et les trois ordres
contribuent donc réellement aux couvertures comparées.
Un second mutant force le seuil `1` pour tous les ordres : les deux portes
rougissent, tandis que seul `k=1` reste correct. Les seuils $k=1,2,3$ sont donc
sémantiquement distingués par les campagnes, même si les compteurs par ordre ne
sont pas encore publiés.

## Preuve conditionnelle du cœur actuel

Supposons que le catalogue contienne exactement un record par générateur
saturé, avec son niveau et tous ses membres. Par induction sur les lots de
niveau : avant le lot, le DSU représente `H_k(<a)`; activer tous les nouveaux
générateurs et toutes les paires d'intersection au moins `k` construit
exactement `H_k(<=a)`; S.4 identifie ses composantes à celles de Gamma; l'union
des membres donne leur couverture. Figer les identifiants des racines strictes
avant les unions permet alors de distinguer naissance, continuation et
multifusion sans ordre séquentiel entre ex æquo.

Cette preuve crédite le **fold d'une famille complète**. Elle ne prouve pas que
la famille fournie par la source actuelle est complète.

## Verrous à fermer sans perdre ce progrès

### 1. `smax>=n` retire une censure; il ne certifie pas la source

Le commentaire courant affirme que `smax>=n` donne la famille saturée entière.
Cette inégalité empêche bien la troncature par rang, mais ne prouve pas que la
navigation de `flat_catalogue` a visité tous les supports bien centrés, agrégé
tous leurs témoins et dédupliqué toutes les boules. Les accords 90/90 et 60/60
sont une excellente validation bornée, pas ce théorème universel.

Au régime produit, l'obstruction est même structurelle :
`mhgp::kMaxRank=32`, donc l'entrée actuelle interdit `smax>=50000`. Un run G4
50 k du fold alimenté par ce `Catalogue` serait nécessairement une
`partial_refinement` S.6, jamais une qualification exacte. La solution est un
record séparé `SaturatedGenerator` : support témoin de taille au plus quatre,
niveau exact, span de membres complet potentiellement grand, digest et
certificat de source. Il ne doit pas réutiliser la borne de rang de
`CriticalSphere`.

La troisième vérité doit rester indépendante : énumérer tous les supports de
tailles un à quatre avec `exact_geometry`, classifier leur boule fermée sur le
nuage, dédupliquer les saturés, puis comparer successivement :

1. `tour exhaustive == Gamma exhaustif`;
2. `catalogue == tour exhaustive`;
3. `fold(catalogue) == fold(tour exhaustive)`.

Le header actuel est donc un **candidat produit** très utile; il ne doit pas
être rebaptisé oracle de sa propre source.

### 1 ter. La troisième vérité indépendante donne déjà un résultat positif

Un harness temporaire hors dépôt a ensuite réalisé exactement la porte
demandée : énumération indépendante de tous les sous-ensembles de tailles un à
quatre avec `exact_geometry::exact_miniball`, classification fermée de chaque
nuage par `side_of<=0`, déduplication par saturé, puis comparaison de la famille
et du niveau rationnel exact à `flat_catalogue(pts,n)`.

| campagne | mode source | générateurs produit | générateurs oracle | manquants | extras | niveaux différents |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 130 nuages, `n=8`, coord 40 | normal | 8 491 | 8 491 | 0 | 0 | 0 |
| 120 nuages, `n=9`, coord 4 | normal | 9 562 | 9 562 | 0 | 0 | 0 |
| 110 nuages, `n=8`, coord 40 | owner | 7 180 | 7 180 | 0 | 0 | 0 |
| 110 nuages, `n=9`, coord 4 | owner | 8 770 | 8 770 | 0 | 0 | 0 |

Les 470 nuages et 34 003 générateurs rendent aussi zéro statut, miniboule, record
malformé ou doublon de niveau fautif. C'est une recertification matérielle de
la famille complète à petite taille, indépendante de la reverse-search et du
fold. Elle renforce fortement le résultat 90/90--60/60.

Le harness est encore sous `/tmp`; il doit devenir une cible oracle permanente,
avec permutations, carré cosphérique, E5, supports multiples et mutants
d'omission. Il ne prouve ni la complétude universelle de `flat_catalogue`, ni sa
capacité 50 k, mais il ferme exactement le prochain échelon expérimental.

### 1 bis. Source output-sensitive certifiable

La façon utile d'éviter l'allocation quartique sans prétendre supprimer son pire
cas est une frontière de tâches sur les supports de tailles un à quatre. Chaque
tâche représente un bloc combinadique ou un produit de nœuds spatiaux et se
termine par l'un des reçus suivants : domaine vide, dépendance affine
universelle, impossibilité exacte du bon centrage, borne inférieure de niveau
au-delà de l'horizon demandé, ou feuilles explicitement évaluées.

À la feuille : miniboule exacte, requête fermée complète, puis record
`SaturatedGenerator{ball_key, exact_level, canonical_support, sorted_members,
member_digest, source_certificate}`. La déduplication agrège tous les supports
d'une même boule au lieu de créer plusieurs générateurs.

Le reçu terminal de la source doit satisfaire sur les domaines disjoints :

$$n+\binom{n}{2}+\binom{n}{3}+\binom{n}{4}=N_{\mathrm{exclus}}+N_{\mathrm{invalides}}+N_{\mathrm{feuilles}}.$$

Pour streamer les lots, un watermark de niveau est obligatoire : publier le lot
`a` seulement lorsque toute tâche résiduelle possède une borne inférieure
strictement supérieure à `a`. Sinon un support non résolu peut encore produire
un générateur oublié au même niveau.

Cette architecture évite de matérialiser l'univers des supports et permet
rollback/reprise. Elle ne promet pas un travail sous-quartique universel : le
nombre de générateurs distincts peut lui-même atteindre ce régime. Sous budget,
la frontière est sérialisée et le statut reste `partial_refinement`.

### 2. Distinguer validité structurelle et complétude scientifique

`SaturatedFold::ok` signifie actuellement que les tranches et listes sont
lisibles. Il ne dit ni famille complète, ni sous-famille certifiée. Le contrat
doit porter au moins `source_complete`, `join_complete` et `forest_semantics` :

- `exact` seulement avec certificats de source et de join;
- `partial_refinement` sous S.6 lorsqu'un budget omet des générateurs;
- `refused` si la provenance ne permet ni l'un ni l'autre.

Un simple accord sur une campagne ou `smax>=n` ne remplit pas ces champs.
Le fold ne reçoit actuellement ni `n` ni `smax`; une invocation directe avec
`smax<n` peut donc encore rendre `ok=true`. Les deux campagnes choisissent une
valeur supérieure à `n`, mais le type lui-même n'en porte aucune preuve.

### 3. Les projections forestières restent à construire

Le résultat courant compare les couvertures de coupe. Il ne publie pas encore
les partitions de facettes, le journal des incidences silencieuses, les
verticales, les identifiants persistants, ni le `MergeForest` contractuel. Une
forêt couvrante de générateurs à une coupe est un certificat de connectivité;
ses remplacements internes ne sont pas des événements topologiques.

Le lot persistant doit comparer snapshot strict et snapshot fermé, émettre
naissances, continuations et multifusions, journaliser la croissance silencieuse
et seulement ensuite attribuer les identifiants canoniques.

Les compteurs `births`, `continuations` et `fusions` du header ne sont jamais
lus par le juge. Supprimer toute leur classification ne change donc aucune
porte. Pour préparer le `MergeForest`, comparer un transcript **par niveau** à
la vérité Gamma; des totaux globaux égaux ne suffiraient pas davantage.

Un mutant hors dépôt remplaçant toute la classification du lot par « toujours
naissance » passe encore les trois portes fold 3/3 en 3,59 s. Le snapshot strict
et ces compteurs sont donc actuellement du code mort du point de vue de la
réception, même si les partitions fermées restent correctes.

Un second mutant qui découpe chaque classe de niveau en commits d'un seul
générateur passe lui aussi 3/3 en 3,14 s. Le juge prend simplement le dernier
snapshot fermé portant ce niveau et masque tous les états intermédiaires
illégaux. Exiger des niveaux de fold strictement croissants, exactement un
commit par classe rationnelle, puis comparer le transcript strict--fermé tue ce
mutant et reçoit littéralement l'atomicité.

### 4. Les portes doivent tuer le fold, pas seulement sa CLI

La porte nommée `reject_fold_mutant` refuse actuellement la combinaison
`--subject-fold 1 --force-shift-level 1`; elle ne mute aucun calcul du fold.
Ajouter au minimum :

- omission d'un générateur déterministe;
- suppression d'une union dont `|M intersection N|=k` exactement;
- commit séquentiel de deux générateurs ex æquo;
- membre retiré d'un saturé;
- permutation des records et des membres;
- doublon de générateur et sphère/niveau invalide, tous fail-closed.

La campagne Gamma doit rendre les quatre premières rouges et les permutations
vertes avec le même digest canonique. Les fixtures strict/union décrites dans
[`AUDIT_RECEPTION_GAMMA_478CFE8.md`](AUDIT_RECEPTION_GAMMA_478CFE8.md) restent
également nécessaires : les portes courantes laissent encore passer la
suppression isolée de la coupe stricte ou des niveaux sujet.

De même, `--require-degenerate-agreement` n'a pas encore son injection hostile :
les positifs concordent, mais retirer le bloc qui transforme une divergence
dégénérée en échec ne change aucune entrée actuelle. Une omission d'un overlap
sur la fixture carrée doit produire cette divergence et exiger le code 1.
Le mutant de seuil ci-dessus fournit déjà cette divergence dégénérée et rend le
programme rouge; il reste à l'intégrer comme porte dédiée. Son résumé révèle un
défaut de libellé mineur : les 60 fautes dégénérées sont conclues par
`desaccords hors degenerescence`. Le total final doit distinguer les deux
catégories.

Le lecteur vérifie l'ordre strict des membres mais pas encore leur appartenance
à `[0,n)`, l'unicité des générateurs, `Sphere::den>0` ni la cohérence
rang--taille--boule. Soit ces obligations arrivent avec un certificat de
catalogue authentifié, soit le fold les refuse lui-même; elles ne doivent pas
rester des préconditions implicites d'un résultat dit industriel.

L'API header doit aussi garder sa propre borne d'ordre : une sonde directe
`build_saturated_fold(Catalogue{}, -1)` termine actuellement par
`std::length_error`, car l'entier négatif est converti en taille avant
`orders.resize`. Les CLI filtrent ce cas, mais une fonction annoncée fail-closed
doit refuser `maximum_order<1` et toute valeur au-delà de sa borne avant toute
allocation.

Une fixture header-only donne positivement, au même niveau, `M={0,1}` et
`N={1,2}` : une composante à `k=1`, deux couvertures chevauchantes à `k=2`.
Elle confirme que le fold ne confond pas couverture et partition du nuage. La
même sonde injecte ensuite le saturé `{0,1}` à deux niveaux distincts : le fold
l'accepte et publie deux snapshots identiques avec une continuation fictive.
L'unicité/déduplication des générateurs doit donc être reçue avant le transcript.

## Route exacte vers un join plus léger

Le balayage `O(K*G^2)` est une bonne vérité de fold, pas la forme 50 k. Pour un
lot `B`, maintenir les postings anciens `P_x` des générateurs contenant le point
`x` et les postings locaux `B_x`. Pour chaque `M` du lot, scanner les postings
de ses membres; une réduction par paire rend exactement `w(M,N)=|M intersection
N|`. Une paire de poids `w` alimente simultanément tous les DSU
`k<=min(K,w)`.

Les identités de reçu sont : visites ancien--nouveau égales à la somme, sur les
nouveaux `M` et leurs membres `x`, de `|P_x|`; visites nouveau--nouveau égales à
la somme des `C(|B_x|,2)`; leur somme égale la somme des poids de toutes les
paires examinées. Après gel des racines strictes, le lot entier est réduit et
committé atomiquement.

Cette jointure évite les sous-simplexes, mais elle n'a pas de borne légère
universelle. Son coût total est
`P_post=sum_x C(d_x,2)=sum_{M<N}|M intersection N|`, potentiellement dense.
Les compteurs `G`, somme des tailles, longueurs de postings, `P_post`, paires
uniques, pic de l'accumulateur et arêtes retenues doivent précéder toute
extrapolation 50 k.

Ne pas jeter un générateur silencieux après une simple continuation. À l'ordre
deux, `A={1,2,3}`, puis `S={2,3,4}` peuvent déjà appartenir à la même composante;
un futur `N={3,4,5}` s'attache pourtant par `S`. Supprimer `S` perd cette
connexion. La réduction sûre la plus simple est l'alias par inclusion `S` dans
un générateur actif `T`, avec preuve et historique; toute compression plus
forte exige un certificat préservant les intersections futures.

## Décision live

**GO pour stabiliser ce fold CPU, construire la troisième vérité indépendante
en porte permanente et graver ses mutants. NO-GO pour l'appeler déjà backend industriel exact ou
pour lancer un benchmark 50 k comme qualification exacte.** Un benchmark G4
peut mesurer explicitement une sous-famille ou un flux synthétique complet,
mais son statut doit le dire. Le résultat 60/60 est une avancée mathématique
forte; les prochaines portes doivent convertir son
hypothèse de source complète en certificat et mesurer le coût de sa jointure.

Le message du commit parle de « sémantique industrielle démontrée »; cette
phrase appartient au ledger de Claude, pas au verdict d'audit. Le verdict reçu
est l'accord de couvertures borné et mutation-résistant sur le seuil, sous une
famille catalogue encore à certifier.

GCP non utilisé à ce snapshot.
