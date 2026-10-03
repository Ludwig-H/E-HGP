# Vérification adverse de « La règle H_m face aux cibles de l'utilisateur » (marge_cibles)

3 octobre 2026, 22 h 17 UTC (heure lue par `date -u`). CPU total des calculs : environ 15 min, dont 7 min de deux
passages complets du catalogue arrêtés (§ 3). Agent `verif_marge_cibles` du workflow « meilleure
méthode mathématique ». Cadre :

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé. Aucune construction ni test natif. Aucune commande git.
Lecture seule sur le dépôt, le worktree v11 et les dossiers v10 ; écritures sous verif_marge_cibles/ seulement.
```

Objet vérifié : `build/v11-points-math/marge_cibles/RAPPORT.md` (21 h 37) et ses 14 affirmations. Toute valeur
ci-dessous vient d'un calcul rejoué ici (commandes au § 6, reçus dans `recus/`) ou d'un fichier cité.

## En bref

| # | Affirmation du rapport | Verdict | Appui |
| --- | --- | --- | --- |
| 1 | Adaptateur valide (ER0h 125/125, empreinte `bd7a1cbf0083cc95` ; `core`, `cover`, P_2 = scores publiés) | **confirmé** (cellules ; catalogue sur les 47 entrées K ≤ 3) | rejeu v10 (même empreinte) ; ma route rend la table publiée de `VERDICT_FINAL.md` § 3.1 et les reçus natifs `cat_natif_lib.json` |
| 2 | Cellules : H_1 35, H_{k+1} 70, H_{max(k+1,mcs)} 70 sur 125 (n ≤ 9 : 30, 70, 70) | **confirmé** | route indépendante, même ventilation cellule par cellule |
| 3 | Routes A et B concordantes | **confirmé** (par une troisième route) | mêmes scores et mêmes dates sur tous les sites cités |
| 4 | Théorème Q1bis / Q2 : aucun seuil m = f(K, mcs) ne passe les deux | **confirmé** | faits (1)–(4) recalculés ; preuve relue |
| 5 | Lemme de propagation par H3 (minorant 1 926,33 sur T0_P1) | **confirmé** | fait (4) recalculé ; Λ valide ici |
| 6 | H4 en rayon = P_κ v10 ; H_1 = mutant `beta_mutant` ; T5 non cité | **confirmé** | P_2 70/125, même ventilation et mêmes dates (1 487,404 ; 54,844 ; 1 086,814) ; P_2 identique entrée par entrée aux reçus natifs sur les 47 entrées K ≤ 3 ; proposition 3.1 du mémo |
| 7 | F1, F2, H1, H3, borne de H4 justes | **confirmé**, avec la réserve du n° 14 | preuves rederivées |
| 8 | H4 surinterprétée (borne κ/2 de T6, κ = 1 le plus tardif) | **confirmé** ; constante κ/2 propre au rayon | T6 rederivé ; monotonie en κ |
| 9 | H2 : un rival tardif retarde rétroactivement | **confirmé** | x1 : 20 800 → 47 285,13 (rival dès 41 016,93, racine 59 039,25) |
| 10 | Échelle mélangée : x entre à √(2s+2) en niveau carré, à 2 en rayon | **confirmé** ; petite inexactitude sur y à m = 3 | niveaux exacts 2s + 2 |
| 11 | Catalogue n ≤ 16 (first 1 991, cover 1 832, H_{k+1} 1 656, …) | **confirmé entrée par entrée** sur les 47 entrées K ≤ 3 (1 484 jugements, 9 règles) et sur n ≤ 9 ; totaux à 64 entrées **non vérifiés** (17 entrées K = 4–5 non rejouées) | `recus/catalogue_kf3_*.json`, `recus/catalogue9_*.json` |
| 12 | Marge en rayon : « seule modification » qui améliore, « gagne partout » | **réfuté pour « partout »** ; chiffres des cellules confirmés | entrée `tetraedres_sommet_partage_K3__mcs4` ; n ≤ 9 à m = 1 : 250 contre 251 |
| 13 | H5 (m = max(k+1, mcs)) : dépend de mcs, échoue Q-Π2, incohérent avec le § 6 | **confirmé** | amas + x à 453,471 ; QPi2 0/5 |
| 14 | Conjecture : Λ = niveau de la racine suffit dans H3 | **réfuté** | nuage cocirculaire exact, deux routes ; bornes de H3 violées (6,8 δ) |
| 15 | Conjecture : Q1(b) et Q4(a) ne sont séparées que par une bande de temps de couverture | **non vérifié** | rapports de naissance exacts ; aucune preuve dans un sens ou dans l'autre |

**Verdict.** Le cœur du rapport tient : les scores, les dates, les faits de FULL et le théorème d'impossibilité du
§ 4 sont justes et reproduits par un code écrit de zéro. Deux corrections : (i) la conjecture qui rendrait H3 exacte
avec Λ = niveau de la racine est **fausse** (contre-exemple cocirculaire : une première couverture au-dessus de la
racine, et |Δe| = |Δu| = 6,80 δ, au-delà de 3δ et de 5δ) — la formule du développeur, qui identifie Λ à la racine,
est donc fausse en configuration cosphérique, fréquente sur une grille entière ; la marge en rayon (T5 v10, sans Λ)
n'a pas ce défaut ; (ii) la marge en rayon n'est **pas** meilleure partout : elle fait entrer chaque site au plus tôt
que la marge en niveau carré (lemme du § 4), ce qui crée des clusters interdits sur une entrée du catalogue.

## 1. Méthode

**Route indépendante.** `indep_full.py`, écrit de zéro, n'importe aucun code du rapport ni de l'oracle v11 :
L_K(a) est l'union des lentilles P_F(r) = ∩_{y∈F} B(y, r), |F| = K ; deux lentilles se coupent si et seulement si
MEB(F ∪ G) ≤ r ; la MEB est exacte (énumération des supports affinement indépendants de 2 à 4 points, `Fraction`) ;
l'arbre de fusion est construit par balayage des niveaux distincts en coupe fermée ; la couverture est
x ∈ E_C(a) ⟺ ∃ F ∈ C, MEB(F ∪ {x})² ≤ a, d'où c_x(ν) = max(h(ν), min_{F ⊂ sous-arbre(ν)} MEB(F ∪ {x})²).
Règles : `core`, `cover`/`first` (LCA des ex æquo), H_m en niveau carré et en rayon, famille H4 de pente κ, écrites
depuis le § 3 de `HIERARCHIE_POINTS.md`. Les décisions en rayon utilisent `vrad.R` (arithmétique exacte des sommes
de racines de la v10, sans logique de règle). `indep_full_j.py` est la même construction avec les seules arêtes de
Johnson (unions de K + 1 sites), même nerf.

**Auto-contrôles.** Contre l'oracle v11 (`hgp11_ref` + `points_reference`, témoin de mon code seulement) :
500 nuages aléatoires (n de 4 à 7, K = 2 ou 3, grilles de 4 à 30), 77 350 comparaisons d'ultramétriques exactes
(`core`, `cover`, `first`, H_1, H_{k+1}), 0 désaccord, mêmes niveaux de nœuds (`recus/test_indep.json`). Variante
de Johnson contre la variante par paires : 300 nuages, 1 500 ultramétriques, 0 désaccord.

**Juges.** Celui de la v10 (`ver.juger`, inchangé) et ma réécriture de la section 1.1 de `CIBLES_REVISEES.md`
(`cells.mon_conforme`), sur les mêmes points de contrôle : 0 désaccord sur tous les jugements (cellules et
catalogue n ≤ 9). Cellules ancrées recopiées de `verdict/cellules.py` v10 (modes `triangles`, `aucun`, `q4strict`).

## 2. Résultats confirmés

**Cellules ancrées (125 jugements), ma route** (`recus/cells_*.json`) :

| Règle | T0 | Q1 | Q1bis | Q2 | Q3 | Q-Π2 | Q4 | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `core` | 0 | 5 | 0 | 0 | 25 | 5 | 10 | 45 |
| `cover` (LCA) | 14 | 5 | 0 | 5 | 25 | 5 | 20 | 74 |
| `first` (m = k+1) | 40 | 10 | 10 | 0 | 0 | 0 | 10 | 70 |
| H_1 | 0 | 5 | 5 | 0 | 0 | 5 | 20 | 35 |
| H_{k+1} | 40 | 10 | 10 | 0 | 0 | 0 | 10 | 70 |
| H_{max(k+1,mcs)} | 40 | 10 | 10 | 0 | 0 | 0 | 10 | 70 |
| H_1 en rayon | 0 | 5 | 0 | 0 | 25 | 5 | 20 | 55 |
| H_{k+1} en rayon | 40 | 10 | 10 | 0 | 0 | 0 | 10 | 70 |
| H4, κ = 2, rayon (P_2) | 0 | 5 | 0 | 5 | 25 | 5 | 30 | 70 |
| H4, κ = 2, niveau carré | 0 | 5 | 0 | 5 | 25 | 5 | 30 | 70 |

Identique au tableau du rapport, et pour `core`, `cover`, P_2 à la table publiée de la v10 (`VERDICT_FINAL.md`
§ 3.1). Rejeu du code v10 : ER0h(1, 12) 125/125, empreinte `bd7a1cbf0083cc95` égale au reçu
`verdict/recus/cel_ER0h_e1_k12.json`.

**Dates** (`recus/dates.json`, toutes égales à celles du rapport) : T0_P1 H_1, C à 1 931,816 (racine 1 931,827) ;
T0_P2 H_1, CD à 1 930,355 ; Q1 H_1, C dans CD à 1 707,981 (rayon : 1 637,382 ; P_2 : 1 487,404) ; Q2 H_1, x à
67,373 (rayon : 65,052 ; P_2 : 54,844), H_{k+1}, {x, b1, b2} à 60,360 et a à 75,122 ; Q3 H_1, x à 744,180 (rayon :
696,120), H_{k+1}, x dans l'amas à 590,540, m = 9 : amas + x à 453,471 ; Q4 H_1, P, Q, R à 899,514 et CmD à
1 262,273 (rayon : 884,720), H_{k+1}, CPQR | DP2Q2R2 à 866,025 et m à la racine 1 334,166. Les rivaux qui fixent
ces retards sont ceux que le rapport nomme (Q4 : triplet mixte CPm né à 1 009,950, réuni à 1 078,174, sur une
première couverture de P à 816,497).

**Faits de FULL du théorème** (`recus/faits.json`) : (1) Q1 base, A_2(C) = 722 500, un seul nœud (lentille CD),
couverture {C, D} jusqu'à son parent, la racine 3 194 656 ; (2) Q2 base, les seuls nœuds qui couvrent a jusqu'à
5 625 sont la lentille {a, x} (couverture {a, x} jusqu'à la racine 90 625/16) et la lentille {a, b1} née à 5 625 ;
le premier niveau où un nœud couvre a et au moins trois sites est 144 671 725/25 636 (rayon 75,122) ; (3) Q4 base,
A_3(C) = 480 000, un seul nœud (CmD), couverture {C, m, D}, parent la racine 1 780 000 ; (4) T0_P1 D−1, première
couverture unique de C à 3 996 001/4 (lentille CD), sa lignée ne couvre A qu'à 14 920 361/4 (rayon 1 931,344).
La preuve du § 4 est correcte : (a) a ne peut entrer avant 3 721 sous qualification m ≥ 3, donc le bloc
obligatoire {x, a} manque à r = 61 ; (b) par F1, C reste dans {C, D} sous la racine, donc le bloc obligatoire ABC
manque à 72 079 426/25 ; (c) même couple (K, mcs) = (2, 2).

**Lemme de propagation.** Base et D−1 ont la même racine 3 731 956 ; toutes les premières couvertures sont
≤ 1 000 000 ; Λ = 14 935 289/4 majore donc tous les niveaux transportés, δ = 2√Λ + 1 = 3 865,62, et
14 920 361/4 − 5δ = 3 710 762,1 (rayon 1 926,33). H_1 vérifie : u(C, A) = 3 731 912 dans la base (1 931,816).

**P_κ.** La proposition 3.1 du mémo `ancrage_marges` donne t_{P_κ} = max(α, sup_{r ≥ α}(M_x(r) − κ(r − α))) ;
avec M_x(r) = niveau de rencontre de la lignée de première couverture avec toutes les composantes qui couvrent x
à r, c'est exactement H4 en rayon à m = 1. `ownership.rule_pkappa(..., beta_mutant=True)` calcule
`level(J) − κ(c − α²)` en niveau carré : c'est H_1 à κ = 1. Le document du développeur ne cite ni P_κ, ni T5,
ni le mémo.

**Preuves.** F1, F2, H1 relues ; H3 rederivée (D^Y ≤ D^X + 2δ via p_i^X, t^Y ≤ t^X + δ, rencontre des
propriétaires avant e^X + 5δ, ultramétrie) ; H4 : (1 + 2κ)δ et (1 + 4κ)δ rederivées. T6 du mémo rederivé
(réflexion de F2^±, δ_0 = L/(2κ − 1), rapport κ/2) ; e_κ décroît en κ, donc κ = 1 est le membre le plus tardif.
Réserve : T6 porte sur la précocité **en rayon** ; transposée à la famille du développeur (précocité en niveau
carré), la même famille F2 donne encore Θ(κ), avec une constante plus faible (de l'ordre de κ/4 avec la
normalisation δ = 2ε√Λ + ε², calcul à la main sur F2, non rejoué). La conclusion « précocité et stabilité
s'échangent en Θ(κ) » tient.

**Rival tardif (H2).** `k5_spheres_face_a_face`, base, K = 5, m = 6 (`recus/rival_k5.json`) : x1 couvert par sa
sphère à 20 800 ; rival maximal couvrant x1 dès 41 016,93 et rejoignant sa lignée à la racine 59 039,25 ; entrée à
47 285,13 > 23 401. À m = 1 : 47 766,90. Les six sites de la sphère a sont tous entrés à 12 493,862 au plus tard
(leurs seuls rivaux sont internes au sous-arbre de la sphère ; `recus/rival_k5_tous.json`). Lecture : sous la
lecture grammaticale de H2 (« avant de rejoindre C » porte sur le site, donc sur les niveaux ≤ a), l'hypothèse est
satisfaite à a = 23 401 et la conclusion est fausse (x1 manque au bloc) ; sous la lecture « avant que la rivale
rejoigne C », l'énoncé est juste mais non local en échelle. Le rapport dit les deux ; c'est exact.

**Échelle mélangée** (`recus/echelle.json`) : niveau d'entrée de x exactement 2s + 2 (rayons 4,690 ; 14,213 ;
44,744 ; 141,428), 2 en rayon, y à 1. Inexactitude mineure : à m = 3, x entre vers s + 1/2 (√(s² + s + 5/4)),
mais y n'entre qu'à la racine s + 1, pas « vers s + 1/2 ».

## 3. Catalogue v2

Sous-ensemble n ≤ 9 (19 entrées, 331 jugements), ma route complète (`recus/catalogue9_*.json`) : `core` 256,
`cover` 301, `first` 323, H_1 251, H_{k+1} 293, H_{max(k+1,mcs)} 292 — **identiques** au rapport. En plus :
H_1 en rayon 250, H_{k+1} en rayon 294, P_2 283.

Catalogue n ≤ 16, entrées à K ≤ 3 (47 des 64 entrées, 1 484 des 2 013 jugements), ma route complète (variante
de Johnson, juge v10 ; `recus/catalogue_kf3_*.json`, 112 s et 135 s) : pour les neuf règles (`core`, `cover`,
`first`, H_1, H_{k+1}, H_{max(k+1,mcs)}, H_1 en rayon, H_{k+1} en rayon, P_2), les résultats sont **identiques entrée
par entrée** au reçu du rapport (`marge_cibles/recus/catalogue_hm_n16_b.json`, lu comme donnée) et, pour `core`,
`cover` (= `A5_U1`) et P_2, aux reçus natifs de la v10 (`verdict/recus/cat_natif_lib.json`). Totaux sur ces 47
entrées : `core` 1 098, `cover` 1 356, `first` 1 468, H_1 1 075, H_{k+1} 1 297, H_{max(k+1,mcs)} 1 235, H_1 en
rayon 1 155, H_{k+1} en rayon 1 373, P_2 1 304.

Les 17 entrées à K = 4 et 5 (529 jugements) n'ont pas été rejouées : un passage complet a été arrêté après
5 min 30 s de CPU pour tenir le budget (MEB de six points en Fraction). Les totaux à 64 entrées du rapport
(`first` 1 991, `cover` 1 832, H_{k+1} 1 656, …) ne sont donc pas vérifiés ici ; aucune des 66 entrées rejouées
(n ≤ 9 et K ≤ 3) ne s'écarte de son reçu. Sur les 47 entrées, la marge en rayon gagne sur 9 entrées (m = 1) et
5 (m = k + 1) et perd sur une seule, la même pour les deux m (§ 4.2) ; H_{max(k+1,mcs)} perd contre H_{k+1} sur 11
entrées et gagne sur 3.

## 4. Réfutations et compléments

### 4.1 Conjecture « Λ = niveau de la racine suffit » : fausse (n° 14)

Nuage cocirculaire à K = 3 : x = (0, 221, 0), s = (0, −221, 0), p = (−21, −220, 0), q = (21, −220, 0)
(21² + 220² = 221², s antipode de x). Les trois 3-parties contenant x ont pour MEB le disque du cercle (rayon 221 :
{x, p, q} acutangle, {x, p, s} et {x, q, s} rectangles en p, q) ; cette boule contient {p, q, s}, née à 21² = 441.
La lentille de x naît donc **collée** : FULL_3 n'a qu'un nœud, né à 441, qui est la racine, et x n'est couvert
qu'à 48 841. Mon code et l'oracle v11 concordent (nœuds, dates de H_1 et H_4, ultramétriques ;
`recus/lambda_racine.json`). Une première couverture peut donc dépasser la racine.

Conséquence sur H3 : X = 10P, Y = 11P (déplacement ε = 221 pour chaque site, |P_i| = 221). Racines 44 100 et
53 361 ; avec Λ = max des racines, δ = 2ε√Λ + ε² = 150 943. H_1 et H_{k+1} (= H_4) : |Δe| = |Δu| = 1 025 661
= 6,80 δ > 5δ > 3δ. Avec c = 30 : 8,85 δ. Avec Λ = max des premières couvertures, δ' = 1 123 343 et
|Δu|/δ' = 0,91 : la borne tient. La phrase du développeur « des niveaux au plus Λ (niveau de la racine) » est donc
fausse dans les configurations cosphériques ; la condition correcte est que Λ majore aussi les premières
couvertures (qualifiées) des deux nuages. En position générale l'argument du rapport tient : si la boule minimale
F* de x contenait strictement une autre K-partie, retirer un point de support ≠ x de F* et ajouter le point
intérieur donnerait une K-partie contenant x de MEB strictement plus petite, contradiction ; la lentille de x naît
alors isolée, c'est une feuille, donc A_K(x) ≤ racine. Le défaut est propre aux égalités cosphériques, que la
grille entière produit. La marge en rayon (T5, entrelacement uniforme en ε) n'a besoin d'aucun Λ.

### 4.2 La marge en rayon n'est pas meilleure partout (n° 12)

**Lemme.** À même première couverture qualifiée t et mêmes rivaux (t ≤ c_q ≤ M_q), l'entrée en rayon
e_r = √t + max_q(√M_q − √c_q) vérifie e_r ≤ √(t + max_q(M_q − c_q)) = √e_sq. *Preuve.* Terme à terme,
(√t + √M − √c)² ≤ t + M − c ⟺ (√c − √t)(√c − √M) ≤ 0. □ Les deux règles ont la même lignée ; la marge en rayon
fait entrer chaque site **au plus tôt** que la marge en niveau carré. Elle ne peut donc réparer que des retards, et
elle peut créer des entrées trop précoces.

Contre-exemple à « gagne partout » : `tetraedres_sommet_partage_K3__mcs4`, variante `gigue_totale`, cible « aucun
cluster » sur [866,025 ; 1 279,204[ (S libre) : H_1 en rayon forme ABCS à 1 278,506, H_{k+1} en rayon forme EFGS à
1 278,498, alors que les versions en niveau carré passent (`recus/partout.json`). Sur le sous-ensemble n ≤ 9 :
H_1 en rayon 250 contre H_1 251 ; H_{k+1} en rayon 294 contre 293. La recommandation « marge en rayon » reste
défendable (stabilité uniforme sans Λ, fin du mélange d'échelles, gain agrégé sur les cellules), mais pas
« gain partout », et « la seule modification minimale » n'est pas démontrée.

### 4.3 Ce qui reste non vérifié

- n° 15 (Q1(b) et Q4(a) séparées seulement par une intégration de temps de couverture dans une bande) : conjecture ;
  les rapports de naissance sont exacts (999,978/850 = 1,17644 ; 1 154,684/850 = 1,35845 ;
  816,497/692,820 = 1,17851 ; 866,025/692,820 = 1,25), aucune preuve d'impossibilité.
- Le caractère « nouveau » de la pathologie d'échelle (n° 10) : le développeur signalait déjà la dépendance de la
  constante de H3 à l'échelle (§ 5, limite 2), et la v10 traitait la version en niveau carré comme un mutant.

## 5. Lecture critique

- Le théorème du § 4 est l'apport le plus solide : il ne dépend que de deux faits exacts de FULL, que je retrouve
  par une construction indépendante, et de la sémantique écrite des cibles. Il ne vaut que pour m = f(K, mcs) et
  pour des règles qui suivent la lignée de la première couverture qualifiée ; une règle qui compare les rivaux (ER0h)
  y échappe, ce que le rapport dit.
- Le rapport est plus sévère avec H2 et H4 qu'avec H3 ; c'est H3 qui contient la seule affirmation fausse du
  document du développeur (Λ = racine), et le rapport l'a laissée en conjecture favorable.
- Les scores du catalogue classent sur des cibles extrapolées ; l'entrée qui fait perdre la marge en rayon est une
  variante de gigue près du bord droit d'une fenêtre dérivée : effet de bord, mais réel.
- **Document du développeur révisé après le rapport.** `HIERARCHIE_POINTS.md` a été réécrit à 22 h 01 (le rapport
  lisait la version de 21 h 18) : il retient désormais H^r_{k+1} (P_1 de la v10 en rayon sur la qualification
  Π_{k+1}), cite le mémo v10, retire m = max(k + 1, mcs) et appelle la version en niveau carré « Q_1 de la v10, une
  régression » ; `bench/points_radius.py` est apparu à 21 h 55. Le rapport vérifié reste exact pour la version qu'il
  juge, et ses recommandations (1) et (2) y sont reprises. Ma réfutation du n° 14 vise la version de 21 h 18 et la
  conjecture du rapport ; elle ne touche pas la version en rayon, dont l'entrelacement est uniforme. Le contre-exemple
  du § 4.2 touche en revanche la règle désormais retenue (H^r_{k+1} perd un jugement que H_{k+1} passe).

## 6. Reproduction

Depuis `build/v11-points-math/verif_marge_cibles/`, avec `PYTHONDONTWRITEBYTECODE=1 python3 -B` :

```bash
python3 -B test_indep.py 500        # 20 s : 77 350 comparaisons, 0 désaccord contre l'oracle v11
python3 -B test_j.py                # 16 s : variante de Johnson = variante par paires
python3 -B faits.py                 # faits (1)-(4)
python3 -B cells.py core,cover,first,H1,Hk1,Hmcs     # 8 s
python3 -B cells.py H1r,Hk1r,P2r,P2sq                # 9 s
python3 -B dates.py ; python3 -B echelle.py ; python3 -B partout.py
python3 -B rival_k5.py ; python3 -B rival_k5_tous.py    # 10 s chacun (K = 5, n = 14, aretes de Johnson)
python3 -B lambda_racine.py         # contre-exemple cocirculaire, mon code et oracle v11
python3 -B catalogue9.py core,cover,first,H1,Hk1,Hmcs,H1r,Hk1r,P2r   # 36 s
python3 -B test_hf.py                                                # lecteur rapide = lecteur simple (261 partitions)
python3 -B catalogue_kf.py core,cover,first,H1,Hk1,Hmcs 3 fast      # 112 s, K <= 3, n <= 16, ecarts aux recus
python3 -B catalogue_kf.py H1r,Hk1r,P2r 3 fast                      # 135 s
# catalogue16.py (toutes les entrees, K <= 5) : arrete apres 5 min 30 s de CPU, non utilise
MHGP10_FIXTURES_SCRATCH=$PWD/scratch python3 -B \
  /workspaces/E-HGP/build/v10-verrou-points/juge_final/verdict/cellules.py --regle ER0h --eta 1 --kappa 12 \
  --out recus/cel_ER0h_e1_k12_rejoue_verif.json      # 125/125, bd7a1cbf0083cc95
```

Limites : nuages de n ≤ 16, rôle d'oracle de correction ; aucune mesure aux tailles d'intérêt ; rien ici ne
qualifie un statut public.
