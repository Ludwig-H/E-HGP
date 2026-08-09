# Audit de `order_k_flats` — snapshot `9c587e6`

Date : 9 août 2026 UTC.

> [!CAUTION]
> **Verdict : NO-GO pour fermer la correction du germe, promouvoir la navigation multiplicitaire ou relier ce prototype au contrat 50 k.** Un nuage u16 valide de cinq points réfute la « descente stricte du rayon » : le sujet rend `germe_non_certifie`, étape 6, puis un catalogue vide. Le même nuage réussit ou échoue selon sa renumérotation. Les collisions de quantification produisent une seconde non-invariance sans être déclarées par le statut. Les campagnes vertes restent utiles comme falsificateur borné, mais leur vérité partage des primitives décisives avec le sujet et leur payload ne couvre ni la géométrie complète du catalogue, ni sa sérialisation, ni les forêts.

Cadre de cet audit : aucune phase officielle n'est ouverte; `backend=reference_cpu_local`, `profile=order_k_multiplicities_prototype`, `mode=exploration/diagnostic_only`, `public_status=not_claimed`. Ces étiquettes décrivent le travail d'audit et ne créent aucune entrée dans le registre produit.

## 1. Snapshot épinglé

| objet | identité |
| --- | --- |
| `HEAD` au début de l'audit | `2cf30abbf5f89bcc05a07a8c7ac17bfd182d1ffd` |
| `prototype/order_k_flats.hpp` non suivi | `9c587e674b04510ad0876f9142074f76954c4905bad70368ff31d9c306e16056` |
| `prototype/flats_differential.cpp` non suivi | `00e5ff16deaf990480d47a09636df4fb3dbc57287658b572e40186ee6d7c5d37` |
| `CMakeLists.txt` live | `30110825a225470cd1ce74536acd7233bfdc1063d9ee18f48820190bab4cf759` |
| `README.md` live | `f955a1f022624ce0dec5985712e578176610228c3c1a0233a78b1b41962cd18d` |
| binaire Release du différentiel | `0b34ba948b747eb14c22efe59f8dafe62c1abc48faa7951c3db2b090660ac546` |

Les deux sources étaient non suivies pendant leur lecture. Claude les a ensuite committées sans changement d'empreinte dans `68b1938aba343be2ffe80b0a4062cbd01d6bc5ed`; le verdict s'applique donc aussi à ce commit. Toute correction doit être traitée comme un delta distinct.

## 2. P0 — la descente stricte du rayon est fausse

Le germe cherche un triangle de Delaunay dans une face support. Les lignes 470--474 du header et les lignes 105--117 du README affirment que, si un point est strictement intérieur au cercle d'un triangle, l'un des trois triangles obtenus en remplaçant un sommet a un rayon strictement plus petit. Les lignes 533--538 refusent donc tout remplacement de rayon égal.

La fixture suivante est sur la grille u16, porte cinq points distincts et a une dimension affine égale à trois grâce à `Q` :

| identifiant | coordonnées |
| --- | --- |
| `A` | `(0,0,0)` |
| `B` | `(0,3,0)` |
| `C` | `(2,1,0)` |
| `P` | `(1,1,0)` |
| `Q` | `(1,1,2)` |

La face support est le plan `z=0`. L'énumération par identifiants choisit d'abord `ABC`; le prédicat entier donne `in_circle_coplanar(A,B,C,P) = -72`, donc `P` est strictement intérieur. Pourtant les quatre rayons carrés sont exactement égaux : $R^2(ABC)=R^2(ABP)=R^2(BCP)=R^2(CAP)=\frac{5}{2}$. Des centres possibles sont respectivement $(\frac{1}{2},\frac{3}{2})$, $(-\frac{1}{2},\frac{3}{2})$, $(\frac{3}{2},\frac{5}{2})$ et $(\frac{3}{2},-\frac{1}{2})$ dans le plan.

Reproduction Release locale :

```text
status=germe_non_certifie stage=6 shell=0 level=0
current_beta=2.5 intruder_side=-1 incircle_sign=-1
drop=A beta=2.5
drop=B beta=2.5
drop=C beta=2.5
```

L'effet public est plus grave qu'un diagnostic de construction. Pour chaque `s_max` de 2 à 6, `flat_catalogue` tente les cinq singletons, puis retourne avant de sérialiser `kept` :

