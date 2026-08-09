# Réponse aux audits `AUDIT_PROMOTION_M3_2E3FA7B` et `AUDIT_DELTA_ORDER_K_2E3FA7B_468635C`

Date : 9 août 2026 UTC. Objet : la promotion M3 est retirée, la voie
multiplicitaire est implémentée et jugée, et deux résultats nouveaux sont
versés au dossier — dont un qui corrige l'arithmétique de la question 50 k.

> [!NOTE]
> **Réponse chronologique, pas autorité d'audit.** Les §§1 à 10 documentent les
> snapshots qui ont mené à `1a0a1f8`; plusieurs limites y sont historiques. Le
> §11 répond au premier delta Gate D. L'état courant et les findings indépendants
> sont dans [`AUDIT_DELTA_ORDER_K_FLATS_2532FD5.md`](AUDIT_DELTA_ORDER_K_FLATS_2532FD5.md)
> et les deux notes Gate D.

> [!IMPORTANT]
> **Le verdict des deux audits est accepté sans réserve.** Les quatre P0 ont été
> reproduits ici, sur le header committé `a6d0a3e…`/`47ee376…`, contre une
> énumération exhaustive indépendante du parcours mais relative aux primitives
> géométriques v2 partagées. Ils sont réels. Le README est revenu à
> `exploration_v3`.
>
> **Deux faits nouveaux, tous deux défavorables à mes propres affirmations
> antérieures.** Le rapport 100:1 travail/sortie était un **artefact de la
> récolte défaillante** : mesuré contre le catalogue complet, il vaut 17:1, mais
> la sortie est six fois plus grosse qu'annoncée. Et la convention de support
> canonique la plus naturelle — plus petit sous-ensemble par identifiant — est
> **non invariante par permutation** ; une seule renumérotation suffit à changer
> la sortie sur le cube.

## 1. Reproduction exhaustive indépendante du parcours

Sonde écrite pour cet audit, incluant `prototype/order_k_bfs.hpp` **sans
modification**, avec une vérité exhaustive locale (tous les sous-ensembles de
taille au plus quatre, miniboule, census par `mhgp::sphere_side`, déduplication
par coquille) :

```text
[germe_coplanaire]   seed_shell ok=1 shell={0,1,3,4}  niveau_stocke=0 niveau_EXACT=1
[cube]      s_max=2  sujet=8  spheres  force brute=20   arites 8/0/0/0 contre 8/12/0/0
[coq. cst.] s_max=2  sujet=6            force brute=15   arites 6/0/0/0 contre 6/9/0/0
[n=2]       s_max=4  sujet=2            force brute=3
[n=3]       s_max=4  sujet=3            force brute=6
```

Le compteur `coupes` vaut 1 sur le cube et sur la coquille constante : le germe
est coupé **avant** toute navigation, ce qui confirme le mécanisme décrit au §4.2
de l'audit de promotion. Le témoin coplanaire produit `hors_domaine=1` et un
catalogue vide, ce qui confirme le §4 de l'audit différentiel.

## 2. Portes fermées

Référence : `AUDIT_PROMOTION_M3_2E3FA7B.md` §9 et `AUDIT_DELTA_…` §7.

| porte | statut | où |
| --- | --- | --- |
| P1 — README à `exploration_v3`, exactitude et architecture rouvertes | **fermée** | `README.md` |
| P2 — fixtures permanentes : cube, pont, coplanaire, coquille constante, `n=2/3`, u16 extrême | **fermée** | 19 fixtures aux coordonnées exactes des audits, ordres 2 à 8, dans `mhgp3v_flats_differential` |
| P4 — navigation multiplicitaire sans coupe par rang fermé, `(coquille, niveau)` comparés | **fermée** | `prototype/order_k_flats.hpp` ; la coupe est $\ell\le s_{\max}-2$ et le rang fermé n'apparaît que dans `try_emit` |
| D1 — fixture du germe à cinq points, `(shell, exact_level)` avant propagation | **fermée** | census exact à chaque sommet, compteurs `census_mismatch_shell` et `census_mismatch_level` |
| D2 — vrai germe de niveau zéro sur une face non triangulaire | **fermée après correction** | la première construction, par descente de rayon, était fausse et l'audit `9c587e6` l'a réfutée ; voir §8 |
| D3 — non-régression de `468635c` (coquille constante) | **fermée** | fixture `constant_shell_members` |
| D6 — niveaux et coquilles comparés à une énumération indépendante du parcours sur coplanarités, cosphéricités, limites u16 | **fermée relativement aux primitives v2 partagées**; **ouverte** pour l'oracle rationnel M1 | voir §5 |
| P3 — `Grid::ball` fail-open | **non fermée, contournée** : aucun accélérateur n'est branché, la requête balaie le nuage | — |
| P5 — intégration au catalogue complet et aux forêts, injection de fautes | **partielle** : catalogue oui, forêts non, injection non | — |
| P6 — porte M3 définie dans la spécification et le registre | **non fermée** ; aucune phase n'est ouverte, donc rien à mettre à jour | — |
| P7 — pipeline complet séquentiel, pic mémoire | **non fermée** | — |
| D5 — recentrer l'amorce sur l'axe $a+q+tu$ | **caduque** : l'amorce appartenait au chemin rapide, qui n'existe plus | — |

## 3. Ce que la voie multiplicitaire a coûté et rendu

Implémentée telle que la propose `AUDIT_VOIE_MULTIPLICITES_ORDER_K.md` §9, aux
étapes 1 à 5 :

- **`Rank3Flat` par fermeture.** Les arêtes incidentes sont en bijection avec les **plans distincts** engendrés par au moins trois points non alignés de la coquille. Un triplet qui n'est pas la base canonique de sa fermeture est écarté avant toute requête — compteur `triples_quotiented`.
- **Transition $S(w)=C(F)\cup A$ et transport par lots.** Aucune supposition « un seul point change d'état ». Le compteur `batches_multiple` mesure les lots réels.
- **Plafond $\ell\le s_{\max}-2$.** Le rang fermé n'est plus jamais un critère de parcours.
- **Voie directe déclarée** pour $n<4$ et pour la dimension affine inférieure à trois — exhaustive, donc exacte, et hors du théorème de propriétaire dont elle ne relève pas.
- **`CloudStatus` remplace `out_of_domain`.** Chaque refus nomme sa cause ; un échec de germe porte son **étape**.

Les 4-sous-ensembles ne sont pas récoltés, et l'argument est court : si le
support canonique a quatre points il est affinement indépendant, sa sphère est
le sommet et sa coquille est $S(v)$ ; si quatre points de la coquille sont
coplanaires, leur miniboule a un support d'au plus trois points.

## 4. Deux résultats que les audits n'avaient pas, et un qu'ils avaient prévu

### 4.1 L'ambiguïté de demi-tour dans l'emballage du germe

`orient3d` ne voit un plan qu'à $\pi$ près. Deux candidats situés **dans** le
plan vertical support mais de part et d'autre de l'axe sont donc déclarés à
égalité alors que leurs angles valent $0$ et $\pi$ : la rotation part du mauvais
côté. Sur 3 000 nuages tirés, un seul l'exhibe :

```text
(26,30,33) (27,30,34) (27,30,26) (34,30,33) (30,33,26) (25,30,25) (35,31,30)
```

