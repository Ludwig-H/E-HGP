# Généraliser le complexe alpha à l'ordre K pour représenter robustement les niveaux d'un polyèdre

7 octobre 2026, synthèse rédigée de 01 h 29 à 01 h 40 UTC (heures lues par `date -u`), pour Louis Hauseux. Elle réunit
l'oracle exact, quatre notes théoriques, quatre expériences et deux critiques adverses de ce workflow, après lecture
intégrale des deux réponses de l'auditeur (`d2be6bdc7`, `28d70f8ab`) et de la réponse du développeur (`ee2df0362`).
Version courte, sans jargon : [REPONSE_COURTE.md](REPONSE_COURTE.md).

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucun commit ; aucune écriture hors de build/
```

Statuts : **[P]** prouvé (preuve écrite ici ou par l'auditeur, relue par la critique) ; **[V]** vérifié borné
(arithmétique exacte, petits nuages : oracle de correction, jamais une pente) ; **[M]** mesuré ; **[E]** extrapolé ;
**[J]** jugement visuel non aveugle ; **[C]** conjecture ; **[R]** réfuté par un contre-exemple exact. Rien n'est
« exact » au sens du registre. Dans les formules, l'ordre K est noté k ; A_k(r) désigne aussi sa réalisation.

## 0. La réponse en bref

1. **L'objet (fixé avec l'auditeur).** Au niveau r et à l'ordre K, garder les cellules σ de la mosaïque de Delaunay
   d'ordre K dont la date ne dépasse pas r² :

   $$a_{\sigma}=\min_{y\in F_{\sigma}}d_{k}(y)^{2},\qquad A_{k}(r)=\lbrace\sigma : a_{\sigma}\le r^{2}\rbrace$$

   (F_σ face de Voronoï d'ordre k duale, d_k distance au k-ième site). C'est la **tranche de profondeur K du pavage
   rhomboïdal**, filtrée par le rayon de ses sphères, et c'est aussi le **Delaunay pondéré des barycentres** des
   K-parties (BCY th. 4.8), à condition de le dater par le rayon du K-ième voisin et non par la puissance. Pour K = 1,
   c'est le complexe alpha. Une cellule appartient à un nœud par ses étiquettes (K-parties), jamais par sa position.
2. **Ses garanties [P].** A_k(r) a le type d'homotopie de la région dense Ω_k(r), dégénérescences comprises ; ses
   composantes sont celles de Γ_K, donc les nœuds de FULL ; les étiquettes de ses sommets donnent exactement
   P ∩ (C ⊕ B_r), le K-polyèdre discret de la thèse (déf. 8, th. 2) ; A_{k,v}(r) ⊆ C_v(r) ⊕ B_r, à distance de
   Hausdorff ≤ r de C_v(r). Vérifié : 0 désaccord nœud par nœud avec FULL sur 91 ordres (oracle) et 417 nœuds,
   dont ceux de nuages de 8 000 à 45 845 sites [V, M].
3. **Le représentant robuste recommandé** (§ 1.3) : **stockage par événements** (chaque cellule une fois, avec sa date
   et le nœud où elle apparaît ; une coupe est une requête) ; **réduction filtrée certifiée à sommets protégés**,
   causale, certifiée une fois au sommet de la chaîne puis restreinte, vérifiée sur toutes ses dates (modèle
   topologique compact, pas un dessin) ; **dessin** par faces exposées et strates isolées de A_k ; **budget
   géométrique** D_v(r) publié par composante ; **niveaux** : naissance, mort, paliers, coupe intérieure à marges,
   coupe de couverture ; **entre ordres** : un représentant par ordre et les liens π0 de FULL.
4. **Robustesse (§ 3).** Robustes : la filtration (entrelacement en r sous déplacement apparié, décalage d'ordre sous
   ajouts ou retraits) [P ; M : 64/64, 0 violation sur 209 406] ; l'identité des nœuds FULL de vie > 2δ [P] ; la forme
   aux coupes intérieures à marges > δ [M : Hausdorff médian 0,95 δ]. Non robustes : le dessin à une coupe critique
   [M : 44 nœuds sur 141 sautent], l'invariance à K fixé sous ajouts [M : 33/33], l'identité des chaînes contractées
   [R], la décimation sans poids [P, M].
5. **La question épineuse reste ouverte.** Aucun représentant n'est à la fois petit, certifié et au coût visé. A_k
   compte ~1 000 faces par site à K = 5 [M] ; la réduction certifiée gagne ×2 à ×7 sur un nuage ou un objet (×1 à ×3
   par nœud et par niveau) et n'allège pas le dessin [M] ; sous une hypothèse non vérifiée, A_5 d'une trame coûterait
   6 à 8 s en natif [E]. Réponse pratique : **identité, hiérarchie et couverture viennent de FULL (exactes, dans le
   budget) ; la forme exacte se calcule en aval, à la demande, sur quelques nœuds ou chaînes, K = 2 d'abord.** Un petit
   représentant de toute la hiérarchie ne peut être qu'approché, avec un contrat déclaré (D_v, Alonso), non qualifié.
6. **Ce qui fait reconnaître un objet, c'est le rayon choisi le long de la chaîne, pas le rendu [J].** Vélo
   synthétique à K = 5 : roue en anneau à 46,6 mm ; vélo à deux anneaux, cadre, selle et guidon à 77–89 mm ; disques
   pleins au nœud « vélo » (155 mm). Ces nœuds vivent 0,1 à 0,4 mm, sous le bruit de quantification. Le vélo réel
   (08/002852) n'est reconnaissable à aucun K.

## 1. L'objet et le représentant

### 1.1 Les idées de l'utilisateur, une par une

- **Garder les supports des boules.** Ce sont les supports S* de FULL : l'analogue du K-MST (déf. 27–30, th. 4–6),
  squelette des fusions, constant pendant la vie d'un nœud. Dessinés par enveloppe, ils remplissent les roues
  (4 ouvertures sur 7 aux nœuds « roue » à K = 5, contre 6 sur 7 pour A_5) [M]. C'est un squelette π0, pas une forme.
- **Une tranche du pavage rhomboïdal.** Oui, exactement : la tranche horizontale de profondeur K est la mosaïque
  d'ordre K (Edelsbrunner–Osang), et le rayon d'une cellule est celui de son plus petit rhomboïde porteur ; l'oracle
  retrouve ces rayons égaux à a_σ sur 32 ordres sur 32 [V]. Garder « tous les rhomboïdes de petit rayon » sans la
  contrainte de profondeur, ou tronquer naïvement les porteurs, est faux (auditeur, § 5).
- **Barycentres pondérés et Delaunay pondéré (BCY).** Même squelette, mauvaise horloge : le diagramme de puissance des
  (c_Q, w_Q) est celui d'ordre K, donc la même mosaïque ; mais l'alpha pondéré date les cellules par la distance à la
  mesure, b_σ ≤ a_σ ≤ k b_σ : autre hiérarchie, qui fusionne plus tôt ({0, 1, 2, 11}, k = 3 : arête à 251/12 au lieu
  de 121/4) [V]. Le nerf abstrait du diagramme n'est pas la mosaïque plongée (tétraèdre régulier, k = 2 : cellule
  octaèdre) [V]. La distance à la mesure reste un attribut (couleur, priorité, indicateur), jamais une décision.
- **Somme de Minkowski avec B_r.** L'offset C ⊕ B_r est une enveloppe de couverture : sa trace sur P est l'amas
  discret [P], mais il bouche des trous et en crée [V : anneau8, fer à cheval], et un contact d'offsets annonce une
  fusion dans (r, 2r] sans en être une [P, borne atteinte]. Celui du vélo est un bloc [J].

### 1.2 Tableau d'analogie K = 1 ↔ K

| Rôle | K = 1 (thèse, ch. 2) | Ordre K (thèse, ch. 6 et 8 ; auditeur) |
| --- | --- | --- |
| Objet de Hartigan | Ω_1(r) = réunion des B(p, r) ; Single-Linkage | Ω_K(r) = L_K(r) (déf. 6–7) |
| Épais, combinatoire | Čech ; graphe géométrique G(X, r) (déf. 5, 20) | nerf des régions témoins W_Q(r) ; 1-squelette Γ_K (déf. 21–22, th. 2) |
| Mince : la forme | complexe alpha = Delaunay filtré (classique, absent de la thèse) | A_K(r) : mosaïque d'ordre K (déf. 31) filtrée par a_σ |
| Squelette H0 | MST euclidien (faits 1–2, th. 1) | K-MST (déf. 30, th. 5) ; MST de la mosaïque (E14) ; supports S* |
| Points couverts | P ∩ C : partition, constante pendant une vie | P ∩ (C ⊕ B_r) : recouvrement, croît pendant une vie (déf. 8) |
| Partition de l'arbre | sites | K-parties (§ 9.1) ; cellules de la mosaïque (E1) |
| Réduction certifiée | Wrap (Bauer–Edelsbrunner) ; le causal y est inclus | représentant causal à sommets protégés (T2) ; pas de Wrap par analogie |
| Rendu sur les données | ombre = alpha (O2) | ombre ⊇ A_K, trace exacte, non homotope (O5) |
| Points pondérés | puissance = Voronoï ; f_1 = d_1 | même dual ; f_K ≠ d_K, autre hiérarchie |
| Robustesse | entrelacement de δ ; offset = Ω_1(2r) | idem sous bijection ; population : décalage de K |

À K = 1, chaque proposition redonne le complexe alpha : A_1 [V : mêmes simplexes que gudhi hors des cellules
cosphériques, gardées en polytopes], l'ombre (O2) [P], D_1 (§ 1.4) [P], et le représentant causal y est inclus dans
le Wrap filtré [P].

### 1.3 Le représentant recommandé, pièce par pièce

**Réalisation.** Subdivision régulière exacte (cellules polytopes, aucune triangulation imposée, aucune simulation de
simplicité), sommets aux barycentres c_Q, niveaux certifiés par un témoin et des multiplicateurs KKT exacts (oracle).
Toute mosaïque passe un contrôle **strict** (appariement des 2-faces, faces de bord sur l'enveloppe, Euler = 1),
jamais un certificat de volume, aveugle au trou de 29 220 mm³ de 08/000100 (localisé à 2 cellules, reproduit sur 13
sites : défaut du prototype, pas des mathématiques).

**Stockage et niveaux (E1) [P].** Chaque cellule σ a un propriétaire ν(σ), le nœud vivant à √a_σ qui la contient ;
les descendants Z_v sont laminaires et A_{k,v}(r) = {σ ∈ Z_v : a_σ ≤ r²}. La famille de tous les nœuds tient dans
la taille de la mosaïque. La profondeur des arbres (moyenne 4 800 à 10 000 à K = 5 sur KITTI [M]) interdit de
stocker une coupe par nœud. Les niveaux publiés d'un nœud : b_v et d_v exacts ; ses paliers (intervalles entre
événements H1/H2 de Z_v) ; la coupe canonique (milieu du plus long palier, marge = demi-palier), certifiable au bruit
δ si le palier dépasse 4δ (E3) ; la coupe de couverture A_v(d_v^−) = {a_σ < d_v²} avec sa frange ; la profondeur
d'ordre ; pour la racine, la borne canonique R(P), rayon de la plus petite boule englobante (L0) [P].

**Réduction [P].** Représentant causal à sommets protégés (T2) : appariement acyclique de paires (facette, cofacette)
de même naissance exacte ; dates d'entrée e_σ ≥ a_σ (une paire reste effondrée jusqu'à ce qu'une coface nouvelle
l'en empêche, puis entre dans L avec son partenaire) ; L_r = {e_σ ≤ r²} est emboîté, A_k(r) s'effondre sur L_r à
tout r, et L_r ne dépend que de la composante. On certifie une fois au sommet de la chaîne voulue puis on restreint : taille mesurée +0 à 3 % ; une suite
certifiée pour un nœud seul cesse d'être exécutable sur la vie du parent dans 47 cas sur 57 [M]. Le vérificateur
lit toutes les dates : un échantillon de 50 dates a laissé passer un mutant [M]. Appariement par dimension croissante
ou optimum tabulé par type de groupe ; départage par diamètre puis étiquettes triées (reproductible, pas canonique).

**Budget géométrique.** Sommets protégés ⇒ d_H(L_{r,v}, A_{k,v}(r)) ≤ D_v(r) (plus grand diamètre de cellule de la
composante), d_H(L_{r,v}, C_v(r)) ≤ r et couverture exacte [P, auditeur]. D_v(r) ≤ min(j, m − j) diam(U)/k, donc
≤ 4r/k en position générale ; mesuré max D_v/r = 2,00 / 1,96 / 1,32 / 0,79 à K = 1 / 2 / 3 / 5, jamais au-dessus de
4/k [M ; oracle : 1,876 sur 4 119 couples (composante, niveau) à k = 2, V]. Règle locale optionnelle :
diam τ ≤ θ√a_τ ⇒ d_H(L, A) ≤ θr (P7) [P]. Retirer des sommets coûte la borne à C : d_H(L, C) ≤ r + λ avec
λ ≤ 2r/k, et « ≤ r » devient faux (F1) [R].

**Dessin.** Faces exposées + strates isolées de A : le rendu qui garde le mieux les ouvertures des roues à leur nœud
(26/29, contre supports 23, ombre 21, offset 12) [M]. L n'est pas un dessin : ses faces exposées valent 0,5 à 5,8 fois
celles de A (médiane ≈ 1), et il « ouvre » une roue pleine (jusqu'à 47 points de pourcentage du disque) sans changer
H1 [M]. L'ombre S_v(r), réunion des conv(Q réels) des cellules, a la trace exacte, est contenue dans C ⊕ B_r et à
Hausdorff ≤ r de C [P] ; mais elle n'est pas homotope (O5) et ne sert jamais d'identité : à K ≥ 2, 100 % des nœuds
mesurés ont un amas qui recoupe celui d'un autre nœud vivant, sans fusion [M].

**Entre ordres.** Ordre partiel (K ↓, r ↑) ; régions, offsets et couvertures sont emboîtés, pas les représentants
(A_2(1) ⊄ A_1(1), fixture pqst ; ombres, O4) [R] ; aucun représentant homotope compatible ne garde un trou que la
région a fermé (E13) [P]. L'identité se transporte par π0 ; le domaine d'isolement (E12) lit la robustesse aux
aberrants. Au même rayon, l'image à K = 2 d'un nœud de K = 5 peut être tout l'objet (planche 3 du § 4).

### 1.4 Les candidats écartés ou réservés

| Candidat | Garanties | Défaut décisif | Usage |
| --- | --- | --- | --- |
| A_k exact | homotopie, π0 = FULL, couverture, d_H ≤ r | ~10³ faces par site à K = 5 | référence, en aval |
| L causal, sommets protégés | idem + d_H(L, A) ≤ D_v | plancher = sommets (~K n) ; dessin non réduit | certificat, modèle topologique |
| Ombre S_v | trace exacte, ⊆ C ⊕ B_r, garde les trous libres (O6) | non homotope ; pas une identité | vue de couverture |
| Offset C ⊕ B_r | trace = amas discret | bouche et crée des trous | enveloppe |
| D_v = Del_1 des sites couverts | trace exacte, emboîté en r, K et arbre ; C_v ⊆ R_v ⊆ C_{v↑3r}(3r) | non homotope ; facteur 3 vide sur LiDAR | piste approchée (~28 faces par site, Del_1 global mesuré) |
| Alonso (SoCG 2025) | entrelacement (1 + 3ε) ; taille O(n) | borne de pire cas ≈ 2,9·10⁴ « amis » par point en d = 3 ; identité hors fenêtres (r, cr] seulement | piste pour les niveaux grossiers |
| Barycentres témoins, DTM | stabilité de Wasserstein | autre hiérarchie (×√(6k), non vérifié) | attribut |

## 2. Énoncés, statuts et verdict de la critique

Critique théorique : 66 énoncés jugés, 42 confirmés, 8 nuancés, 12 non vérifiés ; son tableau liste trois
réfutations (N3, P5 lu comme taille atteignable, L), son résumé en annonce quatre. Critique expérimentale : une mesure
clé de chaque expérience rejouée indépendamment, sans désaccord ni contradiction mathématique.

| id | Énoncé | Statut | Verdict |
| --- | --- | --- | --- |
| Aud. | A_k(r) ≃ Ω_k(r) même dégénéré ; π0 = Γ_K ; couverture par les labels ; ⊆ C ⊕ B_r | [P] ; [V] 91 ordres, 1 609 tests d'offset | invoqué, rejoué |
| T2 | représentant causal : emboîté, effondrement à tout r, local à la composante | [P] | confirmé (déf. 2 : « plus petite » à lire « plus grande ») |
| P4 | taille de L ≥ Σ μ(G) et ≥ 2V − β0 + β1 + β2 | [P] | confirmé |
| P5, L | « au mieux 295/1 246 à K = 5 », « seuls les tétraèdres critiques restent », « un quart sur LiDAR » | [R] | réfutés : minorant non atteignable (bipyramide_forcage) |
| T6, F1, P7 | retrait de sommets : d_H(L, C) ≤ r + λ, « ≤ r » faux ; diam τ ≤ θ√a ⇒ d_H(L, A) ≤ θr | [P], [R], [P] | confirmés |
| O, O2 | dimension croissante atteint μ ; départage par diamètre suffit | [V] partiel, [C] | nuancé ; non vérifié |
| P8 | A_{k,v}(r) se construit sur P_v(r) plus un test ambiant | [P] | confirmé |
| E1 | propriétaire unique, Z_v laminaires, coupes = filtres par date | [P] | confirmé |
| E2, E3 | sandwich d'identité et de couverture ; homologie certifiée si palier > 4δ | [P] | confirmés (« ssi » à lire « si ») |
| E7, E9 | contact d'offsets ⇒ fusion dans (r, 2r] ; l'offset bouche et crée des trous | [P], [V] | confirmés |
| E11, O4 | pas d'inclusion géométrique entre ordres (A_k, ombre) | [R] | confirmés (rejeu oracle) |
| E13, E14 | obstruction des trous entre ordres ; MST de la mosaïque = arbre de Γ_K | [P] | confirmés |
| S6 | six points, K = 2 : la fusion finale crée deux trous sur [√(2+√3), 2) | [V] | confirmé |
| R0, N1 | (δ ; d, e)-entrelacement des tours π0 et H_i ; nœud de vie > 2δ : image = chaîne, nœuds robustes distincts | [P] | confirmés |
| N2 | l'identité d'un nœud est discontinue même si δ → 0 | [V] (fixture coupure) | confirmé |
| N3 | identités stables sur l'arbre δ-contracté | [R] | réfuté (contraction_instable) |
| N4, E12, Q1, S1 | profondeur d'ordre ≥ d + e protège l'identité ; multiplicités : ordre conservé ; sans poids : décalage ≥ 1 | [P] | confirmés ; S1 nuancé |
| G0–G5 | Ω_1 δ-stable, pas Ω_k ; dist(y, C) ≤ κ (d_k − r) ; dates de cellules non lipschitziennes | [P] (lemme cité de mémoire), [R] | confirmés ; G2–G3 nuancé (κ à corriger) |
| O1–O6 | ombre : définition intrinsèque, = alpha à K = 1, non homotope, garde les trous libres | [P] | confirmés |
| D3–D5 | D_v : facteur 3 atteint, d_H ≤ 2r | [P] | confirmés ; D4 nuancé (sites partagés) |
| D7, A1–A4, DTM | ~28 faces par site ; constantes d'Alonso ; ×√(6k) | [M] global ; cités | non vérifiés |

## 3. Robustesse

### 3.1 Définition opérationnelle retenue

Une représentation des niveaux est **(δ, m)-robuste** si : **RR1** sa topologie est certifiée sur toute la filtration
(A_k, ou L causal vérifié sur toutes ses dates) ; **RR2** elle est emboîtée en r à K fixé, ses composantes égalent
celles de FULL à chaque coupe publiée, et étiquettes, dates et couverture sont exactes ; entre ordres, seuls les
liens π0 sont exigés ; **RR3** l'identité n'est revendiquée que pour les **nœuds FULL** de vie > 2δ (N1), avec une
profondeur d'ordre ≥ m si l'on revendique la résistance à m aberrants, **jamais pour des chaînes contractées** (N3
réfuté) ; **RR4** D_v(r)/r est publié, la stabilité du dessin n'est revendiquée qu'aux coupes intérieures à marges
> δ, avec un certificat κ (corrigé, § 3.2) si l'on veut une borne ; **RR5** taille et coût sont mesurés à 8 000,
16 000, 32 000 sites. δ déclaré : √3/2 mm pour l'arrondi au mm sans fusion de retours, plus le bruit capteur ; sinon
modèle à multiplicités. Bilan : RR1, RR2 tiennent ; RR3 tient pour une minorité de nœuds ; RR4 tient par nuage ;
RR5 échoue pour « petit ».

### 3.2 Énoncés

Ceux du § 2 (R0, N1–N4, Q1, S1, E2–E3, E12, G0–G5) ; un seul demande une précision ici. **G2–G3 [P]** : si
τ = sup R_Σ(x)/d_k(x) < 1 sur la coquille d'une composante, alors dist(y, C) ≤ κ (d_k(y) − r), κ = (1 − τ²)^(−1/2),
et d_H(C^P, C^P') ≤ κδ. Le certificat calculable (sur la mosaïque, pas par FULL seul) est corrigé par la critique,
l'ancien valant 100/81 > 1 sans point critique ; le maximum porte sur les cellules σ dont la face duale rencontre la
coquille par son intérieur relatif :

$$\tau\le\max_{\sigma}\frac{R(\cup\,\mathrm{labels}(\sigma))}{\max(r,\sqrt{a_{\sigma}})}$$

### 3.3 Mesures

| Épreuve | Résultat | Statut |
| --- | --- | --- |
| Bruit apparié σ = 1, 3, 10 mm, 7 jeux, K = 1–5 | d_B ≤ δ dans 64/64 cas (médiane 0,36 δ en H0, 0,26 δ en H1, max 0,61) ; DTM aussi | [M] |
| Quantification h = 1, 2, 5 mm | 0 fusion de sites sur 48 cas, d_B ≤ δ ≤ √3 h/2 ; trames 08 à 1 mm : 0 fusion (e ≡ 0) | [M] |
| Forme à rayon fixé (167 coupes intérieures, 141 nœuds) | marges > δ : Hausdorff médian 0,95 δ (33 coupes) ; toutes : 2,5 δ ; coupes critiques : 44 nœuds sautent, jusqu'à 1 426 mm ; triangle : H/δ = 22,5 | [M] |
| Vies des nœuds FULL (3 trames, 35 551 à 45 845 sites, δ = √3/2 mm) | K = 5 : vie > 2δ pour 33–39 %, > 4δ pour 22–28 % ; vie médiane 0,58–0,87 mm ; d_v ≥ 3 b_v : 0,01 % | [M] |
| Ajouts proches de m < K sites, dès m = 1 | composante nouvelle 33/33, fusion 26/33 (fréquence dictée par le protocole) | [M] |
| Groupe lointain (> 2r*) ; inclusions décalées en K | m < K : diagrammes identiques sous r* 12/12 ; 0 violation sur 209 406 | [M] |
| Décimation 50 % sans poids | d_B ≥ espacement 14/18 ; k' = arrondi(kρ) meilleur 8/18 ; transport pondéré non mesuré | [M] |
| Entrelacements π0 exacts (oracle) | 2 993 coupes : composées = inclusions ; composantes différentes à (k, r) fixés : 37 % (positions), 19 % (ajouts) | [V] |
| Réétiquetage, réduction | mosaïque identique ; L change (Jaccard 0,91–0,97) : reproductible, pas canonique | [M] |

### 3.4 Lecture

La filtration est robuste, le dessin ne l'est pas. Une hiérarchie de polyèdres robuste se lit sur les nœuds de vie
longue, aux coupes intérieures, et le long de l'axe des ordres pour les aberrants. Or, à K = 5, les nœuds qui portent
les formes reconnaissables vivent 0,1 à 0,4 mm : sous σ = 10 mm, la composante appariée par les labels garde sa forme
(écart moyen d'IoU ≤ 0,03, grâce aux voisins dans la chaîne), mais ce n'est pas « le même nœud ». La forme est
robuste comme **rayon d'une chaîne**, pas comme identité de nœud ; indexer un jeton par (chaîne, rayon, marges) est
une conjecture [C], et aucune identité de chaîne n'est prouvée stable (N3).

## 4. Preuves expérimentales et planches à montrer

Je les ai regardées ; chemins relatifs à ce dossier.

1. [Chaîne roue → vélo, K = 5](experience_rendu/png/synth_velo_05m_k5_chaine_vélo_roue_avant.png) : anneau à 46,6 mm,
   deux anneaux et cadre à 77–89 mm, disques pleins à 155 mm ; l'ombre (bas) en plus épais. La planche centrale.
2. [Candidats au nœud « vélo »](experience_rendu/png/candidats_synth_velo_05m_objet0.png) : A_1 à A_5, ombres, offset
   (un bloc), supports : roues pleines partout au nœud objet.
3. [Liens K = 5 → K = 2](experience_rendu/png/synth_velo_05m_liens_k5_k2.png) : au même rayon, la roue de K = 5 a
   pour image à K = 2 le vélo entier ; un représentant par ordre, aucune inclusion des solides.
4. [Six points du § 6.1, K = 2](experience_rendu/png/six_points_k2.png) : 7 → 3 → 1 composantes, ombres qui se
   touchent sans fusion ; [version réduite](experience_reduction/planches/planche_six_points_k2.png) : figure 6.5.
5. [Un nœud à vie longue](experience_rendu/png/synth_anneau_perce_10m_k2_noeud120_niveaux.png) (55 → 141 mm, K = 2) :
   le dual passe d'un « C » fin à un « C » épais ; l'offset devient un disque.
6. [A, L réduit, L partielle, ombre](experience_reduction/planches/planche_synth_velo_10m_k5_objet0.png) : L est un
   filet qui « ouvre » les roues sans trou topologique ; l'ombre est un bloc épais.
7. [Bruit σ = 10 mm sur l'anneau](experience_robustesse/png/planche_synth_anneau_perce_05m_k2_sigma10.png) : à la
   naissance, le dessin saute (IoU 0,46, Hausdorff 475 mm) ; au milieu et avant la mort, IoU 1 et 26 à 28 mm.
8. [Triangle de l'auditeur](experience_robustesse/png/planche_contre_epreuve_triangle.png) : plein contre deux côtés, même homotopie, H/δ = 22,5.
9. [Vélo réel, K = 5](experience_echelle/png/planche_decoupe_08_002852_deux_velos_6_51_instances_k5_objet0.png) : un bloc, trois coupes identiques.
10. [Piéton, K = 5](experience_rendu/png/synth_pieton_05m_k5_chaine_piéton_jambe_gauche.png) : jambe à 45 mm, corps entier à 88 mm.

Nœuds choisis par la vérité terrain : ces images bornent ce que l'arbre contient, elles ne sont pas une méthode.

## 5. Taille et coût ; ce que FULL doit exporter

### 5.1 Mesuré [M]

| Quantité | K = 1 | K = 2 | K = 3 | K = 5 |
| --- | --- | --- | --- | --- |
| Faces de la mosaïque par site (mosaïques locales ; nuages entiers pour K ≤ 2) | 26–28 | 132–144 | 333–353 | 1 003–1 086 |
| Nœuds FULL par site (sous-ensembles KITTI ; tuiles) | 2,0 | 4,65–4,80 ; 4,01 | — | 13,6–16,1 ; 9,35 |
| Nœud d'objet à d_v^− : faces (par point couvert) | — | 1,8 k–143 k (3,6–129) | — | 6 k–1,08 M (76–983) |
| L/K, nuage entier de 8 000 à 32 000 sites (trame 08/000000) | 0,44–0,53 | 0,38–0,47 | non mesuré | non mesuré |
| L/A par nœud d'objet | 0,28–1,0 | 0,21–0,82 | — | 0,16–0,45 |
| Faces exposées par point couvert (nœud objet, petits jeux) | 1,8 | 5,6 | 10,4 | 18,8 |
| Construction Python par point du voisinage Y | 0,4–1,0 ms | 1,7–6,8 ms | 7–14 ms | 38–75 ms |

La mosaïque compte 28–30 fois (K = 2) et 62–82 fois (K = 5) plus d'éléments que la tour. La taille d'un nœud ne
dépend pas de n (mêmes comptes exacts à 16 000, 32 000 et 45 845 sites) ; le nombre de nœuds et la profondeur, si.
Les certificats de réduction sont complets à K = 1 (8 000 à 32 000) et K = 2 (8 000) ; à K = 2, 16 000 et 32 000,
seules 20 000 dates sont vérifiées : non établis. À K = 5, aucun nuage entier n'est mesuré.

### 5.2 Extrapolé [E]

Hypothèse H, non vérifiée : un constructeur natif au coût par élément de la tour sur G4 (~165 ns par nœud à K = 5).
Alors A_5 d'une trame (≈ 48 M faces) coûte 6 à 8 s, 62 à 82 fois le budget de 100 ms, et ~1 Go ; A_2 (≈ 6,2 M
faces) ~1 s ; une voiture à K = 5 (0,56 M faces ; 1,08 M au haut de vie) coûte à peu près la tour entière, ~1/10 à
K = 2. Rien n'est mesuré en natif ni sur G4. Le complexe exact, même réduit, n'entre pas dans 100 ms pour toute la
hiérarchie ; il peut accompagner, en aval et à la demande, quelques jetons à K = 2.

### 5.3 Ce que FULL doit exporter pour un calcul local, sans mosaïque globale

1. b_v et d_v exacts (déjà publiés) ; une K-partie de naissance par nœud (population I_b ∪ U_b), aujourd'hui recalculée ;
2. les points couverts à d_v^− (la pendaison MHGP11PT devrait les donner : à vérifier), d'où Y = P ∩ (étiquettes ⊕
   B_{2 d_v}) et le certificat de localité sans itération ; le nuage et son index spatial ; boules de naissance et
   postordre du sous-arbre pour les contrôles ;
3. pour la robustesse : vie, liens verticaux (profondeur d'ordre), δ déclaré, e(ρ) ou multiplicités si des retours
   fusionnent ; pour la piste D_v : la date de couverture g_k(p) de chaque site (disponibilité non vérifiée).

FULL ne contient pas, et il faut construire en aval, les sphères non critiques pour H0 (il publie 0,3 à 0,8 % des
3-cellules sur trames) et les cellules critiques H1/H2 qui commandent la réduction.

## 6. Limites, fixtures, questions

### 6.1 Limites

- Tout est en Python, en aval, sur un codespace partagé ; aucun natif, aucun G4. K ≥ 3 n'est mesuré que par nœud.
- Le représentant causal n'est mesuré que dans le plan ; en 3D, on a mesuré le certificat fixe (global, ou par nœud
  sur [0, d_v)). Hors oracle, a_σ suit la formule de la construction, recoupée (oracle, gudhi, FULL, QP numérique),
  sans multiplicateurs exacts. Les Hausdorff sont échantillonnés (bornes inférieures) ; seul D_v est exact.
- La fusion de retours n'a jamais été exercée ; le transport pondéré n'est pas mesuré ; le κ corrigé non plus.
- Reconnaissance : nœuds choisis par la vérité terrain, jugement non aveugle ; « 0 écart π0 » du rendu compare des
  comptes (`jeton.bijection`) ; l'identité nœud par nœud est établie par l'oracle (91 ordres) et l'échelle (417).
- Prototype `mosaique.py` : échoue sans sortie sur les nuages plans ou alignés, et en silence à grande étendue autour
  d'amas presque cosphériques ; utilisable seulement après contrôle strict.

### 6.2 Fixtures (coordonnées exactes, gravées dans `build/`)

Réfutations d'énoncés : `contraction_instable` (N3) : k = 1, P = {(0,0,0), (50,0,0), (99,0,0)},
P' = {(0,0,0), (49,0,0), (99,0,0)}, δ = 1 ; `bipyramide_forcage` (P5) : k = 1, (0,0,30), (10,0,0), (−6,8,0),
(−6,−8,0), (0,0,−5) ; `certificat_kappa_lache` : k = 1, (±10,0,0), (0,5,0), r² = 81, (r + Δ)² = 625/4 ; F1 : (0,0),
(4,1), (8,0), (12,1), (16,0), (20,1), (24,0), (2,9), (22,9), (12,12), k = 2, r² = 289/4, y = (−11/2, 6) ; pqst :
(−1,0), (0,−1/2), (1,0), (0,3/2), r = 1, x = (−1/4, −1/4) ; O4 (ombre d'ordre 2 contre alpha) : (±4,0), (0,4),
(0,1), r² = 16, z = (0, 1/2) ; moyeu carré (O5) : (0,0), (±2,0), (0,±2), k = 2, r² = 3 ; coupure (N2) : (±1,0,0),
(0,1,0), (0,11/10,0), k = 2, avec (0,1,0) → (0,101/100,0) ; aberrants de l'auditeur : {0, 6, 12} + 1 et
{0, 2, 5} + 3, k = 2, r = 2, qui réfutent l'affirmation du développeur (`ee2df0362`) ; F3, F4, F6 dans les notes.
Faits nouveaux exacts : six points à K = 2 (deux trous nés à la fusion finale), anneau8 + centre (obstruction
verticale), fer à cheval (l'offset crée un trou). Défauts d'implémentation, pas des contradictions :
`spheres_concentriques` (oracle), `trou_prototype_minimal` (13 sites), Betti triangulés faux sur cellules
dégénérées, dépassement int64 d'un contrôle, feuille → site à K = 1. Ces réfutations visent des notes de ce
workflow et l'affirmation du développeur, pas le registre : la règle de CLAUDE.md (fixture permanente,
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`) relève du développeur, l'écriture hors de `build/` étant interdite
ici. Je recommande d'y porter au moins N3, l'affirmation sur les aberrants, E11/O4 et F1.

