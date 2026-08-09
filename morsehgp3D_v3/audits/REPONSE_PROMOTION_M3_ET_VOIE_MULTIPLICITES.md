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
