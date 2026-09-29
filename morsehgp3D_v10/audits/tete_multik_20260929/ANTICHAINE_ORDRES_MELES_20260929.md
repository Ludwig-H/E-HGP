# Antichaîne à ordres mêlés sur la tour : chaque amas à son ordre (DEV, 29 septembre 2026)

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=dev_seulement (plan('dev', [2000, 8000], 2), réplicat 0 : 128 scènes)
public_status=not_claimed
GCP non utilisé. Aucun fichier du dépôt modifié, aucun commit. Aucune graine test ni test_v10b.
Tour : binaire figé /workspaces/E-HGP/build/v10-dev-multik/mhgp10_tower (sha256 272477f2…c542e9), KMAX = 10,
entrée cover, 1 fil. Rôle : concepteur « antichaîne à ordres mêlés » (un des trois concepteurs multi-K).
```

## 0. En bref

- **Construit.** Un graphe des amas candidats de tous les ordres k = 2..KMAX. Les candidats sont les clusters
  condensés de chaque tranche (mcs = √n, entrée cover). Ils sont reliés par l'inclusion de leurs points, qui raffine
  les verticales L_k(r) ⊆ L_{k−1}(r) : contrôle sur 25 904 clusters (2 000 et 8 000 points), 100 % cohérent
  (§ 2.3).
- **Sélection.** Un programme dynamique choisit une antichaîne où chaque amas final a son propre ordre et son
  propre niveau. Restreint à un seul ordre, il redonne exactement l'EOM de la tête v10 : 0 écart sur 768 contrôles.
- **Fonctionnelle.** L'excess of mass de chaque candidat, dans une échelle commune à tous les ordres : le contenu de
  probabilité (Rinaldo et al. 2012), transporté sur l'échelle de l'ordre de référence KMAX. Sans échelle commune,
  l'ordre 2 gagne presque partout (§ 3.1). Deux règles d'agrégation sur les ordres :
  - `mixq` : maximum, soit la sélection d'échelle de Lindeberg avec k pour échelle ;
  - `mixq_sum` : somme, soit le volume de l'amas dans la bifiltration, motivé par la stabilité de Blumberg–Lesnick.
- **Mesure (128 scènes dev, KMAX = 10, même z, même remplissage que la tête à une tranche).**
  - `mixq` gagne +0,005 à +0,009 pour z ∈ {4, 5, 6}, avec ou sans remplissage, et +0,013 à +0,014 à z = 8. L'IC à
    95 % exclut 0. Pour z = 4–6, il gagne 14 à 24 scènes et en perd 2 à 6. À z = 3, rien (±0,001).
  - À 8 000 points (la taille des tests) : +0,006 à +0,007 à z = 5 et z = 6, avec ou sans remplissage. Dans la
    configuration du lot C (z = 6, b1.5), 0,7965 contre 0,7901, IC [+0,002 ; +0,012], 9 scènes gagnées pour 1
    perdue.
  - `mixq_sum` gagne +0,016 à +0,027 à 2 000 points avec remplissage (z = 5–8), mais rien à 8 000 points (−0,003 à
    −0,001 à z = 5–6). Il n'est pas robuste à la taille des tests.
  - **Tous ces gains restent sous le delta_min = 0,02 du lot C.** Ce n'est pas un saut de performance.
- **Où, et pourquoi.**
  - Coquilles : +0,007 à +0,069 selon la taille et z. Les 8 coquilles sont des branches à tous les ordres. L'EOM de
    l'ordre 10 retient leurs parents, alors que les ordres 3–4 les retiennent ; 72 % des amas finaux y sont pris à
    k ≤ 5.
  - Filaments : +0,023 à 8 000 points et z = 5, mais −0,004 à 2 000 points et quasi nul à z = 6 avec b1.5.
  - Familles déséquilibrée et hétéroscédastique : +0,005 à +0,020.
  - Neutre (±0,002) sur les gaussiennes sphériques et anisotropes, `bridge` et `hierarchical` : la sélection y
    reprend l'ordre 9–10.
- **Objection de l'utilisateur (« si le mélange est séparable, on devrait identifier les modes »).** Diagnostic avec
  la vérité (§ 5.4) :
  - sur les quatre familles gaussiennes, tout groupe qu'un amas de la tour recouvre (Jaccard ≥ 0,5) est retenu par
    la tête à une tranche aux niveaux easy, medium et hard : 122/122, 107/107, 94/94 ;
  - les groupes manquants ne sont pas des amas de l'arbre, à aucun ordre ≤ 10 : groupes plus petits que mcs = √n,
    groupes diffus, modes fusionnés au niveau extrême ;
  - mêler les ordres n'y change presque rien (+1 groupe retenu). L'écart au plafond de Bayes tient à l'appartenance
    sous le col et au régime K ≤ 10, pas à l'ordre (audit, § 1.2) ;
  - mêler les ordres aide sur d'autres modes : ceux que K = 10 contient sans que son EOM les retienne (coquilles :
    128/128 au lieu de 123) et ceux que les petits ordres séparent mieux (filaments).
- **Limites.**
  - Rien n'est préenregistré. Les graines dev ont été vues pendant la conception. Une variante (le départage des
    égalités) a été corrigée après une première mesure, et un défaut du programme dynamique (double compte) après
    la deuxième. Les chiffres sont ceux du code corrigé, qui abaisse le gain de `mixq` de 0,0006 (2 000 points) et
    0,0024 (8 000 points) en moyenne (§ 6).
  - À z = 2, le maximum sur les ordres amplifie la préférence de l'EOM pour les grands blocs.
  - À KMAX = 5, il y a moins d'ordres à mêler et le gain est plus faible : +0,003 à +0,007 à z = 5 (§ 5.6).

## 1. Question et cadre

- **Acquis de l'audit du 29 septembre** (`AUDIT_HIERARCHIE_KNN_20260929.md`) :
  - la tranche d'ordre K de la tour est l'arbre plug-in exact de l'estimateur K-NN ;
  - la tête v10-b n'en lit qu'une tranche ;
  - l'axe K est le seul atout propre non exploité : verticales, stabilité de la multicouverture ;
  - une tranche à K fixe est instable (Rolle–Scoccola), ce qui est mesuré : les coquilles sont retenues à K = 8 et
    perdues à K = 10.
- **Mon rôle** : laisser chaque amas final être pris à son propre ordre. Une vallée nette ou un objet mince peut être
  mieux résolu à petit k, une région bruitée à grand k. Il faut :
  - un graphe des candidats de tous les ordres, relié par les verticales ;
  - une sélection par une fonctionnelle fondée, sans réglage par scène sur la vérité ;
  - une mesure honnête face à la tête à une tranche au même KMAX.
- **Protocole commun aux trois concepteurs.**
  - Les 128 scènes de `run_campaign.plan('dev', [2000, 8000], 2)`, réplicat 0 : 8 familles × 4 niveaux ×
    bruit {0 ; 0,1} × n {2 000 ; 8 000}.
  - KMAX = 10, avec KMAX = 5 en complément.
  - J = moyenne de l'ARI_s pondérée par cellule. Au réplicat 0, une cellule est une scène : J est une moyenne simple.
  - Deux politiques de remplissage : `none` et `b1.5` (`methods.bounded_fill`, k = max(KMAX, 5), le même pour
    toutes les méthodes d'un même KMAX).
  - Comparaison appariée à la tête v10-b à une tranche, au même z. sklearn HDBSCAN à min_samples = KMAX, lu dans les
    CSV dev existants, sur les mêmes graines.

## 2. L'objet : tranches, verticales, candidats

### 2.1 Tranches et condensation

- **Tranches et inclusion.** L_k(r) = {y : |B̄(y, r) ∩ X| ≥ k}. Une boule qui contient k points en contient k − 1,
  d'où L_k(r) ⊆ L_{k−1}(r).
- **Arbres.** La tour donne, pour chaque ordre, l'arbre exact T_k des composantes de L_k(r). Chaque point x y entre
  au rayon α_k(x) (entrée cover, α_k ≤ d_k), dans la composante de sa première boule couvrante.
- **Condensation.** Chaque T_k est condensé exactement comme dans la tête v10 (mcs = round(√n)), ce qui donne l'arbre
  condensé CT_k. Un cluster condensé c a :
  - un ensemble de points S(c) : sa masse à la naissance ;
  - un niveau de naissance b_c ;
  - une liste d'événements (m_e, ℓ_e) : un point qui sort à son niveau d'entrée, un sous-arbre de masse < mcs
    abandonné, ou un enfant de masse ≥ mcs à la scission.
- **Stabilité (excess of mass).** Avec ℓ le rayon carré et λ(ℓ) = ℓ^(−z/2) :

$$\sigma_z(c) = \sum_{e} m_e \, \lambda(\ell_e) - M_c \, \lambda(b_c)$$

- **Implémentation.** La condensation est écrite sous forme d'événements, ce qui donne σ pour tout z sans
  recondenser. Elle reproduit exactement la tête du binaire : 0 écart sur 3 072 comparaisons (scène, K, z,
  remplissage) contre `kcover_dev_zgrid*.csv`.

### 2.2 Verticales

- **Définition.** Pour c ∈ CT_k, φ(c) ∈ CT_{k−1} est le cluster condensé qui contient la composante de L_{k−1}(r)
  où vit la composante de c, juste sous sa naissance. On l'obtient par le pointeur `lower` du nœud de tête de c,
  puis par la remontée des parents jusqu'au niveau b_c.
- **Garantie.** C'est l'inclusion exacte de la bifiltration : composante_k(c, r) ⊆ composante_{k−1}(φ(c), r).
- **Vers les ordres inférieurs, pas d'inclusion.** La bifiltration ne donne aucune inclusion de L_{k'} dans L_k
  quand k' < k. Un amas d'ordre 3 n'est donc « dans » un amas d'ordre 10 que par ses points.

### 2.3 Graphe des candidats et contrôle des verticales

- **Candidats.** Tous les clusters condensés non racines des ordres 2..KMAX. L'ordre 1 est exclu : toutes les entrées
  y sont au niveau 0 et c'est la liaison simple.
- **Arcs.** Un arc c → d relie c à d quand S(d) ⊂ S(c) à ε = 10 % de S(d) près et |S(d)| < |S(c)|. Les
  intersections sont calculées d'un coup par un produit de matrices d'incidence creuses.
- **Contrôle (`check_verticals.py`).**
  - Emboîtement le long des verticales (entrée cover comprise) : ρ(c) = |S(c) ∩ S(φ(c))| / |S(c)|.
    - À 2 000 points, sur 11 206 clusters des ordres 3 à 10 : médiane 1,000, minimum 0,957, 100 % des clusters à
      ρ ≥ 0,9.
    - À 8 000 points, sur 14 698 clusters : médiane 1,000, minimum 0,933, 100 % à ρ ≥ 0,9.
  - Le substitut ensembliste retrouve l'homologue, sous la verticale. Le plus petit cluster d'ordre k − 1 qui
    contient au moins 90 % de S(c) :
    - n'est φ(c) que dans 10 % (2 000 points) à 15 % (8 000 points) des cas ;
    - est toujours dans le sous-arbre de φ(c) (100 %).
  - Lecture : à même rayon, L_{k−1} est plus connexe que L_k. L'homologue de c à l'ordre k − 1 se sépare donc plus
    bas (à un rayon plus petit), et la verticale prise au niveau de naissance de c tombe le plus souvent sur son
    parent. L'inclusion ensembliste raffine donc les verticales : c'est le graphe utilisé.

## 3. La fonctionnelle

### 3.1 Il faut une échelle commune aux ordres

- **Le problème.** À densité f fixée, le rayon d'ordre k croît comme r_k ∝ (k/f)^(1/m). Des stabilités en r^(−z)
  d'ordres différents ne sont donc pas comparables.
- **Mesure.** Sans normalisation (λ = r^(−z) brut), la sélection prend l'ordre 2 presque partout. Mesure
  exploratoire sur 13 scènes de 2 000 points (variante `none`) :
  - elle perd 0,03 à 0,11 sur les 7 scènes gaussiennes et `bridge` ;
  - elle gagne sur les coquilles et les filaments, où le petit ordre est le bon, mais pour une mauvaise raison
    (l'inflation des petits ordres, § 3.2).

### 3.2 Les unités de densité ne suffisent pas

- **Forme.** f̂_k = k/(n v_m r^m), d'où λ = f̂_k^(z/m) = (k/(n v_m))^(z/m) r^(−z). Deux défauts :
  - il faut la dimension m, qui change d'une famille à l'autre (1, 2 ou 3) ;
  - les queues sont lourdes aux petits ordres.
- **Les queues.** Sous l'approximation poissonnienne locale, n f v_m d_k^m suit une loi Γ(k, 1), et

$$\mathbb{E}\left[ d_k^{-z} \right] \propto \frac{\Gamma(k - z/m)}{\Gamma(k)}$$

- **Conséquence.** Cette espérance est infinie dès que z ≥ m k : pour z = 6 et m = 3, dès k ≤ 2. Une somme de λ sur
  les points est alors dominée par les paires les plus proches. Tout maximum sur les ordres favorise les petits
  ordres (inégalité de Jensen).
- **Variante « rapport des médianes ».** Elle corrige la position mais pas la queue (variante `mixs`, § 5.3).

### 3.3 Le contenu de probabilité, transporté sur l'ordre de référence

- **Contenu.** À l'ordre k, un niveau ℓ porte la masse p_k(ℓ) = #{x : α_k(x)² ≤ ℓ}/n. C'est la masse empirique de
  l'ensemble dilaté {α_k ≤ r} = X ∩ (L_k(r) ⊕ B̄(r)).
- **Fondement.** Rinaldo, Singh, Nugent & Wasserman (JMLR 2012, § 3.2) indexent l'arbre des amas par ce contenu. Il
  est invariant par reparamétrisation du niveau et ne demande pas m.
- **Transport.** On transporte chaque ordre sur l'échelle de l'ordre de référence KMAX, par une interpolation
  linéaire en log ℓ entre statistiques d'ordre :

$$\ell \mapsto F_{\mathrm{ref}}^{-1}\left( F_k(\ell) \right), \qquad \lambda = \left( F_{\mathrm{ref}}^{-1}(F_k(\ell)) \right)^{-z/2}$$

- **Propriétés.**
  - L'application est strictement croissante : l'arbre de chaque ordre est inchangé, seules les hauteurs changent.
  - Elle vaut l'identité à l'ordre KMAX : la tête à une tranche est le cas particulier K_set = {KMAX}.
  - Dans une région homogène, F_k(ℓ) ≈ F_ref(c_k ℓ). Le transport est alors le changement d'échelle des unités de
    densité, avec un c_k estimé sans connaître m.
  - Dans les queues, la queue lourde des petits ordres est ramenée sur celle de la référence : l'inflation de
    Jensen disparaît.
- **Point faible.** Au-dessus de la dernière entrée (fusions proches de la racine), le transport extrapole par
  translation en log ℓ.

### 3.4 Sélection : programme dynamique sur les décompositions mêlées

- **Décompositions et créneaux.**
  - Pour un candidat c et un ordre k', D_{k'}(c) est l'ensemble des clusters maximaux d'ordre k' strictement
    contenus dans c.
  - Les décompositions de tous les ordres sont regroupées en créneaux : les composantes connexes du graphe de
    recouvrement, avec un recouvrement d'au moins 10 % du plus petit.
  - Dans chaque créneau, on garde un seul ordre.
- **Valeur.**

$$V(c) = \max\left( \sigma(c), \; \sum_{s} \max_{k'} \sum_{d \in D_{k'}(c) \cap s} V(d) \right)$$

- **Univers et racine.** L'univers se traite de même, avec les enfants des racines. La racine est exclue, comme dans
  la tête v10 (allow_single = False). Le parent l'emporte en cas d'égalité, comme dans l'EOM.
- **Deux fonctionnelles σ, dans l'échelle commune du § 3.3.**
  - **`mixq` (maximum).** σ(c) = σ_{k(c)}(c). Dans chaque créneau, l'ordre qui donne la plus grande valeur l'emporte.
    C'est le principe de sélection automatique d'échelle de Lindeberg (IJCV 30(2), 1998) : une structure est
    représentée à l'échelle où sa réponse normalisée est maximale. Ici l'échelle est k et la réponse l'excess of
    mass.
  - **`mixq_sum` (somme).** σ(c) = Σ_{k'} σ_{k'}(h_{k'}(c)), où h_{k'}(c) est le candidat d'ordre k' de Jaccard
    maximal avec c, s'il dépasse 0,7. C'est l'excess of mass intégré le long de l'axe des ordres : le volume de
    l'amas dans la bifiltration (r, k).
    - Motivation : la multicouverture est stable en distance de Prohorov (Blumberg–Lesnick, FoCM 2024), ses tranches
      ne le sont pas (Rolle–Scoccola, JMLR 2024).
    - Intégrer sur k moyenne l'instabilité d'une tranche. Exemple : les coquilles, perdues à K = 10 et présentes à
      K ≤ 8.
- **Égalités entre ordres.** Même piste, même valeur : on prend le plus grand ordre, l'estimateur le plus lisse
  (principe de Lepski : entre deux estimations indiscernables, la moins variable). Avec `mixq_sum`, les membres d'une
  même piste ont la même valeur. Le départage fixe alors les points de l'amas final ; la variante `mixq_sumlo`, qui
  prend le plus petit ordre, en donne l'effet.
- **Paramètres.** Tous sont fixés a priori, aucun n'est réglé sur la vérité :
  - ε = 0,1 (inclusion) ;
  - seuil de recouvrement 0,1 (créneaux) ;
  - τ = 0,7 (homologues).
- **Sensibilité.** Elle est mesurée à ε = 0,05 et 0,2 (§ 5.3).
- **Points contestés.** Un point peut appartenir à deux amas choisis, à cause de la tolérance ε. Il va à l'amas où il
  reste le plus longtemps en λ normalisé : son propre événement s'il y est le plus profond, sinon la scission de
  l'amas.

### 3.5 Ce qui n'est pas fondé

- **Aucun théorème de consistance.** Ni le maximum ni la somme n'en ont.
- **Malédiction du gagnant.** Le maximum sur 9 ordres de valeurs bruitées est biaisé vers l'alternative la plus
  variable (§ 6).
- **La somme suppose l'appariement des homologues.** Le seuil τ est conventionnel.
- **La vraisemblance n'est pas utilisée**, pour trois raisons :
  - la vraisemblance croisée (leave-one-out) de l'estimateur K-NN est biaisée vers les petits k. Sous Poisson,
    E[log f̂_k] − log f = log k − ψ(k), quantité positive et décroissante ;
  - l'estimateur K-NN n'est pas intégrable ;
  - elle juge la densité, pas la partition. À K ≤ 10, elle choisirait KMAX partout dans les régions lisses.

## 4. Prototype (dossier `/workspaces/E-HGP/build/v10-persist/multik/ordres_meles/`)

### 4.1 Fichiers

| fichier | rôle |
|---|---|
| `build_cache.py` | exécute le binaire figé (KMAX = 10, cover, 1 fil) sur les 128 scènes ; lit le dump, le remet dans l'ordre du nuage (le dump est en ordre de Morton) ; cache npz hors de /workspaces |
| `import_cache.py` | reprend les arbres déjà calculés par un autre concepteur (même binaire), après contrôle bit à bit de G et T ; structure égale à notre propre dump, niveaux à un ulp près ; 0 écart au binaire (§ 2.1) |
| `multik_lib.py` | condensation par événements, EOM v10, étiquettes, verticales, échelle par contenu de probabilité (`LevelMap`) |
| `mixed_orders.py` | `MixedScene` : candidats, décompositions, créneaux, σ, programme dynamique, étiquettes |
| `evaluate.py`, `report.py`, `summarize.py` | évaluation sur le sous-ensemble commun ; tableaux |
| `check_verticals.py`, `check_dp_single_order.py`, `modes_present.py` | contrôles et diagnostic |
| `TABLEAUX_DEV_20260929.md` | tous les tableaux (J par z et par taille, écarts appariés avec IC, familles, ordres, KMAX = 5), code corrigé |
| `eval_n2000_v3.csv.gz`, `eval_n8000_v3.csv.gz` | résultats bruts par scène, code corrigé (chiffres du document) |
| `eval_n2000_v2.csv.gz`, `eval_n8000_v2.csv.gz`, `eval_n2000_v1_premiere_passe.csv.gz` | passes précédentes, gardées pour la traçabilité (§ 6) |
| `modes_present_z5.csv` | diagnostic modes présents / retenus, par scène (code corrigé) |
| `journaux/` | journaux des constructions, évaluations et contrôles |

### 4.2 Rejouer

```bash
cd /workspaces/E-HGP/build/v10-persist/multik/ordres_meles
python3 build_cache.py --cache CACHE --jobs 1             # ~25 s de tour + 5 s de lecture par scène de 8 000 points
python3 evaluate.py --cache CACHE --out eval.csv --sizes 2000,8000 --zs 2,3,4,5,6,8 --kmax 10,5
python3 report.py eval.csv --out TABLEAUX.md
```

### 4.3 Coût

- Condensation des 9 ordres, en Python pur : environ 2 s à 2 000 points, 10 à 20 s à 8 000 points. La tour
  elle-même (binaire, KMAX = 10, 1 fil) prend 5 à 13 s à 2 000 points et 20 à 65 s à 8 000 points, selon la charge.
- Graphe des candidats et programme dynamique : de l'ordre de la seconde. Il y a quelques centaines de candidats par
  scène (62 à 316 sur les 13 scènes de 2 000 points examinées une à une).
- Une évaluation complète (128 scènes, 6 valeurs de z, 2 KMAX, 7 méthodes), cache construit, prend environ 45 min
  de CPU à un fil : 10 min pour les 64 scènes de 2 000 points, 35 min pour celles de 8 000.

## 5. Résultats (128 scènes dev, graines identiques pour toutes les méthodes)

### 5.1 KMAX = 10 : J, toutes tailles (entre parenthèses : 8 000 points)

Sans remplissage :

| z | une tranche | `mixq` | `mixq_sum` |
|---|---|---|---|
| 3 | 0,7624 (0,7508) | 0,7619 (0,7473) | 0,7604 (0,7448) |
| 4 | 0,7666 (0,7595) | **0,7717** (0,7637) | 0,7670 (0,7587) |
| 5 | 0,7636 (0,7594) | 0,7691 (0,7657) | 0,7676 (0,7575) |
| 6 | 0,7583 (0,7621) | 0,7671 (**0,7690**) | 0,7674 (0,7591) |
| 8 | 0,7336 (0,7293) | 0,7477 (0,7486) | 0,7605 (0,7588) |

Avec remplissage b1.5 :

| z | une tranche | `mixq` | `mixq_sum` |
|---|---|---|---|
| 3 | 0,7737 (0,7691) | 0,7737 (0,7657) | 0,7760 (0,7671) |
| 4 | 0,7810 (0,7826) | 0,7865 (0,7863) | 0,7854 (0,7841) |
| 5 | 0,7805 (0,7850) | 0,7860 (0,7910) | 0,7883 (0,7843) |
| 6 | 0,7775 (0,7901) | 0,7865 (**0,7965**) | **0,7907** (0,7893) |
| 8 | 0,7598 (0,7651) | 0,7724 (0,7817) | 0,7870 (0,7923) |

- **sklearn HDBSCAN à min_samples = 10**, avec les têtes du lot C, mcs = √n :
  - `leaf_a2` avec b2.5 : 0,7775 (8 000 points : 0,7785) ;
  - `eom_a1` sans remplissage : 0,6513 (0,6550) ;
  - meilleure tête à remplissage égal b1.5 : `leaf_a2`, 0,7540 (0,7445).
- **z = 2.** La tête à une tranche s'y effondre à 8 000 points (0,68 sans remplissage) ; `mixq` en limite la chute
  (0,75). Le détail est au § 6.

### 5.2 Écarts appariés à la tête à une tranche (même z, même remplissage), KMAX = 10

L'IC à 95 % est un bootstrap stratifié par taille. p est celui du retournement de signe, **descriptif**.

| méthode | remplissage | z | Δ tout | IC 95 % | gagne / perd | Δ 2 000 | Δ 8 000 |
|---|---|---|---|---|---|---|---|
| `mixq` | none | 4 | +0,0052 | [+0,0020 ; +0,0092] | 14 / 2 | +0,0062 | +0,0042 |
| `mixq` | none | 5 | +0,0055 | [+0,0020 ; +0,0098] | 18 / 4 | +0,0046 | +0,0064 |
| `mixq` | none | 6 | +0,0088 | [+0,0040 ; +0,0142] | 19 / 6 | +0,0108 | +0,0068 |
| `mixq` | b1.5 | 4 | +0,0055 | [+0,0030 ; +0,0081] | 20 / 2 | +0,0072 | +0,0037 |
| `mixq` | b1.5 | 5 | +0,0055 | [+0,0025 ; +0,0086] | 22 / 5 | +0,0050 | +0,0060 |
| `mixq` | b1.5 | 6 | +0,0090 | [+0,0050 ; +0,0135] | 24 / 4 | +0,0117 | +0,0064 |
| `mixq_sum` | none | 5 | +0,0040 | [−0,0015 ; +0,0096] | 22 / 30 | +0,0099 | −0,0019 |
| `mixq_sum` | none | 6 | +0,0092 | [−0,0005 ; +0,0185] | 27 / 29 | +0,0214 | −0,0030 |
| `mixq_sum` | b1.5 | 5 | +0,0078 | [+0,0006 ; +0,0144] | 33 / 9 | +0,0164 | −0,0007 |
| `mixq_sum` | b1.5 | 6 | +0,0132 | [+0,0024 ; +0,0223] | 42 / 11 | +0,0272 | −0,0008 |

- **À 8 000 points, `mixq` seul reste positif.** Pour la configuration du lot C (z = 6, b1.5), l'IC est
  [+0,0019 ; +0,0119], avec 9 scènes gagnées pour 1 perdue. `mixq_sum` y est nul.
- **Meilleure configuration contre meilleure configuration.** Chaque méthode prend son meilleur z :
  - avec b1.5, toutes tailles : une tranche 0,7810 (z = 4), `mixq` 0,7865 (z = 4 ou 6), `mixq_sum` 0,7907 (z = 6) ;
  - avec b1.5, à 8 000 points : 0,7901 (z = 6), 0,7965 (z = 6), 0,7923 (z = 8) ;
  - sans remplissage, à 8 000 points : 0,7621 (z = 6), 0,7690 (z = 6), 0,7591 (z = 6).
- **Écart au meilleur de la tête à une tranche.** Pour `mixq`, +0,005 à +0,007. Il reste bien sous
  delta_min = 0,02.

### 5.3 Sensibilité et témoins (KMAX = 10, toutes tailles, écart apparié à la tête à une tranche)

| variante | none, z = 4 / 5 / 6 | b1.5, z = 4 / 5 / 6 |
|---|---|---|
| `mixq`, ε = 0,1 (référence) | +0,0052 / +0,0055 / +0,0088 | +0,0055 / +0,0055 / +0,0090 |
| `mixq`, ε = 0,05 | +0,0052 / +0,0055 / +0,0088 | +0,0052 / +0,0055 / +0,0091 |
| `mixq`, ε = 0,2 | +0,0061 / +0,0041 / +0,0046 | +0,0085 / +0,0054 / +0,0052 |
| `mixs` (rapport des médianes) | +0,0038 / −0,0010 / +0,0001 | +0,0055 / +0,0004 / +0,0013 |
| `mixq_sumlo` (égalités vers le petit ordre) | −0,0194 / −0,0168 / −0,0120 | +0,0052 / +0,0085 / +0,0135 |

- **La tolérance ε pèse peu entre 0,05 et 0,1.** À ε = 0,2, les inclusions lâches rendent la sélection plus
  erratique : autant de scènes gagnées que perdues (18 / 18 à z = 5 sans remplissage).
- **La queue compte.** L'échelle par rapport des médianes, qui ne traite pas les queues, ne gagne presque rien.
- **Le départage vers les petits ordres montre un effet d'entrée, pas un effet d'objet.** Il perd 0,012 à 0,019
  sans remplissage et gagne 0,005 à 0,014 avec. Les amas pris à petit ordre sont des cœurs plus serrés, avec plus de
  bruit déclaré, que le remplissage borné réaffecte bien.

### 5.4 Par famille et par ordre choisi

Écart apparié à la tête à une tranche, z = 5 sans remplissage (Δ 2 000 / Δ 8 000) :

| famille | `mixq` | `mixq_sum` | ordres des amas finaux de `mixq` (z = 5), part à k ≤ 5 |
|---|---|---|---|
| spherical | −0,000 / −0,000 | −0,010 / −0,003 | 9–10 pour 98 % ; 0 % |
| anisotropic | +0,002 / +0,000 | +0,002 / −0,002 | 6–10 ; 0 % |
| heteroscedastic | +0,000 / +0,000 | +0,004 / −0,002 | surtout 9–10 ; 4 % |
| unbalanced | +0,000 / +0,020 | +0,006 / +0,010 | surtout 9–10 ; 4 % |
| shells | **+0,039 / +0,007** | +0,036 / −0,003 | 2–4 surtout (59 amas à k = 3) ; **72 %** |
| bridge | −0,000 / +0,000 | −0,003 / −0,005 | 8–10 ; 0 % |
| hierarchical | −0,000 / +0,000 | +0,002 / +0,002 | 5–10 ; 1 % |
| filaments | −0,004 / +0,023 | +0,043 / −0,012 | 3–10, étalés ; 5 % |

À z = 6 avec b1.5, `mixq` donne sur les coquilles +0,069 / +0,033, sur la famille hétéroscédastique +0,010 /
+0,012, et sur les filaments +0,007 / +0,001. `mixq_sum` y perd 0,045 sur les filaments à 8 000 points.

- **Le mélange est local.** 77 % des scènes (z = 5) ont des amas finaux à au moins 2 ordres, avec 2,5 ordres
  distincts par scène en moyenne ; l'ordre dominant porte 74 % des amas. Ce n'est donc pas un simple choix d'un K par
  scène. Ce dernier aurait d'ailleurs un plafond oracle de +0,013 à +0,025 au-dessus du meilleur K fixe (K ∈ {1, 2, 3,
  5, 8, 10}, mesuré sur les CSV dev avec la vérité, diagnostic seulement).
- **Modes présents dans la tour et modes retenus** (diagnostic avec la vérité, jamais utilisé pour choisir ; z = 5,
  KMAX = 10). Un groupe vrai est « présent » si un candidat a un Jaccard ≥ 0,5 avec lui, et « retenu » si un amas
  final l'a :

Par famille (8 groupes par scène, 16 scènes par famille) :

| famille | groupes vrais | présents à K = 10 | présents à un ordre 2..10 | retenus, une tranche | retenus, `mixq` | retenus, `mixq_sum` |
|---|---|---|---|---|---|---|
| spherical | 128 | 128 | 128 | 128 | 128 | 128 |
| anisotropic | 128 | 114 | 119 | 111 | 111 | 112 |
| heteroscedastic | 128 | 90 | 90 | 90 | 90 | 90 |
| unbalanced | 128 | 60 | 62 | 60 | 61 | 61 |
| shells | 128 | 128 | 128 | 123 | **128** | **128** |
| bridge | 128 | 128 | 128 | 128 | 128 | 127 |
| hierarchical | 128 | 128 | 128 | 1 | 1 | 3 |
| filaments | 128 | 102 | 110 | 84 | 84 | 83 |
| tout | 1 024 | 878 | 893 | 725 | 731 | 732 |

Quatre familles gaussiennes (spherical, anisotropic, heteroscedastic, unbalanced), par niveau :

| niveau | groupes vrais | présents à K = 10 | présents 2..10 | retenus, une tranche | retenus, `mixq` | retenus, `mixq_sum` |
|---|---|---|---|---|---|---|
| easy | 128 | 122 | 122 | 122 | 122 | 122 |
| medium | 128 | 107 | 108 | 107 | 107 | 108 |
| hard | 128 | 94 | 94 | 94 | 94 | 94 |
| extreme | 128 | 69 | 75 | 66 | 67 | 67 |

Lecture, qui répond à l'objection de l'utilisateur :

- **Sur les familles gaussiennes, un mode présent dans la tour est retenu.** À easy, medium et hard, chaque groupe
  qu'un candidat recouvre est retenu par la tête à une tranche (122/122, 107/107, 94/94). La sélection ne perd donc
  pas les modes d'un mélange séparable.
- **Les groupes manquants ne sont pas dans l'arbre sous forme d'amas.** Aucun candidat, à aucun ordre ≤ 10, n'en
  recouvre la moitié. Trois causes :
  - des groupes plus petits que mcs = √n : `unbalanced` à 2 000 points, groupes de ~31 points pour mcs = 45 ;
  - des groupes diffus, dont la composante ne capte jamais la moitié des points à K ≤ 10 (`heteroscedastic`, écarts
    types 0,4 / 1 / 2,5) ;
  - des modes fusionnés par le recouvrement aux niveaux extrêmes.
  Pour les deux dernières causes, c'est l'appartenance sous le col et le régime K ≤ 10 < d log n qui décident, pas la
  sélection.
- **Les ordres mêlés ajoutent peu de présence et de rétention.** Aux niveaux extrêmes, 6 groupes de plus sont
  présents à un ordre < 10 et 1 de plus est retenu.
- **Là où la sélection perd des modes présents, les ordres mêlés corrigent un cas sur trois.**
  - Coquilles : 128/128 retenus au lieu de 123.
  - `hierarchical` : 128 groupes présents, 1 à 3 retenus. L'EOM choisit les sous-amas, à tous les ordres : c'est un
    choix de niveau, que le mélange des ordres ne règle pas.
  - Filaments : 102 à 110 présents, 83–84 retenus : le choix du niveau reste perdant.

### 5.5 Contrôles

- La tête à une tranche recalculée sur le cache est égale au binaire : 0 écart sur 3 072 comparaisons.
- Le programme dynamique à un seul ordre est égal à l'EOM v10 : 0 écart sur 768 (128 scènes × K ∈ {10, 5} ×
  z ∈ {3, 5, 6}), avant et après la correction du § 6. L'échelle y vaut exactement l'identité.
- Verticales : voir § 2.3. À 8 000 points, sur 14 698 clusters des ordres 3 à 10 :
  - ρ médian 1,000, minimum 0,933, 100 % des clusters à ρ ≥ 0,9 ;
  - le substitut ensembliste égale φ(c) dans 15 % des cas et est toujours sous φ(c) (100 %).

### 5.6 KMAX = 5 (ordres 2..5)

- **Sans remplissage.** `mixq` gagne +0,003 à z = 5 (IC [+0,001 ; +0,007]) et +0,009 à z = 8. Il reste neutre à
  z = 4 et z = 6 (+0,001 et +0,002, non significatifs).
- **Avec b1.5, z = 5.** +0,0065 sur toutes les tailles (IC [+0,002 ; +0,012]) et +0,005 à 8 000 points
  (IC [+0,002 ; +0,009]). À z = 4 et z = 6 : +0,003, non significatif.
- **`mixq_sum`.** Non significatif à z = 4–6 à 8 000 points (−0,006 à +0,002).
- **Lecture.** Avec 4 ordres, il y a moins à mêler, et le gain est plus faible qu'à KMAX = 10. sklearn à
  min_samples = 5 (lot C, `leaf_a2` b2.5) vaut 0,7651, contre 0,7836 pour `mixq` à z = 5 avec b1.5.

## 6. Lecture critique

- **Le gain est réel mais petit, et concentré.**
  - Il vient de quelques scènes : 14 à 24 gagnées sur 128 pour z = 4–6. Sur les familles gaussiennes, `bridge` et
    `hierarchical`, la sélection reprend l'ordre 10 et ne change presque rien.
  - C'est ce qu'annonçait la conception : les petits ordres ne l'emportent que là où ils montrent une structure plus
    persistante en contenu de probabilité (surfaces minces, filaments).
- **Coquilles : le mécanisme est celui de l'audit.**
  - Les 8 coquilles sont des branches de l'arbre à tous les ordres.
  - À K = 10, l'EOM retient souvent leurs parents : 5 coquilles sur 128 perdues à z = 5 (§ 5.4). Aux ordres 3–4,
    les coquilles se séparent à un contenu plus élevé et leur excess of mass normalisé dépasse celui des parents.
  - C'est l'instabilité de Rolle–Scoccola, contournée par la sélection d'échelle.
- **Malédiction du gagnant, et le cas z = 2.**
  - Le maximum sur 9 ordres favorise l'alternative dont la valeur est la plus haute à un ordre quelconque.
  - À z = 2 (granularité basse), l'EOM préfère les grands blocs. Sur `spherical_extreme_n2000_nu0.1`, le bloc de
    6 gaussiennes est gardé entier par l'EOM à 8 ordres sur 9 ; seul l'ordre 10 le découpe, sur une quasi-égalité
    (3,16 contre 3,23 × 10⁻⁶). La tête à une tranche y est bonne par chance, et la sélection mêlée suit la majorité
    des ordres (ARI_s 0,19).
  - À 8 000 points, c'est l'inverse : la tête à une tranche elle-même s'effondre à z = 2 (J = 0,68).
  - Les deux pertes nettes de `mixq` sont des filaments à 2 000 points, à z = 5 :
    - `filaments_hard_nu0` (−0,045) : un bloc d'ordre 5 fusionne deux filaments que l'ordre 10 sépare. C'est la même
      malédiction ;
    - `filaments_easy_nu0.1` (−0,047) : un filament est coupé en deux amas d'ordres 6 et 7.
- **La somme ne tient pas à 8 000 points.**
  - `mixq_sum` intègre sur les ordres : un amas présent à un seul ordre n'y est pas récompensé. C'est l'idée de
    Blumberg–Lesnick.
  - Mais son gain est un gain à 2 000 points : +0,016 à +0,027 avec b1.5 (z = 5–8). À 8 000 points il est nul
    (−0,003 à −0,001 à z = 5–6), et il perd 0,045 sur les filaments à z = 6.
  - À z ≤ 4 sans remplissage, il perd aussi sur les gaussiennes à 2 000 points (−0,01 à −0,03). La somme y favorise
    des découpages persistants mais plus fins.
  - Une part de son gain avec remplissage vient d'amas plus serrés, que le remplissage réaffecte. C'est un effet de
    l'entrée, comme `mixq_sumlo` le montre (§ 5.3), et non de l'objet.
- **L'échelle commune est le point essentiel, et c'est aussi le point le plus fragile.**
  - Sans elle, le mélange est nuisible.
  - Le contenu de probabilité est une convention : il traite la masse entrée comme l'unité commune.
  - Au-dessus de la dernière entrée, les fusions proches de la racine sont extrapolées.
- **z reste un cadran.** Le mélange des ordres ne supprime pas la dépendance à z : l'optimum reste à z = 4–6.
  `mixq_sum` aplatit la courbe aux grands z. Entre z = 5 et z = 8, il perd −0,010 sans remplissage et −0,005 avec
  b1.5, contre −0,030 et −0,021 pour la tête à une tranche.
- **Deux corrections faites en cours de route, toutes deux déclarées.**
  - Après la première passe (2 000 points), le départage des égalités de `mixq_sum` : l'ordre 2 gagnait les égalités
    par artefact de tri. Il prend maintenant le plus grand ordre, et `mixq_sumlo` garde l'ancien comportement comme
    témoin.
  - Après la deuxième passe complète, un défaut du programme dynamique. La maximalité des décompositions ne testait
    que le parent. Or, avec la tolérance ε, l'inclusion à ε près ne se transmet pas aux sous-ensembles : un ancêtre et
    son descendant pouvaient entrer tous deux dans la même décomposition, d'où un double compte.
    - Scènes touchées à ε = 0,1 : 2 à 2 000 points, 15 à 8 000 points (surtout coquilles et filaments).
    - Effet de la correction : le gain moyen de `mixq` baisse de 0,0006 à 2 000 points et de 0,0024 à 8 000 points.
      Celui des filaments à 8 000 points et z = 5 passe de +0,069 à +0,023.
    - Tous les chiffres de ce document sont ceux du code corrigé. Les CSV de la passe précédente sont gardés
      (`eval_*_v2.csv.gz`).
- **Rien ici n'est confirmatoire.**
  - Les 128 scènes sont celles de la conception, et sept variantes ont été mesurées.
  - Les p sont descriptifs.
  - Une conclusion demanderait un préenregistrement et des graines de test neuves. Proposition : méthode `mixq`,
    z choisi par la règle du lot C sur les 8 000 points dev, soit z = 6 avec b1.5 — la configuration même de la tête
    v10-b à K = 10.

## 7. Conclusion et suite

- **Réponse à la consigne.**
  - Une sélection qui choisit l'ordre par région bat la tête à une tranche au même KMAX et au même z. La
    fonctionnelle est fondée : l'excess of mass dans l'échelle du contenu de probabilité, avec un maximum sur les
    ordres (`mixq`, sélection d'échelle).
  - Le gain est de +0,005 à +0,009 pour z = 4–6 (KMAX = 10), avec les IC hors de zéro, et de même signe dans les
    deux tailles. Il vient de familles identifiables : coquilles, filaments, amas déséquilibrés ou hétéroscédastiques.
  - Il est inférieur au seuil de décision du lot C (0,02). À KMAX = 5, il est plus faible encore (+0,003 à +0,007).
  - L'intégrale sur les ordres (`mixq_sum`), mieux motivée par la stabilité de la bifiltration, ne tient pas à
    8 000 points.
- **Objection de l'utilisateur.**
  - Sur les mélanges gaussiens, tout groupe qu'un amas de la tour recouvre est retenu par la tête à une tranche aux
    niveaux easy, medium et hard ; mêler les ordres n'ajoute rien.
  - Les groupes manquants ne sont des amas à aucun ordre ≤ 10 : trop petits pour mcs, trop diffus, ou fusionnés au
    niveau extrême. Il leur manque l'appartenance des points sous les cols et un K plus grand que 10, pas un meilleur
    choix d'ordre.
- **Pistes, par ordre de rapport attendu.**
  1. Préenregistrer `mixq` (z = 6, b1.5, KMAX = 10) face à la tête v10-b et à sklearn, sur des graines de test
     neuves. Hypothèse : Δ > 0, sans atteindre delta_min.
  2. Corriger la malédiction du gagnant par un rééchantillonnage (bootstrap de Kim et al. 2016). La décision serait
     alors « changer d'ordre seulement si l'écart de valeur dépasse sa variabilité ». Cela exigerait des tours sur
     sous-échantillons : environ 25 s par tirage à 8 000 points.
  3. Calibrer le haut de l'arbre (au-delà de la dernière entrée) sur les niveaux de fusion plutôt que par
     translation.
  4. Combiner avec les deux autres concepteurs (tranche oblique, persistance à travers K), sur le même sous-ensemble.