### 6.3 Questions ouvertes

1. Une réduction certifiée d'ordre K dont le dessin ne change qu'aux valeurs critiques (analogue du Wrap) ?
2. Annuler de façon certifiée des paires critiques H1/H2 de persistance < ε, π0 exact : seule voie sous le plancher.
3. Une identité robuste au-delà des nœuds FULL : (chaîne, rayon, marges), entrelacement d'arbres, sélection du § 9.1 ?
4. Sur LiDAR : κ, part des coupes δ-régulières, paliers H1/H2 des objets devant 4δ ; D_v ⊆ S_v à K = 2 ? g_k(p) exact ?

### 6.4 L'auditeur

`git fetch` à 01 h 23 et 01 h 39 UTC (par les critiques à 00 h 43 et 01 h 21) : aucun commit d'audit après `28d70f8ab`,
seulement des commits du développeur (vitesse de la tour, notes X et Y), aucun `receipts/audit_*` nouveau : pas de
réponse à confronter. Sa liste : oracle et labels, réduction à sommets protégés avec journal et vérificateur,
diamètres par composante, deux rendus nommés, épreuves positions et population, tailles et coûts **faits** ;
échantillonnage avec contrat de masse et Sparse Multicover **non faits**. Ses avertissements sont tous confirmés par
la mesure (dessin instable à rayon fixé, ombre non homotope, aberrants, réductions nœud par nœud, gain modeste).

