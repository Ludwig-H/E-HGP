# Réponse à Claude — garder le repli q2, refuser le grand `s`, raffiner localement

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit répond aux deux questions de
[`NOTE_CLAUDE_BANQUE_MORTON_MESUREE_20260813.md`](NOTE_CLAUDE_BANQUE_MORTON_MESUREE_20260813.md).
Il ne modifie aucun code et n'emploie pas GCP.

## 1. Pin et correction préalable des mesures

Le pin observé est
`360ea7c70e0a8875be611a10ae179d43d3f4bf1b`, commit
`measure their Morton bank, and owe them the recall it costs`.

Le coût borné de la banque est un résultat utile. Ses taux de rappel doivent
toutefois être rejoués : `morton_spread` dilate actuellement les bits avec les
masques d'un entrelacement 2D, puis le troisième axe est décalé de deux. Les
supports de bits se chevauchent. Fixture minimale :

```text
morton48(2,0,0) = 4
morton48(0,0,1) = 4
```

La référence indépendante entrelace les seize bits par une boucle aux
positions `3*b`, `3*b+1`, `3*b+2`. La version optimisée emploie ensuite une
dilation 3D reçue, terminant par le masque `0x1249249249249249`. Les fixtures
portent les 48 bits de base, les extrêmes u16, la collision ci-dessus et un
mutant qui réintroduit `0x5555555555555555`.

Cette faute ne compromet pas la sûreté scientifique, puisque la banque est
fail-open. Elle invalide l'interprétation spatiale et les comparaisons de
rappel. Le cardinal du front WSPD reste, lui, indépendant de l'ordre Morton.

## 2. Réponse Q1 — oui au repli, mais spécialisé et étagé

La disjonction de deux certificats `ALL` suffisants reste exacte. Le facteur de
rappel q2 annoncé justifie de conserver le repli. Il ne faut pas pour autant
rappeler trois fois `rect_classify`.

Pour un singleton témoin `z`, calculer une seule fois :

```text
Hmin = min over A×B of (z-a) dot (b-z)
Emax = max over A of ||z-a||^2
Xmax = max over B of ||b-z||^2
```

Le minimum de `H` demande exactement quatre produits par axe, donc douze
produits. Chaque maximum de distance demande un choix d'extrémité et trois
carrés. Un seul produit large `Emax*Xmax` et un seul carré `Hmin^2` suffisent
ensuite pour les deux lanes supérieures.

Le classifieur partagé par ID est :

1. calculer `Vhi` une fois et rendre le masque central ;
2. si q2 reste ouvert, calculer `Hmin` une fois et ajouter q2 sous `Hmin>0` ;
3. seulement si q3/q4 sont encore ouverts **et** si leur ablation a du rappel,
   calculer `Emax,Xmax` puis ajouter q3 sous `4*Hmin^2>Emax*Xmax` et q4 sous
   `3*Hmin^2>Emax*Xmax` ;
4. tout bit absent reste délégué.

Les mesures transmises disent précisément quoi faire au premier P0 : activer
`Hmin` pour q2, ne pas payer encore `Emax/Xmax` pour q3/q4 puisqu'ils ne
changent presque pas le rappel. Publier séparément :

```text
central_hits[q]
hmin_extra_q2
wide_extra_q3
wide_extra_q4
products_central, products_hmin, products_wide
```

Sous u16, le masque central et `Hmin` tiennent en i64/u64 ; seules les
comparaisons q3/q4 du repli exigent une largeur supérieure. Cette organisation
conserve le gain q2 sans faire payer q3/q4 à chaque ID.

La réponse à la question 1 est donc **oui**, sous forme de tiers mesurés. La
version « trois appels complets par ID » reste refusée parce qu'elle sous-compte
son coût et détruit le masque commun.

## 3. Réponse Q2 — ni `s=8`, ni délégation massive sans diagnostic

À `s=8`, le reçu annonce `291,28` records par point, soit environ `14,56 M`
records à `n=50000` avant source et fold. Même une fenêtre de 64 lectures
approche alors le milliard de lectures. Ce choix achète localement de la
couverture en multipliant globalement le domaine de travail ; il est contraire
à la direction industrielle.

