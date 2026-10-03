# La règle $H_m$ face aux cibles ancrées de l'utilisateur et à ses propres théorèmes

3 octobre 2026, 21 h 37 UTC (heure lue par `date -u`). Agent `marge_cibles` du workflow « meilleure méthode
mathématique ». Cadre :

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé. Aucune construction ni test natif. Aucune commande git.
Lecture seule sur le dépôt, le worktree v11 et les dossiers v10 ; écritures sous marge_cibles/ seulement.
```

Objet jugé : la règle $H_m$ de `morsehgp3D_v11/docs/HIERARCHIE_POINTS.md` (version relue à 21 h 18, § 1 à 6).
Toute valeur chiffrée vient d'un calcul exact rejoué ici (commande au § 9) ou d'un fichier cité. Les rayons sont
donnés en clair ; les niveaux sont des rayons carrés exacts (`Fraction`).

## En bref

1. **Adaptateur validé trois fois.** Le code v10 rejoue ER0h(1, 12) à 125/125 (empreinte `bd7a1cbf0083cc95`,
   identique au reçu publié). Mon adaptateur (oracle v11 → ultramétrique exacte → juge condensé v10 inchangé)
   redonne la même empreinte pour ER0h, et redonne cellule par cellule les scores publiés de `core` (45/125) et
   `cover` (74/125). Sur le catalogue, `core`, `cover` (= `A5_U1`) et `P_2` coïncident **entrée par entrée** avec
   les reçus natifs v10 sur les 64 entrées jugées. Deux routes indépendantes de $H_m$ (oracle v11, réécriture sur
   l'arbre Γ_K v10) concordent sur 542 hiérarchies.
2. **Cellules ancrées (125 jugements).** $H_1$ : **35/125**. $H_{k+1}$ (règle mesurée au § 6 du document) :
   **70/125**. $H_{\max(k+1,\mathrm{mcs})}$ (choix retenu en H5) : **70/125**. Témoins : ER0h 125, `cover` 74,
   `first` ($m=k+1$, sans marge) 70, `core` 45.
3. **La plupart des échecs sont des échecs de lignée, et aucune règle de date ne les répare.** Théorème prouvé au
   § 4 : sur la paire de cellules Q1bis et Q2 (toutes deux à K = 2, mcs = 2), toute pendaison fidèle qualifiée à
   $m\geq 3$ échoue Q2, et toute pendaison fidèle de lignée « première couverture » ($m\leq 2$) échoue Q1bis,
   **quelle que soit la date**. Aucun seuil $m=f(K,\mathrm{mcs})$ ne passe les deux. Par H3 lui-même, l'échec se
   propage : toute règle qui vérifie la borne de H3 et suit la première couverture échoue aussi T0_P1 (base).
4. **$H_m$ n'est pas nouvelle.** La famille H4 en **rayon** est exactement l'« ancrage persistant » $P_\kappa$ de
   la v10 (mémo du 30 septembre) : à κ = 2, ma réécriture redonne P_2 publié (70/125 avec la même ventilation,
   même date 1 487,404 sur Q1 ; 1 734 jugements identiques sur 64 entrées du catalogue). H3/H4 sont le théorème T5
   de ce mémo, transposé en niveau carré ; la version en niveau carré y était un **mutant** (`beta_mutant`).
   $P_\kappa$ avait été exclu par la réponse de l'utilisateur sur T0. L'apport propre de $H_m$ est la
   qualification $m$ (H5), qui répare T0/Q1 et casse Q2, Q3, Q-Π2 et Q4.
5. **Preuves.** F1, F2, H1, H3 et la borne de H4 sont justes (relues pas à pas, § 6). Failles : H2 est non local
   en échelle (un rival tardif retarde rétroactivement) ; H4 ne compare que des majorants (la borne inférieure
   κ/2 de T6 v10 manque) et son cadrage « de combien, au minimum » est inversé (κ = 1 est le retard
   **maximal** de la famille) ; « aucun autre paramètre » cache le choix de l'échelle des niveaux ; H5 rend la
   projection dépendante de mcs, contre le verdict v10 et contre le § 6 du même document.
6. **Pathologie nouvelle de la marge en niveau carré** (fixture exacte, § 7) : un amas lointain qui couvre
   brièvement un site d'une paire serrée le retarde jusqu'à $\sqrt{2s+2}$ (141,4 pour s = 10 000) ; en rayon,
   jusqu'à 2. La marge en rayon est la seule modification minimale sans paramètre que je recommande : elle ne
   coûte rien en preuve (T5, constantes 3ε et 5ε uniformes) et gagne partout ici (cellules 55 contre 35 à m = 1 ;
   catalogue 1 732 contre 1 656 à m = k + 1), mais elle ne répare **aucun** échec de lignée.
7. **Catalogue v2** (64 entrées de n ≤ 16, 2 013 jugements ; 151 entrées sautées et comptées car n > 16) :
   `first` 1 991, ER0h 1 835 (reçu), `cover` 1 832, P_2 1 734, $H_{k+1}$ en rayon 1 732, $H_{k+1}$ 1 656,
   $H_{\max(k+1,\mathrm{mcs})}$ 1 595, $H_1$ en rayon 1 517, $H_1$ 1 437, `core` 1 412. La marge de $H_{k+1}$ coûte
   335 jugements sur `first`, tous des retards.

**Verdict.** $H_m$ est une pendaison fidèle propre, stable et équivariante, mais sa lignée est celle de la
première couverture (qualifiée) : c'est précisément ce que les réponses ancrées de l'utilisateur réfutent, dans
les deux sens (Q1/T0 contre la première couverture brute, Q2/Q3/Q4/Q-Π2 contre la qualification). Elle ne peut
pas être « la » réponse si ces cibles tiennent ; elle reste un bon témoin stable, à condition de passer la marge
en rayon. Une règle qui passe les cibles doit choisir la lignée en **comparant les rivaux** (majorité de temps de
couverture, comme ER0h) : la question mathématique ouverte est la stabilité uniforme d'une telle règle, pas la
marge.

## 1. Méthode

**Règles calculées.** Pour chaque nuage et chaque ordre K : `hgp11_ref.Definition(P).order(K)` (oracle de la
définition, Γ_K exhaustif, fractions exactes), puis `points_reference.reference_rules(res, n, m)` pour
$m = 1$ (`core`, `cover`, `margin1` = $H_1$) et $m = K+1$ (`first`, `margin` = $H_{k+1}$), et
$m=\max(K+1,\mathrm{mcs})$ pour la variante de H5 ; ultramétrique exacte par `reference_ultrametric`.

**Route B (réécriture indépendante).** $H_m$ réécrite depuis la définition du § 3 du document sur l'arbre Γ_K de
la v10 (`vfull.full_gamma`, couverture $c_x(v)$ par nœud) : $t$ = premier niveau qualifié, $D=\max(0,\max_v h(\mathrm{lca}(o,v))-s_v)$
sur les nœuds qualifiés hors lignée, $e=t+D$, propriétaire = ancêtre vivant à $e$. Même code avec une pente κ
(famille H4) et en rayon (sommes exactes de racines, `vrad.R`). Recoupe A contre B : 542 hiérarchies $H_1$ et
$H_{k+1}$ des cellules, 0 désaccord (`recus/cellules_hm.json`, champ `stats`).

**Juge.** Celui de la v10, inchangé : `cellules.juger_cellule` (cellules ancrées, modes `triangles`, `aucun`,
`q4strict`) et `ver.juger` (catalogue), sémantique condensée de `CIBLES_REVISEES.md` § 1.1. Mon seul ajout est
l'objet `UHier` (`adaptateur.py`) qui expose `partition(r)` et `rayons_changement()` à partir de l'ultramétrique :
deux sites entrés sont dans le même bloc au rayon r si et seulement si $u(i,j)\leq r^2$ ; un site non entré est
un singleton, comme dans `ver.Hier`. La projection ne dépend pas de mcs pour $H_1$ et $H_{k+1}$ ; la condensation
à mcs est celle du juge v10.

**Validation de l'adaptateur.**

| Contrôle | Attendu (publié) | Obtenu | Reçu |
| --- | --- | --- | --- |
| ER0h(1, 12), code v10, cellules | 125/125, empreinte `bd7a1cbf0083cc95` | identique | `recus/cel_ER0h_e1_k12_rejoue.json` |
| ER0h v10 relue par `UHier` | idem | 125/125, même empreinte (suites condensées des cellules ouvertes comprises) | `recus/cellules_validation_ER0h_U.json` |
| `core` v11, cellules | 45/125 : T0 0/40, Q1 5/10, Q1bis 0/10, Q2 0/5, Q3 25/25, Q-Π2 5/5, Q4 10/30 | identique | `recus/cellules_hm.json` |
| `cover` v11 (LCA des ex æquo), cellules | 74/125 : T0 14/40, Q1 5, Q1bis 0, Q2 5, Q3 25, Q-Π2 5, Q4 20/30 | identique | idem |
| famille H4 en rayon, κ = 2, cellules | P_2 : 70/125, Q1 retardé à 1 487,404 | identique, même date | `recus/cellules_famille_H4.json` |
| catalogue, 64 entrées n ≤ 16 | reçus natifs `core`, `A5_U1`, `P_2` | 64/64 entrées identiques pour chacune ; `cover_A1` (départage natif) diffère sur 5 entrées d'ex æquo | `recus/catalogue_hm_n16_b.json` |
| catalogue, ER0h relue par `UHier` | reçu Γ_K `catalogue_gamma_er0h_1_12.json` | 8/8 entrées identiques | `recus/catalogue_hm_n9.json` |

**Échantillon.** Cellules : les 10 cellules ancrées (125 jugements), toutes calculées ; leurs nuages ont n ≤ 14
et K ≤ 3. Dans la consigne stricte « n ≤ 9 » tombent T0, Q1, Q1bis, Q2, Q4 (95 jugements) ; Q3 et Q-Π2
(n = 14, K = 2, 30 jugements) ont été calculées aussi, l'oracle y coûtant moins d'une seconde. Catalogue : 215
entrées hors fixtures de l'utilisateur ; 64 jugées (n ≤ 16, K ≤ 5, même périmètre que le reçu Γ_K de la v10),
151 sautées et comptées (n > 16) ; sous-ensemble strict n ≤ 9 : 19 entrées. CPU total des calculs : environ
6 min 30 s.

## 2. Cellules ancrées

| Règle | T0 (40) | Q1 (10) | Q1bis (10) | Q2 (5) | Q3 (25) | Q-Π2 (5) | Q4 (30) | Total | dont n ≤ 9 (95) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ER0h(1, 12), v10 rejouée | 40 | 10 | 10 | 5 | 25 | 5 | 30 | **125** | 95 |
| `cover` (LCA) | 14 | 5 | 0 | 5 | 25 | 5 | 20 | 74 | 44 |
| `first`, $m=k+1$ | 40 | 10 | 10 | 0 | 0 | 0 | 10 | 70 | 70 |
| **$H_1$** (niveau carré) | 0 | 5 | 5 | 0 | 0 | 5 | 20 | **35** | 30 |
| **$H_{k+1}$** (règle mesurée au § 6) | 40 | 10 | 10 | 0 | 0 | 0 | 10 | **70** | 70 |
| **$H_{\max(k+1,\mathrm{mcs})}$** (H5) | 40 | 10 | 10 | 0 | 0 | 0 | 10 | **70** | 70 |
| $H_1$ en rayon (= $P_1$ v10) | 0 | 5 | 0 | 0 | 25 | 5 | 20 | 55 | 25 |
| $H_{k+1}$ en rayon | 40 | 10 | 10 | 0 | 0 | 0 | 10 | 70 | 70 |
| famille H4, κ = 2, rayon (= P_2 v10) | 0 | 5 | 0 | 5 | 25 | 5 | 30 | 70 | 40 |
| famille H4, κ = 2, niveau carré | 0 | 5 | 0 | 5 | 25 | 5 | 30 | 70 | 40 |
| `core` | 0 | 5 | 0 | 0 | 25 | 5 | 10 | 45 | 15 |

Suites condensées des bases (`recus/suites_condensees.log`) :

| Fixture | $H_1$ | $H_{k+1}$ | ER0h(1, 12) |
| --- | --- | --- | --- |
| T0_P1, mcs 2 | AB \| EF dès 1 154,684 ; ABC \| DEF de **1 931,816** à 1 931,827 | ABC \| DEF de 1 154,684 à 1 931,827 | idem $H_{k+1}$ |
| T0_P2, mcs 2 | AB \| EF ; AB \| CD \| EF de 1 930,355 à 1 930,861 | ABC \| DEF dès 1 154,684 | idem |
| T0_S_3D, mcs 2 | AB \| EF dès 816,497 ; C, D à la racine 1 224,745 | ABC \| DEF dès 816,497 | idem |
| Q1 (T1_1700), mcs 2 | AB \| EF dès 1 154,684 ; AB \| CD \| EF dès **1 707,981** | ABC \| DEF dès 1 154,684 | AB \| EF ; ABC \| DEF dès 1 407,584 |
| Q2 (S17), mcs 2 | b1b2 dès 9,087 ; ax \| b1b2 dès **67,373** | **b1b2x dès 60,360** ; tout à 75,122 | b1b2 ; ax \| b1b2 dès 50 |
| Q3 (filament), mcs 2 | filament sans x dès 700,501 ; x dans le filament à **744,180** | **x dans l'amas dès 590,540** | x avec f1 dès 350,003 ; filament entier dès 700,501 |
| Q3, mcs 9 | rien avant 796,117 | **amas + x (9 points) dès 590,540** | rien avant 796,117 |
| Q4 (T6, K = 3), mcs 2 | PQR \| P2Q2R2 dès **899,514** ; CmD dès 1 262,273 | **CPQR \| DP2Q2R2 dès 866,025** ; m à la racine | PQR \| P2Q2R2 dès 866,025 ; CmD dès 1 157,276 |

## 3. Liste des échecs de $H_m$ et réparation minimale

« Lignée » : la composante choisie ne rejoint celle de la cible qu'après la fenêtre ; aucune date ne répare.
« Retard » : la lignée est bonne, l'entrée est trop tardive. « Réparation » : modification **minimale,
compatible avec H3, sans paramètre ad hoc**.

| Règle | Cellule (fixture, cible) | Sortie | Nature | Réparation minimale ? |
| --- | --- | --- | --- | --- |
| $H_1$ | T0_P2 (pont 0,1 % plus court), ABC \| DEF sur [1 300 ; 1 700] | C entre dans CD à 1 930,355 | lignée (première couverture unique = CD) | **non** (§ 4, même argument que Q1) |
| $H_1$ | T0_P1, idem | C entre dans ABC à 1 931,816 (racine 1 931,827) | retard **forcé par H3** : la variante D−1 a CD pour première couverture unique | **non** : lemme de propagation (§ 4.3), minorant 1 926,33 > 1 700 |
| $H_1$ | T0_S_plan, T0_S_3D (ex æquo exacts) | C et D attendent la racine (1 897,367 ; 1 224,745) | lignée indécidée : ex æquo résolus par l'attente | non sans majorité (2 lentilles ex æquo sur 3 mènent au triangle : c'est un comptage) |
| $H_1$ | Q1 (mcs 3) et Q1bis (mcs 2), ABC exigé sur [19/20 f ; f) | C entre dans CD à 1 707,981 | lignée | **non** (théorème du § 4) |
| $H_1$ | Q2, {x, a} \| {b1, b2} sur [61 ; 75] | x entre à 67,373 | retard (rival xb1 né à 60,208, réuni à la racine) | marge en rayon : 65,052, échoue encore ; κ = 2 (P_2) : 54,844 passe, mais κ est un paramètre |
| $H_1$ | Q3, filament \| amas sur [705 ; 790] | x entre à 744,180 | retard (lentille xc0 à 450, réunie à 796,117) | **oui** : marge en rayon, x à 696,120 (25/25) |
| $H_1$ | Q4, PQR \| P2Q2R2 dès 866,025 | P, Q, R entrent à 899,514 | retard (triplets mixtes nés à 1 009,95, réunis à 1 078,174) | marge en rayon : 884,720, échoue ; κ = 2 passe (paramètre) |
| $H_{k+1}$, $H_{\max(k+1,\mathrm{mcs})}$ | Q2 | x pris par le triangle {x, b1, b2} à 60,360 ; a n'entre qu'à 75,122 | lignée imposée par la qualification | **non**, pour tout $m\geq 3$ et toute date (§ 4) |
| idem | Q3 (mcs 2 à 6) | x entre dans l'amas à 590,540 | lignée (le filament n'est qualifié qu'à 700,0, l'amas dès 453,471) | non : amas et filament ne se rejoignent qu'à 796,117 > 790 |
| idem | Q-Π2 (mcs 9) | amas + x = 9 points, cluster sur [705 ; 790] | complétion par un point de bord, refusée par l'utilisateur | une date plus tardive passerait Q-Π2 mais pas Q3 : la projection est la même |
| idem | Q4 (cibles v2) | C entre dans CPQR à 866,025 ; m à la racine | lignée : la chaîne (3 sites) n'est jamais qualifiée à $m=4$ avant la racine | **non** (fait (3) du § 4) |

Q4 compte 30 jugements : 10 « Q4 strict » (C, m, D sans cluster commun à la naissance des tétraèdres), qui
passent pour toutes les règles $H$, et 20 sur les deux cibles v2. $H_{k+1}$ échoue les 20 ; $H_1$ échoue les 10 de
la première fenêtre (retard de P, Q, R) et passe la seconde (CmD formé à 1 262,273 < 1 267,46).

## 4. Théorème d'impossibilité pour toute règle à lignée de première couverture qualifiée

**Faits exacts** (oracle v11, `recus/faits_full.json`, recoupés par la route B) :

- (1) Q1, base, K = 2 : $A_2(C)=722500$ (rayon 850), atteint par un seul nœud, la lentille CD ; son parent est la
  racine, de niveau 3 194 656 (rayon 1 787,360) ; avant la racine, sa couverture est exactement {C, D}.
- (2) Q2, base, K = 2 : tout nœud vivant qui couvre a à un niveau au plus 5 625 (rayon 75) couvre au plus deux
  sites (la lentille {a, x}, puis la lentille {a, b1} née à 5 625).
- (3) Q4, base, K = 3 : $A_3(C)=480000$ (rayon 692,820), atteint par la seule chaîne CmD, dont le parent est la
  racine (1 780 000, rayon 1 334,166) ; avant la racine, sa couverture est {C, m, D}.
- (4) T0_P1, variante D−1 (D déplacé d'une unité) : la première couverture de C est la seule lentille CD
  (niveau 3996001/4) ; sa lignée ne couvre A qu'à partir de 14920361/4 (rayon 1 931,344) (`recus/verif_d1.json`).

**Théorème.** (a) Toute pendaison fidèle qualifiée à $m\geq 3$ (chaque site entre en un point de $R_i^{(m)}$)
échoue la cellule Q2 (K = 2, mcs = 2), quels que soient la lignée et la date. (b) Toute pendaison fidèle qui pend C
sur la lignée de sa première couverture (cas $m\leq K=2$, où la qualification est vide) échoue Q1bis (K = 2,
mcs = 2) et Q1 (mcs = 3), quelle que soit la date. (c) Donc aucune règle « lignée de la première couverture
qualifiée au seuil $m=f(K,\mathrm{mcs})$ », avec n'importe quelle règle de date (en particulier toute la famille
H4, en niveau carré ou en rayon, et `first`, `cover`), ne passe à la fois Q1bis et Q2.

*Preuve.* (a) La cible de Q2 exige, à chaque rayon de [61 ; 75], un cluster contenant x et a (bloc {x, a}, aucun
point toléré, mcs = 2). Au niveau 3 721, a doit donc être entré. Une pendaison qualifiée à $m\geq3$ ne fait entrer
a qu'en un point d'un nœud qui couvre a et au moins trois sites ; par (2), aucun tel nœud n'existe jusqu'au niveau
5 625 > 3 721. Donc a est un singleton (bruit) au rayon 61 : la cible échoue, pour toute lignée et toute date.
(b) Par (1), la première couverture de C est unique ; une pendaison de lignée « première couverture » met C, à
tout niveau inférieur à 3 194 656, dans un bloc du sous-arbre de la lentille CD. Par F1, ce bloc est inclus dans
l'amas discret du nœud, donc dans {C, D}. La seconde cible de Q1 et de Q1bis exige, au niveau 72079426/25 (rayon
1 697,99) < 3 194 656, un cluster contenant A, B et C (aucune tolérance) : impossible. (c) Q1bis et Q2 ont le même
couple (K, mcs) = (2, 2) : si $f(2,2)\geq 3$, (a) s'applique ; sinon la qualification est vide et (b) s'applique. □

**Ce que dit le théorème.** Pour passer les deux cellules, une règle doit, à K = 2 et mcs = 2, à la fois admettre
qu'une lentille de deux sites porte un cluster (Q2) et retirer C à sa première couverture unique, la lentille CD,
pour le donner au triangle (Q1bis). Elle doit donc **comparer** la première couverture à ses rivales : c'est ce que
fait ER0h (deux lentilles CA, CB contre une lentille CD, temps de couverture dans la bande). Aucun réglage de la
marge, de l'échelle des niveaux ou du seuil $m$ ne remplace cette comparaison.

**4.3 Lemme de propagation (par H3 lui-même).** Toute pendaison fidèle qui vérifie la borne $|\Delta u|\leq 5\delta$
de H3 et pend C sur sa première couverture dans la variante D−1 de T0_P1 vérifie, dans la base,
$u(C,A)\geq 14920361/4-5\delta$. Avec ε = 1 et Λ = 14935289/4 (plus grande racine des variantes, choix prudent),
δ = 3 865,62 et le minorant vaut 3 710 762, soit un rayon de **1 926,33**, hors de la fenêtre [1 300 ; 1 700] de T0
(`recus/propagation_h3_T0_P1.json`). *Preuve.* Par le fait (4), $u^{D-1}(C,A)\geq 14920361/4$ ; H3 conclut. □
Le retard de $H_1$ sur T0_P1 n'est donc pas un défaut de réglage : la stabilité le **transmet** depuis la
configuration voisine où le pont passe devant. C'est la version quantitative de l'exclusion de $P_\kappa$ par la
cible T0 (QUESTIONS_UTILISATEUR.md : « Cela exclut la première couverture et l'ancrage persistant P_κ »).

## 5. Catalogue v2

Périmètre : 64 entrées (n ≤ 16, K ≤ 5 ; 2 013 jugements) ; 151 entrées sautées (n > 16), comptées. Les cibles du
catalogue sont extrapolées (une seule confirmée) : ce tableau classe, il ne qualifie pas.

| Règle | Entrées passées | Jugements passés | Échecs par retard | Échecs de structure |
| --- | ---: | ---: | ---: | ---: |
| `first`, $m=k+1$, sans marge | 56/64 | **1 991** | 0 | 22 |
| ER0h(1, 12) (reçu Γ_K v10) | 50/64 | 1 835 | — | — |
| `cover` (= `A5_U1` natif) | 44/64 | 1 832 | 103 | 78 |
| P_2 (= H4 en rayon, κ = 2) | 44/64 | 1 734 | — | — |
| $H_{k+1}$ en rayon | 45/64 | 1 732 | — | — |
| **$H_{k+1}$** | 43/64 | **1 656** | 345 | 12 |
| **$H_{\max(k+1,\mathrm{mcs})}$** | 33/64 | **1 595** | 371 | 47 |
| $H_1$ en rayon | 37/64 | 1 517 | — | — |
| **$H_1$** | 36/64 | **1 437** | 557 | 19 |
| `core` | 36/64 | 1 412 | 601 | 0 |

Sous-ensemble strict n ≤ 9 (19 entrées, 331 jugements) : ER0h 325, `first` 323, `cover` 301, $H_{k+1}$ 293,
$H_{\max(k+1,\mathrm{mcs})}$ 292, `core` 256, $H_1$ 251 (`recus/catalogue_hm_n9.json`).

Lectures :

- La marge de $H_{k+1}$ coûte 335 jugements sur `first`, tous par retard : `k5_spheres_face_a_face` 0/48,
  `k3_dos_a_dos` 0/40, `octaedres_pont_carre_K4`, `octaedres_pont_centre_K5`, `tetraedres_releves_pont_K5` 0/25,
  `tetraedres_pont_arete_K3` 0/15 (`first` passe tout). Le document du développeur mesure le même signe sur ses
  scènes synthétiques G4 (`first` au-dessus de $H_{k+1}$ à chaque k).
- Mécanisme type (`k5_spheres_face_a_face`, base, `diag_rival.py`) : x1 est couvert par sa sphère dès 20 800 ; la
  sphère opposée ne le couvre qu'à partir de 41 017 et ne rejoint sa lignée qu'à 59 039 ; la marge le fait entrer à
  **47 285**, après la borne gauche 23 401 de la cible. La marge ne pondère pas l'avance prise par la première
  couverture : avec κ = 1, le terme d'un rival est $m(p,q)-h(q)$, indépendant de $t$.
- `first` domine ici aussi parce que la convention du catalogue à K ≥ 3 (famille `simplexes_et_ponts`) est
  l'option (b) « tétraèdres » de Q4, **rejetée** par l'utilisateur. Le catalogue ne départage donc pas les règles
  sur le point qui compte.
- $H_{\max(k+1,\mathrm{mcs})}$ fait moins bien que $H_{k+1}$ (1 595 contre 1 656) : faire dépendre la projection
  de mcs n'aide pas.

## 6. Relecture des théorèmes F1, F2, H1 à H5

| Énoncé | Verdict | Détail |
| --- | --- | --- |
| F1 (laminarité, fidélité) | juste | P4 pour la laminarité ; $R_i$ stable par remontée donne l'inclusion des blocs dans les amas. Antériorité : T1, T2 du mémo `ancrage_marges` v10. |
| F2 (trilemme) | juste | Sur {0, 2, 4} toute entrée à la première couverture choisit une des deux composantes ; la réflexion force un saut de 3. C'est T6 (ii) du même mémo. |
| H1 (bonne définition) | juste | Pour $p'$ ex æquo : $m(p,p')\leq e$, puis $m(p',q)-h(q)\leq\max(e-h(q),D)\leq D$ car $h(q)\geq t$ ; symétrie. |
| H2 (équivariance, exactitude sans rival) | juste avec une lecture non locale | L'hypothèse doit exclure les rivaux à **tous** les niveaux jusqu'à la réunion des lignées, y compris après $a$ ; la phrase de preuve « chaque terme de $D_i$ provient d'un point de la lignée de C » est fausse pour un rival tardif. Conséquence mesurée : x1 ci-dessus, amas bien formé à 23 401, retardé par un rival apparu à 41 017. |
| H3 (stabilité 3δ, 5δ) | juste | Pas à pas : $\psi R_i^{(m),Y}\subseteq R_i^{(m),X}$ (transport des identifiants couverts) ; $m^Y(p,q)\leq m^X(\psi p,\psi q)+\delta$ (φ commute aux remontées et φψ est la remontée de 2δ) ; ultramétrie via $p_i^X$ : $m^X(\psi p_i^Y,\psi q')\leq D_i^X+h(q')+\delta$ ; d'où $D^Y\leq D^X+2\delta$ et $e^Y\leq e^X+3\delta$ ; paires : $m^Y(p_i^Y,\varphi p_i^X)\leq e_i^Y+2\delta$, puis $\leq e_i^X+5\delta$. La même preuve en rayon donne 3ε et 5ε (T5 v10). Précision : Λ doit majorer tous les niveaux transportés (racines et premières couvertures des deux nuages) ; je n'ai trouvé aucun nuage où une première couverture dépasse la racine (deux essais, `verif_faits.py`, fait H3_lambda), sans preuve générale. |
| H4 (famille κ) | borne juste, conclusion surinterprétée | (i) La borne $(1+2\kappa)\delta$, $(1+4\kappa)\delta$ est vérifiée (je l'ai rederivée). (ii) « κ = 1 la plus stable » compare des **majorants** ; la borne inférieure manque : T6 du mémo v10 montre que toute règle κ-précoce a une constante de Lipschitz au moins κ/2 (famille {0, L ∓ δ, 2L} : la réflexion force $u^-(a,m)\geq L$ dès que $u^+(a,m)<L$ ; avec $\delta_0=L/(2\kappa-1)$, le rapport vaut κ/2). Précocité et stabilité s'échangent en Θ(κ). (iii) Le cadrage « de combien, au minimum, pour rester continu » est inversé : tout κ fini est continu ; κ = 1 est le membre le **plus tardif**. (iv) « Aucun autre paramètre » : l'échelle des niveaux (carré, rayon, ou toute reparamétrisation) est un paramètre fonctionnel qui change la règle (§ 7) ; le mémo v10 donne la famille complète des profils stables g (lipschitzien, $g(0)=0$, $g(s)-s$ non décroissant). |
| H5 (seuil m) | juste ; choix contestable | « Toute fusion est qualifiée à $m=k+1$ » : deux composantes distinctes ne contiennent pas la même k-partie, donc deux k-parties distinctes, donc au moins k + 1 sites. Mais le « choix retenu » $m=\max(k+1,\mathrm{mcs})$ rend la projection dépendante de mcs, ce que le verdict v10 avait réfuté (Q1bis, Q-Π2), et ce n'est pas la règle mesurée au § 6 du même document ($m=k+1$). Il échoue Q-Π2 (amas + x à 453,471 pour m = 9). « Une boule de k points n'est pas un amas » contredit la réponse Q2 de l'utilisateur. |

**Antériorité non citée.** Le document du développeur ne mentionne ni $P_\kappa$, ni le mémo
`v10-verrou-points/ancrage_marges/MEMO_ANCRAGE_MARGES_20260930.md`, ni le verdict v10 (ER0h). Ce mémo contient :
$P_\kappa$, $t=\max(\alpha,\max_j(m_j-\kappa(c_j-\alpha)))$ en rayon, lignée de première couverture (son point 3) ;
T5, $|\Delta t|\leq(1+2\kappa)\varepsilon$ et $|\Delta u|\leq(1+4\kappa)\varepsilon$ ; T6, borne inférieure κ/2 ;
et, dans `ownership.rule_pkappa`, la version en rayon carré comme mutant (`beta_mutant`, « remise en rayon
carré »). Ma route B à κ = 2 en rayon redonne P_2 publié à l'identique (cellules et 64 entrées du catalogue) ; à
κ = 1 c'est la même formule, donc $P_1$. Conclusion : **$H_m$ = $P_1$ transposé en niveau carré, plus la
qualification $m$**, et H3/H4 = T5 transposé, avec une constante moins bonne.

## 7. Fixture nouvelle : la marge en niveau carré mélange les échelles

Nuage K = 2 : x = (0, 0, 0), y = (−2, 0, 0) (paire serrée, lentille de rayon 1), amas lointain
z = (2s, 0, 0), w1 = (2s + 2, 0, 0), w2 = (2s + 1, 2, 0) (`echelle_melangee.py`, oracle v11 et route B).

| s | $H_1$ (niveau carré) : entrée de x | $H_1$ en rayon : entrée de x | y |
| ---: | ---: | ---: | ---: |
| 10 | 4,690 | 2,000 | 1 |
| 100 | 14,213 | 2,000 | 1 |
| 1 000 | 44,744 | 2,000 | 1 |
| 10 000 | 141,428 | 2,000 | 1 |

*Calcul exact.* La lentille {x, z} couvre x dès le niveau $s^2$ et rejoint la lignée de {x, y} à $(s+1)^2$
(triplet {y, x, z}, diamètre 2s + 2). En niveau carré, $e=1+(s+1)^2-s^2=2s+2$ : x entre au rayon
$\sqrt{2s+2}$. En rayon, $e=1+(s+1)-s=2$. Un rival qui ne vit qu'une unité de rayon, à l'échelle s, retarde x d'un
facteur qui croît comme $\sqrt{s}$ par rapport à l'échelle propre de sa paire. La règle reste homogène (une
homothétie multiplie tout), mais elle n'est pas **locale en échelle** : un événement lointain et bref pèse comme
un rival proche et durable. Avec $m=3$, la paire n'est jamais qualifiée et x, y sont absorbés par l'amas lointain
au rayon s + 1/2 environ (complétion par un bord, le défaut que l'utilisateur refuse en Q-Π2).

La marge en rayon est donc la seule modification minimale et sans paramètre que je recommande pour $H_m$ :
même preuve (T5, constantes uniformes 3ε et 5ε, sans Λ), et gain partout ici (cellules 55 contre 35 à m = 1,
70 contre 70 à m = k + 1 ; catalogue 1 517 contre 1 437 et 1 732 contre 1 656). Elle ne répare aucun échec de
lignée et elle aggrave Q1bis à m = 1 (C entre dans CD à 1 637,382, dans la fenêtre : 0/10 au lieu de 5/10).

## 8. Lecture critique : thèse, verdict v10, auditeur, développeur

- **Développeur.** Construction propre, preuves justes, mais elle redécouvre $P_\kappa$ sans le dire et choisit la
  pire échelle de niveaux. La qualification $m=k+1$ est le seul ingrédient neuf ; elle répare T0/Q1 en cassant
  Q2, Q3, Q-Π2 et Q4. Le § 6 (G4) mesure le meilleur IoU de chaque objet parmi tous les blocs : ce critère tolère
  retards et mauvaises lignées ponctuelles, il ne contredit donc pas mes échecs (structure à fenêtres fixées).
  « HDBSCAN ne peut pas battre la tour » reste compatible avec ses chiffres ; « $H_{k+1}$ est la bonne
  hiérarchie de points » ne l'est pas avec les cibles ancrées.
- **Verdict v10.** ER0h passe 125/125, mais **dans l'échantillon** : sa région (η de 0,788 à 1,126 à κ = 12, κ de
  5,16 à 28,26 à η = 1) a été tirée de ces mêmes cellules ; sa continuité est conjecturée, ses pentes locales vont
  jusqu'à 103. Sur le catalogue (n ≤ 16), `first` le dépasse (1 991 contre 1 835), notamment sur les entrées à
  sommet commun d'un simplexe et d'un pont, où ER0h fait 0/25 (trois entrées à K = 4 et 5) et 0/45
  (`k3_pont_egalite`) ; mais les cibles à K ≥ 3 suivent la convention (b) de Q4, rejetée.
- **Cibles de l'utilisateur.** Elles sont fines. Q1 (K = 2) et Q4 (K = 3) ont la même forme : une k-partie propre
  née environ 15 % avant les k-parties rivales qui forment un (k+1)-simplexe. Q4 a **plus** de rivales (3 faces
  contre 2 lentilles), un simplexe rival **relativement plus précoce** (866,025/692,820 = 1,250 contre
  1 154,684/850 = 1,358) et des rivales nées à peine plus tard (1,1785 contre 1,1764) ; l'utilisateur répond (b)
  en Q1 et (a) en Q4. Seule une intégration du temps de couverture dans une bande de largeur fixée les sépare
  (les faces de Q4 vivent peu, les lentilles de Q1 longtemps). C'est une décision de modélisation légitime, mais
  elle impose un paramètre de bande ; je n'en connais pas de version sans paramètre (conjecture, non prouvée).
- **Thèse.** Le § 6 refuse de forcer une partition et le § 9.1 nomme l'objet (partition des (K−1)-simplexes,
  partition de l'unité $S_\tau/T_x$). Le théorème du § 4 donne raison à cette prudence : la première couverture,
  même qualifiée, ne suffit pas à lire une hiérarchie de points conforme aux réponses de l'auteur ; il faut un
  vote entre rivaux, ce que propose le § 9.1, à condition de le figer une fois (laminarité) et de le dater par une
  marge (continuité), comme ER0h. Les critiques du développeur contre le § 9.1 (départage non équivariant,
  changement de propriétaire) ne s'appliquent pas à ER0h (majorité stricte sans départage, engagement unique).
- **Auditeur.** Sa fermeture qualifiée sort de mon périmètre ; je note seulement que son seuil $m=k+1$ partage le
  défaut prouvé au § 4 (a) sur Q2.

## 9. Reproduction

Depuis `/workspaces/E-HGP/build/v11-points-math/marge_cibles/`, avec `PYTHONDONTWRITEBYTECODE=1` et
`python3 -B` (aucun bytecode écrit chez les autres) :

```bash
MHGP10_FIXTURES_SCRATCH=$PWD/scratch python3 -B /workspaces/E-HGP/build/v10-verrou-points/juge_final/verdict/cellules.py \
  --regle ER0h --eta 1 --kappa 12 --out recus/cel_ER0h_e1_k12_rejoue.json          # 6,3 s ; 125/125, bd7a1cbf0083cc95
