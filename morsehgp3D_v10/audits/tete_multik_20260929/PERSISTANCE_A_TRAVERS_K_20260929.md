# Sélection par persistance à travers K : une tête multi-K sur la tour (29 septembre 2026)

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=benchmark_only (graines dev : réplique 0 = sous-ensemble commun des trois concepteurs ; réplique 1 = contrôle interne)
public_status=not_claimed
GCP non utilisé ; aucun fichier du dépôt modifié ; aucune graine test ni test_v10b
rôle : sélection par PERSISTANCE À TRAVERS K (un des trois concepteurs de tête multi-K)
```

## 0. Réponse courte

- **La persistance à travers K se mesure exactement, et pour presque rien, sur la tour.** Les images verticales
  (`lower`, puis ancêtre vivant au même rayon) donnent deux mesures sans constante cachée : (a) l'**identité** d'un
  amas à travers les ordres (mêmes cœurs de feuilles, égalité d'ensembles) et le nombre p d'ordres où il persiste ;
  (b) la **prominence verticale** VP : à rayon fixe, le seuil de densité de L_k(r) est proportionnel à k, donc la
  plage d'ordres où deux amas restent séparés mesure le rapport de densité cœur/col, **sans dimension ni exposant
  z**. Verticales : 0,6 s sur 30 s à 8 000 points ; appariement et VP : quelques millisecondes.
- **Stabilité bi-paramètre et antichaîne** : EOM sur l'arbre de référence T~_10 avec la somme, sur les ordres où
  l'amas garde ses cœurs, des stabilités EOM de l'amas apparié (λ_k = (r/s_k)^(−z), s_k rayon d'entrée médian de
  l'ordre ; poids w_k = k, w_k = 1 équivalent). **À z égal, elle bat la tranche** : sur 256 scènes dev (répliques
  0 et 1), +0,007 [+0,004 ; +0,011] avec remplissage b1.5 (39 gains, 6 pertes) et +0,012 [+0,008 ; +0,016] sans
  (42 / 3). Elle supprime la sur-segmentation des coquilles et des filaments, fragments qui n'existent qu'aux
  ordres hauts.
- **Elle ne bat pas la tranche réglée** : contre la meilleure configuration unique de v10-b (z = 5), +0,003 (IC
  contenant 0 ou y touchant) ; contre z réglé par taille, parité. Le regret du choix de z quand n change est divisé
  par deux, pas supprimé (réplique 1 : l'optimum reste z ≈ 3 à 2 000 et 6 à 8 000).
- **Tête sans z** (feuilles de l'arbre contracté aux séparations VP ≥ 1/√K*, seuil fixé d'avance) : à 0,003 de la
  meilleure configuration unique de la tranche sur 256 scènes, dans les deux sens ; à égalité avec la tranche réglée
  par taille sur la réplique 0, jusqu'à 0,014 en dessous sur la réplique 1. Elle remplace le cadran z par un seuil
  de bruit, sans gain.
- **Contre sklearn à min_samples = 10** : +0,033 [+0,015 ; +0,048] avec remplissage, +0,12 sans (comme v10-b).
  **KMAX = 5** : aucun gain (cinq crans verticaux, seuil trop grossier). **Stabilité vis-à-vis de K*** : pas
  améliorée (les candidats restent ceux d'une tranche).
- **À la remarque de l'utilisateur.** Les modes d'un mélange séparable sont des branches de la tranche (audit) ;
  il manquait un critère de significativité, et la persistance à travers K en fournit un, en unités de densité. Il
  retrouve sans réglage les modes nets (`spherical` à 0,026 du plafond de Bayes, `anisotropic` +0,028 à 8 000),
  mais **il ne peut ni créer les modes absents de la tranche** (gaussiennes larges d'`heteroscedastic`, amas sous
  mcs = √n d'`unbalanced`), **ni garder les modes plus faibles que le bruit de l'ordre 10** (rapport de densité
  < 1,4) : ceux-là, il les élague (`unbalanced` −0,03 à −0,05). À n ≤ 8 000 et K ≤ 10, un mode vrai faible et un
  fragment de filament ont le même profil (p, VP). Les identifier demande plus d'ordres (tester la persistance vers
  le HAUT), un autre mcs, et une affectation de la masse sous le col — pas une autre sélection.
- **Honnêteté du protocole** : la pondération des ordres par leur précision (w_k = k) a été ajoutée après coup ; la
  réplique 1 montre qu'elle n'apporte rien de mesurable (0,7921 contre 0,7924). Une correction de code a été faite
  en cours de route (élagage : contraction au lieu d'abandon du sous-arbre, § 3) ; les résultats publiés sont ceux
  de la version corrigée, l'ancienne est archivée dans `out/old/`.


## 1. Question

L'utilisateur : « la tour est censée identifier exactement les niveaux de densité K-NN ; si le mélange gaussien est
séparable (un mode par amas), on devrait les identifier ». L'audit du 29 septembre
(`morsehgp3D_v10/audits/audit_hierarchie_knn_20260929/AUDIT_HIERARCHIE_KNN_20260929.md`) a établi :

- la tranche d'ordre K de la tour est l'arbre plug-in exact de l'estimateur K-NN f̂_K ;
- la tête v10-b n'en lit qu'une tranche (entrée cover, condensation mcs = round(√n), EOM à λ = r^(−z), remplissage
  b(ρ)) ; à même entrée et même tête, la tour égale la hiérarchie d'HDBSCAN munie de l'entrée « bord » ;
- la sélection est le point faible : l'EOM dépend de z (cadran de granularité), l'optimum de z croît avec n
  (≈ 3–4 à 2 000 points, ≈ 6 à 8 000) et la tranche à K fixe est instable (coquilles retenues à K = 8, perdues à
  K = 10 à z = ẑ) ;
- le seul atout propre inexploité est l'AXE K : applications verticales L_K(r) ⊆ L_(K−1)(r), stabilité de la
  bifiltration (Blumberg–Lesnick), instabilité des tranches (Rolle–Scoccola).

Mon rôle : pour chaque amas candidat, mesurer s'il subsiste aux autres ordres, définir une stabilité
bi-paramètre et choisir une antichaîne qui la maximise ; sans oracle, sans réglage par scène sur la vérité, en
fixant d'avance toute constante nécessaire.

## 2. Fondement

Étiquettes : [V] lu dans le texte (copies locales de l'audit, `v10-persist/audit_hier/litterature_statistique/pdf/`),
[D] dérivation de ma part, [E] mesure de ce travail (dev).

### 2.1 L'objet : une bifiltration et ses verticales

- L_k(r) = {y : |B̄(y, r) ∩ X| ≥ k} croît avec r et décroît avec k. La tour en donne, pour chaque ordre k ≤ KMAX,
  l'arbre de fusion T_k (composantes de L_k(r) quand r croît) et l'application verticale `lower` : le nœud d'ordre
  k−1 vivant au niveau de création d'un nœud d'ordre k, qui le contient.
- **Images à rayon fixe [D].** L'image d'une composante de L_K(r) dans L_k(r), k < K, s'obtient en composant les
  `lower` puis en remontant, à chaque ordre, jusqu'à l'ancêtre vivant au rayon r. C'est bien défini parce que les
  inclusions commutent : la composante de L_(k−1)(r) qui contient celle de L_k(r) contient aussi son état antérieur.
- **Monotonie [D].** À r fixe, si deux composantes de L_K(r) ont la même image à l'ordre k'', elles ont la même
  image à tout ordre k' < k'' (l'application L_K → L_k' se factorise par L_k''). Les ordres où deux amas restent
  séparés à rayon fixe forment donc un intervalle [k_min, K].
- **Stabilité de la bifiltration [V, relu ici dans la copie locale].** Blumberg–Lesnick (FoCM 24(2), 2024,
  th. 1.6(i) et th. 3.1) : pour les mesures normalisées, B(μ)_(k, r) ⊆ B(η)_(k−δ, r+δ) dès que δ > d_Pr(μ, η) ;
  la multicouverture est donc 1-entrelacée en distance de Prohorov, le décalage portant À LA FOIS sur le rayon et
  sur la masse k/n. Une tranche à k fixe n'hérite d'aucune stabilité, puisque l'entrelacement déplace k
  [D, audit § 1.5].
- **Instabilité des tranches [V, lu par l'audit].** Rolle–Scoccola (JMLR 25(258), 2024) : la liaison simple
  robuste à κ fixé est discontinue (prop. 44) et passer de κ à κ′ peut changer arbitrairement le résultat
  (prop. 45) ; les tranches linéaires de pente négative sont stables (résultat B). L'audit l'a mesuré sur les
  coquilles (K = 8 contre 10).
- **Conséquence [D].** Une scission (deux amas séparés) qui n'existe que sur une plage d'ordres de largeur Δk n'est
  protégée par l'entrelacement que contre des perturbations de Prohorov de masse inférieure à ≈ Δk/(2n) : au-delà,
  déplacer quelques points peut la créer ou la détruire. Une scission qui subsiste sur une large plage d'ordres ET
  de rayons est robuste au sens de l'entrelacement. C'est la motivation de la persistance à travers K. Ce n'est
  PAS un théorème de sélection : aucun article lu ne dit quelle antichaîne choisir sur une bifiltration.

### 2.2 Deux mesures de persistance à travers K

**(a) Identité par les cœurs (existence).** Soit T~_K* l'arbre condensé de référence (K* = KMAX, mcs = round(√n)).
Le cœur d'une feuille condensée est le nœud du bas de sa chaîne, pris à son propre niveau. Son image verticale à
l'ordre k tombe dans un amas condensé de T~_k. Un amas c de T~_K* **persiste à l'ordre k** s'il existe un amas C
de T~_k dont le sous-arbre reçoit EXACTEMENT les images des cœurs des feuilles de c (égalité d'ensembles). C'est un
appariement canonique, sans seuil, qui conserve l'identité d'un amas à travers les ordres ; le compte
p(c) = |{k ≤ K* : c persiste à l'ordre k}| mesure l'étendue verticale de l'amas en tant qu'amas.

**(b) Prominence verticale à rayon fixe (séparation).** Pour un enfant c d'une scission de T~_K*, on prend le plus
bas rayon ρ(c) où c et au moins un frère existent encore comme gros amas d'ordre K* (le cœur du plus faible des
deux). À ce rayon FIXE on descend les ordres : c reste séparé à l'ordre k si aucune image d'un gros amas vivant de
son sous-arbre n'est l'image d'un gros amas vivant du sous-arbre d'un frère. Par monotonie, les ordres séparés
forment un intervalle [k_min(c), K*] ; on pose VP(c) = log(K*/k_min(c)) ∈ [0 ; log K*].

**Lemme (l'axe vertical est un axe de densité sans exposant) [D].** À r fixe,
L_k(r) = {y : f̂_r(y) ≥ k/(n v_d r^d)} où f̂_r(y) = |B̄(y, r) ∩ X|/(n v_d r^d) est l'estimateur à noyau boule de
largeur r. Le seuil de densité est LINÉAIRE en k : le rapport de deux seuils au même rayon vaut k/k′, sans
dimension ni exposant. Dans le modèle idéal (f̂_r ≈ f), si c est séparé de son frère par un col de densité f_col
et si ρ est le rayon où son cœur tient encore mcs points, alors k_min ≈ K* f_col / f_ρ et
VP ≈ min(log(f_ρ / f_col), log K*). VP est donc la **prominence en unités de densité** (écart en log-densité entre
le cœur du plus faible des deux enfants et le col, l'analogue de la persistance de mode de ToMATo), mesurée sans
choisir d ni z. En comparaison, la durée de vie horizontale en log r vaut
(1/m) log(f_ρ / f_col) avec m la dimension intrinsèque locale : c'est elle que l'EOM à λ = r^(−z) pondère, d'où
le cadran z.

**Seuil a priori.** La fluctuation relative de f̂_K est ≈ 1/√K (Moore–Yackel ; en log, √ψ′(K) = 0,32 à K = 10).
Une séparation dont la prominence verticale est inférieure à UN écart-type du bruit ponctuel de l'ordre de
référence n'est pas significative : τ = 1/√K*, fixé une fois pour toutes, soit k_min ≤ 7 à K* = 10 et k_min ≤ 3
à K* = 5. Ce seuil calibre le bruit ponctuel, pas celui d'un amas (réserve de l'audit, § 1.6) ; sa sensibilité
est publiée (§ 4).

### 2.3 Trois têtes (antichaînes sur T~_K*)

1. **EOM bi-paramètre (`eom2`, `eom2w`).** S2(c) = Σ_(k : c persiste) w_k S_k(C_k(c)), où S_k est la stabilité
   EOM de l'amas apparié à l'ordre k, avec λ_k = (r/s_k)^(−z) et s_k le rayon d'entrée (cover) médian de l'ordre
   k. C'est l'excès de masse intégré sur l'axe de masse de la bifiltration, restreint à la région où l'amas garde
   son identité (mêmes cœurs). La normalisation s_k rend les ordres commensurables sans choisir de dimension
   (dans le modèle, s_k ∝ k^(1/m) ; à z = m, λ_k est un seuil de densité). Poids : `eom2` w_k = 1 (mesure de
   comptage) ; `eom2w` w_k = k, l'inverse de la variance relative de f̂_k (Moore–Yackel : √k (f̂_k − f)/f → N(0, 1) ;
   sous approximation poissonnienne Var(log f̂_k) = ψ′(k) ≈ 1/(k − 1/2)) : une combinaison des ordres à la
   précision de chacun, sans constante libre. C'est une motivation, pas un théorème : les stabilités des
   différents ordres ne sont pas des estimations indépendantes d'une même quantité. L'antichaîne maximise Σ S2
   (EOM de Campello, racine exclue). Il reste z. **`eom2w` a été introduit APRÈS avoir vu les résultats de `eom2`** (même
   dev) : c'est un choix post hoc, contrôlé sur la réplique 1 du dev (§ 4.8).
2. **Élagage par persistance puis EOM (`vp_eom`, `pc_eom`).** Un amas non significatif (VP < τ, ou p(c) < θ) est
   CONTRACTÉ : ses points et sa stabilité passent à son plus proche ancêtre significatif, et ses descendants
   significatifs sont rattachés à cet ancêtre (on retire le nœud, pas le sous-arbre). Un nœud qui ne garde qu'un
   enfant significatif se prolonge en lui (chaîne fusionnée, stabilités additionnées : le télescopage des
   contributions EOM est exact). EOM ensuite sur l'arbre contracté. Il reste z.
3. **Feuilles persistantes, sans z (`vp_leaf`, `pc_leaf`).** Les modes significatifs = les feuilles de l'arbre
   contracté aux séparations significatives (VP ≥ τ) ou aux amas persistants (p(c) ≥ θ = ⌈K*/2⌉, majorité des
   ordres). C'est la réponse la plus littérale à la remarque de l'utilisateur : un mélange séparable a un mode par
   amas, et les modes sont les feuilles significatives de l'arbre de densité.

Constantes : mcs = round(√n) (inchangée, commune à toutes les têtes), K* = KMAX, τ = 1/√K*, θ = ⌈K*/2⌉ ; aucune
n'est réglée sur la vérité. Pour les têtes à z, la grille est celle de v10-b ({3, 4, 5, 6, 8}) et le choix se fait
comme pour v10-b (argmax de J sur dev), en publiant toute la grille.

## 3. Prototype

Dossier : `/workspaces/E-HGP/build/v10-persist/multik/persistance_k/` (hors dépôt, Python 3.12, numpy/scipy, un fil).

| Fichier | Rôle |
| --- | --- |
| `cache_towers.py` | sous-ensemble dev (`run_campaign.plan('dev', [2000, 8000], 2)`, réplique 0 ou 1 par `--replicate`), tour figée `v10-dev-multik/mhgp10_tower --k=10 --entry=cover --threads=1 --dump`, lecture du dump en tableaux compacts (npz, hors dépôt dans le scratchpad) |
| `multik.py` | lecture du cache ; masse des sous-arbres (système triangulaire creux) ; condensation N-aire HDBSCAN par ordre (`Condensed`) ; stabilité EOM et sélection EOM/feuilles de v10-b ; images verticales à rayon fixe (`MultiK.image`, sauts par l'arbre condensé) ; images des cœurs et ensembles de feuilles |
| `persist_head.py` | appariement par ensembles de cœurs et compte p(c) ; `s2` (EOM bi-paramètre) ; `split_prominence` (prominence verticale VP) ; `Pruned` (arbre élagué, EOM et feuilles) |
| `validate_head.py` | port contre la tête de référence `zhead_dev.cluster` (égale au binaire) et contre les CSV dev |
| `eval_persist.py`, `eval_extra.py`, `eval_r1.py` | évaluation ARI_s (sans remplissage et b1.5, k = max(K*, 5) comme le banc) : toutes les têtes (réplique 0), poids des ordres (réplique 0), contrôle (réplique 1) |
| `stability_k.py` | stabilité de la sélection entre K* = 10, 8 et 5 |
| `analyse.py`, `summarize.py`, `paired.py` | J pondéré par cellule, bases v10-b et sklearn lues dans les CSV dev existants (mêmes graines), écarts appariés avec IC bootstrap |
| `timing.py` | coût de la tête (Python) |
| `explore_*.py` | diagnostics lisibles (arbre condensé annoté : vérité, p(c), durées de vie par ordre, VP) |
| `out/` | CSV de résultats (`eval_v3_*`, `extra_*`, `r1_*`, `stab_*`), tableaux Markdown ; `out/old/` : première version de l'élagage (abandon du sous-arbre, corrigée) |

Les tours en cache (256 scènes, 4,2 Go) sont hors du dépôt et hors de `/workspaces` (scratchpad de session, sur
`/tmp`) : elles se reconstruisent par `cache_towers.py` (≈ 40 min à deux processus d'un fil).

**Validation du port.** Sur 22 scènes de 2 000 points, à K = 3 et 10 et z = 3 et 6 : 0 écart de partition sur
44 comparaisons avec `zhead_dev.cluster` (qui égale le binaire C++), et |ARI_s − CSV| ≤ 5·10⁻⁷ (arrondi du CSV). Sur
le sous-ensemble commun, la tête « une tranche » du prototype redonne exactement les J de v10-b des CSV
`kcover_dev_zgrid*.csv` (§ 4).

**Algorithme (par scène).**
1. Pour chaque ordre k = 1..K*, masse des nœuds, gros nœuds (masse ≥ mcs), chaînes condensées, niveau de sortie de
   chaque point (entrée propre s'il est attaché à un gros nœud, sinon niveau du gros nœud où son petit sous-arbre
   fusionne) : exactement la condensation de v10-b.
2. Images verticales : `lower`, puis ancêtre vivant au même rayon ; les chaînes de gros nœuds se franchissent par
   recherche dichotomique dans l'arbre condensé.
3. Cœurs des feuilles de T~_K* → amas de T~_k → ensembles Λ_k(C) → appariement c ↔ C_k(c) et compte p(c).
4. VP(c) par descente des ordres à rayon fixe, avec arrêt à la première collision (monotonie).
5. Sélection (EOM bi-paramètre, élagage + EOM, feuilles), étiquettes, remplissage borné b(1,5) éventuel.

**Correction en cours de route.** La première version de l'élagage abandonnait tout le sous-arbre d'un amas non
significatif, y compris ses descendants significatifs : un regroupement intermédiaire non persistant emportait les
vraies gaussiennes (J = 0,41–0,44 pour l'élagage par p). La version publiée contracte le nœud ; contrôle : avec tous
les amas gardés, l'arbre contracté redonne exactement l'EOM de la tranche (3 scènes, z = 3 et 6).

## 4. Résultats (dev commun : 128 scènes, réplique 0)

Plan : `run_campaign.plan('dev', [2000, 8000], 2)`, réplique 0 : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n
{2 000 ; 8 000}, une scène par cellule, donc J (moyenne pondérée par cellule de l'ARI_s) est la moyenne simple.
KMAX = 10 (tour figée `v10-dev-multik`, entrée cover, un fil ; 0 refus sur 128). Bases lues dans les CSV dev
existants, mêmes graines : v10-b = `kcover_dev_zgrid*.csv` (`tower_cover`, K = 10) ; sklearn = `kcover_dev_k10cap.csv`
(`hdb`, min_samples = 10). Le prototype redonne exactement les J de v10-b (lignes « une tranche »). Remplissage
b(1,5) à k = max(K, 5), comme le banc.

### 4.1 Tableau principal (K* = 10)

| Tête | Rempl. | n = 2 000 | n = 8 000 | Tout |
| --- | --- | ---: | ---: | ---: |
| v10-b une tranche, z = 3 | sans | 0,7741 | 0,7508 | 0,7624 |
| v10-b une tranche, z = 4 | sans | 0,7736 | 0,7595 | 0,7666 |
| v10-b une tranche, z = 5 (choix lot C sans remplissage) | sans | 0,7679 | 0,7594 | 0,7636 |
| v10-b une tranche, z = 6 | sans | 0,7544 | 0,7621 | 0,7583 |
| v10-b une tranche, z = 3 | b1.5 | 0,7782 | 0,7691 | 0,7737 |
| v10-b une tranche, z = 4 | b1.5 | 0,7795 | 0,7826 | 0,7810 |
| v10-b une tranche, z = 6 (choix lot C) | b1.5 | 0,7649 | 0,7901 | 0,7775 |
| v10-b une tranche, z = 8 | b1.5 | 0,7544 | 0,7651 | 0,7598 |
| EOM bi-paramètre w = 1, z = 6 | sans | 0,7755 | 0,7636 | 0,7696 |
| EOM bi-paramètre w = 1, z = 6 | b1.5 | 0,7824 | 0,7862 | 0,7843 |
| EOM bi-paramètre w = 1, z = 8 | b1.5 | 0,7653 | 0,7914 | 0,7783 |
| **EOM bi-paramètre w = k, z = 6** (post hoc) | sans | 0,7708 | **0,7697** | **0,7702** |
| **EOM bi-paramètre w = k, z = 6** (post hoc) | b1.5 | 0,7787 | **0,7942** | **0,7865** |
| EOM bi-paramètre w = k, z = 5 | b1.5 | 0,7818 | 0,7800 | 0,7809 |
| élagage VP (τ = 1/√K*) + EOM z = 6 | sans | 0,7762 | 0,7673 | 0,7718 |
| élagage VP (τ = 1/√K*) + EOM z = 6 | b1.5 | 0,7797 | 0,7843 | 0,7820 |
| **feuilles VP, τ = 1/√K*, sans z** | sans | 0,7753 | 0,7622 | 0,7687 |
| **feuilles VP, τ = 1/√K*, sans z** | b1.5 | 0,7791 | 0,7847 | 0,7819 |
| feuilles persistantes p ≥ 5, sans z | sans | 0,7709 | 0,7568 | 0,7639 |
| feuilles persistantes p ≥ 5, sans z | b1.5 | 0,7678 | 0,7645 | 0,7662 |
| élagage p ≥ 5 + EOM z = 6 | b1.5 | 0,7654 | 0,7634 | 0,7644 |
| feuilles condensées (HDBSCAN leaf, une tranche) | b1.5 | 0,6863 | 0,5955 | 0,6409 |
| sklearn ms = 10, EOM α = 1 | sans | 0,6475 | 0,6550 | 0,6513 |
| sklearn ms = 10, feuilles α = 2 | b1.5 | 0,7635 | 0,7445 | 0,7540 |
| sklearn ms = 10, feuilles α = 2 (b2, meilleur remplissage de sklearn) | b2 | 0,782 | 0,776 | 0,779 |

Grilles complètes (z ∈ {3, 4, 5, 6, 8} pour chaque famille de têtes, K* = 5) : `out/table_main_k10.md`,
`out/eval_v3_*.csv`, `out/extra_*.csv`.

### 4.2 Écarts appariés (même scène), IC bootstrap à 95 % (20 000 tirages des scènes)

| Tête | Base | Taille | Écart | IC 95 % | Gains / pertes |
| --- | --- | --- | ---: | --- | --- |
| EOM bi-param. w = k, z = 6, b1.5 | v10-b z = 6, b1.5 (même z) | tout | +0,0090 | [+0,0050 ; +0,0135] | 19 / 1 |
| EOM bi-param. w = k, z = 6, sans | v10-b z = 6, sans (même z) | tout | +0,0120 | [+0,0070 ; +0,0178] | 20 / 0 |
| EOM bi-param. w = k, z = 6, b1.5 | v10-b z = 6, b1.5 | 8 000 | +0,0041 | [+0,0003 ; +0,0088] | 5 / 1 |
| EOM bi-param. w = k, z = 6, b1.5 | v10-b z = 4, b1.5 (meilleur z à 2 000) | 2 000 | −0,0007 | [−0,0065 ; +0,0048] | 5 / 5 |
| EOM bi-param. w = k, z = 6, sans | v10-b z = 5, sans (choix lot C) | tout | +0,0066 | [+0,0010 ; +0,0143] | 11 / 2 |
| EOM bi-param. w = 1, z = 6, sans | v10-b z = 6, sans | tout | +0,0113 | [+0,0017 ; +0,0201] | 21 / 1 |
| feuilles VP (sans z), sans | v10-b z = 3, sans (meilleur z à 2 000) | 2 000 | +0,0012 | [−0,0119 ; +0,0165] | 6 / 13 |
| feuilles VP (sans z), sans | v10-b z = 6, sans (meilleur z à 8 000) | 8 000 | +0,0001 | [−0,0092 ; +0,0096] | 10 / 14 |
| feuilles VP (sans z), b1.5 | v10-b z = 4, b1.5 | tout | +0,0009 | [−0,0089 ; +0,0123] | 16 / 26 |
| feuilles VP (sans z), b1.5 | v10-b z = 6, b1.5 | 8 000 | −0,0054 | [−0,0170 ; +0,0061] | 9 / 15 |
| élagage VP + EOM z = 6, sans | v10-b z = 6, sans | tout | +0,0135 | [+0,0064 ; +0,0212] | 27 / 19 |
| EOM bi-param. w = k, z = 6, b1.5 | sklearn ms = 10 feuilles α = 2, b1.5 | tout | +0,0325 | [+0,0151 ; +0,0484] | 97 / 29 |
| feuilles VP (sans z), sans | sklearn ms = 10 EOM α = 1, sans | tout | +0,1175 | [+0,0753 ; +0,1611] | 89 / 31 |

Lecture :
- **À z égal, l'EOM bi-paramètre domine presque la tranche** : 20 gains pour 0 ou 1 perte sur 128 scènes. Les
  gains sont des sur-segmentations supprimées : coquilles (11–13 → 8 amas), filaments (14–17 → 8–12),
  `anisotropic` medium (10 → 8), `bridge` extreme (9 → 8). La seule perte (w = k) est `unbalanced` extreme à
  8 000 (3 → 2 amas, −0,035). Sans pondération (w = 1), une perte grave : `filaments` extreme à 8 000 (13 → 2 amas,
  −0,39), le parent accumulant la stabilité des ordres bas où ses enfants n'existent pas (§ 5.3).
- **Contre la tranche à son meilleur z par taille, parité** : aucun écart significatif. Sur cette réplique, un seul z
  (= 6) convient aux deux tailles pour la tête bi-paramètre (perte de 0,003 à 2 000 contre son propre optimum),
  alors que la tranche perd 0,015 à 2 000 avec z = 6 et 0,0075 à 8 000 avec z = 4 ; la réplique 1 ne le confirme
  qu'à moitié (§ 4.8).
- **La tête sans z (feuilles VP) égale la tranche à son meilleur z à chaque taille** sur cette réplique (écarts de
  +0,000 à +0,001 sans remplissage ; −0,005 à 8 000 avec remplissage, non significatif) : la persistance à travers
  K remplace le cadran z et sa dépendance à n, sans rien gagner de plus. Sur la réplique 1, elle est 0,007 sous la
  tranche réglée à 2 000 (§ 4.8).

### 4.3 Par famille (K* = 10)

n = 2 000 :

| Tête | Rempl. | sph | ani | het | unb | she | bri | hie | fil |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| v10-b z = 4 | b1.5 | 0,921 | 0,856 | 0,673 | 0,736 | 0,936 | 0,902 | 0,467 | 0,744 |
| v10-b z = 6 | b1.5 | 0,921 | 0,849 | 0,673 | 0,736 | 0,882 | 0,899 | 0,467 | 0,692 |
| EOM bi-param. w = k, z = 6 | b1.5 | 0,921 | 0,858 | 0,673 | 0,736 | 0,941 | 0,902 | 0,467 | 0,731 |
| feuilles VP (sans z) | b1.5 | 0,921 | 0,852 | 0,664 | 0,705 | 0,949 | 0,899 | 0,474 | 0,769 |
| sklearn feuilles α = 2 | b1.5 | 0,840 | 0,785 | 0,642 | 0,691 | 0,981 | 0,851 | 0,573 | 0,744 |

n = 8 000 :

| Tête | Rempl. | sph | ani | het | unb | she | bri | hie | fil |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| v10-b z = 4 | b1.5 | 0,921 | 0,827 | 0,701 | 0,787 | 0,964 | 0,902 | 0,463 | 0,696 |
| v10-b z = 6 | b1.5 | 0,921 | 0,827 | 0,701 | 0,787 | 0,940 | 0,902 | 0,463 | 0,779 |
| EOM bi-param. w = k, z = 6 | b1.5 | 0,921 | 0,827 | 0,701 | 0,783 | 0,964 | 0,902 | 0,463 | 0,793 |
| feuilles VP (sans z) | b1.5 | 0,921 | 0,855 | 0,681 | 0,737 | 0,952 | 0,902 | 0,463 | 0,766 |
| sklearn feuilles α = 2 | b1.5 | 0,860 | 0,803 | 0,696 | 0,708 | 0,827 | 0,857 | 0,437 | 0,768 |

Sans remplissage :

| Tête | n | sph | ani | het | unb | she | bri | hie | fil |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| v10-b z = 3 | 2 000 | 0,890 | 0,822 | 0,701 | 0,772 | 0,954 | 0,890 | 0,460 | 0,703 |
| v10-b z = 6 | 2 000 | 0,890 | 0,812 | 0,701 | 0,772 | 0,874 | 0,886 | 0,460 | 0,640 |
| EOM bi-param. w = k, z = 6 | 2 000 | 0,890 | 0,823 | 0,701 | 0,772 | 0,940 | 0,890 | 0,460 | 0,690 |
| élagage VP + EOM z = 6 | 2 000 | 0,890 | 0,815 | 0,698 | 0,765 | 0,956 | 0,886 | 0,469 | 0,732 |
| feuilles VP (sans z) | 2 000 | 0,890 | 0,815 | 0,698 | 0,765 | 0,949 | 0,886 | 0,469 | 0,732 |
| sklearn EOM α = 1 | 2 000 | 0,529 | 0,581 | 0,647 | 0,659 | 0,974 | 0,594 | 0,645 | 0,551 |
| v10-b z = 3 | 8 000 | 0,877 | 0,794 | 0,705 | 0,745 | 0,967 | 0,803 | 0,466 | 0,648 |
| v10-b z = 6 | 8 000 | 0,877 | 0,789 | 0,705 | 0,745 | 0,941 | 0,862 | 0,447 | 0,731 |
| EOM bi-param. w = k, z = 6 | 8 000 | 0,877 | 0,789 | 0,705 | 0,765 | 0,967 | 0,862 | 0,447 | 0,745 |
| élagage VP + EOM z = 6 | 8 000 | 0,877 | 0,794 | 0,703 | 0,755 | 0,957 | 0,862 | 0,447 | 0,744 |
| feuilles VP (sans z) | 8 000 | 0,877 | 0,797 | 0,703 | 0,755 | 0,952 | 0,862 | 0,447 | 0,705 |
| sklearn EOM α = 1 | 8 000 | 0,537 | 0,646 | 0,664 | 0,664 | 0,981 | 0,602 | 0,589 | 0,557 |

La tête bi-paramètre prend, famille par famille, le meilleur des deux
z de la tranche (coquilles comme z = 4, filaments mieux que z = 6). Les feuilles VP gagnent sur `anisotropic`
(+0,028 à 8 000), coquilles et filaments à 2 000, et perdent sur `unbalanced` (−0,03 à −0,05) et
`heteroscedastic` (−0,01 à −0,02).

### 4.4 Mélanges gaussiens et plafond de Bayes (réponse directe à la remarque)

Plafond = ARI_s de la partition MAP du mélange ; « meilleure coupe » = meilleure coupe horizontale de la tranche
K = 10 (reçu `bench_dev_objet_20260929`, `gauss_ceiling_dev.csv`), mêmes graines ; remplissage b1.5.

| Famille | n | Plafond | Meilleure coupe | v10-b z = 6 | v10-b z = 4 | Feuilles VP | EOM bi-param. w = k, z = 6 | sklearn feuilles α = 2 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| spherical | 2 000 | 0,947 | 0,843 | 0,921 | 0,921 | 0,921 | 0,921 | 0,840 |
| spherical | 8 000 | 0,948 | 0,828 | 0,921 | 0,921 | 0,921 | 0,921 | 0,860 |
| anisotropic | 2 000 | 0,950 | 0,757 | 0,849 | 0,856 | 0,852 | 0,858 | 0,785 |
| anisotropic | 8 000 | 0,942 | 0,746 | 0,827 | 0,827 | 0,855 | 0,827 | 0,803 |
| heteroscedastic | 2 000 | 0,792 | 0,706 | 0,673 | 0,673 | 0,664 | 0,673 | 0,642 |
| heteroscedastic | 8 000 | 0,796 | 0,703 | 0,701 | 0,701 | 0,681 | 0,701 | 0,696 |
| unbalanced | 2 000 | 0,882 | 0,788 | 0,736 | 0,736 | 0,705 | 0,736 | 0,691 |
| unbalanced | 8 000 | 0,884 | 0,758 | 0,787 | 0,787 | 0,737 | 0,783 | 0,708 |

- `spherical` : toutes les têtes de la tour sont à 0,026 du plafond, identiques ; la persistance n'a rien à
  corriger.
- `anisotropic` : l'écart au plafond (0,09–0,12) est un effet de niveau d'ensemble (masse sous le col) et
  d'affectation ; les feuilles VP le réduisent à 8 000 (+0,028).
- `heteroscedastic` et `unbalanced` : l'écart vient de modes ABSENTS ou FAIBLES dans la tranche : les gaussiennes
  larges (σ = 2,5) ne forment pas de branche à K = 10 (diagnostic `explore_vp.py`, § 5.2) ; les petits amas
  (poids 1/65 à 4/65 : 30 à 123 points à 2 000 pour mcs = 45, 123 à 492 points à 8 000 pour mcs = 89) sont sous ou
  près de mcs, ou séparés de leur gros voisin par un rapport de densité ≈ 1,1.
  La persistance à travers K ne crée pas de branche : elle ne peut qu'en retirer. Elle retire justement ces modes
  faibles (VP = log(10/9) = 0,11 pour la gaussienne 2 de `unbalanced` hard à 8 000).

### 4.5 Sensibilité aux constantes (K* = 10, J sur les 128 scènes)

| Seuil | Feuilles VP, sans | Feuilles VP, b1.5 | VP + EOM z = 6, sans | VP + EOM z = 6, b1.5 |
| --- | ---: | ---: | ---: | ---: |
| k_min ≤ 5 (τ = 0,69) | 0,7584 | 0,7542 | 0,7584 | 0,7542 |
| k_min ≤ 6 (τ = 0,51) | 0,7676 | 0,7701 | 0,7678 | 0,7699 |
| **k_min ≤ 7 (τ = 1/√10, a priori)** | **0,7687** | **0,7819** | **0,7718** | **0,7820** |
| k_min ≤ 8 (τ = 0,22) | 0,7409 | 0,7650 | 0,7635 | 0,7780 |
| k_min ≤ 9 (τ = 0,11) | 0,6598 | 0,7153 | 0,7578 | 0,7741 |

| Seuil de persistance p (appariement exact) | Feuilles, sans | Feuilles, b1.5 |
| --- | ---: | ---: |
| p ≥ 3 | 0,7359 | 0,7644 |
| p ≥ 4 | 0,7619 | 0,7749 |
| **p ≥ 5 (majorité, a priori)** | 0,7639 | 0,7662 |
| p ≥ 6 | 0,7539 | 0,7439 |

- Le seuil a priori τ = 1/√K* tombe sur le maximum de la grille, mais l'optimum est étroit : un cran de k de part et
  d'autre coûte 0,01 à 0,03. C'est la grossièreté de l'axe vertical (dix ordres, pas de log(k/(k−1))).
- Le compte d'ordres p (identité seule) est moins bon que VP : il ne mesure pas la séparation, et des regroupements
  intermédiaires sans durée de vie persistent à tous les ordres (l'ordre de fusion de gaussiennes équidistantes est
  le même à tous les ordres, puisque ce sont les mêmes points).
- **Appariement tolérant rejeté.** Un appariement par indice de Jaccard pondéré > 1/2 (seuil de dissolution de
  Hennig 2007 [A]) au lieu de l'égalité des ensembles de cœurs fait persister les fragments (un fragment qui porte plus
  de la moitié de la masse des cœurs d'une coquille s'apparie, aux ordres bas, à la coquille entière) :
  coquilles hard à 2 000 : 0,642 au lieu de 1,000 ; filaments medium : 0,603 au lieu de 0,998 (z = 3). L'égalité
  exacte est le discriminant.

### 4.6 KMAX = 5 (K* = 5, ordres 1..5), contre sklearn ms = 5

| Tête | Rempl. | n = 2 000 | n = 8 000 | Tout |
| --- | --- | ---: | ---: | ---: |
| v10-b une tranche K = 5, z = 4 | sans | 0,7623 | 0,7527 | 0,7575 |
| v10-b une tranche K = 5, z = 6 | b1.5 | 0,7707 | 0,7923 | 0,7815 |
| EOM bi-paramètre w = 1, z = 5 | b1.5 | 0,7802 | 0,7828 | 0,7815 |
| EOM bi-paramètre w = k, z = 4 | b1.5 | 0,7809 | 0,7797 | 0,7803 |
| EOM bi-paramètre w = k, z = 4 | sans | 0,7662 | 0,7529 | 0,7596 |
| feuilles VP (τ = 1/√5 : k_min ≤ 3) | sans | 0,7618 | 0,7503 | 0,7561 |
| feuilles VP (τ = 1/√5 : k_min ≤ 3) | b1.5 | 0,7640 | 0,7661 | 0,7650 |
| sklearn ms = 5, EOM α = 1 | sans | 0,6678 | 0,6706 | 0,6692 |
| sklearn ms = 5, feuilles α = 2 | b1.5 | 0,7601 | 0,7039 | 0,7320 |

À K* = 5, aucun gain : l'axe vertical n'a que cinq crans (VP ∈ {0 ; 0,22 ; 0,51 ; 0,92 ; 1,61}) et le seuil
a priori exige k_min ≤ 3, un rapport de densité ≥ 1,67, qui élague trop.

### 4.7 Stabilité de la sélection vis-à-vis de l'ordre de référence

ARI (bruit en singletons) entre les partitions obtenues avec K* = 10 et avec K* = 8 ou 5, même tête, sans
remplissage :

| Tête | 10 / 8, n = 2 000 | 10 / 8, n = 8 000 | 10 / 5, n = 2 000 | 10 / 5, n = 8 000 |
| --- | ---: | ---: | ---: | ---: |
| v10-b une tranche z = 3 | 0,956 | 0,972 | 0,923 | 0,949 |
| v10-b une tranche z = 6 | 0,956 | 0,971 | 0,906 | 0,928 |
| EOM bi-paramètre w = 1, z = 6 | 0,962 | 0,971 | 0,925 | 0,945 |
| EOM bi-paramètre w = k, z = 6 | 0,961 | 0,960 | 0,919 | 0,929 |
| feuilles VP (τ = 1/√K*) | 0,950 | 0,928 | 0,905 | 0,896 |

**La persistance à travers K ne rend pas la sélection plus stable vis-à-vis de K*** : les candidats restent ceux de
la tranche de référence T~_K*, qui changent avec K* ; le seuil VP, discret, change de cran avec K* (k_min ≤ 7, 5, 3).
L'instabilité de Rolle–Scoccola est déplacée, pas supprimée.

### 4.8 Contrôle sur la réplique 1 du dev (128 autres scènes, graines seed_of('dev', base, 1))

La pondération w_k = k et le choix z = 6 ont été faits en regardant la réplique 0. La réplique 1 (toujours dev,
jamais test) les contrôle : mêmes têtes, mêmes constantes, aucune modification.

| Tête (K* = 10) | Rempl. | n = 2 000 | n = 8 000 | Tout |
| --- | --- | ---: | ---: | ---: |
| v10-b z = 3 | b1.5 | 0,7976 | 0,7729 | 0,7852 |
| v10-b z = 4 | b1.5 | 0,7931 | 0,7871 | 0,7901 |
| v10-b z = 5 | b1.5 | 0,7880 | 0,7972 | 0,7926 |
| v10-b z = 6 | b1.5 | 0,7748 | 0,7985 | 0,7867 |
| EOM bi-param. w = 1, z = 6 | b1.5 | 0,7870 | 0,7978 | 0,7924 |
| EOM bi-param. w = k, z = 6 | b1.5 | 0,7848 | 0,7994 | 0,7921 |
| EOM bi-param. w = k, z = 3 | b1.5 | 0,7955 | 0,7832 | 0,7893 |
| élagage VP + EOM z = 6 | b1.5 | 0,7789 | 0,7957 | 0,7873 |
| feuilles VP (sans z) | b1.5 | 0,7789 | 0,7901 | 0,7845 |
| v10-b z = 4 | sans | 0,7769 | 0,7649 | 0,7709 |
| v10-b z = 5 | sans | 0,7701 | 0,7714 | 0,7707 |
| v10-b z = 6 | sans | 0,7516 | 0,7683 | 0,7599 |
| EOM bi-param. w = k, z = 6 | sans | 0,7693 | 0,7724 | 0,7709 |
| **élagage VP + EOM z = 6** | sans | 0,7753 | 0,7761 | **0,7757** |
| feuilles VP (sans z) | sans | 0,7753 | 0,7671 | 0,7712 |
| sklearn ms = 10, feuilles α = 2 | b1.5 | 0,7692 | 0,7494 | 0,7593 |

Écarts appariés sur les DEUX répliques (256 scènes dev), IC bootstrap 95 % :

| Tête | Base | Écart | IC 95 % | Gains / pertes |
| --- | --- | ---: | --- | --- |
| EOM bi-param. w = k, z = 6, b1.5 | v10-b z = 6, b1.5 (même z, choix lot C) | +0,0072 | [+0,0036 ; +0,0108] | 39 / 6 |
| id., à 8 000 seulement | v10-b z = 6, b1.5 | +0,0025 | [−0,0008 ; +0,0058] | 10 / 3 |
| EOM bi-param. w = k, z = 6, sans | v10-b z = 6, sans | +0,0115 | [+0,0077 ; +0,0155] | 42 / 3 |
| EOM bi-param. w = 1, z = 6, sans | v10-b z = 6, sans | +0,0118 | [+0,0062 ; +0,0172] | 46 / 5 |
| EOM bi-param. w = k, z = 6, b1.5 | v10-b z = 5, b1.5 (meilleure config. unique) | +0,0027 | [−0,0013 ; +0,0079] | 20 / 12 |
| EOM bi-param. w = k, z = 6, sans | v10-b z = 5, sans (choix lot C) | +0,0034 | [+0,0000 ; +0,0076] | 21 / 11 |
| élagage VP + EOM z = 6, sans | v10-b z = 5, sans (choix lot C) | +0,0066 | [+0,0019 ; +0,0118] | 41 / 43 |
| feuilles VP (sans z), b1.5 | v10-b z = 5, b1.5 | −0,0033 | [−0,0105 ; +0,0041] | 40 / 49 |
| feuilles VP (sans z), sans | v10-b z = 5, sans | +0,0028 | [−0,0026 ; +0,0086] | 41 / 48 |
| EOM bi-param. w = k, z = 6, b1.5 | sklearn ms = 10 feuilles α = 2, b1.5 | +0,033 | [+0,016 ; +0,048] (réplique 1) | 101 / 25 |

Ce que le contrôle confirme et infirme :
- **Confirmé** : à z égal, la tête bi-paramètre bat la tranche (+0,007 avec remplissage, +0,012 sans, 3 à 6 pertes
  sur 256) ; les gains se concentrent à 2 000 points, où z = 6 sur-segmente la tranche.
- **Infirmé** : « un seul z pour les deux tailles ». Sur la réplique 1, l'optimum de la tête bi-paramètre est z = 3–4
  à 2 000 et z = 6 à 8 000, comme pour la tranche ; z = 6 coûte 0,011 à 2 000 (0,023 pour la tranche) : le regret
  est divisé par deux, pas supprimé.
- **Infirmé** : l'intérêt de la pondération w_k = k. Sur la réplique 1, w = 1 et w = k sont égaux (0,7924 contre
  0,7921) ; w = k évite seulement la chute isolée de la réplique 0 (§ 5.3).
- **Tête sans z** : à 0,003 près de la meilleure configuration unique de la tranche sur 256 scènes, dans les deux sens
  selon le remplissage ; elle reste 0,014 (avec remplissage) et 0,006 (sans) sous la tranche réglée PAR TAILLE sur
  la réplique 1 (réglage que la tranche ne peut faire qu'en choisissant z selon n).
- **Seule avance significative contre la meilleure configuration de la tranche** : élagage VP + EOM z = 6 SANS
  remplissage, +0,0066 [+0,0019 ; +0,0118] sur 256 scènes, mais avec autant de pertes que de gains (41 / 43) : un
  déplacement de la répartition des erreurs plus qu'une amélioration uniforme.


## 5. Lecture critique

### 5.1 Ce que la persistance à travers K voit (arbres annotés, `explore_match.py`, `explore_vp.py`, n = 2 000)

| Scène (K* = 10) | Amas | p (ordres où l'amas persiste) | VP | Durée de vie à K* (log r) |
| --- | --- | --- | --- | --- |
| `shells` hard | 8 vraies coquilles | 10 | log 10 (saturé) | 0,28–1,09 |
| `shells` hard | fragments de coquilles (46–182 points) | 1–4 (ordres 7–10) | 0–0,22 | 0,01–0,27 |
| `shells` hard | regroupements de coquilles | 8–10 | log 10 | 0,00–0,05 (un seul réel : 0,64) |
| `filaments` medium | vrais filaments | 8–10 | 1,20–2,30 | 0,37–1,33 |
| `filaments` medium | fragments d'un filament (45–159 points) | 1–6 (ordres 5–10) | 0–0,51 | 0,01–0,32 |
| `anisotropic` extreme, bruit 0,1 | gaussiennes 4 et 7 (faibles, vraies) | 4 (ordres 7–10) | 0,36 | 0,15–0,29 |
| `unbalanced` hard (n = 8 000) | gaussienne 2 (492 points, vraie) | 2 | 0,11 | 0,05 |

- Les coquilles et les filaments se fragmentent aux ordres HAUTS : à grand K, le rayon d'échelle dépasse
  l'épaisseur et les fluctuations 1D/2D de grande échelle, qui ont alors assez de points pour dépasser mcs, créent
  des morceaux. Aux ordres bas, ces fluctuations ne donnent que des morceaux sous mcs. D'où l'intérêt de la
  persistance vers le bas, et le gain sur ces familles.
- Mais une gaussienne faible (rapport de densité cœur/col ≈ 1,4) n'est résolue, elle aussi, qu'aux ordres hauts,
  parce que les ordres bas sont plus bruités (écart-type relatif 1/√k) et chaînent. **À n ≤ 8 000 et K ≤ 10,
  un fragment de filament et un mode vrai faible ont le même profil (p, VP)** ; ce qui les distingue en principe
  est la dimension locale (un creux sur une structure 1D est bien plus probable qu'un col 3D de même rapport), que
  ni p ni VP ne mesurent.
- Les regroupements intermédiaires persistent à tous les ordres (l'ordre de fusion de gaussiennes équidistantes est
  fixé par les mêmes points) : le compte p seul ne suffit pas, il faut la durée de vie (EOM) ou la séparation (VP).

### 5.2 Ce qu'elle ne peut pas faire

- **Créer une branche.** Les gaussiennes larges d'`heteroscedastic` (σ = 2,5, densité au mode ≈ 1/250 de celle des
  étroites) ne forment pas de gros amas distinct à K = 10 ; les plus petits amas d'`unbalanced` (1/65 de n) sont
  sous ou près de mcs = √n. Aucune sélection sur l'arbre ne les rend ; c'est l'écart au plafond de Bayes (§ 4.4).
- **Garder un mode vrai faible sous le seuil.** La persistance vers le bas est un test de robustesse à PLUS de bruit ;
  elle retire, à juste titre statistiquement mais à tort pour la vérité du banc, les modes dont le rapport de
  densité est inférieur à ≈ 1,4 à K* = 10 (`unbalanced` : −0,03 à −0,05).
- **Tester vers le haut.** La vérification naturelle d'un mode faible serait sa persistance à K > K* (moins de
  bruit) ; avec KMAX = 10, elle n'est pas disponible.

### 5.3 Pourquoi l'EOM bi-paramètre non pondéré peut s'effondrer

Sur `filaments` extreme à 8 000 (filaments qui se croisent), T~_10 sépare le filament 0 d'un bloc #1 de sept
filaments ; #1 persiste aux ordres 2–10, ses enfants seulement aux ordres 8–10 ou 9–10. Aux ordres bas, #1
accumule sa stabilité (38 → 1 000 en unités normalisées) alors que ses enfants n'y ont pas d'identité : l'EOM
choisit #1 (2 amas, ARI_s 0,08 contre 0,46). C'est le prix de l'appariement exact : la structure qui ne se
retrouve pas à l'identique aux ordres bas y est perdue pour les enfants, pas pour le parent. La pondération par
la précision (w_k = k) donne aux ordres bas, les plus bruités et les plus chaînants, le poids que leur variance
justifie ; la scène redevient identique à la tranche. Restreindre la somme à la moitié haute des ordres (w_k = 1
si 2k ≥ K*, sinon 0) donne le même résultat (J = 0,7869 contre 0,7865 avec remplissage) : c'est le
sous-poids des ordres bas qui compte, pas la forme exacte des poids.

### 5.4 Ce que l'exactitude de la tour apporte ici, et ce qu'elle n'apporte pas

- Les deux mesures reposent sur les verticales exactes (`lower`) et sur la comparaison, au MÊME rayon, de
  composantes de deux ordres : la tour fournit ces applications sans ambiguïté.
- Mais l'axe multi-K existe aussi pour les hiérarchies de cœurs (l'atteignabilité mutuelle est emboîtée en
  min_samples ; Neto et al. 2017 calculent toutes les hiérarchies HDBSCAN* d'une plage de mpts). **Je n'ai pas
  appliqué les mêmes têtes à MR-bord** : rien ici ne dit que le gain de l'EOM bi-paramètre sur la tranche est
  propre à l'objet exact. Seules la stabilité de Prohorov (Blumberg–Lesnick) et la lecture « rapport de densité à
  rayon fixe » de VP sont propres à la multicouverture.

## 6. Limites

- Dev seulement, 128 scènes (réplique 0) et un contrôle sur 128 autres (réplique 1), 8 familles synthétiques ; les
  écarts à z réglé sont de l'ordre de 0,01 et leurs IC contiennent 0.
- La pondération w_k = k a été choisie après avoir vu w_k = 1 (un seul autre candidat essayé, la moitié haute, au
  même niveau). Le seuil τ = 1/√K* et θ = ⌈K*/2⌉ ont été fixés avant toute mesure.
- mcs = √n est gardé tel quel ; il explique une part de l'écart sur `unbalanced`.
- Les candidats viennent de la seule tranche de référence ; pas d'antichaîne entre ordres (famille non laminaire,
  empaquetage d'ensembles).
- Les étiquettes viennent de T~_K* ; un point peut être attaché à des composantes différentes selon l'ordre (entrée
  cover, théorème 2 : un point peut être à distance ≤ r de deux composantes) ; ignoré.
- Prototype Python, un fil ; aucune porte, aucun mutant ; ce n'est pas une implémentation candidate.

## 7. Coût (8 000 points, `spherical` hard bruit 0,1, un fil, machine chargée)

| Étage | Une tranche (v10-b, K = 10) | Multi-K (ordres 1..10) |
| --- | ---: | ---: |
| catalogue d'ordre 10 | 17,0 s | 17,0 s (le même) |
| tour | 3,6 s (ordre 10 seul, `only_order`) | 11,7 s (tour FULL : tous les ordres, dont attaches 0,75 s et verticales 0,60 s) |
| tête | 0,9 s (C++) | 1,1–1,7 s (Python : condensation des dix ordres) + quelques ms (appariement, VP, sélection) |
| total | ≈ 21,5 s | ≈ 30 s (+40 %) |

- Le surcoût vient de la construction des ordres 1 à 9 (descentes), pas des verticales (0,6 s). Les nœuds d'ordre
  bas sont peu nombreux (16 000 à l'ordre 1, 914 000 à l'ordre 10) : la mémoire est dominée par les ordres hauts
  (3,8 millions de nœuds au total, contre 0,9 million pour l'ordre 10 seul).
- La tête elle-même est négligeable une fois les arbres condensés : l'appariement et VP ne touchent que les amas
  condensés (quelques dizaines par ordre). Un portage C++ serait direct (mêmes structures que `head.cpp`).
- Le temps de tour croît avec n comme la tour FULL (lot de mesure v10) ; aucune mesure à 16 000 ou 32 000 ici.


## 8. Recommandation

1. **Ne pas remplacer la tête v10-b sur la foi de ce travail.** Aucune tête par persistance à travers K ne bat la
   tranche à son meilleur z ; les écarts sont de l'ordre de 0,003 et leurs IC contiennent 0, sauf une configuration
   sans remplissage à gains et pertes équilibrés.
2. **Ce qui est acquis, et utile** :
   - la persistance à travers K se calcule exactement et pour presque rien sur la tour (verticales 0,6 s à 8 000
     points ; appariement et VP en millisecondes) ;
   - à z donné, l'EOM bi-paramètre corrige la sur-segmentation de la tranche (coquilles, filaments) avec très peu
     de pertes (6 sur 256 avec remplissage) : c'est une assurance contre un z trop grand, qui divise par deux le
     regret du choix de z quand n change ;
   - la prominence verticale VP est une mesure de significativité en unités de densité, sans d ni z ; avec un seuil
     fixé d'avance (1/√K*), les feuilles significatives valent la meilleure configuration unique de la tranche (à
     0,003 près ; 0,006 à 0,014 de moins que la tranche réglée par taille) sans aucun cadran : c'est l'outil à
     garder pour publier « combien de modes significatifs » et pour le LiDAR, où z n'a pas de valeur naturelle.
3. **Si l'on veut tester une tête multi-K en préenregistrement**, le candidat le mieux fondé est l'EOM
   bi-paramètre à z = 6 (w = 1 ou w = k : indiscernables, +0,001 [−0,003 ; +0,006] sur 256 scènes ; w = k perd
   un peu dans 12 scènes et évite une chute de 0,39 dans une ; préférer w = k pour cette assurance), comparé à
   v10-b au MÊME z et avec le
   même remplissage, avec en contrôle les mêmes têtes sur MR-bord (hiérarchies d'HDBSCAN, qui ont aussi un axe
   min_samples) pour attribuer un éventuel gain à l'objet ou à la seule idée multi-K.
4. **Pour identifier les modes d'un mélange séparable que la tranche manque** (remarque de l'utilisateur) : la
   sélection n'est pas le levier. Il faut (a) des ordres plus hauts que 10 pour résoudre les modes faibles avec
   moins de bruit (et tester la persistance VERS LE HAUT), (b) un mcs qui ne soit pas √n pour les petits amas,
   (c) une règle d'affectation pour la masse sous le col (le remplissage).
5. **Pistes ouvertes**, par ordre de promesse : dimension locale lue sur la pente des niveaux de fusion à travers k
   (log μ_k contre log k), pour séparer un fragment 1D d'un mode 3D faible, que ni p ni VP ne distinguent ;
   candidats pris dans tous les ordres (arbre de consensus des ensembles de cœurs) pour une vraie stabilité en K* ;
   mêmes têtes sur MR-bord.

