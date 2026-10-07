# Rapport A — L'objet normatif (la thèse) et sa traduction dans la v11

**Cadre.** Lecture seule du dépôt (HEAD `e968aba8d`, moteur v11 gelé à `ac081a06f`). GCP non utilisé. Aucune
compilation, aucune mesure. Cinq petits calculs exacts en `Fraction`, écrits par moi (scripts `py/meb.py`, `e5.py`,
`gab.py`, `six.py`, `mass91.py` de mon dossier de travail), sur au plus six sites : ils jouent le rôle d'oracle borné et
ne mesurent rien.

**Pages.** « p. » désigne la page imprimée du manuscrit, « PDF » la page du fichier, avec PDF = p. + 26 (la p. 1 est la
PDF 27). J'ai lu intégralement à l'outil `Read` les Parties I–II (PDF 35–134, soit p. 9–108), plus les p. 1–8
(introduction) et la conclusion (p. 165–169). Le dépôt cite la thèse par ses pages imprimées : MATHEMATIQUES.md § 10,
l. 399, et la « p. 97 » de la passation.

**Légende.**
- Sans marque : lu dans la source citée.
- **[C]** : recalculé par moi en arithmétique exacte.
- **[I]** : inférence de ma part.
- **[R]** : chiffre rapporté par un document du dépôt, non rejoué.

---

## 0. Verdict

1. **La tour FULL de la v11 est l'objet de la thèse, enrichi ; ce n'est pas un autre objet.**
   - À chaque ordre k, ses composantes sont exactement celles du graphe Γ_k de la Déf. 21 (p. 58).
   - Par le Th. 2 (p. 60), ce sont les composantes de L_k, et leurs ensembles de points sont les K-polyèdres, c'est-à-dire
     les amas discrets de la Déf. 8.
   - La v11 y ajoute : niveaux carrés exacts, identité des composantes et pas seulement de leurs points, multifusions
     N-aires, coupes ouvertes et fermées, verticales entre ordres.
   - **La tour multi-ordres et ses verticales ne sont pas dans la thèse.** Elles viennent de
     `docs/SPECIFICATION_MORSEHGP3D.md` (§ 3, l. 85–103) et de `docs/math/DEFINITION_HGP_3D.md` (§ 3, naturalité
     h∘v = v∘h).