### 6.5 Questions pour l'auditeur

1. Le représentant causal (T2) satisfait-il l'exigence « certificat sur toute la plage » (§ 3 de `28d70f8ab`) à la
   place du sous-complexe fixe ? « Certifier au sommet de la chaîne puis restreindre » vous convient-il ?
2. N1 tient pour les nœuds FULL, l'arbre δ-contracté est réfuté, et les formes reconnaissables sont portées par des
   nœuds de vie 0,1–0,4 mm : quelle identité accepteriez-vous pour un jeton de forme ?
3. Validez-vous le certificat de κ corrigé (§ 3.2) et une référence précise du lemme de déformation non lisse ?
4. Sous le plancher des sommets : retrait de sommets avec registre (r + λ) et annulation de paires critiques H1/H2
   de persistance < ε, π0 exact : ces voies sont-elles acceptables, et sous quel certificat ?
5. Un résultat sur l'appariement optimal restreint aux intervalles de même naissance, ou une borne du forçage ?
6. Six points, K = 2 : confirmez-vous les deux trous sur [√(2+√3), 2), et faut-il l'annoter dans le § 6.1 ?
7. D_v daté par max(α, g_k), dual de SCov : acceptables comme approximations déclarées (facteur 3 vide sur LiDAR) ?
8. Retours fusionnés : multiplicités dans le prédicat (ordre conservé) ou excès e(ρ) publié par le contrat d'entrée ?

