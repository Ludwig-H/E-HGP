# Note de Claude — état mesuré de la source par cellules de centres

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note rapporte ce que j'ai mesuré, ce que j'ai réparé et ce que je n'ai
pas fait. Elle ne revendique aucun GO. Le verdict live reste
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) et l'autorité normative est
[`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md). La
[`NOTE_SOLUTION_SOURCE_CELLULES_CENTRES_20260812.md`](NOTE_SOLUTION_SOURCE_CELLULES_CENTRES_20260812.md)
décrit le prototype et ses invariants conditionnels.

## 1. Réorientation assumée

Je suspends comme sources générales le parcours d'arrangement `order_k_flats`
et la cascade duale q2 profonde Yao48/LBVH. Ils restent des oracles et
falsificateurs. Cette décision expérimentale ne supprime pas le transcript
Yao-1 exact ni l'EMST sparse de la route `k=1`. Pour q2 profond, q3 et q4, la
voie examinée est la subdivision de l'espace des **centres**, parce qu'elle
possède un théorème de complétude local conditionnel et qu'elle est transposable
en `count/scan/fill`; son admission produit reste ouverte.

## 2. Ce que la machine fait maintenant

Le contrat visé est d'énumérer les supports minimaux positifs d'arité deux,
trois et quatre vérifiant `p+q<=smax` et de publier leur census global exact
`I_B`/`U_B`, avec la classe `extra_shell` séparée. Le snapshot pincé s'accorde
sur les **identités** avec un juge exhaustif borné sur cinq nuages dont une
grille gravée. Ce juge partage encore les lifts et puissances de
`ball_front.hpp`; cet accord ne reçoit ni la complétude générale, ni
l'indépendance arithmétique, ni le source live postérieur.

Réparations faites depuis l'audit du 12 août :

- porte `--fixtures` réparée; le parseur accepte `--fixtures` et
  `--fixtures=<nom>`, et refuse les suffixes numériques;
- tampon de coquille `[24]` et son `exit(3)` supprimés; la contre-fixture de
  coquille de l'audit publie `shell_high_water=30`;
- groupement par clé géométrique de centre **avant** le census, avec séparation
  des rayons par test de puissance exact;
- census stratifié par budget avec promotion et scan des seuls nouveaux
  buckets; garde d'égalité de fermeture `r==h`;
- lanes d'arité génératives indépendantes, avec les deux contre-fixtures de
  l'audit gravées et vérifiées;
- surclaim `O(nombre de cliques)` retiré.

## 3. Mesures

Machine partagée à deux cœurs, fortement chargée : seuls les temps `user` sont
cités, et aucun n'est un benchmark. Les compteurs, eux, sont déterministes.

### Effet du filtre d'enveloppe du support

`terrain`, `n=2000`, `smax=11`, `pair_cap=256`, sortie identique
`supports_total=134 300` :

| version | lifts | `user` |
| --- | ---: | ---: |
| sans | `31 246 503` | `6,95 s` |
| avec | `12 156 467` | `4,85 s` |

### Effet du filtre droite--cellule de la lane q4

Même configuration, sortie identique :

| version | quadruplets énumérés | lifts |
| --- | ---: | ---: |
| sans | `10 704 370` | `12 156 467` |
| avec | `7 747 706` | `10 689 083` |

`axis_pruned=3 501 372` sur `10 525 157` tests, `axis_unusable=0`. Le filtre est
exact et fail-open, mais sur CPU son coût annule son gain : `user` passe de
`4,85 s` à `6,7 s`. Il est conservé parce qu'il déplace du travail vers un test
sans division, plus favorable à un portage device, mais **il n'est pas reçu
comme une accélération**.

### Rampe contractuelle, premier point

`terrain`, `n=12 500`, `smax=11`, `pair_cap=256`, binaire avec filtre
d'enveloppe :

```text
cells_created=8 338 753  terminal=5 477 375  pruned=1 819 034  depth_max=11
clique_pairs=13 137 824  clique_triples=224 126 048  clique_quads=137 856 613
lifts_built=104 352 433
ball_groups=2 437 049  census_point_tests=29 049 994  census_promotions=852 885
supports_q2=252 764  supports_q3=587 098  supports_q4=66 216
supports_total=906 078  supports_per_point=72,4862
```

Entre `n=2 000` et `n=12 500`, les supports croissent d'un facteur `6,75` pour
un facteur de taille `6,25` : la sortie est quasi linéaire. Les lifts croissent
d'un facteur `8,58`, soit un exposant `1,17`. Les points `25 000` et `50 000`
ne sont pas terminés à cette heure et ne sont pas publiés.

## 4. Verdict que je tire moi-même

L'ordonnance mesurée **ne tient pas** la seconde à 50 000 points. Sur un cœur de
cette machine, l'extrapolation du premier point donne de l'ordre de deux cents
secondes pour `terrain`. Même en supposant deux ordres de grandeur de
parallélisme sur une G4, le budget serait consommé par la seule Source S, sans
`k=1`, sans gateways, sans resolver, sans MSF, sans fold, sans verticales et
sans payload officiel.

Je ne demande donc **pas** de session G4 pour un benchmark de latence. Le
`NO-GO` de l'audit est repris tel quel.

Le rapport travail/sortie est le vrai verrou : environ cent-quinze lifts par
support à `n=12 500`. Il vient de ce que les filtres de bissecteur et
d'intervalle ne deviennent nets que lorsque la cellule est petite devant la
configuration, tandis que le nombre de cellules croît quand elle rétrécit.

## 5. Ce que je propose de faire ensuite, dans cet ordre

1. **Coordonnées locales et arithmétique 64 bits.** Dans une cellule terminale,
   le rayon local mesuré vaut quelques dizaines d'unités u16, jamais `65 535`.
   Tous les prédicats de la génération terminale tiennent alors en `i64` au lieu
   de `i128`, avec un garde de magnitude et un repli `i128`. C'est le seul levier
   qui vaut un facteur trois à quatre sans changer un seul verdict, et il est
   indispensable au portage device, où l'entier 64 bits est déjà lent.
2. ~~Critère de split exact~~ — **fait**. Le comptage de paires compatibles est
   remplacé par le potentiel exact `sum_i C(a_i,q-1)` calculé par un seul sweep,
   pondéré `E+3T+6Q`, et l'option est renommée `--work-cap`. Le fait utilisé est
   que `u_j<l_i` entraîne `j<i` quand `l` est trié, donc le compte global des
   `u` inférieurs est exactement le compte des antérieurs. À `n=1 500` sur
   `terrain`, `--work-cap=20000` rend `208 705` cellules et `7 015 571` lifts,
   contre `86 361` cellules et `9 181 507` lifts à `100000` : l'optimum est
   plat et le critère distingue enfin une liste ordonnée d'une liste ambiguë.
   Les vingt-quatre portes restent vertes.
3. **Clé chaude à cinq coefficients primitifs** issue de la forme liftée, à la
   place du centre réduit et du test de puissance.
4. **Préflight d'octets** choisissant bitset, CSR sparse ou
   `resource_exhausted` avant toute allocation.
5. **Juge arithmétiquement indépendant** : étendre
   `oracle/locality_census_judge.cpp` aux identités `(support, I_B, U_B)` de
   cette source, sans partager `ball_front.hpp`.
6. Seulement ensuite : rampe complète sur quatre familles plus le mélange
   d'amas manquant, puis décision de portage.

## 6. Questions ouvertes pour l'auditeur

1. Le filtre droite--cellule est conservé bien qu'il ralentisse le CPU.
   Acceptez-vous ce choix comme préparation device, ou exigez-vous qu'un filtre
   non rentable soit retiré du chemin par défaut et gardé derrière une option ?
2. La baseline Poisson--Delaunay donne environ `480` supports par point en
   régime volumique uniforme, contre `72` mesurés sur `terrain`. Le contrat
   `warm_e2e<1 s` porte sur « une famille volumique favorable dont le certificat
   reste sparse ». Existe-t-il, dans votre lecture de la spécification, un
   régime volumique dont la Source S soit `O(n)` avec une petite constante, ou
   faut-il admettre que seul le régime surfacique peut viser la seconde ?
3. Le rapport travail/sortie de cent-quinze lifts par support est-il réductible
   dans ce cadre, ou considérez-vous qu'il faut un producteur terminal d'une
   autre nature — diagramme de Voronoï local d'ordre `<=k` sur la liste — pour
   descendre à un facteur constant petit ?

GCP non utilisé.
