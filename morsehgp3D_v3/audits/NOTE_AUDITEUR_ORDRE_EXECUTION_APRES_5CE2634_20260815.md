# Note de l'auditeur — ordre d'exécution après `5ce2634`

Date : 15 août 2026 UTC.

Pin fonctionnel courant audité :
`5ce2634cc6e1e5fa9dedc3b9736ce799802d40a5`.

Cette note ne remplace pas les audits détaillés. Elle fixe uniquement leur ordre
pratique, afin que les bonnes idées ne se transforment pas en neuf chantiers
ouverts et zéro oracle fermé, issue étonnamment commune aux projets ambitieux.

> [!IMPORTANT]
> **Ordre recommandé :**
>
> 1. fermer causalement le tape pair-level de `5ce2634` ;
> 2. prototyper `AcuteCarrierGateway` comme autorité autonome ;
> 3. graver la séparation des étages q4 ;
> 4. raccorder seulement ensuite `Q4SeedAxisTopR4` ;
> 5. mesurer en flux, jamais par catalogue global de supports.

## P0 — Fermer le commit `5ce2634`, sans nouveau mécanisme géométrique

Le cœur de la scission du cap est reçu. Il reste à rendre ses portes causales :

```text
cap=scission + --oracle=200
  -> chaque PairId exactement une fois ;

fixture D0
  -> C(5,2)-1 = 9 ancres valides par lane ;

pair_lane frontières
  -> coefficients 4/3, stricte, H=0 ;

budget
  -> evals == travail
  -> travail <= (n-2)*paires_uniques ;

max_rectangles / max_vivant_visites
  -> refus avant allocation ou scan hors enveloppe.
```

Ajouter les deux mutants de couverture de scission et le mutant
`vivant-inclut-D0`. Tant que ces portes ne sont pas vertes, les compteurs
`Vq_pair_walive` restent des diagnostics utiles mais pas une autorité de tape.

## P1 — Prototype autonome `AcuteCarrierGateway`

Ne pas le brancher immédiatement dans le gros probe. Construire un microbinaire
sur trois AABB :

```text
Phi exact : 24 produits pour max, 12 pour min ;
Delta_E / Delta_X exacts par distances carrées intervalle-point ;
verdict : NONE_ACUTE_OWNER / ALL_STRICT_OWNER / MIXED ;
arithmétique : int64 sous profil u16.
```

Portes :

- oracle exhaustif sur petites boîtes entières ;
- permutations d'axes, échange `A/B`, translations ;
- frontières `Phi=0`, ties de longueur ;
- mutants `oublie-coin`, `coins-seuls-pour-min`, `oublie-DeltaE/X`.

Critère d'adoption : zéro faux `NONE`, zéro faux `ALL`, mutants tués. Le gain de
performance vient après. C'est moins spectaculaire qu'un histogramme, mais les
histogrammes ont rarement empêché un faux prune.

## P2 — Séparer les étages q4 avant leur optimisation

Publier avec unités distinctes :

```text
V4_pair_walive,
C4_carrier,
R4_bundle,
T4_site,
M4_apex,
W4_positive,
H4_rank,
N4_event,
pending,
bytes/HWM.
```

Quatre fixtures permanentes :

1. `two_lines` : `V4_pair_walive=Theta(n²)`, tous les étages aval nuls ;
2. tétraèdre régulier entier : un vrai support q4 exact ;
3. tétraèdre `one_acute_incident_face` : un seul `Q4Seed3`, donc aucune division
   automatique du nombre d'incidences par deux ;
4. cube `{0,2}³` : un groupe de root peut transporter plusieurs `PointId`.

Borne reçue :

```text
R4_bundle <= 2*r4*C4_carrier,
N4_event <= R4_bundle.
```

Ne pas écrire `N4_event <= r4*C4_carrier` : la fixture à une seule face aiguë la
réfute.

## P3 — Raccorder `Q4SeedAxisTopR4`

Seulement après P1/P2 :

```text
PairBlock4
  -> AcuteCarrierGateway
  -> Q4Seed3 exact-once
  -> top-(r4-p) First/Last par BVH
  -> groupes égaux complets
  -> owner6 + positivité + primary
  -> census/rang
  -> BallKey/RLE/fold.
```

Exigences :

- `OVERFLOW` n'est jamais agrégé à `DEAD_*` ;
- les égalités sont range-reportées, jamais tronquées ;
- publier nœuds BVH visités et feuilles touchées ;
- ne revendiquer aucun `O(k log n)` avant mesure ;
- aucun tableau de taille `V4_pair_walive` ou `C4_carrier`.

## P4 — Modèles quantitatifs à utiliser comme oracles, pas comme contrats de sûreté

Sous Poisson homogène :

```text
q4 W-vivantes / point
  ~ 34,624 sur une surface,
  ~ 139,070 dans un volume ;

carriers aigus / point
  ~ 190,170 sur une surface,
  ~ 4 079,607 dans un volume ;

vrais supports q4 positifs, profondeur < 8, en volume
  ~ 45*pi²/2 = 222,066 par point.
```

La dernière constante prédit environ `6,66 milliards` de supports à `30 M` de
points dans un Poisson volumique. Elle impose le streaming et le fold par vague,
même si l'espérance est linéaire.

Ces constantes servent à détecter une perte ou une duplication de masse. Elles
ne remplacent ni les fixtures déterministes, ni `pending=0`, ni l'exact-once.

## Décision immédiate

Le prochain commit de code le plus rentable n'est pas encore le BVH q4. C'est :

```text
A. fermer les cinq portes P0 ;
B. ajouter le microprototype AcuteCarrierGateway exact ;
C. démontrer sur two_lines qu'il élimine la masse croisée sans PairId ;
D. démontrer sur les deux tétraèdres positifs qu'il ne perd rien.
```

Lorsque ces quatre points tiennent, la route vers `Q4SeedAxisTopR4` devient
courte, testable et mathématiquement séparée du broad phase. Avant cela, la
brancher directement ferait surtout circuler plus vite des unités dont le nom
vient encore de changer.