## 7. Suite concrète

### 7.1 Prototype (Python, en aval, borné ; prédictions écrites avant toute mesure)

1. **Fixtures et registre** : graver les fixtures du § 6.2 en tests rejouables (python3 et `-O`, sans `assert`) ;
   proposer au développeur les entrées de `STATUT_PREUVES`.
2. **Constructeur local strict** depuis les exports du § 5.3 : mosaïques d'ordre 1 à K sur Y, contrôle strict,
   re-proposition des cellules refusées par un oracle exact local (corrige le trou silencieux), nuages de rang < 3
   traités. Porte : égalité avec `oracle_ak` (faces, clés, niveaux exacts) sur n ≤ 60 ; mutant « cellule retirée » tué.
3. **Stockage par événements** (propriétaire, date) ; porte : identité nœud par nœud avec FULL par les K-parties de
   naissance, à toutes les coupes, jamais par comptes.
4. **Réduction causale en 3D**, dates d'entrée en ligne ; porte : vérificateur sur toutes les dates, mutants
   (date, liberté, sommet retiré, facette) tués ; mesures : L_r/A_r à K = 2 sur 8 000 / 16 000 / 32 000 sites et sur
   des nœuds d'objets à K = 5, contre le certificat fixe.
5. **Jetons** : paliers, coupe canonique, coupe de couverture, profondeur d'ordre ; part des nœuds et des chaînes à
   palier > 4δ sur LiDAR ; sélection sans vérité terrain (excès de masse, § 9.1) ; reconnaissance en aveugle.
