# De FULL à une hiérarchie laminaire de points : axiomes, impossibilités, constructions canoniques

30 septembre 2026. Approche « axiomes et impossibilité » du verrou posé par l'utilisateur :
comment passer de la tour FULL complète à la hiérarchie laminaire de points la plus pertinente.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference (oracle Gamma exhaustif borne + exports natifs en lecture seule)
profile=quantized_u18_input_only
mode=audit_math_axiomes
public_status=not_claimed
GCP non utilise. Aucun moteur, aucun fichier du worktree partage modifie.
Worktree lu : build/v9-open-worktree a 408d1ffe4 (auditeurs actifs).
```

Tout est au rayon `r`. Le moteur publie des niveaux `β = r²` ; les fixtures donnent les deux.

## 0. Réponse courte

1. **Il n'existe pas de hiérarchie « la plus pertinente » dans l'absolu.** Sept exigences naturelles
   (non-percolation, fidélité à la définition 8, respect du cœur, neutralité, canonicité, stabilité,
   verticalité) sont prouvées incompatibles par petits groupes. Chaque incompatibilité a une fixture
   exacte, recoupée par un oracle exhaustif, la référence du dépôt et le moteur natif ; les familles
   de grand K le sont par un moteur aligné contre-vérifié. La minimalité est prouvée, sauf au
   théorème C.
2. **Trilemme (théorème B).** Laminarité + non-percolation + fidélité non ambiguë impliquent une
   discontinuité. Trois sites suffisent : {0, L−1, 2L} contre {0, L+1, 2L}. Un déplacement de 2
   change une hauteur de réunion de (L+1)/2. Chaque paire du trilemme est réalisée : core, percolation,
   unanimité.
3. **Cœur contre couverture (théorème C).** Pour tout K ≥ 3, aucune hiérarchie laminaire non
   percolante ne garde ensemble à la fois les points couverts sans ambiguïté par une composante
   et les points cœur d'une composante. Famille explicite à 2K−1 sites alignés, vérifiée jusqu'à
   K = 15. À K = 2, les deux sont compatibles (règle UC, preuve).
4. **Axe K (théorème D).** Pour tout K ≥ 2, la fidélité à deux ordres consécutifs et l'emboîtement
   vertical P_{K+1} ⪯ P_K sont incompatibles. Famille à 2K sites, vérifiée jusqu'à K = 12.
   Ordres (1, 2) compatibles.
5. **Majorités à masse fixe (théorème E).** Pour tout poids positif continu, elles violent la
   fidélité non ambiguë sur un triangle rectangle de trois sites. Elles dépendent aussi de l'univers
   de témoins et sont discontinues. Elles ne figurent dans aucun ensemble maximal.
6. **Caractérisation (théorème F).** L'unanimité U1, `t1(x) = min { r ≥ α_K(x) : |Cov_x(r)| = 1 }`,
   est la règle ancrée la plus fine parmi les règles fidèles, et la plus grossière parmi les règles
   neutres. C'est l'unique règle à la fois fidèle et neutre. C'est l'ancrage au LCA de toutes les
   composantes qui couvrent le point jusqu'à sa date d'attache.
7. **Réponse à la question du LCA.** Oui, l'ancrage au LCA à déferrement minimal est l'unique règle
   laminaire, non percolante et à entrée minimale, si « canonique » signifie neutralité N1. Non, si
   « canonique » signifie seulement équivariance : la règle équivariante la plus précoce attache à
   α_K(x) sauf aux symétries exactes (proposition G). A5 (LCA des seules couvertures à α) est
   canonique, plus précoce qu'U1, et non neutre.
8. **En position générique, la question disparaît.** Si aucune première couverture n'est ex æquo
   entre deux composantes, la seule règle ancrée fidèle est la première couverture : U1 = A5 = A1.
   Les choix ne portent que sur les ex æquo exacts. Les quasi-ex-æquo relèvent du trilemme.
9. **Recommandation (cible utilisateur : un K fixé).** Publier U1 comme projection canonique de la
   définition 8, core comme contrôle stable, et le certificat d'ambiguïté par point. À K ≥ 3,
   déclarer si l'on respecte les couvertures (U1) ou les cœurs (core). Ne pas porter la majorité.

## 1. Objet et notations

- X : n sites distincts de R³. K ≥ 1 fixé. `L_K(r) = { y : |X ∩ B̄(y, r)| ≥ K }`.
- Composantes `π0(L_K(r))`, inclusions `ι_{r,s}` pour r ≤ s. FULL_K est leur arbre de fusion.
- Amas discret (définition 8, convention fermée) : `D_r(C) = { x ∈ X : dist(x, C) ≤ r }`.
- Profil de couverture : `Cov_x(r) = { C ∈ π0(L_K(r)) : x ∈ D_r(C) }`.
- Première couverture : `α(x) = min { r : Cov_x(r) ≠ ∅ }`. Cœur : x ∈ L_K(r) ⇔ r ≥ d_K(x).
  On note `c_r(x)` la composante qui contient x.
- Date d'unanimité : `t1(x) = min { r ≥ α(x) : |Cov_x(r)| = 1 }`.

**Lemme 1 (monotonie de la couverture).** Si C ∈ Cov_x(r) et r ≤ s, alors ι_{r,s}(C) ∈ Cov_x(s).
*Preuve.* C ⊆ ι(C), donc dist(x, ι(C)) ≤ dist(x, C) ≤ r ≤ s. ∎

**Lemme 2 (calcul exact, rappel).** x ∈ D_r(C) si et seulement s'il existe une K-partie F ∋ x de
rayon MEB ≤ r dont le sommet de Γ_K est dans C (théorème 2 de la thèse ; lemme de couverture des
auditeurs pour les témoins critiques p + q_min ≤ K). Tout est donc constant par morceaux, fermé à
gauche, sur les niveaux critiques. Les minimums qui définissent α et t1 sont atteints ; t1 existe
car la racine est unique.

**Sortie.** Une hiérarchie de points est une famille P(r) de partitions de X, constante par
morceaux et continue à droite. Hauteur de réunion : `u(x, y) = min { r : x ~ y dans P(r) }`.

**Règle ancrée.** Chaque x reçoit (t(x), v(x)), avec v(x) ∈ π0(L_K(t(x))). Étiquette
`σ_r(x) = ι_{t(x), r}(v(x))` pour r ≥ t(x). Blocs : fibres de σ_r ; points non entrés en singletons.

## 2. Axiomes

Chaque axiome a une forme non ancrée (partitions seules) et, si utile, une forme ancrée (T).
Les impossibilités utilisent les formes non ancrées, les plus faibles.

| Code | Nom | Énoncé exact |
| --- | --- | --- |
| L | laminarité | r ≤ s ⇒ P(r) raffine P(s) |
| T | ancrage dans FULL_K | P provient d'une règle ancrée |
| NP | non-percolation | tout bloc B de P(r) avec \|B\| ≥ 2 est inclus dans un seul D_r(C) ; ancrée : v(x) ∈ Cov_x(t(x)) |
| E∞ | fidélité totale | chaque D_r(C) est inclus dans un bloc de P(r) |
| E_u | fidélité non ambiguë | pour tout C, `U_r(C) = { x : Cov_x(r) = {C} }` est inclus dans un bloc ; ancrée : Cov_x(r) = {C} ⇒ σ_r(x) = C |
| E+ | entrée immédiate | ancrée : t(x) = α(x) ; non ancrée : en α(x), le bloc de x contient un autre point d'un amas qui couvre x |
| CR | respect du cœur | pour tout C, C ∩ X est inclus dans un bloc ; ancrée : x ∈ C ⇒ σ_r(x) = C |
| N1 | neutralité | ancrée : Cov_x(t(x)) = {v(x)} ; on ne tranche jamais entre deux composantes qui couvrent x à la même date |
| Can | canonicité | P_{gX}(r) = g(P_X(r)) pour toute isométrie g ; aucun départage par identifiant ni par rang de Morton |
| TI | intrinsèque | P est une fonction équivariante de D = (FULL_K avec niveaux exacts, relations de cœur et de couverture) : deux nuages de données isomorphes ont des hiérarchies images l'une de l'autre ; l'univers de témoins qui réalise D n'intervient pas |
| Stab | stabilité | ∃ c : si \|x_i − y_i\| ≤ ε pour tout i, alors \|u_X(i, j) − u_Y(i, j)\| ≤ c ε ; C0 : continuité seule |
| V | verticalité | P_{K+1}(r) raffine P_K(r) pour tout r |
| Loc | coût | calcul par FULL, incidences des témoins forts et requêtes d'ancêtre ou de LCA ; aucune K-partie énumérée |

Implications immédiates : T ⇒ L ; NP ancrée ⇒ NP ; E+ et NP ancrée ⇒ E_u ancrée ⇒ E_u ;
E∞ ⇒ E_u et E∞ ⇒ CR ; N1 ⇒ NP ancrée.

## 3. Constructions examinées

| Nom | Définition |
| --- | --- |
| core (A0) | t = d_K(x), v = c_{d_K}(x) : blocs C ∩ X |
| percolation | fermeture transitive des amas discrets qui se recouvrent (non ancrée) |
| A1 natif | t = α(x), composante de la première boule couvrante ; ex æquo par l'ordre (niveau, S*) en rangs de Morton |
| A5 | à α : une seule couvrante ⇒ attache ; sinon LCA de Cov_x(α), à sa naissance |
| bande η | nœuds minimaux des composantes couvrant x sur [α, (1+η)α], puis LCA ; η = 0 donne A5 ; c'est la variante « antichaîne » de l'auditeur |
| U1 (unanimité) | t = t1(x), v = l'unique élément de Cov_x(t1) |
| UC | t = min(t1, d_K), composante unique ou composante cœur |
| première couverture canonique | à α, départage par un invariant d'isométrie ; ex æquo ⇒ LCA des minimaux |
| majorités A3/A4 | majorité stricte à masse fixe, poids 1 ou 1/β, univers fort p + q_min ≤ K |
| argmax par coupe | vote du § 9.1 recalculé à chaque coupe |

## 4. Résultats structurels (prouvés)

**Proposition 1 (ancrage).** (a) T ⇒ L. (b) Une règle ancrée vérifie NP ancrée si et seulement si
v(x) ∈ Cov_x(t(x)) pour tout x. (c) Les règles ancrées NP se découplent : chaque point choisit
indépendamment un couple (t, C) avec C ∈ Cov_x(t).
*Preuve.* (a) Deux points de même étiquette en r ont l'étiquette ι(C) en s ≥ r ; un point non entré
ne quitte pas de bloc. (b) Par le lemme 1, σ_r(x) ∈ Cov_x(r) pour tout r ≥ t(x) ; chaque bloc est
alors inclus dans l'amas de son étiquette. (c) Ni L ni NP ancrée ne relient deux points. ∎

Les axiomes qui couplent les points sont les formes non ancrées de E_u, CR, V et E∞.

**Lemme 3 (entrelacement du profil de couverture).** Soient X, Y appariés, |x_i − y_i| ≤ ε.
(i) L_K^X(r) ⊆ L_K^Y(r + ε). (ii) Si φ(C) est la composante de L_K^Y(r + ε) qui contient C,
alors φ(Cov^X_{x_i}(r)) ⊆ Cov^Y_{y_i}(r + ε). (iii) Avec ψ symétrique, ψ ∘ φ = ι^X_{r, r+2ε}.
*Preuve.* (i) Les K témoins d'un centre bougent d'au plus ε. (ii) dist(y_i, φ(C)) ≤ dist(y_i, C) ≤
ε + dist(x_i, C) ≤ r + ε. (iii) C ⊆ φ(C) ⊆ ψ(φ(C)), composante de L_K^X(r + 2ε) qui contient C. ∎

**Proposition 4 (stabilités prouvées, en rayon).**

| Quantité | Constante | Preuve |
| --- | ---: | --- |
| première couverture α | 1 | lemme 3 (ii) : y_i est couvert en α(x_i) + ε |
| distance cœur d_K | 2 | chaque distance entre sites bouge d'au plus 2ε |
| hauteurs core | 2 | preuve de l'audit continu, § 2.2 ; F4 atteint 2 |
| hauteurs percolation | 1 | lemme 3 (ii) : chaque amas de X s'envoie dans un amas de Y en r + ε ; les chaînes se transportent |

Un contrôle d'échantillon dev (355 entrées, 688 paires, dimensions 1 à 3, K ≤ 3) ne trouve aucune
violation, en comparaison exacte de racines carrées.

## 5. Théorèmes d'impossibilité

### Théorème A — symétrie (fixture F1)

**Énoncé.** (a) NP + Can + E+ sont incompatibles. (b) NP + E∞ sont incompatibles.
**Fixture.** Sites alignés {0, 2, 4}, K = 2, r = 1 (β = 1). Deux composantes, de centres 1 et 3,
d'amas {0, 2} et {2, 4}. La réflexion x ↦ 4 − x fixe X et le site 2, et échange les composantes.
**Preuve.** (a) Par Can, P(1) est invariant par la réflexion. Le bloc de 2 contient 0 ou 4 (E+),
donc les deux. {0, 2, 4} n'est dans aucun amas : NP échoue. (b) E∞ impose {0, 2} et {2, 4} dans
un bloc, donc {0, 2, 4}. ∎
**Contrôle exhaustif.** Aucune des 5 partitions de trois points ne vérifie les trois axiomes.
Chaque paire est réalisable. **Minimalité.** À deux sites, chaque point a au plus une composante
couvrante (K = 1 : deux boules qui se touchent fusionnent ; K = 2 : une lentille convexe).
**Conséquence pour le moteur.** A1 natif publie {0, 2} | {4} sur un nuage invariant par la
réflexion : il viole Can. Son départage par rangs de Morton est invariant par renommage, pas par
isométrie.

### Théorème B — trilemme de la frontière

**Énoncé.** L + NP + E_u impliquent la discontinuité : aucune règle ne vérifie aussi C0, a fortiori
Stab. Quantitativement, pour 0 < δ < L :

| Nuage | Fait forcé | Hauteur (0, m) |
| --- | --- | --- |
| X₊ = {0, L−δ, 2L} | en r = (L−δ)/2, 0 et m sont couverts par C₁ seul (E_u) | ≤ (L−δ)/2 |
| X₋ = {0, L+δ, 2L} | m et 2L couverts par C₂ seul en (L−δ)/2 ; aucun amas ne contient {0, m, 2L} avant L (L, NP) | ≥ L |

Le saut vaut au moins (L+δ)/2 pour un déplacement 2δ du seul point m.
**Preuve.** Dans X₊, E_u réunit 0 et m dès (L−δ)/2. Dans X₋, E_u réunit m et 2L dès (L−δ)/2 ;
L les garde ensemble. Un bloc contenant 0, m et 2L avant L violerait NP. ∎
Les dates d'entrée sautent aussi : pour U1, t1(m) = L en X₀ = {0, L, 2L} (deux composantes nées
ensemble en L/2), contre (L−δ)/2 en X₊ et X₋.
**Modèle continu contre grille.** Dans le modèle réel, δ → 0 donne une discontinuité en {0, L, 2L}.
Sur la grille, X₊ = {0, L−1, 2L} et X₋ = {0, L+1, 2L} donnent une amplification (L+1)/4 par unité
de déplacement ; avec L jusqu'à 2^17 en u18, un saut jusqu'à 65 536 pour un déplacement de 2.
**Fixtures exactes.** L = 1 000 : niveaux 998001/4 contre 10^6, soit rayons 499,5 et 1 000.
L = 100 000 : 9999800001/4 contre 10^10. Variante non alignée {(0,0,0), (999,1,1), (2000,0,0)}
contre (1001,1,1) : 998003/4 contre 10^6. A1, A5, U1, UC et la majorité 1/β sautent tous ;
core reste dans 2ε, la percolation dans ε.
**Minimalité.** Trois sites (voir théorème A).
**Le trilemme est serré.** Sans E_u : core (stable, constante 2). Sans NP : percolation (stable,
constante 1). Sans Stab : U1.
**Bande η.** Elle échange la discontinuité aux ex æquo contre une discontinuité au bord de bande.
Pour η = 1/8, L = 1 000, déplacer m d'une unité (δ = 58 → 59) fait passer la hauteur de 1 000 à
470,5. Elle viole aussi E_u (δ = 58 : m seul couvert par C₁ sur [471, 529)).

### Théorème C — cœur contre couverture

**Énoncé.** (i) Pour tout K ≥ 3, L + NP + E_u + CR sont incompatibles. (ii) À K = 2, la règle UC
vérifie T, NP, E_u, CR et Can (en formes ancrées). (iii) T + N1 + CR sont incompatibles : de façon
générique pour K ≥ 3, aux seuls ex æquo de plus proches voisins pour K = 2.

**Famille (i).** Paramètres `h = K`, `t = 2`, `Q = K(K−2) + 2`. Sites alignés, avant translation :
`l_j = −Q − 2(K−1−j)` pour j = 1..K−2, `q = −Q`, puis `R = {0, K, 2K, …, (K−1)K}` avec p = 0 ∈ R.
Soit 2K−1 sites. K = 3 donne {0, 2, 7, 10, 13} ; K = 4 donne {0, 2, 4, 14, 18, 22, 26}.

**Preuve de (i).** Pour des sites alignés dans R³, la projection orthogonale sur l'axe ne s'éloigne
d'aucun site ; les composantes de L_K(r) correspondent à celles de leur trace sur l'axe, et la
distance d'un site à une composante est celle à sa trace (contre-audit des fixtures, § 1). Ce sont
donc de vrais nuages 3D. Sur l'axe, les composantes sont des chaînes de K-fenêtres consécutives de
demi-étendue ≤ r, deux voisines étant reliées si la (K+1)-fenêtre l'est ; x est couvert
exactement par les composantes des K-fenêtres actives qui le contiennent (preuve dans `line1d.py`).
- r_R = (K−1)K/2. Toute autre fenêtre contenant un point de R a une étendue > (K−1)K : pour la
  fenêtre de départ l_j, l'écart vaut j(K−2) > 0 ; pour celle de départ q, (K−1)(K−2) > 0. Donc
  les points de R sont couverts par la seule composante de R en r_R (E_u : R dans un bloc).
- d_K(p) = Q : ses K−1 plus proches autres sont K, …, (K−2)K puis q, car (K−2)K < Q < (K−1)K.
  d_K(q) = Q : ses plus proches sont l_{K−2}, …, l_1 (à 2, …, 2(K−2)) puis p.
- En r = Q, les fenêtres de départ l_1, …, q sont reliées : la (K+1)-fenêtre de départ l_j a
  l'étendue jK + Q + 2(K−1−j) ≤ 2Q, avec égalité pour j = K−2. Donc p et q sont des points cœur
  d'une même composante M (CR : p ~ q).
- La composante de R n'est pas reliée à M en Q : la (K+1)-fenêtre de départ q a l'étendue
  Q + (K−1)K > 2Q. Elles sont les deux seules composantes.
- Seules des fenêtres de M contiennent q ; seule celle de R contient (K−1)K. Aucun amas ne contient
  {q, (K−1)K} en Q. Or L, E_u et CR placent q, p et R dans un même bloc : NP échoue. ∎

**Vérification.** Contradiction au niveau Q² pour K = 3 à 15 (moteur aligné), confirmée par Γ pour
K = 3 et 4. La référence du dépôt redonne les couvertures et C ∩ X à tous les niveaux pour K = 3,
4, 5 ; le moteur natif redonne le certificat pour K = 3 et 4. Fixture 3D de dimension affine 3,
K = 3 : {(4,3,3), (4,2,2), (3,4,3), (3,2,1), (2,1,2)}, contradiction au niveau 2, aussi native. Des recherches dev trouvent des témoins à K+2 sites pour K = 3 à 8
et 10 ; aucun témoin à 4 sites alignés pour K = 3 sur la grille ≤ 22 (1 540 nuages). La minimalité
n'est pas prouvée.

**Preuve de (ii), K = 2.** UC : si t1 ≤ d_2(x), attache (t1, C₁) ; sinon (d_2, c_{d_2}(x)).
Soit a un plus proche voisin de x, d = |x − a|. La lentille de {x, a} naît en d/2 et couvre x ; son
image en t1 est donc C₁. En r = d, le segment [x, m_a] est dans L_2(d) : tout w de ce segment a
|w − x| ≤ d/2 et |w − a| ≤ d. Donc ι(C₁) = c_d(x). E_u ancrée : Cov_x(r) = {C} impose r ≥ t1, et
l'étiquette appartient à Cov_x(r) par le lemme 1. CR ancrée : pour r ≥ d_2 l'étiquette est
c_r(x) dans les deux cas. ∎ Contrôle : 150 nuages dev, zéro échec.

**Preuve de (iii).** K ≥ 3 : dans la famille, la composante de R couvre p sans interruption depuis
r_R ; p n'est donc jamais couvert par la seule lignée de M avant Q. En Q, p est couvert par M et
par la composante de R. Une règle N1 laisse p non entré en Q, ou l'a attaché plus tôt à la lignée
de R. Dans les deux cas p n'est pas avec q, qui n'est pas couvert par R (NP ancrée) : CR échoue.
K = 2 : {(2,0,0), (0,0,0), (4,0,0), (2,4,0)}. Le site x = (2,0,0) a deux plus proches voisins à
distance 2. Il est couvert par au moins deux composantes pour tout rayon de [1, √5[ (niveaux
[1, 5)). En r = 2 (niveau 4), il est cœur avec eux dans la même composante. Toute règle N1 le
laisse seul : CR échoue. ∎ Sans ex æquo de plus proche voisin, t1 = α à K = 2 et U1 respecte le
cœur (même argument que (ii)).

### Théorème D — axe K

**Énoncé.** Pour tout K ≥ 2, L (aux ordres K et K+1) + NP + E_u(K) + E_u(K+1) + V sont
incompatibles. Pour les ordres (1, 2), ils sont compatibles.

**Famille.** Sites alignés `l_j = −3(K−j)` pour j = 1..K−1, p = 0, et `R = {3K + 2m : m = 0..K−1}`.
Soit 2K sites. K = 2 donne {0, 3, 9, 11} ; la fixture {0, 3, 9, 10} a la même structure.

**Preuve.** Ordre K : W_L = {l_1, …, l_{K−1}, p} naît en r_L = 3(K−1)/2. Toute autre fenêtre
contenant ses points a une étendue > 3K − 3 : 6K − j − 4 pour la fenêtre de départ l_j (j ≥ 2),
5K − 4 pour celle de départ p. Donc E_u(K) réunit W_L. Ordre K+1 :
W'_R = {p} ∪ R a l'étendue 5K − 2 ; toute autre (K+1)-fenêtre contenant p a l'étendue
6K − j − 2 > 5K − 2 ; donc E_u(K+1) réunit p et R en r₂ = (5K−2)/2. V descend ce bloc à l'ordre K.
À l'ordre K en r₂, la seule fenêtre qui contient l_1 est W_L, non reliée à sa voisine car
6K − 3 > 5K − 2 ; la seule qui contient le dernier point de R est la fenêtre R. Aucun amas ne
contient les deux : NP échoue à l'ordre K. ∎
**Ordres (1, 2).** À K = 1, chaque point n'est couvert que par sa propre composante : si y ∈ C'
est à distance ≤ r de x, la boule B̄(x, r) relie x à y. E_u(1) et NP(1) donnent exactement la
liaison simple. Les blocs NP d'ordre 2 sont dans des amas d'ordre 2, eux-mêmes inclus dans l'amas
d'ordre 1 de la composante qui les contient. U1 à l'ordre 2 et la liaison simple à l'ordre 1
conviennent. ∎
**Borne inférieure.** Il faut au moins K+2 sites : avec K+1 sites, l'ordre K+1 n'a qu'un sommet X,
et l'arête X relie toute l'ordre K au même niveau. {0, 3, 9, 10} est donc minimal pour (2, 3).
**Ce qui reste vertical.** core vérifie V (L_{K+1} ⊆ L_K et d_{K+1} ≥ d_K). La percolation aussi :
un amas d'ordre K+1 est inclus dans l'amas d'ordre K de l'image verticale. U1, A1, A5, UC et la
majorité 1/β violent V sur {0, 3, 9, 10}. Rappel F3 : la réunion libre des blocs cœur de tous les
(K, r) n'est pas un arbre.
**Vérification.** Familles K = 2 à 12 (moteur aligné) ; référence du dépôt pour les familles K = 2
et 3 ; certificat natif pour {0, 3, 9, 10} et pour la fixture 3D de dimension affine 3
{(4,0,4), (3,1,1), (0,0,3), (0,1,3)}.

### Théorème E — majorités à masse fixe

**Énoncé.** (i) Pour tout poids ω > 0 continu sur ]0, ∞[ et tout seuil θ ≥ 1/2, la majorité
stricte à masse fixe viole E_u. (ii) Elle n'est pas intrinsèque : deux univers complets pour la
couverture donnent des sorties différentes. (iii) Elle est discontinue.
**Preuve de (i).** x = (0,0,0), a = (2ρ,0,0), b = (0,2ρ',0), ρ' ↓ ρ. Les témoins forts de x sont
les paires xa, xb et la boule de diamètre ab (x sur la coquille, p = 0, q_min = 2). Sur [ρ, ρ'[,
x et a ne sont couverts que par C₁. À ρ², M = ω(ρ²) et W = ω(ρ²) + ω(ρ'²) + ω(ρ² + ρ'²). Quand
ρ' → ρ, la condition M > θ W tend vers ω(ρ²) > θ(2ω(ρ²) + ω(2ρ²)), fausse avec une marge stricte
pour θ ≥ 1/2. Elle est donc fausse pour ρ' assez proche de ρ : x est différé. ∎
**Grille.** (0,0,0), (8,0,0), (0,10,0) : en 1/β, a entre à 16, x à 41. Famille (2L, 2L+2) :
report pour tout L ≥ 4, 16, 48 avec ω = β^−1, β^−2, β^−3 (vérifié jusqu'à L = 200) ; en uniforme,
pour tout L ≥ 1. Trois sites sont minimaux (à deux sites, un seul témoin). Plus petit que les
fixtures d'audit à quatre et cinq sites.
**(ii).** Six sites alignés, K = 2 : univers fort contre admission p + q_min ≤ 5, même couverture,
majorité uniforme 1 contre 9/4 (constat de l'audit, retrouvé). Quatre sites plans
{(1,4,0), (2,4,0), (4,3,0), (4,5,0)} : la majorité 1/β change aussi.
**(iii).** Contact coquille/intérieur de l'audit, C = (0,0,0) contre (1,1,1), M = 8, 64, 2 048 :
les deux majorités sortent de la borne 2ε (ε = √3), alors qu'U1 et core y restent. Normalisé par M,
le saut de la majorité 1/β reste proche de 1,2 (rayons 3,20 M contre 2,0 M) : discontinuité dans
le modèle continu, amplification sur la grille. F2 : tout poids strictement décroissant saute.
**Poids non continus.** Des poids lexicographiques ramènent à la première couverture : le
théorème B s'applique.

## 6. Caractérisation : l'unanimité U1

### Théorème F — treillis des règles ancrées

Soit R une règle ancrée non percolante.
1. Si R vérifie E_u ancrée, alors t_R(x) ≤ t1(x) et σ^R_r(x) = σ^{U1}_r(x) pour r ≥ t1(x).
   Donc P_{U1}(r) raffine P_R(r) pour tout r.
2. Si R vérifie N1, alors t_R(x) ≥ t1(x) et σ^R_r(x) = σ^{U1}_r(x) pour r ≥ t_R(x).
   Donc P_R(r) raffine P_{U1}(r).
3. U1 vérifie T, NP, E_u, N1, Can, TI. C'est l'unique règle ancrée à la fois fidèle et neutre.

*Preuve.* (1) En t1, Cov_x(t1) = {C₁} ; E_u impose σ^R_{t1}(x) = C₁, donc t_R ≤ t1 ; ensuite les deux
règles suivent les ancêtres de C₁. Un bloc d'U1 en r est formé de points de même étiquette dans R.
(2) N1 impose |Cov_x(t_R)| = 1, donc t_R ≥ t1. L'étiquette d'U1 en t_R appartient à Cov_x(t_R)
(lemme 1) = {v_R(x)}. (3) Par construction ; l'unicité découle de (1) et (2). ∎

**U1 est l'ancrage au LCA à déferrement minimal.** En t1, toutes les composantes qui ont couvert x
avant t1 ont leur image dans Cov_x(t1) = {C₁}. C₁ est donc l'ancêtre vivant de leur LCA, et aucune
autre composante ne couvre x. Aucune règle neutre n'entre plus tôt.

**Calcul (Loc).** Par le lemme de couverture, Cov_x(r) est l'ensemble des ancêtres vivants des
nœuds des témoins forts de niveau ≤ r. On balaie les témoins de x par niveau ; J = LCA des témoins
activés ; t1 = max(ℓ_m, niveau(J)) au premier palier m où niveau(J) < ℓ_{m+1}. Coût O(D_x log H) par
point, D_x = témoins lus jusqu'à t1. C'est la même architecture que la bande, sans troncature en
(1+η)α ; D_x n'est pas borné par le lemme de voisinage de l'audit si t1 est tardif.

**Corollaire F' (généricité).** Si |Cov_x(α(x))| = 1 pour tout x, la seule règle ancrée NP qui
vérifie E_u est la première couverture ; U1 = A5 = A1. *Preuve.* t1 = α et le point 1. ∎
Contrôle : 80 couples (nuage, K) dev génériques (boîte 10^6), égalité dans tous les cas. Sur une
petite grille (côté 6), 30 points sur 444 ont une première couverture ambiguë : sur la grille du
LiDAR, la fréquence des ex æquo exacts doit être mesurée.

**Corollaire F'' (rappel avant fusion parasite).** Parmi les règles neutres, U1 maximise à chaque
rayon, pour chaque composante, la part de son amas discret déjà réunie dans son bloc. Un point
ambigu entre deux composantes ne peut entrer dans aucune d'elles avant leur fusion, pour aucune
règle neutre. Tout rappel supplémentaire provient d'une préférence déclarée entre composantes.

### Proposition G — la canonicité seule ne force pas le LCA

1. Pour toute règle Can + NP ancrée : `t_R(x) ≥ t_can(x) = min { r ≥ α(x) : Cov_x(r) contient une composante fixée par le stabilisateur de x }`.
   Le stabilisateur est { g isométrie : gX = X, gx = x } ; l'équivariance impose g v(x) = v(x).
2. t_can est atteint par une règle canonique : parmi les composantes fixées, prendre le minimum d'une
   forme canonique de (X, x, C). Deux composantes fixées distinctes ont des formes distinctes, sinon
   l'isométrie qui les échange fixerait x et X.
3. Si le stabilisateur est trivial (cas générique), t_can = α : l'entrée canonique minimale est la
   première couverture, avec un départage géométrique. Elle n'est pas unique et reste instable.
4. Fixture {10, 14, 17, 19, 22}, K = 3, x = 17 : une règle à départage par invariant d'isométrie
   entre en r = 5/2, A5 en 4, U1 en 9/2 (niveaux 25/4, 16, 81/4). A5 attache x quand deux
   composantes le couvrent : A5 n'est pas neutre. Même constat en 3D, K = 2, sur quatre sites.

A5 est la plus précoce des règles neutres vis-à-vis des seules couvertures en α. La bande η est la
plus précoce des règles neutres vis-à-vis des couvertures sur [α, (1+η)α]. U1 est la plus précoce
des règles neutres vis-à-vis de tout ce qui couvre le point à sa date d'attache.

**Remarque (lien avec le vote du § 9.1).** Pour un vote de masses actives gelé à la première décision,
le seuil θ = 1 signifie : tous les témoins actifs dans une même composante, soit |Cov_x(r)| = 1.
Le vote gelé à l'unanimité est U1, quel que soit le poids positif et l'univers complet. C'est le seul
membre de cette famille intrinsèque à FULL. Pour θ < 1, la décision dépend des poids.

### Réponse à la question posée

« L'ancrage à déferrement minimal au LCA de toutes les composantes couvrantes est-il l'unique règle
laminaire, non percolante, canonique et à entrée minimale ? »

- **Oui**, avec deux précisions. « Canonique » doit signifier la neutralité N1 : à la date d'attache,
  le propriétaire est la seule composante qui couvre le point. « Toutes les composantes couvrantes »
  désigne alors toutes celles qui couvrent le point jusqu'à cette date. « Entrée minimale » signifie
  date d'entrée ponctuellement minimale parmi ces règles, ou de façon équivalente partitions les plus
  grossières à chaque rayon. La règle est U1 (théorème F).
- **Non** si « canonique » signifie seulement équivariance : la règle canonique la plus précoce
  attache en α(x) sauf aux symétries exactes (proposition G). **Non** si l'on ne retient que les
  couvertures à α : A5 est canonique et plus précoce qu'U1, mais non neutre.
- Génériquement, U1 = A5 = A1 (corollaire F'). La question ne porte que sur les ex æquo exacts.

## 7. Tableau axiomes × constructions

✓ : prouvé. ✗ : réfuté par la fixture indiquée. — : non établi ou sans objet.

| Construction | L | T | NP | E∞ | E_u | E+ | CR | N1 | Can | TI | Stab | V | Loc |
| --- | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| core | ✓ | ✓ | ✓ | ✗ F2 | ✗ F2 | ✗ F2 | ✓ | ✗ ex æquo K2 | ✓ | ✓ | ✓ (2) | ✓ | ✓ |
| percolation | ✓ | ✗ {0,1,3,5,6} | ✗ F1 | ✓ | ✓ | ✓ | ✓ | — | ✓ | ✓ | ✓ (1) | ✓ | ✓ |
| A1 natif | ✓ | ✓ | ✓ | ✗ F1 | ✓ | ✓ | ✗ th. C (K ≥ 3) | ✗ F1 | ✗ F1 | ✗ F1 | ✗ F2 | ✗ th. D | ✓ |
| A5 | ✓ | ✓ | ✓ | ✗ F1 | ✓ | ✗ F1 | ✗ th. C (K ≥ 3) | ✗ prop. G | ✓ | ✓ | ✗ F2 | ✗ th. D | ✓ |
| bande η > 0 | ✓ | ✓ | ✓ | ✗ F1 | ✗ bande | ✗ F1 | — | — | ✓ | ✓ | ✗ bord | — | ✓ |
| U1 | ✓ | ✓ | ✓ | ✗ F1 | ✓ | ✗ F1 | ✗ th. C | ✓ | ✓ | ✓ | ✗ F2 | ✗ th. D | ✓ |
| UC | ✓ | ✓ | ✓ | ✗ F1 | ✓ | ✗ F1 | ✓ K = 2, ✗ K ≥ 3 | ✗ ex æquo K2 | ✓ | ✓ | ✗ F2 | ✗ th. D | ✓ |
| majorité uniforme | ✓ | ✓ | ✓ | ✗ F1 | ✗ th. E | ✗ th. E | — | — | ✓ | ✗ th. E | ✗ contact | — | ✓ |
| majorité 1/β | ✓ | ✓ | ✓ | ✗ F1 | ✗ th. E | ✗ th. E | — | — | ✓ | ✗ th. E | ✗ contact, F2 | ✗ fixture V | ✓ |
| argmax par coupe | ✗ F5 | — | — | — | — | — | — | — | — | — | — | — | — |

Notes. Percolation et T : sites {0, 1, 3, 5, 6}, K = 2. En r = 1/2, la percolation a les blocs
{0, 1} et {5, 6}, qui ne peuvent porter que les deux composantes alors vivantes ; en r = 1, elle
les réunit alors que ces composantes restent distinctes (fixture gravée). E∞ ✗ F1 vaut pour
toute règle NP (théorème A b). A1, A5, U1, UC vérifient E_u et L, donc violent CR à K ≥ 3 par le
théorème C. core viole N1 : sur la fixture ex æquo, il attache x en r = 2 quand deux composantes
le couvrent.

## 8. Ensembles maximaux compatibles

On retient sept exigences discriminantes {NP, E_u, CR, N1, Can, Stab, V}. L et Loc sont satisfaits
par toutes les constructions examinées sauf l'argmax par coupe ; T par toutes sauf la percolation ;
TI par toutes sauf A1 et les majorités.
Parties interdites prouvées : {NP, E_u, Stab} (B), {NP, E_u, CR} pour K ≥ 3 (C), {N1, CR} (C),
{NP, E_u, V} avec E_u à chaque ordre (D). De plus N1 ⇒ NP. Can n'entre dans aucune partie interdite
de cette liste ; elle n'interdit que E+ (A).

Énumération. Si NP et E_u : ni Stab, ni CR (K ≥ 3), ni V ; reste N1. Si NP sans E_u : CR et N1
s'excluent. Sans NP : pas de N1. D'où, à K ≥ 3 :

| Ensemble maximal (avec Can, L, TI, Loc) | Construction canonique | Caractérisation extrémale |
| --- | --- | --- |
| {NP, CR, Stab, V} | core | la plus fine des règles ancrées NP respectant le cœur |
| {NP, E_u, N1} | U1 | unique règle fidèle et neutre (théorème F) |
| {E_u, CR, Stab, V} (+ E∞) | percolation | la plus fine hiérarchie laminaire vérifiant E∞ |
| {NP, N1, Stab, V} | — | réalisé seulement par une règle dégénérée (aucune entrée) ; règle complète ouverte (§ 11) |

À K = 2, {NP, E_u, CR} devient compatible : UC en est la plus fine réalisation, et reste maximale
car N1 + CR échouent aux ex æquo. Sans Can, {NP, E_u, E+} est réalisé par A1 (départage arbitraire).
À K = 1, aucune exigence n'est en conflit : toutes les constructions sont la liaison simple.

Diagramme en K : K = 1 sans conflit ; K = 2 trilemme de la frontière ; K ≥ 3, en plus, cœur contre
couverture ; entre ordres consécutifs (K ≥ 2), fidélité contre verticalité.

## 9. Conséquences pour le projet

La cible utilisateur courante est un seul K fixé (auditeur indépendant, 30 septembre).

1. **Projection canonique de la définition 8 : U1.** Elle ne percole jamais, entre chaque point dès
   que son appartenance est non ambiguë, n'utilise ni identifiant ni rang de Morton, et diffère
   exactement les points réellement ambigus. Génériquement, elle coïncide avec A1 actuel. Coût : le
   balayage de témoins du lemme de couverture, déjà écrit pour la bande.
2. **Publier le certificat d'ambiguïté** par point : α(x), t1(x), t1 − α, |Cov_x(α)|, et la masse
   différée. C'est exactement ce qu'une règle non neutre prétend récupérer (corollaire F'').
3. **Garder core comme contrôle stable** : c'est la seule construction connue, non dégénérée, à la
   fois stable et non percolante.
4. **À K ≥ 3 (donc K5 et K10), déclarer la cible** : couvertures non ambiguës (U1) ou cœurs (core).
   Aucune hiérarchie laminaire non percolante ne respecte les deux. À K = 2, UC respecte les deux.
5. **Ne pas porter la majorité à masse fixe** : elle paie un retard sans gagner de stabilité.
6. **Bande η** : elle n'est dans aucun ensemble maximal. Elle n'est justifiée que par un gain
   statistique mesuré, bord de bande publié.
7. **Stabilité** : impossible avec E_u. La stabilisation doit venir de la tête (condensation, mcs)
   ou d'un renoncement déclaré à E_u, jamais d'un départage caché.
8. **Plusieurs K** : une hiérarchie unique fidèle à deux ordres et emboîtée verticalement n'existe
   pas. Choisir une tranche monotone (la théorie à K fixé s'y applique mot pour mot) ou transporter
   depuis l'ordre le plus haut (fidélité à cet ordre seulement).

## 10. Littérature (énoncés vérifiés)

- J. Kleinberg, *An Impossibility Theorem for Clustering*, NIPS 2002 : aucune fonction de partition
  n'est à la fois invariante d'échelle, riche et cohérente. Nos impossibilités portent sur autre chose :
  la projection d'un objet de densité fixé vers une hiérarchie de points.
- G. Carlsson, F. Mémoli, *Characterization, Stability and Convergence of Hierarchical Clustering
  Methods*, JMLR 11 (2010) 1425–1470. Théorème 18 : une méthode hiérarchique qui vérifie (I) la règle
  à deux points, (II) des hauteurs non croissantes sous les applications qui ne dilatent pas les
  distances, et (III) u ≥ sep(X), est la liaison simple. Proposition 26 : stabilité de Gromov–Hausdorff.
  Leur figure 15 montre l'instabilité de la liaison complète. À K = 1, FULL est la liaison simple et
  toutes nos exigences sont compatibles.
- A. Rolle, L. Scoccola, *Stable and Consistent Density-Based Clustering via Multiparameter
  Persistence*, JMLR 25 (2024), article 258. Les tranches à un paramètre du degree-Rips (liaison simple
  robuste, algorithme plug-in) sont discontinues pour la distance de Gromov–Hausdorff–Prokhorov
  (§ 3.3) ; la λ-liaison (exemple 34) vérifie `d_CI ≤ max(2|σ|, 1) d_GHP` (corollaire 42) et est
  consistante (théorème 58). Le terme employé est λ-liaison, pas γ-liaison. Notre théorème B est une
  autre instabilité : l'arbre à K fixé est exact et stable, c'est la projection sur les points qui ne
  peut pas l'être dès qu'elle est fidèle.
- K. Chaudhuri, S. Dasgupta, *Rates of Convergence for the Cluster Tree*, NIPS 2010. Liaison simple
  robuste (figure 3) : sommets `{ x_i : r_k(x_i) ≤ r }`, arêtes `‖x_i − x_j‖ ≤ α r`. C'est une
  sémantique cœur : un point n'entre qu'à son rayon k-NN.
- K. Chaudhuri, S. Dasgupta, S. Kpotufe, U. von Luxburg, *Consistent Procedures for Cluster Tree
  Estimation and Pruning*, IEEE Trans. Inf. Theory 60(12), 2014 : consistance et élagage.
- J. Eldridge, M. Belkin, Y. Wang, *Beyond Hartigan Consistency: Merge Distortion Metric for
  Hierarchical Clustering*, COLT 2015, PMLR 40:588–606. Notre Stab est formulé sur les mêmes hauteurs
  de réunion.
- R. Campello, D. Moulavi, J. Sander, *Density-Based Clustering Based on Hierarchical Density
  Estimates*, PAKDD 2013, 160–172 (HDBSCAN).
- E. Schubert, J. Sander, M. Ester, H.-P. Kriegel, X. Xu, *DBSCAN Revisited, Revisited*, ACM TODS
  42(3), article 19, 2017. Texte relu : les points frontière « can be density-reachable from more than
  one cluster » ; DBSCAN « simply assigns border points to the first cluster they are reachable from » ;
  le résultat « may change if the dataset is permuted » ; dans HDBSCAN*, « the concept of border points
  was abandoned, and only core points are considered to be part of a cluster ». C'est le dilemme de nos
  théorèmes A et B : DBSCAN a choisi A1, HDBSCAN* a choisi core ; U1 est l'option canonique neutre.
- F. Chazal, L. Guibas, S. Oudot, P. Skraba, *Persistence-Based Clustering in Riemannian Manifolds*,
  J. ACM 60(6), 2013 (ToMATo) : cible de bassins, distincte des composantes de niveau.
- A. Blumberg, M. Lesnick, *Stability of 2-Parameter Persistent Homology*, Found. Comput. Math. 24(2)
  (2024), 385–427, en ligne en 2022 : stabilité de la multicouverture. Elle vaut pour FULL, pas pour une projection fidèle.

## 11. Questions ouvertes et conjectures

1. **Règle stable plus précoce que core.** Existe-t-il une règle NP laminaire lipschitzienne qui entre
   strictement avant d_K sur un ouvert de configurations ? Candidat : unanimité adoucie, dont la date
   d'entrée dépend continûment de l'écart entre composantes concurrentes. Elle violerait E_u.
2. **Stabilité conditionnelle d'U1 (conjecture).** U1 est localement 1-lipschitzienne là où les
   événements des profils de couverture ne coïncident pas et où aucune coquille n'est dégénérée.
   Obstructions connues : ex æquo (F2) et branches fantômes (fixture K5 de l'audit, § 11). Seul le
   lemme 3 est prouvé.
3. **N1 et stabilité.** Existe-t-il une règle neutre, complète et stable, non dégénérée ?
4. **Nombre minimal de sites** pour le théorème C : K+2 trouvé pour K = 3 à 8 et 10, 2K−1 prouvé.
5. **Fréquence des ex æquo exacts** de première couverture sur les trames LiDAR à 1 mm : c'est
   l'ensemble où U1, A5 et A1 diffèrent.
6. **Multiplicités** : la tour pondérée est refusée ; la théorie n'est pas écrite pour des poids.

## 12. Reçus et reproduction

Dossier : `/workspaces/E-HGP/build/v10-verrou-points/axiomes_impossibilite/`.

| Fichier | Rôle | SHA-256 (16 premiers) |
| --- | --- | --- |
| `code/hgpax.py` | oracle Γ_K exhaustif (toutes les K- et (K+1)-parties), règles, axiomes, fermeture forcée | `471be3f17adb530a` |
| `code/line1d.py` | moteur aligné (K-fenêtres consécutives), contre-vérifié contre Γ | `bd2e2d56e9fa555d` |
| `code/fixtures_axiomes.py` | toutes les fixtures des théorèmes, contrôles exacts | `4df39f7a6581eaf5` |
| `code/native_check.py` | exports natifs `export_frontier` contre Γ | `36ad11df0f6cc851` |
| `code/reference_check.py` | référence du dépôt `hgp10_ref.py` contre Γ | `833f0d893c9a05c2` |
| `code/crosscheck_line.py` | moteur aligné contre Γ | `256c3deebd3b3604` |
| `code/search_conflicts.py`, `code/explain.py` | recherches bornées et explications pas à pas | — |

| Reçu | Contrôles | Écarts | Normal et −O |
| --- | ---: | ---: | --- |
| `receipts/fixtures_{normal,optimized}.json` | 123 | 0 | contenus identiques |
| `receipts/native_{normal,optimized}.json` | 267 (13 fixtures, 1 à 2 ordres) | 0 | empreintes d'export identiques |
| `receipts/reference_{normal,optimized}.json` | 417 (19 fixtures, coupes fermées, ouvertes, C ∩ X) | 0 | lignes identiques |
| `receipts/crosscheck_line_{normal,optimized}.log` | 45 845 (300 nuages dev) | 0 | code 0 |

Commandes, depuis `code/` : `python3 -B fixtures_axiomes.py --out=R.json`, puis la même avec `-O` ;
`python3 -B native_check.py --work=/tmp/... --out=R.json` ; `python3 -B reference_check.py --out=R.json` ;
`python3 -B crosscheck_line.py dev_axiomes_line_2 300`. Graines : `dev_axiomes_*` uniquement.
Binaire natif : `export_frontier`, SHA-256 `b2dc98243c4230761ff6384660978f5972e21d9db62f30621eb8de2be78d6616` ;
`frontier_core.py` `86ba984ff7986bdd…` ; référence `hgp10_ref.py` `2cb84ad549b1f898…`. Aucun de ces
fichiers n'est modifié ; aucun bytecode n'est écrit hors du dossier de travail.

Limites. Les fixtures établissent des impossibilités et des égalités exactes ; elles ne mesurent ni
ARI, ni EOM, ni coût à 8k/16k/32k points, ni fréquence des ex æquo sur LiDAR. Les preuves
générales sont dans ce mémo ; les contrôles d'échantillon ne les remplacent pas. Le registre
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (section V10) devrait recevoir les théorèmes A à F
avec ces fixtures ; ce mémo ne l'édite pas, le worktree partagé étant en lecture seule.
