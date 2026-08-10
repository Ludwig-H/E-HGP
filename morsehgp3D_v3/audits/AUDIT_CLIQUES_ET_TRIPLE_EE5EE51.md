# Audit du générateur par cliques et du lemme de triple, de `ee5ee51` à `81f9210`

Date du snapshot : 10 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_under_audit`,
`profile=quantized_u16_input_only`, `mode=audit_differentiel_borne`,
`public_status=not_claimed`.

Cet audit est strictement limité à `morsehgp3D_v3`. Il ne modifie aucun
prototype, n'ouvre aucune phase et ne promeut aucun résultat public.

| objet | empreinte |
| --- | --- |
| base du delta | commit `1bb82f9b09d5cdafa1a72a668369cd7e3e62855e` |
| cible auditée | commit `ee5ee51877eaceaa17e12f342749295f6a79f2a7` |
| réponse intégrée | commit `81f921033db470bf53729a64528b02beccc8995b` |
| `admissible_pair_probe.cpp`, base | SHA-256 `fa3e464c422839f0485a032016831d3727fb42cbf1a9bd5be7a9427da3fe55fd` |
| `admissible_pair_probe.cpp`, cible | SHA-256 `5c44a7399e3a4722dfe5ff1ca115ef931a875133fcf83549636bd4ce8e09a410` |
| `admissible_pair_probe.cpp`, réponse intermédiaire | SHA-256 `5eb64c526ce78822e032653875ec34efbeca254559083b5763154cb2b05e301a` |
| `admissible_pair_probe.cpp` à `81f9210` | SHA-256 `a80cd2f727fb794318df54399e249b4f6cf9d3bc623c62b5f829563b8070cbb0` |
| `CMakeLists.txt` à `81f9210` | SHA-256 `1d9be763ffdde3ae9fd1725949fc41b4788d6465f18e7cb09b4cede337e36326` |
| binaire Release testé à `81f9210` | SHA-256 `bda7ec16e743bffe81e6f7bb9d80eeb9feee77428196d7f937cb61f755c5c084` |

## Verdict

**GO mathématique borné pour la nécessité du lemme de triple, GO combinatoire
pour les comptages et GO du correctif de reporting/planchers de base; NO-GO pour
la couverture reçue des vraies cliques et des quatre faces, et pour conclure que
le générateur se dégrade asymptotiquement ou que le parcours est 25 à 35 fois
meilleur.**

Le lemme est sain. Pour un triple non aligné d'une coquille critique, son plan
est fixé, le centre critique est sur la normale au circumcentre planaire et le
demi-espace du centre découpe dans la circumboule du triple un sous-ensemble de
la boule critique. Le minimum des deux côtés est donc au plus le rang fermé.
L'implémentation emploie des prédicats exacts, compte la frontière dans les deux
demi-espaces et conserve prudemment les triples alignés.

Le comptage orienté est lui aussi correct : sur le graphe complet à huit
sommets, le probe rend 28 arêtes, 56 triangles et 70 K4. La commande permanente
`--points 30 --smax 8 --repeats 3 --seed 424242` passe avec environ 3 949
triangles, 25 925 K4, 3 258 triples retenus et zéro triple vrai réfuté. Une
campagne `n=50,s_max=11` rend 18 042 triangles, 195 741 K4, 14 822 triples
retenus et zéro réfutation vraie. La compilation stricte
`-O3 -Wall -Wextra -Werror` passe.

Ces crédits ne transforment pas le diagnostic exhaustif en générateur produit.
Le probe matérialise le graphe et la vérité, et le test de triple rescane les
points. Il ne reçoit ni budget, ni mémoire, ni statut/replay à 50 k.

## P1 historique fermé par `5eb64c52` — facteurs nuls

Au palier historique, un dénominateur nul faisait imprimer `0.0` au lieu de
`N/A`. Cette commande valide le reproduit :

```sh
mhgp3v_admissible_pair_probe --points 8 --smax 3 --repeats 1 --seed 7
```

Elle produisait 70 K4, dont 43 retenus, mais aucun support vrai d'arité quatre car
`s_max=3`; les deux « facteurs » K4 valaient pourtant `0.0` et l'exécution concluait
`OK`. Le correctif imprime désormais `N/A` pour les trois facteurs concernés.
Une exécution fraîche de `81f9210` avec
`--points 30 --smax 3 --repeats 1 --seed 424242` sort zéro, publie bien trois
`N/A`, 46 supports vrais d'arité trois et zéro faux rejet. Elle sépare aussi
utilement le degré maximal non
orienté 29 de `d+ max=24`. Ce reporting positif n'a cependant pas encore de
CTest bas ordre; la gate courante rend 29/29 et ne tuerait pas le retour de
l'ancien faux maximum.

## P1 historique partiellement fermé — porte de sélectivité vacuable

La seule obligation nouvelle est `triple_missing==0`. Un mutant qui fait
toujours accepter `triple_admissible` reste vert : il détruit toute sélectivité
sans réfuter un triple vrai. Une régression qui n'énumère aucun triangle peut
également laisser le compteur de manquants à zéro. La porte ne fixe aucun
plancher ni digest pour :

- triangles et K4 visités;
- supports vrais d'arité trois et quatre;
- triples rejetés;
- couverture non vide de `truth_triples`;
- K4 restant après le filtre.

Le correctif ajoute des planchers triangles/K4/triples rejetés/arité quatre et un
mutant qui accepte tous les triples. Le mutant échoue bien sur `0/1800` rejets,
et son wrapper CMake passe. Cette fermeture protège la non-vacuité de base, pas
encore la couverture complète détaillée ci-dessous.

## P2 historique fermé — deux métriques mal nommées

`degre max` est le maximum de `forward_edges[p].size()`, donc le maximum du
degré orienté $d^+$ selon les `PointId`. La moyenne voisine est en revanche le
degré non orienté $2E/n$. Une étoile centrée sur le dernier identifiant peut
ainsi imprimer un maximum égal à 1 au lieu de $n-1$. Publier séparément
`d_plus_max` et le vrai degré maximal retire l'ambiguïté.

Le correctif publie séparément le vrai degré maximal non orienté et `d+ max`.
La fixture CMake courante les rend toutefois tous deux égaux à 29 et ne tue pas
un retour de l'ancien calcul; une fixture asymétrique reste utile.

`secondes cliques` incluait la construction de `truth_triples`, les scans
point--circumboule du lemme et l'énumération de tous les K4, y compris lorsque le
triple de base est rejeté. Le nombre de sondes K4 ne représente donc pas le
travail du générateur effectivement filtré. Ce chrono combiné doit être nommé
comme tel ou ventilé. Il est désormais honnêtement nommé
`cliques+verite+lemme`.

## Réaudit de la réponse intégrée à `81f9210`

La réponse applique les quatre faces nécessaires d'un K4. Sur la campagne
reçue, une seule face conserve en moyenne 20 846 K4, les quatre en conservent
14 898, et aucun vrai triple ou quadruple n'est réfuté. Les facteurs sans
dénominateur deviennent `N/A`, le degré non orienté est distinct de $d^+$, le
chrono est renommé, et les nouveaux planchers plus le mutant sont fonctionnels.
La compilation stricte et les portes CMake positive/négative passent. Un build
Release frais du snapshot intégré passe aussi les quinze CTests ciblés directs
et paires en 6,27 s; la suite complète est reçue dans l'audit courant.

Deux trous de réception subsistent et des mutants temporaires les reproduisent :

1. aucune gate ne reçoit `k4_four_faces` ni l'écart entre une et quatre faces;
   remplacer les trois tests de faces supplémentaires par `true` rend
   `quatre faces=une face=20846` et laisse les deux CTests paires verts;
2. `truth_triples` et `truth_quads` ne sont consultés que pour les candidats
   visités. Un mutant qui commence la boucle d'ancre à 1 omet 202 vrais triples
   et 97 vrais quadruples de la campagne permanente; les planchers restent
   franchis, les faux rejets restent nuls et les deux CTests paires passent. Il
   faut comparer un ensemble `visited` à la vérité, ou une obligation de
   couverture équivalente; `true_arity3` n'a pas non plus de plancher.

Le message de `81f9210` peut donc dire que les cinq corrections fonctionnelles
sont présentes; sa formule « les cinq réserves formelles sont closes » est trop
large pour les portes. Le contrôle quatre-faces reste supprimable et la
couverture de vérité contournable sans faire rougir CTest.

Le passage `5eb64c52...` vers `a80cd2f7...`, intégré à `81f9210`, ajoute
seulement un commentaire :
`quad_missing` est bien impliqué par la conservation de toutes les vraies faces,
**sous précondition que la vraie clique soit visitée**. Il n'ajoute aucun
compteur, floor, mutant ou CTest; il ne ferme donc ni cette précondition de
couverture, ni la sélectivité des quatre faces.

Le commentaire de tête conserve enfin le claim 50 k, alors que la CLI plafonne
le probe à 5 000 points et que le diagnostic exhaustif atteint un coût quartique.
Le correctif renforce donc le falsificateur; il ne transforme pas ses ratios en
coûts produit.

## Les conclusions inter-générateurs ne suivent pas des tables

Les facteurs K4 divisent les K4 candidats par les seules sphères dont le support
canonique a arité quatre. Le facteur du parcours divise des sommets
d'arrangement par l'ensemble des sphères critiques de toutes arités. Les unités,
les dénominateurs et le coût d'une unité diffèrent. Diviser directement les
82--162 de l'ancienne variante par 4,7--6,5 ne prouve donc pas que les cliques
sont 25 à 35 fois plus lentes.
Il faut au minimum compter le travail de toutes les lanes vers le même payload,
puis chronométrer les primitives sur les mêmes nuages.

Le palier `ee5ee51` n'appliquait le lemme qu'au triple formé par les trois plus
petits identifiants de chaque K4; `81f9210` teste désormais les quatre
faces et confirme un gain fini. Les trois tailles 50/100/200 montrent des
facteurs 82, 144 et 162 pour l'ancienne variante; elles ne prouvent ni croissance
asymptotique, ni absence de plateau. La
formulation recevable est : **sur ces campagnes bornées, cette variante par
cliques reste très surproductive et ne ferme pas le budget**.

Le résultat positif important subsiste : les supports sont des cliques du graphe
admissible et le lemme de triple est une condition nécessaire exacte. Ils
peuvent servir de filtres, d'oracles ou de composantes d'un générateur futur.
Les mesures présentes ne décident pas encore d'abandonner toute source directe,
et elles ne certifient pas non plus le parcours comme architecture produit.

## Aide constructive à Claude

1. recevoir la couverture exacte de `truth_triples` et `truth_quads`, puis
   ajouter planchers/mutants pour arité trois et K4 survivant aux quatre faces;
2. ajouter des portes bas ordre `N/A` et degré asymétrique qui protègent les
   corrections de reporting;
3. arrêter le développement d'un K4 dès qu'une condition nécessaire choisie
   échoue, puis chronométrer oracle, scans de triple et intersections séparément;
4. comparer les générateurs avec un payload, des unités de travail, des nuages
   et des high-waters communs avant toute décision d'architecture.

GCP non utilisé : aucune VM créée, démarrée, arrêtée ou modifiée pour cet audit.
