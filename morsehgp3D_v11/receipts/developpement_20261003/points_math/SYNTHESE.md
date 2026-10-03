# Synthèse du juge final : de la tour FULL à une hiérarchie laminaire de points, à k fixé

3 octobre 2026, rédigée de 22 h 37 à 23 h 05 UTC (heures lues par `date -u`). Label `synthese` du workflow
« meilleure méthode mathématique ».

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (oracle de la définition hgp11_ref, Python exact ; archives G4 du développeur lues seulement)
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé. Aucune commande git. Aucune construction ni test natif.
Écritures : build/v11-points-math/SYNTHESE.md et build/v11-points-math/synthese/ seulement.
```

**Ce qui est jugé.** Les quatre propositions du workflow (`marge_cibles`, `majorite_vote`, `fermeture`, `axiomes`)
et leurs quatre vérifications adverses ; `docs/HIERARCHIE_POINTS.md` dans sa version de 22 h 01, qui retient
désormais H^r_{k+1} ; `docs/MATHEMATIQUES.md` § 7 ; le reçu `qualified_proof` et l'audit de l'auditeur ; le verdict
et les mémos v10 (ancrage, axiomes, synthèse) ; les réponses de l'utilisateur ; les extraits de la thèse (ch. 5, 6, 7
et 9) ; la lentille L03 de l'audit v10 (`build/v11-persist/audit_v10/L03_MATH_POINTS.md`).

**Règle de lecture.** Je ne retiens que ce que les vérificateurs ont confirmé, ce que j'ai rejoué ici (§ 10), ce
qui est marqué conjecture, ou ce qui est cité d'un reçu antérieur (v10, L03), signalé comme tel. Les petits nuages
(n ≤ 9, 14 sites pour Q3) sont des oracles de correction : ils n'établissent ni fréquence ni pente.

**Notations.** r est un rayon, r² le niveau publié ; coupes fermées. α_k(x) est le rayon de première couverture à
l'ordre k, d_k(x) le rayon cœur (k-ième voisin, x compris). R_x est la région de couverture de x dans FULL_k ;
Π_1(R_x) = R_x ; Π_{k+1}(R_x) garde ses points dont l'amas discret compte au moins k + 1 sites. P_κ∘Π est
l'ancrage persistant de la v10 sur le profil Π : date e = t + sup_q (m(p, q) − κ(h(q) − t)) **en rayon**,
propriétaire = ancêtre vivant à e du point le plus bas p. Q_κ est la même formule en niveau carré.
H^r_{k+1} du développeur = P_1∘Π_{k+1} ; H_1 et H_{k+1} de sa première version = Q_1∘Π_1 et Q_1∘Π_{k+1}.

## 1. Recommandation

> **À k fixé, retenir H^r_{k+1} = P_1∘Π_{k+1} (l'ancrage persistant en rayon, κ = 1, restreint aux composantes de
> FULL_k qui couvrent au moins k + 1 sites), puis condenser à mcs et sélectionner sur cette hiérarchie, le vote de la
> thèse ne servant qu'à compléter, après sélection, les points restés seuls.**

Pourquoi elle : la famille P_κ∘Π_{k+1} est la seule connue qui soit à la fois fidèle à FULL, laminaire,
équivariante, indépendante de mcs, prouvablement stable en rayon et conforme aux deux triangles de la thèse comme à
Q1/Q1bis. κ = 1 en est le membre canonique (3ε en dates et en hauteurs, constante minimale). Ce qu'elle coûte, à
publier avec elle : Q2, Q3 et Q4 perdus pour tout seuil d'effectif, respect du cœur perdu à K = 2, aucune entrée
avant α_{k+1}. Ce n'est pas une solution du verrou : aucune règle connue ne réunit toutes les réponses de
l'utilisateur et une stabilité uniforme (§ 2).

## 2. Pourquoi : un arbitrage démontré, pas un réglage

### 2.1 Ce que les théorèmes ferment

Tous ces énoncés sont confirmés par les vérificateurs, avec les réserves indiquées.

| Résultat | Énoncé | Source |
| --- | --- | --- |
| Trilemme | fidélité, laminarité et entrée immédiate à la première couverture imposent une discontinuité ({0, 2, 4}) | v10 axiomes, th. B ; MATHEMATIQUES P4–P5 |
| Anticipation (B) | toute règle fidèle, équivariante, A5_glob et continue lit des événements postérieurs à sa date | `axiomes`, th. B |
| Front exact (C) | pour les règles locales au profil : constante des dates ≥ 3 ; P_κ atteint 1 + 2κ et, sous monotonie, est la plus précoce à cette constante | `axiomes`, th. C, **cadre intrinsèque seulement** (profils abstraits) |
| Prix de la précocité (T6) | toute règle κ-précoce a une constante ≥ κ/2 (en rayon) | v10, mémo d'ancrage § 6 |
| T0 contre monotonie (E) | une règle locale au profil **brut**, équivariante, fidèle et monotone n'entre C des deux triangles qu'à la racine | `axiomes`, th. E |
| Seuil d'effectif (F) | aucune règle « lignée de première couverture qualifiée à m = f(K, mcs) » ne passe Q1bis et Q2 ; m = k + 1 perd Q2, Q3 et Q4 | `axiomes`, th. F ; `marge_cibles` § 4 ; `majorite_vote`, prop. O |
| Pente d'ER0h (S) | sur trois sites, \|Δu\|/δ ≥ (64κ/129)ρ² − 1, donc aucune constante uniforme | `majorite_vote`, prop. S |

Lecture. Parmi les règles stables connues (P_κ∘Π_m), le profil brut perd T0 et Q1, et le profil qualifié perd Q2,
Q3 et Q4. Les seules règles connues qui passent tout (ER0h, ER0hr) n'ont aucune constante uniforme connue, et pour
ER0h cette absence est prouvée. **Aucune règle connue ne réunit les réponses ancrées de l'utilisateur et une
stabilité uniforme.** C'est le verrou mathématique ; il reste ouvert (§ 8).

### 2.2 Les priorités de l'utilisateur

- **Réponses les plus fermes.** La cible des deux triangles de la thèse (§ 6.1), fixée le 30 septembre, puis Q1 et
  Q1bis : pont 15 % plus court, ABC | DEF à mcs 3 comme à mcs 2. Les réponses du 30 septembre sont consignées dans
  `CIBLES_REVISEES.md` ; celles du 1er octobre dans `REPONSES_UTILISATEUR_20261001.md`.
- **Consignes de méthode.** « Privilégie l'aspect mathématique » et « Q2 ou Q3 ne sont que de peu d'importance par
  rapport au modèle mathématique » (en-tête de `HIERARCHIE_POINTS.md`) ; « il n'y a pas d'oracle ».
- **« HDBSCAN ne peut pas battre la tour ».** La projection doit garder ce que FULL voit et que HDBSCAN ne voit pas.
  Or la différence n'est pas métrique :
  - les couvertures FULL sont 2-entrelacées avec la liaison simple de l'atteignabilité mutuelle (`verif_fermeture`,
    41 499 contrôles) ;
  - au meilleur bloc, MR₂-bord, sans tour, égale `cover` : −0,011 [−0,025 ; +0,005] à K = 5 sur 16 scènes de
    8 000 points (L03, constat 02).

  La différence est structurelle, et T0 en est l'exemple : FULL_2 sépare ABC | CD | DEF, HDBSCAN réunit les six
  points d'un coup. Une projection qui perd T0 (P_κ∘Π_1 n'y montre qu'AB | EF) laisse de côté la structure que seule
  la tour voit.

### 2.3 Conséquence

Exigences dures : stabilité prouvée, T0 et Q1.

- Par le théorème E, le profil brut est exclu.
- Par la proposition S, ER0h l'est aussi ; ER0hr n'a aucune constante prouvée (pire rapport aléatoire 69,5).
- Reste la famille P_κ∘Π_{k+1}. Dans cette famille, κ = 1 est le seul choix sans paramètre : c'est l'unique
  constante minimale (3), et le membre le plus précoce à cette constante parmi les règles monotones du profil
  qualifié.

D'où H^r_{k+1}, qui est aussi le choix actuel du développeur. Plusieurs de ses justifications sont toutefois à
corriger (§ 9).

La décision ne change que si les priorités changent :

- **Q2, Q3 et Q4 redeviennent dures, T0 et Q1 négociables** → P_2∘Π_1. C'est la même famille sur le profil brut,
  stable en 5ε. Les cellules strictes imposent κ ∈ [1,397 ; 3,785] (`verif_axiomes`, R2).
- **Toutes les cellules dures, stabilité uniforme négociable** → ER0h(1, 12) ou ER0hr(1, 10), avec leurs
  obligations : preuve de continuité, borne locale.
- **Une règle stable passant toutes les cellules est trouvée** (§ 8, P1) → elle remplace H^r_{k+1}.

## 3. La règle retenue : H^r_{k+1} = P_1∘Π_{k+1}

### 3.1 Définition

FULL_k est vu comme un espace : un point est p = (ν, r) avec b(ν) ≤ r < b(parent ν), en rayon. La région de
couverture est R_x = {(ν, r) : x ∈ E_ν(r)}, où E_ν(r) est l'amas discret de la définition 8 ; elle est stable par
remontée.

**Qualification.** (ν, r) est qualifié si |E_ν(r)| ≥ k + 1. De façon équivalente, la composante de Γ_k(r) a au moins
deux sommets, donc une (k+1)-partie de niveau ≤ r qui en relie deux. Une composante réduite à un (k−1)-simplexe
isolé ne reçoit pas de points.

**Date et propriétaire.** Soit t le rayon le plus bas de Π_{k+1}(R_x), p un point qualifié à ce rayon, et m(p, q) le
rayon de rencontre des remontées. On pose :

e_x = t + max(0, sup_{q ∈ Π_{k+1}(R_x)} (m(p, q) − h(q))) et o_x = ancêtre de p vivant à e_x (coupe fermée).

x est seul avant e_x, puis suit les ancêtres de o_x. La hauteur de réunion est u(x, y) = m((o_x, e_x), (o_y, e_y)),
avec u(x, x) = e_x.

### 3.2 Paramètres

| Paramètre | Valeur | Statut |
| --- | --- | --- |
| échelle de la marge | rayon | **imposée.** En niveau carré, la règle est Q_1 : elle est dominée par P_1 et n'a aucune constante uniforme en rayon (31,8 à L = 10⁴) (`axiomes`, vérifié) |
| κ | 1 | **canonique** : constante minimale 3. κ = 2 est l'alternative déclarée : 5ε, horizon d'inspection borné (élagage exact T3(e) de la v10), moins de retards. À départager par l'expérience E1, avec un critère écrit d'avance |
| m | k + 1 | **postulat de modèle**, pas une conséquence : « un (k−1)-simplexe isolé n'est pas un amas ». m ≤ k équivaut à m = 1 ; m > k + 1 n'a pas de lecture canonique ; un seuil dépendant de mcs est réfuté (prop. G) |
| mcs | absent | la projection ne lit pas mcs |

### 3.3 Ce qui est prouvé

1. **Laminaire et fidèle.** Les blocs sont inclus dans l'amas discret de leur nœud, et deux sites ne sont réunis
   qu'à la fusion FULL de leurs propriétaires. Le propriétaire ne dépend pas du choix de p (H1). Sources : v10 T1–T2,
   F1 et H1 du développeur. Contrôle : 0 bloc hors de son amas sur 13 133 pour les règles en rayon de l'oracle, dont
   H^r_{k+1}, et sur 29 692 pour ses règles exactes (§ 10).
2. **Équivariante** (renumérotation, isométries ; une homothétie multiplie les rayons), sans départage.
3. **Stable en 3ε.** |Δe| ≤ 3ε et |Δu| ≤ 3ε sous déplacement apparié ε. Les dates relèvent de T5 (v10), les hauteurs
   de la proposition D ; P5 transporte la qualification. En rayon, aucun Λ n'intervient. Mesures : 2,28ε chez le
   développeur (502 488 paires) ; 1,50 dans `verif_axiomes`.
4. **Optimalité relative, dans le cadre intrinsèque.** P_1 est la plus précoce des règles monotones, locales au
   profil qualifié, de constante 3, et 3 est la constante minimale de cette classe (th. C). En géométrie, seule la
   borne κ/2 de T6 est connue.
5. **Encadrement des entrées : α_{k+1}(x) ≤ e_x ≤ α_{k+1}(x) + d_k(x)/2.**
   - Minorant : à m = k + 1, la première couverture qualifiée à l'ordre k est la première couverture d'ordre k + 1
     (T1 de `fermeture`). Contrôle : 1 764 sites, 0 écart.
   - Majorant : par L6 (v10), toutes les composantes qui couvrent x au rayon c ≥ α sont réunies avant c + d_k/2 ;
     chaque terme m(p, q) − h(q) est donc au plus d_k/2.
   - Contrôle global : 2 418 sites, 0 violation, pire (e − α_{k+1})/d_k = 0,441.
6. **k = 1 :** liaison simple.
7. **Cibles tenues.**
   - T0 : ABC | DEF de 0,8165 à la racine 1,2247.
   - Q1 et Q1bis : ABC | DEF dès 1 154,684 (le pont CD n'est jamais qualifié).

### 3.4 Prix démontrés, à publier avec la règle

1. **Q2, Q3, Q4 et Q-Π2 sont perdus par la lignée** (th. F), et aucun réglage de date ne les répare :
   - Q2 : x rejoint {b1, b2} à 60,36 ;
   - Q3 : x rejoint l'amas à 549,09 (rayon) ;
   - Q4 : C rejoint CPQR à 866,03 ;
   - Q-Π2 : à mcs 9, l'amas et x forment un cluster.
2. **Le respect du cœur est perdu à K = 2**, alors que P_κ∘Π_1 le garde (v10, synthèse § 4.1). Sur Q3, x est un
   point cœur (d_2 = 700,006) de la composante du filament sur [700,006 ; 796,117[. Pourtant son bloc est l'amas aux
   rayons 705, 750 et 790 (`synthese/recu_coeur_q3.txt`), là où `core` et `cover` rendent {x, f1, …, f5}.
3. **Aucune structure isolée de k sites n'est un bloc**, et toute entrée est ≥ α_{k+1} : à K = 2 aucune paire
   isolée, à K = 5 aucun groupe isolé de 5 points. Sur petits nuages aléatoires (oracle de correction, pas une
   fréquence), 515 sites sur 2 418 entrent après d_k, jusqu'à 6,79 d_k (`synthese/recu_bornes_hr.json`).
4. **La proposition I ne se transpose pas telle quelle.** Fixture R1 de `verif_axiomes`, rejouée ici : 8 sites,
   K = 2, fusion parasite à F = 12, x point cœur (d_2 = 10) de la composante qui fusionne.
   - P_1∘Π_1 entre x à 9,631 ; H^r_3 seulement à 13,311.
   - Cause : un rival qualifié {w1, w2, x}, né à 12,5, rejoint la lignée de x à 15,811.
   - Énoncé correct : P_κ∘Π_{k+1} place x à F dès que α_{k+1}(x) + d_k(x)/2 ≤ F.
5. **Anticipation non bornée pour κ = 1** : la date peut dépendre d'événements lointains de la remontée. C'est un coût
   de calcul, pas une faute.
6. **La marge en rayon est précoce.** À même lignée, elle ne fait jamais entrer un site plus tard que la marge en
   niveau carré (`verif_marge_cibles`). Cette précocité a un prix : sur une entrée du catalogue (gigue près du bord
   d'une fenêtre), elle forme un cluster interdit à 1 278,498, avant la borne 1 279,204. Effet de bord, mais réel.

### 3.5 Ce qui reste conjecturé ou non mesuré

- L'existence d'une règle stable qui passe toutes les cellules (§ 8, P1).
- **La règle retenue elle-même n'a pas été mesurée à l'échelle.** Les sessions G4 du développeur (`claudepts1`,
  `claudepts2`) ont mesuré `margin` = Q_1∘Π_{k+1} et `margin1` = Q_1∘Π_1, toutes deux en niveau carré, et non
  H^r_{k+1} (§ 3.6).
- Le comportement à mcs = k (sur-segmentation) : prédiction non mesurée (§ 8, E1).
- La constante géométrique exacte, entre κ/2 et 1 + 2κ.

### 3.6 Ce que disent les mesures disponibles (archives lues, pas rejouées)

Archives G4 du développeur, résumées ici dans `synthese/g4/effets.txt` et `demos_detail.txt`. Critère : meilleur IoU
d'un bloc par groupe, c'est-à-dire présence de l'objet dans la hiérarchie (oracle optimiste). Échantillon :
128 scènes synthétiques de n = 2 000 et 8 000, 133 trames LiDAR après dédoublonnage.

- **La qualification seule pèse peu.** Synthétique, `first` − `cover` (sans marge) : +0,0031 ; +0,0036 ; +0,0026 ;
  +0,0013 à k = 2, 3, 5, 10. Les IC 95 % sont positifs, mais minuscules.
- **L'écart annoncé « H_{k+1} − H_1 = +0,011 à +0,021 » vient surtout du niveau carré**, qui pénalise davantage le
  profil brut : `margin1` − `cover` va de −0,0095 à −0,0262, contre −0,0020 à −0,0071 pour `margin` − `first`. Le
  choix entre Π_1 et Π_{k+1} ne se tranche donc pas au meilleur bloc.
- **Contre HDBSCAN**, `margin` gagne +0,010 ; +0,029 ; +0,053 ; +0,079 (k = 2, 3, 5, 10).
- **Démos Zoltan : une seule réussite franche**, le vélo C de la démo 04 (instance 56) à K = 3. HDBSCAN y reste
  ≤ 0,361 à tous les ordres testés, contre `margin` 0,559, `cover` 0,565, `first` 0,563 et `margin1` 0,544.
  - Deux cas limites : démo 01, instance 55 (HDBSCAN jusqu'à 0,500, `margin` 0,550) ; démo 04, instance 55
    (HDBSCAN jusqu'à 0,465, `margin` 0,512).
  - Ces gains sont communs à toutes les règles de la tour sauf `core`. L03 montre en outre que MR₂-bord, sans tour,
    rattrape aussi le vélo C (0,568).
  - Une perte, commune à toutes les règles de la tour : démo 02, instance 59, à K = 3 (0,443 contre 0,667).
- **Cohortes LiDAR à K = 5** (sauvetages / pertes face à HDBSCAN) : trames voisines, `cover` +29/−4, `first` +30/−4,
  `margin` +27/−5, `margin1` +20/−5 ; témoins, 0/0.

Conclusion. L'exigence « marcher sur au moins un exemple de Zoltan/ où HDBSCAN échoue » est tenue par le vélo C à
K = 3, avec une marge mince. Elle l'est par toute règle à entrée de couverture : elle ne départage pas les
projections, et elle reste à confirmer pour la version en rayon.

## 4. Tableau comparatif

### 4.1 Propriétés mathématiques

Les stabilités sont en rayon, pour un déplacement apparié ε. « Cœur K = 2 » : respect du cœur à K = 2.

| Règle | Laminaire | Fidèle | Stabilité prouvée | Équivariante | Sans mcs | Cœur K = 2 | Paramètres libres |
| --- | :-: | :-: | --- | :-: | :-: | :-: | --- |
| `core` (P1) | ✓ | ✓ | 2ε (atteinte) | ✓ | ✓ | ✓ | aucun |
| `cover` (première couverture, LCA des ex æquo) | ✓ | ✓ | ✗ discontinue (F2) | ✓ | ✓ | ✓ | aucun |
| `first` (m = k + 1, sans marge ; = EC) | ✓ | ✓ | ✗ discontinue | ✓ | ✓ | ✗ (Q3) | m |
| Q_1∘Π_1 (H_1, niveau carré) | ✓ | ✓ | ✗ aucune constante uniforme | ✓ | ✓ | — | échelle, cachée |
| P_1∘Π_1 (H^r_1 = P_1 v10) | ✓ | ✓ | 3ε | ✓ | ✓ | ✓ | κ = 1 |
| P_2∘Π_1 (P_2 v10) | ✓ | ✓ | 5ε | ✓ | ✓ | ✓ | κ = 2 |
| Q_1∘Π_{k+1} (H_{k+1}, niveau carré) | ✓ | ✓ | 3δ et 5δ en niveau, δ = 2ε√Λ + ε², Λ majorant aussi les premières couvertures ; non uniforme en rayon | ✓ | ✓ | ✗ | m, échelle |
| **P_1∘Π_{k+1} (H^r_{k+1}, retenue)** | ✓ | ✓ | **3ε, dates et hauteurs** | ✓ | ✓ | ✗ (Q3) | aucun, une fois posés κ = 1 et m = k + 1 |
| H_{max(k+1, mcs)} (retirée) | ✓ | ✓ | comme H_{k+1} | ✓ | ✗ | ✗ | m(mcs) |
| ER0h(1, 12) (verdict v10) | ✓ | ✓ | ✗ aucune constante uniforme (prop. S) ; continuité conjecturée | ✓ | ✓ | ✗ (Q1, choix de l'utilisateur) | η, κ, réglés sur les cellules |
| ER0hr(1, 10) | ✓ | ✓ | inconnue (pire 69,5 au hasard) ; continuité conjecturée | ✓ | ✓ | ✗ (Q1) | η, κ′, réglés sur les cellules |
| ER0(1, 12) | ✓ | ✓ | ✗ discontinue (fixture exacte) | ✓ | ✓ | ✗ | η, κ |
| vote de la thèse, argmax par niveau | ✗ (4 sites) | — | — | départage requis | ✓ | — | F_K, p |
| vote de la thèse, majorité figée V½ | ✓ | ✓ (prop. M) | ✗ discontinu (5 et 6 sites) | ✓ si F_K non ambigu | ✓ | — | F_K, p, κ |
| fermeture qualifiée m = k + 1 (auditeur) | ✓ | ✗ (réunit avant FULL, jusqu'à 2× en rayon) | 1ε | ✓ | ✓ | — | m |

### 4.2 Cibles de l'utilisateur (125 jugements ancrés, juge v10)

| Règle | T0 (40) | Q1 + Q1bis (20) | Q2 (5) | Q3 (25) | Q-Π2 (5) | Q4 (30) | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `core` | 0 | 5 | 0 | 25 | 5 | 10 | 45 |
| `cover` | 14 | 5 | 5 | 25 | 5 | 20 | 74 |
| `first` | 40 | 20 | 0 | 0 | 0 | 10 | 70 |
| Q_1∘Π_1 (H_1) | 0 | 10 | 0 | 0 | 5 | 20 | 35 |
| P_1∘Π_1 (H^r_1) | 0 | 5 | 0 | 25 | 5 | 20 | 55 |
| P_2∘Π_1 | 0 | 5 | 5 | 25 | 5 | 30 | 70 |
| Q_1∘Π_{k+1}, H_{max(k+1, mcs)} | 40 | 20 | 0 | 0 | 0 | 10 | 70 |
| **P_1∘Π_{k+1} (H^r_{k+1})** | 40 | 20 | 0 | 0 | 0 | 10 | 70 |
| ER0h, ER0hr (κ′ ∈ {6, 10, 16, 24}), ER0 | 40 | 20 | 5 | 25 | 5 | 30 | 125 |
| V½, Gabriel, p = 2, sans cône | 40 | 20 | 0 | 0 | 0 | 10 | 70 |
| V½, toutes faces, p = 2 et p = 0 | 0 | 10 et 5 | 0 | 0 | 5 | 10 | 25 et 20 |
| fermeture m = k + 1 | ✓ | ✓ | ✗ | ✗ | ✗ | ✗ | non jugée sur 125 |

Sources : `marge_cibles` § 2 et `majorite_vote` § 6.2, deux routes concordantes confirmées par leurs vérificateurs ;
`VERDICT_FINAL.md` v10 § 3.1. Pour la fermeture : `axiomes` (e1) et `fermeture` § 6.

**Catalogue v2** (47 entrées de n ≤ 16 et K ≤ 3, 1 484 jugements, vérifiés entrée par entrée) : `first` 1 468,
H^r_{k+1} 1 373, `cover` 1 356, P_2 1 304, H_{k+1} 1 297, H_max 1 235, H^r_1 1 155, `core` 1 098, H_1 1 075. Les cibles
du catalogue sont extrapolées, et celles à K ≥ 3 suivent l'option (b) de Q4, rejetée : ce tableau classe, il ne
qualifie pas.

## 5. La sortie plate

### 5.1 Factorisation : projection sans mcs, puis condensation (prouvée ici, contrôlée)

**Proposition.** Soit H une pendaison fidèle (propriétaire o_x, date e_x, blocs inclus dans les amas discrets de
leurs nœuds) et mcs ≥ 1. On définit H^mcs, lecture « absorption » de la proposition de l'utilisateur du 1er octobre
(propriétaire calculé sur la tour condensée, taille = sites couverts) : même lignée, date max(e_x, a_x), où a_x est
le premier rayon où la lignée de x couvre au moins mcs sites. Alors, à tout rayon, H et H^mcs ont les mêmes blocs
d'au moins mcs points.

*Preuve.* Le nombre de sites couverts croît le long d'une remontée (fait 0 de `fermeture`). Donc x est entré dans
H^mcs au rayon r si et seulement s'il est entré dans H et que son nœud à r couvre au moins mcs sites. Le bloc de H^mcs
porté par un nœud v est celui de H si v couvre au moins mcs sites, et il est vide sinon. Enfin, un bloc de H d'au
moins mcs points est inclus dans l'amas discret de son nœud (fidélité), qui couvre donc au moins mcs sites. □

Cette proposition généralise la proposition E de la v10, établie pour ER0h, à toute pendaison fidèle. Contrôles :

- règles exactes de l'oracle (`core`, `cover`, `margin1`, `first`, `margin`) : 1 500 couples (nuage, k, m, règle),
  29 692 blocs dont 0 hors de l'amas de leur nœud, 153 445 comparaisons (niveau, mcs), 0 écart
  (`synthese/recu_factorisation.json`) ;
- règles en rayon (P_1∘Π_1 et H^r_{k+1}) : 600 couples, 13 133 blocs dont 0 hors amas, 66 708 comparaisons, 0 écart
  (`synthese/recu_factorisation_rayon.json`).

Conséquences :

- « Projection sans mcs, puis condensation » réalise exactement la proposition de l'utilisateur dans sa lecture
  « absorption ». La lecture « admission stricte » (Π_mcs) est réfutée par Q1bis, Q-Π2 et la proposition G.
- Le **vote de faces** de cette proposition n'est pas repris. Le propriétaire est celui de l'ancrage, qui est stable ;
  pris comme propriétaire, le vote de faces est non laminaire par niveau ou discontinu (`majorite_vote`).

### 5.2 Condensation et existence des clusters

- **Masses entières après engagement** : un point compte pour 1. Avec les masses fractionnaires du § 9.1
  (m_τ = S_τ Σ 1/T_x), chaque triangle de T0 pèse 8/3 < 3 et n'est plus un cluster à mcs 3, contre la cible (v10,
  `VERDICT_FINAL.md` § 6).
- **Clusters = blocs d'au moins mcs points.** C'est la lecture de « on ne doit faire apparaître les clusters qu'à
  partir de min_cluster_size ».
- **Existence.** Le principe de Q-Π2 (« un point de bord ne fait pas exister un cluster ») n'est tenu par aucune
  projection fidèle dès qu'on retire le filament de la fixture : `cover`, `cover1` et ER0h avec cohortes créent alors
  {amas, x} à 453,471 (L03, constat 06). Ce n'est pas une propriété de projection mais un critère d'existence : taille
  de cœur, sites couverts, ou maturité interpolée θ (étude v10 « existence mûre », non conclue). C'est le verrou qui
  reste à l'étage condensé.

### 5.3 Sélection

Excès de masse, feuilles, ou coût sur mesure (« hacker HDBSCAN », thèse § 5.2) : hors du périmètre de cette
synthèse. u est une ultramétrique, entrées sur la diagonale, ce qui suffit aux résolutions exactes sur l'arbre
(k-moyennes contraintes, coûts géométriques).

Exigences : même sélection pour toutes les hiérarchies comparées, HDBSCAN calculé sur la même machine (l'ordre des
ex æquo de scikit-learn dépend de la machine, v10).

### 5.4 Complétion (optionnelle, après sélection)

Elle porte sur un point resté bruit mais couvert par un cluster sélectionné.

- **Défaut proposé : complétion par lignée.** On affecte x au cluster sélectionné que traverse la remontée de son
  point qualifié le plus bas p, s'il est unique.
  - Aucun paramètre, équivariante, cohérente avec la hiérarchie : aucune réunion nouvelle.
  - Discontinue, comme toute coupe plate.
  - C'est la sortie plate de `first`, restreinte aux points restés seuls.
- **Bras déclaré : le vote de la thèse**, V_x(c) = Σ S_τ/T_x sur les faces τ ∋ x d'étiquette c, restreint aux
  clusters sélectionnés.
  - Il dépend de F_K (Gabriel) et de p : sur T0_P1, la décision se retourne (rien, AB | EF, ou ABC | DEF).
  - Il demande un départage.
  - Il ne doit jamais servir de projection : l'argmax par niveau n'est pas laminaire, et la majorité figée est
    discontinue.
- **Autre tête fractionnaire possible :** les parts M(ℓ)/W d'ER0h.

Au niveau C, il reste à mesurer : complétion par lignée, vote, ou pas de complétion.

### 5.5 Chaîne complète

FULL_k exact → H^r_{k+1} (sans mcs ; ultramétrique u, entrées sur la diagonale) → condensation à mcs (masses
entières ; critère d'existence à fixer) → sélection → complétion optionnelle.

À publier à côté, sur les mêmes scènes : P_2∘Π_1, `first`, `cover`, `core`, ER0h(1, 12) quand son coût le permet, la
fermeture qualifiée (k, k + 1), MR₂-bord et HDBSCAN.

## 6. Lecture critique

### 6.1 La thèse (§ 6, 7, 9.1, et § 5)

- **§ 6 (déf. 8, th. 2, fig. 6.5).** L'objet est un recouvrement, et la thèse compte {C, D} comme un 2-polyèdre au
  même titre qu'ABC. La cible ABC | DEF n'est donc pas une conséquence de la thèse : c'est une exigence ajoutée par
  l'utilisateur le 30 septembre. H^r_{k+1} la réalise par un postulat (la propriété d'un point ne va qu'aux
  polyèdres non triviaux), ER0h par un vote ; ni l'un ni l'autre ne figure dans la thèse.
  - Le postulat de H^r_{k+1} est dans l'esprit du chapitre 6, qui veut réparer l'asymétrie entre des contraintes
    fortes sur les points et des contraintes lâches sur les liens : un simplexe isolé n'a utilisé aucune connexion
    d'ordre supérieur.
  - Il faut cependant l'écrire comme un postulat.
- **§ 6.1 contre HDBSCAN.** L'argument est juste dans la convention de niveau de la thèse. Mais la supériorité de la
  tour n'est pas métrique (2-entrelacement, MR₂-bord) : elle est structurelle. Une projection qui perd T0 perd cet
  avantage.
- **§ 7, théorème 3.**
  - Il est énoncé en sémantique de couverture : Θ^poly compte un point couvert par la composante infinie. C'est un
    plafond pour toute projection fidèle, pas une propriété d'une hiérarchie laminaire.
  - La continuité de Θ est supposée, et la preuve est une esquisse (convergence locale vers un processus de Poisson).
  - L'ordre poly ≥ core ne passe pas aux projections laminaires à K ≥ 3 (FX-A9). Pour P_κ∘Π_1, la proposition I ne le
    donne qu'avec une perte de rayon ; pour H^r_{k+1}, seulement à partir de α_{k+1} (R1).
  - La thèse ne mesure aucune projection laminaire.
- **§ 9.1.** La partition de l'unité (partager un point avant de l'engager) est la bonne intuition, et ER0h la
  réalise. Mais :
  - S_τ dépend de F_K, et Gabriel est un artefact de l'algorithme ;
  - S_τ dépend aussi de p ;
  - l'argmax par niveau n'est pas laminaire ;
  - à majorité figée, le vote est discontinu ;
  - les masses fractionnaires font perdre T0 à mcs 3 ;
  - la proposition 7 est plate et vient après la sélection.

  À garder comme tête de complétion, pas comme projection.
- **§ 5.** L'arbre comme espace de résolution est compatible avec la règle retenue : u est une ultramétrique.

### 6.2 L'auditeur (fermeture qualifiée)

- **Ses cinq garanties sont exactes.** Preuves relues, 0 écart sur plus de 250 000 contrôles, 0 violation de 1ε sur
  170 870 comparaisons.
- **Pour m ≤ k + 1, la fermeture est la liaison simple de w_{k+1}** (th. T1), avec
  w_{k+1}(i, j) = min β(F) sur les (k+1)-parties contenant i et j. Elle ne lit pas la connexité de FULL, ce que le
  chapitre 6 reproche précisément à HDBSCAN.
- **Elle réunit deux amas entiers avant FULL** : vallée, 50 contre 100 ; Q3, 700,50 contre 796,12 ; Q4, 1 078,17
  contre 1 334,17. L'avance est bornée par un facteur 2 en rayon, et ce facteur est atteint (T3). Aucune variante
  extérieure fidèle n'existe (T5).
- **Son « optimalité minmax » est relative à w** (plus grande ultramétrique sous w) : elle est formelle, ce que
  l'auditeur dit lui-même.
- **« Encadrée par HDBSCAN à un facteur 2 » ne la distingue pas de FULL**, qui l'est aussi.
- **Son succès sur les triangles tient à m = k + 1**, pas à la fermeture : H^r_{k+1} a les mêmes entrées α_{k+1} et
  rend ABC | DEF sans fusion parasite.
- **Sa place :**
  - enveloppe extérieure canonique et certificat (u_CL ≤ w ≤ u_F ≤ 4 max(e_i, e_j, w) et w ≤ 4 u_CL, vérifiables à
    l'échelle sur un échantillon de paires) ;
  - détecteur d'ambiguïté (w/u_CL ∈ [1, 4]) ;
  - témoin négatif de la tour, puisqu'elle n'a besoin que des MEB des (k+1)-parties.

### 6.3 Le verdict v10 (ER0h)

- **Le pipeline est confirmé** (projection sans mcs, puis condensation) et généralisé au § 5.1.
- **Le 125/125 est obtenu dans l'échantillon.** La région η ∈ [0,788 ; 1,126] à κ = 12 vient des mêmes cellules, et
  la bascule du pont (paire pour L ≤ 1 632, triangles pour L ≥ 1 633, pont ancré à 1 700) laisse peu de marge.
  ER0 et ER-hv(2/5), discontinues, passent aussi 125/125 : les cellules ne testent pas la stabilité.
- **L'absence de constante uniforme est prouvée** (prop. S) : 304,6 → 307 204,6 pour ρ = 5 → 160, et 9 597 en
  unités v10. La v10 écrivait « non bornées a priori » ; c'est désormais un théorème.
- **La continuité reste une conjecture** : aucun saut trouvé, ni sur 29 750 déplacements adverses (v10), ni dans
  deux recherches de 3 000 et 3 407 configurations (ce workflow).
- **La proposition U** (héritage proportionnel) force la mesure de crédits dans une classe donnée ; elle ne prouve
  pas la continuité de la pendaison.
- **Q1 (b) et Q4 (a) ont la même forme combinatoire.** Seule une intégrale de temps dans une bande de largeur fixée
  les sépare (conjecture : aucune version sans paramètre).
- **Faiblesses :** petits groupes de taille proche de K (0,26 contre 0,41 pour `cover` à K = 5) ; règle non définie à
  K = 1 ; coût élevé (41 à 299 votes par point, rationnels jusqu'à 4 790 bits) ; aucun noyau natif.

### 6.4 Le développeur (version de 22 h 01)

**Ce qui est juste :**

- le passage au rayon ;
- la citation de P_κ ;
- le retrait de m = max(k + 1, mcs) ;
- κ = 1 reconnu comme retard maximal ;
- F1, F2, H1, H3 (3ε), H6 et H7 ;
- la porte native contre oracle : 4 602 nuages, 261 335 ultramétriques, 0 désaccord, pour les règles mesurées.
  C'est l'atout d'ingénierie de la famille.

**Ce qui est à corriger (§ 9) :**

- la dernière phrase de H5, réfutée par R1 ;
- « constante minimale de toute règle continue de sa classe », vrai seulement dans le cadre intrinsèque ;
- H2 (« sans rival »), qui n'est vrai que lu de façon non locale ;
- la raison donnée pour m = k + 1, qui est un postulat contraire à Q2–Q4 et à la figure 6.5 ;
- les prix manquants : respect du cœur à K = 2, entrées ≥ α_{k+1} ;
- le § 6 : c'est la règle en niveau carré qui a été mesurée, la qualification seule pèse peu, et les sauvetages
  Zoltan sont communs à toutes les règles de la tour.

### 6.5 Les rapports de ce workflow, après vérification

- **`marge_cibles`.**
  - Solides : le théorème Q1bis/Q2 et le lemme de propagation.
  - Faux : « Λ = racine suffit » (contre-exemple cocyclique à K = 3 : 6,80 δ) et « la marge en rayon gagne partout ».
- **`majorite_vote`.**
  - Solides : propositions M, S et U, lemme 4a.
  - Réserves : ER0hr est réglée sur les mêmes cellules ; la proposition O redémontre le théorème F ; « réfute la v10 »
    exagère, le rapport précise la v10.
- **`fermeture`.**
  - Solides : T1 à T5.
  - Faux : « H_1 rend Q2–Q4 » (lecture laxiste ; en lecture stricte, H_1 est en retard sur les trois) et « FULL
    réunit A et B à 110,11 » (en fait 91,382) ; « HDBSCAN à facteur 2 » ne discrimine pas.
  - Précision : EC = `first` pour m ≥ k + 1.
- **`axiomes`.**
  - Solides : théorèmes B, C, E et F, propositions D, G, H et I.
  - Faux : « la qualification est sans objet au sens du chapitre 7 » (R1) et « la famille P_κ échoue exactement T0 et
    Q1 » (vrai seulement pour κ ∈ [1,397 ; 3,785]).

## 7. Preuves, références et fixtures à garder

| Énoncé | Statut | Référence |
| --- | --- | --- |
| K-polyèdres = amas discrets (th. 2) | prouvé | thèse, ch. 6 |
| P5 : entrelacement de FULL en rayon, transport de la couverture | prouvé | MATHEMATIQUES § 7 |
| Toute pendaison couvrante est laminaire et fidèle ; médianes sur l'arbre | prouvé | v10 ancrage T1–T2 ; F1 ; `majorite_vote`, prop. M |
| Trilemme {0, 2, 4} | prouvé | v10 axiomes, th. B ; P4–P5 |
| P_κ : forme code-barres, retard ≤ d_k/2, dates en (1+2κ)ε | prouvé | v10 ancrage : prop. 3.1, T3, T5 |
| Hauteurs en (1+2κ)ε | prouvé | v10 synthèse § 5.2 ; `axiomes`, prop. D |
| Précocité κ ⇒ constante ≥ κ/2 | prouvé | v10 ancrage, T6 |
| Théorèmes B, C (cadre intrinsèque), E, F ; propositions G, H, I | prouvé | `axiomes` ; `verif_axiomes` |
| P_1 ≤ Q_1 ; Q_1 sans constante uniforme | prouvé | `axiomes` § 5 |
| Impossibilité Q1bis/Q2 ; propagation par la stabilité | prouvé | `marge_cibles` § 4 |
| Fermeture : cinq garanties, T1–T5 ; Θ^CL ≥ Θ^poly et λ_c^CL ≤ λ_c^poly ≤ 2^p λ_c^CL | prouvé | `qualified_proof` ; `fermeture` ; `verif_fermeture` |
| Lemme 4a, corollaire D, propositions S, U (cadre abstrait), O | prouvé | `majorite_vote` |
| ER0h : lemme M, lemme d'encadrement, proposition E | prouvé | `VERDICT_FINAL.md` § 2.2–2.6 |
| Factorisation pour toute pendaison fidèle | prouvé ici, contrôlé | § 5.1 |
| α_{k+1} ≤ e ≤ α_{k+1} + d_k/2 pour H^r_{k+1} | prouvé ici (T1, L6), contrôlé | § 3.3 |
| ER0h, ER0hr continues ; ER0hr à constante locale C·N_x | conjecture | `majorite_vote` |
| Q1 (b) et Q4 (a) séparables seulement par une bande de largeur fixée | conjecture | `marge_cibles` |
| C1–C2 (percolation de la fermeture) | conjecture | `fermeture` § 4.5 |

**Énoncés réfutés, à inscrire au registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` :**

