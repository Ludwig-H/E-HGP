# Représenter ROBUSTEMENT les niveaux d'un polyèdre d'ordre K : modèles de perturbation, identité des nœuds, géométrie certifiable

6 octobre 2026, rédaction commencée à 22 h 42 UTC (heure lue par `date -u`). Rôle : théoricien du workflow
« complexe alpha d'ordre K », volet robustesse. Cadre :

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

GCP non utilisé. Aucun commit, aucune écriture hors de `build/`. Statuts employés : **prouvé** (preuve complète
ici ou résultat primaire cité), **prouvé, référence de mémoire** (théorème classique cité sans relecture dans ce
workflow : à vérifier avant tout usage contractuel), **vérifié borné** (calcul exact sur petits nuages, oracle de
correction seulement), **mesuré** (sorties réelles du moteur), **conjecture**.

Point de départ : les deux réponses de l'auditeur, lues en entier — `d2be6bdc7`
(`receipts/audit_hartigan_delaunay_20261006/README.md`) et `28d70f8ab`
(`receipts/audit_hartigan_robustesse_20261006/README.md`, capsules `robustesse/` et `ombre_et_niveaux/`). Je ne
refais pas ses preuves ; je les cite par [A·] et je les prolonge.

## 0. Résultats en bref

1. **Un seul énoncé porte toute la robustesse topologique : l'entrelacement de la tour.** Déplacement apparié δ,
   d suppressions et e ajouts donnent des applications π0 de la tour de P vers celle de P' décalées de (+δ en r,
   −d en k) et retour (+δ, −e), dont les composées sont les applications de structure de la tour (R0, prouvé
   à partir de [A2]). À k fixé, seul le déplacement apparié est contrôlé ; tout le reste décale l'ordre.
2. **L'identité d'un nœud HGP n'est pas stable, même sous un déplacement infinitésimal** : la convention FULL
   (toute fusion tue ses enfants) coupe un nœud long en deux dès qu'une composante éphémère le rejoint. Théorème N1
   (prouvé) : sous déplacement δ, un nœud de vie > 2δ a pour image une **chaîne** de nœuds dont les seules coupures
   intérieures sont des fusions avec des branches de vie ≤ 2δ ; deux nœuds robustes simultanés ont des images
   distinctes. La borne est atteinte qualitativement (fixture exacte `coupure`, quatre sites, k = 2).
3. **Mesuré sur les arbres publiés par mhgp11** (trois trames, n = 35 551 à 45 845) : à δ = √3/2 mm (arrondi au mm),
   la part des nœuds de vie ≤ 2δ est de 12–18 % à K = 1, 41–49 % à K = 2, **61–67 % à K = 5**, 73–79 % à K = 10
   (vie médiane 0,58 à 0,87 mm à K = 5). Les deux tiers des nœuds de K = 5 n'ont aucune correspondance garantie
   sous le seul bruit de quantification : une hiérarchie de polyèdres robuste se lit sur l'**arbre δ-contracté**.
4. **Quantification avec fusion de sites** : l'ensemble dédupliqué S vérifie Ω_k^S(r) ⊆ Ω_k^P(r+δ) et
   Ω_k^P(r) ⊆ Ω_{k−e}^S(r+δ), où e est l'excès local de multiplicité ; avec multiplicités, l'ordre est conservé
   (Q1, prouvé). FULL unitaire n'a qu'une garantie unilatérale. Mesuré : aucune fusion à 1 mm sur les trois trames,
   donc e ≡ 0 et la quantification y est un pur déplacement apparié.
5. **Échantillonnage** : toute ε-approximation non pondérée décale l'ordre d'au moins εn ≥ 1 ; aux ordres HGP
   (K ≤ 10, n ≈ 4·10⁴), seul un transport **pondéré** conserve l'ordre (S1, prouvé).
6. **Géométrie** : pour K = 1 la région Ω_1(r) est δ-stable en Hausdorff sans hypothèse (union de boules) ; seul le
   représentant alpha saute. Pour K ≥ 2 la région elle-même saute. Le gradient généralisé de d_k a une norme
   explicite √(1 − R_{Σ(x)}²/d_k(x)²) (G1, prouvé) ; si son rapport τ reste < 1 sur la coquille d'une composante,
   alors dist(y, C) ≤ (d_k(y) − r)/√(1−τ²) **dans la même composante** et d_H(C^P, C^{P'}) ≤ κδ (G2–G3,
   prouvés, référence de mémoire pour le lemme de déformation non lisse). κ se certifie par des rayons de boules
   minimales des labels de la mosaïque ; FULL seul ne le peut pas (il ne publie que les événements H0).
7. **Axe des ordres** : la distance à la mesure est exactement la moyenne quadratique des tranches de la
   bifiltration le long de l'axe des ordres ; c'est cette moyenne qui achète la stabilité Wasserstein à paramètre
   fixé, au prix de l'identité (k, r). D1 (prouvé) place {f_k ≤ r} dans un escalier de la bifiltration.
8. **Définition opérationnelle** (§ 5) : une représentation (δ, m)-robuste des niveaux = identité sur l'arbre
   δ-contracté et la profondeur d'ordre m, topologie certifiée sur toute la filtration, couverture exacte par
   labels, coupes intérieures robustes [b_v + δ, d_v − 2δ), erreurs géométriques mesurées et, si la stabilité du
   dessin est revendiquée, certificat κ et marge combinatoire. A_k exact et A_k réduit à sommets protégés satisfont
   RR1–RR4 ; aucun candidat ne satisfait RR5 de façon établie ; le réduit est le seul qui puisse y prétendre sans
   perdre RR1–RR3.

## 1. Cadre et notations ; ce que l'on reprend de l'auditeur

P est un ensemble fini de n sites de poids unitaire (ou une mesure pondérée μ = Σ w_p δ_p), boules fermées,
dimension 3 (tout reste vrai en dimension quelconque). On note N_μ(y, r) = μ(B̄(y, r)), et pour un seuil de masse t,

$$\Omega^{\mu}_{t}(r)=\lbrace y\in\mathbb{R}^{3} : N_{\mu}(y,r)\ge t\rbrace ,\qquad \Omega_{k}(r)=\Omega^{P}_{k}(r)=\lbrace d_{k}\le r\rbrace .$$

**Bifiltration et tour.** (r, k) ≤ (r', k') si r ≤ r' et k ≥ k' ; alors Ω_k(r) ⊆ Ω_{k'}(r'). La **tour FULL** est
le foncteur T_P(r, k) = π0 Ω_k(r) sur ce poset (k entier), avec les applications de structure induites par les
inclusions (horizontales en r, verticales en k). C'est le contenu de FULL : par le théorème 2 de la thèse, π0 Γ_K
calcule T_P(·, K) ; les « supports » sont un arbre couvrant de ce graphe.

**Nœuds (convention FULL).** À k fixé, un nœud v est une famille (C_v(r)) pour r ∈ [b_v, d_v) avec
ι_{r,r'}^{-1}(C_v(r')) = {C_v(r)} pour b_v ≤ r ≤ r' < d_v (aucune fusion pendant la vie) ; b_v est une naissance
(aucune préimage avant) ou une fusion (au moins deux préimages juste avant) ; d_v est sa fusion. **Toute fusion tue
tous ses enfants et fait naître un parent**, et une naissance au contact d'une composante existante n'est pas un
événement (la composante grandit). La vie de v est d_v − b_v (rayons, pas rayons carrés).

**Repris de l'auditeur, sans le refaire.**