2. **La thèse a une erreur de fond, connue et gravée** : la Prop. 6 (p. 90), dont dépendent le Th. 5 (p. 91) et la
   Déf. 30.
   - Je l'ai revérifiée en exact **[C]**.
   - J'ajoute un point : **E5 est en position générale au sens de la Déf. 26** (aucune violation sur toutes ses parties).
     E5 réfute donc **directement** le Th. 5 sous ses propres hypothèses. Le registre dit seulement que le Th. 5 « hérite »
     du contre-exemple (`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, l. 41).
   - Le contre-exemple plan L02, où le graphe de Gabriel reste déconnecté pour toujours, est recalculé **[C]**. Il n'est
     **pas** gravé dans le dépôt ; il n'existe que hors dépôt, dans `build/v11-persist/audit_v10/L02_MATH_TOUR.md` § 4.11.
3. **Les expériences de la thèse calculent une « HGP-Gabriel ».** L'Algorithme 1 (p. 100), sur lequel reposent les
   Tab. 9.1–9.3, suit la voie Gabriel : ce n'est pas exactement θ^HGP.
   - [I] L'effet est probablement faible : L02 compte 11 désaccords sur 1 500 nuages aléatoires.
   - C'est néanmoins à dire à l'auteur.
4. **Ce qui est juste dans la thèse et a été porté par la v11** :
   - le Th. 2 et la Prop. 5 ;
   - le Th. 4, étendu sans position générale par le lemme W.4 ;
   - l'échec d'HDBSCAN dès K = 2 (§ 6.1), confirmé **[C]**.
5. **Trois glissements de la chaîne documentaire, à corriger avant la v12.**
   - (a) `CLAUDE.md` (l. 134) attribue au § 9.1 un « seuil relatif » et une interdiction de binariser. La thèse prescrit
     un seuil **absolu**, `min_cluster_size`, appliqué à la masse m_τ (p. 97), et ne dit rien de la binarisation.
   - (b) Le profil dit « normatif » `hgp_reduced` (SPEC § 7) reprend la formulation du Th. 5, qui est fausse ; la
     Déf. 22, elle, inclut les facettes isolées. FULL (`full_pi0`) est plus proche de la Déf. 22.
   - (c) En présence de doublons, `DEFINITION_HGP_3D.md` (l. 11, un ensemble) et SPEC § 3 (l. 91, « avec multiplicité »)
     ne définissent pas la même région L_k.
6. **Corrections à la passation et au rapport A.**
   - Le modèle « par copies » est **implanté et jugé dans l'oracle borné**, étages A et B. La formule « ni relu ni
     implanté » est inexacte.
   - La suffisance de Kruskal n'est **pas** inscrite au registre.
   - Au registre, la ligne du lemme E est **collée** à la ligne du témoin D2 par un `\n` littéral (l. 1370).
7. **Question d'objet prioritaire pour la v12 [I] : l'identité des nœuds sous quantification.**
   - La sélection plate ne sait pas garder certaines séparations « fugaces » : le vélo 3 n'est séparé qu'entre 107,4 et
     107,8 mm.
   - Les formes reconnaissables du vélo synthétique sont portées par des nœuds qui vivent 0,1 à 0,4 mm.
   - Tous vivent sous 2δ ≈ 1,73 mm, avec δ = √3/2 mm. Ce ne sont pas des traits stables d'une donnée quantifiée au
     millimètre.
   - L'objectif « garder les séparations fugaces » est donc mal posé tel quel.

---

## 1. L'objet de la thèse (Parties I–II)

### 1.1 Acronyme, question, réponse

- **HGP signifie « Hypergraphe-Percol' »** (p. 5, PDF 31 ; p. 41 ; p. 52 ; Déf. 22, p. 58, PDF 84).
- **La question** (p. 2) : « Si la hiérarchie des niveaux de densité est donnée par un simple graphe lorsque la densité
  est estimée avec un seul voisin, que devient cette hiérarchie lorsque K ≥ 2 ? »
- **La réponse** :
  - remplacer le graphe géométrique par le complexe de Čech, et la composante connexe par le K-polyèdre ;
  - pour K = 2, « ce ne sont plus les points qui percolent directement, mais les arêtes, recollées entre elles par des
    triangles » (p. 5).
- **Le cadre statistique** est celui de Hartigan (Déf. 6, p. 18–19). X ⊂ ℝ^p est un **ensemble fini** : les points sont
  distincts.
- **L'estimateur K-NN** (Déf. 7, p. 19–20) :
  - f̂_K(y) = K / (n ω_p r_K(y)^p), où r_K(y) est la distance au K-ième voisin ;
  - L_K(r) = {y : |B̄(y,r) ∩ X| ≥ K} (p. 1, 4, 59) ;
  - f̂_K ≥ λ équivaut à r_K ≤ r (Rem. 3, p. 20).
- **Convention de niveau** : le **rayon r**. La liaison simple fusionne au niveau d/2 (Déf. 1, p. 12–13). Les coupes
  sont **fermées** : la Déf. 10 (p. 25–26) exige la continuité à droite.

### 1.2 Définitions

| Déf. | p. (PDF) | Contenu exact |
|---|---|---|
| 8 | 21 (47) | x est *couvert* par C ∈ H(r) « s'il se trouve à moins de r » de C ; C^discret = X ∩ δ_r(C), où δ_r est la dilatation **fermée** (p. 66). Rem. 1 : poser C ∩ X « aurait été une erreur ». Rem. 3 : pour K ≥ 2, pas de partition. |
| 10 | 25–26 | clustering hiérarchique θ : ℝ₊ → Part(X), croissant, continu à droite, θ(0) = singletons |
| 20 | 57 (83) | σ ∈ Č(X,r) ⇔ ∩_{x∈σ} B̄(x,r) ≠ ∅ |
| 21 | 58 (84) | K ∈ ⟦1,\|X\|⟧. Γ_K(X,r) a pour sommets les (K−1)-simplexes de Č (des K-parties) ; σ et τ sont reliés dès que σ ∪ τ ∈ Č. Un **K-polyèdre** est l'ensemble des points d'une composante de Γ_K. |
| 22 | 58 (84) | θ_K^HGP(r) = {K-polyèdres de Č(X,r)}, pour tout r ≥ 0 |
| 23–24 | 65, 72–73 | probabilité de percolation Θ^cc ; vitesse v = λ_c/λ_{1−ε}, ou rapport de quantiles (Éq. 7.2) |
| 25 | 84 (110) | rayon de naissance ρ(σ) = inf_y max_{x∈σ} ‖y−x‖ ; B_σ est la plus petite boule englobante |
| 26 | 84–85 | **position générale** : ∂B_σ ∩ (X\σ) = ∅ pour **tout** σ avec \|σ\| ≥ 2 ; plus forte que la Déf. 4.2 de [37] |
| 27 | 86–87 | σ (K+1 points) est **K-séparant** si deux facettes actives (ρ(τ) < ρ(σ)) sont dans deux composantes distinctes de Γ_K(X)_{<r_σ} |
| 28 | 87 | σ est **de Gabriel** si B̊_σ ∩ (X\σ) = ∅ (plus petite boule englobante, pas boule circonscrite) |
| 29 | 89–90 (115–116) | **K-graphe de Gabriel** : sommets = facettes d'au moins un K-simplexe de Gabriel ; une clique de facettes par σ de Gabriel, au poids ρ(σ) |
| 30 | 90–91 | **K-MST** : arbre couvrant minimal de G_K^Gab ; K-MST_{≤r} |
| 31 | 91–92 | Vor_k(Q) et Del_k(X) ; un k-simplexe est « porté » s'il est la réunion de deux sommets adjacents |

**Les « liaisons »** (p. 86, avant la Prop. 5) désignent les liens que créent les K-simplexes entre les sommets de Γ_K.
`MATHEMATIQUES.md` § 10.1 (l. 404–411) distingue deux sens :
- la liaison de la thèse, une (K+1)-partie, comptée par `cofaces` ;
- les « liaisons d'un support » au sens de l'utilisateur, c'est-à-dire les K-parties que la boule relie, comptées par
  `kparties_reliees`.

Le vocabulaire de la v12 doit garder cette distinction.

### 1.3 Résultats, hypothèses et portée

| Résultat | p. | Hypothèses | Ce qu'il garantit | Mon verdict |
|---|---|---|---|---|
| Th. 1 | 17 | position générale, « pour l'unicité de l'MST » | composantes du graphe géométrique = celles de l'MST élagué | juste ; l'hypothèse est superflue pour l'égalité des composantes (prouvé pour tout EMST, registre § 2) |
| **Th. 2** | 60 (86) | aucune position générale | P_C = C^discret = {x : ∃y∈C, ‖x−y‖ ≤ r} ; θ_K^HGP = H^discrets ; θ_1 = liaison simple | **juste** (relu) : L_K(r) est la réunion des régions témoins T_r(σ), convexes compactes, et Γ_K est leur graphe d'intersection |
| Prop. 1–2 | 67, 69 | Poisson, p ≥ 2 | seuils critiques non dégénérés ; λ_c^dbscan = λ_c^core | standard |
| Prop. 3, **Th. 3** | 71–72 | deux densités constantes par morceaux, corridor, **Θ supposée continue** | fraction récupérable Θ^cc(λ_c ρ₁/ρ₀) ; rappel ≥ 1−ε ⇔ ρ₀/ρ₁ ≤ λ_c/λ_{1−ε} | preuve heuristique : passage à la limite non contrôlé ; c'est un théorème conditionnel |
| Tab. 7.1 | 74 | simulations, n = 100 000, 100 tirages | vitesse HGP > RSL et DBSCAN dès K ≥ 2 (par ex. p = 3, K = 5 : 0,732 / 0,399 / 0,625) | empirique |
| Prop. 4, § 7.5 | 77–80 | limite gaussienne « admise » (p. 79) ; quantiles « sous réserve que ces intuitions soient justes » (p. 80) | a_c^core = a_c^dbscan ≤ a_c^poly | conjectural, déclaré comme tel |
| **Prop. 5** | 86 (112) | aucune | les adjacences élémentaires (\|τ∪τ'\| = K+1) suffisent | **juste** |
| **Th. 4** | 88 (114) | position générale (Déf. 26) | tout K-simplexe séparant est de Gabriel | **juste** ; la position générale est inutile (lemme W.4) |
| **Prop. 6** | 90 (116) | aucune affichée ; la preuve invoque le Th. 4 | points des composantes non triviales de G^Gab_{≤r} = K-polyèdres non triviaux | **faux** (§ 2.4) |
| **Th. 5** | 91 (117) | position générale | même énoncé pour K-MST_{≤r} | **faux, même en position générale** (§ 2.4) |
| Th. 6 | 92 (118) | position générale | tout K-simplexe de Gabriel est porté par Del_K | juste ; [I] la preuve vaut sans position générale avec la définition fermée de Vor_k : c_σ ∈ Vor_K(σ\\{s}) ∩ Vor_K(σ\\{t}) |
| Prop. 7 | 97 | un départage déterministe | le vote donne une partition stricte | trivial |
| Prop. 8–9 | 98–99 | plan, position générale | K = 2 par la triangulation de Delaunay ordinaire, en O(n log n) | — |
| Th. 7 | 99 | implicites | tout Gabriel d'ordre K s'identifie depuis deux (K−1)-simplexes portés par Del_{K−1} | **preuve esquissée** (« arguments maintenant habituels ») ; [I] lacune ci-dessous |

**La lacune du Th. 7 [I].** Une facette active τ_s n'est pas forcément de Gabriel : sa plus petite boule peut sortir de
B_σ. Le Th. 6 ne s'applique donc pas tel quel. Le registre classe l'usage algorithmique de ce théorème `not_proved`
(l. 248). La v11 n'utilise aucune mosaïque de Delaunay.

### 1.4 Le § 9.1 (p. 96–97, PDF 122–123)

- **L'objet** : « l'objet naturel n'est pas une partition de X, mais un recouvrement de X (ou bien une partition des
  (K−1)-simplexes) ».
- **Les faces** : F_K est l'ensemble des (K−1)-simplexes « effectivement construits par l'algorithme » ; dans la version
  standard, ce sont les facettes de Gabriel. Chaque face reçoit une étiquette ℓ(τ) ∈ {0, …, q−1}, ou −1 pour le bruit,
  après « condensation de l'arbre et sélection des clusters pertinents par excès de masse comme HDBSCAN ».
- **Le score** : S_τ = Σ_{σ⊃τ, |σ|=K+1} ψ(ρ(σ)).
  - Par défaut ψ(t) = 1/t^p, car ce choix « reflète plus exactement la densité locale » ; ψ ≡ 1 est permis.
  - En dimension 3, cela fait z = 3.
- **La masse** : T_x = Σ_{τ∈F_K, x∈τ} S_τ et m_τ = S_τ Σ_{x∈τ} 1/T_x, avec 1/T_x = 0 si T_x = 0.
  - w_{xτ} = S_τ/T_x est une **partition de l'unité** : « il distribue une masse totale égale à 1 entre les faces qui le
    contiennent ».
  - Donc Σ_τ m_τ = #{x : T_x > 0}. **m est un comptage doux de points**, pas un comptage de faces.
  - **[C]** Sur les deux triangles, la somme vaut 6 pour trois ψ différents.
- **Le seuil, p. 97** : « C'est ce poids m_τ, et non le simple comptage des faces, qui est utilisé par le seuil
  `min_cluster_size` dans l'arbre condensé ». C'est un **seuil absolu**, en unités de points :
  - Alg. 1, l. 7 (p. 100) ;
  - `min_cluster_size` = √n dans les expériences SIPU (p. 101).
- **Le vote** : V_x(c) = Σ_{τ∋x, ℓ(τ)=c} S_τ/T_x, et ℓ̂(x) ∈ argmax_c V_x(c) (Prop. 7).
- **L'exposant.** La thèse en a deux usages :
  - un défaut ψ = t^{−p}, avec p la dimension ambiante ;
  - dans les expériences, ψ(t) = 1/t et ρ̂ = 1/r « pour l'équité » avec HDBSCAN, soit z = 1 (p. 101) ;
  - avec la remarque que ρ̂ = 1/r², c'est-à-dire 1/r^p en 2D, corrige birch2 (p. 103).
  - La mémoire utilisateur `exposant-z-ponderation.md` ajoute la critique : prendre la dimension intrinsèque, 2 pour des
    surfaces LiDAR, plutôt que la dimension ambiante.
- **Ce que le § 9.1 ne dit pas** :
  - aucun seuil relatif ;
  - aucune interdiction de binariser ;
  - aucune famille de faces intrinsèque à l'objet : F_K dépend de l'algorithme. Le registre (l. 153) grave un
    contre-exemple v9 : le catalogue Gabriel et le catalogue ordre-Voronoï donnent des poids différents au sens du § 9.1.

### 1.5 Le § 6.1 : HDBSCAN échoue dès K = 2 (p. 54–56, PDF 80–82)

- **La configuration** : deux triangles équilatéraux de côté 2r ; C et D se font face à 2r.
- **RSL et (H)DBSCAN à K = 2** : les six feuilles fusionnent au niveau r (Fig. 6.1b). La note de la p. 55 ajoute que
  c'est aussi le cas à K = 3, et pour la liaison simple.
- **La hiérarchie de Hartigan à K = 2** :
  - au niveau r, sept 2-polyèdres : les sept segments, qui sont des facettes isolées ;
  - à r' = 2√3/3 r, {A,B,C}, {C,D} et {D,E,F}, qui se recouvrent (Fig. 6.5, p. 59) ;
  - à r'' = AD/2 ≈ 1,932 r, une seule composante. C'est une **fusion ternaire simultanée** (Fig. 6.4 : « l'apparition
    simultanée des quatre points de tangence ») : la thèse contient elle-même une multifusion.
- **[C] Vérification** (coordonnées entières de la mémoire `hdbscan-echoue-deja-k2.md`) :
  - RSL à K = 2 réunit tout dès a = 10^6, soit r = 1000 ;
  - Γ₂ y a 7 composantes ;
  - ABC | CD | DEF à a = 1 333 294, soit r ≈ 1154,7 ;
  - une seule composante à a = 3 731 956, soit r ≈ 1931,8.
- **Fixture exacte** : `tests/fixtures/exact/chapter6_six_points.json` (niveaux carrés 1, 4/3 et 2+√3 pour r = 1) ;
  registre, l. 71.

### 1.6 Ce que la thèse ne contient pas

- **Ni tour multi-ordres ni verticales.** Chaque θ_K est séparé ; le chapitre 7 compare des valeurs de K sans relier
  les hiérarchies.
- **Aucun théorème de stabilité.** La stabilité au sens de Gromov–Hausdorff est une perspective (p. 168). La thèse
  écrit aussi : « HGP-Clusterer n'est pas plus consistant (au sens de Hartigan) que le Single-Linkage. Il est
  seulement fractionnellement consistant » (p. 168).
- **Aucune multiplicité** : X est un ensemble.
- **Aucune règle pour les événements simultanés**, bien que la Fig. 6.4 en montre un.
- **Aucune borne de coût en 3D.** On y trouve seulement C(n, K+1) pour Čech (p. 83) et O(n log n) pour K = 2 dans le
  plan (Prop. 9).
- **L'origine du contrat de 100 ms**, en revanche, y est : § 5.4.1, p. 47–48. La segmentation 4D sur SemanticKITTI est
  « de l'ordre de la seconde pour l'instant lorsqu'une version industrielle impose de pouvoir traiter […] dix trames par
  seconde ».
- **[I] Cette application est plus facile que la cible v11.** Elle tourne « pour chaque trame et pour chaque classe
  d'intérêt » (p. 47), avec des a priori de taille par classe. La v11 traite des trames entières sans sol. Or les
  objets en contact entre classes (vélo contre un mur) sont précisément les cas où la sémantique aurait aidé.

---

## 2. Le pont entre la thèse et la v11

### 2.1 Correspondance

| Thèse | v11 (`morsehgp3D_v11/docs/MATHEMATIQUES.md`) |
|---|---|
| X ensemble fini de ℝ^p | n sites distincts de poids un sur la grille [0,2^B)³, B = 21 par défaut (§ 1, l. 9–17) |
| r, Č(X,r), Γ_K(X,r) (Déf. 20–21) | a = r² ; sommets de Γ_k(a) : k-parties avec β(F) ≤ a ; chaque (k+1)-partie G avec β(G) ≤ a relie ses k-faces (§ 4, l. 137–140). C'est la Déf. 21 restreinte par la Prop. 5. |
| Th. 2 | T1 (l. 147–154), qui ajoute les **coupes ouvertes** sur boules ouvertes et la naturalité des inclusions |
| θ_K^HGP (Déf. 22) | forêt T_k de π₀(L_k(a)) ; K-polyèdre = pts(C_v(a)), reconstruit par le lemme H (l. 808–838) |
| K-séparant (Déf. 27) | cellule de W_K de rôle **fusion** (lemmes B et C, l. 580–627), au niveau de la boule et non du seul simplexe |
| Gabriel (Déf. 28) | `gabriel_cofaces` ; toute liaison de Gabriel a sa boule dans W_K (W.3, l. 556) |
| amas discret (Déf. 8) | `cover` : A_k(x) = min_{F∋x} β(F), et E_k(x) (P2, l. 253–261), coupe **fermée** |
| — | verticales L_{k+1}(a) ⊆ L_k(a) (§ 4, l. 156–158 ; T6, l. 237–241), plateaux atomiques N-aires (T4), numérotation canonique (§ 10.2) |

### 2.2 Différences d'objet

1. **Convention de niveau.**
   - La thèse travaille en rayon ; la v11 en rayon carré pour FULL.
   - L'arbre est le même, à un changement monotone de paramètre près.
   - Tout ce qui lit la **valeur** du niveau dépend en revanche du choix : marges, scores d'EOM λ = r^{−z}, masses ψ(ρ),
     constantes de stabilité.
   - La v11 l'a payé et corrigé. La marge en niveau carré (Q₁ de la v10) n'a aucune constante uniforme : rapport 31,8 à
     L = 10⁴ (registre, l. 1311). La règle retenue mesure donc la marge en rayon, avec des dates √t+√m−√q.
   - **Règle pour la v12 [I]** : stocker et comparer en niveau carré exact, mais déclarer le rayon comme unité de toute
     grandeur métrique (vie, marge, EOM, stabilité).
2. **Composantes contre ensembles de points.**
   - La thèse publie des ensembles de points (Déf. 22) ; FULL publie des composantes, numérotées.
   - Le Th. 2 donne une bijection entre composantes de L_K et composantes de Γ_K. [I] L'injectivité de la projection
     d'une composante sur ses points n'est ni annoncée ni démontrée ; publier l'identité des composantes évite la
     question.
3. **`full_pi0` contre `hgp_reduced`.**
   - La Déf. 22 et la Fig. 6.5 comptent les facettes isolées, par exemple {C, D}. FULL aussi : ses naissances d'ordre
     k ≥ 2 sont les boules de naissance de W_K (lemme B).
   - `hgp_reduced` (SPEC § 7, l. 268) retire ces facettes. Sa formulation, « non réduits à un (K−1)-simplexe isolé »,
     est celle de la Prop. 6 et du Th. 5, qui sont faux.
   - **Point nouveau [I]** : la qualification Π_{k+1} des points v11 (`HIERARCHIE_POINTS.md`, l. 90–96 : composante de
     Γ_k à au moins deux sommets) est exactement le critère de visibilité de `hgp_reduced`. Le profil réduit vit donc
     déjà dans la v11, comme vue aval de FULL.
4. **Position générale.**
   - La thèse la suppose pour les Th. 1 et 4–6, implicitement pour le 7.
   - La v11 n'en suppose aucune : T2, « une trace est stricte si et seulement si elle est séparable » (l. 171), la
     remplace.
   - Les coquilles étendues sont énumérées. Au-delà des capacités du produit, le refus est explicite : `wide_leaf`,
     `support_shell_capacity`.
5. **Multiplicités.**
   - La thèse n'en connaît pas ; SPEC § 3 les compte.
   - Le moteur les refuse : raison `multiplicity_unsupported`, classe `unsupported_degeneracy`
     (`src/core/reasons.def:46`, `src/api/compute.cpp:85`).
   - L'oracle borné les accepte (§ 5.b).
6. **Domaine de K.**
   - Thèse : K ∈ ⟦1,|X|⟧ (Déf. 21), et K ≤ |X|−1 au chapitre 8.
   - v11 : 1 ≤ K ≤ n, et K ≤ 12 dans le produit. FULL admet K = n (naissance unique B(X)) ; `points` et `plat` la
     refusent dès K ≥ 2.

### 2.3 Ce que la v11 a étendu (sans position générale, statut `proved_here`)

- **T1** (Th. 2 aux coupes fermées et ouvertes) et **T2–T6** (trace stricte, fenêtre d'événement [p+q−1, p+m],
  plateau atomique, descente valide, suffisance constructive).
- **W.4** (l. 558–569) : le Th. 4 sans position générale.
  - Je l'ai relu : si z ∈ I_b \\ G, les parties (G\\{s})∪{z} sont strictes, car leur trace est inchangée, et deux d'entre
    elles partagent la face (G\\{s,s'})∪{z}.
  - C'est la preuve de la thèse, où T2 remplace la position générale. **Correct.**
- **P5** (l. 301–318) : stabilité de FULL par entrelacement **en rayon**, sous déplacement apparié ε.
  - Je l'ai relue : la k-ième statistique d'ordre est 1-lipschitzienne. **Correct.**
  - La thèse n'a aucun résultat de ce type.
  - P5 ne vaut pas sous insertion. Moins de k sites ajoutés peuvent créer ou réunir des composantes : la bonne forme
    est le décalage en k, Ω_k^P ⊆ Ω_k^{P∪O} ⊆ Ω_{k−m}^P (registre, l. 154).
- **Autres résultats** :
  - P1–P4 : projections `core` et `cover`, laminarité à ordre fixé ;
  - J1–J3 : restriction, ordre un, et Euler à K+2, qui est un filet et non un certificat ;
  - lemmes A–H des supports ;
  - suffisance de Kruskal (l. 924–940).

### 2.4 Erreurs connues de la thèse, et leurs fixtures

| # | Énoncé | Statut | Témoin |
|---|---|---|---|
| 1 | **Prop. 6** (p. 90) | **faux**, y compris en position générale | E5 : `tests/fixtures/regressions/gabriel_point_set_counterexample.json`. **[C]** A=(0,0,7), B=(0,9,6), C=(1,4,0), D=(0,0,1), E=(4,1,2), K = 2. Aucune violation de la Déf. 26, toutes parties testées. Sur [83886/3563, 24), Γ₂ a une composante {A..E}, mais G^Gab en a deux, {A,B,C} et {A,C,D,E}. Cause : AC est attachée dès 33/2 par ACD et ACE, qui ne sont pas de Gabriel. |
| 2 | **Th. 5** (p. 91) | **faux sous ses hypothèses** | E5 directement : K-MST_{≤r} a les composantes de G^Gab_{≤r} (Fait 2). **[C]** L02 (plan) : A=(−100,0), C=(100,0), z=(1,−90), y=(30,−85), w=(3,300), K = 2. Position générale vérifiée ; seuls ACw, Ayz et Cyz sont de Gabriel. Dès a = 10001440081/360000, Čech n'a qu'une composante, tandis que Gabriel en garde deux, {A,C,w} et {A,C,y,z}, **pour toujours**. **Non gravé dans le dépôt.** |
| 3 | **Déf. 30**, « un arbre » | mal posée | L02 : G^Gab n'est pas connexe, donc le K-MST n'est qu'une forêt |
| 4 | **Alg. 1** (p. 100), exactitude | n'est pas θ^HGP | voie Gabriel ; [R] L02 § 4.11 : 11 désaccords sur 1 500 nuages aléatoires en position générale |
| 5 | Preuve de la Prop. 6 | lacune identifiée | la récurrence porte sur des ensembles de **points**, pas sur l'appartenance des **facettes** à des composantes. Correction : SPEC § 7 et `INCIDENCES_SILENCIEUSES_GAMMA.md` (Gabriel plus incidences silencieuses), ou W et descentes (v11). |
| 6 | Th. 7 (p. 99) | preuve esquissée | [I] voir § 1.3 |
| 7 | Th. 1 (p. 17) | hypothèse superflue | registre § 2 : tout EMST donne les mêmes coupes |
| 8 | Déf. 8 contre Th. 2 | formulation | « à moins de r » (Déf. 8) contre « ≤ r » (Th. 2, δ_r fermée) ; la lecture fermée est la bonne, et c'est celle de la v11 |

**Constat de méthode.** `CLAUDE.md` exige qu'une contradiction devienne une fixture permanente. Pour le Th. 5, E5
suffit à la règle, puisqu'il est en position générale. Mais L02 montre un mode de défaillance plus fort, la déconnexion
permanente ; il mérite sa propre fixture.

### 2.5 Écarts délibérés de la v11 par rapport à la thèse (déclarés)

- **La qualification Π_{k+1} des points contredit la Fig. 6.5** ({C,D} y est un 2-polyèdre). L'écart est assumé dans
  `HIERARCHIE_POINTS.md`, l. 92–94.
- **Les masses entières remplacent les masses fractionnaires m_τ** (critère A de la tête plate,
  `SORTIE_PLATE.md`, l. 20–23). Le vote de la thèse n'est gardé que comme bras déclaré, jamais comme projection
  (`HIERARCHIE_POINTS.md`, l. 183–186).
- **La tête plate prend z = 1 par défaut pour le LiDAR**, choisi par un critère écrit d'avance (`SORTIE_PLATE.md`,
  l. 147–151), alors que la thèse prend ψ = t^{−p} par défaut.

---

## 3. Pourquoi cet objet : enjeux

### 3.1 Ce que la thèse revendique

- **Exactitude statistique** : HGP est la hiérarchie exacte de Hartigan pour l'estimateur K-NN discret (Th. 2), et
  θ_1 est la liaison simple.
- **Critique des concurrents** :
  - la liaison simple chaîne et n'est pas consistante pour p ≥ 2 (Fait 8, p. 33) ;
  - les liaisons complète et moyenne sont instables (Fait 9, p. 34) ;
  - RSL impose une contrainte forte aux sommets et une connexité lâche par arêtes (§ 4.3.1) ;
  - DBSCAN répare la frontière, mais garde une percolation précoce (p. 79).
- **Le « paradoxe du choix de K »** pour HDBSCAN (§ 4.4.5, p. 39–40) : au-delà d'une dizaine, `min_samples` dégrade les
  résultats, parce que le graphe minmax ne représente pas les amas K-NN.
- **La percolation comme mesure de performance** (chapitre 7) : vitesse de percolation, fraction récupérable (Th. 3),
  avantage de HGP (Tab. 7.1).
- **Le K-MST est présenté comme l'outil algorithmique** : il est faux (§ 2.4).
- **Résultats empiriques** :
  - huiles d'olive italiennes (8D, K = 2) : 412 points classés directement, dont 96,1 % justes ; ARI 0,890 après
    extension, contre 0,848 pour Persistable ;
  - SIPU : meilleur ARI sur a2, a3, d31, s2 et s3 ; birch2 n'est corrigé qu'avec ρ̂ = 1/r².
  - Ces résultats ont été obtenus avec la voie Gabriel.
- **Stabilité** : rien n'est revendiqué au-delà de la percolation.
- **Coût** : rien d'autre que la nécessité d'éviter Čech, et les 10 trames/s.

### 3.2 Ce que la v11 en a vérifié [R : chiffres lus dans `HIERARCHIE_POINTS.md` § 7]

**Le paradoxe de K se retrouve au niveau B** (meilleur bloc), en synthétique à n = 8 000 : l'écart `margin_r` −
HDBSCAN croît avec k.

| k | 2 | 3 | 5 | 10 |
|---|---|---|---|---|
| écart `margin_r` − HDBSCAN | +0,008 | +0,026 | +0,050 | +0,078 |

Source : l. 262.

Deux limites :
- Le gain vient de FULL, pas de la projection. « MR₂-bord, sans tour, rattrape aussi le vélo C » (l. 305). La mémoire
  du 29 septembre (`clustering-depuis-la-tour.md`) note déjà : à même entrée et même tête, la tour égale MR₂-bord.
  [I] Une bonne part de l'avantage empirique tient donc à la **sémantique d'entrée** (couverture de la Déf. 8 contre
  cœur), transposable à HDBSCAN. L'avantage propre de la connexité d'ordre supérieur reste l'axe K et l'exactitude.
- Sur le LiDAR, l'avantage est petit hors des trames choisies, et la sélection plate reste le goulot
  (`AUDIT_FINAL_V11.md`, § 8.3).

### 3.3 Ce qu'apporte l'axe K

- **Contacts ténus et ponts de bruit.** Monter K retarde la percolation parasite (Tab. 7.1 : la vitesse de HGP croît
  avec K, celle de RSL décroît). En revanche, monter K fond les parties minces. [R] Les roues lisibles à K = 2 ne le
  sont plus à K = 5 (`receipts/notes_hors_depot_20261007/polyedres_reconnaissables/SYNTHESE.md`, l. 191).
- **Une tranche à K fixé n'est pas robuste aux aberrants.** La forme juste est le décalage en k
  (Ω_k^P ⊆ Ω_k^{P∪O} ⊆ Ω_{k−m}^P, registre, l. 154). [I] C'est la raison mathématique de garder **toute la tour**
  avec ses verticales, conformément au fait 2 de `CLAUDE.md` (Rolle–Scoccola).
- **Aucune sortie v11 n'exploite l'axe K.** La synthèse multi-ordres reste ouverte : P6 de `HIERARCHIE_POINTS.md`,
  l. 324.
- **La densité ne sépare pas ce qui se touche.** [R] Mémoire `verrou-full-vers-points.md` : à K = 5 et 10, ni la tour
  ni HDBSCAN n'isolent les objets en contact (0 exacte sur 42 objets à moins de 0,1 m).

---

## 4. Hiérarchies sur points dérivées

### 4.1 Ce que la thèse prescrit

- **La sémantique d'appartenance** : la couverture de la Déf. 8, fermée, contre le cœur C ∩ X, qui « aurait été une
  erreur ».
- **La partition stricte** par vote pondéré, **après** sélection (§ 9.1, Prop. 7).
- **Le seuil absolu sur masses fractionnaires** (p. 97).
- **Pas de hiérarchie laminaire de points.** [I] Le vote est défini sur une sélection, pas par niveau. Par niveau il
  n'est pas laminaire, et figé il est discontinu (`HIERARCHIE_POINTS.md`, l. 183–186).

### 4.2 Les règles étudiées par la v11

| Règle | Définition | Statut |
|---|---|---|
| `core` | x entre à D_k(x) (P1) | cœur de RSL/HDBSCAN ; perd 0,017 à 0,047 au niveau B |
| `cover` | première couverture A_k(x) ; E_k(x) est un **ensemble** de composantes (P2) | fidèle à la Déf. 8 ; LCA discontinu sur {0,2,4} (P5, l. 320–325) |
| MR₂-bord | HDBSCAN avec entrée de bord | {0,2,5} : cover ≠ MR₂-bord (`reference/test_projection_contracts.py`, l. 21–63) |
| **H^r_{k+1} = P_1∘Π_{k+1}** | ancrage persistant en rayon, κ = 1, sur couverture qualifiée | H1–H7 prouvés : laminarité, stabilité 3ε, retard borné α_{k+1} ≤ e ≤ α_{k+1}+d_k/2 ; **non stable par insertion** ; échoue Q2–Q4 (70/125) ; obstruction de Palm **conditionnelle** (Θ_H < Θ^poly à k = 2, m = 3) |
| vote sur tour condensée | proposition de l'utilisateur du 1er octobre (v10) | non retenu ; l'existence par taille de cœur échoue les deux triangles (mémoire) |

**[I] Tension structurelle.** La règle retenue s'écarte de la Déf. 22 (Π_{k+1}) pour réussir les deux triangles. Le
registre ne donne qu'un théorème conditionnel, mais si l'obstruction de Palm se confirme, cet écart a un coût
asymptotique au sens du chapitre 7.

### 4.3 Le seuil de condensation : quatre conventions dans la chaîne d'autorité

| Source | Masse | Seuil |
|---|---|---|
| Thèse : p. 97, Alg. 1 l. 7, p. 101 | m_τ fractionnaire, comptage doux de points | **absolu**, `min_cluster_size` (√n dans les expériences) |
| SPEC § 17, `CondensedView` (l. 1537, 1565) | nombre entier de `PointId` distincts | absolu, `relation=at_least`, 20 par défaut |
| v11 `plat`, critère A | sites engagés, entier | absolu, mcs 20 par défaut |
| `Zoltan/FoundationModel/SPECIFICATION.md` § 2.1–2.2 (l. 48–92) | m_τ, ψ = t^{−3} | **relatif** au parent (α), sans binarisation |
| `CLAUDE.md` l. 134 | m_τ, « jamais un comptage » | « seuil relatif », **attribué à tort au § 9.1** |

**Mesures [R]**, `notes_hors_depot_20261007/polyedres_reconnaissables/SYNTHESE.md`, l. 170–181 :
- REL20 (α = 1/10) ne garde que 9 objets sur 19, et 0 vélo sur 3 contre le mur. Sa propre règle, écrite d'avance, le
  rejette.
- ABS (m_min = 20) garde les 19 objets.
- La masse de substitution (S_τ ≡ 1) donne le même PQ qu'un comptage, au bit près.

**[C]** Sur les deux triangles de la thèse, les masses du § 9.1 calculées sur les faces de Gabriel donnent au triangle
ABC une masse de 1,90 (ψ ≡ 1), 2,10 (ψ = 1/r) ou 2,48 (ψ = 1/r³), toujours inférieure à 3. La conclusion de
`HIERARCHIE_POINTS.md` (l. 175–177) est donc confirmée : à mcs 3, le triangle ne serait pas un cluster. La valeur
exacte 8/3 qu'il cite n'est pas reproduite sur la géométrie de la thèse ; elle dépend de conventions qui ne sont pas
précisées (faces vivantes, horizon).

**Mon avis.**
- **La thèse est claire : seuil absolu.** « Jamais un comptage » veut dire « jamais un comptage de faces » ; m_τ est un
  comptage doux de points.
- **Défaut de la v12 [I]** : seuil absolu, masse entière ou masse m_τ déclarée, le seuil relatif restant un bras
  explicite.
- **À corriger dans `CLAUDE.md`** : le seuil relatif et l'interdiction de binariser y sont présentés comme prescrits par
  le § 9.1. Ce sont des choix de conception (justifiés : la Fig. 6.4 montre une fusion ternaire) ; ils doivent être
  présentés comme tels.
- **Une décision de l'utilisateur est attendue** (`PASSATION.md` § 6.1, point 5).
- **La masse m_τ n'est pas définie sur FULL** tant qu'on n'a pas choisi la famille de cofaces : Gabriel, toutes les
  cofaces de W_K, ou un autre choix. [R] Elle n'est pas calculable depuis SPv2 (rapport D, l. 207).

---

## 5. Questions ouvertes au niveau de l'objet, et priorités pour la v12

### a) Identité des nœuds sous quantification — **priorité haute** [I]

- **Le fait.** Sur ng00–ng02, 61 à 67 % des nœuds K5 vivent moins de 2δ, avec δ = √3/2 mm [R]
  (`audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md`, l. 50).
- **Le cadre théorique.** P5 ne garantit une image appariée qu'aux nœuds de vie supérieure à 2ε en rayon, par rapport
  au nuage non quantifié. Entre deux quantifications du même nuage, il faut au moins 4δ.
- **La conséquence.** Les « séparations fugaces » du § 3.2 de `SORTIE_PLATE.md` (vélo 3 : 0,4 mm) et les formes
  reconnaissables (0,1 à 0,4 mm) sont **sous ce seuil** : ce ne sont pas des traits identifiables de la donnée.
- **Pour la v12** :
  - publier la vie de chaque nœud rapportée à δ ;
  - n'appuyer sélection et jetons que sur des vies supérieures à 2δ ;
  - reformuler la question ouverte « garder les séparations fugaces » de `PASSATION.md` § 7 en critère explicite de
    stabilité.

### b) FULL pondéré, modèle par copies — **décider maintenant, porter plus tard**

- **L'objet est défini** : SPEC § 3, D_k compté avec multiplicité.
- **Il est implanté dans l'oracle borné.**
  - `reference/README.md`, l. 29 : l'étage A « travaille sur les copies », l'étage B sur des sites pondérés avec le
    quotient de Gordan.
  - Le juge B = A couvre 34 nuages à doublons, 264 boules à coquille pondérée et 220 multiensembles alignés à doublons
    [R].
  - L'audit géant (`receipts/audit_geant_20261005/math/REPORT.md`) le confirme : « A/B admettent les copies ».
- **Il manque** : la preuve dans le contrat du moteur (naissances nulles pondérées, fenêtres non contiguës, quotient
  local) et le port natif.
- **Sur les données cibles** : **[C] aucun doublon au millimètre dans les 720 enregistrements `"duplicates"`** des reçus
  v11, et « l'arrondi au millimètre ne fusionne aucun retour sur ces trames » (même note, l. 51).
- **Garde à noter.** `bench/points_lidar_prepare.py` (l. 134–141) **dédoublonne** par `np.unique` et ne fait que
  compter les doublons. Si des doublons apparaissaient, le refus du moteur serait contourné.
- **Pour la v12** : refus explicite, avec le compteur publié dans le manifeste de l'exécution. Unifier la définition de
  L_k (`DEFINITION_HGP_3D.md` contre SPEC § 3). Le moteur pondéré passe après le contrat de temps.

### c) Coquilles étendues — **priorité basse pour la vitesse, moyenne pour la complétude**

- La coquille maximale vaut 5 sur le LiDAR [R].
- Les cas adverses existent :
  - 84 points entiers sur x²+y²+z² = 50 (`MATHEMATIQUES.md`, l. 739) ;
  - une coquille de 270 sites est refusée [R].
- **Pour la v12** : garder le refus explicite, graver les témoins adverses, et laisser le quotient polynomial à la
  recherche.

### d) Polyèdre d'ordre k, A_k(r) — **priorité basse dans le cœur de la v12**

- L'objet est fixé avec l'auditeur : la mosaïque d'ordre k filtrée par d_k.
- Trois réfutations sont gravées (`polyhedron_order_k_counterexamples.json`, `1fbeea5b8`).
- Le coût est d'environ 1 000 faces par site à K5 [R].
- La thèse ne le propose que comme analogie (mémoire `polyedre-ordre-k.md`).
- **Pour la v12** : en aval et à la demande, K = 2 d'abord ; aucune contrainte sur le chemin des 100 ms.

### e) Objet normatif de la v12 — **priorité haute, décision peu coûteuse**

- Déclarer FULL, c'est-à-dire `full_pi0` avec ses verticales et sa naturalité. `hgp_reduced` devient une vue,
  identique à Π_{k+1}.
- Le mot `profile` est **surchargé** :
  - `CLAUDE.md` en fait le profil d'objet (`hgp_reduced`, `full_pi0`, `generic_core`) ;
  - le cadre v11 l'emploie pour la quantification d'entrée (`quantized_u21_input_only`).
- Le cadre v12 doit annoncer les deux axes séparément.

### f) Points et synthèse multi-K — **priorité moyenne, après le contrat de temps**

- Il reste à trouver une règle stable par insertion, locale au profil, qui passe T0 et Q1–Q4.
- Le critère d'existence d'un cluster est ouvert.
- [I] La synthèse multi-K doit s'appuyer sur le décalage en k (§ 3.3) et sur les verticales.
- Le dépôt contient déjà un candidat dans la ligne enregistrée : niveaux k/r^z, décidables exactement
  (`docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md` ; registre, l. 31).

---

## 6. Vérification des affirmations de l'audit final (§ 4.1–4.5) et du rapport A

| Affirmation | Verdict | Preuve |
|---|---|---|
| § 4.1 : l'objet (sites distincts de poids un ; niveau = rayon carré non réduit ; T_k ; coupes ; verticales à la coupe fermée ; Cat_K) | **confirmé** | `MATHEMATIQUES.md` § 1–4, § 6 et § 10.2. Nuance : la borne K ≤ 12 est celle du produit (SPEC : K_max ≤ 10 ; la thèse n'en a pas). |
| § 4.2 : contrat sans position générale ; T2 remplace la position générale | **confirmé** | l. 171–173 |
| § 4.2 et rapport A : « lemmes A, P, W, B à H et suffisance de Kruskal au registre, `proved_here` » | **partiellement faux** | la section V11 du registre (l. 1352–1376, commit `5adf6a59f` du 4 octobre) ne contient pas la proposition de suffisance de Kruskal du 6 octobre (l. 924) ; la ligne E est fusionnée à la ligne D2 par un `\n` littéral (l. 1370) |
| W.4 : le Th. 4 sans position générale | **confirmé** | relu |
| Contre-épreuve de l'auditeur : 65 ordres, 17 276 unions, 1 453 coupes, 15 925 faces | **confirmé** | `receipts/audit_geant_20261005/math/REPORT.md` (base `238734f1d`, 10 nuages de 6 à 8 sites ; les deux nuages u24 n'exercent qu'A et S1) |
| J3 : Cat_{K+2} suffit ; Euler est un filet | **confirmé** | registre, l. 1339–1350 |
| Oracle : 1 362 ordres, 48 234 coupes, 22 mutants, 190 dumps v10 | **confirmé** | `reference/README.md` |
| § 4.3, tableau : « Marge en niveau carré (Q₁ v10, ER0h) — rapport 31,8 » | **imprécis** | 31,8 concerne Q₁ (registre, l. 1311) ; ER0h tombe par la proposition S (l. 1312). Le rapport A cite correctement les deux. |
| § 4.4 : E5, la Prop. 6 est fausse | **confirmé [C]** | |
| § 4.4 : manques | **à compléter** | le Th. 5 est réfuté directement (E5 en position générale) ; L02 n'est pas gravé |
| § 4.4 : {0,2,4}, {0,2,5}, sept sites, D2 | **confirmé** | fichier de test et § 10.11 |
| § 4.4 : 61 à 67 % des nœuds de vie < 2δ | rapporté [R] | note du 7 octobre, l. 50 |
| § 4.5 et rapport A : « le modèle par copies n'est ni relu ni implanté » | **inexact** | il est implanté et jugé dans l'oracle (§ 5.b) ; ce qui manque est la preuve dans le contrat du moteur et le port natif |
| Rapport A § 5, [I] : « L02 n'a pas de fixture v11 » | **confirmé** | hors dépôt seulement ; la fixture `contre_exemple_th5_plan` est introuvable sous le dépôt |
| Rapport A § 1 : poids > 1 → `unsupported_degeneracy` | **confirmé** | raison `multiplicity_unsupported` |
| Passation § 6.1.5 : « seuil absolu (thèse, p. 97) » | **confirmé** | p. 97 imprimée, soit PDF 123 |

**Manques des documents de passation, dans mon domaine :**
- l'Algorithme 1 est la voie Gabriel ;
- l'origine des 100 ms est à la p. 47–48 ;
- la tour multi-ordres est hors thèse ;
- `hgp_reduced` équivaut à Π_{k+1} ;
- les quatre conventions de seuil ;
- la dépendance de m_τ à la famille de faces ;
- la tension entre séparations fugaces et 2δ ;
- la surcharge du mot `profile` ;
- l'incohérence ensemble/multiensemble entre `DEFINITION_HGP_3D.md` et la SPEC.

---

## 7. Recommandations pour la v12 (domaine A)

1. **Contrat d'objet.**
   - FULL avec verticales et naturalité ; niveaux exacts carrés en stockage, rayon pour toute grandeur métrique.
   - `hgp_reduced`, points et plat sont des vues.
   - Porter tels quels Th. 2, Prop. 5, T1–T6, W.4, P5 et les lemmes A–H.
2. **Registre.**
   - Graver L02 en fixture permanente.
   - Remplacer « hérite » par « réfuté sous ses hypothèses (E5, L02) ».
   - Ajouter les lignes « Alg. 1 = voie Gabriel » et « suffisance de Kruskal ».
   - Réparer la ligne E (l. 1370).
3. **Chaîne documentaire.**
   - Corriger dans `CLAUDE.md` l'attribution au § 9.1 (seuil relatif, binarisation).
   - Faire trancher l'utilisateur sur le seuil.
   - Unifier la définition de L_k en présence de doublons.
4. **Entrée.**
   - Refus des multiplicités, avec un compteur publié de bout en bout.
   - Aucune préparation ne dédoublonne sans que ce soit vu.
5. **Sorties.**
   - Vie de chaque nœud rapportée à δ.
   - Sélection et jetons sur vies supérieures à 2δ.
   - Cible « séparations fugaces » reformulée.
6. **Errata à transmettre à l'auteur de la thèse** (autorisé : mémoire `these-source-pas-autorite.md`) :
   - Prop. 6, Th. 5, Déf. 30 et l'exactitude de l'Alg. 1 ;
   - lacune de la preuve du Th. 7 ;
   - hypothèse superflue du Th. 1 ;
   - formulation de la Déf. 8.

## 8. Références principales

**Thèse** : `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, p. 1–108 et 165–169.

**v11** :
- `morsehgp3D_v11/docs/MATHEMATIQUES.md` (§ 1–10) ;
- `HIERARCHIE_POINTS.md`, `SORTIE_PLATE.md` ;
- `AUDIT_FINAL_V11.md` § 4 ;
- `PASSATION.md` § 4, § 6.1, § 7 ;
- `receipts/passation_20261007/rapports/A_mathematiques_exactitude.md` ;
- `reference/README.md`, `reference/test_projection_contracts.py` ;
- `bench/points_lidar_prepare.py:134` ;
- `src/core/reasons.def:46`.

**Racine** :
- `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, l. 30–71, 150–156 et 1293–1376 ;
- `docs/SPECIFICATION_MORSEHGP3D.md` § 3, 4, 7, 8, 12, 17 ;
- `docs/math/DEFINITION_HGP_3D.md` ;
- `tests/fixtures/regressions/gabriel_point_set_counterexample.json`,
  `tests/fixtures/regressions/polyhedron_order_k_counterexamples.json`,
  `tests/fixtures/exact/chapter6_six_points.json` ;
- `Zoltan/FoundationModel/SPECIFICATION.md` § 2 ;
- `CLAUDE.md`, l. 134.

**Hors dépôt** : `build/v11-persist/audit_v10/L02_MATH_TOUR.md` § 4.11.

**Mémoires utilisateur** : `these-source-pas-autorite.md`, `hgp-old-code-de-la-these.md`,
`hdbscan-echoue-deja-k2.md`, `exposant-z-ponderation.md`, `verrou-full-vers-points.md`,
`clustering-depuis-la-tour.md`, `ordre-tour-hierarchie-puis-z.md`.
