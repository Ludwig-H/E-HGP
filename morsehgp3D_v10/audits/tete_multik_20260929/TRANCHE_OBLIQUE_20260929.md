# Tête sur une tranche oblique de la multicouverture, construite par les verticales de la tour (29 septembre 2026)

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=dev_seulement (run_campaign.plan('dev', ...), replicate 0 ; aucune graine test ni test_v10b)
public_status=not_claimed
tour : binaire figé build/v10-dev-multik/mhgp10_tower (sha256 272477f2…c542e9), entrée cover, K = 10, 1 fil
GCP non utilisé ; aucun fichier du dépôt modifié ; au plus 2 processus à 1 fil
```

Rôle : concepteur « tranche oblique » (Rolle et Scoccola, JMLR 2024 : λ‑link / γ‑link). Lu d'abord :
`morsehgp3D_v10/audits/audit_hierarchie_knn_20260929/AUDIT_HIERARCHIE_KNN_20260929.md` et son annexe de littérature.

## 0. Réponse courte

- **La construction est faite et vérifiée.**
  - La tranche est S(r) = L_{k(r)}(r) le long de la droite de Rolle–Scoccola, avec k(r) = ⌈K_max (1 − r / r_0)⌉ et
    r_0 = c · médiane(d_{K_max}).
  - Elle est construite par les verticales de la tour : champ `lower`, puis remontée dans l'ordre inférieur au
    niveau de bascule.
  - Contrôle d'identité : en tranche horizontale, le prototype reproduit **à l'identique** l'ARI_s du banc v10‑b.
    Cela vaut sur 3 840 lignes (128 scènes ; K = 10 avec six valeurs de z ; K = 5 avec trois ou six valeurs ; sans
    remplissage et b1.5), avec un écart maximal de 0.
- **Le fondement tient dans la construction discrète, mais il est qualitatif.**
  - La droite est entrelacée : pour un écart de Prohorov ε, δ = ε · max(1, n r_0 / K_max). C'est
    Blumberg–Lesnick, puis le corollaire 42 de Rolle–Scoccola, redémontré ici pour la multicouverture.
  - Le palier entier est exact : ce n'est pas une approximation de la droite continue.
  - En retirant p points, les temps d'entrée vérifient t_X ≤ t_Y ≤ t_X + p r_0 / K_max. On observe **0 violation**
    sur 128 perturbations à 2 000 points et sur 16 à 8 000 points.
  - La tranche horizontale n'a aucune borne. Retirer 2 points déplace 1 314 temps d'entrée à 2 000 points.
  - La constante est vide dès p ≥ K_max points. Ni la règle de couverture ni l'EOM ne sont couvertes par le
    théorème.
- **Mesure : la tranche oblique ne bat pas la tête v10‑b à une tranche.**
  - Configuration P1, pré‑choisie sur 2 000 points puis figée avant 8 000. Écart moyen apparié avec la tête v10‑b à
    K = 10, même z :
    - à 2 000 points : +0,003 sans remplissage, +0,007 avec b1.5 ;
    - à 8 000 points : −0,003 sans remplissage, +0,002 avec b1.5.
  - Face au meilleur z de v10‑b à 8 000 points (z = 6), l'écart est de −0,006.
  - Tous les intervalles à 95 % contiennent 0, sauf à 2 000 points avec b1.5 (borne basse +0,0006).
  - La meilleure oblique choisie a posteriori ne gagne que +0,008 (8 000 points, b1.5), avec un biais de sélection.
- **Ce que l'oblique apporte vraiment : moins de dépendance aux paramètres fragiles, sur une famille.**
  - Gains sur `shells` (+0,024 à 2 000 points, b1.5) et sur `filaments` à 2 000 points : ce sont les familles où
    l'audit a mesuré l'instabilité en K.
  - L'EOM dépend moins de z à 8 000 points : à z = 2, J vaut 0,750 contre 0,692 pour K = 10 fixe. Tout l'écart
    vient de `shells` (0,972 contre 0,462) : l'effondrement en 3 parents que l'audit a documenté disparaît. Il y a
    une contrepartie sur `spherical` (0,757 contre 0,841). À z optimal, l'écart disparaît.
  - La sortie plate n'est pas plus stable en général.
    - À 2 000 points, changer r_0 de 10 % laisse un ARI de 0,994, contre 0,972 pour passer de K = 9 à K = 10.
    - À 8 000 points (16 scènes hard), c'est 0,971 contre 0,984, avec des pires cas équivalents, tous sur
      `filaments`.
    - Le théorème stabilise la hiérarchie, pas l'EOM.
- **L'aplatissement par écart de proéminence de Rolle–Scoccola (PF), avec nombre d'amas automatique, échoue**
  (J = 0,43–0,71).
  - Le plus grand écart désigne le niveau le plus persistant. Ce n'est la vérité du banc que pour `hierarchical`
    (0,59–0,93 contre 0,46 pour l'EOM).
  - Sur `spherical`, les 8 centres forment un anneau hexagonal plus 2 pôles. La structure la plus persistante a donc
    3 amas, et PF les trouve.
  - À n = 8 connu (diagnostic, pas une méthode), PF atteint 0,81–0,84 sur les deux tranches. L'oblique n'y apporte
    rien.
- **Réponse à l'objection « un mélange séparable doit être identifié ».**
  - La tranche oblique ne change ni l'objet ni la sélection des modes.
  - Elle ne rapproche pas du plafond de Bayes aux niveaux difficiles. Exemple, `anisotropic` extrême à 2 000 points :
    0,671 contre un plafond de 0,889.
  - Le levier n'est pas la tranche. Il est dans l'affectation sous le col et dans la sélection du niveau.
- **Recommandation.**
  - Ne pas remplacer la tête v10‑b par l'oblique pour le score.
  - Garder la construction (exacte, peu coûteuse, vérifiée) comme :
    - objet stable ;
    - option de robustesse sur les coquilles ;
    - support d'une future sélection par vignoble.

## 1. Fondement

### 1.1 L'objet que la tour calcule tranche par tranche

- Mesure empirique ν_X = (1/n) Σ_{x ∈ X} δ_x.
- Multicouverture en boules fermées (Sheehy ; Blumberg–Lesnick, déf. 2.13) :
  M_X(r, m) = {y ∈ ℝ³ : ν_X(B̄(y, r)) ≥ m}.
- Les comptes sont entiers. Pour tout réel m > 0, on a donc M_X(r, m) = L_{⌈n m⌉}(r), avec
  L_k(r) = {y : |B̄(y, r) ∩ X| ≥ k}.
- La tour donne, pour k = 1..K_max :
  - l'arbre exact des composantes de L_k(r) quand r croît ;
  - les inclusions verticales L_k(r) ⊆ L_{k−1}(r). Le champ `lower` d'un nœud est le nœud d'ordre k − 1 vivant à son
    niveau de création, qui le contient.

### 1.2 Tranches

- Une courbe t ↦ (r(t), m(t)), avec r croissant et m décroissant, donne une filtration emboîtée
  S(t) = M_X(r(t), m(t)).
- **Horizontale** : m = K / n fixe. C'est la tête v10‑b.
- **Oblique linéaire** (λ‑link, Rolle–Scoccola ex. 34, en coordonnées rayon–masse) :
  - m(r) = m_0 (1 − r / r_0) sur [0, r_0), avec m_0 = K_max / n, puis S = ℝ³ au‑delà de r_0 ;
  - en ordres : k(r) = ⌈K_max (1 − r / r_0)⌉ ;
  - l'ordre j est actif sur [r_0 (K_max − j) / K_max, r_0 (K_max − j + 1) / K_max).

### 1.3 Stabilité : les résultats publiés, redémontrés pour notre objet

- **Lemme 1** (Blumberg–Lesnick, th. 1.6 (i), écrit en boules fermées).
  - Énoncé : si d_Pr(ν_X, ν_Y) < ε, alors M_X(r, m) ⊆ M_Y(r + ε, m − ε) pour tous r et m.
  - Preuve : ν_X(B̄(y, r)) ≤ ν_Y(B̄(y, r)^ε) + ε ≤ ν_Y(B̄(y, r + ε)) + ε.
  - C'est une inclusion d'ensembles dans le même ℝ³ : elle entrelace les hiérarchies de composantes, avec la
    correspondance identité.
- **Lemme 2** (tranche linéaire).
  - Énoncé : avec δ = ε · max(1, r_0 / m_0) = ε · max(1, n r_0 / K_max), on a S_X(r) ⊆ S_Y(r + δ), et
    symétriquement.
  - Preuve : m(r + δ) = m(r) − m_0 δ / r_0 ≤ m(r) − ε.
  - C'est la forme du corollaire 42 de Rolle–Scoccola.
- **Tranche horizontale** : il faudrait m − ε ≥ m, et aucun δ fini n'existe.
  - C'est l'instabilité des propositions 44–45 de Rolle–Scoccola.
  - L'audit l'a mesurée : coquilles retenues à K = 8, perdues à K = 10.
- **Aplatissement PF** (Rolle–Scoccola, th. 84).
  - Si deux hiérarchies sont ε‑entrelacées avec ε < gapsize_n / 16, les n amas de PF se correspondent, et chaque
    paire est 3ε‑entrelacée.
  - L'EOM n'a aucun théorème de ce type.
- **Réparamétrisation en densité** (Rolle–Scoccola, déf. 57).
  - Le seuil de densité K‑NN le long de la droite est m(r) / (v_d r^d).
  - La tête `eom_m` prend λ = m(r) · r^(−z). À z = d = 3, c'est ce seuil à une constante près : l'excès de masse en
    unités de densité (Polonik), sans z libre.
- **Consistance** (Rolle–Scoccola, th. 58) : elle suppose une droite fixée en masse quand n croît.
  - Notre intercept K_max / n tend vers 0 : on sort du théorème, comme toute tranche à K fixe (audit § 2.4).

### 1.4 Ce qui tient dans la construction discrète, et ce qui ne tient pas

1. **Palier exact.** L'escalier ⌈K_max (1 − r / r_0)⌉ est exactement la droite continue en masse, parce que
   M_X(r, m) = L_{⌈n m⌉}(r). Il n'y a aucune erreur de discrétisation.
2. **Verticales.** À la bascule A_j, une composante d'ordre j + 1 encore vivante est envoyée par `lower` dans l'ordre
   j. On remonte ensuite jusqu'à l'ancêtre vivant en A_j. C'est l'inclusion L_{j+1}(A_j) ⊆ L_j(A_j), parce que les
   inclusions commutent.
   - Invariants contrôlés à chaque construction : parent d'indice supérieur et de temps supérieur ou égal, racine
     unique, point entré entre la création de son nœud et celle du parent.
   - Le cas horizontal redonne l'arbre de la tour, contrôlé par l'identité V1.
3. **Lemme 3 : retrait de p points, version entière** (p = 2 et 1 %, 64 scènes de 2 000 points ; p = 2, 16 scènes
   « hard » de 8 000 points).
   - Inclusions : L^X_k(r) ⊆ L^Y_{k−p}(r) et L^Y_k(r) ⊆ L^X_k(r). D'où S_Y(r) ⊆ S_X(r) ⊆ S_Y(r + p r_0 / K_max).
   - Temps d'entrée par première couverture : on a α^X_k(x) ≤ α^Y_k(x), et α^Y_{k−p}(x) ≤ α^X_k(x) (la boule
     couvrante de X contient au moins k − p sites de Y). On en tire **t_X(x) ≤ t_Y(x) ≤ t_X(x) + p r_0 / K_max**.
     Mesure : **0 violation** (tableau 6).
4. **Ce que le théorème ne couvre pas.**
   - La règle de couverture choisit la composante de la boule couvrante. Les hauteurs de fusion entre points peuvent
     donc sortir de la bande. On le mesure : 0,02 % des paires pour p = 2 (tableau 6).
   - δ = p r_0 / K_max vaut une fenêtre d'ordre par point retiré. La borne est vide dès que p ≥ K_max, donc dès 10
     points.
   - L'EOM n'hérite d'aucune garantie.

## 2. Construction (prototype `prototype/oblique.py`)

- **Fenêtres.** A_j = début de la fenêtre de l'ordre j, en rayon carré ; A_{K_max} = 0 ; A_0 = r_0², où tout fusionne
  (m = 0). Ce sont les classes `Curve('linear' | 'loglin' | 'fixed')`.
- **Nœuds de la tranche.**
  - Ce sont les paires (j, v) où v est un nœud d'ordre j vivant dans sa fenêtre : lvl(v) < A_{j−1} et
    lvl(parent(v)) > A_j.
  - Temps : τ = max(lvl(v), A_j).
  - Parent :
    - le parent d'ordre j s'il naît dans la fenêtre ;
    - sinon la verticale `lower`, remontée dans l'ordre suivant au niveau A_{j−1}, en traversant les fenêtres vides ;
    - sinon la racine forcée en r_0.
- **Entrée des points.** x entre au premier ordre j, en partant de K_max, tel que α_j(x)² < A_{j−1}.
  - Temps : max(α_j(x)², A_j).
  - Nœud : ancêtre, vivant à ce temps, de la composante de sa boule couvrante (`pnode`).
  - C'est exactement « le plus petit t tel que α_{k(t)}(x) ≤ r(t) ».
- **Arbre réduit.** On retire les sous‑arbres sans point et on contracte les nœuds unaires sans point : de 50 000 à
  900 000 nœuds, on passe à 1 500–9 600.
  - Contrôle V3 : les partitions EOM et PF et les codes‑barres sont identiques, 324 sur 324.
- **Règle de r_0.** r_0 = c · médiane_x d_{K_max}(x), où d_{K_max} est la distance au K_max‑ième site, point compris.
  C'est l'échelle naturelle de L_{K_max}. La règle est équivariante par homothétie et ne lit pas la vérité.
  - c est choisi au milieu du plateau de la grille à 2 000 points (tableau 3 : c ∈ [1,5 ; 12] donne ±0,01). Choix :
    c = 3.
  - Il est figé avant toute mesure à 8 000 points (`resultats/PRECHOIX_8000.txt`, horodaté 11:05:28Z, avec la
    sha256 de la grille 2 000).
  - Le critère (Rolle–Scoccola, prop. 43 et § 7.1.3) est de prendre la droite dans une région stable du vignoble.
    L'effet de c est plat : même l'oracle du meilleur c par scène (lecture de la vérité, diagnostic) ne gagne que
    +0,013.
  - Analogue publié : le code de Persistable (Scoccola–Rolle, `find_end`, lu sur GitHub) met par défaut la fin de la
    droite à 4 × le quantile 95 % des hauteurs de fusion de la liaison simple, sur les grands jeux. Notre règle en
    est la version K‑NN : même esprit (une échelle de distances du nuage, sans vérité), constante différente.
- **Têtes.**
  - `eom_r{z}` : condensation N‑aire, mcs = round(√n), EOM avec racine exclue, λ = r^(−z). C'est la tête v10‑b.
  - `eom_m{z}` : λ = m(r) · r^(−z), la densité le long de la droite. **P2** = eom_m3.
  - `pf_s1` et `pf_smcs` : PF de Rolle–Scoccola.
    - Nombre d'amas donné par le plus grand écart de proéminence, n ≥ 2, barres en rayon.
    - Une composante naît à son premier point (s1) ou à mcs points (smcs).
  - `pfq_*` : variante non publiée, qui prend le plus grand rapport L_{n−1} / L_n.
  - **P1** = droite c = 3 et eom_r4 (z = 4 : l'optimum de v10‑b à 2 000 points).
- **Remplissage.** none, et b1.5 avec distance‑cœur au K_max‑ième voisin, comme v10‑b à K = K_max.

## 3. Mesures (sous‑ensemble dev commun : 128 scènes, K_max = 10)

J est la moyenne par cellule (famille × niveau × bruit × n) de l'ARI_s, avec une scène par cellule. Les bases viennent
des CSV dev existants, sur les mêmes graines.

### Tableau 1 — J global

| tête | 2000 none | 2000 b1.5 | 8000 none | 8000 b1.5 |
|---|---|---|---|---|
| **oblique P1** (c = 3, λ = r⁻⁴) | 0,776 | 0,787 | 0,756 | 0,785 |
| **oblique P2** (c = 3, λ = m(r) r⁻³) | 0,774 | 0,783 | 0,756 | 0,782 |
| v10‑b K = 10, z = 4 | 0,774 | 0,779 | 0,760 | 0,783 |
| v10‑b K = 10, meilleur z par taille (3 ou 4 / 6) | 0,774 | 0,779 | 0,762 | 0,790 |
| sklearn min_samples = 10, leaf α = 1 | 0,668 | 0,772 | 0,573 | 0,722 |
| sklearn min_samples = 10, leaf α = 2 | 0,610 | 0,764 | 0,567 | 0,744 |
| sklearn min_samples = 10, EOM α = 1 | 0,648 | 0,678 | 0,655 | 0,693 |

- **Meilleure oblique a posteriori** (grille de 8 courbes linéaires, 2 log‑linéaires, 6 z et 2 λ ; biais de
  sélection) :
  - 0,780 (2000, none) ;
  - 0,788 (2000, b1.5) ;
  - 0,768 (8000, none) ;
  - 0,798 (8000, b1.5 ; c = 3, z = 6).

### Tableau 5 — Écarts appariés par scène (IC bootstrap 95 %, signes +/−/=)

| comparaison | 2000 none | 2000 b1.5 | 8000 none | 8000 b1.5 |
|---|---|---|---|---|
| P1 − v10‑b K = 10 z = 4 | +0,003 [−0,003 ; +0,009] 25/34/5 | +0,007 [+0,001 ; +0,015] 29/30/5 | −0,003 [−0,009 ; +0,003] 20/39/5 | +0,002 [−0,003 ; +0,007] 32/27/5 |
| P1 − v10‑b meilleur z | +0,002 (z = 3) | +0,007 (z = 4) | −0,006 [−0,022 ; +0,006] (z = 6) | −0,006 [−0,028 ; +0,010] (z = 6) |
| P2 − v10‑b K = 10 z = 4 | +0,000 | +0,004 [−0,008 ; +0,014] | −0,004 | −0,000 |
| P1 − sklearn ms = 10 leaf α = 1 | +0,109 [+0,085 ; +0,132] | +0,015 [+0,000 ; +0,028] | +0,183 [+0,130 ; +0,240] | +0,062 [+0,025 ; +0,098] |
| P1 − sklearn ms = 10 leaf α = 2 | +0,166 | +0,023 [−0,006 ; +0,048] | +0,189 | +0,040 [+0,014 ; +0,062] |

Lecture :
- l'oblique est **indiscernable** de la tête à une tranche ;
- son avantage sur sklearn est celui de la tête v10‑b (entrée et tête, audit § 1.5), pas celui de la tranche.

### Tableau 2 — J par famille (b1.5)

| n | tête | spherical | anisotropic | heteroscedastic | unbalanced | shells | bridge | hierarchical | filaments |
|---|---|---|---|---|---|---|---|---|---|
| 2000 | oblique P1 | 0,919 | 0,855 | 0,684 | 0,745 | **0,960** | 0,902 | 0,467 | **0,762** |
| 2000 | v10‑b K = 10 z = 4 | 0,921 | 0,856 | 0,673 | 0,736 | 0,936 | 0,902 | 0,467 | 0,744 |
| 2000 | sklearn ms = 10 leaf α = 1 | 0,881 | 0,839 | 0,692 | 0,713 | 0,886 | 0,887 | 0,506 | 0,775 |
| 2000 | PF Rolle–Scoccola (pf_smcs) | 0,467 | 0,508 | 0,432 | 0,538 | 0,165 | 0,441 | **0,841** | 0,508 |
| 8000 | oblique P1 | 0,922 | 0,828 | 0,707 | 0,795 | 0,972 | 0,906 | 0,462 | 0,684 |
| 8000 | v10‑b K = 10 z = 4 | 0,921 | 0,827 | 0,701 | 0,787 | 0,964 | 0,902 | 0,463 | 0,696 |
| 8000 | v10‑b K = 10 z = 6 | 0,921 | 0,827 | 0,701 | 0,787 | 0,940 | 0,902 | 0,463 | 0,779 |
| 8000 | sklearn ms = 10 leaf α = 2 | 0,860 | 0,803 | 0,696 | 0,708 | 0,827 | 0,857 | 0,437 | 0,768 |
| 8000 | PF Rolle–Scoccola (pf_smcs) | 0,380 | 0,640 | 0,516 | 0,477 | 0,173 | 0,462 | 0,593 | 0,472 |

- **Plus grands gains par scène**, à 2 000 points et sur P1 − v10‑b :
  - `unbalanced` hard ν = 0,1 : +0,16 ;
  - `filaments` medium et extrême : +0,07 à +0,08 ;
  - `shells` hard et extrême : +0,05.
- **Plus grandes pertes** :
  - `unbalanced` medium ν = 0,1 : −0,06 ;
  - `filaments` easy : −0,04 ;
  - `spherical` hard : −0,02 à −0,04 sans remplissage.

### Tableau 3 — Sensibilité à c (droite, λ = r⁻⁴) et dépendance à z

| n, remplissage | c = 1,5 | 2 | 3 | 4 | 5 | 6 | 8 | 12 | K = 10 fixe |
|---|---|---|---|---|---|---|---|---|---|
| 2000, none | 0,768 | 0,772 | 0,776 | 0,778 | 0,776 | 0,777 | 0,776 | 0,773 | 0,774 |
| 2000, b1.5 | 0,786 | 0,788 | 0,787 | 0,786 | 0,785 | 0,785 | 0,781 | 0,781 | 0,779 |
| 8000, none | 0,745 | 0,752 | 0,756 | 0,759 | 0,762 | 0,762 | 0,760 | 0,760 | 0,760 |
| 8000, b1.5 | 0,776 | 0,780 | 0,785 | 0,782 | 0,785 | 0,785 | 0,784 | 0,783 | 0,783 |

J en fonction de z (λ = r^(−z), b1.5) :

| n | tranche | z = 2 | 3 | 4 | 5 | 6 | 8 |
|---|---|---|---|---|---|---|---|
| 2000 | K = 10 fixe | 0,768 | 0,778 | 0,779 | 0,776 | 0,765 | 0,754 |
| 2000 | oblique c = 3 | 0,773 | 0,784 | 0,787 | 0,780 | 0,776 | 0,763 |
| 8000 | K = 10 fixe | **0,692** | 0,769 | 0,783 | 0,785 | 0,790 | 0,765 |
| 8000 | oblique c = 3 | **0,750** | 0,774 | 0,785 | 0,786 | 0,798 | 0,788 |
| 8000 | oblique c = 1,5 | 0,748 | 0,766 | 0,776 | 0,791 | 0,790 | 0,794 |

- **D'où vient l'écart à petit z.** À z = 2 et 8 000 points, il vient entièrement de `shells` : 0,972 contre 0,462
  pour K = 10 fixe (l'EOM y retenait 3 parents au lieu de 8 coquilles). Il y a une perte sur `spherical` : 0,757
  contre 0,841.
- **À 2 000 points et z = 6**, `shells` passe de 0,882 à 0,937.
- **L'oblique corrige l'instabilité en K** documentée par l'audit sur les coquilles, dans les deux sens de z.
- **Mécanisme, partiellement vérifié.** L'ordre décroît le long de la droite, donc les fusions arrivent à plus petit
  rayon.
  - Mesure sur les 16 scènes « hard » de 8 000 points : les fusions significatives (au moins deux enfants de mcs
    points) couvrent 0,72 en log r au lieu de 0,92.
  - L'EOM, dont les poids varient en (r_2 / r_1)^z, dépend alors moins de z.
- **Portée.** C'est un gain de robustesse (z mal choisi, famille fragile en K), pas un gain au meilleur z.

### Tableau 4 — K_max = 5 (même protocole, ordres ≤ 5)

| tête | 2000 none | 2000 b1.5 | 8000 none | 8000 b1.5 |
|---|---|---|---|---|
| **oblique K_max = 5**, c = 3, λ = r⁻⁴ | 0,761 | 0,780 | 0,749 | 0,782 |
| oblique K_max = 5, c = 3, λ = m(r) r⁻³ | 0,762 | 0,780 | 0,744 | 0,772 |
| v10‑b K = 5, z = 4 | 0,762 | 0,778 | 0,753 | 0,780 |
| v10‑b K = 5, meilleur z (3 / 6) | 0,769 | 0,781 | 0,754 | 0,792 |
| sklearn min_samples = 5, leaf α = 2 | 0,636 | 0,760 | 0,538 | 0,704 |
| sklearn min_samples = 5, EOM α = 1 | 0,668 | 0,697 | 0,671 | 0,705 |

- **Écarts appariés**, oblique K_max = 5 − v10‑b K = 5 z = 4 :
  - 2 000 points : −0,001 [−0,007 ; +0,004] sans remplissage, +0,002 [−0,006 ; +0,011] avec b1.5 ;
  - 8 000 points : −0,003 [−0,008 ; +0,001] sans remplissage, +0,002 [−0,004 ; +0,009] avec b1.5.
- **Face à sklearn** à min_samples = 5 (leaf α = 2), avec b1.5 : +0,020 [−0,001 ; +0,039] à 2 000 points, et
  +0,079 [+0,041 ; +0,116] à 8 000 points.
- **Conclusion identique à K_max = 10.**

### PF et nombre d'amas : pourquoi Rolle–Scoccola ne convient pas à la vérité du banc

- **Ce que PF désigne.** Le plus grand écart de proéminence (th. 84 : le plus stable) donne le niveau le plus
  persistant de l'arbre de densité.
- **Sur `spherical`, ce niveau n'est pas la vérité.**
  - Les centres sont pris gloutonnement dans la grille 4 × 4 × 4 : 6 centres en anneau à distance 1, et 2 pôles à
    1,41.
  - Les 6 amas de l'anneau fusionnent d'abord, les pôles ensuite. Barres observées, en unités de s : 0,48–0,82, puis
    1,71–1,73.
  - PF choisit donc 3 amas, et c'est juste pour l'arbre.
- **Sur `hierarchical`, le niveau persistant est la vérité** (8 groupes de 3 sous‑amas) : PF donne 0,59–0,93, et
  l'EOM 0,46.
- **La vérité du banc suit deux conventions opposées** : composantes du mélange (7 familles) ou groupes
  (`hierarchical`). Aucune règle de niveau par persistance ne sert les deux.
- **Variante par rapport** (pfq, non publiée) : 0,63–0,71 avec b1.5. Elle reste loin de l'EOM.
- **Diagnostic, pas une méthode.** Avec n = 8 donné (le nombre de groupes, lu dans la vérité), PF donne en b1.5 :
  - 0,840 (s1) et 0,823 (smcs) sur la tranche K = 10 ;
  - 0,813 et 0,829 sur l'oblique.

  Les 8 modes sont en général les 8 barres les plus proéminentes, mais l'oblique ne les rend pas plus lisibles.

### Réponse à l'objection de l'utilisateur, vue depuis la tranche oblique

- **L'oblique reste un chemin d'ensembles de niveau K‑NN exacts.** Chaque S(r) vaut
  {f̂_k ≥ k / (n v_d r^d)} pour son ordre k. Elle traverse donc, à n infini, le même arbre de densité que la tranche
  horizontale.
- **Elle ne rapproche pas du plafond de Bayes aux niveaux difficiles** (tableau 2 bis, 2 000 points, b1.5) :

  | famille, niveau | plafond | K = 10 | oblique |
  |---|---|---|---|
  | `spherical` hard | 0,946 | 0,929 | 0,922 |
  | `spherical` extrême | 0,893 | 0,852 | 0,842 |
  | `anisotropic` hard | 0,942 | 0,860 | 0,860 |
  | `anisotropic` extrême | 0,889 | 0,674 | 0,671 |
  | `unbalanced` hard | 0,851 | 0,541 | 0,617 |
- **La couverture sans remplissage ne monte pas** (0,88 contre 0,90 sur `spherical` hard).
- **Les modes sont dans l'arbre.** Le déficit est celui qu'a décrit l'audit (§ 1.2) : la masse sous le col, puis la
  sélection du niveau.

## 4. Stabilité mesurée (sorties comparées entre elles, jamais à la vérité)

### Tableau 6 — Retrait de points (Prohorov), bande du lemme 3

| perturbation, tranche | temps d'entrée hors bande | max (t_Y − t_X) / s | paires hors bande | ARI plat eom_r4 / eom_m3 / pf_smcs |
|---|---|---|---|---|
| 2 000 pts, p = 2, **oblique c = 3** (δ = 0,6 s) | **0** (64 scènes) | 0,142 | **0,02 %** | 0,9991 / 0,9998 / 0,9999 |
| 2 000 pts, p = 2, K = 10 fixe (δ = 0) | 1 314 | 0,181 | 1,7 % | 0,9995 / 0,9984 / 0,9928 |
| 2 000 pts, p = 2, K = 5 fixe (δ = 0) | 574 | 0,433 | 1,5 % | 0,9998 / 0,9925 / 0,9999 |
| 2 000 pts, p = 20 (1 %), oblique (δ = 6 s > r_0 : vide) | 0 | 0,300 | 0,19 % | 0,9912 / 0,9943 / 0,9885 |
| 2 000 pts, p = 20, K = 10 fixe | 11 093 | 0,478 | 14,6 % | 0,9882 / 0,9866 / 0,9769 |
| 2 000 pts, p = 20, K = 5 fixe | 4 986 | 0,935 | 6,5 % | 0,9824 / 0,9786 / 0,9905 |
| 8 000 pts « hard », p = 2, **oblique c = 3** (δ = 0,6 s) | **0** (16 scènes) | 0,132 | **0,01 %** | 0,9999 / 0,9999 / 1,0000 |
| 8 000 pts « hard », p = 2, K = 10 fixe | 247 | 0,100 | 0,58 % | 0,9972 / 0,9999 / 1,0000 |
| 8 000 pts « hard », p = 2, K = 5 fixe | 119 | 0,204 | 0,09 % | 0,9999 / 0,9999 / 0,9999 |

- **Unités et mode de comptage.**
  - s = médiane de d_10 ; « hors bande » veut dire hors de [t_X ; t_X + δ].
  - Pour les tranches horizontales, il n'existe aucun δ : on compte tout retard.
  - Les paires sont 20 000 paires tirées, avec la hauteur de fusion ultramétrique de la hiérarchie de points.
- **Le lemme 3 se vérifie exactement.** Toute violation aurait signalé une faute de construction.
- **La sortie plate, elle, est peu sensible pour toutes les tranches.** À ces perturbations, le banc ne discrimine
  pas.

### Tableau 7 — Vignoble (2 000 points, 64 scènes, eom_r4)

| comparaison | ARI moyen | médiane | part < 0,9 | min |
|---|---|---|---|---|
| horizontale K = 9 contre 10 | 0,972 | 0,988 | 0,06 | 0,735 |
| horizontale K = 8 contre 10 | 0,969 | 0,980 | 0,06 | 0,799 |
| oblique c = 3, r_0 contre 1,1 r_0 | **0,994** | 0,998 | **0,00** | 0,935 |
| oblique c = 5, r_0 contre 1,1 r_0 | **0,997** | 0,999 | 0,00 | 0,949 |
| oblique c = 3, intercept K_max 9 contre 10 | 0,969 | 0,992 | 0,08 | 0,497 |

- **Deux perturbations de même borne.** Les deux font une fenêtre d'ordre selon la prop. 43.
  - Changer la pente (r_0 × 1,1) ne bouge presque pas la sortie.
  - Changer l'intercept en masse la bouge autant que passer de K = 9 à K = 10.
- **Raison.** La stabilité de Rolle–Scoccola porte sur la hiérarchie, pas sur l'EOM.

### Tableau 7 bis — Vignoble à 8 000 points (16 scènes « hard »)

| tête | comparaison | ARI moyen | médiane | part < 0,9 | min |
|---|---|---|---|---|---|
| eom_r4 | horizontale K = 9 contre 10 | 0,984 | 0,982 | 0,00 | 0,966 |
| eom_r4 | horizontale K = 8 contre 10 | 0,957 | 0,975 | 0,06 | 0,642 |
| eom_r4 | oblique c = 3, r_0 contre 1,1 r_0 | 0,971 | 0,997 | 0,06 | 0,632 |
| eom_r4 | oblique c = 3, intercept 9 contre 10 | 0,953 | 0,985 | 0,12 | 0,644 |
| eom_r6 | horizontale K = 9 contre 10 | 0,982 | 0,982 | 0,00 | 0,940 |
| eom_r6 | oblique c = 3, r_0 contre 1,1 r_0 | 0,977 | 0,997 | 0,06 | 0,721 |
| eom_r6 | oblique c = 3, intercept 9 contre 10 | 0,936 | 0,973 | 0,12 | 0,644 |

- **À la taille d'intérêt, l'avantage de 2 000 points disparaît.** La médiane reste meilleure sur la pente (0,997),
  mais les pires cas sont aussi mauvais que sur l'horizontale, et même pires pour eom_r6.
- **Tous les pires cas sont des `filaments` hard**, pour les deux tranches.
- **Conclusion.** La stabilité démontrée de la hiérarchie ne se traduit pas en stabilité de la sortie EOM. Une
  sélection qui en hériterait reste à construire : PF à gap large, ou vignoble.

## 5. Coût

- **La tête multi‑K a besoin de toute la tour** : ordres 1..K_max et verticales.
  - Scène `spherical` hard, 8 000 points, K = 10, 1 fil, machine chargée : catalogue 23,7 s, tour 17,3 s.
  - Les cartes verticales pèsent 0,76 s, soit 4 % de l'étage tour.
  - Le temps d'une tour réduite à l'ordre 10 n'est pas mesuré.
- **Tranche**, en Python : 0,2 s (2 000 points) à 1,4 s (8 000 points), linéaire en nombre de nœuds vivants. En C++,
  ce serait négligeable.
- **Tête sur l'arbre réduit** : 0,01–0,03 s.
- **Mémoire** : la tranche ne garde que les nœuds vivants dans leur fenêtre (au plus le total de la tour).
- **Campagne** :
  - la grille complète (12 courbes, jusqu'à 16 têtes chacune, 2 remplissages) prend 8 s par scène à 2 000 points et
    45 s à 8 000 points, surtout en remplissage b1.5 et en ARI ;
  - les tours du cache (5,6 Go, dans le scratch, hors dépôt) ont été adoptées d'un autre concepteur. Elles sont
    identiques bit à bit à nos propres lectures sur 2 scènes vérifiées, G et T sont vérifiés sur toutes, et nous
    avons calculé nous‑mêmes 24 des 64 tours à 8 000 points. L'identité V1 sur les 128 scènes contrôle le tout, de
    bout en bout, pour les ordres 5 et 10.

## 6. Limites

- **Dev seulement.** Une scène par cellule. Le choix P1 est fait sur 2 000 points et testé sur 8 000, qui sont deux
  tailles du même espace dev. Aucune graine test n'a été lue.
- **Portée du théorème.** Il porte sur la multicouverture ambiante :
  - la hiérarchie de points dépend de la règle de couverture, non couverte (0,02 % des paires hors bande) ;
  - la sélection EOM n'a pas de garantie ;
  - la constante est vide au‑delà de K_max points perturbés.
- **Consistance.** À K_max fixe, la droite sort de la consistance du th. 58 (intercept en masse K_max / n → 0).
- **Pré‑choix.** P1 fixe z = 4 aux deux tailles. Le meilleur z de v10‑b à 8 000 points (z = 6) est choisi sur les
  mêmes scènes, ce qui avantage la base.
- **PF.** Le choix automatique de n (plus grand écart) est un choix de ma part. Rolle–Scoccola le laissent à
  l'utilisateur, par le vignoble. La variante par rapport n'est pas publiée.
- **K_max = 5.** Mesuré avec la même règle (c = 3, s = médiane de d_5), sans nouveau choix.

## 7. Recommandation

1. **Ne pas adopter** la tranche oblique comme tête de score à la place de v10‑b : aucun gain significatif, aux deux
   tailles.
2. **Garder la construction** (`oblique.py`, verticales + remontée, arbre réduit). C'est le seul objet de la chaîne
   dont la stabilité est démontrée et vérifiée exactement. Usages :
   - (a) **option de robustesse** : elle supprime l'effondrement des coquilles à petit z et le sur‑découpage à grand
     z. Ailleurs, elle ne gagne rien, et elle coûte sur `spherical` à petit z.
   - (b) **support d'une sélection par vignoble**, qui choisit la droite et le niveau dans une région stable, sans
     vérité.
     - Le vignoble en pente est lisse en médiane (tableaux 7 et 7 bis).
     - Ses pires cas (`filaments`) ne le sont pas avec l'EOM. Une tête qui hérite de la stabilité (PF à gap large)
       doit d'abord résoudre le choix du nombre d'amas (§ 3).
3. **L'objection de l'utilisateur ne se règle pas par la tranche.** Elle se règle par l'affectation de la masse sous
   le col et par le choix du niveau. Pour ce dernier, noter l'opposition des conventions du banc (`hierarchical`
   contre le reste) avant toute « sélection fondée ».
4. **Si une tête multi‑K est retenue**, préférer la droite (constante uniforme) à la courbe log‑linéaire. La constante
   de Lipschitz de la courbe log‑linéaire croît en log(r_0 / r_lo) et ses scores ne sont pas meilleurs.

## Fichiers

Tout est dans `/workspaces/E-HGP/build/v10-persist/multik/tranche_oblique/`.

- **`prototype/`**
  - `oblique.py` : lecture de la tour, courbes, tranche par les verticales, arbre réduit, EOM à λ quelconque, code‑barres
    et PF.
  - `run_grid.py` : grille.
  - `stability.py` : E1 vignoble, E2 retrait de points.
  - `verify.py` : V1 identité, V2 invariants, V3 réduction.
  - `analyse.py`, `paired.py`, `final_tables.py`, `stab_tables.py`, `baselines.py`, `fam.py` : tables.
  - `diag_pf.py` : diagnostic PF à n connu.
  - `build_cache.py`, `fill_cache_rev.py`, `mkscene.py` : cache des tours.
- **`resultats/`**
  - `grid_2000b.csv`, `grid_8000b.csv`, `grid5_2000.csv`, `grid5_8000.csv` ;
  - `e1_2000.csv`, `e1_8000_hard.csv`, `e2_2000.csv`, `e2_8000_hard.csv`, `diag_pf_2000.csv` ;
  - `PRECHOIX_8000.txt`, `verify.txt`, `tables.md` et les journaux ;
  - `SHA256SUMS`.
- **Rejouer**, depuis un répertoire de travail contenant `cache/` (tours) et `runs/` :

  ```bash
  python3 run_grid.py --out runs/grid_2000b.csv --sizes 2000
  python3 verify.py runs/grid_2000b.csv
  ```
