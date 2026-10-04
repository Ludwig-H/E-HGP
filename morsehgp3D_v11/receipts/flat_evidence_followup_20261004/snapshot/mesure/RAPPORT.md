# Protocole de mesure : sortie plate de $H^{r}_{k+1}$ contre la vérité terrain, le MAP et HDBSCAN

4 octobre 2026, travail de 07 h 02 à 07 h 54 UTC (heures lues par `date -u`). Rôle : protocole de mesure et
statistique, workflow « de la hiérarchie de points v11 à un clustering plat ».

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (Python local : numpy 2.5.3, scipy 1.18.1, scikit-learn 1.9.1 ; archives G4 lues seulement)
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé. Aucune commande git qui écrit. Aucune construction native. Aucun réseau.
Écritures : build/v11-points-select/mesure/ seulement.
```

**Règle de lecture.** Chaque nombre de ce rapport vient d'un script de ce dossier et de sa sortie gardée dans
`sorties/` (table du § 9). Chaque énoncé mathématique est soit prouvé ici (preuve courte), soit une fixture exacte
(`fixtures_metriques.py`), soit marqué **conjecture** ou **hypothèse**. Les petits nuages locaux (n ≤ 2 000) sont des
oracles de correction ou des pilotes déclarés : ils n'établissent ni une pente ni une conclusion d'échelle. La v10
est lue comme source de données et de contre-exemples, jamais comme autorité (consigne du 4 octobre).

## 0. Décisions proposées (résumé)

1. **Métrique primaire : mIoU un-à-un** (`miou_h`) : IoU moyenne par groupe vrai, appariement hongrois de somme
   maximale, groupe non apparié compté 0, bruit vrai compté contre le cluster qui l'avale, points void ignorés.
   **Gardes** : PQ (seuil IoU > 1/2, filtre FP déclaré sur LiDAR) et ARI_s (bruit en singletons). **Descriptifs** :
   AMI_s, pureté et pureté inverse, nombre de clusters, part de bruit, couverture, bruit absorbé. **Diagnostics
   seulement** : mIoU « meilleur cluster », ARI_nc, ARI restreinte aux inliers (§ 2).
2. **MAP** : calculable en forme close à 32 000 points (≤ 0,6 s par scène, génération comprise) ; référence de
   difficulté (« niveau de Bayes ») et seconde vérité, **pas un plafond d'IoU** (contre-exemple exact F7) (§ 2.6, § 3.1).
3. **Plan synthétique** : 8 familles × {medium, hard} × {3, 8, 20} groupes × {8 000, 16 000, 32 000} points, bruit
   5 %, soit 144 cellules, **R = 3 répétitions (432 scènes) par défaut**, scènes **gelées en fichiers** avant le test
   (l'empreinte flottante du générateur épinglé dépend du BLAS : 80 scènes G4 sur 128 diffèrent du rejeu local) (§ 3.1).
4. **Bras** : tour (tête réglée sur dev) ; **même tête sur l'arbre de HDBSCAN** (attribution, fusions ex æquo lues en
   multifusions) ; HDBSCAN sklearn tel quel réglé sur dev à budget égal ; témoins `cover`, `first`, `core`,
   fermeture, MR₂-bord ; MAP. **Mêmes mcs** des deux côtés, `min_samples = k`, même machine, même processus (§ 3.2).
   La sélection commune est portée et qualifiée : identique à sklearn sur 672 configurations sur 672 (§ 3.2).
5. **LiDAR** : les cohortes existantes (démos, échecs, voisines, témoins) sont **choisies sur l'issue de HDBSCAN** ;
   redressées sur la population du criblage, l'écart de niveau B devient −0,003 à +0,004 (§ 3.3). Le test LiDAR se
   fait sur un **échantillon systématique de trames non choisi sur l'issue** (pas ≥ 30 trames), en non-infériorité à
   marge 0,02, avec strate pré-déclarée « objets minces ».
6. **Analyse** : unité = scène ; estimande = moyenne équipondérée des cellules ; test t sur l'erreur type
   intra-cellule (niveau réel 0,042 à 0,058 en simulation à queues lourdes), IC par bootstrap stratifié corrigé ;
   retournement de signe publié comme contrôle (prouvé conservateur) (§ 4).
7. **Règle de décision écrite d'avance** (§ 4.4) ; dev et test disjoints, configuration gelée dans un
   préenregistrement, test exécuté une fois ; prédictions P1–P9 à graver avant E1 (§ 4.3).
8. **Budget** : 6 sessions G4 de 2 300 s (hypothèse basse) à 9 (hypothèse haute), dont 1 à 2 de dev ; la session 1
   remesure coût et mémoire à 16 000 et 32 000 avant de figer le plan (§ 5).

## 1. Sources lues et données réutilisées

- `contexte/CONTEXTE.md`, `morsehgp3D_v11/docs/HIERARCHIE_POINTS.md` (en entier), synthèse du juge
  (`receipts/developpement_20261003/points_math/SYNTHESE.md`, § 1–5, 8–10), réponses de l'auditeur
  (`audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md`, Q4 et suivi), extraits de la thèse (§ 4.4.4, ch. 5,
  § 9.1), `Zoltan/FoundationModel/MESURE.md` et `SPECIFICATION.md` (§ 2), `Zoltan/demos/README.md` et
  `recherche/criblage_08.jsonl`, banc v11 (`bench/points_campaign.py`, `points_hierarchy.py`, `points_summary.py`).
- v10, lus comme données : `docs/conception/EVAL_v2.md` (§ 0, 5, 6, 7), `docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md`,
  `audits/REPONSE_CLAUDE_MAP_UNIVERS_ET_BATTERIE_20261001.md`, `REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md`,
  `build/v10-batteries-iou/PREPARATION.md`.
- Archives G4 du reçu `points_g4` extraites dans `archives/` : sessions D (`claudepts4`) et F (`claudepts6`), plus
  C (`claudepts3`, non utilisée dans les tableaux) et les manifestes. F : commit poussé f02f91c7e, porte stricte
  conforme ; D et F publient des scènes identiques hors chronométrage (reçu). Générateur v10 épinglé copié en
  `vendor_scenes_v10_pin.py` (`git show origin/main:.../full_points_20261003/experiment/vendor_scenes.py`).

## 2. (a) Métriques d'un clustering plat avec bruit

### 2.1 Conventions

Vérité : groupe ≥ 0, bruit vrai −1 (il compte contre le cluster qui l'avale), void −2 (retiré de tout calcul, comme
l'évaluation panoptique de SemanticKITTI). Prédiction : cluster ≥ 0, bruit < 0. Pour un groupe $G$ et un cluster
$C$ : $n=\lvert G\cap C\rvert$, $\mathrm{IoU}(G,C)=n/(\lvert G\rvert+\lvert C\rvert-n)$, où $\lvert C\rvert$ compte
tous les points non void du cluster, bruit vrai compris. Implantation : `metriques.py`.

### 2.2 Énoncés prouvés

**M1 (seuil entier).** $\mathrm{IoU}(G,C)>1/2\iff 3n>\lvert G\rvert+\lvert C\rvert$, et alors $n>\lvert G\rvert/2$ et
$n>\lvert C\rvert/2$ ; l'appariement « IoU > 1/2 » est donc unique (deux clusters disjoints ne peuvent contenir
chacun plus de la moitié d'un même groupe, ni deux groupes disjoints chacun plus de la moitié d'un même cluster).
*Preuve.*
$2n>\lvert G\rvert+\lvert C\rvert-n\iff 3n>\lvert G\rvert+\lvert C\rvert\geq\lvert G\rvert+n$, d'où $2n>\lvert G\rvert$ ;
symétriquement pour $C$. □ Toute décision PQ est ainsi entière (`pq_match`).

**M2 (réduction du hongrois).** Pour $g$ groupes, il existe une affectation optimale (somme d'IoU maximale) où
chaque groupe est affecté à l'une de ses $\min(g,c)$ meilleures colonnes. *Preuve.* Si le groupe $i$ est affecté à
une colonne $j$ hors de sa liste, au plus $g-1$ colonnes de sa liste sont prises par les autres groupes : l'une
$j'$ est libre et $\mathrm{IoU}(i,j')\geq\mathrm{IoU}(i,j)$ ; on réaffecte $i$ à $j'$ sans toucher aux autres ; chaque
réaffectation place définitivement un groupe dans sa liste. □ Cela borne la matrice LiDAR (milliers de clusters)
sans changer l'optimum ; la somme retenue est recalculée en `Fraction`.

**M3 (chaîne des plafonds).** Si chaque cluster plat est un bloc d'une hiérarchie $H$ (antichaîne de l'arbre
condensé, sans complétion : c'est le cas de HDBSCAN EOM, feuilles et ε, et de la sélection commune du § 3.2 sur
$H^{r}_{k+1}$), alors $\mathrm{mIoU}_h\leq\mathrm{mIoU}_{\mathrm{best}}\leq B(H)$, où $B(H)$ est le niveau B (meilleur bloc
par groupe, moyenne). *Preuve.* Le cluster affecté à un groupe est un des clusters plats, donc au plus le meilleur
cluster plat ; chaque cluster plat est un bloc de $H$ (ensemble des points présents dans le cluster condensé à sa
naissance), donc au plus le meilleur bloc. □ Une **complétion** (rattacher après coup des points restés seuls)
casse M3 : un cluster complété n'est plus un bloc. Le niveau B n'est donc un plafond que pour les sorties sans
complétion.

**M4 (MAP).** Sous le modèle de mélange i.i.d. aux vrais paramètres $\theta$, la règle MAP
$\hat y(x)=\arg\max_c\pi_c p_c(x)$ maximise l'exactitude attendue parmi **toutes** les règles d'étiquetage, même
celles qui utilisent tout l'échantillon. *Preuve.* Sachant $\theta$ et les positions $X$, les étiquettes sont
indépendantes et $P(y_i=c\mid X)=P(y_i=c\mid x_i)$ ; pour toute règle,
$P(\hat y_i=y_i\mid X)\leq\max_cP(y_i=c\mid x_i)$, atteint par le MAP ; on somme sur $i$. □ Le plan v10 est à
composition fixe (effectifs exacts), pas i.i.d. : M4 n'y vaut qu'approximativement. **Le MAP n'est pas un plafond
de mIoU** : fixture exacte F7 ci-dessous (exactitude 4/5 contre 3/5, mIoU 2/5 contre 5/12).

**M5 (fractionner).** Si $C\subseteq G$ et $C=C_1\sqcup C_2$ avec $C_1,C_2$ non vides, alors
$\mathrm{IoU}(G,C_i)=\lvert C_i\rvert/\lvert G\rvert<\lvert C\rvert/\lvert G\rvert=\mathrm{IoU}(G,C)$ : fractionner un cluster
pur ne peut que baisser mIoU_h. Fractionner un cluster impur peut la monter : c'est corriger une
sous-segmentation, pas en commettre une. □

### 2.3 Fixtures exactes (`fixtures_metriques.py`, sortie `sorties/fixtures_metriques.txt`, conforme)

| # | Cas minimal | Ce qu'il montre |
| --- | --- | --- |
| F1 | vérité [0,0,1,1], prédiction [0,0,−1,−1] | ARI_nc = 1 et ARI restreinte = 1 : étiqueter un groupe entier en bruit n'est qu'un renommage pour « bruit = classe » ; ARI_s = 4/7 ; mIoU_h = 1/2 |
| F2 | deux groupes de 4 fusionnés | mIoU_best = 1/2 (le même cluster compté deux fois), mIoU_h = 1/4, PQ = 0, pureté inverse = 1 : **mIoU_best et pureté inverse récompensent la sous-segmentation** |
| F3 | chaque groupe coupé en deux moitiés | mIoU_h = 1/2, PQ = 0 (IoU = 1/2 n'est pas > 1/2), pureté = 1 : **la pureté récompense la sur-segmentation** ; AMI_s 0,533 contre ARI_s 0,364 (AMI plus indulgente à la sur-segmentation ici) |
| F4 | groupes exacts + deux clusters faits de bruit vrai | mIoU_h = 1 : **aveugle aux clusters parasites** ; PQ = 2/3 et ARI_s 0,904 les voient |
| F5 | tout en bruit | 0 partout (mIoU_h, PQ, ARI_s) : pas de prime à l'abstention |
| F6 | 7 + 2 points, 2 clusters | l'affectation de somme maximale (2/7) rompt la paire IoU 5/9 > 1/2 de la PQ : mIoU_h et PQ n'apparient pas les mêmes paires |
| F7 | positions {a, b}, a priori 0,8/0,2 | le MAP (tout A) : exactitude 4/5, mIoU 2/5 ; la règle « b → B » : exactitude 3/5, mIoU 5/12 |
| F8 | tout en singletons ; un seul cluster | pureté 1 et mIoU_h 1/5 ; pureté inverse 1 et mIoU_h 1/4 : aucune solution triviale ne gagne en mIoU_h |
| F9–F11 | partition parfaite ; void ; filtre FP « stuff » | void retiré même dans un cluster ; un cluster non apparié majoritairement « stuff » n'est pas un FP |

### 2.4 Quelle métrique répond à « IoU de préférence »

| Métrique | Récompense la sur-segmentation | la sous-segmentation | l'abstention (bruit) | Aveugle à | Rôle proposé |
| --- | --- | --- | --- | --- | --- |
| mIoU_h (hongrois, un-à-un) | non (F3, F8) | non (F2, F8) | non (F5) | clusters parasites hors des groupes (F4) | **primaire** |
| mIoU_best (meilleur cluster) | non | **oui** (F2) | non | idem | diagnostic (écart à mIoU_h = double compte) |
| PQ = SQ × RQ (IoU > 1/2) | non, mais seuil dur (F3 : 1/2 → 0) | non | non | qualité sous le seuil | **garde** (et RQ = F1 objet) |
| ARI_s (bruit en singletons) | non | non | non | — | **garde** |
| AMI_s | plus indulgente que ARI_s (F3) | non | non | — | descriptif |
| ARI_nc (bruit = classe) | — | — | **oui** (F1) | — | à proscrire |
| ARI restreinte aux inliers | — | — | **oui** (F1) | — | diagnostic seulement |
| pureté | **oui** (F3, F8) | non | — | rappel | descriptif, jamais seule |
| pureté inverse | non | **oui** (F2, F8) | — | précision | descriptif, jamais seule |
| nombre de clusters, part de bruit | — | — | — | qualité | descriptif ; ratio aux groupes vrais |

Se garder de la sur- et de la sous-segmentation : (i) mIoU_h primaire (un-à-un : un cluster ne sert qu'un groupe, un
fragment ne vaut que sa part) ; (ii) PQ et ARI_s en gardes de non-infériorité, qui voient les clusters parasites que
mIoU_h ignore (F4) ; (iii) publier toujours le couple pureté / pureté inverse et le nombre de clusters ; (iv) dans le
réglage sur dev, contraindre la PQ (§ 4.5) pour qu'une tête ne gagne pas en mIoU_h en multipliant les fragments.

**Accord empirique** (`accord_metriques.py`, pilote HDBSCAN, n = 2 000, 30 configurations) : les moyennes classent
les configurations presque pareil (Spearman avec mIoU_h : 0,996 mIoU_best, 0,991 PQ, 0,999 ARI_s, 0,998 F-pureté ;
0,754 pour la couverture seule), mais **scène par scène** le signe d'une différence appariée selon la PQ ne suit
celui de mIoU_h que dans 39 % à 82 % des scènes (62 % à 82 % en EOM), selon ARI_s dans 80 % à 92 %. Les gardes se
lisent donc en moyenne, jamais comme un vote par scène. L'écart mIoU_best − mIoU_h atteint 0,064 à mcs = k = 3
(52 clusters) : c'est le double compte de F2 sur des sorties réelles.

**Coûts** : ARI_s négligeable ; AMI_s 0,03 / 0,09 / 0,32 s à 8 000 / 16 000 / 32 000 points avec 1 600 à 4 300
classes singletons (`cout_ami.py`). AMI_s baisse un peu avec n à structure égale (0,786 → 0,766) : elle n'est pas
invariante à la taille, raison de plus pour ne pas en faire la primaire.

### 2.5 LiDAR : conventions

Instances « thing » d'au moins 50 points (seuil `min_points` de la PQ SemanticKITTI, convention du banc v11) ; void
(sémantique 0, 1, 52, 99) ignoré ; « stuff » et petites instances comptent contre un cluster (−1). mIoU_h par trame
sur ses instances ; **PQ_th avec filtre FP déclaré** : un cluster non apparié dont plus de la moitié des points
non void sont « stuff » n'est pas un faux positif (F11). Sans ce filtre, la PQ d'un clustering sans sémantique
compte chaque façade comme une erreur (v10 : PQ 0,008 et 0,020, « des centaines de clusters non appariés »).

### 2.6 Le MAP : rôle

Le MAP (`carte_map.py`) rejoue le générateur épinglé en capturant ses paramètres (centres, rotations, échelles,
rayons, directions, boîte du bruit) et calcule $\arg\max_c n_c p_c(x)$ en forme close : gaussiennes ; mélange des
trois sous-amas (`hierarchical`) ; coquille radiale avec son pli $s<0$ (`shells`) ; segment uniforme convolué par
une gaussienne isotrope (`filaments`, ponts de `bridge`, dont la classe bruit est « ponts + uniforme ») ; uniforme
sur la boîte élargie de 10 %. Porte de rejeu : empreinte sha256 des points égale à celle du générateur sur la même
machine ; normalisation des densités contrôlée par Monte-Carlo (`controle_map.py`, conforme). Le MAP sert à trois
choses : (i) **niveau de Bayes** de chaque cellule (difficulté indépendante de toute méthode) ; (ii) **seconde
vérité** (mIoU contre les étiquettes MAP) ; (iii) **regret** (MAP − méthode), qui peut être négatif (F7). Il ne
sert jamais de plafond ni de critère de réglage.

## 3. (b) Plans de mesure

### 3.1 Synthétique aux tailles d'intérêt

**Plan.** 8 familles × {medium, hard} × {3, 8, 20} groupes × {8 000, 16 000, 32 000} points × bruit 5 % = 144
cellules ; R répétitions par cellule (R = 3 par défaut, § 4.6). **Dev** : les 48 cellules famille × niveau × groupes
à 8 000 et 16 000, 2 répétitions (192 scènes). Bloc secondaire de robustesse au bruit : 8 familles ×
hard × 8 groupes × n = 16 000 × bruit {0 ; 20 %} × 3 répétitions (48 scènes, descriptif). `easy` et `extreme` :
témoins descriptifs (16 cellules à 8 000). Graines dérivées par sha256 de (espace, spécification, répétition), espaces
`dev_v11e1` et `test_v11e1` disjoints (vérifié à la création du manifeste).

**Les niveaux nommés ne mesurent pas la difficulté.** Ils ont été calibrés en v10 sur l'échec de HDBSCAN
(`CALIBRATION` : ARI de HDBSCAN(mcs = 20) visé 0,95 / 0,70 / 0,40 / 0,15 à n = 800) ; le niveau de Bayes le dément
(`resume_bayes.py`, MAP mIoU_h moyen sur g = 3, 8, 20, à n = 32 000, medium / hard) :

| Famille | MAP medium | MAP hard | Lecture |
| --- | --- | --- | --- |
| anisotropic | 0,979 | 0,959 | séparable |
| bridge | 0,967 | 0,961 | séparable |
| filaments | 0,987 | 0,963 | séparable |
| heteroscedastic | 0,929 | 0,825 | confusion bayésienne réelle |
| hierarchical | 0,997 | 0,997 | **aucune** difficulté statistique : tout est dans le choix du niveau |
| shells | 0,999 | 0,999 | **aucune** difficulté statistique |
| spherical | 0,977 | 0,962 | séparable |
| unbalanced | 0,847 | 0,691 | petits groupes noyés |

Le niveau de Bayes ne dépend pas de n (8 000 et 32 000 à 0,01 près). On publie donc chaque résultat aussi par
**strate de Bayes** (MAP ≥ 0,97 ; [0,90 ; 0,97[ ; < 0,90), et non seulement par nom de niveau.

**MAP à 32 000 : oui.** Génération, rejeu, quantification et MAP prennent au plus 0,13 / 0,19 / 0,58 s par scène à
8 000 / 16 000 / 32 000 points, 20 groupes compris (`controle_map.py`, deux exécutions sur le codespace partagé :
0,29 puis 0,58 s à 32 000) : O(n × composantes), aucun voisinage.

**Scènes gelées (obligatoire).** Le générateur épinglé n'est pas reproductible bit à bit d'une machine à l'autre :
sur les 128 scènes de la session F, **80 ont une empreinte flottante différente** du rejeu local (familles à produit
matriciel BLAS : spherical, anisotropic, heteroscedastic, unbalanced, hierarchical) ; bridge, filaments et shells
sont identiques (`map_contre_niveau_B.py`). La grille 18 bits reste identique (marge minimale à une frontière
d'arrondi : 8,1 × 10⁻⁸ pas, contre un décalage du dernier bit ≤ 10⁻¹⁰ pas). Conséquence : générer les scènes de test
**une fois**, calculer le MAP **sur les mêmes flottants**, et livrer grille, vérité, étiquettes MAP et empreintes
(sha256 de la grille entière) dans un manifeste ; la VM consomme des fichiers, elle ne régénère pas.

**Ce que le niveau B dit déjà, rapporté au MAP** (`map_contre_niveau_B.py`, mêmes 128 scènes) : le meilleur bloc
dépasse l'IoU MAP d'un groupe dans 8 % à 15 % des groupes (le MAP n'est pas un plafond, M4) ; le regret moyen
(MAP − niveau B) de HDBSCAN croît avec k (0,128 ; 0,142 ; 0,162 ; 0,175 pour k = 2, 3, 5, 10) quand celui de la tour
décroît (0,120 ; 0,115 ; 0,112 ; 0,096) ; corrélation entre regret de HDBSCAN et gain de la tour : +0,15, +0,53,
+0,56, +0,73. À k = 5, l'écart de niveau B vaut +0,031 (MAP ≥ 0,97), +0,075 ([0,90 ; 0,97[) et +0,050 (< 0,90). Le
grand regret commun (≈ 0,1 même pour la tour) est la queue des groupes, que seules la sélection et la complétion
peuvent récupérer : c'est ce que le niveau C doit mesurer.

### 3.2 Bras comparés, mcs, ordres, sélection commune

Ordres k ∈ {2, 3, 5, 10}, HDBSCAN à `min_samples = k` (point compté, convention v11/Zoltan). Pour chaque scène,
chaque k et chaque mcs, tous les bras tournent dans **le même processus sur la même machine** (l'ordre des ex æquo
de sklearn dépend de la machine, v10).

| Bras | Contenu | Rôle |
| --- | --- | --- |
| T* | $H^{r}_{k+1}$ → condensation à mcs (masses entières) → tête $\theta\in\Theta_T$ réglée sur dev → complétion éventuelle | la méthode jugée |
| T*₀ | T* sans sa complétion | isole l'effet de la complétion |
| A (attribution) | **la même condensation et la même sélection que T*₀** (mcs, z, ε, méthode) appliquées à l'arbre de HDBSCAN lu en multifusions | sépare l'effet de la hiérarchie de celui de la tête |
| H* | `sklearn.cluster.HDBSCAN` tel quel, configuration $\eta\in\Theta_H$ réglée sur dev, même cardinal que $\Theta_T$ | l'adversaire |
| H0 | HDBSCAN tel quel, EOM, ε = 0 | adversaire par défaut, publié |
| témoins | $H^{r}_{k+1}$ à κ = 2, `cover`, `first`, `core`, fermeture (k, k+1), MR₂-bord, $P_2\circ\Pi_1$ si porté ; mêmes têtes | contexte (niveaux B et C) |
| MAP | § 2.6 | référence |

**mcs.** Comparaisons **à mcs égal**. Primaire : la moyenne sur mcs ∈ {10, 20, √n} (trois valeurs sans étiquette).
Secondaire : chaque mcs, et **mcs = k** (perte prédite, P3). Diagnostic seulement : mcs « relatif à la taille des
objets » (fraction de la taille du plus petit groupe vrai), qui lit la vérité et ne fonde aucune revendication.
LiDAR : mcs ∈ {10, 20, 50} (50 = seuil d'évaluation, sans étiquette ; √n vaudrait 180 à 280 et tuerait les piétons).

**Grilles à budget égal (proposition par défaut, à figer avant dev).** $\Theta_H$ = {eom, leaf} × ε ∈ {0, 1, 2, 4} ×
médiane de $d_k$ (8 configurations). $\Theta_T$ : 8 têtes choisies par les rapports de sélection du workflow (EOM
$\lambda=r^{-z}$ avec z = 1 toujours publié, feuilles, analogue de ε, complétion par lignée ou aucune). La
famille `hierarchical` impose ε des deux côtés : sans lui, HDBSCAN plafonne à 0,333 (24 sous-amas au lieu de 8,
`epsilon_sklearn.py`) et la v10 mesurait 0,98 contre 0,33 avec ε = 5 × médiane.

**Sélection commune qualifiée** (`selection_commune.py`, `porte_selection.py`, conforme). Condensation, stabilité,
EOM, feuilles, ε et étiquetage transcrits de `sklearn/_hdbscan/_tree.pyx`, généralisés aux multifusions (jamais
binarisées) et aux entrées tardives des points (un point possédé par un nœud quitte le cluster à
$\lambda(e_x)$). Résultats :

- sur les arbres binaires de sklearn : **arbre condensé et étiquettes identiques dans 672 configurations sur 672**
  (8 familles, n = 300 à 2 000, k = 2 à 10, mcs ∈ {k, 5, 10, 20}, EOM et feuilles) ; ε > 0 identique dans les 204
  configurations où sklearn tourne ;
- fixture T0 (deux triangles de la thèse, k = 2, $H^{r}_{3}$ par l'oracle exact) : **ABC | DEF** à mcs 2 et 3, EOM
  et feuilles ; l'arbre de HDBSCAN fusionne les six points à 1,4142 d'un coup : tout en bruit ;
- quatre mutants tués (λ = r au lieu de 1/r ; mcs + 1 ; z = 2 au lieu de 1 ; stabilité inversée).

**Les ex æquo de HDBSCAN ne sont pas anodins** (`ex_aequo.py`, n = 2 000, 768 configurations) : 10 % des fusions de
l'arbre sont ex æquo (médiane ; 22 % au plus) ; lire ces plateaux en multifusions change la partition dans 400
configurations sur 672 (`porte_selection.py`), le plus souvent de quelques points (|Δ mIoU_h| médiane 0,0004,
p95 0,0069), mais de plus de 0,05 dans 20 configurations sur 768 et **jusqu'à 0,41** (anisotropic medium, k = 10,
mcs 20, EOM : 0,392 en lecture binaire de sklearn, 0,802 en multifusions). Le bras A lit donc les ex æquo en
multifusions (règle « ne jamais binariser », la même que pour la tour) ; H* reste sklearn tel quel.

**Enregistrement par scène** (lecteur de reçu qui recalcule tous les tableaux, comme `check.py` de `points_g4`) :
pour chaque bras, k, mcs et configuration, l'IoU un-à-un de chaque groupe, TP, FP, FN, FP ignorés, SQ, ARI_s, AMI_s,
pureté et pureté inverse, nombre de clusters, part de bruit, couverture, bruit absorbé, sur- et sous-segmentation au
seuil mcs, temps et RSS ; par scène, le niveau B de chaque règle par groupe et les métriques du MAP ; sur LiDAR, les
seules clés `sem | inst << 16` des instances, jamais de coordonnée ni d'étiquette par point.

**Porte d'environnement ε (à passer sur G4).** Avec numpy 2.5.3 et scikit-learn 1.9.1, HDBSCAN avec
`cluster_selection_epsilon` > 0 lève `TypeError` dès que la recherche remonte l'arbre : 20 configurations sur 36
(`epsilon_sklearn.py`, refus local ;
défaut déjà vu en v10). G4 épingle numpy 2.2.6 et scikit-learn 1.7.2 : la porte doit y être conforme,
sinon la grille ε de HDBSCAN passe par le port vérifié ci-dessus (identique à sklearn là où il tourne) et la
revendication le dit. Un adversaire amputé de ε serait un homme de paille sur `hierarchical`.

### 3.3 LiDAR

**Les cohortes existantes sont choisies sur l'issue de HDBSCAN** (vérifié sur `criblage_08.jsonl`,
`ponderation_criblage.py`) : les 39 « échecs » sont **exactement** les trames du criblage 1 sur 8 (299 trames) où
HDBSCAN laisse une instance ≥ 50 points à IoU ≤ 1/2 à K = 5 ou 10 ; les 20 « témoins » sont pris parmi les 260
trames **sans** échec ; les démos sont des échecs cherchés ; les 72 voisines entourent des échecs. Choisir sur le
score de HDBSCAN biaise la différence : si les scores $T$ (tour) et $H$ (HDBSCAN) d'un objet sont corrélés sans
l'être parfaitement, $E[T-H\mid H\leq 1/2]>E[T-H]$ (pour un couple gaussien de même moyenne $\mu$, de même variance et de
corrélation $\rho<1$, $E[T-H\mid H=h]=(1-\rho)(\mu-h)$, positif sous la moyenne). Sur les échecs, une méthode différente de
HDBSCAN « sauve » donc des objets même sans être meilleure en moyenne ; sauvetages et pertes n'y mesurent pas une
supériorité.

**Redressement** (Horvitz-Thompson, poids 1 pour un échec, 260/20 = 13 pour un témoin ; **hypothèse** : les 20
témoins sont un tirage au hasard des 260, non vérifiable dans les archives). Niveau B, unité trame, $H^{r}_{k+1}$ −
HDBSCAN, IC bootstrap stratifié à 95 % :

| k | population du criblage (pondéré) | échecs seuls | témoins seuls |
| --- | --- | --- | --- |
| 2 | −0,0028 [−0,0056 ; −0,0003] | −0,0002 | −0,0032 |
| 3 | −0,0026 [−0,0069 ; +0,0008] | +0,0040 | −0,0036 |
| 5 | −0,0003 [−0,0056 ; +0,0036] | +0,0098 | −0,0019 |
| 10 | +0,0042 [+0,0012 ; +0,0070] | +0,0190 | +0,0020 |

Sur une trame typique de la séquence 08, l'avantage de niveau B de la tour est nul à k ≤ 5 (HDBSCAN légèrement
devant à k = 2) et de +0,004 à k = 10. Il se concentre sur les **objets minces** (vélo, moto, piéton, cycliste,
motard : +0,002 / +0,006 / +0,008 / +0,017 ; sauvetages 6/8/8/17 contre pertes 6/2/1/0) ; les véhicules sont neutres
ou un peu en dessous (−0,005 / −0,005 / −0,002 / +0,001). Ce sont des prédictions pour le test, pas des conclusions.

**Dépendance entre trames** (`variance_niveau_B.py`). Les 859 lignes instance × trame des voisines ne recouvrent que
74 instances physiques (identifiant d'instance persistant dans la séquence) ; corrélation des écarts d'une même
instance entre deux apparitions (k = 5) : +0,27 à une trame d'écart, +0,29 à 2–3, +0,21 à 4–8, +0,04 à 9–30, +0,11
au-delà (peu de paires). Effet de plan par instance jusqu'à 5,1 (voisines, k = 10). Le dédoublonnage par empreinte
des sites retire 2 échecs (= démos 01 et 03) et 1 voisine (000882 = démo 02) ; démos 01 et 04 sont la même trame sans
et avec sol.

**Plan LiDAR.**

1. **Test** : échantillon systématique de trames, **sans regarder HDBSCAN** : pas de 27 dans la séquence 08 (4 071
   trames, soit ≈ 150 trames), ou pas ≥ 30 sur plusieurs séquences si elles sont sur la VM ; toutes classes ; sol
   retiré par la même sonde gelée ; 150 trames visées (§ 4.6). Unité : la trame (moyenne de mIoU_h sur ses
   instances) ; IC aussi par grappes d'instances physiques.
2. **Dev** (réglage, estimation de σ) : 24 à 48 trames d'autres séquences (00–07, 09, 10) si elles sont sur la VM ;
   sinon séquence 08 décalée d'une demi-période (≈ 13 trames) par rapport au test (corrélation résiduelle des
   écarts d'une même instance ≈ 0,04 au-delà de 9 trames, déclarée).
3. **Strates pré-déclarées** : objets minces / véhicules ; portée ; contact (distance minimale à une autre instance
   < 0,3 m, champ du criblage). Aucune strate définie par le score de HDBSCAN.
4. **Descriptif seulement** : 5 démos, 37 échecs, 71 voisines, 20 témoins, rapportés par cohorte avec leur règle de
   choix ; démos objet par objet.
5. Le retrait du sol est une constante posée à la main (thèse de Zoltan, fait 3) : la démo 04 (avec sol) reste un
   témoin descriptif de ce choix.

### 3.4 Démos de Zoltan

L'exigence de l'utilisateur (« au moins un exemple de Zoltan/ où HDBSCAN échoue ») se lit au niveau C : un objet
d'une démo dont l'IoU de la sortie plate de la tour (configuration gelée par dev, sans regard sur les démos)
dépasse 1/2 alors que **toutes** les configurations de $\Theta_H$ au même k restent ≤ 1/2 (« sauvetage fort au
niveau C »). Rapport objet par objet, avec le niveau B à côté. Cinq trames choisies sur l'échec ne fondent aucune
supériorité (§ 6).

## 4. (c) Analyse, prédictions et règle de décision

### 4.1 Unité, estimande, tests

- **Unité** : la scène (synthétique) ; la trame (LiDAR, trames à pas ≥ 30).
- **Différence appariée** $D_s=\mathrm{mIoU}_h(T^{*},s)-\mathrm{mIoU}_h(H^{*},s)$, moyenne sur les mcs primaires.
- **Estimande** : $\bar\Delta=\frac{1}{C}\sum_c\bar D_c$, moyenne équipondérée des moyennes de cellule ; les cellules
  sont fixées par le plan (on ne généralise pas à d'autres familles).
- **Test primaire** : $t=\bar\Delta/\widehat{\mathrm{se}}$ avec $\widehat{\mathrm{se}}^{2}=\frac{1}{C^{2}}\sum_c s_c^{2}/R_c$
  (variance intra-cellule), loi de Student à $\sum_c(R_c-1)$ degrés de liberté, bilatéral, sens favorable exigé.
- **IC** : bootstrap stratifié (graines rééchantillonnées dans chaque cellule), en tirant $R_c-1$ valeurs parmi
  $R_c$ (correction de McCarthy–Snowden), 10 000 tirages, graine = sha256 du préenregistrement.
- **Contrôle publié** : retournement de signe des $D_s$ (100 000 tirages).

**Proposition B1 (correction du bootstrap).** Tirer $m$ valeurs avec remise parmi $R$ donne une moyenne de variance
$\frac{R-1}{R}\frac{s^{2}}{m}$ ; avec $m=R-1$, c'est $s^{2}/R$, la variance sans biais de la moyenne de cellule.
*Preuve.* La variance de la loi empirique est $\frac{1}{R}\sum(x_i-\bar x)^{2}=\frac{R-1}{R}s^{2}$. □

**Proposition B2 (retournement de signe conservateur sous effets de cellule hétérogènes).** Sous l'hypothèse nulle
faible $\bar\Delta=0$ avec des moyennes de cellule $\mu_c$ non nulles, la variance de référence du retournement,
$\sum_sw_s^{2}D_s^{2}$, a pour espérance $\sum_sw_s^{2}(\mu_{c(s)}^{2}+\sigma_{c(s)}^{2})\geq\sum_sw_s^{2}\sigma_{c(s)}^{2}=\mathrm{Var}(\bar\Delta)$ :
le test est asymptotiquement conservateur (puissance perdue, niveau tenu). □ Or la famille explique 15 % à 76 % de
la variance des écarts de niveau B (η², `variance_niveau_B.py`) : c'est le cas réel. D'où le test t intra-cellule
en primaire.

**Simulation** (`puissance.py`, loi des résidus du pilote, excès de kurtosis 19,5, effets de cellule d'écart-type 2δ) :
niveau réel du test t 0,042 à 0,058 ; du retournement de signe 0,015 à 0,049 (conservateur, B2) ; puissances au § 4.6.
Le dépassement le plus net (0,058) est à R = 2 : on prend R ≥ 3.

**Le niveau B ne prédit pas le niveau C** (`pilote_niveau_C.py`, HDBSCAN seul, n = 2 000). Pour HDBSCAN k = 5 contre
k = 2 : niveau B −0,040, niveau C +0,007 (mcs 10, EOM), **+0,038** (mcs 10, feuilles), **+0,070** (mcs 20, feuilles) ;
k = 10 contre k = 2 : −0,058 au niveau B, **+0,194** au niveau C (mcs 20, feuilles). Le signe s'inverse : aucune
supériorité plate ne se déduit des tableaux de niveau B du § 7 de la note. M3 ne donne qu'un plafond.

### 4.2 Effet de la sélection seule

Même arbre, deux sélections (pilote) : EOM contre feuilles à mcs 20 : +0,44 / +0,40 / +0,34 / +0,20 de mIoU_h
(k = 2, 3, 5, 10), écart-type par scène 0,29 à 0,33 ; mcs = k contre mcs 20 en EOM : −0,63 / −0,28 / −0,09 (k = 2, 3,
5 ; 540, 52 et 8 clusters en moyenne). La perte de la sélection (niveau B − meilleure sortie plate) vaut ≈ 0,16
(0,834 contre 0,672 à k = 2). L'effet de la tête est donc d'un ordre de grandeur au-dessus des écarts de niveau B
entre hiérarchies (0,008 à 0,078) : **le bras d'attribution A est obligatoire**, et toute phrase « la tour bat
HDBSCAN » doit dire ce qu'en fait la même tête sur l'arbre de HDBSCAN. (La v10 l'avait mesuré : l'exposant z seul,
sur l'arbre de HDBSCAN, donnait déjà +0,039 des +0,042 à +0,058 revendiqués.)

### 4.3 Prédictions à graver avant E1

Proposées ici, à recopier dans le préenregistrement avant toute exécution. Elles ne conditionnent pas la règle de
décision ; une prédiction ratée est publiée comme telle.

| # | Prédiction (n = 8 000 à 32 000 sauf mention) | Fondement |
| --- | --- | --- |
| P1 | Niveau B : $H^{r}_{k+1}$ − HDBSCAN > 0 à k = 3, 5, 10 à chaque taille (borne basse > 0), à ± 0,02 des valeurs de 8 000 (+0,026, +0,050, +0,078) ; à k = 2, \|Δ\| < 0,015 | session F |
| P2 | Niveau C, décomposition T* − H0 = (T* − T*₀) [complétion] + (T*₀ − A) [hiérarchie] + (A − H0) [tête] : à k = 5 et 10, le terme « hiérarchie » vaut au moins la moitié de T* − H0 ; à k = 2, \|T*₀ − A\| < 0,02 | niveau B ; v10 (z seul +0,039) |
| P3 | mcs = k : T* et `cover` sur-segmentent ; T* < H* à k = 2, 3 et 5 ; nombre de clusters de T* < celui de `cover` (qualification, prédiction E1 de l'auditeur) | v10 : 0,447 contre 0,580, 5 732 contre 867 clusters par trame |
| P4 | `hierarchical` : sans ε des deux côtés, H* bat T* ; avec ε des deux côtés, \|Δ\| < 0,02. `shells` : \|Δ\| < 0,02 | MAP ≈ 1 ; v10 0,98 contre 0,33 |
| P5 | T* > H* (moyenne des mcs primaires) à k = 5 et 10 avec Δ ≥ 0,02 ; strate de Bayes [0,90 ; 0,97[ la plus favorable | niveau B par strate (+0,075 à k = 5) |
| P6 | Regret (MAP − méthode, niveau C) de T* < celui de H* à k ≥ 3 | regret de niveau B 0,112 contre 0,162 à k = 5 |
| P7 | LiDAR, échantillon systématique, mcs ∈ {10, 20, 50}, k = 5 et 10 : non-infériorité (borne basse > −0,02) ; supériorité **seulement** sur la strate « objets minces » à k = 10 | redressement HT § 3.3 |
| P8 | LiDAR, mcs = k = 5 : T* < H* et ≥ 1,3 fois plus de clusters par trame | v10 (`cover`), P3 |
| P9 | Démo 04, vélo C (instance 56), k = 3 : sauvetage fort au niveau C par T* | niveau B 0,557 contre HDBSCAN ≤ 0,361 à tous les ordres |

### 4.4 Règle de décision (écrite d'avance)

Notations : $\bar\Delta_k$ = estimande du § 4.1 à l'ordre k sur les 144 cellules de test ; $\delta_{\min}=0{,}02$ (mIoU) ;
marge de non-infériorité 0,01 (synthétique), 0,02 (LiDAR).

| Niveau | Conditions (toutes requises) | Phrase autorisée |
| --- | --- | --- |
| R0 | défaut | « Aucune revendication de supériorité sur HDBSCAN. » |
| R1(k) | (i) test t intra-cellule : p < 0,05 bilatéral, sens favorable, **Holm sur les 4 ordres** ; (ii) $\bar\Delta_k\geq\delta_{\min}$ et borne basse de l'IC > 0 ; (iii) $\bar\Delta_{k,n}>0$ à chacune des trois tailles (aucune inversion à l'échelle) ; (iv) gardes : ni la PQ ni ARI_s significativement pires (p < 0,05 unilatéral, sans correction : volontairement sévère) ; (v) refus ou plantage de T* ≤ 1 % des scènes ; (vi) portes vertes au commit préenregistré (porte points, porte de sélection, porte ε sur G4, porte MAP) | « Sur le plan v11-E1 préenregistré <sha8>, à l'ordre k, la tour bat HDBSCAN (sklearn <version>, réglé sur dev à budget égal) de +x [a ; b] de mIoU un-à-un ; PQ … ; ARI_s … ; attribution : … » |
| R2 | conditions (i)–(vi) de R1 à k = 3, 5 et 10, la condition (i) prise sans Holm (intersection-union : chacune à 0,05) **et** non-infériorité à k = 2 (borne basse de $\bar\Delta_2$ > −0,01) | « La tour bat HDBSCAN sur le synthétique. » |
| R3 | R2, et aucune famille en perte significative (Holm sur 8 familles, à chaque k) | « … sur chacune des huit familles. » |
| Attribution | T*₀ − A significativement > 0 (même test) | sinon phrase obligatoire : « la même tête sur l'arbre de HDBSCAN fait aussi bien (Δ = … [..]) : le gain vient de la tête, pas de la tour » ; T* − T*₀ publié comme effet de la complétion |
| LiDAR-NI | borne basse de l'IC de la moyenne par trame > −0,02 à k = 5 et 10 (échantillon systématique) | « Sur LiDAR, la tour n'est pas inférieure à HDBSCAN (marge 0,02). » |
| LiDAR-S | LiDAR-NI, borne basse > 0 et Δ ≥ 0,01, **et** sauvetages > pertes au test binomial exact (une ligne par instance physique, première apparition), p < 0,05 | « Sur LiDAR, la tour bat HDBSCAN. » |
| Démos | descriptif | « Sauvetage fort au niveau C : <objets>. » (aucune généralisation) |

**Si HDBSCAN gagne** ($\bar\Delta_k$ significativement < 0, ou R1 refusé par une garde) : on publie « HDBSCAN bat la
tour à l'ordre k » ; conformément à la consigne du 28 septembre, c'est l'algorithme tiré de la tour qui est à revoir
depuis le modèle mathématique. Toute révision repasse par un nouveau dev et un **nouvel espace de test**
(`test2_v11e1`) ; le résultat du premier test n'est jamais retiré, et le nombre de tests exécutés est publié.

### 4.5 Contre le réglage sur le test

- Espaces de graines dev et test disjoints (dérivation sha256), trames LiDAR dev et test disjointes (§ 3.3) ;
  démos et cohortes descriptives jamais lues avant le préenregistrement.
- Choix sur dev, **la même procédure des deux côtés** : $J(\theta)$ = moyenne équipondérée des cellules de dev de
  mIoU_h, sous contrainte PQ ≥ (meilleure PQ de la grille) − 0,02 ; égalité à 0,002 près départagée par la
  configuration la plus simple (z = 1, ε = 0, sans complétion). $\lvert\Theta_T\rvert=\lvert\Theta_H\rvert$.
- Préenregistrement versionné (commit du développeur, pas de ce rapport) : configurations choisies, grilles,
  manifeste des scènes gelées (sha256), commit et binaires, versions Python épinglées, R, règle de décision,
  prédictions P1–P9. Le test s'exécute une fois ; une réexécution sur les mêmes graines n'est permise que si l'échec
  vient d'une porte présente au commit préenregistré (règle D11 de la v10, reprise parce qu'elle est saine).
- **Mesure de l'optimisme** (`optimisme_reglage.py`, pilote) : choisir la meilleure configuration et la réévaluer sur
  les mêmes scènes surestime en moyenne de +0,001 à +0,004 sur ces grilles (une configuration domine), mais l'écart
  dev − test d'une moitié de 64 scènes atteint +0,034 à +0,038 au 95ᵉ centile. *Preuve du signe* : la
  configuration choisie $\hat\jmath$ vérifie $E[\bar X_{\hat\jmath}^{\mathrm{dev}}]=E[\max_j\bar X_j^{\mathrm{dev}}]\geq\max_jE[\bar X_j]\geq E[\bar X_{\hat\jmath}^{\mathrm{test}}]$
  (convexité du maximum, puis indépendance de dev et test) : le biais n'est jamais négatif et grandit avec le nombre
  de configurations quasi équivalentes. □

### 4.6 Nombre de scènes

**Écarts-types du niveau B** (`variance_niveau_B.py`, session F, n = 8 000, $H^{r}_{k+1}$ − HDBSCAN, 64 scènes) :

| k | Δ moyen [IC 95 %] | sd par scène | sd intra-cellule (32 ddl) | η² famille |
| --- | --- | --- | --- | --- |
| 2 | +0,0082 [+0,0034 ; +0,0131] | 0,0199 | 0,0174 | 0,15 |
| 3 | +0,0262 [+0,0197 ; +0,0330] | 0,0270 | 0,0148 | 0,52 |
| 5 | +0,0500 [+0,0397 ; +0,0607] | 0,0444 | 0,0262 | 0,64 |
| 10 | +0,0783 [+0,0630 ; +0,0941] | 0,0645 | 0,0208 | 0,76 |

Corrélation des écarts par scène entre ordres : 0,90 (k = 5 et 10), 0,35 (k = 2 et 10). Grappe des objets dans une
scène : ICC 0,08 à 0,43, effet de plan 1,4 à 2,9 (raison de prendre la scène, pas l'objet, pour unité).

**Du niveau B au niveau C.** Pilote (HDBSCAN, deux min_samples, même sélection, 25 paires) : sd(niveau C) /
sd(niveau B) = **3,5** en médiane (1,5 à 7,5) ; en intra-cellule **4,6** (1,5 à 9,0) ; écarts de niveau C à queues
lourdes (quantiles 0 / 50 / 99 % : −0,60 / −0,005 / +0,27). **Hypothèse** de transfert : le même rapport vaut pour tour
contre HDBSCAN. D'où sd_C intra-cellule ≈ 4,6 × 0,015–0,026 = 0,07 à 0,12.

**Approximation normale** (`puissance.py`, puissance 0,9, δ = 0,02, α = 0,05 ; entre parenthèses Holm pire cas
α = 0,0125) : k = 2 : 168 (229) ; k = 3 : 122 (165) ; k = 5 : 382 (519) ; k = 10 : 240 (327) scènes au rapport
médian ; 13 à 1 985 sur l'étendue des rapports. **Simulation** (144 cellules, queues lourdes) :

| sd_C | R | scènes | puissance du test t | niveau réel |
| --- | --- | --- | --- | --- |
| 0,07 | 2 | 288 | 0,98 | 0,048 |
| 0,10 | 3 | 432 | 0,97 | 0,045 |
| 0,12 | 3 | 432 | 0,90 | 0,048 |
| 0,12 | 4 | 576 | 0,95 | 0,044 |
| 0,15 | 4 | 576 | 0,87 | 0,050 |
| 0,20 | 4 | 576 | 0,67 | 0,052 |

**Règle de taille, écrite d'avance** : R = 3 (432 scènes) par défaut ; après dev, si le sd_C intra-cellule mesuré à
8 000 et 16 000 (pire des quatre ordres) dépasse 0,12, R = 4 ; s'il dépasse 0,15, R = 4 et la revendication R1 ne vise
plus que δ = 0,03 (annoncé dans le préenregistrement) ; au-delà de 0,20, le plan est revu avant tout test.

**LiDAR** : sd par trame au niveau B 0,007–0,012 (témoins), 0,015–0,020 (échecs), 0,018–0,024 (voisines) ; au rapport
médian, 80 à 109 trames (type témoin) ou 222 à 302 (type échec) pour δ = 0,02 à puissance 0,9 (α = 0,05 ; 0,0125).
**150 trames** systématiques par défaut ; même règle de révision après dev.

### 4.7 Multiplicité

Primaire : 4 ordres (Holm) ; R2 en intersection-union. Secondaires, chacun avec sa propre correction de Holm : par
famille (8), par mcs (3 + mcs = k), par strate de Bayes (3), par taille (3), LiDAR par strate de classe (2).
Descriptif sans test : cohortes choisies, démos, bruit 0 % et 20 %, easy et extreme, témoins de la tour.

## 5. (d) Budget G4

**Repères mesurés** (`couts_g4.py`, session F, 24 scènes en parallèle, 48 vCPU AMD EPYC 9B45, 190 Go) :

| Étage | n = 2 000 | n = 8 000 (médiane ; p90) | exposant 2 000 → 8 000 |
| --- | --- | --- | --- |
| scène entière (un processus sous charge) | 11,7 s | 59,1 s ; 72 s | 1,17 |
| FULL natif (2 fils) | 4,6 s | 24,9 s | 1,22 |
| $H^{r}_{k+1}$ (Python, k = 10) | 0,8 s | 4,3 s | 1,22 |
| HDBSCAN (sklearn, k ≥ 3) | 0,045 s | 0,24 s | 1,22 |
| export (octets) | 38 Mo | 207 Mo | 1,21 |

LiDAR (voisines, 37 k à 82 k sites, médiane 70 k) : 270 s par trame (p90 308 s) ; HDBSCAN 74 s sur 4 ordres ;
`margin_r` 40 s ; FULL + export 77 s ; RSS maximale d'un processus 7,6 Go (démo 04, 126 k sites, k = 10).

**Modèle** (`budget_g4.py`, hypothèses à remesurer) : coût de scène = celui de F (la tête de niveau C remplace les
règles témoins retirées), facteur d'incertitude 1 à 1,5 ; exposant de taille 1,17 à 1,32 ; RSS ∝ incidences à
k = 10, soit ≈ 11 Go par processus à 32 000 sites, d'où 12 scènes de 32 000 en parallèle au plus ; par session
2 300 s utiles dont 420 s de portes (porte points 300 s, exigée par la campagne avant tout calcul ; porte de
sélection 120 s).

| Taille | coût par scène (s) | parallélisme | scènes par session (≈ 1 880 s de calcul) |
| --- | --- | --- | --- |
| 8 000 | 59 à 89 | 24 | ≈ 500 à 750 |
| 16 000 | 133 à 221 | 24 | ≈ 200 à 340 |
| 32 000 | 299 à 553 | 12 | ≈ 40 à 75 |

**Découpage** (emballage glouton, barrière entre dev et test) :

| Hypothèse | Sessions | Contenu |
| --- | --- | --- |
| basse (b = 1,17, ×1) | 6 | S1 dev (96 + 96 scènes à 8 000 et 16 000, sonde 8 scènes à 32 000, 24 trames dev) ; S2 test 8 000 + 16 000 + 24 à 32 000 ; S3 test 32 000 (72) ; S4 test 32 000 (48) + 51 trames ; S5 99 trames + cohortes descriptives ; S6 reliquat |
| haute (b = 1,32, ×1,5) | 9 | S1–S2 dev ; S3–S7 test synthétique (dont 4 sessions pour les 144 scènes de 32 000) ; S8–S9 LiDAR test et descriptif |

La session 1 commence par une **sonde de coût** (4 scènes à 16 000 et 4 à 32 000, toutes têtes) qui fixe l'exposant,
le facteur et la RSS ; le découpage du test est refait avec ces valeurs avant le préenregistrement. Chaque session
reste une session gardée de `gcp-migration/v11_session.py` (arrêt ciblé certifié, `TERMINATED` relu) : ce rapport
n'en lance aucune.

## 6. (e) Ce que le protocole ne peut pas établir

1. **Hors du plan, rien.** La conclusion vaut pour les huit familles du générateur v10, bruit 5 %, n = 8 000 à 32 000,
   quantification 18 bits, scikit-learn et numpy épinglés, sur G4. Elle ne dit rien d'autres densités ni de
   n = 64 000 et au-delà.
2. **Aucune propriété mathématique.** Laminarité, fidélité, stabilité en 3ε, cibles T0, Q1–Q4 et Q-Π2 relèvent des
   preuves et des fixtures ; un score d'IoU ne les teste pas (T0 est une porte, pas une mesure).
3. **Pas le chapitre 7.** La fraction récupérée avant fusion parasite, son déficit conditionnel (obstruction de Palm)
   et l'asymptotique à contraste fixé ne se lisent pas sur des nuages finis à densité fixée (E4 est une autre
   expérience).
4. **Pas d'équivalence.** « Pas de différence » ne s'énonce qu'en non-infériorité, à la marge déclarée.
5. **Attribution partielle.** Le bras A sépare hiérarchie et tête pour les têtes applicables aux deux arbres ; la
   complétion par lignée n'a pas d'analogue exact sur l'arbre de HDBSCAN : son effet est mesuré à part, pas attribué.
6. **LiDAR.** Une séquence (08) si les autres manquent sur la VM ; vérité d'instances SemanticKITTI (void, petites
   instances, bruit d'annotation) ; retrait du sol fixé à la main ; les cohortes choisies sur l'issue de HDBSCAN et
   les démos ne fondent aucune supériorité ; le redressement du § 3.3 repose sur une hypothèse de tirage des témoins.
7. **Le MAP n'est pas un plafond** (F7) : une méthode au-dessus du MAP n'est pas une anomalie ; le MAP suppose les
   vrais paramètres et la composition fixée.
8. **Coûts.** Ceux de la référence Python (pendaisons, tête) et du moteur au commit épinglé, en parallèle sur G4 : pas
   un temps produit ni la cible des 100 ms.
9. **Variance.** Le rapport niveau C / niveau B vient d'un pilote HDBSCAN à n = 2 000 ; la vraie variance tour contre
   HDBSCAN au niveau C n'existe qu'après la session dev. La puissance annoncée est conditionnelle à cette hypothèse.
10. **L'aval.** Un meilleur clustering plat ne dit rien du tokenizer de `Zoltan/FoundationModel` (qui consomme la
    hiérarchie entière, `MESURE.md` § 0.1 et § 3) ; un mcs choisi par la taille vraie des objets ne fonde rien.
11. **Machine.** Les étiquettes de sklearn dépendent de l'ordre des ex æquo, donc de la machine : les comparaisons ne
    valent que sur la même machine, et l'écart observé entre lectures binaire et multifusion (jusqu'à 0,41 sur une
    configuration) est publié avec elles.

## 7. Lecture critique

**Thèse.** Le § 4.4.4 prend l'excès de masse de HDBSCAN avec $\hat\lambda_x=1/r_x$ : en 3D, la densité K-NN varie en
$r^{-3}$, donc $1/r$ n'est pas une densité ; l'exposant est un paramètre de tête (z), à publier à z = 1 et à soumettre
au bras d'attribution. Le § 5.2 (« hacker » HDBSCAN) dit à raison que la sélection est séparable de l'arbre, mais
n'en tire aucun protocole : le § 5.3 invoque des données confidentielles, et le tableau 9.3 (une exécution par jeu,
`min_samples` et traitement du bruit non déclarés, relevé par la v10) n'établit pas de supériorité. Les masses
fractionnaires du § 9.1 changent le sens de mcs (T0 perdu à mcs 3) : le protocole fixe des masses entières.

**Auditeur.** Sa prédiction E1 porte sur le nombre de clusters ; réduire la sur-segmentation de `cover` ne suffit
pas à battre HDBSCAN en IoU (P3 et P8 le testent séparément). Les sauvetages LiDAR qu'il recoupe sont réels mais
mesurés sur des cohortes choisies sur l'échec de HDBSCAN ; redressés, ils disparaissent à k ≤ 5 (§ 3.3). Son
constat « le MAP n'est ni un concurrent ni un plafond d'IoU » est exact : F7 en est la fixture.

**v10.** (i) Sa métrique primaire, ARI_s, ne répond pas à « IoU de préférence » : elle devient une garde. (ii) Son
gain de niveau C (0,758–0,761 contre 0,703–0,716, soit +0,042 à +0,058) mêle tour et tête : z seul sur l'arbre de
HDBSCAN en donnait +0,039.
(iii) Sa règle D11 (test unique, `test2` sinon) et sa correction de McCarthy–Snowden (B1) sont saines et reprises.
(iv) Son intersection-union sur six adversaires est remplacée par deux adversaires (H* réglé, H0 par défaut) et un
bras d'attribution, plus lisibles. (v) Ses défauts trouvés dans sklearn (ε avec numpy ≥ 2.5, ex æquo dépendant de la
machine) sont confirmés ici et deviennent des portes.

**Note v11 (§ 7).** Le niveau B (meilleur bloc) ne prédit pas le signe du niveau C (§ 4.1) ; le bootstrap de
`points_summary.py` rééchantillonne les scènes sans strates (prudent, mais il mêle l'hétérogénéité des familles à
l'aléa) ; les cohortes LiDAR sont toutes choisies sur l'issue de HDBSCAN, et la phrase « la tour rattrape » devrait
porter cette réserve.

## 8. Points ouverts

- **Conjecture** : le rapport sd(niveau C)/sd(niveau B) mesuré sur des paires HDBSCAN transfère à la paire tour
  contre HDBSCAN (à remesurer en dev).
- **Hypothèse** : les 20 témoins du criblage sont un tirage au hasard des 260 trames sans échec.
- Les têtes $\Theta_T$ (existence des clusters P4, sélection, complétion) viennent des autres rapports du workflow ;
  ce protocole les prend comme une liste finie gelée avant dev.
- Porte ε et identité de la sélection commune à rejouer **sur G4** (environnement épinglé) avant le dev.

## 9. Reproduction

Depuis `build/v11-points-select/mesure/`, `python3 -B` (aucun bytecode écrit). Archives : `archives/` (extraites des
`results.tar.gz` du reçu `points_g4`, sessions D et F, et des manifestes). Temps total local < 6 min.

| Script | Sortie | Rôle | Statut |
| --- | --- | --- | --- |
| `metriques.py` | — | métriques d'une scène (bibliothèque) | — |
| `fixtures_metriques.py` | `fixtures_metriques.txt` | F1–F11 exactes | conforme, code 0 |
| `carte_map.py`, `controle_map.py` | `controle_map.txt` | rejeu, normalisation, coût et niveau de Bayes | conforme, code 0 |
| `resume_bayes.py` | `resume_bayes.txt` | tableau du niveau de Bayes | — |
| `variance_niveau_B.py archives` | `variance_niveau_B.txt` | variances G4, cohortes LiDAR, dépendance temporelle | — |
| `map_contre_niveau_B.py archives` | `map_contre_niveau_B.txt` | niveau B rapporté au MAP ; empreintes flottantes | — |
| `ponderation_criblage.py archives …/criblage_08.jsonl` | `ponderation_criblage.txt` | redressement des cohortes LiDAR | — |
| `pilote_niveau_C.py` | `pilote_niveau_C.txt`, `.json` | pilote de variance du niveau C (HDBSCAN, n = 2 000) | — |
| `accord_metriques.py` | `accord_metriques.txt` | accord des métriques | — |
| `optimisme_reglage.py` | `optimisme_reglage.txt` | biais du meilleur de la grille | — |
| `puissance.py` | `puissance.txt` | tailles d'échantillon, simulation | — |
| `selection_commune.py`, `porte_selection.py` | `porte_selection.txt` | sélection commune, identité sklearn, T0, mutants | conforme, code 0 |
| `ex_aequo.py` | `ex_aequo.txt` | effet des fusions ex æquo de HDBSCAN | — |
| `epsilon_sklearn.py` | `epsilon_sklearn_local.txt` | porte d'environnement ε | **refus local** (numpy 2.5.3), code 1 : attendu ; à passer sur G4 |
| `cout_ami.py` | `cout_ami.txt` | coût de AMI_s | — |
| `couts_g4.py archives`, `budget_g4.py` | `couts_g4.txt`, `budget_g4.txt` | coûts G4 et découpage en sessions | — |

**Limites.** Aucune mesure nouvelle de la tour : les nombres de la tour sont lus dans les archives G4 (niveau B). Les
nuages locaux sont petits (n ≤ 2 000), sauf la génération et le MAP (O(n × composantes), sans méthode de
clustering). Rien ici ne qualifie un statut public.