```text
smax=2 status=germe_non_certifie stage=6 spheres=0 members=0 attempts=5
smax=3 status=germe_non_certifie stage=6 spheres=0 members=0 attempts=5
smax=4 status=germe_non_certifie stage=6 spheres=0 members=0 attempts=5
smax=5 status=germe_non_certifie stage=6 spheres=0 members=0 attempts=5
smax=6 status=germe_non_certifie stage=6 spheres=0 members=0 attempts=5
```

Ce n'est pas seulement une censure dépendant de la géométrie. Sur les 120 permutations des cinq identifiants, 90 construisent un germe et 30 échouent à l'étape 6; les 30 échecs sont exactement ceux où `P` arrive après les trois autres points coplanaires dans l'ordre employé pour choisir le premier triangle. Le statut et le catalogue dépendent donc de la numérotation, avant même la convention de support canonique.

La fixture n'est pas dans `flats_differential.cpp`; la campagne permanente de 152 cas reste verte. La porte D2 annoncée fermée dans la réponse aux audits et le §2 du README doivent rester ouvertes.

### Conséquence de correction

Remplacer `>= 0` par `> 0` ne suffit pas : autoriser les égalités sans ordre bien fondé peut créer un cycle. Il faut soit une construction exacte de Delaunay planaire dont la terminaison et les cas cocirculaires sont démontrés, soit une recherche exhaustive bornée d'un cercle vide pour le prototype. La nouvelle fixture doit être permanente et tester les 120 permutations, le statut, la coquille, le niveau et le catalogue.

Le garde `q*q+8` mérite une preuve séparée : la seule finitude des triangles donne au plus un nombre cubique de candidats et ne justifie pas ce plafond quadratique. Il porte en outre un P0 propre au contrat 50 k : `q` est converti en `int`, donc le produit déborde dès `q >= 46341`. Un nuage affine-3 de 50 000 points peut avoir 49 999 points dans la face support; `49999*49999+8` dépasse `INT_MAX` et déclenche un overflow signé avant même la navigation.

### Point positif — la non-récolte explicite des quadruplets

L'argument des lignes 930--935 paraît correct sous les préconditions de points distincts et de miniboule exacte. Un support minimal d'arité quatre est affinement indépendant, détermine donc le sommet et sa sphère; la miniboule de la coquille entière reste cette sphère, puisque le centre est déjà dans l'enveloppe du support. Un quadruplet affinement dépendant est coplanaire et sa miniboule possède un support d'arité au plus trois, récolté par la boucle dédiée. Ce crédit ne ferme ni le germe, ni le domaine des collisions, ni la règle de propriétaire.

## 3. Ce que les verts établissent réellement

Crédit au delta CMake live : le helper négatif propre au nouveau binaire a remplacé le branchement transitoire sur l'oracle historique. Le CTest généré cible bien `mhgp3v_flats_differential` et les trois rejets rendent les codes 2, 2 et 3.

Rejeu Release local sur le snapshot épinglé :

| porte | cas | désaccords | temps local |
| --- | ---: | ---: | ---: |
| fixtures | 152 | 0 | 0,20 s |
| générique | 1 087 | 0 | 8,29 s |
| grille saturée | 1 202 | 0 | 6,65 s |
| cosphérique | 1 952 | 0 | 5,51 s |
| total positif | 4 393 | 0 | 20,65 s |
| interface négative | 3 tests | 0 échec CTest | 0,15 s |

UBSan reste vert sur les 152 fixtures et sur 502 cas de la campagne dégénérée dans le domaine u16. Ces résultats créditent le quotient par flats, les transitions par lots et le transport sur les cas effectivement atteints. Ils ne ferment pas les limites suivantes.

### 3.1 La vérité ne partage pas seulement `sphere_side`

Le commentaire CMake, le début du différentiel et le README disent que sujet et vérité ne partagent que `mhgp::sphere_side`. Le code partage aussi :

- `mhgp::sphere4` pour construire les sommets exhaustifs;
- `mhgp::miniball_of` pour décider les candidats du catalogue et relire le support canonique, exactement comme le sujet.

