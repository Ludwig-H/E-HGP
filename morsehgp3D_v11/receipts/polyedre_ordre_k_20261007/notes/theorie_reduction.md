# Réduction certifiée du complexe alpha d'ordre k et son budget géométrique

6 octobre 2026 ; travail de 22 h 14 à 22 h 57 UTC, rédaction à partir de 22 h 44 (heures lues par `date -u`). Rôle : théoricien du workflow « généraliser le complexe alpha à l'ordre K », chantier « la réduction certifiée et son budget géométrique ».

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (oracles Python exacts, Fraction et Q(√3), en aval ; aucun moteur natif)
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

Point de départ : les deux réponses de l'auditeur, `d2be6bdc7` (objet, labels, inclusions) et `28d70f8ab` (robustesse, effondrements polyédraux, sommets protégés, ombre, niveaux). Je ne refais pas ses preuves ; je les cite « [Aud1 § x] » et « [Aud2 § x] ». Aucun commit postérieur à `28d70f8ab` n'a été trouvé pendant la rédaction.

Étiquettes : **[P]** prouvé ici (preuve complète) ; **[P-ext]** théorème externe cité ; **[V]** vérifié par calcul exact borné (oracle, `n ≤ 14`, plan) ; **[V-exh]** vérifié exhaustivement sur un ensemble fini de cas ; **[M]** mesuré (flottant, approché) ; **[C]** conjecture ; **[R]** réfuté (contre-exemple exact). Aucune mesure d'échelle : tout ce qui touche au coût à `n = 8 000, 16 000, 32 000` est **non mesuré à l'échelle**.

Fichiers (tous dans ce dossier) :

| Fichier | Rôle |
| --- | --- |
| `mosaique2d.py` | oracle exact de `Del_k` dans le plan, dégénérescences comprises (corps `Fraction` ou `Q(√3)`), contrôles stricts (aire exacte, incidences, monotonie, `χ = 1`) et oracle indépendant de `π0` par le graphe `Γ_k` de la thèse |
| `reduction.py` | appariement, dates d'entrée (représentant causal, fixe, profondeur 1), vérificateur indépendant (effondrement effectif niveau par niveau, `π0`, Euler), borne locale exhaustive |
| `experiences.py`, `synthese_aleatoire.py`, `budget_theta.py` | jeux exacts (fixtures, anneaux, six points, chaîne, nuages aléatoires, règle `θ`) ; résultats `resultats/fixtures.json`, `resultats/aleatoire.json`, `resultats/synthese_aleatoire.json` |
| `contre_exemples.py` | F1 (retrait de sommets), F2 (certificat fixe inutile) ; `resultats/contre_exemples.json` |
| `poisson.py` | combinatoire exacte des intervalles relâchés d'Edelsbrunner–Nikitenko et espérances de Poisson ; `resultats/poisson.json` |
| `ordres_intervalles.py`, `ordres_exhaustifs.py`, `ordres_echantillon.py` | gloutons et ordres des ex aequo, par type d'intervalle ; `resultats/ordres_exhaustifs.log`, `resultats/ordres_echantillon.json` (type `(3,3,3)`, échantillonné) |
| `octaedre3d.py` | mosaïque exacte d'ordre 2 de quatre sites de `R^3` (octaèdre), fixtures F3 et F4 ; `resultats/octaedre3d.json` |

## 0. En bref

1. **La bonne forme du certificat est causale, pas fixe.** Toute réduction par paires de même naissance se décrit par un appariement acyclique `M` et des **dates d'entrée** `e_σ ≥ a_σ` (plus petit point fixe : une cellule libre entre à sa naissance, une face entre au plus tard avec ses cofaces, deux partenaires entrent ensemble). Le représentant `L_r = {σ : e_σ ≤ r^2}` est emboîté, `A_k(r) ↘ L_r` à **tout** niveau, et `L_r` ne dépend que de la composante de `A_k(r)` (théorème 2). C'est une certification sur toute la plage, comme l'exige [Aud2 § 3], mais elle ne fige pas le sous-complexe. Pour `k = 1`, c'est (au plus) le Wrap filtré de Bauer–Edelsbrunner. Le certificat fixe de [Aud1 § 4.2] est le cas où l'on fige `L = {e < ∞}` : il peut ne rien réduire du tout (F2 : anneau, 43 cellules sur 43, alors que le causal passe de 42 à 24 cellules au niveau 89).
2. **Combien on peut réduire avec tous les sommets protégés (Q1).** La borne inférieure exacte est locale : chaque groupe de cellules de même centre garde au moins `μ(G)` cellules (lemme 1, proposition 4). Dans le modèle de Poisson et en position générale, par point, on garde au mieux **`6k − 2` cellules sur `12k − 6`** dans le plan (`1/2` asymptotiquement) [P modulo les constantes de Miles] et, dans l'espace, **295 sur 1 246 à `k = 5`** (24 %) et 1 201 sur 5 506 à `k = 10` (22 %) [M : constantes par Monte-Carlo]. Ce qui reste de dimension 3 n'est que les tétraèdres critiques qui bouchent une bulle (13 % des 3-cellules à `k = 5`). Le représentant réduit est donc une **mousse** de dimension ≤ 2, pas un solide. Au pire : aucune réduction (anneau cocyclique, triangulations aiguës de Gabriel).
3. **Protéger moins de sommets (Q2) : oui, sous condition.** Si les sommets retirés sont non critiques, appariés à une arête de même naissance et **deux à deux non adjacents**, alors `d_H(L_{r,v}, A_{k,v}(r)) ≤ D_v(r)` reste vrai, la couverture reste exacte par un registre de labels transférés, mais la borne à `C_v(r)` devient `r + λ_v(r)` avec `λ_v(r) ≤ 2r/k` en position générale (théorème 6). La borne `≤ r` est **fausse** sans protection complète : contre-exemple exact F1 (`24649/340 > 289/4`). Gain : de `6k − 2` à `4k` cellules par point dans le plan au mieux.
4. **Budget géométrique local (Q2/Q3).** Refuser toute paire dont la grande cellule a un diamètre `> θ·√a` donne, sommets protégés, `d_H(L_{r,v}, A_{k,v}(r)) ≤ θ r` à tout niveau (proposition 7) : la condition se vérifie paire par paire.
5. **Ordre des paires (Q3).** La validité ne dépend pas de l'ordre. La taille, si. En dimension 3, « dimension décroissante » **avec un départage arbitraire** n'atteint pas le minimum local sur deux types d'intervalles, les octaèdres non critiques `(2,3,2)` et `(1,3,2)` : 6 ou 8 cellules gardées au lieu de 0 ou 2 [V-exh]. Fixtures exactes à 4 sites, `k = 2`, au § 7. Le départage complet de l'auditeur (petit diamètre, puis labels) n'a échoué sur aucune des 2 747 configurations exactes tirées [V], sans preuve [C]. « Dimension croissante » atteint le minimum sur les 24 types du plan et de l'espace pour **tout** départage, sommets protégés [V-exh ; partiel pour `(1,3,2)`, échantillonné pour `(3,3,3)`]. Les groupes étant petits et de 17 types en position générale, l'optimum exact se tabule par type.
6. **Calcul local depuis FULL (Q4).** Les cellules de `A_{k,v}(r)` sont déterminées par la couverture `P ∩ (C_v(r) ⊕ B_r)` plus un test ambiant de vacuité de chaque sphère (proposition 8) ; une paire se certifie dans son seul groupe, c'est-à-dire une sphère `I, U` (proposition 9) ; la date causale demande la composante entière (pas de rayon de localité borné : l'anneau) mais se calcule en ligne, chaque cellule entrant une fois. FULL ne suffit pas seul : il ne publie que les sphères critiques pour `H0` (naissances et fusions), au plus 16 % des groupes dans le modèle de Poisson volumique à `k = 5`. Coût par point de l'ordre de la mosaïque locale (≈ 1 250 cellules par point à `k = 5` dans le modèle de Poisson volumique ; ≈ 1 000 à 1 200 faces par point mesurées sur trames au workflow précédent) : **incompatible avec 100 ms par trame**, envisageable par nœud sélectionné, en aval. Non mesuré à l'échelle.