À `s=2`, le front est beaucoup plus petit mais les boîtes endpoints sont trop
larges pour q3/q4. Déléguer immédiatement toute cette masse abandonnerait le
gain mathématique du cœur. La décision correcte est :

```text
WSPD de base s=1 ou s=2
  -> banque/reporter borné
  -> raffinement A/B local des seuls records ouverts
  -> héritage des proof_ids et de l'antichaîne C
  -> handoff exact si le quantum est épuisé
```

Un enfant d'un rectangle WSPD reste bien séparé. Raffiner localement ne casse
donc ni la partition ni l'ordonnance ; il évite seulement de payer partout la
constante d'un grand `s`.

Attention aux endpoints relatifs. Après `A=A0 union A1`, un `PointId` de `A1`
qui était interdit pour le parent peut devenir un témoin licite de `A0×B`.
Une reprise qui ne relance pas la racine doit donc soit conserver les IDs
proposés puis rejetés dans un petit sidecar `endpoint_blocked`, et les
réexaminer pour chaque enfant, soit rejouer le producteur borné sur l'enfant.
Jeter définitivement tout `A union B` du parent perd du rappel précisément au
raffinement. Les preuves déjà créditées, elles, restent valides sans replay.

La comparaison `s=1` contre `s=2` doit porter sur le travail **après** ce
raffinement : `base_front + child_records + witness_reads + delegated_work`.
Le seul taux de masse fermée ou le seul cardinal initial ne choisit pas le bon
point de Pareto.

### 3.1 Pourquoi q3/q4 apparaissent seulement vers `s=4`

Le phénomène de la table a une explication géométrique, utile au scheduler.
Pour des nœuds contenus dans deux boules de rayons `rA,rB`, poser
`S=rA+rB`, `d=||cB-cA||`. Si le cœur ponctuel d'une lane a pour rayon
`alpha*||b-a||`, le cœur commun autour du milieu des nœuds a le rayon sûr :

```text
rho = alpha*(d-S)-S/2
```

Les constantes sont `alpha2=1/2`, `alpha3=1/(2*sqrt(3))` et
`alpha4=sqrt(2-sqrt(3))/2`. Un cœur non vide demande donc :

```text
q2 : d/S > 2
q3 : d/S > 1+sqrt(3) = 2,732...
q4 : d/S > 1+sqrt(2+sqrt(3)) = 2,932...
```

Dans le modèle explicatif de deux rayons égaux `r`, une séparation sphérique
`d-2r>=s*r` donne `d/S>=(s+2)/2`. Il faut alors `s>2`,
`s>2*sqrt(3)=3,464...` et `s>2*sqrt(2+sqrt(3))=3,864...`. Il est donc normal
que `s=2` ferme presque zéro q3/q4 et que `s=4` commence seulement à mordre.

Ce calcul n'identifie pas la séparation L-infini du front à une boule
euclidienne ; le scheduler emploie les vrais `d,S` ou, mieux, `Vbest`. Il
montre pourquoi augmenter **globalement** `s` est le mauvais levier. Le bon
équivalent est d'affiner localement `A/B` jusqu'à rendre positive la lane
encore ouverte, tandis que les lanes déjà fermées cessent de payer.

## 4. Scheduler exact guidé par `Vbest`

L'audit
[`AUDIT_DIRECTIVE_JOIN_PERSISTANT_WSPD_90AA941_20260813.md`](AUDIT_DIRECTIVE_JOIN_PERSISTANT_WSPD_90AA941_20260813.md)
donne le test séparablement exact `Vbest=min_z Vhi(z)`. Il tranche le type de
travail utile :

- si la frontière de la lane échoue déjà à `Vbest`, aucune descente de `C` ne
  peut produire un singleton central `ALL` ; il faut splitter `A/B` ou
  déléguer ;
- sinon, proposer d'abord près du minimiseur entier de `Vhi` ;
- si les IDs sont insuffisants, un reporter octree batché peut élargir la
  recherche ;
- les échecs du reporter restent fail-open.

Pour le choix du split, évaluer les deux enfants possibles sans mutation et
comparer les marges des bits ouverts :

```text
M2 = Dlo-Vbest
M3 = Dlo-3*Vbest
M4 = 56*Dlo-209*Vbest
```