- [A1] Bijection de déplacement ≤ δ : ‖d_k^P − d_k^{P'}‖∞ ≤ δ, d'où Ω_k^P(r) ⊆ Ω_k^{P'}(r+δ) et réciproquement ;
  entrelacement des A_k à homotopie près par les équivalences naturelles A_k ≃ Ω_k (`d2be6bdc7` § 1.1).
- [A2] Théorème de comptage (capsule `robustesse` § 5) : d suppressions, e ajouts, déplacement δ des sites
  conservés : N_{P'}(y, r+δ) ≥ N_P(y, r) − d et N_P(y, r+δ) ≥ N_{P'}(y, r) − e, donc
  Ω_k^P(r) ⊆ Ω_{k−d}^{P'}(r+δ) et Ω_k^{P'}(r) ⊆ Ω_{k−e}^P(r+δ).
- [A3] Moins de k aberrants à distance > 2r de P ne changent pas Ω_k(r) ; sans séparation, ils peuvent créer une
  composante ou un pont (P = {0, 6, 12} + 1 ; P = {0, 2, 5} + 3, k = 2, r = 2).
- [A4] Pas de stabilité de Hausdorff du dessin à rayon fixé : perte de composante ({0, 2, L, L+1}, k = 2) et saut
  d'alpha à homotopie constante (triangle {(−4,0), (4,0), (1,2)}, k = 1, r² = 377/16).
- [A5] Sommets protégés : d_H(L_{r,v}, A_{k,v}(r)) ≤ D_v(r) et d_H(L_{r,v}, C_v(r)) ≤ r ; certificat d'effondrement
  sur toute la plage de filtration (A = (0,6), B = (4,6), C = (2,7), D = (2,0)).
- [A6] Ombre S_v(r) : P ∩ S_v(r) = P ∩ (C_v(r) ⊕ B_r), S_v(r) ⊆ C_v(r) ⊕ B_r, d_H(S_v(r), C_v(r)) ≤ r ; pas un
  représentant homotopique (P = {0, 2, 4}, k = 2, r = 1).
- [A7] Coupe de fin de vie A_v(d_v^−) = {a_σ < d_v²} ; coupe intérieure avec marges ; pas de plus grande coupe Ω
  avant la mort (P = {0, 2, 10}, k = 2).

## 2. Modèles de perturbation : filtrations, arbre, correspondance des nœuds

### 2.1 Un énoncé unique : l'entrelacement de la tour

**Définition.** Deux tours T, T' sont (δ; d, e)-entrelacées s'il existe des familles d'applications
φ_{r,k} : T(r, k) → T'(r+δ, k−d) et ψ_{r,k} : T'(r, k) → T(r+δ, k−e), naturelles (elles commutent aux
applications de structure), telles que ψ ∘ φ et φ ∘ ψ soient les applications de structure
(r, k) → (r+2δ, k−d−e). Les grades k ≤ 0 sont interprétés comme ℝ³ entier (une composante).

**Théorème R0 (prouvé).** Sous les hypothèses de [A2], T_P et T_{P'} sont (δ; d, e)-entrelacées, ainsi que les
modules H_i(A_k(r)) et les classes d'homotopie des A_k(r) (par les équivalences naturelles de [A1]).

*Preuve.* Les inclusions de [A2] sont compatibles entre elles et avec celles de la bifiltration (ce sont toutes des
inclusions de sous-ensembles de ℝ³) ; π0, H_i et la classe d'homotopie sont des foncteurs, et la composée des deux
inclusions est l'inclusion de structure. ∎

Conséquences immédiates : (i) **à k fixé**, la tranche T_P(·, k) n'est entrelacée avec T_{P'}(·, k) que si
d = e = 0 ; (ii) les diagrammes de persistance H0 de d_k^P et d_k^{P'} sont à distance de goulot ≤ δ (stabilité
des diagrammes, Cohen-Steiner–Edelsbrunner–Harer 2007 : référence de mémoire) ; (iii) R0 ne dit rien de l'identité
des **nœuds**, qui sont des arcs de l'arbre et pas des classes de persistance : c'est l'objet de N1.

### 2.2 Déplacement apparié δ (bruit, quantification sans fusion) : le théorème N1

Hypothèses : bijection P → P' de déplacement ≤ δ, ordre k fixé, **aucune position générale** (les ex æquo de la
grille sont permis). Notons φ_r : π0 Ω^P(r) → π0 Ω^{P'}(r+δ) et ψ_r symétrique, induites par [A1] ; donc
ψ_{r+δ} ∘ φ_r = ι_{r→r+2δ} et φ commute aux ι. Pour un nœud v de P, sa **trace** est
τ_v(t) = φ_{t−δ}(C_v(t−δ)) ∈ π0 Ω^{P'}(t), t ∈ [b_v + δ, d_v + δ).

**Théorème N1 (prouvé).** Soit v un nœud de P de vie d_v − b_v > 2δ.

- (a) La trace monte dans l'arbre de P' : τ_v(t') = ι'(τ_v(t)) pour t ≤ t'. Elle est portée par une chaîne de
  nœuds de P' (enfant → parent).
- (b) Toute fusion de P' au rayon s ∈ (b_v + δ, d_v − δ) qui tue le nœud portant la trace ne fait intervenir,
  en plus de lui, que des nœuds de vie ≤ 2δ (nés dans [s − 2δ, s)).
- (c) Si u ≠ v sont deux nœuds de P vivants sur [r, r+2δ], alors φ_r(C_u(r)) ≠ φ_r(C_v(r)).
- (d) Si v est un nœud de naissance (pas de fusion), le nœud de P' qui porte la trace en b_v + δ est né dans
  [b_v − δ, b_v + δ].
- Tout cela vaut en échangeant P et P'.

*Preuve.* (a) naturalité de φ. (c) ψ_{r+δ}(φ_r(C_u(r))) = C_u(r+2δ) ≠ C_v(r+2δ) = ψ_{r+δ}(φ_r(C_v(r))), car deux
nœuds vivants au même rayon ont des composantes distinctes.

(b) Soit s ∈ (b_v + δ, d_v − δ) une fusion de P' qui tue le nœud T' portant la trace juste avant s, M' la composante
fusionnée, et D' ≠ T' un autre nœud mourant en s. Par naturalité, τ_v(s) = M'. Comme v vit sur [s−δ, s+δ],
ψ_s(M') = ψ_s(φ_{s−δ}(C_v(s−δ))) = ι(C_v(s−δ)) = C_v(s+δ). Supposons b_{D'} < s − 2δ et prenons
t ∈ [max(b_{D'}, b_v − δ), s − 2δ), intervalle non vide car s > b_v + δ. Posons X = ψ_t(D'(t)), composante de
Ω^P(t+δ) avec t + δ ∈ [b_v, d_v). Par naturalité, ι_{t+δ→s+δ}(X) = ψ_s(ι'(D'(t))) = ψ_s(M') = C_v(s+δ). Comme v ne
subit aucune fusion sur [t+δ, s+δ], la seule préimage de C_v(s+δ) au rayon t+δ est C_v(t+δ) : X = C_v(t+δ). Alors
D'(t+2δ) = ι'(D'(t)) = φ_{t+δ}(X) = φ_{t+δ}(C_v(t+δ)) = τ_v(t+2δ), avec t + 2δ < s : la trace serait portée par D'
jusqu'à sa mort en s, contredisant le fait qu'elle est portée par T' ≠ D' juste avant s. Donc b_{D'} ≥ s − 2δ.

(d) Si le nœud T'_0 portant τ_v(b_v + δ) était né en b' < b_v − δ, Y = ψ_{b'}(T'_0(b')) serait une composante de
Ω^P(b'+δ), b' + δ < b_v, avec ι(Y) = ψ(τ_v(b_v+δ)) = C_v(b_v + 2δ) ; une composante antérieure à b_v devrait
alors rejoindre v avant b_v + 2δ < d_v, ce qu'interdit la naissance isolée de v suivie d'une vie sans fusion. ∎

**Proposition N2 (la coupure existe ; vérifié borné, coordonnées exactes).** P = {a = (−1,0,0), b = (1,0,0),
c = (0,1,0), d = (0, 11/10, 0)}, k = 2 ; P' déplace c en (0, 101/100, 0) (δ = 1/100). Dans P, c est sur le cercle de
diamètre ab : la lentille {a, b} naît en r² = 1 **au contact** de la composante (aucun événement) ; l'arbre de P a
quatre nœuds, la racine naît en 221/400 et vit jusqu'à l'infini. Dans P', la même lentille naît isolée en 1 et
fusionne en 408080401/408040000 (cercle circonscrit de a, b, c') : la racine de P correspond à **deux** nœuds de P'
séparés par une fusion avec une branche de vie ≈ 4,95·10⁻⁵ ≤ 2δ. La coupure ne disparaît pas quand δ → 0 : pour
tout ε > 0, c' = (0, 1+ε, 0) donne une lentille {a, b} née isolée en r² = 1 (|m − c'| = 1+ε > 1 au milieu m de ab)
et fusionnant en r² = 1 + y_0², y_0 = ((1+ε)² − 1)/(2(1+ε)) > 0. C'est l'identité du nœud, et non la tour, qui est
discontinue.

*Remarque (fusions multiples).* Les fusions multiples ne sont pas toutes des dégénérescences. Les k+1 lentilles
d'une (k+1)-partie se rencontrent toutes à sa boule minimale au même rayon (les intersections deux à deux sont
la même intersection de k+1 boules) : ces fusions triples sont **génériques**. Dans l'exemple à six points (§ 6.1),
les fusions triples internes aux triangles survivent à toute perturbation ; seule la fusion finale (quatre contacts
simultanés par symétrie) se scinde.

**Corollaire N3 (arbre δ-contracté).** Contracter, dans l'arbre de P, toute fusion dont un seul enfant a une vie
> 2δ (le parent continue cet enfant) et oublier les nœuds de vie ≤ 2δ donne des **chaînes robustes** ; N1 les
apparie injectivement à celles de P' sur leurs intérieurs [b + δ, d − δ]. C'est le bon support des identités
d'une hiérarchie de polyèdres soumise au bruit δ.

**Mesure (mhgp11 07428324e, sorties `supports` en cache du harnais, trois trames sans sol).** Vie = rayon du
parent − rayon du nœud. Part des nœuds éphémères (vie ≤ 2δ), à δ = 0,866 mm puis 3 mm :

| Trame (n) | K | nœuds | vie médiane (mm) | éphémères δ = 0,866 | robustes | fusions coupées / fusions | éphémères δ = 3 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 08/000000 (39 885) | 1 | 79 681 | 22,8 | 13,7 % | 68 807 | 8 502 / 39 796 | 25,9 % |
| 08/000000 | 2 | 178 127 | 2,57 | 43,9 % | 99 908 | 26 893 / 77 038 | 63,8 % |
| 08/000000 | 5 | 576 371 | 0,631 | 65,9 % | 196 613 | 80 400 / 235 290 | 85,3 % |
| 08/000000 | 10 | 1 638 573 | 0,256 | 78,4 % | 353 907 | 186 762 / 659 223 | 93,6 % |
| 08/000100 (35 551) | 5 | 478 265 | 0,874 | 60,9 % | 186 992 | 68 301 / 195 058 | 80,7 % |
| 08/000200 (45 845) | 5 | 609 376 | 0,583 | 66,9 % | 202 002 | 77 615 / 248 050 | 85,6 % |

(Tableau complet, K = 1, 2, 5, 10 pour les trois trames : `resultats_vies_full.json`.) « Fusions coupées » = fusions
où un seul enfant est robuste et au moins un autre éphémère : la moitié des fusions à K = 5 ne sont pas des
rencontres entre objets robustes. À δ = 10 mm, les nœuds robustes ne sont plus que 16 044 à 31 514 pour K ≥ 2
selon la trame et l'ordre (43 012 / 29 471 / 20 485 / 16 044 pour K = 1, 2, 5, 10 sur 08/000000), soit 94 à 97 %
d'éphémères à K = 5 et 98 à 99 % à K = 10 : l'arbre robuste garde 0,4 à 0,8 nœud par site à K = 5 et 10 quand l'arbre
publié passe de 2 (K = 1) à 41 (K = 10) nœuds par site. Statut : **mesuré** sur trois trames de taille native ; aucune pente d'échelle
n'est déduite (les tailles d'échelle 8 000 / 16 000 / 32 000 n'ont pas été mesurées ici).

### 2.3 Quantification avec fusion de sites : le modèle pondéré

Soit P le multi-ensemble des retours bruts, q : P → G l'arrondi au plus proche (|q(p) − p| ≤ δ = √3 h/2),
S = q(P) l'ensemble des sites distincts, m_g = |q^{-1}(g)|, μ_S = Σ_g m_g δ_g la mesure image, et l'excès local

$$e(\rho)=\max_{y\in\mathbb{R}^{3}}\sum_{g\in S\cap\bar{B}(y,\rho)}(m_{g}-1).$$

**Théorème Q1 (prouvé).** (i) Ω_k^P(r) ⊆ Ω_k^{μ_S}(r+δ) ⊆ Ω_k^P(r+2δ) : avec multiplicités, l'ordre est conservé.
(ii) Pour l'ensemble dédupliqué de poids unitaires : Ω_k^S(r) ⊆ Ω_k^P(r+δ) et Ω_k^P(r) ⊆ Ω_{k−e(r+δ)}^S(r+δ).

*Preuve.* Chaque retour dans B̄(y, r) a son image dans B̄(y, r+δ), et chaque image a au moins une préimage à
distance ≤ δ ; donc N_{μ_S}(y, r+δ) ≥ N_P(y, r) et N_P(y, r+δ) ≥ N_{μ_S}(y, r). Ensuite
N_S(y, ρ) = N_{μ_S}(y, ρ) − Σ_{g ∈ B̄(y,ρ)} (m_g − 1) ∈ [N_{μ_S}(y, ρ) − e(ρ), N_{μ_S}(y, ρ)]. ∎

Le témoin de l'auditeur P = {1/10, 1/5, 10, 20} (capsule `robustesse` § 2) est le cas e = 1. **Ce que FULL permet :**
le moteur unitaire calcule Ω^S ; il garantit seulement Ω_k^S(r) ⊆ Ω_k^P(r+δ) (la déduplication perd de la masse).
**Ce qu'il faudrait changer :** compter les multiplicités dans le prédicat (k-ième voisin avec multiplicité, soit
la tour de la mesure μ_S) ; à défaut, publier e(ρ) aux rayons utiles, ce qui transforme la garantie en décalage
d'ordre déclaré. N1 s'applique alors au modèle pondéré sans changement de preuve.

**Mesuré** (`mesure_fusions.py`, retours bruts SemanticKITTI 08 du manifeste, arrondi floor(1000x + 1/2)) : **aucune
fusion** à 1 mm sur les trois trames (123 389, 124 479 et 125 526 retours bruts ; 39 885, 35 551 et 45 845 retours
hors sol, exactement les tailles des trames). Donc e ≡ 0 sur ces trames : la quantification y est un pur déplacement
apparié δ ≤ √3/2 mm, et N1 s'applique tel quel aux arbres de mhgp11 (§ 2.2). Le modèle pondéré reste nécessaire pour
les découpes recousues, l'agrégation de plusieurs balayages ou une grille plus grossière.

### 2.4 Modifications de population : ajouts, suppressions, aberrants

R0 avec δ = 0 donne un entrelacement **vertical** : φ : T_P(r, k) → T_{P'}(r, k−d), ψ : T_{P'}(r, k) → T_P(r, k−e).

**Définition (profondeur d'ordre).** Un nœud v de P vivant en (r, k) a une profondeur d'ordre ≥ m si C_v(r) et
la composante de tout autre nœud vivant en (r, k) restent dans des composantes distinctes de Ω_{k−m}(r).

**Proposition N4 (prouvé).** Si d + e ≤ m et si v et u sont vivants en (r, k), de profondeur d'ordre ≥ m, alors
φ(C_v(r)) ≠ φ(C_u(r)) dans T_{P'}(r, k − d). *Preuve* : comme N1 (c), avec ψ ∘ φ = application verticale (r, k) → (r, k−d−e) et
k − d − e ≥ k − m. ∎

Ainsi « robuste à m aberrants » se lit sur une **fenêtre d'ordres de largeur m**, jamais à k fixé ([A3] montre
qu'un seul point suffit à créer une naissance ou un pont). L'exemple de l'anneau (§ 6.2) le montre sur une classe
H1 : un aberrant central ramène la mort du trou de r² = 1 à r² = 5/18 à k = 1, et ne la change pas à k = 2.

### 2.5 Masse déplacée loin, Wasserstein, échantillonnage avec contrat de masse

Mesures de probabilité μ, ν, seuil de masse t ∈ [0, 1] (t = k/n pour n sites unitaires).

**Proposition P1 (prouvé ; distances de référence de mémoire).** (i) Si la distance de Lévy–Prokhorov π(μ, ν) ≤ ε,
alors Ω_t^μ(r) ⊆ Ω_{t−ε}^ν(r+ε) et symétriquement : la bifiltration est entrelacée de (+ε, −ε), ce qui est le
cadre de la stabilité de Blumberg–Lesnick (FoCM 2024). En unités HGP, c'est un décalage d'ordre εn. (ii) Déplacer
m sites arbitrairement loin donne π ≤ m/n mais, plus finement, (δ = 0, d = e = m) dans R0 : seul l'ordre bouge.
(iii) π(μ, ν)² ≤ W_1(μ, ν) ≤ W_2(μ, ν) (Gibbs–Su 2002) : une petite distance de Wasserstein donne un entrelacement
en (r, t) de √W_1 dans les deux paramètres, jamais à t fixé (π mélange longueur et masse : l'énoncé dépend de
l'unité de longueur, que l'on fixe en la déclarant).

*Preuve de (i).* B̄(y, r) a son ε-voisinage ouvert dans B̄(y, r+ε), donc μ(B̄(y, r)) ≤ ν(B̄(y, r+ε)) + ε. ∎

**Proposition S1 (prouvé).** Soit S une ε-approximation de P pour les boules fermées
(|N_S(B)/|S| − N_P(B)/n| ≤ ε pour toute boule B). Alors, en masse normalisée, Ω_{t+ε}^P(r) ⊆ Ω_t^S(r) ⊆ Ω_{t−ε}^P(r)
**au même rayon** ; en ordre HGP, un décalage de εn. Si S ⊊ P, alors ε ≥ 1/n (une petite boule autour d'un site
retiré), donc le décalage est ≥ 1 ordre. Une ε-approximation aléatoire exige |S| d'ordre ε⁻² log(1/ε) (classe de VC
finie des boules ; référence de mémoire) : εn < 1 demanderait |S| > n², impossible pour S ⊆ P. *Preuve* :
définitions. ∎

Conséquence : à K ≤ 10 et n ≈ 4·10⁴, **aucun sous-échantillon non pondéré ne garde l'ordre** ; le seul contrat
qui garde l'ordre est le transport à poids ([A] § 1.3, item 1) : chaque site va à un représentant à distance ≤ δ
avec son poids, et la tour pondérée est δ-entrelacée à ordre fixe. C'est une tour **pondérée**, pas FULL unitaire.

### 2.6 Bilan : ce que FULL unitaire garantit et ce qu'il faudrait changer

| Perturbation | Filtrations A_k | Arbre à k fixé | Correspondance des nœuds | FULL unitaire aujourd'hui | À changer |
| --- | --- | --- | --- | --- | --- |
| Déplacement δ (bijection) | entrelacement δ (R0) | goulot H0 ≤ δ | chaînes robustes, vie > 2δ (N1) | oui | publier δ et l'arbre δ-contracté |
| Quantification avec fusions | entrelacement δ avec multiplicités (Q1) | idem si pondéré | idem si pondéré | unilatéral (perte de masse) | multiplicités dans le prédicat, ou e(ρ) publié |
| Ajouts / suppressions / aberrants | entrelacement (0; d, e) vertical | aucune garantie à k fixé | profondeur d'ordre (N4) | la tour entière est calculée : oui | publier la profondeur d'ordre |
| Masse déplacée loin (Prokhorov) | entrelacement (ε, εn) | aucune | N4 avec décalage de rayon | oui (tour) | idem |
| Échantillonnage | transport pondéré : δ à ordre fixe ; non pondéré : décalage ≥ 1 ordre (S1) | idem | idem | non (unitaire) | poids de transport |

### 2.7 Le cas K = 1 (complexe alpha)

Tout ce qui précède redonne, à K = 1, des faits connus : R0 est la stabilité de la liaison simple (hauteurs de
fusion δ-stables ; Carlsson–Mémoli 2010, prop. 26, résumé du dépôt) ; N1 dit que les nœuds internes du dendrogramme
de vie ≤ 2δ (fusions presque simultanées) n'ont pas d'identité robuste, ce que montre la fixture `triple` ; Q1 (ii)
est inutile car Ω_1 ne dépend que du support : **à K = 1 la déduplication est sans effet** (Ω_1^P(r) ⊆ Ω_1^S(r+δ)
directement) ; D1 est trivial (f_1 = d_1) ; la profondeur d'ordre n'a pas de sens (pas d'ordre inférieur), ce qui
est exactement la fragilité de K = 1 aux aberrants (§ 6.2). Côté représentants : A_1(r) est le complexe alpha
([A] § 1), et l'**ombre coïncide avec lui** à K = 1 (les labels sont des singletons, les barycentres sont les sites,
conv(∪Q) d'une cellule est la cellule elle-même).

## 3. Stabilité géométrique : sous quelles hypothèses certifiables ?

### 3.1 K = 1 contre K ≥ 2

**Proposition G0 (prouvé).** Pour k = 1, d_H(Ω_1^P(r), Ω_1^{P'}(r)) ≤ δ pour tout r (chaque boule B̄(p, r) est à
distance de Hausdorff ≤ δ de B̄(p', r)). Composante par composante : si r ≥ δ et si aucune fusion de P ni de P' n'a
lieu dans [r − δ, r] pour les composantes considérées, alors la composante C de P et la composante C' de P' qui
contient C ∩ Ω_1^P(r−δ) vérifient d_H(C, C') ≤ δ (les boules B̄(p', r) ⊇ B̄(p, r−δ) des sites de C sont toutes dans
C', car C ∩ Ω_1^P(r−δ) est connexe ; symétriquement). Le saut du triangle [A4] est donc un saut du **représentant** alpha, pas de la région. Pour
k ≥ 2, Ω_k = ∪ W_Q est une union d'**intersections** de boules : une lentille peut disparaître sous un déplacement
arbitrairement petit ([A4], {0, 2, L, L+1}) ; la région elle-même est instable et il faut une hypothèse.

### 3.2 Le gradient généralisé de d_k et la constante κ

Pour x avec ρ = d_k(x) > 0, notons Σ(x) les sites à distance exactement ρ et R_Σ le rayon de la plus petite boule
contenant Σ(x).

**Lemme G1 (prouvé).** Le sous-différentiel de Clarke de d_k en x est contenu dans (x − conv Σ(x))/ρ, et

$$\min\lbrace \Vert\xi\Vert : \xi\in(x-\mathrm{conv}\,\Sigma(x))/\rho\rbrace =\frac{\mathrm{dist}(x,\mathrm{conv}\,\Sigma(x))}{\rho}=\sqrt{1-R_{\Sigma(x)}^{2}/\rho^{2}}.$$

*Preuve.* d_k = min_Q g_Q avec g_Q(x) = max_{q ∈ Q} |x − q| ; le sous-différentiel de Clarke d'un min fini est dans
l'enveloppe convexe de ceux des fonctions actives, celui d'un max de fonctions lisses dans l'enveloppe des gradients
actifs (Clarke 1983, prop. 2.3.12 : référence de mémoire). Les gradients actifs sont (x − σ)/ρ, σ ∈ Σ(x). Pour
l'égalité : soit c le centre de la boule minimale de Σ, c = Σ λ_σ σ (poids convexes sur les sites de son bord).
|σ − x|² = ρ² et |σ − c|² ≤ R² donnent ⟨σ − c, x − c⟩ ≤ 0 pour tout σ ∈ Σ, donc c est la projection de x sur
conv Σ ; la moyenne pondérée des |σ − x|² = |σ − c|² + |c − x|² + 2⟨σ − c, c − x⟩ sur les sites du bord donne
ρ² = R² + |c − x|². ∎

Pour k = 1, c'est le gradient de Lieutier de la fonction distance. **Remarque décisive :** la pente forte
(descente la plus raide) ne suffit pas : au milieu de deux sites, k = 1, elle vaut 1 alors que le point est une
selle (fusion) ; une borne d'erreur à la Ekeland tirée de la pente forte ne voit donc pas les fusions. Le contrôle
des composantes demande la borne de Clarke ci-dessus, qui s'annule en tout point critique au sens de Clarke (x centre
de la boule minimale de Σ(x)) : naissances, fusions, événements d'homologie supérieure, et éventuellement des points
sans effet topologique. Elle est semi-continue : pour y voisin de x, Σ(y) ⊆ Σ(x) (les sites à distance ≠ d_k(x) de x
restent hors de la sphère de y).

**Théorème G2 (borne d'erreur par composante ; prouvé, lemme de déformation de référence de mémoire).** Soit C une
composante de Ω_k(r), Δ > 0, U la composante de Ω_k(r+Δ) qui contient C, et

$$\tau=\sup\lbrace R_{\Sigma(x)}/d_{k}(x) : x\in U,\ r<d_{k}(x)\le r+\Delta\rbrace <1 .$$

Alors (i) U ∩ Ω_k(r) = C et C ↪ U est une équivalence d'homotopie (ni naissance, ni fusion, ni autre changement
topologique dans U entre r et r+Δ) ; (ii) pour tout y ∈ U, dist(y, C) ≤ κ (d_k(y) − r)_+ avec κ = (1 − τ²)^{-1/2}.

*Preuve.* Par G1, dist(0, ∂_C d_k(x)) ≥ μ0 = √(1 − τ²) sur la coquille. Le lemme de déformation des fonctions
localement lipschitziennes (pseudo-gradient de Clarke, Chang 1981 : référence de mémoire) fournit, pour tout
ε > 0, un champ localement lipschitzien V, |V| ≤ 1, le long duquel d_k décroît à vitesse ≥ (1−ε) μ0 ; on prend
V(x) proche de −ξ0/|ξ0|, ξ0 l'élément de norme minimale de (x − conv Σ(x))/ρ, qui vérifie ⟨ξ, −ξ0/|ξ0|⟩ ≤ −|ξ0|
pour tout ξ de cet ensemble convexe ; la semi-continuité Σ(y) ⊆ Σ(x) étend l'inégalité, à (1−ε) près, à un voisinage
de x, et une partition de l'unité recolle ces directions ; le long d'une trajectoire, la dérivée de Dini de d_k est
majorée par la dérivée de Clarke dans la direction V. Le flot arrêté au niveau r est une rétraction par déformation forte de U sur
U ∩ Ω_k(r) ; chaque trajectoire reste dans U (connexe, d_k décroissant) et a une longueur ≤ (d_k(y) − r)/((1−ε)μ0).
U ∩ Ω_k(r), rétracte d'un connexe, est connexe et contient C : c'est C. On fait ε → 0 (C est fermé). La même preuve
vaut pour toute composante U de Ω_k(r+Δ) sur laquelle τ < 1 : le flot atteint le niveau r dans U, donc U rencontre
Ω_k(r) en une seule composante (pas de naissance dans U). ∎

**Corollaire G3 (stabilité géométrique certifiée ; prouvé).** Bijection de déplacement ≤ δ. Soit C = C_v^P(r),
W la composante de Ω^P(r+2δ) qui contient C et W' la composante de Ω^{P'}(r+2δ) qui contient C (C ⊆ Ω^{P'}(r+δ) est
connexe). Si τ_P < 1 sur W ∩ {r < d_k^P ≤ r+2δ} et τ_{P'} < 1 sur W' ∩ {r < d_k^{P'} ≤ r+2δ}, alors
C' = W' ∩ Ω^{P'}(r) est une composante de Ω^{P'}(r) et

$$d_{H}(C_{v}^{P}(r),C'^{P'}(r))\le\delta\,\max(\kappa_{P},\kappa_{P'}).$$

*Preuve.* G2 (i) pour P' sur W' : C' est connexe, c'est une composante. Soit U' la composante de Ω^{P'}(r+δ)
contenant C ; le flot de P' la rétracte sur U' ∩ Ω^{P'}(r), connexe, non vide, réunion de composantes de Ω^{P'}(r)
et contenu dans la composante C', donc égal à C' ; ainsi C' ⊆ U'. Or U' ⊆ Ω^P(r+2δ) est connexe et contient C, donc U' ⊆ W. Tout y ∈ C' vérifie d_k^P(y) ≤ r+δ et y ∈ W,
donc dist(y, C) ≤ κ_P δ par G2 (ii) (Δ = 2δ). Tout x ∈ C vérifie d_k^{P'}(x) ≤ r+δ et x ∈ W', donc
dist(x, C') ≤ κ_{P'} δ. ∎

**Comment certifier κ sur des données.** Si x appartient à la fermeture d'une face F de la mosaïque d'ordre k, les
sites à distance ≤ d_k(x) de x sont dans la réunion des labels des cellules incidentes, donc
R_{Σ(x)} ≤ R(∪ labels(σ_F)), où σ_F est la cellule duale de F. D'où le certificat calculable exactement :

$$\tau\le\max\lbrace R(\cup\,\mathrm{labels}(\sigma))/r : F_{\sigma}\cap U\cap\lbrace r<d_{k}\le r+\Delta\rbrace \ne\varnothing\rbrace .$$

Les R sont des rayons carrés de boules minimales de sous-ensembles de sites (rationnels sur la grille ; FULL calcule
déjà des boules minimales exactes). Deux conséquences : **(a)** le certificat échoue dès qu'un point **critique de
d_k de n'importe quel indice** (pas seulement un événement H0) ou presque critique se trouve dans la coquille : les
marges en r doivent entourer toutes les valeurs critiques de la composante, y compris celles qui remplissent un trou ;
**(b)** FULL seul ne peut pas certifier κ (il ne publie que les naissances et fusions) : il faut les cellules de la
mosaïque au bord de la composante, en aval et borné (invariant d'architecture respecté). Le certificat est suffisant,
pas nécessaire ; sa valeur sur LiDAR n'est **pas mesurée** ici.

### 3.3 Les représentants : A_k exact, réduit, ombre

**Proposition G4 (prouvé).** Si les complexes actifs étiquetés de P et P' au rayon r coïncident (mêmes cellules
avec les mêmes labels Q), alors d_H(|A_{k,v}^P(r)|, |A_{k,v}^{P'}(r)|) ≤ δ, de même pour l'ombre S_v(r) et pour une
réduction à sommets protégés qui utilise le même appariement. Sinon, la seule borne générale (sommets protégés [A5]
et G3) est d_H(L^P, L^{P'}) ≤ 2r + κδ, sans intérêt à l'échelle r : pas de borne petite sans marge combinatoire.

*Preuve.* Un barycentre c_Q bouge d'au plus δ ; un point d'une cellule est une combinaison convexe fixe de ses
sommets ; conv(∪Q) bouge d'au plus δ en Hausdorff. ∎

**Proposition G5 (les dates de cellules ne sont pas lipschitziennes ; prouvé).** k = 1, A = (−1, 0, 0), B = (1, 0, 0),
C = (0, h, 0), 0 < h < 1. L'arête AB a pour face duale la demi-droite {(0, y) : y ≤ (h²−1)/(2h)}, donc sa date est
le rayon carré circonscrit 1 + ((1−h²)/(2h))², d'ordre 1/(4h²) : déplacer C de δ change sa date d'environ δ/(2h³) en
rayon carré. Les dates des **événements topologiques** sont 1-lipschitziennes (R0), celles des **cellules** ne le
sont pas. Mais AB entre avec ABC à la même date : (AB, ABC) est une paire libre de même naissance, que
l'effondrement certifié à sommets protégés retire [A5]. C'est le mécanisme du saut [A4]. Pour le triangle de [A4]
lui-même (k = 1, A = (−4,0), B = (4,0), C = (1,2)), le centre circonscrit (0, −11/4) a Σ = {A, B, C}, de boule
minimale le disque de diamètre AB (R² = 16), et d_1² = 377/16 : τ² = 256/377 < 1 (κ = √377/11 ≈ 1,77). Le saut du
dessin a lieu à une **valeur régulière** de d_1 : la région ne change pas de topologie, seul le représentant saute.

**Pour K = 1, il existe une réduction dont la géométrie ne change qu'aux valeurs critiques** : le complexe Wrap de
Bauer–Edelsbrunner (théorème 5.10, relu : Čech_r ↘ DelČech_r ↘ Del_r ↘ Wrap_r pour tout r, en position générale),
Wrap_r = ↓Sing_r (éq. 26 : ensemble inférieur des intervalles singuliers de rayon carré ≤ r²) ; Wrap_r est donc
constant entre deux valeurs critiques. Pour k > 1, l'auditeur a écarté le transfert par analogie (pas de fonction de Morse discrète généralisée
en général). **Question ouverte centrale** (§ 8) : existe-t-il, à l'ordre k, une réduction filtrée certifiée dont le
dessin ne change qu'aux valeurs critiques de d_k ? Ce serait l'analogue géométrique de N1.

### 3.4 Coupes régulières

**Définition.** Pour un nœud v et une tolérance δ, une coupe r est **δ-régulière** si (i) r ∈ [b_v + δ, d_v − 2δ)
(correspondance garantie par N1), (ii) le certificat κ_v tient sur [r, r+2δ] (G3), (iii) aucune cellule de la
composante (du complexe affiché, réduit ou non) n'a un rayon de date √a_σ dans la bande [r − L_σ δ, r + L_σ δ], où
L_σ borne la variation de ce rayon sous déplacement δ, et la combinatoire étiquetée est localement constante
(marge combinatoire, G4). La **marge géométrique** d'une coupe est le plus grand δ pour lequel
elle est δ-régulière. La coupe de fin de vie A_v(d_v^−) n'est jamais régulière (elle touche d_v) : la dernière coupe
à correspondance garantie est A_v((d_v − 2δ)^−).

Sur grille, (iii) est souvent vide : les dégénérescences (cosphéricités) annulent des prédicats, la combinatoire
change sous perturbation arbitrairement petite. La subdivision régulière à cellules polytopes ([A] § 1.1) est le
bon objet : une perturbation la raffine, et le dessin reste proche si les dates des sous-cellules sont du même côté
de r. À K = 5 (10³ faces par point, vie médiane < 1 mm), la marge géométrique d'une coupe fixe sera très petite :
**la stabilité du dessin à rayon fixé est l'exception, celle de la tour la règle.**

## 4. L'axe des ordres et r ensemble

**Bifiltration contre tranche.** R0, N4, P1 et S1 disent la même chose : les perturbations de population, de masse
ou d'échantillonnage ne sont absorbées que par un **décalage d'ordre**. C'est le contenu de la stabilité de la
multicouverture de Blumberg–Lesnick (Prokhorov, toutes les (r, k) à la fois) et de l'instabilité des tranches à un
paramètre de Rolle–Scoccola (JMLR 2024, § 3.3 ; leur λ-liaison, stable, lit la bifiltration le long d'une courbe).
FULL calcule la tour entière K = 1..Kmax : il possède l'objet stable ; une tranche K fixée ne l'est que pour le
déplacement apparié.

**Conséquence pour les niveaux d'un nœud.** Un nœud HGP vit à un ordre ; sa robustesse se lit dans le plan (r, k) :
(i) sa **vie** (b_v, d_v) à k fixé (robustesse au déplacement, N1) ; (ii) sa **profondeur d'ordre** (robustesse aux
aberrants et à l'échantillonnage, N4) ; (iii) les applications verticales de la tour transportent son identité vers
les ordres inférieurs ([A] § 5). Le chemin emboîtant de Zoltan (r croissant, K décroissant) est exactement la
direction dans laquelle une perturbation (δ; d, e) est absorbée. **Proposition R1 (prouvé).** Soit γ(s) = (r(s), k(s))
une courbe monotone (r croissant, k décroissant) telle que r(s+c) ≥ r(s) + δ et k(s+c) ≤ k(s) − max(d, e) ; alors les
filtrations à un paramètre F_P(s) = Ω^P_{k(s)}(r(s)) et F_{P'}(s) sont c-entrelacées (F_P(s) ⊆ Ω^{P'}_{k(s)−d}(r(s)+δ)
⊆ F_{P'}(s+c), et symétriquement), et N1 s'applique à leurs arbres avec δ remplacé par c. Une lecture de la tour le long
d'un chemin emboîtant est donc stable sous les perturbations de population, au contraire d'une tranche K fixée ;
son utilité pour la reconnaissance n'est pas mesurée.

**La distance à la mesure est la moyenne de la bifiltration le long de l'axe des ordres.** Avec la convention de BCY
(déf. 10.12), δ_{μ,u}(x) = inf{r : μ(B̄(x, r)) > u} est la fonction de rang des tranches, et

$$f_{k}(x)^{2}=\frac{n}{k}\int_{0}^{k/n}\delta_{\mu,u}(x)^{2}\,du=\frac{1}{k}\sum_{i=1}^{k}d_{i}(x)^{2}.$$

La stabilité Wasserstein de BCY (th. 10.16, ‖d_{μ,m} − d_{ν,m}‖∞ ≤ W_2/√m) vient de cette **moyenne** : une tranche
seule (δ_{μ,u}) n'est pas W_2-stable. La DTM achète la stabilité à paramètre fixé en mélangeant les ordres.

**Proposition D1 (prouvé).** Pour 1 ≤ j ≤ k : d_j ≤ √(k/(k−j+1)) f_k, d'où

$$\Omega_{k}(r)\subseteq\lbrace f_{k}\le r\rbrace \subseteq\bigcap_{j=1}^{k}\Omega_{j}\left(\sqrt{k/(k-j+1)}\,r\right).$$

*Preuve.* f_k² = (1/k) Σ_{i ≤ k} d_i² ≥ ((k−j+1)/k) d_j² car d_i ≥ d_j pour i ≥ j ; et d_i ≤ d_k. ∎

Le sous-niveau DTM est donc coincé dans un **escalier** de la bifiltration (j = k donne le facteur √k de l'auditeur,
j = ⌈k/2⌉ un facteur ≈ √2 à l'ordre moitié). Ce n'est pas une tranche : il n'a pas l'identité HGP en (k, r)
([A] : fusion plus précoce sur {0, 1, 2, 11}). D'où le statut retenu : **attribut** (couleur, priorité d'appariement,
indicateur d'erreur), stable sous W_2, jamais filtration de décision.

## 5. Définition opérationnelle : représentation robuste des niveaux

On fixe des tolérances déclarées : δ (déplacement, au moins √3/2 mm sur la grille de 1 mm), m (profondeur d'ordre),
θ (budget géométrique relatif), κ_max.

**Définition.** Une représentation des niveaux est une famille L_v(r) de complexes polyédriques étiquetés, pour les
nœuds v publiés et les coupes r ∈ J_v ⊆ [b_v, d_v). Elle est **(δ, m)-robuste** si :

- **(RR1) Topologie certifiée sur toute la filtration.** L_v(r) ≃ C_v(r) avec des équivalences naturelles en r, par
  un certificat vérifiable (A_k exact, ou effondrements certifiés sur toute la plage [A5]). *Vérification* :
  vérificateur indépendant des paires et de la fermeture.
- **(RR2) Hiérarchie et couverture.** Inclusions L_v(r) ⊆ L_w(r') le long de l'arbre et compatibilité avec T_P (π0
  et applications verticales) ; labels et dates conservés ; couverture exacte P ∩ (C_v(r) ⊕ B_r) = ∪ labels.
  *Vérification* : égalité des partitions π0 avec FULL à chaque coupe publiée ; égalité des ensembles couverts.
- **(RR3) Identité robuste.** Les identités publiées sont celles de l'arbre δ-contracté (N3) ; chaque chaîne
  publiée a une vie > 2δ et, si la robustesse aux aberrants est revendiquée, une profondeur d'ordre ≥ m (N4).
  *Mesures* : vie, profondeur d'ordre, part des nœuds éphémères (§ 2.2).
- **(RR4) Géométrie mesurée.** Pour chaque coupe publiée : d_H(L_v(r), C_v(r)) ≤ r (garanti à sommets protégés),
  d_H(L_v(r), A_{k,v}(r)) ≤ D_v(r) ≤ θ r (mesuré), trous et ouvertures (b1, b2) égaux à ceux de A ; si la stabilité
  du dessin est revendiquée, la coupe est δ-régulière (§ 3.4) avec κ_v ≤ κ_max (G3). Sinon, elle est déclarée
  « géométrie non certifiée » sans que les autres garanties tombent.
- **(RR5) Coût et taille.** Faces par point et par nœud mesurées à n = 8 000, 16 000, 32 000 ; calcul local depuis
  FULL sans mosaïque globale ; prédicats exacts, sans simulation de simplicité.

**Les vues des niveaux** que l'on peut publier sans mentir : la famille en événements ([A7]) ; une coupe intérieure
r* ∈ [b_v + δ, d_v − 2δ) de marge géométrique maximale (jeton de forme) ; la coupe de fin de vie robuste
A_v((d_v − 2δ)^−) (couverture maximale à identité garantie) ; la coupe A_v(d_v^−) seulement comme vue de couverture
non robuste.

**Quels candidats satisfont quoi.**

| Candidat | RR1 | RR2 | RR3 | RR4 | RR5 |
| --- | --- | --- | --- | --- | --- |
| A_k exact (mosaïque, dates a_σ) | oui | oui | oui (sur l'arbre contracté) | d_H ≤ r ; stabilité seulement aux coupes régulières, rares sur grille | non : ~10³ faces par point à k = 5 (workflow précédent) |
| A_k réduit, sommets protégés, certificat global | oui | oui (labels gardés) | oui | d_H ≤ r et ≤ D_v ; retire les sauts de paires libres (G5), sans garantie générale | à mesurer ; au moins le nombre de sommets de la mosaïque (tous gardés) |
| Ombre S_v(r) | non (contacts sans fusion [A6]) | couverture exacte ; identité par labels | oui si identités FULL | S ⊆ C ⊕ B_r, d_H ≤ r ; bouge ≤ δ à labels constants (G4) | à mesurer |
| Offset C_v(r) ⊕ B_r par pièces convexes | non (l'offset ferme trous et contacts) | couverture = sa trace | oui | d_H = r ; pour k = 1 stable ≤ δ (G0) ; pour k ≥ 2 hérite de κ | lourd (pièces C_Q) |
| Approximations DTM (barycentres témoins, sites pondérés) | à décalage près | non à (k, r) | non | stables en W_2 (D1, BCY 10.16) | petites |
| Multicouverture creuse (Alonso 2025) | à rayon décalé | non à (k, r) | non | — | O(n) en théorie |
| Boules critiques FULL | non ([A] § 6) | partielle | oui | ⊆ C ⊕ B_r | petites |

**Recommandation.** A_k réduit à sommets protégés, sur l'arbre δ-contracté, avec RR1–RR3 obligatoires et RR4 mesuré
(certificat κ seulement pour les coupes que l'on veut déclarer géométriquement stables) ; ombre comme vue de
couverture ; DTM comme attribut.

## 6. Exemples exacts

### 6.1 Les six points du § 6.1 de la thèse (deux triangles équilatéraux), dans Q(√3)

A = (−√3, 1), B = (−√3, −1), C = (0, 0), D = (2, 0), E = (2+√3, 1), F = (2+√3, −1) (côté 2, distance CD = 2, r = 1
dans la notation de la thèse), plongés dans ℝ³. Arbres exacts (rayons carrés), `resultats_six.json` :

- **k = 1** : six feuilles fusionnent toutes en r² = 1 (la liaison simple de la figure 6.1b).
- **k = 2** : sept lentilles nées en r² = 1 ; deux fusions triples en r² = 4/3 (cercles circonscrits des triangles) ;
  fusion triple finale en r² = 2 + √3 = AD²/4 (les quatre contacts AD, BD, CE, CF). C'est exactement la hiérarchie
  des figures 6.2–6.4 de la thèse.
- **k = 3** : deux naissances en 4/3 (les triangles), quatre en 2 + √3, deux fusions triples en r² = 4, racine en
  r² = 4 + 2√3 = (1 + √3)².

Robustesse (k = 2, six perturbations rationnelles de déplacement ≤ 1/100, √3 remplacé par 1732/1000 ou 17321/10000) :
11 nœuds au lieu de 10, un seul nœud de vie ≤ 2δ (la fusion finale se scinde en deux fusions binaires), N1 vérifié
(84 contrôles, aucune violation). Les fusions triples internes aux triangles **ne** se scindent **pas** (remarque de
§ 2.2). La configuration exacte n'est pas réalisable sur une grille entière (√3 irrationnel) : sur la grille de 1 mm,
la fusion finale est toujours scindée, et l'identité robuste est celle de l'arbre contracté.

### 6.2 Un anneau, avec et sans aberrant

Douze sites rationnels du cercle unité ((±1, 0), (0, ±1), (±3/5, ±4/5), (±4/5, ±3/5), plongés dans ℝ³), et un
aberrant au centre. Pour un nuage plan, Ω_k(r) ⊂ ℝ³ se rétracte sur sa trace plane (projeter sur le plan diminue
toutes les distances), donc le H1 calculé est celui de ℝ³. H1 par le nerf des régions témoins W_Q (dates exactes,
persistance gudhi sur les flottants de ces dates), `resultats_anneau.json` :

| | k = 1 | k = 2 |
| --- | --- | --- |
| sans aberrant | trou [1/10, 1) en r² | trou [9/25, 1) |
| avec aberrant central | trou [1/10, 5/18) et onze petits trous | trou [5/18, 1) et douze petits trous |

Un seul aberrant détruit l'essentiel de la persistance du trou à k = 1 (mort de r = 1 à r ≈ 0,527, cercle
circonscrit de (0, p_i, p_{i+1})) et **ne change pas sa mort à k = 2** (d_2(0) = 1 dans les deux cas). C'est N4 / R0
avec e = 1 : Ω_2^P ⊆ Ω_2^{P∪O} ⊆ Ω_1^P. Pour un objet annulaire (une roue), la robustesse aux aberrants du trou se
lit en montant d'un ordre, jamais à k = 1.

### 6.3 Fixtures minimales

| Nom | Données exactes | Énoncé |
| --- | --- | --- |
| `coupure` | P = {(−1,0,0), (1,0,0), (0,1,0), (0,11/10,0)}, P' : (0,1,0) → (0,101/100,0), k = 2 | N2 : racine de P (née 221/400) coupée dans P' en 408080401/408040000 par une lentille éphémère née en 1 |
| `triple` | P = {0, 2, 4}, P' = {0, 2, 4+δ}, k = 1, δ = 1/10 et 1/100 | fusion triple dégénérée scindée ; nœud intermédiaire de vie δ/2 |
| `six_points` | § 6.1, Q(√3) | arbres k = 1, 2, 3 ; fusions triples génériques contre dégénérées |
| `anneau` | § 6.2 | H1 : aberrant central, k = 1 contre k = 2 |
| `date_non_lipschitz` | k = 1, (−1,0,0), (1,0,0), (0,h,0), 0 < h < 1 | G5 : date de AB d'ordre 1/(4h²) |
| `selle_pente_forte` | k = 1, deux sites, milieu | la pente forte vaut 1 à une selle : seule la norme de Clarke (G1) détecte les fusions |

Campagne N1 (vérifiée bornée) : 2 400 couples (P, P') aléatoires entiers en dimension 1, 2, 3, n = 4 à 7,
k = 1 à 3, δ ∈ {1/8, 1/4, 1/2}, plages étroites (beaucoup d'ex æquo) : 19 715 contrôles de N1 (b, c), 559 730
inclusions Γ_K^P(r) ⊆ Γ_K^{P'}(r+δ), **aucune violation**, 21 coupures rencontrées (`resultats_campagne.json`).
Contrôle indépendant de l'outil d'arbres (`controle_grille.py`, flottant, indicatif) : nombre de composantes de
Ω_k(r) par étiquetage d'une grille plane fine, au milieu des intervalles entre événements, contre le nombre de nœuds
vivants des arbres exacts (six points k = 2, `coupure` P et P', anneau k = 1, 2 avec et sans aberrant) : 22 accords
sur 22.

## 7. Ce qui reste vrai sur la grille, et ce qui ne l'est pas

- R0, N1, N4, Q1, S1, P1, D1, G1, G2, G3 : aucune position générale supposée ; les ex æquo de la grille sont permis.
  N1 est d'ailleurs **motivé** par la grille : les naissances au contact (dégénérées) et les fusions simultanées
  sont fréquentes, et une perturbation sous-millimétrique les transforme en nœuds éphémères.
- G4 et la marge combinatoire supposent une combinatoire localement constante : faux aux dégénérescences ; utiliser
  la subdivision régulière à cellules polytopes et ne revendiquer la stabilité du dessin qu'aux coupes régulières.
- Le théorème 5.10 de Bauer–Edelsbrunner (Wrap, k = 1) suppose la position générale.

## 8. Questions ouvertes

1. **Réduction à événements critiques.** Existe-t-il, à l'ordre k, une réduction filtrée certifiée dont le dessin ne
   change qu'aux valeurs critiques de d_k (analogue du Wrap de k = 1) ? Sinon, quelle borne sur le nombre de sauts
   géométriques d'une réduction à sommets protégés par unité de rayon ?
2. **κ sur LiDAR.** Distribution du certificat τ (G2) sur les composantes réelles à K = 5 et 10 ; part des coupes
   δ-régulières pour δ = √3/2 mm. Non mesuré.
3. **Arbre δ-contracté dans FULL.** Contraction exacte publiée par le moteur (seuil déclaré), et sa compatibilité avec
   la sélection et la condensation du § 9.1 de la thèse (masses, votes).
4. **Multiplicités.** Coût d'un prédicat « k-ième voisin avec multiplicité » dans le moteur ; mesure de e(ρ) sur les
   trames (fusions publiées par le contrat d'entrée).
5. **Lecture le long d'une courbe de la tour.** Une lecture de type λ-liaison (r croissant, K décroissant) de la
   tour FULL donne-t-elle une hiérarchie de polyèdres plus stable et aussi reconnaissable ? Prédiction à écrire avant
   mesure.
6. **Sharpness de N1.** La borne 2δ sur la vie des branches de coupure est-elle atteinte ? (Atteinte qualitative
   seulement ici.)

## 9. Fichiers et rejeu

Dossier : `/workspaces/E-HGP/build/v11-persist/polyedres_ordre_k/theorie_robustesse/`.

- `exact_outils.py` : corps Q(√3), boule minimale exacte, arbre HGP par Γ_K, vérificateurs N1 et injectivité.
- `exemples.py` : `six`, `coupure`, `triple`, `campagne`, `anneau` ; sortie non nulle si un contrôle échoue ; aucune
  vérification par `assert`.
- `mesure_vies.py` : vies des nœuds des sorties `supports` en cache (lecture seule).
- `controle_grille.py` : contrôle flottant indépendant des arbres exacts (22 points de comparaison).
- `mesure_fusions.py` : fusions de retours par l'arrondi au mm sur les trames brutes (lecture seule).
- `resultats_*.json` : résultats (petits).

```sh
PY=/workspaces/E-HGP/build/v11-persist/videos/venv/bin/python
cd /workspaces/E-HGP/build/v11-persist/polyedres_ordre_k/theorie_robustesse
nice -n 10 $PY -B exemples.py tout        # environ 90 s, dont 75 s de campagne
nice -n 10 $PY -B mesure_vies.py          # lecture du cache du harnais
nice -n 10 $PY -B controle_grille.py      # environ 5 s
```

Références relues dans ce workflow : les deux réponses de l'auditeur et leurs capsules ; Bauer–Edelsbrunner 2017
(théorème 5.10, texte local de l'essai 1) ; la thèse, § 6.1 (texte local) ; le résumé de Rolle–Scoccola et de
Blumberg–Lesnick dans `receipts/hm_followup_20261003/source/workflow/axiomes_v10.md`. Citées de mémoire, à relire
avant usage contractuel : Clarke 1983 (calcul du sous-différentiel), Chang 1981 (lemme de déformation non lisse),
Lieutier 2004 et Chazal–Cohen-Steiner–Lieutier 2009 (gradient de la distance), Gibbs–Su 2002 (π² ≤ W_1),
Cohen-Steiner–Edelsbrunner–Harer 2007 (stabilité des diagrammes), théorie VC des ε-approximations.