python3 -B cellules_hm.py --regles ER0h_U --out recus/cellules_validation_ER0h_U.json    # 9 s ; 125/125, bd7a1cbf0083cc95
python3 -B cellules_hm.py --regles core,cover,first_k1,H1,Hk1,Hmcs,H1r,Hk1r --out recus/cellules_hm.json   # 24 s
python3 -B cellules_hm.py --regles P2r,P2sq,Hk1_k2r --out recus/cellules_famille_H4.json  # 16 s
python3 -B suites.py H1,Hk1,Hmcs,cover,ER0h_U                                           # suites condensées
python3 -B catalogue_hm.py --nmax 9 --kmax 4 --valider-er0h 8 --out recus/catalogue_hm_n9.json        # 3,6 s
python3 -B catalogue_hm.py --nmax 16 --kmax 5 --route-b --out recus/catalogue_hm_n16_b.json           # 251 s
python3 -B diag_rival.py k5_spheres_face_a_face__mcs2-7 base k1
python3 -B echelle_melangee.py
python3 -B verif_faits.py ; python3 -B propagation_h3.py ; python3 -B verif_d1.py
```

Empreintes (sha256 tronqué, sémantique des verdicts) : cellules `core` 7d0b5533e381bc2c, `cover`
0f1237d369e5f511, `first` cd0a115088ae2e9c, $H_1$ 0fbee6b1456bcb02, $H_{k+1}$ 36fe6de6320291fe,
$H_{\max(k+1,\mathrm{mcs})}$ 22633156a2370c45, $H_1$ en rayon e12e3ec42505311b, $H_{k+1}$ en rayon 2cac6422bedb83e5,
P_2 (route B) 3b544974524cfaca ; catalogue n ≤ 16 88bf5c1ee87d0271, n ≤ 9 ea7575691fc73d01. Catalogue v2 :
sha256 a68b54ead709998e… ; scripts : `adaptateur.py` 0c39f5dfe1c8df28, `cellules_hm.py` 260de1fce36c1814,
`catalogue_hm.py` ad464cd2ff00c0e3 (état final ; `catalogue_hm_n16.json`, sans route B, a été produit par une
version antérieure du même script et donne les mêmes nombres pour les règles de la route A).

Limites : cellules et catalogue sont de petits nuages (n ≤ 16), rôle d'oracle de correction et de cible, jamais
de pente ; aucune mesure aux tailles d'intérêt ; le catalogue est extrapolé ; rien ici ne qualifie un statut public.