La correction classe l'angle explicitement : $e=p_1-p_0$, $g=(-e_y,e_x,0)$ la
normale intérieure du plan vertical support, $f=g\times e$ ; l'angle vaut $0$ si
$(w\cdot g=0,\ w\cdot f>0)$, $\pi$ si $(w\cdot g=0,\ w\cdot f<0)$, et il est
strictement intermédiaire sinon, où `orient3d` redevient un ordre total.
Fixture permanente `germe_demi_tour`. Le garde de vérification avait bien
rougi : le germe a refusé au lieu de produire un faux, ce qui est le
comportement voulu, mais le refus censurait le nuage.

### 4.2 Le support canonique n'est pas invariant par permutation

C'est la porte ouverte n°10 du contrat. Elle est **exhibée**, et elle mord :

| convention | comportement mesuré |
| --- | --- |
| support lu sur le candidat de découverte | force brute $\lbrace2,5\rbrace$ contre navigation $\lbrace0,7\rbrace$, même sphère |
| support lu sur la coquille triée par **identifiant** | une permutation suffit à changer la sortie sur `cube`, `constant_shell_members`, `coplanaire_pur`, `germe_demi_tour` |
| support lu sur la coquille triée par **coordonnées** | invariant sur toutes les campagnes |

La troisième convention est retenue. Ce n'est **pas** une démonstration
d'invariance topologique : c'est une convention géométrique, qui ne dépend plus
que de l'ensemble de points. Le cas de deux points de coordonnées identiques
reste hors contrat.

### 4.3 Le rapport 100:1 était un artefact

C'est le point qui déplace la question 50 k. Le rapport comparait les sommets
visités à un compteur de sphères critiques produit par la récolte défaillante —
celle-là même qui omettait l'essentiel des arités deux et trois. Mesuré sur le
catalogue complet **selon cette vérité relative**, confronté à l'énumération
exhaustive :

| $n$ | sommets/point | critiques/point | travail/sortie |
| ---: | ---: | ---: | ---: |
| 100 | 776,9 | 49,4 | 15,7 |
| 200 | 935,5 | 55,7 | 16,8 |
| 300 | 1 027,2 | 60,7 | 16,9 |

Profil LiDAR à densité fixe, emprise $\propto\sqrt{n}$, $s_{\max}=11$, un cœur.
Les 777 sommets par point à $n=100$ retrouvent la mesure publiée
précédemment : les deux profils sont comparables.

Trois lectures, et la troisième est la seule qui compte :

1. le facteur travail/sortie vaut **17**, pas 100 ;
2. la sortie est **six fois plus grosse** qu'annoncée — les triangles dominent le catalogue, 11 593 sur 18 207 à $n=300$ ;
3. **les deux ratios croissent encore** à $n=300$. Toute extrapolation à 50 000 points, y compris la mienne, est une extrapolation d'une croissance non stabilisée.

Le mur n'est donc plus celui que le README décrivait. `candidats/sommet` vaut
exactement $8(n-4)$ : quatre flats, deux directions, un balayage complet du
nuage. C'est une absence d'index, pas une propriété du problème. Et la récolte
paie un census en $O(n)$ par candidat avec **43 %** de tentatives redondantes,
que la règle de propriétaire supprimerait.

## 5. Ce qui n'est pas jugé, et pourquoi je le dis avant d'être audité

- **L'oracle M1 n'a pas été étendu.** Sa référence déclare hors domaine tout nuage portant un point surnuméraire sur une coquille — précisément le régime que ce parcours traite. Le juge utilisé ici est indépendant du germe, du pinceau et du transport, mais il partage `mhgp::sphere_side`, `mhgp::sphere4` et `mhgp::miniball_of` avec le sujet et n'est pas en arithmétique rationnelle. Une référence multiplicitaire rationnelle est nécessaire, et elle devra être auditée à part.
- **Aucun accélérateur spatial.** Les P0 de `Grid::ball` ne sont pas corrigés ; ils sont hors du chemin. Le contrat *fail-open* s'appliquera intégralement au premier index écrit.
- **Ni forêts, ni reverse search, ni propriétaire.** La récolte déduplique encore par une table globale de coquilles ; la mémoire de navigation reste proportionnelle au nombre de sommets.
- **Aucune mesure à l'échelle.** Les tailles mesurées vont de 4 à 300 points. Rien à 50 000, aucun pic mémoire, aucun reçu scellé.

## 6. Campagnes

> [!NOTE]
> Cette section conserve le snapshot antérieur aux audits `9c587e6` et `2532fd5`. Les campagnes et portes courantes sont décrites aux §§8--9; les nombres ci-dessous ne doivent pas être lus comme le reçu du live.

`mhgp3v_flats_differential`, trois portes simultanées — le **sommet** (coquille
et niveau strict contre l'énumération exhaustive des sommets d'arrangement), le
**catalogue** (supports canoniques et rangs), l'**équivariance** par permutation
— avec census exact actif à chaque sommet :

| campagne | nuages | points | grille | $s_{\max}$ | cas | désaccords |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| générique | 2 500 | 12 | $[0,26)$ | 2 à 7 | 18 902 | **0** |
| grille saturée | 1 500 | 10 | $[0,5)$ | 2 à 8 | 13 277 | **0** |
| fixtures et cosphéricités forcées | — | 4 à 12 | — | 2 à 8 | 902 | **0** |

La grille saturée est le régime qui compte : dix points dans une boîte de côté
cinq, donc presque tous les nuages portent des cosphéricités, des coplanarités
et des alignements.

Quatre tests CTest permanents et trois tests négatifs — argument inconnu,
campagne absurde, plancher non atteint. Le fabricant de tests négatifs codait
l'oracle en dur ; un test écrit pour un autre juge aurait donc interrogé le
mauvais binaire et échoué sur son diagnostic au lieu de qualifier le rejet visé.
Corrigé depuis.

## 7. Ce que je demande à l'audit suivant

1. Le théorème de propriétaire est utilisé ici comme **droit de ne pas récolter les 4-sous-ensembles** et comme plafond $s_{\max}-2$. L'argument de redondance des 4-sous-ensembles est-il complet ?
2. La convention de support canonique par ordre des coordonnées est-elle acceptable comme contrat public, sachant que l'invariance topologique reste indémontrée ?
3. Le classement d'angle du germe est-il correct dans tous les cas, y compris quand le plan vertical support contient plus de trois points ?
4. La vérité du différentiel partage `mhgp::sphere_side`, `mhgp::sphere4` et `mhgp::miniball_of` avec le sujet. Est-ce une dépendance acceptable tant que l'oracle rationnel n'est pas étendu, ou faut-il la retirer d'abord ?
5. Quelle est la bonne priorité entre l'index *fail-open* et la règle de propriétaire ? La mesure dit que l'index vaut un facteur $10^3$ et le propriétaire un facteur inférieur à 2 ; mais le propriétaire ferme aussi une porte de mémoire.

GCP non utilisé. Toutes les mesures viennent du codespace, deux vCPU, `-O3
-march=native`, un seul cœur — et c'est délibéré : ce sont des charges CPU, elles
n'ont rien à faire sur la G4.

---

## 8. Delta après `AUDIT_ORDER_K_FLATS_9C587E6`

L'audit du parcours par flats est arrivé pendant l'écriture de ce document. Son
verdict est accepté, son P0 est réel, et il a été reproduit avant d'être corrigé.

**P0 — la descente stricte du rayon est fausse.** Reproduit exactement :

```text
in_circle_coplanar(A,B,C,P) = -72
  R2(ABC) = 2.5   R2(ABP) = 2.5   R2(BCP) = 2.5   R2(CAP) = 2.5
statut=germe_non_certifie etape=6 spheres=0
```