6. **Robustesse restante** : retours fusionnés (grille de 5 à 10 mm sur trames, modèle à multiplicités), transport
   pondéré, κ corrigé mesuré par composante.
7. **Piste approchée** : D_v à 8 000 / 16 000 / 32 000 sites, comparé à A_k sur les mêmes nœuds (trous, facteur réel).

### 7.2 Natif v11, après ces portes

Module C++ en aval, hors du calcul de la hiérarchie (invariant d'architecture), prédicats entiers exacts sur la
grille u21, plateaux en bloc, sans simulation de simplicité ; entrées : exports du § 5.3 ; sorties : (propriétaire,
date, étiquettes) par cellule. Portes à code exact : égalité avec `oracle_ak`, contrôle strict, mutants (cellule
retirée, propriétaire faux, date mutée, sommet protégé retiré), équivariance par permutation, sorties identiques
quel que soit le nombre de fils ; échelle à 8 000 / 16 000 / 32 000 sur G4 dans une session gardée future. Priorité :
jetons à K = 2 à la demande, K = 5 sur objets choisis ; jamais 100 ms annoncés pour toute la tour.

### 7.3 À ne pas faire

Attribuer par position de barycentre ; activer ou rattacher par la DTM ou l'offset ; transposer le Wrap par analogie ;
se fier à un certificat de volume ou à un échantillon de dates ; réduire nœud par nœud sans prolongement ; compter au
lieu d'identifier ; revendiquer un dessin stable à une coupe critique ; présenter L comme le solide.

## 8. Fichiers

`oracle/` (oracle_ak, README) ; `theorie_robustesse/`, `theorie_reduction/`, `theorie_alternatives/`,
`theorie_niveaux/` (NOTE.md) ; `experience_rendu/`, `experience_reduction/`, `experience_echelle/` (README.md) ;
`experience_robustesse/resultats/` (tables.md, synthese.json ; pas de README) ; `critique_theorie/`,
`critique_experiences/` (CRITIQUE.md). Moteur : `mhgp11` à `07428324e`. Rien n'a été recalculé pour cette synthèse.