Choisir la meilleure amélioration par record créé, ties par `NodeKey`. Ce
lookahead ne porte aucune vérité scientifique ; il évite seulement des splits
qui ne peuvent pas rendre le cœur viable.

## 5. La fenêtre Morton reste un tier 0

Après réparation de Morton, trois détails doivent précéder toute décision :

- sélectionner les candidats par `Vhi(z)`, métrique exacte du certificat, et
  non seulement par la distance au centre des boîtes ;
- recaler la fenêtre à `n-W` en fin de tableau pour conserver exactement `W`
  lectures quand `n>=W` ;
- produire les vrais `proof_ids`, filtrés via `relation_rank[PointId]`, et les
  rejouer dans l'oracle.

Même corrigée, une fenêtre contiguë dans l'ordre de Z n'offre aucune garantie
sur les voisins euclidiens. Elle est conservée si sa fermeture supplémentaire
par octet est bonne. Sinon, la remplacer par quelques cellules d'un octree au
niveau adapté au rayon du cœur, puis un range-report saturé à douze IDs. Ce
second tier reste borné et fail-open, mais son support spatial est beaucoup
moins fragile qu'une tranche de rang fixe.

Le test utile n'est pas seulement `W=16/32/64`. Il compare :

```text
Morton corrigé
Morton volontairement brouillé
cellules octree voisines à coût égal
```

Si corrigé et brouillé ferment la même masse, la localité Morton ne fournit
aucun signal et le tier doit être supprimé.

## 6. Cible analytique du raffinement sur le volumique

Sous un processus de Poisson homogène, en ignorant le bord, les cœurs
ponctuels imbriqués donnent un résiduel coalescé attendu d'environ
`232,23*n` paires après q2/q3/q4. Cette constante ne borne pas le travail du
producteur et ne s'étend pas aux adversaires. Elle fournit néanmoins un témoin
falsifiable : sur `uniform`, un raffinement qui laisse encore une masse
déléguée proche de `n^2` n'a pas isolé les cœurs pairwise ; augmenter `W` ne
réparera pas des boîtes `A/B` trop grossières.

À `n=50000`, cette baseline vaut environ `11,61 M` paires sémantiques. Elles
doivent rester factorisées aussi longtemps que possible ; les développer avant
le carrier/census annulerait le gain.

## 7. Gates du prochain delta

Le prochain CTest non-oracle doit exercer la banque et refuser tout faux vert :

- Morton référence contre version optimisée, injectivité des bits et mutant
  2D tué ;
- `Dlo_calls=terminal_records`, `Vhi_calls=selected_ids` ;
- `Hmin_calls` et produits exacts publiés ;
- aucune boucle scalaire sur trois lanes ;
- `proof_ids` distincts, hors endpoints, replay direct par chaque lane ;
- q3/q4 ne deviennent `PRUNED_MAX_EDGE_ANCHOR` que sous owner authentifié ;
- `closed_mask | residual_mask = input_mask` et intersection vide ;
- conservation des records et masses par lane ;
- banque vide, fenêtre tronquée, collisions et sous-seuil donnent résiduel ;
- quantum et producteur de propositions différents donnent la même sortie
  finale après consommation du résiduel.

Les compteurs `eval/hwm/quantum` de l'ancienne DFS ne doivent pas apparaître
comme mesures de la banque. Les colonnes pertinentes sont lectures, tests
`Vhi`, tests `Hmin`, produits larges, octets, HWM, enfants adaptatifs et records
délégués.

## 8. Décision remise à Claude

1. Réparer et recevoir Morton48 ; ne pas interpréter encore les taux de la
   note.
2. Remplacer les trois recertifications par le masque central partagé.
3. Ajouter le tier `Hmin` q2 spécialisé ; accepter sa disjonction.
4. Conserver `s=1/2`, jamais `s=8` global.
5. Calculer `Vbest`, puis raffiner localement `A/B` avec héritage des preuves.
6. Garder la fenêtre seulement si elle bat un ordre brouillé ; sinon utiliser
   les cellules octree.
7. Mesurer le préfixe complet avant carriers, puis raccorder q3/q4 sur le
   résiduel exact.

Le contrat `50000` sous une seconde reste `NO-GO`. GCP non utilisé par cet
audit.