Le bon potentiel de Delaunay n'a jamais été le rayon, c'est le vecteur
des angles ; mon argument confondait les deux. **La correction supprime la
boucle** : sur une arête de l'enveloppe du sous-nuage coplanaire, le troisième
point de Delaunay maximise l'angle inscrit, et « $d$ intérieur au cercle
de $(a,b,c)$ » équivaut à « angle en $d$ supérieur à l'angle en $c$ ». C'est un
ordre **total**, donc une passe suffit et il n'y a plus rien à faire terminer.
L'arête d'enveloppe s'obtient de même en une passe depuis le point lex-min du
sous-nuage, où aucune paire de directions n'est antipodale ; à angle égal on
prend le plus proche, sans quoi un point du segment resterait entre les
extrémités. Le garde `q*q+8` disparaît avec la boucle, et avec lui son propre
débordement `int` au-delà de 46 341 points coplanaires.

La fixture est permanente sous `descente_rayon_refutee`, rejouée sur ses **120
permutations** avec exigence de zéro refus et de signature unique.

**Les autres points de cet audit, fermés dans ce delta.** « Tous » serait faux : l'audit suivant, `AUDIT_DELTA_ORDER_K_FLATS_2532FD5`, a montré que le narrowing du CLI, la garde u16 à l'API, le census fail-open et les planchers par famille restaient ouverts. Le §9 ferme les deux premiers et une partie du troisième; les minima par branche et le contrat complet du census restent ouverts.

| point | correction |
| --- | --- |
| §3.1 — la vérité partage `sphere4` et `miniball_of` | l'autorité du juge est désormais **bornée par écrit** dans le fichier, le README et le commentaire CMake : « portée de navigation et catalogue concordants relativement à ces primitives », pas « catalogue critique exact » |
| §3.2 — payload incomplet | doublons publiés, tranche de membres, contiguïté de `members_begin`, appartenance exacte des membres à la boule fermée, queue de `support` à $-1$ et ordre lexicographique strict sont désormais comparés |
| §3.3 — la porte peut devenir exhaustive sans le signaler | planchers de couverture par campagne : nuages réellement navigués, sommets, coquilles multiples, triplets quotientés, publiés dans le rapport et exigés par CTest |
| §3.4 — équivariance trop étroite | elle est appelée aussi sur les nuages génériques, saturés et cosphériques, avec le **statut** dans la signature et le census actif |
| §3.4 — doublons de coordonnées | statut `kDuplicateCoordinates` : le sujet **refuse** au lieu de publier `ok`, la garde de domaine est symétrique, et le refus est transactionnel |
| §3.5 — CLI non fail-closed | lecture entière **intégrale** (`0junk` rejeté) et borne sémantique u16 sur `--coord` ; trois nouveaux CTests négatifs |
| §3.6 — deux énoncés | la variation de niveau n'est **pas** bornée par un, elle vaut $\lvert D_-\rvert-\lvert A_{\text{int}}\rvert$ ; et la chaîne $s_{\max}-q\le s_{\max}-2$ ne vaut que pour $q\ge2$ |

**Ce que ce delta ne ferme pas**, et je ne le prétends pas : la référence
rationnelle multiplicitaire de l'oracle M1, l'index *fail-open*, la règle de
propriétaire, le reverse search, les forêts, et le §4 entier de l'audit — les
$2{,}5\cdot10^9$ appels à `sphere_side` des singletons avant le germe, les tables
globales `seen`/`frontier`/`visited`, le census en $O(n)$ par tentative. Ce sont
des portes d'architecture avant toute mesure à l'échelle, exactement comme
l'audit le dit. Le statut reste `exploration/diagnostic_only`.

---

## 9. Delta après `AUDIT_DELTA_ORDER_K_FLATS_2532FD5`

L'audit crédite la nouvelle construction du germe. La porte versionnée rejoue
120 permutations à `s_max=5`; un harnais externe les a aussi rejouées aux ordres
2 à 8. Un contrôle Python transitoire de propriétés sur grille $4\times4$ et des
fuzz non archivés n'ont trouvé aucun contre-exemple, mais ne constituent pas un
oracle indépendant ni une porte reproductible. Le P0 de `9c587e6` et le
débordement `q*q+8` sont fermés; le NO-GO exact et 50 k reste inchangé. Les
portes de narrowing et de grille u16 sont fermées ci-dessous, le census est
durci mais encore partiel, et les planchers par branche restent ouverts.

**§4.1 — narrowing entier.** Le token était lu en entier, mais le `long long`
était tronqué en `int` sans contrôle de plage : `--clouds 4294967296` rendait
zéro nuage et `--coord 4295032832` rendait 65536, tous deux avec code 0. Chaque
option porte désormais sa plage sémantique, vérifiée **avant** le cast, et la
graine a son propre contrat de largeur. La demande combinatoirement impossible —
neuf points distincts dans huit positions — est refusée au lieu d'être censurée
par le tirage, avec un produit calculé en `long long`. L'accord
demandé/généré est vérifié par famille et publié.

**§4.2 — la grille u16 n'était gardée qu'au CLI du juge.** C'était le point le
plus grave, parce qu'aucun autre appelant n'était protégé. La garde est
maintenant à la frontière des deux entrées publiques, `navigate_shallow` et
`flat_catalogue`, avec le statut `kOutsideDeclaredGrid`. Reproduction de
l'audit sous UBSan :

```text
hors grille 1e9 : statut=hors_grille_u16_declaree spheres=0 membres=0
frontiere 65535 : statut=ok spheres=16
frontiere 65536 : statut=hors_grille_u16_declaree spheres=0
```

Les quatre frontières — $10^9$, 65535, 65536 et une coordonnée négative — sont
un test permanent de `flat_catalogue`; un probe externe a vérifié
`navigate_shallow`. Il reste à graver cette seconde entrée.

**§4.3 — le census était fail-open.** Une contradiction incrémentait un compteur
que seul ce binaire lisait. Elle positionne désormais `kInvariantViolated`.
`flat_catalogue` rend un résultat vide, mais `navigate_shallow` peut encore
rendre le préfixe poussé avant le census, et un `census(...) == false` reste
ignoré. Le statut est durci; l'atomicité et l'impossibilité de vérifier restent
à fermer.

**§4.5 — les contradictions documentaires.** `PROPOSITION.md` disait encore
« $-1$, $0$ ou $+1$ » là où le header et le README ont corrigé en
$\lvert D_-\rvert-\lvert A_{\text{int}}\rvert$, non borné. Le commentaire CMake
disait encore « partage seulement `sphere_side` ». Le README annonçait « payload
entier » alors que centre, rayon et $\beta$ ne sont pas confrontés à une vérité
distincte, annonçait trois tests négatifs au lieu de neuf, et employait deux fois
l'identifiant 9. Les deux `\textsc{Delaunay}` amputés viennent d'une tabulation
avalée par mon propre script de réécriture. Tout est corrigé.

**§4.4 — planchers par famille.** Partiellement fermé : l'accord
demandé/généré est vérifié et publié par famille, mais les minima CMake restent
agrégés par campagne et non par branche. Je le laisse ouvert plutôt que de le
déclarer fermé.

**Ce qui ne bouge pas, et ne doit pas bouger** : la référence rationnelle
multiplicitaire, l'index *fail-open*, la règle de propriétaire, le reverse
search, les forêts, et tout le §5 de l'audit — les $2{,}5\cdot10^9$ appels des
singletons, `seen`/`frontier`/`visited`, le census en $O(n)$ par tentative. Le
statut reste `exploration/diagnostic_only`, et le NO-GO 50 k est intact.

---

## 10. Delta d'index — réponse à `NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU`

La note est arrivée pendant que j'écrivais l'index, et elle a réfuté ce que
j'étais en train d'écrire. Les deux briques sont intégrées.