**Recommandation au développeur** (prototype en aval, par nœud) :

1. **Représentant.** Prendre le représentant **causal** à sommets protégés : une date `e` par cellule, des paires décidées une fois à leur naissance, une fermeture propagée en ligne. Le vérifier niveau par niveau (sous-complexe, complément apparié, effondrement effectif, `π0`, Euler).
2. **Appariement.** Le choisir **par groupe de même centre**, optimal et tabulé par type en position générale, sinon glouton à dimension **croissante**.
3. **Budget géométrique.** Le déclarer par la règle locale `diam τ ≤ θ √a_τ` (proposition 7), ou publier `max diam` des cellules effondrées par composante.
4. **Ce qu'il faut montrer.** Ne pas présenter `L` comme le solide : c'est une mousse homotope. Le dessin reste celui des faces exposées de `A` ou de l'ombre, avec l'identité du nœud. `L` sert de modèle topologique compact et de certificat.
5. **Au-delà.** Toute réduction supplémentaire passe par l'annulation certifiée de paires critiques de degré ≥ 1 (question 9.1), jamais de `H0`.

## 1. Cadre, notations et un lemme de localisation

`P ⊂ R^3` fini (sites unitaires), `k` fixé, `M = Del_k(P)` la subdivision régulière duale aux domaines `V_Q` pleins [Aud1 § 1.1], `F_σ` la face duale de la cellule `σ`, `a_σ = min_{y ∈ F_σ} d_k(y)^2`, `A_r = A_k(r) = {σ : a_σ ≤ r^2}`. `C_v(r)` désigne une composante de `Ω_k(r)`, `A_{r,v}` la composante correspondante de `A_r` [Aud1 § 1.1–1.3] ; `D_v(r)` le plus grand diamètre d'une cellule de `A_{r,v}` [Aud2 § 2]. Un sommet `c` porte un ou plusieurs labels `Q` (k-ensembles de même relevé). Faits utilisés sans les refaire : `|A_r| ≃ Ω_k(r)` naturellement, la couverture `P ∩ (C ⊕ B_r)` par les labels des sommets actifs [Aud1 § 1.3], et la proposition « sommets protégés » [Aud2 § 2].

**Hypothèses.** Sauf mention contraire, aucun énoncé de cette note ne suppose la position générale ; tous valent sur la grille de 1 mm, plateaux cosphériques compris. Les tableaux de types `(v,u,g)`, les espérances de Poisson et les tests d'ordre « par type » supposent la position générale. Sur la grille, les groupes-plateaux sont plus gros : `μ` s'y calcule par recherche si le groupe est petit, sinon par le glouton croissant, sans garantie d'optimalité.

**Lemme 1 (centre unique ; les paires restent dans un groupe) [P].** Pour toute cellule `σ`, `d_k^2` a un unique minimiseur `p_σ` sur `F_σ`. Si `σ` est une face de `τ` et `a_σ = a_τ`, alors `p_σ = p_τ`. Aucune position générale n'est supposée.

