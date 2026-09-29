# Juge des trois têtes multi-K : vérification, contrôle hors échantillon et recommandation (29 septembre 2026)

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=benchmark_only (dev seulement : réplique 0 = sous-ensemble commun ; réplique 1 = contrôle hors échantillon)
public_status=not_claimed
GCP non utilisé ; aucun fichier du dépôt modifié ; aucun commit ; aucune graine test ni test_v10b ; au plus 2 processus à 1 fil
tour : binaire figé build/v10-dev-multik/mhgp10_tower (KMAX = 10, entrée cover, 1 fil), relu dans les caches dev existants
```

Rôle : juge des trois conceptions de tête multi-K (tranche oblique, persistance à travers K, antichaîne à ordres mêlés).
J'ai lu d'abord l'audit du 29 septembre, puis les trois documents et leur code. Tout est rejouable depuis
`/workspaces/E-HGP/build/v10-persist/multik/juge/` (§ 8).

## 0. Verdict

- **Aucune des trois têtes ne bat la tête v10-b à une tranche au sens du lot C** (Δ ≥ delta_min = 0,02).
  - À 8 000 points, avec b1.5 et z = 6 (la configuration du lot C), les écarts sont de +0,004 à +0,008 sur la réplique 0.
  - Hors échantillon (réplique 1), ils tombent à +0,001 à +0,005.
- **Les chiffres publiés sont exacts, et aucune tête ne lit la vérité.**
  - J'ai tout recalculé depuis les CSV par scène : J, écarts appariés et IC concordent à l'arrondi près.
  - Les trois têtes à une tranche recalculées égalent le CSV v10-b à 0 près sur les 128 scènes, à K = 10 et K = 5.
- **Contrôle hors échantillon** : configurations figées par les concepteurs sur la réplique 0, rejouées sur les 128 scènes
  de la réplique 1 (dev). À 8 000 points, face à v10-b dans sa configuration du lot C :

  | tête figée | avec b1.5 | gains / pertes > 0,005 | sans remplissage |
  |---|---|---|---|
  | `mixq` (ordres mêlés), z = 6 | **+0,0046 [+0,0020 ; +0,0078]** | 12 / 0 | +0,0000 [−0,0033 ; +0,0034] |
  | oblique c = 3, z = 6 | +0,0016 [−0,0045 ; +0,0078] | 21 / 11 | **−0,0082 [−0,0141 ; −0,0026]** |
  | `eom2w` (persistance), z = 6 | +0,0008 [−0,0044 ; +0,0053] | 5 / 2 | +0,0010 [−0,0012 ; +0,0047] |

  **`mixq` est la seule idée dont l'effet se reproduit**, et seulement avec remplissage (réplique 0 : +0,0064).
- **Recommandation.**
  1. Une campagne dev complète pour `mixq`, bras principal ; `eom2w` en bras secondaire ; contrôle d'attribution `mixq`
     sur MR₂-bord (§ 7.1).
  2. Pas de combinaison des trois (§ 5).
  3. Un préenregistrement ne peut viser que « Δ > 0 », pas « Δ ≥ delta_min ». Même l'oracle du meilleur z par scène, qui
     lit la vérité, ne gagne que +0,020 à +0,022 à 8 000 points (§ 7.3).
- **Objection de l'utilisateur** (« un mélange séparable doit être identifié »). Elle se vérifie pour les modes qui existent
  et que f̂_10 peut voir (§ 6).
  - Sur les 512 composantes gaussiennes, 416 sont réellement séparables : mode propre dans la vraie densité, et au moins
    mcs points. La tête v10-b en retient 378 (91 %), et 99 % de celles dont le col est sous 30 % du pic.
  - Les autres composantes ne sont pas des modes du mélange (84), ont moins de mcs points (12), ou sont des modes trop plats
    pour le bruit de f̂_10.
  - Sur les scènes entièrement séparables, l'écart restant au plafond de Bayes vient de l'affectation sous le col.
  - Les têtes multi-K ne changent rien à ce bilan.

## 1. Méthode du juge

- **Recalcul** (`verif_chiffres.py`) à partir des CSV par scène des concepteurs et des CSV dev de base :
  - v10-b : `kcover_dev_zgrid*.csv` ;
  - sklearn : `kcover_dev_k{5,8,10}cap*.csv`.

  J est la moyenne par cellule de l'ARI_s (une scène par cellule et par réplique). L'IC est un bootstrap à 95 %
  stratifié par taille (20 000 tirages). Les gains et pertes se comptent au‑delà de 0,005.
- **Hors échantillon** (`rep1_mixq.py`, `rep1_oblique.py`).
  - Le code des concepteurs est importé tel quel, avec ses constantes figées.
  - Les tours de la réplique 1 viennent du cache dev du concepteur « persistance » (même binaire). G et T sont régénérés
    et égaux bit à bit.
  - Identité de la tête à une tranche au CSV v10-b de la réplique 1 : 0 écart. Aucun échec (règle D8).
- **Règle du lot C croisée, oracles et puissance** : `crossfit.py`, `analyse_rep1.py`.
- **Diagnostic des modes** : `modes_melange.py`, `modes_presence.py`, `analyse_modes.py`, `analyse_presence.py`. Il lit
  le générateur, et ne sert jamais à choisir.

## 2. Vérification des chiffres (réplique 0, 128 scènes, KMAX = 10)

J recalculé : 2 000 / 8 000 points.

| tête | sans remplissage | b1.5 |
|---|---|---|
| v10-b, z = 4 | 0,774 / 0,760 | 0,780 / 0,783 |
| v10-b, z = 6 (lot C avec b1.5) | 0,754 / 0,762 | 0,765 / 0,790 |
| oblique P1 (c = 3, z = 4) | 0,776 / 0,756 | 0,787 / 0,785 |
| oblique c = 3, z = 6 | 0,759 / 0,762 | 0,776 / 0,798 |
| persistance `eom2w`, z = 6 | 0,771 / 0,770 | 0,779 / 0,794 |
| persistance, feuilles VP (sans z) | 0,775 / 0,762 | 0,779 / 0,785 |
| ordres mêlés `mixq`, z = 6 | 0,765 / 0,769 | 0,777 / 0,796 |
| sklearn ms = 10, feuilles α = 2 | 0,610 / 0,567 | 0,764 / 0,744 (b2.5 : 0,776 / 0,778) |

- **Tout concorde avec les trois documents** : J, écarts appariés, IC, gains et pertes.
  - Exemple : `mixq` z = 6 b1.5 donne +0,0090 [+0,0050 ; +0,0134] (24/4) sur tout le sous-ensemble.
  - Oblique P1 contre v10-b z = 4 donne +0,0071 [+0,0008 ; +0,0146] à 2 000 points avec b1.5.
- **Seule anomalie, mineure** : la tête `pfq_smcs` de l'oblique omet une scène (garde `isfinite`), contre la règle D8. Elle
  n'est pas candidate.
- **Écarts appariés à 8 000 points, même z = 6, b1.5** (réplique 0) :
  - oblique : +0,0077 [+0,0023 ; +0,0136], 17/6 ;
  - `eom2w` : +0,0041 [+0,0003 ; +0,0088], 5/1 ;
  - `mixq` : +0,0064 [+0,0019 ; +0,0119], 9/1.

## 3. Fuites de la vérité et comparaisons inéquitables

**Fuites.** Je n'en ai trouvé aucune dans les têtes : la vérité T ne va qu'à `metrics.scores`. Les réglages se font au
niveau du dev, jamais par scène.

| concepteur | réglages faits sur le dev | vérifiable ? |
|---|---|---|
| oblique | c = 3 choisi sur la grille 2 000 | oui, figé avant 8 000 (`PRECHOIX_8000.txt`, 11:05:28Z) |
| persistance | τ = 1/√K* dit « a priori » | non : τ est l'argmax d'une grille de 9 valeurs à optimum étroit (±0,01 à 0,03 par cran), sans horodatage ; je le traite comme réglé |
| persistance | w_k = k | non : ajouté après coup (déclaré) |
| ordres mêlés | ε, recouvrement, τ | non horodatés ; insensibles entre 0,05 et 0,1 |
| ordres mêlés | 7 variantes, 2 corrections de code après résultats | corrections déclarées ; toutes deux abaissent le gain |

**Comparaisons inéquitables.**
1. **Oblique contre « meilleur z de v10-b ».** Ce biais joue *contre* l'oblique : z = 4 y est figé d'avance, alors que le
   z de la base est choisi dans l'échantillon.
   - À règle égale (z choisi sur 8 000 points), l'oblique gagne +0,0077 sur la réplique 0.
   - Cet écart ne se reproduit pas sur la réplique 1 (+0,0016). La conclusion du concepteur (« ne bat pas ») tient, pour
     une autre raison.
2. **sklearn évalué avec b1.5.** Son remplissage du lot C est b2.5 : à 8 000 points, 0,778 contre 0,744 avec b1.5.
   - L'avantage « +0,062 à 8 000 » de l'oblique P1 (mesuré contre sklearn feuilles α = 1 avec b1.5) tombe à +0,006
     face à sklearn dans sa configuration du lot C (+0,019 pour c = 3, z = 6).
   - Cet avantage appartient de toute façon à l'entrée et à la tête (audit § 1.5), pas à l'axe K.
3. **La moyenne « toutes tailles » gonfle l'effet multi-K.** Les gains sont 2 à 3 fois plus grands à 2 000 points, où z = 6
   sur-segmente la tranche : `eom2w` fait +0,0139 à 2 000 points contre +0,0041 à 8 000. Seules les lignes 8 000 comptent
   (CLAUDE.md : tailles d'intérêt 8 000, 16 000, 32 000 ; 16 000 et 32 000 non mesurées).
4. **Le coût est omis.** Une tête multi-K exige la tour FULL (tous les ordres ≤ K), v10-b l'ordre K seul. Mesure du
   concepteur « persistance » à 8 000 points : ≈ +40 % de temps total. Rien n'est mesuré à 16 000 ou 32 000.

## 4. Contrôle hors échantillon (réplique 1, 128 scènes dev neuves)

À 8 000 points, contre v10-b dans sa configuration du lot C (z = 6 avec b1.5 ; z = 5 sans remplissage) :

| configuration figée sur la réplique 0 | b1.5, réplique 0 → **réplique 1** | sans remplissage, réplique 1 |
|---|---|---|
| `mixq` z = 6 (proposition du concepteur) | +0,0064 → **+0,0046 [+0,0020 ; +0,0078], 12/0** | +0,0000 [−0,0033 ; +0,0034] |
| oblique P1 (c = 3, z = 4, pré‑choisie) | −0,0055 → −0,0067 [−0,0216 ; +0,0047] | −0,0085 [−0,0191 ; −0,0009] |
| oblique c = 3, z = 6 (règle du lot C) | +0,0077 → +0,0016 [−0,0045 ; +0,0078], 21/11 | −0,0082 [−0,0141 ; −0,0026] |
| persistance `eom2w` z = 6 (proposition) | +0,0041 → +0,0008 [−0,0044 ; +0,0053] | +0,0010 [−0,0012 ; +0,0047] |
| persistance, feuilles VP (sans z) | −0,0054 → −0,0084 [−0,0226 ; +0,0042] | −0,0043 [−0,0164 ; +0,0063] |

**Réplique 1 seule, 2 000 et 8 000 points, même z = 6, b1.5.**
- `mixq` : +0,0090 [+0,0034 ; +0,0147] ;
- oblique : +0,0063 [+0,0003 ; +0,0123] ;
- `eom2w` : +0,0054 [−0,0004 ; +0,0110].

Ces gains se font surtout à 2 000 points.

**Répliques 0 et 1 réunies, 8 000 points** (128 scènes, b1.5, z = 6) :

| tête | écart | gains / pertes |
|---|---|---|
| `mixq` | +0,0055 [+0,0028 ; +0,0087] | 21/1 |
| oblique | +0,0047 [+0,0004 ; +0,0089] | 38/17, effet diffus |
| `eom2w` | +0,0025 [−0,0008 ; +0,0058] | 10/3 |

**Règle du lot C croisée** (z choisi sur les 8 000 points d'une réplique, jugé sur l'autre) :

| tête | b1.5 | sans remplissage |
|---|---|---|
| `mixq` | +0,0080 [+0,0025 ; +0,0177] | +0,0037 [−0,0003 ; +0,0080] |
| oblique | +0,0072 [+0,0002 ; +0,0176] | −0,0034 [−0,0084 ; +0,0017] |
| `eom2w` | +0,0050 [−0,0011 ; +0,0147] | +0,0072 [+0,0017 ; +0,0152] |

Réserve : le pli « choix sur la réplique 1 » est gonflé pour tous les bras.
- Sur ce pli, la règle d'égalité à 0,002 fait choisir z = 5 à v10-b, qui perd 0,005 sur la réplique 0.
- Le pli propre est « choix sur la réplique 0 », qui retrouve le tableau ci‑dessus.

**Par famille** (8 000 points, répliques 0 et 1, b1.5, z = 6), écart de `mixq` à v10-b :

| shells | heteroscedastic | unbalanced | filaments | anisotropic | spherical | bridge | hierarchical |
|---|---|---|---|---|---|---|---|
| +0,025 | +0,009 | +0,004 | +0,003 | +0,002 | +0,001 | 0 | 0 |

- Les coquilles portent 57 % du gain en J.
- **Mécanisme.** À K = 10, les familles ne veulent pas le même z : les coquilles perdent à z = 6 (0,945 contre 0,961 à
  z = 5), les filaments y gagnent (0,785 contre 0,743). `mixq` prend les coquilles aux ordres bas (72 % de leurs amas à
  k ≤ 5 d'après le concepteur, à z = 5) et garde les ordres 9–10 ailleurs. Il résout ce conflit de z : l'effet est réel,
  mais étroit.

## 5. Valeur de chaque idée

**Tranche oblique : le fondement le plus solide, sans valeur de score.**
- Le lemme de Blumberg–Lesnick et le corollaire 42 de Rolle–Scoccola stabilisent la *hiérarchie*.
- La construction est exacte et vérifiée (0 violation du lemme 3). C'est aussi la seule idée propre à la multicouverture :
  les bifiltrations de cœurs ne sont que faiblement stables.
- Mais ni la règle de couverture ni l'EOM ne sont couvertes, et la borne est vide dès 10 points perturbés.
- Mesure : effet diffus. Les signes sont proches de 50/50, et 43 % des scènes bougent de plus de 0,005. L'écart est
  négatif sans remplissage hors échantillon, et nul avec b1.5.
- À garder comme objet stable, et pour la robustesse à petit z (effondrement des coquilles à z = 2). Pas comme tête.

**Persistance à travers K : la bonne mesure de significativité, pas un gain de score.**
- Le lemme VP est juste : à r fixe, L_k(r) = {f̂_r ≥ k/(n v_d r^d)}, donc la plage d'ordres séparés mesure le rapport de
  densité cœur/col, sans d ni z.
- En revanche, `eom2` (somme des stabilités sur les ordres appariés, échelle s_k, poids w_k) est heuristique.
- Mesure : c'est une assurance contre un z trop grand. Elle vaut +0,014 à 2 000 points et +0,001 hors échantillon à
  8 000 points avec b1.5.
- Les feuilles VP perdent hors échantillon à 8 000 points (−0,008).
- À garder :
  - VP, pour publier le nombre de modes significatifs ;
  - `eom2w`, en bras secondaire, si l'on doit figer un seul z de 8 000 à 32 000 points.

**Ordres mêlés : fondement partiel, seul effet reproduit.**
- Deux choix sont fondés :
  - l'échelle commune par contenu de probabilité (Rinaldo et al. 2012), invariante par reparamétrisation et sans m ;
  - l'égalité départagée vers l'ordre le plus lisse.
- Ce qui ne l'est pas : le maximum de Lindeberg (malédiction du gagnant) et l'absence de théorème de consistance.
- Le graphe des candidats n'utilise que l'inclusion des ensembles de points, pas les verticales. **L'idée n'est donc pas
  propre à la tour** : elle s'applique telle quelle aux hiérarchies d'HDBSCAN à min_samples 2..K. D'où le contrôle
  d'attribution obligatoire (§ 7.1).

**Combinaison : non.**
- Les écarts par scène des trois têtes sont corrélés (0,54 à 0,60) et portent sur les mêmes coquilles et filaments.
- L'oracle du meilleur des trois par scène, qui lit la vérité, ne donne que +0,019.
- `mixq_sum` (somme sur les ordres, l'idée de Blumberg–Lesnick appliquée à la sélection) ne tient pas hors échantillon.
  - À même z = 6, réplique 1, 8 000 points : +0,0014 [−0,0061 ; +0,0081] avec b1.5.
  - Règle croisée : +0,0028 avec b1.5, −0,0020 sans.

**KMAX = 5 : aucune tête ne gagne significativement** (réplique 0 ; tous les IC contiennent 0) :
- `mixq` z = 4 b1.5 : +0,0029 [−0,0002 ; +0,0057] ;
- oblique : +0,0022 ;
- `eom2w` : +0,0014.

## 6. L'objection de l'utilisateur : les modes d'un mélange séparable

**« Exactement » veut dire exactement pour f̂_K, pas pour f.** À K = 10, l'écart-type relatif de f̂_K vaut environ
1/√10 ≈ 0,32 (Moore–Yackel). Un mode dont le col dépasse environ 70 % du pic n'est donc pas visible à n ≤ 8 000, quelle
que soit l'exactitude du calcul.

**Mesure.** 64 scènes gaussiennes de la réplique 0, 512 composantes. Méthode :
- modes de la vraie densité du mélange, par montée de Carreira-Perpiñán depuis chaque moyenne ;
- col approché par le minimum sur le segment entre modes : c'est une borne basse ;
- présence : une branche pure de l'arbre condensé à K = 10 (au moins 80 % de la composante, au moins 20 % de ses points) ;
- rétention : un amas de sortie de v10-b, z = 6, avec le même critère.

**Composantes.**

| catégorie | nombre | ce que fait la tour |
|---|---|---|
| pas de mode propre dans la vraie densité (`heteroscedastic` hard/extreme, `unbalanced` hard/extreme) | 84 | la prémisse est fausse ; aucune méthode de densité ne peut les séparer |
| mode propre mais moins de mcs = √n points (`unbalanced` à 2 000 points : environ 30 points pour mcs = 45) | 12 | interdites par l'a priori de taille de la tête |
| **séparables** (mode propre et au moins mcs points) | **416** | 389 présentes dans l'arbre, **378 retenues (91 %)** |

**Les pertes sur les séparables** sont 11 perdues par la tête (dont 9 en `anisotropic` extrême, où l'EOM fusionne des
modes présents) et 27 absentes de l'arbre à K = 10 (dont 5 présentes à K = 5).

Elles dépendent de la profondeur du col :

| col / pic | composantes | présentes à K = 10 | retenues |
|---|---|---|---|
| < 0,3 | 357 | 99 % | **99 %** |
| 0,3 à 0,5 | 30 | 83 % | 57 % |
| 0,5 à 0,7 | 9 | 100 % | 89 % |
| ≥ 0,7 | 20 | 10 % | **0 %** |

Le seuil observé (0,7) est celui qu'annonce le bruit de f̂_10.

**Exemples.**
- `spherical` : 128/128 à tous les niveaux, col jusqu'à 27 % du pic.
- **Scènes entièrement séparables** : les 8 composantes sont identifiées dans 48 scènes sur 60. Sur les 44 scènes
  gaussiennes, l'ARI_s vaut 0,886 contre un plafond de Bayes de 0,948. Le reste de l'écart est la masse sous le col et le
  bruit uniforme à affecter (audit § 1.2).
- **`hierarchical`** : les 32 groupes ont 3 modes vrais *à tous les niveaux*, alors que la vérité du banc est le groupe.
  Identifier les modes y coûte l'ARI (0,465). C'est 1/8 de J, un plafond structurel pour toute tête qui cherche les modes.
- **Ce que les têtes multi-K y changent** : presque rien. Sur les familles gaussiennes, les ordres mêlés retiennent
  1 groupe de plus (niveau extrême, diagnostic du concepteur) ; la persistance ne crée aucune branche et élague les modes
  faibles.
- **Un levier de sélection visible** : sur `anisotropic` et `filaments` à 8 000 points, sklearn (feuilles α = 2, b2.5)
  fait mieux que toutes les têtes de la tour (0,883 contre 0,85 ; 0,845 contre 0,79). Avec b2.5, v10-b reste à 0,854 et
  0,789 : c'est la préférence de l'EOM pour le parent, pas le remplissage.

**Pour identifier plus de modes**, les leviers sont :
- des K plus grands que 10 (K ≳ d log n ≈ 23 à 27) ;
- un mcs qui ne soit pas √n ;
- une règle d'affectation sous le col ;
- une sélection moins favorable au parent aux niveaux extrêmes.

Pas l'axe K à K ≤ 10.

## 7. Recommandations

### 7.1 Campagne dev complète : `mixq`, protocole exact

- **Scènes** : `run_campaign.plan('dev', [2000, 8000], 2)`, soit 256 scènes, répliques 0 et 1.
- **Tours** : binaire figé `build/v10-dev-multik/mhgp10_tower` (sha256 272477f2…c542e9), `--k=10 --entry=cover --threads=1`.
  - Une tour FULL par scène ; K = 3, 5, 8, 10 lisent les ordres ≤ K de la même tour (identité vérifiée à K = 5 et 10).
  - Les caches existent déjà (`towers/` et `towers_r1/`, 4,2 Go, dans le scratchpad de session). Ils sont à reconstruire par
    `persistance_k/cache_towers.py` s'ils ont disparu.
- **Bras** (mcs = round(√n), entrée cover, remplissage borné à k = max(K, 5)) :
  - **A0** : v10-b, tranche d'ordre K.
  - **A1 (principal)** : `mixq`, ordres 2..K, référence K, ε = 0,1, recouvrement 0,1, contenu de probabilité, égalités vers
    le plus grand ordre. Code `ordres_meles/{mixed_orders,multik_lib}.py`, épinglé par sha256.
  - **A2 (secondaire)** : `eom2w`, w_k = k, ordres 1..K. Code `persistance_k/{multik,persist_head}.py`, épinglé.
  - **Témoins** :
    - sklearn à min_samples = K (têtes et remplissages du lot C) ;
    - MR₂-bord avec la tête A0 ;
    - **A1 sur MR₂-bord**, qui attribue l'effet à l'objet ou à la seule idée multi-K : hiérarchies d'HDBSCAN à min_samples
      2..K avec entrée bord, via `audit_hier/auditeur_objet/objet_lib.py` (`sk_mr_tree`, `mr_border`), données à `mixq`
      qui n'a besoin que des arbres et des ensembles de points.
- **Grille** : z ∈ {3, 4, 5, 6, 8} × remplissage ∈ {none, b1.5, b2, b2.5}.
- **Pilote** : `campagne_dev_multik.py` (A0, A1, A2). Il a été vérifié sur une scène : 120 contrôles contre les CSV
  existants, 0 écart. 19 s mesurées sur une scène facile de 8 000 points (4 K, 5 z, 4 remplissages) ; compter de l'ordre
  de 1 à 3 h à 2 processus pour les 256 scènes.
- **Règle de choix**, identique pour chaque bras et chaque K : celle du lot C.
  - argmax de J sur les scènes dev de 8 000 points ;
  - en cas d'égalité à 0,002 près, le z le plus proche de 3, puis la politique la plus simple.
- **Estimation principale : croisée.**
  - Choix sur une réplique, mesure sur l'autre, à 8 000 points. On publie les deux plis séparément et leur moyenne.
  - IC par bootstrap par cellule, retournement de signe, Holm sur les 4 K par bras.
  - L'estimation dans l'échantillon est publiée à côté, sans valeur de décision.
- **Extension d'échelle obligatoire** (CLAUDE.md) : `plan('dev', [16000], 1)` (64 scènes), configurations figées. On y
  mesure le signe de l'écart, le coût de la tour FULL contre l'ordre seul (temps et mémoire), et la dérive du z optimal.
- **Critère pour passer au préenregistrement** :
  - A1 − A0 croisé > 0, avec borne basse > 0, à K = 10 **et** K = 8, et même signe dans chaque pli ;
  - aucune famille sous −0,01 à 8 000 points ;
  - même signe à 16 000 points ;
  - attribution écrite : A1 tour − A1 MR₂-bord.
- **Attendu** (écrit ici, avant la campagne) :
  - A1 − A0 ≈ +0,003 à +0,006 à 8 000 points avec b1.5, porté par les coquilles et `heteroscedastic` ;
  - environ 0 sans remplissage, à K = 3 et à K = 5.

### 7.2 Ce qu'exige un préenregistrement

- **Graines** : un espace neuf, jamais lu (par exemple `test_v10mk`) ; tailles 8 000, 16 000 et 32 000 ; 5 répliques
  (960 scènes), comme le lot C.
- **Puissance** : l'écart-type par scène vaut 0,017 (`mixq` z = 6 b1.5 à 8 000 points). 71 scènes suffisent pour 80 % de
  puissance à Δ = 0,005 : 960 scènes, c'est large.
- **Code figé** : sha256 des fichiers ; Python et numpy/scipy/scikit-learn épinglés, portables sur G4 comme l'amendement
  du lot C.
- **Portes** :
  - identité à la tranche quand l'ensemble d'ordres vaut {K} (0 écart) ;
  - programme dynamique à un ordre = EOM (0 écart) ;
  - invariants.
- **Configurations** : A0 et A1 par K, choisies sur dev par la règle ci-dessus et publiées avec la sha256 des CSV dev.
- **Hypothèse et règle écrites avant le test.**
  - « A1 améliore A0 : Δ > 0 à K = 10 », avec IC > 0, Δ > 0 à chaque taille, et aucune perte familiale significative.
  - La règle « bat » du lot C (Δ ≥ 0,02) est hors d'atteinte d'après le dev. Il faut soit adopter explicitement cette règle
    plus faible, soit annoncer comme issue attendue « pas de différence au sens de delta_min ».
- **Famille secondaire** : A1 sur MR₂-bord (attribution), K = 3, 5 et 8, et les scores sans remplissage.
- **Règles communes** :
  - échec = ARI 0 (règle D8), refus comptés ;
  - coût déclaré à chaque taille (tour FULL contre ordre seul) ;
  - prédiction chiffrée tirée de l'estimation croisée.

### 7.3 Pourquoi aucune tête ne bat la tête à une tranche

- **La marge de sélection est plus petite que delta_min.** Oracles à 8 000 points, qui lisent la vérité :
  - le meilleur z par scène (K = 10, b1.5) gagne +0,022 (réplique 0) et +0,020 (réplique 1) sur v10-b z = 6 ;
  - le meilleur (K, z) par scène gagne +0,037 et +0,031.

  Une sélection réaliste en capte 15 à 25 %, soit environ +0,005 : exactement ce que `mixq` obtient.
- **Les trois têtes ne changent que la sélection** parmi les mêmes candidats : amas condensés à mcs = √n, ordres ≤ 10,
  entrée cover, EOM. Or la tranche au z du dev retient déjà 91 % des modes séparables (§ 6).
- **Ce qui manque au score est ailleurs** :
  - modes absents ou trop plats pour f̂_10 ;
  - composantes qui ne sont pas des modes ;
  - a priori de taille mcs = √n ;
  - affectation sous le col ;
  - convention de vérité de `hierarchical`.

  Aucun de ces points n'est un problème de choix d'ordre à K ≤ 10.

## 8. Fichiers

Dans `/workspaces/E-HGP/build/v10-persist/multik/juge/` :
- `juge_lib.py` : lecture des CSV, J, écarts appariés.
- `verif_chiffres.py` → `verif_chiffres.txt` : recalcul des chiffres.
- `rep1_mixq.py`, `rep1_oblique.py` → `rep1_*.csv`, `rep1_*.log` : réplique 1.
- `analyse_rep1.py` → `analyse_rep1.txt` ; `crossfit.py` → `crossfit.txt` ; `tables_finales.py` → `tables_finales.md`.
- `modes_melange.py`, `modes_presence.py` → `modes_*.csv` ; `analyse_modes.py`, `analyse_presence.py` → `analyse_*.txt`.
- `campagne_dev_multik.py` : pilote de la campagne du § 7.1.

Les tours relues (4,2 Go, répliques 0 et 1) restent dans le scratchpad de session, hors dépôt.