Une faute commune de miniboule, de bon centrage ou de support canonique est donc invisible. Le différentiel est une bonne vérification de la portée de navigation relativement à ces primitives; ce n'est pas une autorité indépendante pour le catalogue critique exact.

### 3.2 Le payload comparé est incomplet

Le catalogue sujet est réduit à un `set` de couples `(support, rank)`. Ne sont pas comparés :

- les doublons émis;
- `members`, leur ordre et `members_begin`;
- le centre rationnel, le rayon exact et `beta`;
- l'ordre de sérialisation et donc les futurs indices `ForestNode::source`;
- les forêts, absentes de ce chemin.

Une mutation de ces champs pourrait laisser les CTests verts. La formulation exacte est donc « ensembles support--rang concordants sur le domaine atteint », pas « catalogue complet jugé ».

### 3.3 La porte peut devenir exhaustive sans le signaler

`flat_catalogue` bascule sur l'énumération exhaustive des sous-ensembles dès que `affine_dimension_is_three` rend faux. Le juge accepte les statuts `kAffineDimensionBelowThree` et `kTooFewPoints`, puis ne compare les sommets que pour `kOk`. Aucun plancher n'exige un nombre de nuages navigués, de sommets, de census, de flats quotientés, de coquilles multiples ou de lots multiples.

Une régression qui classerait tous les petits nuages en dimension inférieure ferait donc comparer l'exhaustif du sujet à l'exhaustif de la vérité et pourrait garder toute la porte verte sans exercer la navigation. Chaque campagne doit imposer des minima sur les statuts et branches qui donnent son nom, et les publier dans le reçu.

Même quand `verify_census=true`, une contradiction de coquille ou de niveau ne positionne pas `kInvariantViolated` : les lignes 682--688 incrémentent seulement les compteurs et poursuivent. Le différentiel lit ces compteurs, mais un autre appelant peut publier le catalogue avec un statut `ok` s'il ne les inspecte pas. Le mode de vérification doit être fail-closed à l'API, ou son absence d'autorité doit être explicite dans le type de retour.

### 3.4 L'équivariance est beaucoup plus étroite que la campagne

`permutation_equivariant` est appelé seulement sur les 19 fixtures, à `s_max=5`. Il ne l'est jamais sur les nuages génériques, saturés ou cosphériques; sa signature ignore `CloudStatus` et les compteurs, avec `verify_census=false`. La nouvelle fixture du §2 démontre que cette couverture manque un défaut réel d'équivariance.

Le cas de coordonnées dupliquées est en outre annoncé « hors contrat » dans le header et « déclaré à part » dans le README, sans contrôle ni statut correspondant. La commande `--clouds 1 --points 10 --coord 2 --smax 2 --seed 1 --min-cases 153` force un doublon par le principe des tiroirs, mais rend 0 avec « équivariance concordante » alors que cette équivariance n'est pas exécutée sur le nuage aléatoire.

Ce n'est pas seulement une lacune de couverture. La fixture affine-3 `(0,0,0)`, `(0,0,0)`, `(2,0,0)`, `(0,2,0)`, `(0,0,2)` rend `status=ok`. Échanger les deux observations coïncidentes conserve 11 records mais change quatre supports une fois ramenés aux identifiants d'origine : le singleton et les trois paires incidentes choisissent l'autre doublon. Le profil quantifié doit définir les collisions et multiplicités; à défaut d'une sémantique quotientée, le prototype doit au minimum les refuser explicitement au lieu de publier `ok`.

### 3.5 Le CLI n'est pas fail-closed

Les entiers sont lus avec `atoi`. Le suffixe invalide suivant est accepté :

```text
$ mhgp3v_flats_differential --clouds 0junk --min-cases 152
152 cas, 0 desaccords
OK : sommets, catalogue, census et equivariance concordants
exit_code=0
```

Le CLI n'impose pas non plus la grille u16 dont dépendent les bornes des prédicats. Un binaire UBSan exécuté avec `--clouds 1 --points 11 --coord 2147483647 --smax 2 --seed 1 --min-cases 153` échoue sur un overflow signé `__int128` dans `in_sphere_side`, ligne 174. Le correctif attendu est un parseur entier intégral strict, des bornes sémantiques explicites et leurs CTests négatifs.

### 3.6 Deux énoncés de preuve doivent être rectifiés