*Preuve.* Sur `F_σ ⊆ V_Q` (pour tout label `Q` d'un sommet de `σ`), `d_k(y)^2 = max_{q ∈ Q} |y − q|^2`, maximum de fonctions strictement convexes, donc strictement convexe ; `F_σ` est un polyèdre convexe fermé non vide et la fonction est coercive : le minimum existe et il est unique. Si `σ ≤ τ`, alors `F_τ ⊆ F_σ`, et `p_τ ∈ F_σ` réalise `a_τ = a_σ = min_{F_σ} d_k^2`, d'où `p_τ = p_σ`. ∎

On appelle **groupe** `G(p) = {σ : p_σ = p}` : toutes ses cellules ont la naissance `d_k(p)^2`, et sa cellule maximale est la cellule duale à la face de Voronoï contenant `p` dans son intérieur relatif, `conv` des barycentres des k-ensembles réalisés en `p` (sphère `Σ_k(p)`, intérieur `I`, coquille `U`). En position générale, `G(p)` est exactement l'**intervalle relâché** d'Edelsbrunner–Nikitenko de type `(v, u, g)` [P-ext : Edelsbrunner–Nikitenko 2019, lemme 6] ; sur la grille, un groupe peut être plus gros (plateau cosphérique), sans changer l'énoncé. Conséquence pratique : **une paire de même naissance ne relie jamais deux groupes**, même quand deux groupes distincts ont le même rayon. L'oracle le contrôle sur toutes les paires candidates [V].

## 2. Le représentant causal : certifier toute la plage sans figer le sous-complexe

**Définition 1 (certificat d'appariement).** `M` est un ensemble de paires disjointes `(σ, τ)`, `σ` facette de `τ`, `a_σ = a_τ`, **acyclique** (aucun V-chemin fermé, au sens de Forman). Un ensemble `Π` de cellules protégées (au moins les sommets choisis) n'est dans aucune paire.

**Définition 2 (dates d'entrée).** `e` est la plus petite fonction à valeurs dans `[0, ∞]` telle que `e_σ ≤ a_σ` si `σ` n'est pas appariée, `e_σ ≤ e_τ` pour toute cofacette `τ` de `σ`, et `e_σ ≤ e_{M(σ)}` pour un partenaire. Explicitement, `e_σ` est le minimum de `a_ρ` sur les cellules libres `ρ` atteignables depuis `σ` par une chaîne dont chaque pas monte d'une facette à une cofacette ou passe au partenaire (∞ s'il n'y en a pas). On pose `L_r = {σ : e_σ ≤ r^2}` (**représentant causal**) et `L^fix = {σ : e_σ < ∞}`, `L^fix_r = L^fix ∩ A_r` (**certificat fixe** de [Aud1 § 4.2]).

**Théorème 2 (représentant causal) [P].**

1. `e_σ ≥ a_σ` ; `e` croît des faces vers les cofaces ; deux partenaires ont la même date ; une cellule libre a `e = a`.
2. Pour tout `r`, `L_r` est un sous-complexe de `A_r`, `A_r \ L_r` est la réunion disjointe des paires de `M` qu'il contient, et `A_r ↘ L_r` par effondrements élémentaires.
3. `r ≤ r'` implique `L_r ⊆ L_{r'}` ; les inclusions `L_r → A_r` commutent avec les inclusions en `r`. Elles induisent des isomorphismes de `π0` et d'homologie naturels en `r`, compatibles avec les applications `π0` de la tour FULL (par la chaîne `π0 Γ_K = π0 A_k` de [Aud1 § 0]).
4. **Causalité et localité.** `e_σ ≤ r^2` si et seulement s'il existe une chaîne de `σ` vers une cellule libre `ρ` avec `a_ρ ≤ r^2` ; toutes les cellules de cette chaîne sont dans `A_r` et dans la même composante que `σ`. Donc `L_r ∩ A_{r,v}` ne dépend que de `A_{r,v}` et des paires de `M` qui y sont contenues.
5. `L_r ⊆ L^fix_r`, avec égalité dès que `r^2 ≥ max{e_σ < ∞}` (fin de la plage déclarée).
6. Si tous les sommets sont protégés, la proposition de [Aud2 § 2] s'applique telle quelle à `L_r` : `d_H(L_{r,v}, A_{r,v}) ≤ D_v(r)`, `d_H(L_{r,v}, C_v(r)) ≤ r`, couverture exacte par les labels des sommets de `L_{r,v}`.

*Preuve.* (1) Le long d'une chaîne, les naissances ne décroissent pas (facette → cofacette) ou restent égales (partenaires), d'où `a_σ ≤ a_ρ` pour toute cellule libre atteinte, et `e_σ ≥ a_σ`. Une chaîne partant de `σ` se prolonge vers le bas à partir de toute face de `σ` : les faces ont une date inférieure. Les chaînes de `σ` et de `M(σ)` diffèrent d'un pas de partenaire. Une cellule libre se prend elle-même. (2) `L_r ⊆ A_r` par (1), et c'est un sous-complexe par la monotonie. Si `σ ∈ A_r \ L_r`, `σ` est appariée (sinon `e_σ = a_σ ≤ r^2`), son partenaire a la même date `> r^2` et la même naissance `≤ r^2` : il est aussi dans `A_r \ L_r`. La restriction de `M` à ces paires est acyclique ; par le théorème d'effondrement par gradient [P-ext : Bauer–Edelsbrunner 2017, th. 2.1 ; Forman 1998, th. 3.3 pour les complexes cellulaires réguliers], `A_r ↘ L_r`. (3) `L_r` est un sous-niveau de `e`. Les inclusions sont des équivalences d'homotopie (effondrements), et elles commutent strictement. (4) C'est la définition de `e` comme minimum sur les chaînes. Deux cellules consécutives d'une chaîne sont en relation de face, donc dans la même composante de `A_r`. (5) et (6) sont immédiats : la preuve de [Aud2 § 2] n'utilise que `L_r ⊆ A_r`, la présence dans `L_r` de tous les sommets de `A_r` (protégés, donc `e = a`) et la correspondance des composantes. ∎

**Remarques.**

- **Ce que la causalité résout.** Le contre-exemple `A = (0,6), B = (4,6), C = (2,7), D = (2,0)` de [Aud2 § 3] n'invalide que le certificat fixe. Le causal utilise `(AB, ABC)` sur `[25/4, 100/9)`, puis les fait entrer ensemble dans `L` à `100/9`. Tailles exactes `(sommets, arêtes, triangles)` : à `25/4`, `A = (4,3,1)` et `L = (4,2,0)` ; à `10`, `A = (4,5,1)` et `L = (4,4,0)` [V]. Les réductions **nœud par nœud** suffisent pour le causal (point 4), à condition de décider chaque paire une fois, à sa naissance, et de propager les entrées en ligne. Elles ne suffisent pas pour le fixe.
- **Emboîtement vers le parent.** À la mort `d_v`, `L_{d_v} ⊇ L_{d_v^−}` composante par composante : le représentant du parent contient ceux de ses enfants, sans hypothèse supplémentaire.
- **Stockage.** La famille entière se stocke par une date par cellule de `L^fix` (plus les paires et leurs dates de dissolution). Le gain du causal est **par niveau** (taille de `L_r`) ; en stockage total, il n'existe que jusqu'à une borne d'échelle déclarée `R_max` (la racine l'exige déjà [Aud2 § 5]).
- **Certificat de profondeur 1 (le plus local).** Utiliser chaque paire seulement sur `[a_σ, ē)`, où `ē` est la plus petite naissance d'une autre coface de `σ` ou de `τ`. C'est valide : on vérifie les trois conditions du point 2, et une face d'une cellule de paire voit cette cellule dans son étoile. Cela ne demande que les étoiles de `σ` et `τ`, mais réduit moins. Sur l'anneau bruité à `k = 1` et au niveau 89 : 42 cellules dans `A`, 24 dans le causal, 36 en profondeur 1, 42 dans le fixe [V].
- **Les niveaux d'un nœud.** Les dates `e` donnent toute la famille `L_{r,v}`, `r ∈ [b_v, d_v)`. La coupe de fin de vie `L_v(d_v^−) = {σ de la composante : e_σ < d_v^2}` est un état combinatoire atteint avant la mort, comme `A_v(d_v^−)` dans [Aud2 § 5]. Une coupe intérieure `L_{r*,v}` publie ses marges `r* − b_v` et `d_v − r*`. Aucun maillage intermédiaire n'est dupliqué : une date par cellule suffit.
- **Robustesse héritée.** Comme `L_r ≃ A_r ≃ Ω_k(r)` naturellement, la réduction hérite de l'entrelacement à homotopie près sous déplacement apparié, et des décalages d'ordre sous ajouts ou retraits [Aud2 § 1]. Rien de plus : l'appariement de `P'` peut différer de celui de `P`, et le saut géométrique du triangle `T` de [Aud2 § 1.1] demeure possible entre deux nuages proches. Les bornes `D_v(r)` ou `θ r` sont des garanties **par nuage**, pas des stabilités du dessin.

**Corollaire 3 (cas `K = 1`) [P].** À `k = 1`, chaque sommet naît à 0 et ses cofaces après : tous les sommets sont critiques, la protection est gratuite. En position générale, les groupes sont les intervalles `[L, U]` de la fonction rayon de Delaunay ; si `M` raffine ce gradient en paires, `L_r ⊆ Wrap_r(P)` (Bauer–Edelsbrunner 2017, § 4.5, éq. (26)), avec égalité quand les intervalles non singuliers sont des paires (toujours dans le plan). En effet, une chaîne de `σ` vers une cellule critique `ρ` est un chemin du graphe des intervalles de l'intervalle de `σ` vers `[ρ, ρ]`. Le Wrap filtré est donc, à `k = 1`, le représentant causal ; la proposition ci-dessus l'étend à tout `k` **sans supposer** que la fonction rayon d'ordre `k` est une fonction de Morse discrète généralisée (ce que [Aud1 § 4.1] interdit de supposer) : l'appariement est quelconque, et vérifié.

## 3. Q1 — Combien la réduction à sommets protégés peut-elle réduire ?

**Proposition 4 (bornes inférieures, tout certificat) [P].** Soit `Π` contenant tous les sommets. Pour tout `r` :

1. `|L_r| ≥ Σ_{G ⊆ A_r} μ(G)`, où `μ(G)` est le nombre minimal de cellules de `G` non appariées par un appariement acyclique de paires facette/cofacette internes à `G`, dont les cellules non appariées sont fermées par faces dans `G` et contiennent les sommets de `G`.
2. `|L_r| ≥ V_r + (V_r − β_0) + β_1 + β_2`, où `V_r` est le nombre de sommets de `A_r` et `β_i` les nombres de Betti de `A_r` ; en particulier au moins `V_r − β_0` arêtes restent.
3. Un groupe singulier (une seule cellule) est toujours gardé : en position générale, tout sommet critique (naissance d'une composante) et toute cellule critique singulière.

*Preuve.* (1) Par le lemme 1, `L_r ∩ G` est l'ensemble des cellules non appariées de la restriction de `M` aux paires internes à `G \ L_r`, laquelle est acyclique ; `L_r ∩ G` est fermé par faces dans `G` (sous-complexe) et contient les sommets. Somme sur les groupes. (2) Inégalités de Morse fortes pour le complexe cellulaire `L_r ≃ A_r` à `c_0 = V_r` cellules de dimension 0 : `c_1 − c_0 ≥ β_1 − β_0`, puis `c_2 − c_1 + c_0 ≥ β_2 − β_1 + β_0`. (3) Un groupe à une cellule n'a aucune paire interne. ∎

Le **1-squelette ne reste donc pas entier** : une arête appariée à une 2-cellule de même naissance part, même avec tous les sommets. Il en reste au moins une forêt couvrante.

**Combinatoire exacte des groupes en position générale [V-exh].** Pour chacun des 7 types `(v, u, g)` du plan et des 17 de l'espace (lemme 6 d'Edelsbrunner–Nikitenko), `poisson.py` énumère les cellules de l'intervalle et calcule `μ` par recherche exhaustive, avec et sans protection :

| Type `(v,u,g)` | Cellules `(f0,f1,f2,f3)` | `μ` protégés | `μ` libres | Lecture |
| --- | --- | --- | --- | --- |
| `(u,u,u+1)` | `(1,0,0,0)` | 1 | 1 | sommet critique (naissance) |
| `(1,1,1)`, `(2,2,1)`, `(3,3,1)` | une cellule | 1 | 1 | arête, triangle, tétraèdre critiques singuliers |
| `(2,2,2)` | `(0,3,1,0)` | 2 | 2 | triangle critique et ses 3 arêtes (exemple équilatéral de la thèse) |
| `(3,3,2)` | `(0,0,4,1)` | 3 | 3 | octaèdre critique |
| `(3,3,3)` | `(0,6,4,1)` | 3 | 3 | tétraèdre critique de génération 3 |
| `(1,2,1)`, `(1,3,1)`, `(1,3,3)`, `(2,3,1)` | paire ou `(0,1,2,1)` | 0 | 0 | non critique, sans sommet |
| `(2,3,2)` | `(0,3,4,1)` | 0 | 0 | octaèdre non critique, sans sommet |
| `(1,2,2)` | `(1,2,1,0)` | 2 | 0 | non critique, avec un sommet |
| `(1,3,2)` | `(1,4,4,1)` | 2 | 0 | octaèdre non critique, avec un sommet |
| `(2,3,3)` | `(1,3,3,1)` | 2 | 0 | non critique, avec un sommet |

**Proposition 5 (ordres de grandeur, Poisson, position générale).** Les espérances par point des intervalles de type `(v,u,g)` valent `Γ(u+k−g)/((k−g)! Γ(u)) · C_{v,u}` (Edelsbrunner–Nikitenko 2019, éq. (14)). Le produit par les tableaux ci-dessus donne la borne de la proposition 4.1, par point :

| | `k` | cellules de `Del_k` `(f0,f1,f2[,f3])` | total | gardées au mieux, sommets protégés | sans protection | borne de Morse `2V` |
| --- | --- | --- | --- | --- | --- | --- |
| Plan [P] | 1 | `(1, 3, 2)` | 6 | 4 (67 %) | 4 | 33 % |
| Plan [P] | 2 | `(3, 9, 6)` | 18 | 10 (56 %) | 8 | 33 % |
| Plan [P] | 5 | `(9, 27, 18)` | 54 | 28 (52 %) | 20 | 33 % |
| Plan [P] | `k` | `(2k−1)·(1,3,2)` | `12k−6` | `6k−2` | `4k` | `1/3` |
| Espace [M] | 1 | `(1, 7.77, 13.53, 6.76)` | 29.1 | 11.6 (40 %) | 11.6 | 7 % |
| Espace [M] | 2 | `(7.8, 48.3, 67.6, 27.0)` | 150.8 | 45.8 (30 %) | 38.2 | 10 % |
| Espace [M] | 5 | `(68.6, 413.5, 554.5, 209.6)` | 1 246 | 295 (24 %) | 206 | 11 % |
| Espace [M] | 10 | `(305, 1 834, 2 448, 920)` | 5 506 | 1 201 (22 %) | 778 | 11 % |

Statuts. Dans le plan, les constantes `C_{11} = 2`, `C_{12} = C_{22} = 1` (deux triangles par point, dont la moitié aigus, selon Miles 1970 ; mêmes valeurs chez Edelsbrunner–Nikitenko–Reitzner 2017) donnent un résultat [P] modulo ces constantes externes. Contrôle indépendant : la densité `2(i+1)` des cercles à `i` points intérieurs redonne `V = 2k−1` et `F = 2(2k−1)` par la relation d'Euler. Dans l'espace, les six constantes `C_{v,u}` sont **mesurées** par Monte-Carlo (Delaunay flottant de 50 000 points, 17 269 points intérieurs) : `C_{33} = 1.830`, `C_{23} = 3.709`, `C_{13} = 1.2225`, `C_{22} = 4.821`, `C_{12} = 2.552`, `C_{11} = 3.991`. Contrôles : 6.762 tétraèdres par point contre `24π^2/35 = 6.768` ; Euler des cellules critiques `1 − C_{11} + C_{22} − C_{33} = 0.0002`. Une seconde graine donne `C_{33} = 1.859`, `C_{11} = 4.001`, 6.769 tétraèdres par point. Les cellules gardées passent alors à 297.6 sur 1 247.5 à `k = 5` (23.9 % au lieu de 23.7 %) : l'incertitude d'échantillonnage est de l'ordre du pour cent (`resultats/poisson_graine2.json`).

Lecture :

- **Ce qui reste de dimension 3.** Seuls les tétraèdres critiques singuliers `(3,3,1)` restent, ceux qui bouchent une bulle : `C_{33} k(k+1)/2` par point, soit 27.5 à `k = 5` sur 209.6 (13 %). Le reste du solide s'effondre en membranes et en arêtes. À `k = 5`, le représentant réduit a pour profil `(68.6, 120.2, 79.0, 27.5)` par point : une **mousse** homotope au solide, à distance `≤ D_v(r)` de lui, pas un solide. Pour le dessin de faces exposées [Aud1 § 4.4], c'est `A` (ou l'ombre) qu'il faut montrer ; `L` est un modèle topologique compact, pas un meilleur dessin.
- **Plancher des sommets.** Avec tous les sommets protégés, la taille ne descend pas sous `2V − β_0` : `2(2k−1)` par point dans le plan, 137 par point à `k = 5` et 610 à `k = 10` dans l'espace. Les sommets de `Del_k` sont `Θ(k^2)` par point en volume (68.6 à `k = 5`, dont 23.9 sommets critiques, c'est-à-dire des naissances).
- **Pertinence pour le LiDAR.** Le workflow précédent a mesuré sur trames, à `k = 5`, 166 à 172 3-cellules et environ 1 000 à 1 200 faces par point. C'est l'ordre du modèle **volumique** (209.6 et 1 246) plutôt que plan : le bruit et la quantification donnent des cellules 3D partout. Attendre au mieux environ un quart des cellules après réduction, soit quelque 250 à 300 cellules par point à `k = 5`. [C] : extrapolation, non mesurée sur trame.
- **Au pire, rien.** (a) Douze points cocycliques de la grille, `(±5,0), (0,±5), (±3,±4), (±4,±3)` : à `k = 1, 2, 3`, la mosaïque est un seul 12-gone `(12, 12, 1)` ; tous les groupes sont singuliers, aucune paire [V]. (b) À `k = 1`, une triangulation dont tous les triangles sont aigus et toutes les arêtes de Gabriel n'a que des intervalles singuliers [P, par le corollaire 3]. (c) Même avec des paires, le certificat fixe peut tout garder : c'est F2 (§ 7).

**Mesures bornées [V], 12 nuages aléatoires du plan × 5 ordres** (`n = 12` et 14, grille 40 × 40, `k = 1..5`, tous sommets protégés, deux priorités). Les trois certificats sont vérifiés à chaque niveau, effondrement effectif compris, sans aucun échec. Rapport moyen `|L_r|/|A_r|` sur les niveaux d'événement :

| `k` | cellules (moy.) | borne locale / total | causal | profondeur 1 | fixe |
| --- | --- | --- | --- | --- | --- |
| 1 | 58.7 | 0.61 | 0.88 | 0.91 | 0.91 |
| 2 | 146.8 | 0.52 | 0.78 | 0.84 | 0.87 |
| 5 | 247.7 | 0.48 | 0.73 | 0.83 | 0.84 |

Sur ces petits nuages dominés par le bord, l'écart entre la borne locale et le causal vient du forçage (fermeture des cellules critiques ultérieures). L'ordre attendu, causal < profondeur 1 < fixe, est observé en moyenne. Ce sont des oracles, **pas une pente**.

## 4. Q2 — Protéger moins que tous les sommets

Un sommet critique forme un groupe singulier (proposition 4.3) : il ne peut jamais être apparié. Les seuls candidats sont les **sommets non critiques**, dont le groupe contient des arêtes incidentes de même naissance (types `(1,2,2)`, `(1,3,2)`, `(2,3,3)`). Dans le modèle de Poisson, ils représentent `k − 1` des `2k − 1` sommets par point dans le plan et 44.7 des 68.6 dans l'espace à `k = 5`.

**Définition 3.** Un sommet `u` est **retiré** s'il est apparié à une arête `e_u = [u, w_u]` de même naissance. Le **registre** associe à `w_u` les labels de `u` avec la date `a_u`. L'**ombre par registre** est `S^L_v(r) = ⋃_{σ ∈ L_{r,v}} conv(⋃ lab(sommets de σ)) ∪ ⋃_{u retiré, a_u ≤ r^2} conv(lab(u) ∪ lab(w_u))`.

**Théorème 6 (retrait indépendant) [P].** Supposons les sommets retirés deux à deux **non adjacents** dans le 1-squelette de `M` (en particulier `w_u` est gardé). Alors, pour tout `r` et toute composante `v` :

1. `d_H(L_{r,v}, A_{r,v}) ≤ D_v(r)` ;
2. `d_H(L_{r,v}, C_v(r)) ≤ r + λ_v(r)`, où `λ_v(r) = max |e_u|` sur les sommets retirés de `A_{r,v}`. On a `λ_v(r) ≤ D_v(r)`, et `|e_u| ≤ 2√(a_u)/k ≤ 2r/k` lorsque les deux labels de `e_u` diffèrent d'un site (toujours en position générale) ;
3. `P ∩ (C_v(r) ⊕ B_r)` est la réunion des labels du registre portés par les sommets de `L_{r,v}` avec une date `≤ r^2` (couverture exacte) ;
4. `S^L_v(r) ⊆ C_v(r) ⊕ B_r` et `P ∩ S^L_v(r) = P ∩ (C_v(r) ⊕ B_r)`.

*Preuve.* (1) `L ⊆ A` donne un sens. Soit `y` dans une cellule `σ` de `A_{r,v}`. Si `dim σ ≥ 1`, `σ` contient une arête, dont une extrémité au moins est gardée ; ce sommet est libre, donc dans `L_r` (`e = a ≤ a_σ`), dans la même composante, à distance `≤ diam σ ≤ D_v(r)`. Si `σ = {u}` est retiré, `e_u ∈ A_r` (même naissance que `u`), et `w_u` est gardé à distance `|e_u| ≤ D_v(r)`. (2) Pour `y ∈ C_v(r)`, [Aud1 § 2] fournit un sommet actif `c_Q` de la composante avec `|y − c_Q| ≤ r`. S'il est gardé, on conclut ; sinon `w_{c_Q}` est à `|e_{c_Q}|` de plus. Si `Q' = Q − q + q'`, alors `|c_Q − c_{Q'}| = |q − q'|/k`, et `q, q'` sont dans la boule témoin de `e_u`, de rayon `√(a_u)`. Le sens `L ⊆ A ⊆ C ⊕ B_r` est [Aud1 § 2]. (3) Si `a_u ≤ r^2`, `e_u` relie `u` à `w_u` dans `A_r`, et `w_u ∈ L_r` est dans la composante correspondante (théorème 2.3). Les labels du registre sont donc exactement ceux des sommets actifs de `A_{r,v}`, et [Aud1 § 1.3] conclut. (4) Pour `σ ∈ L_{r,v} ⊆ A_{r,v}`, l'argument de l'ombre [Aud2 § 4] s'applique avec les labels propres de `σ`. Pour un retrait, `lab(u) ∪ lab(w_u) ⊆ B(z, √(a_u))` avec `z` témoin de `e_u` dans `C_v(r)`, et la boule est convexe. La trace contient tous les labels actifs par (3). ∎

**Proposition 6' (chaînes de témoins et budget relatif) [P].** Sans indépendance, l'acyclicité de `M` interdit tout cycle `u → w_u → …`, car ce serait un V-chemin fermé. La chaîne des témoins de `u` aboutit donc à un sommet gardé ; ses arêtes ont des naissances `≤ a_u`, elles sont donc dans `A_r`. Avec `δ(u)` la somme des longueurs de la chaîne : `d_H(L_{r,v}, A_{r,v}) ≤ D_v(r) + max δ` et `d_H(L_{r,v}, C_v(r)) ≤ r + max δ`. Refuser un retrait tant que `δ(u) > θ √(a_u)` garantit `+ θ r`. La cascade de [Aud2 § 2] (erreur `Nℓ`) est exactement le cas `δ` non borné.

**[R] La borne `d_H(L_{r,v}, C_v(r)) ≤ r` ne survit pas au retrait, même indépendant.** C'est le contre-exemple exact F1 (§ 7), avec `k = 2` : la distance carrée `24649/340` dépasse `r^2 = 289/4`. La borne démontrée `r + λ` est la bonne forme.

**Gain.** Au mieux, de `6k − 2` à `4k` cellules par point dans le plan (Poisson) et de 295 à 206 par point dans l'espace à `k = 5`, si tous les sommets non critiques pouvaient partir. L'indépendance en retient une partie. Sur les six points de la thèse à `k = 3` : 21 cellules (protection complète), 19 (retrait indépendant), 13 (sans protection, sans garantie) [V].

## 5. Q3 — Ordre des paires

**Proposition 7 (budget par paire, local) [P].** Supposons les sommets protégés, et n'acceptons une paire `(σ, τ)` que si `diam τ ≤ θ √(a_τ)`. Alors `d_H(L_{r,v}, A_{r,v}) ≤ θ r` pour tout `r`, et plus précisément `≤ max{diam σ : σ ∈ A_{r,v} \ L_r}`.

*Preuve.* Un point `y` de `A_{r,v}` est dans l'intérieur relatif d'une cellule `σ`. Si `σ ∈ L_r`, la distance est nulle. Sinon `σ` est dans une paire dont la grande cellule a un diamètre `≤ θ √(a_σ) ≤ θ r`, et les sommets de `σ` sont dans `L_r`. ∎

En position générale, `θ = 4/k` n'interdit rien [Aud2 § 2]. Un `θ` plus petit échange de la taille contre de la fidélité, et ce contrôle se vérifie paire par paire. Mesure bornée à `k = 2`, priorité croissante, tous les certificats vérifiés (`budget_theta.py`) :

- sur l'anneau bruité, les six points et la chaîne, `θ = 1` n'interdit aucune paire ;
- le rapport moyen `|L_r|/|A_r|` vaut alors 0.70, 0.76 et 0.78 ;
- le maximum mesuré de `d_H(L_{r,v}, A_{r,v})/r` vaut alors 0.36, 0.38 et 0.39 [M] ;
- `θ = 1/2` supprime toutes les paires, sauf 5 sur la chaîne (rapport 0.97, erreur 0.04 r).

À `k = 2`, le compromis est donc abrupt ; aux ordres plus élevés, les cellules sont `k` fois plus petites et la règle devient peu contraignante. Non mesuré à l'échelle.

**Propriétés démontrables de l'ordre.**

- **Indépendance (validité).** Homotopie à chaque niveau, emboîtement, `π0`, couverture et bornes du théorème 2.6 valent pour **tout** appariement acyclique [P], quel que soit l'ordre ou la priorité (extérieur d'abord, distance à la mesure, distance aux sites). Ces priorités ne font que choisir parmi les paires admissibles d'un groupe ; elles ne prouvent aucune minimalité [Aud2 § 3].
- **Minimalité locale [V-exh].** En position générale, la taille est bornée par `Σ μ(G)` (proposition 4). `ordres_exhaustifs.py` énumère, pour chaque type abstrait, tous les ordres des ex aequo dans chaque classe de dimension (départage arbitraire), puis applique le glouton avec rejet des cycles et la fermeture forcée :

| Type (espace) | `μ` | Dimension décroissante, départage arbitraire | Dimension croissante, départage arbitraire |
| --- | --- | --- | --- |
| `(2,3,2)`, protégés ou non | 0 | 6 cellules gardées dans 6 912 ordres sur 17 280 | 0 dans les 17 280 ordres |
| `(1,3,2)`, sommets protégés | 2 | 8 cellules dans 44 800 ordres sur 400 000 | 2 dans les 400 000 ordres testés (sur 967 680) |
| `(1,3,2)`, sans protection | 0 | 8 dans 25 600 ordres sur 400 000 | 6 dans 67 200 ordres sur 400 000 |
| `(3,3,3)` | 3 | 3 sur 20 000 ordres tirés (sur `1.15·10^10`) | 3 sur 20 000 ordres tirés |
| les 20 autres types (plan et espace) | `μ` | `μ` pour tout ordre (exhaustif) | `μ` pour tout ordre (exhaustif) |

Pourquoi la dimension décroissante échoue. Dans le groupe `(2,3,2)`, notons `T_0` le triangle médian de la facette visible `V = {y,z,w}`, `T_y` les triangles `({y}, {z,w,x})` et `e_y` leurs arêtes communes avec `T_0`. Apparier l'octaèdre à `T_y`, puis `e_z` à `T_0`, laisse `e_y` et `T_z` sans partenaire. La fermeture force alors `e_z`, `T_0`, puis `e_w`, `T_w` : six cellules.

**Réalisation géométrique exacte [V].** `octaedre3d.py` construit en `Fraction` la mosaïque d'ordre 2 de quatre sites entiers (un octaèdre et ses 26 faces), avec les naissances et les centres exacts. Sur 3 000 tirages, il obtient 1 503 groupes `(2,3,2)` et 1 244 groupes `(1,3,2)`. Le départage complet de l'auditeur (`−dim τ`, puis `diam² τ`, puis labels triés) atteint `μ` dans **tous** ces cas, et la dimension croissante aussi. Un ordre décroissant adversarial existe pour chaque configuration, car la structure du groupe ne dépend que de son type ; il a été exhibé et vérifié sur une configuration de chaque type (fixtures F3 et F4 du § 7). **Conjecture [C] :** le départage par petit diamètre suffit sur ces deux types. Faute de preuve, la recommandation est « dimension croissante » (robuste à tout départage), ou l'**optimum exact par groupe**. En position générale, un groupe a au plus 11 cellules (type `(3,3,3)`) et appartient à l'un des 17 types : l'appariement optimal se tabule une fois par type. Sur les plateaux de la grille, les groupes peuvent être gros ; le problème général d'appariement de Morse optimal est NP-difficile (Joswig–Pfetsch 2006, [P-ext]). On y applique le glouton croissant en publiant l'écart à la borne de Morse. Dans le plan, les deux priorités donnent des tailles identiques sur les 60 cas [V].

- **Ce que la priorité géométrique peut encore faire.** À taille égale, elle choisit **quelles** cellules restent : par exemple garder les deux arêtes les plus courtes d'un groupe `(2,2,2)`, ou refuser les grandes paires (proposition 7). Aucune optimalité géométrique n'est démontrée ici.
- **Toute la plage, nœud par nœud.** Le certificat fixe exige de connaître l'avenir (cofaces futures) ; le causal non (théorème 2.4). C'est la forme qui rend compatibles les réductions par nœud et l'emboîtement vers le parent.

## 6. Q4 — Calcul local depuis FULL

**Proposition 8 (données suffisantes pour un nœud) [P].** Soit `P_v(r) = P ∩ (C_v(r) ⊕ B_r)`, la couverture du nœud, donnée par les labels. Toute cellule de `A_{r,v}` est une face de la cellule maximale `cell(p)` d'un groupe dont le centre `p` vérifie `p ∈ C_v(r)`, `d_k(p) ≤ r` et `I(p) ∪ U(p) ⊆ B(p, r) ∩ P ⊆ P_v(r)`. Donc `A_{r,v}` se construit à partir de `P_v(r)` seul, plus un **test ambiant** de chaque sphère candidate : aucun site de `P` strictement dedans hors de `I`. Ce test reprend la mise en garde de [Aud1 § 7.1] contre les concurrents de Voronoï. Il est exact sur la grille en entiers (centre rationnel, distances carrées comparées sans arrondi), sans simulation de simplicité.

*Preuve.* `σ ∈ A_{r,v}` a son centre `p_σ` (lemme 1) dans `F_σ ∩ Ω_k(r)` et dans `C_v(r)`, puisque son témoin y est. La sphère `Σ_k(p_σ)` a un rayon `√(a_σ) ≤ r`, et `σ` est une face de `cell(p_σ)`, de même naissance. Les sites intérieurs ou sur la sphère sont à distance `≤ r` de `p_σ ∈ C_v(r)`. ∎

**Proposition 9 (certificat d'une paire) [P].** L'acyclicité et l'égalité de naissance d'une paire se décident **dans son groupe** (lemme 1), c'est-à-dire dans une seule sphère `(p, I, U)` : c'est l'information d'une boule du recensement FULL (`I_B`, `U_B`). Sa date de dissolution causale demande en revanche les chaînes de forçage, contenues dans la composante (théorème 2.4) mais **sans rayon de localité borné**. Sur l'anneau bruité, un seul triangle critique force tout le disque : 43 cellules sur 43 à la fin. Le calcul se fait en ligne, pendant le balayage croissant en `r` qui construit `A` : à chaque cellule libre qui naît, un parcours descendant (faces et partenaires vers le haut) s'arrête aux cellules déjà dans `L`. Chaque cellule entre une fois, d'où un coût total en `O(Σ_σ |cofaces(σ)|)`. La certification sur toute une plage `[0, R]` (certificat fixe) exige davantage. Il faut d'abord les étoiles jusqu'au niveau `R`, donc les sites à distance `≤ 2R` des labels de la paire (`Q ⊆ B(p_τ, R)` pour toute coface `τ`). Il faut surtout tout le forçage par les cellules critiques de naissance `≤ R^2`, c'est-à-dire en pratique toute la composante de l'ancêtre à `R`. Le certificat de profondeur 1 (§ 2) est le seul à voisinage borné. Il lui suffit de la plus petite naissance `ē` d'une autre coface de `σ` ou de `τ`. Pour l'obtenir, on énumère les sphères centrées sur `F_σ` (puis `F_τ`) par rayon croissant et on s'arrête à la première. Les sites lus sont ceux de `B(p, √ē)` pour ces centres : le voisinage est borné par la date même qu'on certifie.

**Ce que FULL fournit et ne fournit pas.**

- **Ce qu'il fournit :** l'identité des nœuds, le chemin « MEB de Q → descente datée → ancêtre à r » qui rattache chaque label local à son nœud (accepté par [Aud1 § 7.3], à qualifier), les naissances et fusions, et la couverture `P_v(r)`. Une cellule `cell(p)` se rattache par n'importe lequel de ses labels : tous sont réalisés en `p ∈ C_v(r)`.
- **Ce qu'il ne fournit pas :**
  - les sphères **non critiques pour `H0`** : FULL ne publie que les naissances et les fusions. Dans le modèle de Poisson volumique à `k = 5`, cela fait au plus `2 × 23.9 ≈ 48` groupes par point sur 304 (≤ 16 %) ; sur trames, les cellules publiées ne font que 0,3 à 0,8 % des 3-cellules (workflow précédent) ;
  - les cellules critiques de `H1` et de `H2`, qui commandent le forçage.

  Il faut donc une construction locale de `Del_k` sur `P_v(r)`, en aval et bornée, pour respecter l'invariant d'architecture.

**Coût.** Il est de l'ordre de la mosaïque locale : environ 1 250 cellules par point à `k = 5` et 5 500 à `k = 10` dans le modèle de Poisson volumique ; environ 1 000 à 1 200 faces par point mesurées sur trames à `k = 5` [M, workflow précédent]. S'y ajoute le vérificateur, linéaire en incidences. Pour une trame de 40 000 sites, cela fait de l'ordre de `5·10^7` cellules : **hors du budget de 100 ms par trame**. C'est envisageable par nœud sélectionné (jeton), en aval. Non mesuré à l'échelle.

## 7. Exemples et contre-exemples exacts

**Six points de la thèse, § 6.1** (corps `Q(√3)`, côté 2, soit `r = 1` dans la notation de la thèse) : `A = (−1−√3, 1)`, `B = (−1−√3, −1)`, `C = (−1, 0)`, `D = (1, 0)`, `E = (1+√3, 1)`, `F = (1+√3, −1)` [V].

- `k = 1` : `Del` a pour profil `(6, 9, 4)`. Ses groupes : 6 sommets, 7 arêtes singulières, 2 paires `{arête, triangle obtus}` et 2 triangles équilatéraux critiques. Le représentant final a 15 cellules sur 19, égal à la borne locale.
- `k = 2` : `Del_2` a pour profil `(9, 14, 6)`. On retrouve d'abord 7 sommets critiques de naissance `r^2 = 1` : ce sont les sept segments de la figure 6.2. Viennent ensuite deux groupes `(2,2,2)` de naissance `4/3`, c'est-à-dire `r_0 = 2√3/3` de la figure 6.3. À `r^2 = 4/3`, les **trois composantes** sont `{A,B,C}`, `{C,D}`, `{D,E,F}`, comme dans la figure 6.5 de la thèse ; `π0` coïncide avec `Γ_2` à chaque niveau. Le représentant passe de 7 à 5 cellules par triangle (3 sommets, 2 arêtes). En fin de plage : 21 cellules sur 29, égal à la borne locale ; 17 sans protection.
- `k = 3` : `(10, 18, 9)`, soit 37 cellules ; on en garde 21 (protection complète), 19 (retrait indépendant), 13 (sans protection).

**Anneaux.**

- **Cocyclique** (12 points de rayon 5) : aucune paire à `k = 1, 2, 3`. C'est le pire cas.
- **Bruité** : `(10,0), (9,5), (5,9), (0,10), (−5,8), (−9,4), (−10,0), (−8,−5), (−4,−9), (0,−10), (5,−8), (8,−6)`.
  - À `k = 1`, juste avant que le trou ne se ferme (niveau 89), le causal est **l'anneau polygonal seul** `(12, 12, 0)`, contre `(12, 21, 9)` pour `A`. La distance mesurée vaut `d_H(L, A) = 9.19 ≤ D_v = 18.87`.
  - À `k = 2`, au niveau `1513/18` : `A = (17, 27, 10)`, causal `(17, 17, 0)`, fixe `(17, 27, 10)`.

  La forme réduite garde le trou ouvert jusqu'à sa mort exacte, puis le remplit d'un coup par la **membrane** descendante du triangle critique.

**F1 — retrait de sommets [R pour « `≤ r` sans protection complète »].** Sites `(0,0), (4,1), (8,0), (12,1), (16,0), (20,1), (24,0), (2,9), (22,9), (12,12)`, `k = 2`, niveau `r^2 = 289/4`.

- Retrait indépendant de 6 sommets par l'appariement glouton de `reduction.py` (priorité auditeur), certificat vérifié.
- Le point `y = (−11/2, 6)` vérifie `d_2(y)^2 = 265/4 ≤ 289/4`, donc `y ∈ Ω_2(r)`, et `dist(y, L_r)^2 = 24649/340 > 289/4`.
- Avec tous les sommets protégés, le maximum sur la même grille vaut `225/4 ≤ 289/4`.

Le contre-exemple dépend de l'appariement choisi ; il est rejouable par `contre_exemples.py`.

**F2 — certificat fixe inutile [V].** Avec l'anneau bruité à `k = 1`, `L^fix` contient les 43 cellules sur 43 : aucune réduction à aucun niveau. Le causal donne 24 cellules sur 42 au niveau 89.

**F3 — octaèdre `(2,3,2)` [V-exh, combinatoire, réalisé exactement].** Sites `(4,3,6), (−4,−5,1), (−2,−4,−5), (2,6,5)`, `k = 2`.

- L'octaèdre naît à `13853/256`, de centre `(5/16, 7/8, 0)` ; son groupe a pour profil `(0,3,4,1)`, avec `μ = 0`.
- Ordre décroissant adversarial : `({03,13,23}, octaèdre)`, puis `([02,23], {02,12,23})`, puis `([02,03], {02,03,23})`. Il garde **6** cellules du groupe.
- Les priorités « auditeur » et « croissante » en gardent 0.

**F4 — octaèdre `(1,3,2)`, avec sommet non critique [idem].** Sites `(1,2,6), (1,1,2), (3,−3,−4), (6,2,1)`, `k = 2`, naissance `773/4`, centre `(7, −21/2, 7)`, profil du groupe `(1,4,4,1)`, `μ = 2` (sommets protégés). Un ordre décroissant adversarial garde **8** cellules ; « auditeur » et « croissante » en gardent 2. Données dans `resultats/octaedre3d.json`.

**Triangle `T = {(−4,0), (4,0), (1,2)}`, `k = 2`** (témoin de [Aud2 § 1.1]). Le sommet `{(−4,0),(4,0)}` n'est pas critique : il naît à `377/16` avec deux arêtes et le triangle. Avec protection, on garde 5 cellules sur 7 ; sans protection, 3. La distance `d_H(L, A)` vaut 0.60 avec protection et 1.0 après retrait, contre `D_v = 4`.

## 8. Énoncés numérotés et statut

| # | Énoncé | Statut |
| --- | --- | --- |
| 1 | Centre unique ; une paire de même naissance reste dans un groupe | [P] |
| 2 | Représentant causal : `A_r ↘ L_r` à tout niveau, emboîté, causal, local à la composante ; `L_r ⊆ L^fix_r` | [P] + [V] |
| 3 | `k = 1` : `L_r ⊆ Wrap_r`, égalité si les intervalles sont des paires | [P] |
| 4 | Bornes inférieures : groupes, Morse, singuliers | [P] |
| 5 | Ordres de grandeur de Poisson | plan [P] modulo constantes externes ; espace [M] |
| 6 | Retrait indépendant : `D_v`, `r + λ`, registre et ombre exacts | [P] |
| 6' | Chaînes de témoins : `+ max δ`, règle `θ` | [P] |
| F1 | `d_H(L, C) ≤ r` sans protection complète | [R] |
| 7 | Budget par paire `diam τ ≤ θ √a` ⇒ `d_H(L, A) ≤ θ r` | [P] |
| 8–9 | Données locales suffisantes ; une paire se certifie dans son groupe ; date causale non localisable en rayon | [P] |
| O | « Dimension croissante » atteint `μ` sur tous les types de position générale, sommets protégés, pour tout départage | [V-exh] (`(1,3,2)` : 400 000 ordres sur 967 680 ; `(3,3,3)` : 20 000 tirés) |
| O' | « Dimension décroissante » à départage arbitraire n'atteint pas `μ` sur `(2,3,2)` et `(1,3,2)` | [V-exh] ; réalisé exactement (F3, F4) |
| O'' | Le départage « petit diamètre, puis labels » de l'auditeur suffit sur ces deux types | [C] (0 échec sur 2 747 configurations exactes) |
| L | Après réduction maximale, le LiDAR garderait environ un quart des cellules de `Del_5` | [C] |

## 9. Questions ouvertes

1. **Annuler des paires critiques de degré ≥ 1.** Le plancher des effondrements, ce sont les cellules critiques : environ 80 cellules singulières par point en volume à `k = 5`, dont 27.5 bulles. Annuler les paires critiques de `H1` et `H2` de persistance `< ε`, sans jamais toucher `H0`, garderait `π0`, les nœuds et la couverture exacts. On obtiendrait un entrelacement de `ε` en degré ≥ 1 seulement, compatible avec la robustesse (R4). Reste à démontrer le certificat (chemins de gradient inversés, comme dans la simplification topologique d'Edelsbrunner–Letscher–Zomorodian) et son coût.
2. **Démontrer ou réfuter O''.** Le départage par petit diamètre suffit-il sur les octaèdres `(2,3,2)` et `(1,3,2)` ? Les fixtures F3 et F4 réalisent le piège à quatre sites ; il manque un oracle 3D exact général pour des groupes pris dans de vrais nuages, y compris les plateaux de la grille.
3. **Mesurer** le gain réel du causal et de la règle `θ` sur des nœuds de trames (`n = 8 000` à 32 000 sites), à partir d'une mosaïque locale stricte. Le trou de 29 220 mm³ du workflow précédent interdit d'employer `mosaique.py` sans contrôle strict des faces.
4. **Choisir un appariement optimal sur les plateaux de la grille** (gros groupes cosphériques) : heuristique, borne d'écart publiée.
5. **Priorité géométrique à taille égale.** Existe-t-il un choix, dans chaque groupe, qui minimise `max{diam σ : σ effondrée}`, donc la borne de la proposition 7 ? Il s'agit d'un problème d'affectation local, par type.