- « Λ = niveau de la racine » (H3 en niveau carré) ;
- « la qualification est sans objet au chapitre 7 » (H5) ;
- « P_κ échoue exactement T0 et Q1 » ;
- « la marge en rayon gagne partout » ;
- « H_1 rend Q2–Q4 » ;
- « la fraction récupérable ne peut que baisser » (fermeture, à n fini) ;
- « ER0 continue » ;
- « m = max(k + 1, mcs) ».

**Fixtures minimales permanentes à graver** (règle du dépôt pour toute contradiction ; je n'ai rien modifié) :

| Fixture | Ce qu'elle établit |
| --- | --- |
| R1 : x(0,0,0), y(10,0,0), s2(20,0,0), s3(30,0,0), b1(44,0,0), b2(54,0,0), w1(−20,10,0), w2(−20,−10,0), K = 2 | contredit la dernière phrase de H5 |
| Q3 (filament contre amas) | perte du respect du cœur par Π_{k+1} |
| EQUILATERAL | théorème E |
| Q1bis et Q2 | théorème F |
| rival lointain {−2L, 0, 2} et échelle mélangée {(0,0,0), (−2,0,0), (2s,0,0), (2s+2,0,0), (2s+1,2,0)} | contre le niveau carré |
| cocyclique x(0,221,0), s(0,−221,0), p(−21,−220,0), q(21,−220,0), K = 3 | contre « Λ = racine » |
| {0, 10, 20, 30}, K = 3 | borne 1,5 d_k atteinte |
| famille isocèle de la proposition S | pente non bornée d'ER0h |
| témoins de 4, 5 et 6 sites | contre le vote comme projection |
| amas scindé | non-monotonie de la fraction à n fini |

## 8. Points ouverts et expériences décisives

### 8.1 Points ouverts mathématiques

- **P1.** Existe-t-il une règle fidèle, équivariante, A5_glob et uniformément lipschitzienne qui passe T0, Q1, Q2, Q3
  et Q4 ?
  - Conditions nécessaires : non monotone (E), non réductible à un seuil d'effectif (F), anticipante (B).
  - Pistes : ER0hr (prouver la continuité et la borne en N_x, ou trouver un contre-exemple) ; score « −naissance + λ
    × persistance interne », qui sépare les cinq cellules pour λ ∈ ]0,969 ; 1,813[, borne fragile car elle dépend de
    triplets mixtes.
- **P2.** Continuité d'ER0h et d'ER0hr.
- **P3.** Constante géométrique exacte (entre κ/2 et 1 + 2κ) ; théorème C sans monotonie (sur T0, borne 1,0606
  contre 1,2247).
- **P4.** Critère d'existence des clusters à l'étage condensé (Q-Π2).
- **P5.** Chapitre 7 projeté : Θ^proj contre Θ^core ; C1–C2.
- **P6.** Q1 (b) et Q4 (a) restent inconciliables sans paramètre de bande. À signaler à l'utilisateur comme une
  information, sans nouvelle question synthétique (consigne du 1er octobre) ; ce sont les mesures LiDAR qui
  trancheront.

### 8.2 Expériences décisives

Sessions G4 gardées seulement ; tailles 8 000, 16 000 et 32 000 ; juges d'échantillon, jamais de juge exhaustif ni de
tableau par paire.

- **E1, la décisive.** Comparaison appariée sur les mêmes scènes : synthétique, plus les cohortes LiDAR (démos,
  échecs du criblage, voisines, témoins).
  - Règles : H^r_{k+1} (κ = 1 et 2), P_2∘Π_1, `first`, `cover`, `core`, fermeture (k, k + 1), MR₂-bord, HDBSCAN sur la
    même machine.
  - Niveaux : B (meilleur bloc) et C (condensation à mcs ∈ {k, 10, 20, √n}, même sélection).
  - Mesures : IoU, précision et rappel, nombre de clusters par trame à mcs = k.
  - Prédictions écrites d'avance :
    - au niveau B, H^r_{k+1} reste à moins de 0,01 de `first` à chaque k ;
    - au niveau C, à mcs = k sur LiDAR, H^r_{k+1} réduit la sur-segmentation de `cover`. C'est là que la
      qualification peut battre HDBSCAN (v10 : `cover` 0,450 avec 5 732 clusters par trame, HDBSCAN 0,580 avec 867) ;
    - le vélo C de la démo 04 reste au-dessus de 0,5 à K = 3.
- **E2, stabilité à l'échelle.** Requantification et gigue appariées des trames LiDAR, juge d'échantillon sur les
  entrées et 10⁵ paires.
  - Invariant global : |Δe| et |Δu| ≤ 3ε.
  - Mutants : la marge en niveau carré, sur un rival lointain inséré ; `first`, discontinue.
- **E3, prix du postulat.** Distribution de e/α_k, part des entrées après d_k, devenir des groupes isolés de k sites.
  Mesure R9 (groupes de k à 2k points voisins d'un grand amas), où ER0h perd 0,26 contre 0,41.
- **E4, le chapitre 7 en direct.** Modèle à deux densités du théorème 3 : fraction de A dans le bloc de A juste avant
  la première fusion parasite.
  - Règles : H^r_{k+1}, P_κ∘Π_1, `core`, `cover`, fermeture.
  - Ce qu'elle teste : la proposition I et sa forme corrigée, et C1–C2.
- **E5, Zoltan.**
  - Les sept objets suivis où HDBSCAN échoue (01 B ; 02 A et B ; 03 A ; 04 A, B et C) gardent un plafond de FULL
    ≤ 0,5 à K = 5 et 10 (L03) : aucune projection ne peut les rattraper à ces ordres.
  - Le levier est l'ordre (K = 3 et 4) et l'étage d'existence et de sélection. À publier : le meilleur IoU par objet
    à K = 2 à 5 pour H^r_{k+1}, avec MR₂-bord à côté.
  - Le test LiDAR de la projection elle-même est E1 au niveau C, à mcs = K.

## 9. Corrections à apporter à `docs/HIERARCHIE_POINTS.md` (version de 22 h 01)

1. **H5, dernière phrase (réfutée).** Remplacer « La qualification ne retarde au-delà que des sites dont toutes les
   composantes couvrantes ont au plus k sites, hors de la composante géante » par :
   - la garantie corrigée : P_κ∘Π_{k+1} place x à F dès que α_{k+1}(x) + d_k(x)/2 ≤ F ;
   - le contre-exemple R1.

   Graver R1 et l'inscrire au registre des statuts avant toute suite.
2. **Décision et H4.** Écrire : « dans le cadre intrinsèque (entrelacements de profils abstraits), 3 est la constante
   minimale des règles locales au profil qualifié, équivariantes et A5_glob ; P_1 est la plus précoce de constante 3
   parmi celles qui sont monotones (th. C). En géométrie, seule la borne κ/2 de T6 est connue. »
3. **Le seuil m (§ 3).** Présenter m = k + 1 comme un postulat de modèle et l'énoncer exactement : « une composante
   reçoit des points si et seulement si sa composante de Γ_k a au moins deux sommets ». Ajouter :
   - l'identité e_x ≥ α_{k+1}(x) (première couverture qualifiée = première couverture d'ordre k + 1) ;
   - la borne e_x ≤ α_{k+1}(x) + d_k(x)/2 ;
   - l'écart à la thèse : la figure 6.5 compte {C, D} comme 2-polyèdre.
4. **Prix.**
   - « au plus d_k/2 après sa première couverture » → « après sa première couverture qualifiée α_{k+1} ».
   - Ajouter : respect du cœur perdu à K = 2 (Q3) ; entrées après d_k possibles sans borne (groupes isolés de k
     sites).
   - « cibles … du catalogue v10 » → « cellules ancrées de l'utilisateur ».
5. **H2.** « sans rival » → « sans rival qualifié à aucun niveau avant la rencontre des lignées ». L'énoncé est non
   local (`marge_cibles` § 6, x1 de `k5_spheres`).
6. **Provenance.** La borne (1+2κ)ε des hauteurs figure déjà dans la synthèse v10 (§ 5.2, preuve du vérificateur) :
   ce n'est pas un apport v11.
7. **§ 5, thèse.** Ajouter :
   - masses fractionnaires incompatibles avec T0 à mcs 3 ;
   - décisions du vote retournées par F_K et p sur T0_P1 ;
   - théorème 3 en sémantique de couverture, avec continuité supposée.
8. **§ 5, auditeur.** Retirer « encadrée par HDBSCAN à un facteur 2 près » comme argument, puisque FULL l'est aussi.
   S'appuyer sur T1 et sur les réunions précoces mesurées. Noter qu'à m = k + 1 la fermeture et H^r_{k+1} ont les
   mêmes entrées.
9. **§ 5, verdict v10.** Remplacer « pentes locales jusqu'à 103 » par la proposition S (pente non bornée, prouvée).
   Ajouter qu'ER0 et ER-hv(2/5), discontinues, passent aussi 125/125, et que la région (η, κ) vient des mêmes cellules.
10. **§ 6 (`MESURES_A_COMPLETER`).**
    - Dire que les sessions `claudepts1` et `claudepts2` ont mesuré `margin` = Q_1∘Π_{k+1}, et non `margin_r`.
    - Donner la décomposition : qualification seule +0,001 à +0,004 ; pénalité du niveau carré −0,010 à −0,026.
    - Dire que le vélo C de la démo 04 est rattrapé par toutes les règles de la tour sauf `core`, et par MR₂-bord.
    - Ajouter les témoins MR₂-bord et fermeture (k, k + 1).
    - Mesurer `margin_r` et P_2∘Π_1 aux tailles d'intérêt avant toute phrase de supériorité.
11. **Sortie plate.**
    - Ajouter la factorisation du § 5.1.
    - Le vote de la thèse : seulement après sélection, sur les points restés bruit, comme bras déclaré ; complétion
      par lignée par défaut.
    - L'existence des clusters (Q-Π2) relève de la condensation.
12. **κ.** Écrire que κ = 1 est le membre le plus tardif du front et que κ = 2 est l'alternative déclarée, à
    départager par E1.
13. **Code.** `reference_radius_rules` calcule désormais dans un contexte local de 120 chiffres (version de 22 h 47,
    relue). Il faut rejouer la recoupe `e8` des propriétaires aux égalités exactes, qui avaient touché les six sites
    de T0.

## 10. Reproduction et limites

Depuis `build/v11-points-math/synthese/`, avec `PYTHONDONTWRITEBYTECODE=1 python3 -B` (aucun bytecode écrit dans le
worktree). Oracle de la définition, importé en lecture seule. CPU total d'environ 25 s.

| Commande | Reçu | Résultat |
| --- | --- | --- |
| `factorisation_check.py 20261003 150 150` | `recu_factorisation.json` | 150 nuages, 1 500 couples, 29 692 blocs, 0 hors amas ; 153 445 comparaisons, 0 écart ; première couverture qualifiée = première couverture d'ordre k + 1 sur 1 764 sites |
| `factorisation_rayon.py 20261005 150 150` | `recu_factorisation_rayon.json` | règles en rayon P_1∘Π_1 et H^r_{k+1} : 600 couples, 13 133 blocs, 0 hors amas ; 66 708 comparaisons, 0 écart |
| `bornes_hr.py 20261004 200 120` | `recu_bornes_hr.json` | 2 418 sites, 0 violation de α_{k+1} ≤ e ≤ α_{k+1} + d_k/2 ; pire (e − α_{k+1})/d_k = 0,441 ; 515 entrées après d_k, au plus 6,79 d_k |
| `coeur_q3.py` | `recu_coeur_q3.txt` | Q3 : d_2(x) = 700,006, racine 796,117 ; bloc de x à 705, 750 et 790 : {x, f1, …, f5} pour `core` et `cover`, {x, c0, …, c7} pour `first` et `margin` (m = 3) |
| `r1_check.py` | `recu_r1.txt` | R1 : P_1∘Π_1 9,631 ; H^r_3 13,311 ; Q_1∘Π_3 13,919 ; `first` (m = 3) 10,000 ; Q_1∘Π_1 12,247 ; F = 12 |
| `g4/effets.py`, `g4/demos_detail.py`, `bench/points_summary.py` | `g4/effets.txt`, `g4/demos_detail.txt`, `g4/resume_g4.txt` | lecture des archives `claudepts1` et `claudepts2` |

Pour rejouer la lecture G4, extraire d'abord les archives :
`tar -xzf .../points_g4/sessions/claudepts1/results.tar.gz -C g4/s1 results/cmd/001_synthetic/files results/cmd/002_lidar/files`,
et de même pour `claudepts2` vers `g4/s2`. Les copies extraites ont été effacées après lecture.

**Limites.**

- Mes contrôles portent sur des nuages de n ≤ 8, et 14 sites pour Q3 : ce sont des oracles de correction, sans
  portée d'échelle.
- Les mesures G4 sont lues, pas rejouées. Elles portent sur la règle en niveau carré, ne sont qu'au statut
  `dev_snapshot`, et le meilleur IoU est un oracle par groupe.
- Les cellules et le catalogue sont des cibles de l'utilisateur et extrapolées, pas une vérité.
- Rien ici ne qualifie un statut public.
