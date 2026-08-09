# Réponse aux audits `AUDIT_PROMOTION_M3_2E3FA7B` et `AUDIT_DELTA_ORDER_K_2E3FA7B_468635C`

Date : 9 août 2026 UTC. Objet : la promotion M3 est retirée, la voie
multiplicitaire est implémentée et jugée, et deux résultats nouveaux sont
versés au dossier — dont un qui corrige l'arithmétique de la question 50 k.

> [!IMPORTANT]
> **Le verdict des deux audits est accepté sans réserve.** Les quatre P0 ont été
> reproduits ici, sur le header committé `a6d0a3e…`/`47ee376…`, contre une force
> brute écrite indépendamment. Ils sont réels. Le README est revenu à
> `exploration_v3`.
>
> **Deux faits nouveaux, tous deux défavorables à mes propres affirmations
> antérieures.** Le rapport 100:1 travail/sortie était un **artefact de la
> récolte défaillante** : mesuré contre le catalogue complet, il vaut 17:1, mais
> la sortie est six fois plus grosse qu'annoncée. Et la convention de support
> canonique la plus naturelle — plus petit sous-ensemble par identifiant — est
> **non invariante par permutation** ; une seule renumérotation suffit à changer
> la sortie sur le cube.

## 1. Reproduction indépendante des P0

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
| D2 — vrai germe de niveau zéro sur une face non triangulaire | **fermée** | triangle de Delaunay du sous-nuage coplanaire, par descente de rayon exacte |
| D3 — non-régression de `468635c` (coquille constante) | **fermée** | fixture `constant_shell_members` |
| D6 — niveaux et coquilles comparés à un oracle indépendant sur coplanarités, cosphéricités, limites u16 | **fermée** pour cette vérité ; **ouverte** pour l'oracle rationnel M1 | voir §5 |
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
catalogue **complet**, vérifié contre la force brute :

| $n$ | sommets/point | critiques/point | travail/sortie |
| ---: | ---: | ---: | ---: |
| 100 | 776,9 | 49,4 | 15,7 |
| 200 | 935,5 | 55,7 | 16,8 |
| 300 | 1 027,2 | 60,7 | 16,9 |

Profil LiDAR à densité fixe, emprise $\propto\sqrt n$, $s_{\max}=11$, un cœur.
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

- **L'oracle M1 n'a pas été étendu.** Sa référence déclare hors domaine tout nuage portant un point surnuméraire sur une coquille — précisément le régime que ce parcours traite. Le juge utilisé ici est indépendant du germe, du pinceau et du transport, mais il partage `mhgp::sphere_side` avec le sujet et n'est pas en arithmétique rationnelle. Une référence multiplicitaire rationnelle est nécessaire, et elle devra être auditée à part.
- **Aucun accélérateur spatial.** Les P0 de `Grid::ball` ne sont pas corrigés ; ils sont hors du chemin. Le contrat *fail-open* s'appliquera intégralement au premier index écrit.
- **Ni forêts, ni reverse search, ni propriétaire.** La récolte déduplique encore par une table globale de coquilles ; la mémoire de navigation reste proportionnelle au nombre de sommets.
- **Aucune mesure à l'échelle.** Les tailles mesurées vont de 4 à 300 points. Rien à 50 000, aucun pic mémoire, aucun reçu scellé.

## 6. Campagnes

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
l'oracle en dur ; un test écrit pour un autre juge interrogeait donc
silencieusement l'oracle et passait pour la mauvaise raison. Corrigé.

## 7. Ce que je demande à l'audit suivant

1. Le théorème de propriétaire est utilisé ici comme **droit de ne pas récolter les 4-sous-ensembles** et comme plafond $s_{\max}-2$. L'argument de redondance des 4-sous-ensembles est-il complet ?
2. La convention de support canonique par ordre des coordonnées est-elle acceptable comme contrat public, sachant que l'invariance topologique reste indémontrée ?
3. Le classement d'angle du germe est-il correct dans tous les cas, y compris quand le plan vertical support contient plus de trois points ?
4. La vérité du différentiel partage `mhgp::sphere_side` avec le sujet. Est-ce une dépendance acceptable tant que l'oracle rationnel n'est pas étendu, ou faut-il la retirer d'abord ?
5. Quelle est la bonne priorité entre l'index *fail-open* et la règle de propriétaire ? La mesure dit que l'index vaut un facteur $10^3$ et le propriétaire un facteur inférieur à 2 ; mais le propriétaire ferme aussi une porte de mémoire.

GCP non utilisé. Toutes les mesures viennent du codespace, deux vCPU, `-O3
-march=native`, un seul cœur — et c'est délibéré : ce sont des charges CPU, elles
n'ont rien à faire sur la G4.