Le code transporte correctement les lots par des boucles, mais le commentaire du header et le README affirment encore que le niveau de deux sommets voisins varie seulement de $-1$, 0 ou $+1$. Avec la formule publiée, la variation est $\lvert D_-\rvert-\lvert A_{\mathrm{int}}\rvert$ et son amplitude n'est pas bornée par 1 quand plusieurs hyperplans coïncident sur l'événement.

Le théorème de propriétaire est par ailleurs énoncé pour `q` de 1 à 4, puis la chaîne `s_max-q <= s_max-2` est appliquée uniformément. Cette dernière inégalité ne vaut que pour `q >= 2`; les singletons sont correctement publiés à part dans le code, mais la preuve écrite doit le dire.

## 4. Le contrat 50 k reste architecturalement hors de portée de ce fichier

Le nouveau parcours répare conceptuellement la coupe par niveau et les multiplicités, mais conserve exactement les structures globales que le contrat interdit comme architecture produit :

- avant même le germe, les `n` singletons passent chacun par `try_emit`, qui rescane les `n` points : exactement 2,5 milliards d'appels à `sphere_side` à 50 k, avant la navigation;
- `seen`, `frontier` et `visited` stockent le sous-graphe d'arrangement parcouru et ses coquilles;
- chaque flat incident lance deux balayages de tous les `n` points;
- un sommet simple paie ainsi exactement `8(n-4)` candidats de pinceau;
- une coquille de taille `m` énumère les $\binom{m}{3}$ triplets avant quotient, puis les paires et triplets de récolte;
- chaque tentative d'émission refait un census global en $O(n)$; `miniball_of` sur une grande coquille peut elle-même énumérer jusqu'aux 4-sous-ensembles.

Le coût reste donc au moins proportionnel à $nV$ pour `V` sommets visités, avec des facteurs combinatoires en taille de coquille, et la mémoire reste proportionnelle aux sommets et coquilles matérialisés. Il ne calcule ni les forêts horizontales et verticales, ni les incidences silencieuses, ni les lots exacts et `coverage_log` du contrat HGP complet.

Les portes permanentes utilisent au plus 13 points et `s_max` au plus 8. Elles ne mesurent ni 50 000 points, ni le chemin complet à $K=10$, ni un pic mémoire. L'absence d'index, de propriétaire local, de reverse search ou de streaming est donc une porte d'architecture avant toute nouvelle mesure à l'échelle, pas une optimisation finale.

Ce chemin est CPU-only. Conformément à la consigne utilisateur, aucune G4 ne doit être consommée pour le compiler, le falsifier ou le chronométrer. Une future G4 ne sera justifiée que par un véritable kernel CUDA participant au pipeline qualifié.

## 5. Portes de reprise proposées à Claude

1. Ajouter immédiatement la fixture du §2 et ses 120 permutations; garder le snapshot rouge avant correction, puis exiger statut, germe, niveaux et catalogue complets après correction.
2. Remplacer la descente de rayon par une construction planaire exacte avec preuve de terminaison en présence de cocircularités; ne pas valider une simple tolérance aux égalités.
3. Séparer l'oracle des primitives `sphere4` et `miniball_of`, ou borner explicitement l'autorité du différentiel à la portée de navigation; comparer tout le payload du catalogue et les statuts.
4. Ajouter des planchers de couverture par campagne : nuages réellement navigués, sommets/census, flats quotientés, coquilles multiples, lots multiples et cas directs, avec accord de domaine.
5. Fermer le CLI et le domaine : parsing intégral, grille u16, doublons soit rejetés explicitement, soit définis puis testés sous permutations.
6. Conserver `exploration/diagnostic_only`. Avant 50 k, remplacer la matérialisation globale de l'arrangement par une source locale/streamée prouvée complète et brancher le pipeline HGP entier; seulement ensuite mesurer CPU et CUDA sur le matériel approprié.

Décision : créditer la correction du plafond de niveau, le quotient multiplicitaire, le nouveau census et les 4 393 cas verts. Refuser les claims « D2 fermée », « invariance sur toutes les campagnes », « vérité indépendante sauf `sphere_side` », « catalogue complet jugé » et toute promotion vers le contrat 50 k au snapshot `9c587e6`.

GCP non utilisé.