**Le P0 est réel et reproduit.** Sur les quatre points u16 de la note,
`sphere4` donne `den = 2` et un rayon d'environ $10^{18}$ : le centre d'un
quadruplet presque coplanaire sort arbitrairement loin de la grille. Mon
argument — « l'erreur reste sous $2^{-35}$ puisque coordonnées et rayons restent
sous $2^{17}$ » — ne parlait que des coordonnées, jamais du centre. Mesuré avant
correction :

```text
sphere_side des quatre supports : 0 0 0 0
den=2  |num|~1.89e18
closed_ball : points touches=0  rendus=0  (attendu 4)
```

L'élagage supprimait **la racine**. Après le théorème 1, `points touches=4
rendus=4`. La fixture est permanente, et la porte d'index la teste sur le
prédicat lui-même — grande sphère, nœuds internes réellement exercés, accord avec
une énumération exhaustive — et non seulement à travers le catalogue.

**Le chemin rapide flottant subsiste mais il est gardé** : il n'est autorisé que
si centre et rayon tiennent sous $2^{20}$, où l'erreur absolue reste sous
$2^{-30}$ et la marge d'un demi la domine de plus de $2^{28}$. Hors de cette
garde, le prédicat entier décide. Le header inclut désormais `<cmath>` : il
n'était pas auto-suffisant, c'était le point 1 de votre porte.

**La nuance de vocabulaire est acceptée et corrigée dans le code.** La requête
n'est pas la différence symétrique de deux boules fermées mais le désaccord
**ternaire** de `sphere_side` — la méthode s'appelle maintenant
`sign_disagreement`, et le commentaire dit pourquoi : la différence des boules
fermées perdrait le cas contractuel « sur la coquille à une extrémité,
strictement intérieur à l'autre », et un point du cercle du flat a le même signe
nul aux deux extrémités, donc il n'est pas redécouvert comme événement.

**Ce que l'index a donné, et ce qu'il n'a pas donné.** Deux $O(n)$ disparaissent
— la requête de pinceau et le census par tentative — et les singletons se
publient en temps constant, ce qui supprime les $2{,}5\cdot10^9$ classifications
d'avant le germe. S'y ajoute un résultat que la note n'attendait pas : en
transportant l'**ensemble** intérieur au lieu de son cardinal, les événements
sortants d'une requête deviennent gratuits et la récolte gagne un test de
propriété local qui écarte 88,6 % des tentatives à $s_{\max}=11$ sans census.

Le facteur mesuré, même binaire pour les deux colonnes : **10,7 à 12,2 à
$s_{\max}=5$**, mais **2,8 seulement à $s_{\max}=11$**, parce que les sphères d'un
niveau profond sont grandes et que l'arbre élague moins. Extrapolé depuis
$n=200$ à l'ordre du contrat, 50 000 points demanderaient environ 130 s sur 48
cœurs. **Le NO-GO 50 k n'est pas entamé**, et je ne prétends pas l'entamer :
l'index est une brique, `seen`/`frontier`/`visited` résident toujours, les flats
sortent encore des triplets, et il n'y a ni propriétaire calculé, ni reverse
search, ni forêts.

---

## 11. Gate D — parent local implémenté et jugé

La règle de parent de `NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D` est implémentée
comme **porte**, pas comme parcours : le BFS avec `seen` reste en place, et la
porte recalcule le parent de chaque sommet pour le juger.

**Votre identité §6 remplace ma dérivation.** J'avais construit la direction du
pinceau depuis le circumcentre `sphere3`, ce qui frôlait $2^{127}$ et m'imposait
`BigInt<4>`. Avec $d=(u,2u\cdot a)$ et $a_i\cdot d=-2\,\mathrm{orient3d}(a,b,c,p_i)$,
le signe tangent se lit avec le prédicat que le pinceau évalue déjà. Les deux
formes coïncident puisque $(c_0-a)\cdot u=0$. Tout le grand entier a disparu.

**Votre P0 §11.2 est réel et je l'avais rencontré une heure plus tôt sans en
comprendre la cause.** Sur la grille saturée, un nuage sur six cents produisait
exactement une racine surnuméraire, à tous les ordres. C'était bien la base du
potentiel du germe prise sans vérifier son indépendance. L'extraction est
maintenant gloutonne — un point, un deuxième distinct, un troisième non
colinéaire, un quatrième d'`orient3d_exact` non nul — et un échec rend
`kInvariantViolated`. Votre fixture à six points est permanente, ainsi que
`lex_admissible_cycle`, `lp_optimum_tie` et `level_zero_lex_cycle`.

**[mesuré]** deux campagnes après correction : **510 154** et **530 197** sommets
ont vu leur parent recalculé localement. Une racine par nuage, aucun parent
manquant, aucune inclusion $B(\pi(v))\subseteq B(v)$ violée, aucun cycle, zéro
désaccord sur 4 761 et 5 611 cas.

**Vos deux qualifications §11.3 sont acceptées et inscrites.** Le test
$B(v)\subseteq B_U$ est un préfiltre **nécessaire**, pas une reconnaissance du
propriétaire canonique : la table `emitted` reste indispensable, et le README le
dit maintenant. Et la porte ne vérifie pas encore le rang trois de $C(d)$,
l'identité $S(\mathrm{next})=C(d)\cup A$, la finitude de l'extrémité ni la
stricte variation du potentiel — ces quatre assertions doivent précéder le
remplacement effectif du BFS, et il n'a pas eu lieu. Le bloc doit aussi échouer
si le second parcours ou la taille du vecteur de parents est incohérent; il les
saute encore silencieusement.

**Sur la route GPU, votre note des globalités résiduelles corrige ce que
j'allais écrire.** Le parent local rend le **parcours** sans état partagé de
visitation : plus de déduplication atomique des sommets, plus de table résidente
proportionnelle au nombre visité. Les sorties des workers doivent encore être
fusionnées et ne constituent pas un front GPU qualifié. Le parent ne touche à
aucune des
dépendances globales de la hiérarchie — complétude des incidences silencieuses,
ordre exact en $\beta$, fermeture atomique des ex æquo, partition horizontale
vivante, provenance de couverture, jointure verticale. Le verrou mathématique
aval prioritaire n'est donc plus le parcours : c'est la source complète et sparse
des premières incidences utiles. Je ne prétends pas l'avoir entamée.

---

## 12. Gate D renforcée, et la dichotomie des premières incidences mesurée

**Les quatre assertions manquantes sont ajoutées.** La porte vérifie désormais,
en plus de la racine unique et de l'absence de cycle : le rang trois de la
fermeture $C(d)$ — incluse dans la coquille et portant un triple non aligné —,
l'identité $C(d)=S(v)\cap S(\pi(v))$, qui vaut parce que deux sphères distinctes
d'un même pinceau se coupent exactement selon le cercle du flat, et la **stricte
variation du potentiel** : niveau strictement décroissant, ou ensembles
intérieurs égaux et $L_h$ strictement croissante, ou niveau zéro et $Q_r$
strictement décroissante.

Les deux dernières demandent de comparer des puissances portées par des sphères
**différentes**, ce qu'un signe ne donne pas. Le juge recalcule donc le
numérateur $\lvert w\rvert^2\mathrm{den}-2\langle w,\mathrm{num}\rangle$ — la
quantité même dont `sphere_side` rend le signe — et compare par produit croisé en
`BigInt<4>`. `sphere.hpp` n'est pas élargi : c'est le juge qui porte cette
comparaison, comme votre §6 le prescrit.

Une inversion m'a coûté un détour et vaut d'être notée : j'avais écrit que le bon
cas était $\lvert B(\text{parent})\rvert>\lvert B(\text{fils})\rvert$. C'est
l'inverse — le parent est **moins profond**. Le témoin
`coplanar_constant_witness` l'a exhibé immédiatement, deux sommets sur trois.

**[mesuré]** grille saturée, 5 611 cas, zéro désaccord avec les quatre
assertions actives.

> **Rectification après audit indépendant du snapshot `9eee050`.** Le théorème
> de dichotomie est intact, mais la phrase chronologique « implémentée et jugée »
> était trop forte. `mhgp3v_first_incidence` exerce la factorisation sur les
> facettes qu'il reçoit ; il n'authentifie pas encore une source Gabriel ouverte
> terminale ni un univers indépendant de facettes.

Le prototype produit une source de rang fermé, son flux de suppressions, le
regroupement par facette, puis décide par branche fermée ou minimum direct. Pour
chaque facette ainsi sélectionnée, il confronte le résultat à un balayage de tous
les points extérieurs. Cette comparaison locale est utile, mais elle partage les
primitives `miniball_of` et `sphere_cmp_beta` avec le sujet et ne juge pas les
facettes que la source omet.

| $k$ | cofaces directes | records/coface | facettes | branche fermée | co-min. moy./max | points touchés/facette | désaccords |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 1 457 | 3,00 | 1 952 | 40,5 % | 1,02 / 3 | 11,0 | **0** |
| 3 | 1 543 | 4,00 | 3 447 | 57,7 % | 1,02 / 3 | 11,0 | **0** |
| 4 | 1 438 | 5,00 | 4 597 | 66,4 % | 1,02 / 3 | 11,0 | **0** |
| 5 | 1 222 | 6,00 | 5 101 | 71,8 % | 1,02 / 3 | 11,0 | **0** |

Ces chiffres disent seulement ceci sur les campagnes génériques acceptées : le
flux construit porte $k+1$ suppressions par coface, la branche fermée y domine,
et les co-minimiseurs observés ont une moyenne de 1,02 et un maximum de 3. Ils ne
donnent aucune borne : $M(F)$ peut avoir taille $\Theta(n)$. Les onze points
touchés par facette sont le nuage entier, puisque $n=11$ est inférieur à la
taille de feuille 16 ; aucun nœud interne ni élagage de l'index n'est exercé.

**Frontière exacte.** Un tétraèdre régulier avec un cinquième point sur sa sphère
possède cinq cofaces Gabriel ouvertes de taille quatre ; la source live n'en
conserve qu'une et peut néanmoins annoncer zéro désaccord parce que les facettes
omises ne sont jamais jugées. La commande
`--clouds 1 --points 7 --coord 2 --k 2 --seed 1` rend six désaccords, tandis que
`--no-judge` laisse le même domaine sortir avec succès et une conclusion
« exacte ». La source ouverte, les extra-shells, le CLI fail-closed, les
planchers et les budgets sont donc encore des portes.

Le regroupement reste en mémoire : ce sont les volumes qui sont publiés, pas un
tri externe. La dichotomie produit $M(F)$ sous ses préconditions ; elle ne
produit ni l'autorité de régularité qui autorise la rétraction vers $H_0$, ni le
réducteur, ni les verticales, ni l'identité de sortie. Les fixtures de la note —
les deux intrus de niveau $33/2$, le point exactement sur le shell, les deux
minimums directs ex æquo, les deux déduplications, la permutation des runs et le
budget moins un — restent à graver.

---

## 13. La source directe corrigée — réponse au §7.6

Le verdict est accepté : mon claim de source complète était faux, et le
mécanisme que vous décrivez est le bon. Je filtrais le rang fermé $k+1$, donc la
vacuité **fermée**, là où le théorème demande la vacuité **intérieure**. Vérifié
sur vos cinq points :

```text
Gabriel OUVERT {0,1,2,3}  extra-shell
Gabriel OUVERT {0,1,2,4}  (aussi ferme)
Gabriel OUVERT {0,1,3,4}  extra-shell
Gabriel OUVERT {0,2,3,4}  extra-shell
Gabriel OUVERT {1,2,3,4}  extra-shell
vacuite OUVERTE : 5 cofaces  |  vacuite FERMEE : 1
```

**Et vous avez mis le doigt sur le vrai défaut, qui n'est pas le filtre.** Le
juge dérivait son univers de facettes de ma propre source : une coface omise
faisait disparaître aussi la facette qui l'aurait révélée. Le juge était
circulaire, et aucun renforcement du filtre seul ne l'aurait corrigé.

**Les deux corrections.** La source développe les extra-shells : toute coface de
Gabriel ouverte $Q$ de cardinal $k+1$ a pour miniboule une sphère critique $B$
avec $I(B)\subseteq Q\subseteq I(B)\cup S(B)$, donc $Q=I\cup T$ avec
$T\subseteq S$ ; énumérer les sphères critiques puis ces $T$ est complet, à
condition que le catalogue contienne $B$ — d'où $s_{\max}=n$ et non un plafond de
rang. Et la vérité énumère **son propre** univers de cofaces à vacuité ouverte,
puis ses propres facettes, comparés aux miens avant $\lambda$ et $M$.

**Les dettes effectivement fermées depuis le §7.6.** $\lambda(F)$ est comparé et plus
seulement $M(F)$ — `truth_level` n'est plus calculé pour rien. Un statut non
`kOk` fait échouer au lieu de censurer. Le parseur est intégral :
`--clouds 1junk` et `4294967297` sont refusés. Les planchers portent sur les deux
branches. Le compteur appelé `index_internal_nodes` compte en revanche les
nœuds **construits**, pas ceux visités par les requêtes : son plancher ne prouve
aucune couverture interne. Les deux multiplicités de provenance sont comptées
séparément, mais elles ne sont ni dédupliquées ni certifiées. `deletion_bytes`,
qui n'était ni `sizeof(Record)` ni un wire défini, a été retiré.

**[mesuré]** trois régimes, tout jugé contre un univers énuméré indépendamment
de la source mais relatif aux primitives partagées :

| régime | $n$ | grille | $k$ | cofaces (manq./surn.) | facettes (manq./surn.) | fermée | co-min. moy./max | désaccords |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| cube cosphérique avec extra-shells | 8 | $[0,2)$ | 3 | 2 800 (0/0) | 2 240 (0/0) | 100 % | 2,71 / 5 | **0** |
| index à vingt points | 20 | $[0,20)$ | 3 | 2 496 (0/0) | 5 103 (0/0) | 62,4 % | 1,05 / 4 | **0** |
| ordre plus haut | 9 | $[0,3)$ | 4 | 593 (0/0) | 1 605 (0/0) | 81,1 % | 1,75 / 5 | **0** |

Votre reproduction hostile `--clouds 1 --points 7 --coord 2 --k 2 --seed 1`, qui
rendait six désaccords, rend maintenant 31 cofaces et 21 facettes sans manquante
ni surnuméraire.

**Ce que je ne prétends toujours pas.** La vérité partage encore `miniball_of` et
`sphere_cmp_beta` avec le sujet : l'oracle général reste le juge hostile et le
repli hors porte. Un échec de `miniball_of_set` est encore converti en absence
scientifique dans le sujet comme dans la vérité; cette censure commune doit
devenir un statut fail-closed et une injection. Le regroupement est en mémoire,
donc ce sont des volumes, pas
un tri externe. Les co-minimiseurs observés sont petits mais **sans borne
générale** — $\Theta(n)$ reste possible — et l'identité $k+1$ records par coface
est une identité de construction, qui dimensionne le flux sans certifier la
terminalité. Enfin la dichotomie produit $M(F)$, jamais l'autorité de régularité,
le réducteur, les verticales ou l'identité de sortie. Vos fixtures adverses du §8
— les deux intrus de niveau $33/2$, le budget moins un, la permutation des runs —
ne sont pas gravées.

---

## 14. Une attache par facette cœur — ce qui est vérifiable sans resolver

Votre théorème est accepté. Le bloc suivant conserve la première fixture exacte
à huit points, désormais renforcée dans la note par une fixture u16 à dix points
où **les trois** bras immédiats sont hors du cœur :

```text
|D_3| = 29 facettes
F={2,3,5} support=235  beta_F=258.788216040          (= 96615475/373338)
J_F (intrus STRICTS) = {4,6,7}
z_F=4  u_F=2  T_F={3,4,5}
beta(T_F)=256.250000                                  (= 1025/4, strictement plus petit)
T_F dans D_3 ? NON   ->  la cible BRUTE est REFUTEE
```

Je ne peux pas encore juger l'équivalence des deux quotients. La partie
géométrique de `Resolve` est maintenant un théorème : une descente canonique
atteint $R_F\in D_k$. Le dernier `find_<a_F(R_F)>` interroge l'histoire
horizontale antérieure au lot, et le réducteur n'existe pas. Le snapshot décrit
ici ne vérifie que le premier bras, pas encore la chaîne ni ce `find`.

> **Rectification de provenance et de domaine.** Les nombres suivants viennent
> de 30 nuages, pas des 12 du CTest. Ils sont diagnostiques hors autorité
> régulière : 3 184 branches fermées contre 3 106 facettes ayant un intrus
> strict prouvent au moins 78 égalités extérieures sans intrus.

**[diagnostic]** 30 nuages de 20 points, grille $[0,20)$, $k=3$, sur 5 103 facettes :

| grandeur | valeur |
| --- | ---: |
| intrus stricts nuls / un seul / au moins deux | 1 997 / 3 011 / **95** |
| candidats locaux à une attache | 95 |
| co-minimiseurs fermés portés par ces facettes | 210 (masse potentielle, non remplacée ici) |
| lemme $\beta(T_F)<a_F$ violé | **0** |
| cible brute hors du cœur | **6 sur 95** |

Deux lectures sont autorisées. Le lemme de descente est observé sur les 95
candidats, et **6 cibles brutes sur 95 sont hors de $D_k$** : le resolver est
bien nécessaire. En revanche, support unique essentiel et absence d'extra-shell
ne sont pas authentifiés; on ne peut appeler ces objets des attaches autorisées,
ni affirmer qu'ils remplacent les 210 co-minimiseurs, ni extrapoler un facteur de
gain.

Le plancher `--min-attachments` garantit seulement que la branche candidate est
exercée par CTest; il ne transforme pas la campagne en porte régulière et ne
juge ni la descente complète, ni le `find` pré-lot, ni l'équivalence des
quotients.
Avec ses 12 nuages, ce CTest rend 39 candidats, 88 co-minimiseurs fermés et 5
cibles brutes hors du cœur; ces nombres ne sont pas ceux de la table à 30
nuages.

46/46 CTests.

---

## 15. La reverse search est écrite — item 3 de votre porte de reprise

Le parcours sans table globale `seen` ou `frontier` existe et est différencié
contre le BFS, qui reste l'oracle borné. On descend de $v$ vers $w$ si et
seulement si $\pi(w)=v$; l'unicité du parent rend toute déduplication inutile
pour décider le parcours. L'API de test accumule toutefois encore tous les
sommets dans un vecteur local nommé `visited`: elle prouve la parité, pas encore
le high-water d'un sink streaming.

Trois détails d'implémentation qui comptent. Les voisins sont énumérés dans un
ordre **déterministe** — flats de la coquille dans l'ordre des triplets,
quotientés par base canonique, puis les deux orientations — sans quoi l'indice du
fils ne serait pas reproductible au retour. Le parent d'un candidat est recalculé
localement : une direction canonique par les deux filtres, puis **une seule**
requête de voisin, jamais une réénumération de ses voisins. Et le calcul de la
direction n'appelle aucune requête : il ne lit que la coquille, l'ensemble
intérieur et les signes tangents.

**[reproductible]** les quatre portes CTest positives du commit `969db5c`, tout
comparé au BFS sur les coquilles ET les ensembles intérieurs :

| campagne | cas | désaccords | sommets | profondeur max | fils testés / sommet |
| --- | ---: | ---: | ---: | ---: | ---: |
| fixtures | 211 | **0** | 1 578 | 7 | 5,3 |
| générique | 1 184 | **0** | 110 873 | **21** | 6,0 |
| grille saturée | 1 291 | **0** | 111 170 | 17 | 6,5 |
| cosphérique | 2 071 | **0** | 101 877 | 16 | 6,0 |

Ces portes cumulent 325 498 sommets, 2 012 590 fils testés et une profondeur
maximale observée de 21. La porte vérifie aussi qu'aucun sommet n'est visité deux
fois — le nombre de
sommets rendus égale le nombre de coquilles distinctes — ce qui est la propriété
qu'un parent faux casserait en premier.

**Ce que je ne prétends pas.** Aucune borne de temps : six calculs de parent par
sommet est une mesure de régime, et une grande coquille peut avoir un nombre
combinatoire de flats. Le compteur omet les voisins non bornés ou hors coupe et
les triplets encore parcourus après la découverte d'un enfant; ce n'est pas le
travail total. Chaque requête garde aussi un scratch $O(n)$ au pire; la
pile recopie les coquilles, et le vecteur de sortie retient $\Omega(V)$ objets.
Le delta postérieur appelle l'index, mais ses nuages permanents ont au plus 13
points pour des feuilles de 16 : aucune branche interne de l'index n'est encore
qualifiée sur ce parcours. Le catalogue passe encore
par le BFS, donc la sortie n'est pas streamée. Et rien de ceci ne touche aux
globalités de la hiérarchie que votre note isole — le `find` pré-lot du terminal
cœur, la fermeture des ex æquo, le locator horizontal, la couverture, les
verticales. La décision du
parcours est locale; le sink et le fold restent à écrire.

---

## 16. La porte locale de régularité est renforcée, pas encore globale

La correction de provenance demeure : la table historique venait de trente
nuages, pas des douze du CTest. Le binaire imprime désormais `--clouds`,
`--points`, `--coord`, `--k` et `--seed` avant ses mesures.

Le premier filtre était faux dans deux directions. Il ignorait une égalité
extérieure lorsqu'un intrus strict l'accompagnait, et ne cherchait les supports
alternatifs qu'à la cardinalité déjà rendue. Le delta
`first_incidence_dichotomy.cpp=b0741d4edcc9839ad4ab12bb58867b8c125fc83f9ab127708dcd15a91e640c17`
ferme ces deux réfutations : la fixture mixte à cinq points, les supports
antipodaux contre triangulaires et la coquille hors support sont permanents. Les
descendants sont réauthentifiés eux aussi.

Le CTest à quatorze points rend 26 descentes, quatre pas, neuf terminaux sans
intrus, dix-sept avec un intrus et 26 reçus directs sous le cutoff, sans
désaccord. Des planchers imposent désormais au moins un pas et les deux branches
terminales. Ce sont de vraies fermetures du falsificateur local.

Les deux failles fail-open locales suivantes sont désormais fermées : une panne
de miniboule sur un sous-ensemble du contrôle de support ne fait plus `continue`,
et une panne ou un refus de descendant ne peut plus être soustrait du plancher
ni être accepté sous `--require-regular`. Ce nom reste néanmoins trop fort comme
autorité globale : le théorème de quotient porte aussi sur les objets silencieux
omis, alors que le binaire ne contrôle que les facettes cœur et les chaînes
choisies.

La fixture u16 à dix points garde sa force mathématique : les trois bras de
$F=\lbrace2,8,9\rbrace$ sont strictement plus petits et tous hors de $D_3$. Son
bloc permanent ne rejoue toujours pas à lui seul les 120 triplets et 210
quadruplets de la certification transitoire. Le lookup brut reste réfuté; la
promotion du quotient reste ouverte.

---

## 17. État historique avant le dernier durcissement, supersédé par le §18

Le théorème de la note est positif, et ses deux terminaux ont maintenant des
fixtures permanentes. `E5` verrouille $\beta(F=AC)=33/2$, $\beta(T=CD)=9/2$,
$J_T=\varnothing$ et le témoin $CDE$ au niveau $162/25<\beta(F)$. La fixture à
sept points verrouille $T=126\notin D_3$ et une descente canonique d'un pas vers
le cœur. Le delta réauthentifie chaque facette, distingue refus, primitive et
budget, et recherche une coface directe terminale sous le cutoff.

Sur ce snapshot intermédiaire, ce « reçu » restait un compteur construit depuis `truth_direct`, carte
exhaustive globale de toutes les cofaces. Il ne sérialise ni $R_F$, ni la chaîne,
ni l'identité de la coface directe; la fixture `E5` imprime `CDE` sans engager
cette identité dans le résultat. Une panne de descendant peut encore être
comptée sans rendre la campagne rouge. Enfin aucun `find_<a_F(R_F)>`, aucune
partition pré-lot et aucune comparaison du quotient complet contre l'attache
réduite n'existent. Le code est devenu un bien meilleur oracle local, pas le
resolver ni le fold 50 k.

Le delta reverse épinglé par
`order_k_flats.hpp=35f3d5108cf88f0c858b4f24d6ade3cc6777f991336da6323ce899228a9491f9`
et
`flats_differential.cpp=3937261a076a389953038123265bfe5e5652fa99f3d8457c6dcc51e5c995fd01`
ferme deux défauts réels. `for_each_flat` peut maintenant arrêter toute
l'énumération dès qu'un enfant est trouvé, et l'échec du voisin d'une direction
parent admissible devient `kInvariantViolated` avec sortie vide. Des compteurs,
des planchers reverse et un second rejeu par l'API indexée sont également ajoutés.

La promotion reste prématurée pour cinq raisons.

1. L'absence de direction dans `canonical_parent` est appelée `kIsRoot` sans
   comparer le sommet au germe. Un non-germe défectueux peut encore disparaître
   avec `kOk`.
2. Les nuages permanents ont au plus 13 points pour une feuille d'index de 16.
   Le rejeu indexé ne traverse donc aucun nœud interne de `box` ou
   `sign_disagreement`; sa projection en `map` peut aussi masquer un doublon.
3. `reverse_skipped` est seulement imprimé. Aucun plancher n'impose zéro skip,
   une visite interne indexée ou un nombre de requêtes de parent.
4. Le compteur de flats ne voit que les flats canoniques parvenus au callback,
   pas les triplets non canoniques dont la fermeture a déjà été reconstruite.
5. L'endpoint matérialise toujours un `std::vector<Vertex>`, la pile recopie les
   coquilles et chaque requête garde un scratch $O(n)$. L'équivariance rejoue le
   BFS, pas l'arbre reverse.

Le progrès exact est donc une **décision de parcours locale mieux falsifiée** et
une **première exécution de la descente de carrier**. Le sink borné, la racine
fail-closed, l'index interne, l'autorité régulière et le fold global restent à
fermer.

---

## 18. Dernier delta : fermetures réelles et résiduels exacts

Les réserves structurelles du §17 sont en grande partie fermées, ainsi que les
deux failles fail-open locales du §16. Une API sink est maintenant écrite; son
intégration transactionnelle, son high-water complet et les portes d'élagage et
d'équivariance gardent les limites précisées ci-dessous.

**1. La racine est certifiée, plus déduite.** `canonical_parent` sans direction
admissible ne signifie « racine » que si le sommet EST le germe ; sinon c'est
`kBroken`, donc `kInvariantViolated` et sortie vide. Un non-germe sans direction
ne peut plus disparaître avec `kOk`.

**2. Feuilles de quatre, et une porte permanente qui a un arbre.** Seize
laissaient une feuille unique sur treize points : vous aviez raison, le rejeu
indexé ne qualifiait rien. Le juge construit maintenant l'index à feuille quatre,
et `mhgp3v_flats_indexed_tree` — vingt points, grille 40, `s_max=4` — est
permanente avec `--min-internal-nodes`. Elle rend 3 062 sommets, profondeur 19 et
**1 105 239 nœuds visités dont 472 443 feuilles** : ce sont des nœuds *visités* et
non construits, comme vous le demandiez. L'auto-test juge désormais `box` et le
désaccord ternaire contre l'exhaustif — pour le désaccord, l'inclusion et non
l'égalité, puisque la requête est sûre et non exacte — et il exige qu'au moins une
requête synthétique `box` visite strictement moins de nœuds que l'arbre n'en
contient. Cela ne mesure pas encore l'élagage propre aux requêtes du parcours.
La projection en `map` compare sa taille au
nombre de records des deux côtés : un doublon indexé ne peut plus être masqué.

**Un fail-open que vous n'aviez pas nommé, et qui était pire.** En lisant `box`
pour la mettre sous test j'ai trouvé qu'à pile saturée elle **omettait le
sous-arbre en silence**, là où `closed_ball` et `sign_disagreement` retombaient
toutes deux sur une descente récursive. Elle retombe désormais comme elles.

**3. Les planchers portent enfin sur ce qui compte.** `--min-reverse` implique
zéro porte sautée, et les planchers couvrent les sorties **indexées**, les
requêtes de parent et les nœuds visités. Les quatre portes — fixtures, générique,
grille saturée, cosphérique — les portent toutes, et non plus deux.

**4. Le travail total est mesuré, et il est cinq fois plus grand que « six flats
par sommet ».** Deux compteurs comptent tous les triplets balayés et toutes les
fermetures reconstruites, y compris celles des triplets écartés. Générique à onze
points : 10 626 sommets, 72 236 flats livrés — 6,8 par sommet —, mais **335 314
triplets et autant de fermetures, soit 31,6 par sommet**, chacune un `orient3d`
par point de coquille. Le ratio que je publiais était optimiste d'un facteur 4,6.

Et une petite chose tombe de là : les deux compteurs sont **toujours égaux**, ce
qui n'est pas un hasard de régime. Trois points distincts d'une même sphère ne
sont jamais alignés — une droite coupe une sphère en au plus deux points —, donc
le garde de colinéarité est inactif sur une coquille. La porte exige leur égalité,
et une divergence dénoncerait une coquille non cosphérique ou un point double.

**5. L'équivariance rejoue la reverse search, et le sink est écrit.** La
signature de permutation contient maintenant l'ensemble des sommets atteints par
la reverse search, ramenés par la permutation inverse, plus son statut. Ce qui est
exigé est bien l'**ensemble** : le germe, l'arbre et l'ordre des fils dépendent de
la numérotation, puisque la direction canonique du parent compare des clefs
d'indices. Ce que la porte établit est donc que le même ensemble géométrique est
atteint **depuis une autre racine** — ce qui est le contenu falsifiable, et je ne
prétends pas plus. Cette projection dans un `set` peut toutefois masquer un
doublon propre à une permutation et ne transporte pas l'ensemble intérieur.
**Le sink est écrit.** C'était votre
dette la plus lourde et elle était juste : tant que l'endpoint rendait un
`std::vector<Vertex>`, aucun gain mémoire n'était démontré.
`reverse_search_stream` rend les sommets un à un et publie le high-water des slots
vifs. La porte le juge avec un consommateur qui ne tient **rien** — il compte et
replie un hachage indépendant de l'ordre —, et exige que l'interruption stoppe le
parcours avec un statut non `kOk`. Ce dernier test a attrapé mon trou sur-le-champ :
le refus du **germe** revenait avec `kOk`. Il refuse encore le germe au premier
callback; l'arrêt après un préfixe et la branche enfant ne sont pas exercés. Le
sink est aussi appelé sans `CertifiedIndex`, donc la composition streamée-indexée
reste hors porte. **[mesuré]** vingt points, cinq
nuages : 5 400 sommets rendus, **85 identifiants portés au maximum par les sommets
du chemin**, 109 interruptions vérifiées, zéro désaccord détecté avec la sortie
matérialisée. Le rapport $5400/85$ n'est pas un facteur mémoire : il compare des
records à des identifiants et ignore capacités, temporaires, bitmaps, fermetures,
index et allocateur. Une coquille cosphérique peut être $\Theta(n)$ et la
profondeur n'est bornée par aucun théorème. La parité repose en outre sur un
compte et une somme de hachages 64 bits, donc sur un falsificateur avec collisions
possibles, pas sur une égalité exacte.

**Votre §16, les deux trous.** La panne de primitive sur un sous-ensemble du
contrôle de support faisait `continue` — donc pouvait déclarer unique un support
qui ne l'est pas : elle est maintenant fail-closed. Et une panne ou un refus dans
une descente n'entrait ni dans le booléen `regular` ni, pire, dans mon plancher,
qui **soustrayait** les refus. Le plancher ne soustrait plus rien, et
`--require-regular` exige zéro refus et zéro panne de descendant : une campagne
qui se déclare régulière ne peut pas contenir un descendant où le théorème n'est
pas démontré.

**Votre §17, le reçu.** Vous avez raison qu'un compteur n'engage personne. Le type
de reçu porte maintenant le terminal atteint, la longueur de la chaîne, la
branche, l'**identité** de la coface directe engagée et son niveau. La fixture
`E5` ne vérifie plus « un reçu existe » mais que la coface engagée est exactement
$\lbrace2,3,4\rbrace$ — $CDE$ — de niveau $162/25$, terminal $CD$, zéro pas,
branche vide ; la fixture à sept points vérifie que le terminal est dans le cœur,
que la coface engagée le contient et que son niveau est strictement sous le
cutoff. Les campagnes ordinaires passent encore `receipt=nullptr`. La fonction
n'efface pas non plus un reçu fourni à l'entrée et peut le remplir après un
terminal hors cœur ou un niveau trop tardif; seul un statut typé et une affectation
après toutes les portes donneront l'atomicité requise.

**Ce que je ne prétends toujours pas.** Le reçu est construit depuis
`truth_direct`, carte exhaustive de la vérité : il **juge** le théorème, il ne
l'implémente pas. Il n'y a ni $R_F$, ni partition pré-lot, ni
$\mathrm{find}_{<a_F}$, ni comparaison du quotient complet contre l'attache
réduite. La porte globale sur les objets silencieux omis reste ouverte, et le
binaire ne contrôle que les facettes cœur et les chaînes qu'il choisit. Ce qui est
acquis est un **oracle local nettement mieux falsifié**; le resolver et le fold
50 k restent à écrire. De même, un sink produit devra écrire un segment non
committé : le callback peut déjà avoir émis un préfixe quand une erreur ultérieure
rend le statut rouge, et `kInvariantViolated` confond actuellement contradiction
scientifique et arrêt demandé par le consommateur.

---

## 19. Le poste de coût, et une inférence de ma part qui était fausse

Vous allez regarder le coût de votre côté ; voici ce que j'ai trouvé du mien, y
compris ce que j'ai dû retirer.

**Le coût n'était pas là où le compteur le montrait.** Le parcours n'énumère
qu'une fois les flats du sommet courant, mais chaque fils candidat payait une
énumération **complète** des flats de son propre sommet — 5,7 par sommet visité.
C'était la requête de parent, pas la descente.

Et le minimum n'était pas l'hypothèse utile. Ce qu'Avis--Fukuda demande de $\pi$
est d'être une fonction déterministe du sommet **seul**, acyclique, à racine
unique ; les deux dernières viennent du potentiel et valent pour **n'importe
quel** choix admissible, puisque toute direction admissible fait strictement
croître $L_h$ à intérieur égal ou décroître $Q_r$ au niveau zéro. Le **premier**
couple admissible est donc un parent légitime, et l'arrêt est immédiat.

Il se trouve en plus que ce premier admissible **est** l'ancien minimum, et cela
tient au même petit fait que le reste : trois points distincts d'une même sphère
ne sont jamais alignés. La base canonique d'un flat de coquille est donc le
triplet de ses trois plus petits éléments, deux flats distincts se séparent dès
les trois premiers, et comparer les fermetures revient à comparer les bases —
`for_each_flat` balayant la coquille **triée**, il livre les flats en ordre
croissant de clef. Cette affirmation-là n'étant pas une évidence, une porte
vérifie la monotonie clef par clef sur **chaque** sommet et compare le parent
précoce au balayage complet. Zéro désaccord.

**[mesuré]** générique à onze points : 335 314 → **171 856** fermetures, soit
31,6 → 16,2 par sommet ; sur la requête de parent isolée, 44 036 → 16 981,
rapport 2,59.

**Et voici l'inférence que j'ai retirée avant de la publier.** J'avais rapproché
deux colonnes du README — 1 027 sommets par point et 1 072 sphères d'arité quatre —
et conclu qu'un sommet sur 285 seulement portait de la sortie, donc qu'adresser
l'énumération par **plan** plutôt que par sommet valait deux ordres de grandeur.
C'était faux deux fois : les sommets sont comptés sur le niveau strict
$\le s_{\max}-2$ et les critiques sur le rang fermé $\le s_{\max}$, et les deux
lignes ne venaient pas du même profil de nuage.

Mesuré sur une seule fenêtre et un seul profil, cube uniforme, $s_{\max}=11$ :
**un sommet sur 6,5 à 6,9** porte une sphère critique d'arité quatre — 113,7 puis
145,7 puis 167,5 par point pour $n=100,200,300$. Le filtre de criticité vaut donc
sept à onze, et l'adressage par plan n'attaquerait que ce facteur-là. Je le dis
parce que c'est exactement le genre d'inférence que vous auriez réfutée, et qu'elle
aurait orienté deux semaines de travail.

**Ce qui reste devant est un problème de débit, et c'est mesurable.** Les deux
colonnes croissent encore — 772 → 999 → 1 097 sommets par point et 171 → 210 → 235
sphères par point entre $n=100$ et $n=300$ —, donc rien n'est extrapolable
proprement. Mais la **forme** est maintenant connue : de l'ordre de 65 prédicats
entiers par sommet, sans table de visitation dans la décision, sans sortie
matérialisée, avec un high-water publié. Ce sont les trois conditions d'un front
d'onde device, et c'est pourquoi les 48 cœurs ne suffisent pas là où le GPU
pourrait. Le kernel n'est pas écrit, la croissance n'est pas stabilisée, et le
Kruskal $K$-MST qui doit consommer le flux n'existe pas : le NO-GO tient.

