# Axiomes pour une hiérarchie laminaire de points issue de FULL_k

3 octobre 2026, soir. Label `axiomes` du workflow « meilleure méthode mathématique ». Lecture seule du dépôt et du
worktree `build/v11-claude-20261003/morsehgp3D_v11` ; aucune commande git, aucun natif, aucun GCP.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (oracle de la définition hgp11_ref, Python exact)
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilisé. Écritures : build/v11-points-math/axiomes/ seulement.
```

Conventions. r est un **rayon** ; le niveau publié par le moteur est le rayon carré a = r². Coupes fermées.
d_k(x) est le rayon cœur (distance au k-ième voisin, x compris ; D_k = d_k² dans les notations du développeur),
α(x) le rayon de première couverture. « Prouvé » signifie : preuve complète écrite ici, ou citée avec sa source
et relue. « Testé » signifie : commande et sortie dans ce dossier (§ 10).

## 0. Réponse courte

1. **La règle H_m du développeur est la règle Q_1 de la v10 (marge « quadratique », en niveau carré), munie d'un
   seuil de qualification.** La même formule en rayon est l'ancrage persistant P_1 de la v10
   (`build/v10-verrou-points/ancrage_marges/MEMO_ANCRAGE_MARGES_20260930.md`), que la note
   `HIERARCHIE_POINTS.md` ne cite pas. Contrôle : l'implantation de ce dossier redonne exactement `margin1` et
   `margin` de l'oracle (1 822 comparaisons, 0 désaccord) et, en rayon, les dates publiées de P_2 par la v10
   (54,844 sur Q2 ; 1 487,404 sur Q1). Pendant ce travail (21 h 48 UTC), le développeur a ajouté la version en
   rayon (`bench/points_radius.py`, `reference_radius_rules`) : mêmes dates et mêmes hauteurs que P_1∘Π_m ici
   (14 408 sites, 39 700 paires, 0 écart).
2. **Le passage au niveau carré est une régression.** P_1 entre chaque site au plus tard à la date de Q_1, et
   strictement plus tôt dès que la barre qui fixe la date de P_1 naît strictement après α (prouvé ; testé :
   12 347 sites sur 37 248 strictement plus tôt, aucune violation). Q_1 n'a aucune constante de Lipschitz
   uniforme en rayon (rapport 31,8 pour un déplacement unité à L = 10 000, croissance en √L) et peut entrer un
   site 500 fois après son niveau cœur (en niveau carré) ; P_1 est 3ε-stable en dates **et** en hauteurs (la
   borne 5δ de H3 est lâche : 3δ suffit).
3. **Trilemme généralisé** (§ 3). Fidélité + entrée immédiate locale (E_u) ⇒ discontinuité (rappel v10).
   Fidélité + équivariance + entrée immédiate des sites sans rival (A5_glob) + continuité ⇒ **anticipation
   obligatoire** : aucune règle causale (nouveau, théorème B). Dans la classe des règles locales au profil,
   la constante de Lipschitz des dates est au moins 3 et **le front précocité/stabilité est exact** : la règle la
   plus précoce de constante c est P_κ avec κ = (c − 1)/2 (nouveau, théorème C ; la v10 le connaissait à +1 près
   en géométrie).
4. **Question 2 : H_1 est-elle la plus précoce ?** Non telle qu'écrite (Q_1 est dominée par P_1). Oui pour sa
   version en rayon P_1, et seulement dans un cadre précis : règles fonctions du seul profil couvrant du site,
   équivariantes, monotones, de constante intrinsèque 3 (preuve complète). Hors de ce cadre, des règles fidèles
   et stables entrent plus tôt : P_κ (κ > 1, constante 1 + 2κ), et H_{k+1} en rayon, de même constante 3, qui
   entre C des deux triangles exacts à 0,8165 au lieu de 1,2247.
5. **Question 3 : le seuil m = k + 1 n'est pas forcé par A8 ; il est réfuté par A8.** Dans toute la famille
   P_κ∘Π_m et Q_κ∘Π_m (tout κ ≥ 1, tout m), Q1bis exige m = 3 et Q2 exige m ≤ 2 à K = 2 (prouvé, balayage
   à l'oracle). H_{k+1} échoue Q2, Q3, Q4 et Q-Π2 ; le choix m = max(k+1, mcs) viole en plus A7 et fabrique
   exactement le cluster {c0..c7, x} refusé par l'utilisateur. Aucun comptage de sites ne sépare Q1 de Q2 :
   les deux opposent une structure de 2 sites à une de 3.
6. **Ce que A8 force réellement.** Les deux triangles exacts (T0) sont incompatibles avec toute règle locale au
   profil, équivariante et **monotone** (théorème E). A8 exige donc une règle sensible à la multiplicité des
   branches couvrantes (comme ER0h, 125/125 cellules en v10). La compatibilité de A8 avec une stabilité
   lipschitzienne uniforme reste **ouverte** ; une piste chiffrée (score « −naissance + λ × persistance interne »)
   sépare les cinq cellules pour λ ∈ ]0,969 ; 1,813[ (§ 9), sans preuve de stabilité.
7. **Question 4 : entrée après le cœur.** Q_1 et H_{k+1} : oui, sans borne. P_κ : jamais à K = 2, au plus
   1,5 d_k à K ≥ 3, borne atteinte ({0, 10, 20, 30}, K = 3). Au sens du chapitre 7, ce qui compte est l'entrée
   **avant la fusion parasite F** : P_κ y place tout point cœur de la composante géante tel que
   α + d_k/2 ≤ F (prouvé) ; Q_1 ne garantit rien (un rival apparu après F retarde un point cœur au-delà de F).
   Le défaut est réel pour Q_1, borné pour P_κ, sans objet pour la qualification (ses retards portent sur des
   groupes d'au plus k sites, hors de la composante géante).
8. **Tension à arbitrer par l'utilisateur.** Les cellules A8 veulent la petite structure précoce (Q2, Q3, Q4)
   **et** la grande structure multiple (T0, Q1) ; la mesure agrégée du développeur (meilleur IoU, G4) préfère
   H_{k+1}, qui écarte par construction toute structure de k sites. Aucune règle connue ne satisfait à la fois A8 et une stabilité
   prouvée.

## 1. Sources, et ce qui est nouveau

Lus en entier : `HIERARCHIE_POINTS.md` (version mise à jour pendant ce travail, avec les mesures G4),
`MATHEMATIQUES.md` § 7, `bench/points_reference.py`, `bench/points_gate.py`, `reference/hgp11_ref/definition.py`,
le reçu de l'auditeur `receipts/full_points_20261003/qualified_proof/README.md` et sa section de
`AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md`, `VERDICT_FINAL.md` et les réponses de l'utilisateur de la v10,
les questions Q1–Q4 (`revision_cible/QUESTIONS_UTILISATEUR.md`), les chapitres 6, 7 et 9 extraits de la thèse.
En cours de route : `axiomes_impossibilite/MEMO_AXIOMES_IMPOSSIBILITES_20260930.md`,
`ancrage_marges/MEMO_ANCRAGE_MARGES_20260930.md` et `SYNTHESE/VERROU_FULL_POINTS.md` de la v10, qui contiennent
déjà une grande partie de la théorie demandée.

| Résultat | Statut avant ce rapport | Apport ici |
| --- | --- | --- |
| trilemme L + NP + E_u ⇒ discontinuité | prouvé v10 (théorème B des axiomes) | rappelé |
| P_κ : forme code-barres, dates (1+2κ)ε, retard ≤ d_k/2 | prouvé v10 (ancrage, T3, T5) | identifié à H_κ en rayon ; contrôle oracle |
| hauteurs de P_κ en (1+2κ)ε | prouvé v10 (vérificateur, SYNTHESE § 5.2) | redémontré dans le cadre intrinsèque ; corrige H3 (5δ) |
| front précocité/stabilité | v10 : Λ ∈ [2κ, 1+2κ] (géométrique, hauteurs) | **exact** dans le cadre intrinsèque des dates : c = 1 + 2κ, et c ≥ 3 (théorème C) |
| anticipation obligatoire | absent | **théorème B** |
| T0 contre monotonie | absent (la v10 opposait « masse » et « temps ») | **théorème E** |
| seuil de qualification contre A8 | absent (la qualification est une idée v11) | **théorème F**, balayage oracle |
| Q_1 dominée, non uniforme, retards post-cœur sans borne | v10 : « Q_κ plus tardive, bornes plus faibles » | domination prouvée, contre-familles chiffrées |
| garantie avant fusion F pour P_κ, échec de Q_1 | v10 : corollaire 4.1 (barres) | **proposition I** (points cœur, chapitre 7) |

## 2. Cadre et axiomes

### 2.1 Objets

- T = FULL_k vu comme espace : un point est p = (ν, r) avec b(ν) ≤ r < b(parent ν). Up_s(p) est l'ancêtre
  vivant à s ≥ h(p) = r. La rencontre m(p, q) est le plus petit s où Up_s(p) = Up_s(q) ; elle est
  ultramétrique : m(p, q) ≤ max(m(p, o), m(o, q)).
- **Profil couvrant** de x : R_x = {(ν, r) : dist(x, C_ν(r)) ≤ r}, stable par remontée. Muni des hauteurs et
  rencontres de T, c'est un arbre de fusion abstrait. Son code-barres H0 par la règle de l'aîné a une barre
  essentielle [α, ∞) et des barres finies [c_j, m_j) : une branche couvrante née à c_j meurt en rejoignant une
  branche plus ancienne (v10, ancrage § 2.2).
- **Profil qualifié** Π_m(R_x) : points de R_x dont l'amas discret compte au moins m sites (H_m du
  développeur). Π_1 = R_x.
- **Règle ancrée** : chaque site reçoit p_x = (o_x, e_x) ∈ T ; il est seul avant e_x, puis suit Up.
  Hauteur de réunion u(x, y) = m(p_x, p_y), u(x, x) = e_x.
- **P_κ∘Π** (κ ≥ 1) : p = un point le plus bas de Π(R_x), t = h(p),
  $$ e_x=\sup_{q\in\Pi(R_x)}\left(m(p,q)-\kappa(h(q)-t)\right)=\max\left(t,\max_j\left(m_j-\kappa(c_j-t)\right)\right),\qquad o_x=\mathrm{Up}_{e_x}(p), $$
  en rayon ; Q_κ∘Π est la même formule en rayon carré. Le retard de P_1 est la plus longue barre rivale
  (v10, corollaire 3.2).
- **Identifications** : H_m du développeur = Q_1∘Π_m (testé, § 10, accord exact avec `margin1`/`margin`) ;
  P_κ de la v10 = P_κ∘Π_1 (dates publiées de P_2 retrouvées) ; « first » du développeur = P_∞∘Π_m, c'est-à-dire
  la règle A5 de la v10 sur le profil qualifié (par définition).

### 2.2 Distances

- **Géométrique** : X et Y appariés, |x_i − y_i| ≤ ε.
- **Intrinsèque** : entrelacement δ de FULL décoré, c'est-à-dire φ, ψ décalant les rayons de δ, commutant à Up,
  ψφ = Up_{2δ}, φψ = Up_{2δ}, et transportant les profils (φ(R_x^X) ⊆ R_x^Y, ψ(R_x^Y) ⊆ R_x^X) et la
  qualification. P5 donne δ = ε en rayon. Restreint à un profil, c'est un entrelacement d'arbres de fusion
  (Morozov–Beketayev–Weber) : d_I(R_x^X, R_x^Y) ≤ δ.
- En rayon carré, la même perturbation donne un décalage non uniforme 2εr + ε² : c'est pourquoi H3 du
  développeur porte un δ = 2ε√Λ + ε² dépendant de l'échelle.

### 2.3 Axiomes

| Code | Énoncé |
| --- | --- |
| A1 | Laminarité : les partitions des sites entrés sont emboîtées (u ultramétrique). Toute règle ancrée la vérifie. |
| A2 | Fidélité : tout bloc d'au moins deux sites à r est inclus dans un seul amas discret E_C(r), et deux sites ne sont réunis qu'au plus tôt à la fusion FULL de leurs propriétaires. Pour une règle ancrée : p_x ∈ R_x (v10, T2). |
| A3^geo(Λ) | \|u^X(i, j) − u^Y(i, j)\| ≤ Λε en rayon, diagonale comprise. C0 : continuité seule. |
| A3^int(c) | Même borne avec la distance intrinsèque δ ; A3^int(c) ⇒ A3^geo(c) par P5. |
| A4 | Équivariance par renumérotation et isométrie (homothétie : u suit l'échelle). Variante A4^prof : équivariance par isomorphisme de profils. |
| A5_loc | = E_u : si une seule composante couvre x à r, x est dans son bloc à r. |
| A5_glob | Si le profil de x est une seule remontée (aucun rival, jamais), x entre à α(x). |
| A5_κ | κ-précocité (v10) : x entre à α dès que toute barre vérifie m_j − α ≤ κ(c_j − α). Pour κ = ∞ : entrée à α sauf ex æquo à α. |
| A6 | À k = 1, u est la liaison simple (rayon = demi-distance). |
| A7 | u ne dépend pas de mcs (mcs n'agit qu'à la condensation). |
| A8 | Cibles de l'utilisateur : T0 (deux triangles exacts, ABC \| DEF avant la fusion, mcs 2 et 3) ; Q1 (pont 1700, mcs 3) et Q1bis (mcs 2) : ABC \| DEF avant 1787,36 ; Q2 (mcs 2) : {x, a} \| {b1, b2}, lecture stricte sur [61 ; 75], structurelle à 75 ; Q3 (mcs 2–6) : x dans le filament sur [705 ; 790] ; Q4 (K = 3, mcs 2–3) : C, m, D sans cluster commun à 866,03, PQR \| P2Q2R2, chaîne CmD avant la racine ; Q-Π2 (mcs 9) : aucun cluster sur [705 ; 790]. |

Hypothèses structurelles utilisées par les caractérisations (ce ne sont pas des axiomes de l'utilisateur) :

- **Loc** (locale au profil) : p_x = Φ(Π(R_x)), transporté par le plongement, pour une application de profil Π
  fixée (Π = R_x pour la version brute).
- **Mon** (monotonie) : si P ⊆ P′ (sous-arbre de fusion remonté, mêmes hauteurs et rencontres) et
  min h(P) = min h(P′), alors e(P) ≤ e(P′). « Une couverture rivale de plus ne fait jamais entrer plus tôt. »
- **Causalité** : la décision « x entre à r » ne dépend que de FULL décoré tronqué aux rayons ≤ r.

## 3. Incompatibilités (trilemme généralisé)

### Théorème A (rappel v10, trilemme local)

A1 + A2 + A5_loc ⇒ non C0. *Preuve* (axiomes v10, théorème B). Sur X_± = {0, L ∓ δ, 2L}, K = 2, le site médian m
est couvert seul par la lentille de son côté proche sur [(L−δ)/2 ; (L+δ)/2[ : A5_loc le réunit à ce côté
avant L ; aucune composante ne couvre 0 et 2L avant L ; la réflexion échange les deux nuages. Le saut de
u(0, m) vaut au moins (L + δ)/2 pour un déplacement 2δ → 0. □ C'est le trilemme F2 du développeur.

### Théorème B (anticipation obligatoire) — nouveau

*Énoncé.* Aucune règle ancrée, fonction de FULL décoré, vérifiant A2, A4, A5_glob et C0 n'est causale.

*Preuve.* K = 2. Soit X_δ = {x = (0,0,0), a = (2,0,0), b_δ = (−2−2δ,0,0)}, 0 < δ < 1/2, et
Y = {x, a, b′ = (20,0,0)}. Dans X_δ, FULL_2 a la lentille xa née au rayon 1, la lentille xb_δ née à 1 + δ, et
leur fusion à 2 + δ (boule du triplet aligné). Dans Y : la lentille xa à 1, la lentille ab′ à 9, et la
fusion à 10, où naît aussi la lentille xb′ ; le profil de x dans Y est une seule remontée, donc A5_glob impose
e_x(Y) = 1, propriétaire la lentille xa. Pour r < 1 + δ, FULL décoré tronqué à r est le même dans X_δ et Y
(un seul nœud, la lentille xa, couvrant x et a ; b_δ et b′ non couverts). Une règle causale entre donc x à 1
dans la lentille xa pour tout δ > 0 ; a, sans rival, entre aussi à 1, d'où u_δ(x, a) = 1. À δ = 0 la réflexion
x ↦ −x fixe X_0 et x, et échange les deux lentilles couvrantes : par A2 et A4, le bloc de x à r < 2 est
invariant et inclus dans un seul amas, donc réduit à {x}, et u_0(x, a) ≥ 2. Le saut de u(x, a) en δ = 0
contredit C0. □

*Témoin oracle* (`recus_e2_anticipation.json`) : avec b = (−20,0,0) et b = (−16,12,0), FULL_2 est identique
sous le rayon 10 (un nœud au rayon 1), mais P_1 entre x à 2,0 et à 1,8167, Q_1 à 4,6904 et 4,2426 :
la date dépend d'événements postérieurs à elle. Avec b = (20,0,0), toutes les règles entrent x à 1.

*Portée.* Toute règle stable et fidèle « regarde vers le haut » de l'arbre. L'horizon nécessaire n'est pas
borné pour κ = 1 (un rival de persistance ρ/2 né à n'importe quel rayon retarde x) ; il est borné par
α + d_k/(2(κ−1)) ≤ 2α pour κ ≥ 2 (élagage exact de la v10, T3(e)). C'est un argument de coût et de localité
d'échelle contre κ = 1.

### Théorème C (front intrinsèque exact) — nouveau

*Cadre.* 𝒞_c : règles Loc (p = Φ(P), P profil abstrait), A4^prof, A5_glob, et A3^int(c) sur les dates :
|h(Φ(P)) − h(Φ(P′))| ≤ c·d_I(P, P′) pour tous profils P, P′. On note P(t, s, M) le profil à deux entrées de
rayons t ≤ s, de rencontre M ≥ s, et P_0(t) la remontée seule.

*Énoncé.* (a) 𝒞_c est vide si c < 3. (b) Pour tout Φ ∈ 𝒞_c et tout profil à deux entrées,
$$ h(\Phi(P(t,s,M)))\geq\max\left(t,\ M-\frac{c-1}{2}(s-t)\right). $$
(c) P_κ appartient à 𝒞_{1+2κ}, sa constante 1 + 2κ est atteinte, et elle réalise (b) avec égalité pour
c = 1 + 2κ. (d) Si Φ ∈ 𝒞_c vérifie Mon, alors h(Φ(P)) ≥ e_{P_κ}(P) pour **tout** profil P, κ = (c − 1)/2.
En particulier P_1 est la règle la plus précoce de constante minimale 3 parmi les règles monotones.

*Lemme C1 (deux entrelacements explicites).* Soit g = s − t et μ = (t + s)/2.
(i) d_I(P(t, s, M), P(μ, μ, M + g/2)) ≤ g/2 : envoyer chaque point (branche, r) sur (même branche, r + g/2),
dans les deux sens ; les recollements au-dessus de M et de M + g/2 sont respectés, et les composées sont les
remontées de g. (ii) d_I(P(t, s, M), P_0(t)) ≤ (M − s)/2 : φ envoie (branche, r) sur r + δ ; ψ envoie r sur
(branche aînée, r + δ) ; ψφ = Up_{2δ} exige que la branche cadette, prise au rayon s, ait rejoint l'aînée à
s + 2δ, soit δ ≥ (M − s)/2. □

*Preuve du théorème.* (b) Le profil symétrique P(μ, μ, M′), M′ = M + g/2, a l'automorphisme qui échange ses
deux entrées ; par A4^prof, Φ(P(μ, μ, M′)) est fixe, donc sur le tronc commun : hauteur ≥ M′. Par (i) et
A3^int(c) : h(Φ(P(t,s,M))) ≥ M + g/2 − c·g/2 = M − ((c−1)/2)(s − t). L'entrée est en outre au moins t
(fidélité). (a) Par (ii), A5_glob et A3^int(c) : h(Φ(P(t,s,M))) ≤ t + c(M − s)/2. Avec g = s − t = 1 et
ℓ = M − s, (b) et ce majorant donnent (3 − c)/2 ≤ ℓ(c − 2)/2 pour tout ℓ > 0, impossible si c < 3 (faire
ℓ → 0). (c) A4^prof : P_κ n'utilise que hauteurs et rencontres ; si p′ est un autre point le plus bas,
m(p, p′) ≤ e et l'inégalité ultramétrique donne la même date et le même propriétaire (H1 du développeur, valable
pour κ ≥ 1). A5_glob : sur une remontée seule, les termes valent h(q) − κ(h(q) − t) ≤ t. Lipschitz : soient
φ, ψ un δ-entrelacement de P et P′, p, p′ les points les plus bas, t, t′ leurs hauteurs ; t′ ≤ t + δ. Pour
q′ ∈ P′ : m′(p′, q′) ≤ m(ψp′, ψq′) + δ ≤ max(m(p, ψp′), m(p, ψq′)) + δ. Par définition de e = e_{P_κ}(P),
m(p, q) ≤ e + κ(h(q) − t) pour tout q, d'où m′(p′, q′) ≤ e + κ(h(q′) + δ − t) + δ, puis
m′(p′, q′) − κ(h(q′) − t′) ≤ e + δ + κ(δ + t′ − t) ≤ e + (1 + 2κ)δ. Symétrie. La constante est atteinte :
(t, s, M) → (t + δ, s − δ, M + δ) est à distance au plus δ et déplace M − κ(s − t) de (1 + 2κ)δ. Sur
P(t, s, M), e_{P_κ} = max(t, M − κ(s − t)), qui est la borne (b) pour c = 1 + 2κ. (d) Soit p le point le plus
bas de P et q une entrée rivale. Le sous-profil Up(p) ∪ Up(q) est P(t, h(q), m(p, q)) et a le même minimum ;
Mon et (b) donnent h(Φ(P)) ≥ m(p, q) − κ(h(q) − t). Le supremum sur les entrées rivales, avec h(Φ(P)) ≥ t,
est e_{P_κ}(P) (le sup de P_κ est atteint aux entrées : le long d'une remontée, m − κh décroît). □

*Remarques.* (1) La v10 avait, en géométrie et pour les hauteurs, le front Λ ∈ [2κ, 1 + 2κ] ; dans le cadre
intrinsèque des dates il est exact. (2) Le lignage le plus précoce est forcé par la continuité des hauteurs :
si Φ attache x à la branche cadette pour s proche de t, il doit en changer quand e(s) = s < M, et u(x, a) saute
pour un site a entré tôt sur la branche aînée. (3) La borne inférieure utilise des profils abstraits ; une
règle astreinte seulement à A3^geo sur les nuages réalisables peut entrer plus tôt (la v10 ne connaît que 2κ en
géométrie). (4) Le cadre est relatif à l'échelle : en rayon carré, le même énoncé fait de Q_1 la règle la plus
précoce de constante 3 **pour l'entrelacement en rayon carré**, qui n'est pas celui que produisent les
perturbations de points. L'échelle naturelle est le rayon, seule échelle où P5 est un décalage uniforme.

### Proposition D (hauteurs de P_κ∘Π en (1 + 2κ)δ) — correction de H3

*Énoncé.* Sous un δ-entrelacement intrinsèque, |u^X(x, z) − u^Y(x, z)| ≤ (1 + 2κ)δ, pour toute application de
profil Π transportée (Π_m l'est : un cardinal couvert ne peut que croître sous φ et ψ).

*Preuve* (vérificateur de l'ancrage v10, relue et transcrite). Par définition de e^X(x), le point ψ(p_x^Y),
de hauteur α^Y + δ ≤ α^X + 2δ et couvrant x, rencontre p_x^X avant e^X(x) + 2κδ ; en appliquant φ,
Up_{s+δ}(p_x^Y) = Up_{s+δ}(φ p_x^X) pour s ≥ e^X(x) + 2κδ. Si r = u^X(x, z), prendre
s = max(r, e^X(x) + 2κδ, e^X(z) + 2κδ) ≤ r + 2κδ : les remontées de p_x^Y et p_z^Y coïncident à s + δ, et les
dates Y sont ≤ r + (1 + 2κ)δ (théorème C). Symétrie. □ La borne (1 + 4κ)δ = 5δ de H3 est donc lâche pour κ = 1.
Mesure en rayon (`recus_e3_stabilite_rayon.json`, 6 000 paires) : P_1 2,22 ; P_2 3,83 ; core 2,00 (borne
atteinte) ; cover 50,35.

### Théorème E (T0 contre la monotonie) — nouveau

*Énoncé.* Une règle Loc (profil brut), A4^prof, A2 et Mon n'entre le site C des deux triangles équilatéraux
exacts (fixture `EQUILATERAL`, K = 2) qu'au rayon de la racine ou après. Elle viole donc A8-T0.

*Preuve.* Oracle (`recus_e1_cibles.json`, nœuds exacts) : les lentilles AC, BC, CD naissent toutes au niveau
1/2 (rayon 0,7071), AC et BC rejoignent ABC au niveau 2/3, CD ne rejoint la lignée d'ABC qu'à la racine,
niveau 3/2. Le sous-profil Up(AC) ∪ Up(CD) est P(t, t, M) symétrique, de rencontre M = racine : A4^prof donne
une date ≥ M ; Mon la transmet au profil complet. □

*Lecture.* La cible T0 dit exactement « une branche couvrante de plus (BC) fait entrer C plus tôt » : c'est la
négation de Mon. P_κ (tout κ), cover avec LCA et toute règle « attendre que l'ambiguïté se lève » échouent T0
pour cette raison. H_{k+1} y échappe en changeant d'application de profil (Π_3 efface les lentilles), ER0h en
votant (non monotone).

### Théorème F (seuil de qualification contre A8) — nouveau

*Énoncé.* Pour P_κ∘Π_m et Q_κ∘Π_m, tout κ ≥ 1 et tout m ≥ 1, à K = 2 : Q1bis est satisfaite si et seulement si
m = 3 ; Q2 (lecture structurelle à 75) si et seulement si m ≤ 2. Aucun membre de la famille ne satisfait les
deux. À K = 2, Q3 exige m ≤ 2 ; à K = 3, Q4 exige m ≤ 3. Donc m = k + 1 échoue Q2, Q3 et Q4.

*Preuve.* Le lignage de P_κ∘Π_m est celui de la première couverture qualifiée, indépendant de κ ; la date
décroît en κ (v10, T3(d)), donc κ = 1 est le cas le plus tardif. Niveaux exacts (oracle) :
Q1 : CD naît à 722 500 (rayon 850) et couvre {C, D} seulement jusqu'à la racine 3 194 656 (rayon 1 787,36) ;
CA et CB naissent à 999 956 et rejoignent AB dans ABC à 249 978 000 484/187 489 (rayon 1 154,68), qui couvre
3 sites. Si m ≤ 2, le lignage de C est CD (850 < 999,98) : ABC | DEF n'existe jamais. Si m = 3, Π_3 de C est
la seule remontée d'ABC : C entre à 1 154,68, sans rival qualifié. Si m ≥ 4, seule la racine est qualifiée.
Q2 : xa naît à 2 500 (rayon 50) et ne rejoint le reste qu'à la racine 90 625/16 (75,26) ; xb1 et xb2 naissent
à 3 625 et 14 517/4 et forment {x, b1, b2} à 540 976 005/148 484 (60,36), 3 sites. Si m ≤ 2, le lignage de x
est xa, et la date la plus tardive de la famille (Q_1, 67,37) précède 75 ; si m ≥ 3, le lignage est
{x, b1, b2}. Q3 : pour 3 ≤ m ≤ 9, la première couverture qualifiée de x est l'amas (453,47), la lentille xf1
ne devient qualifiée qu'à 700,50 ; pour m ≥ 10, x n'entre qu'à la racine. Q4 : pour m ≥ 4, la chaîne CmD
(3 sites) n'est jamais qualifiée, donc le bloc CmD n'existe jamais avant la racine (m = 4 : C entre dans son
tétraèdre à 866,03). Balayage : `recus_e5_seuil_m.json`
(m ≤ 6, κ ∈ {1, 2, 4, 1000}, deux échelles). □

*Témoin oracle* (`recus_e5_seuil_m.json`) : `_passent_Q1bis_et_Q2 = []` ; Q1bis passe exactement pour les huit
réglages m = 3 ; Q2 pour les seize réglages m ∈ {1, 2}.

*Pourquoi.* Q1 et Q2 opposent tous deux une structure de 2 sites née la première (pont CD ; paire xa) à une
structure de 3 sites née 15 à 17 % plus tard (lentilles CA, CB ; xb1, xb2). L'utilisateur choisit la grande dans
Q1 et la petite dans Q2. Ce qui les sépare n'est pas un effectif mais la **durée de coexistence des branches
multiples** : CA et CB couvrent C séparément pendant 154,7 (rayon), xb1 et xb2 couvrent x séparément pendant
0,117 (`recus_e6_scores.json`).

### Proposition G (seuil dépendant de mcs) — nouveau

Le choix H5 du développeur, m = max(k + 1, mcs), viole A7 par définition. Sur Q3 à mcs = 9 il donne m = 9 :
l'amas c0..c7 couvre x dès 453,47, compte alors 9 sites, et le bloc {x, c0, …, c7} existe de 453,47 à la
racine (`recus_e1_cibles.json`, règle `Hmcs9_sq`) : c'est exactement le cluster créé par un point de bord que
l'utilisateur a refusé (Q-Π2). Avec m = k + 1 = 3, indépendant de mcs, x rejoint aussi l'amas (549,09 en
rayon, 590,54 en niveau carré) : Q-Π2 échoue encore. La v10 avait déjà réfuté les règles à admission par mcs
(Q1bis, Q-Π2, VERDICT_FINAL § 1.2).

### Rappels v10 hors du champ A1–A8

À K ≥ 3, L + NP + E_u + respect du cœur sont incompatibles (théorème C des axiomes v10) ; fidélité à deux ordres
et emboîtement vertical s'excluent (théorème D des axiomes v10) ; les majorités à masse fixe violent E_u et sont
discontinues (théorème E des axiomes v10). Ces énoncés v10 restent valables ; ils ne sont pas redémontrés ici, et
leurs lettres ne renvoient pas aux théorèmes de ce rapport.

## 4. Tableau règles × axiomes

✓ prouvé ; ✗ réfuté (fixture citée) ; ~ structure juste mais en retard sur la fenêtre stricte ; ? ouvert.
Λ en rayon. Colonnes A8 calculées ici à l'oracle sauf ER0h (reçus v10, non rejoués).

| Règle | A1 | A2 | A3 (Λ, rayon) | A4 | A5_loc | A5_glob | A6 | A7 | T0 | Q1/Q1bis | Q2 | Q3 | Q4 | Q-Π2 |
| --- | :-: | :-: | --- | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| core | ✓ | ✓ | ✓ 2 (atteint) | ✓ | ✗ | ✗ | ✓ | ✓ | ✗ | ✗ | ✗ (x entre à 100) | ✓ | ✗ (pas de CmD) | ✓ |
| cover (LCA) | ✓ | ✓ | ✗ (50,35 mesuré ; F2) | ✓ | ✓ | ✓ | ✓ | ✓ | ✗ | ✗ | ✓ | ✓ | ✗ (CmD dès 692,8) | ✓ |
| Q_1 = H_1 du développeur | ✓ | ✓ | ✗ uniforme (31,8 à L = 10⁴) | ✓ | ✗ | ✓ | ✓ | ✓ | ✗ | ✗ | ~ (67,37) | ~ (744,18 > 705) | ~ (PQR à 899,5) | ✓ |
| P_1 = H_1 en rayon | ✓ | ✓ | ✓ 3 (2,22 mesuré) | ✓ | ✗ | ✓ | ✓ | ✓ | ✗ | ✗ | ~ (65,05) | ✓ (696,12) | ~ (PQR à 884,7) | ✓ |
| P_2 (défaut v10 du 30 sept.) | ✓ | ✓ | ✓ 5 (3,83 mesuré) | ✓ | ✗ | ✓ | ✓ | ✓ | ✗ | ✗ | ✓ (54,84) | ✓ | ✓ | ✓ |
| H_{k+1} (développeur, retenue) | ✓ | ✓ | 3δ en niveau carré, δ = 2ε√Λ + ε² (non uniforme en rayon) ; version en rayon : ✓ 3 | ✓ | ✗ | ✗ brut, ✓ qualifié | ✓ | ✓ | ✓ | ✓ | ✗ (x avec b1b2) | ✗ (x dans l'amas) | ✗ (C avec CPQR) | ✗ (9 points) |
| H_{max(k+1, mcs)} | ✓ | ✓ | comme H_{k+1} | ✓ | ✗ | ✗ | ✓ | ✗ | ✓ | ✓ | ✗ | ✗ | ✗ | ✗ |
| ER0h(1, 12) (verdict v10) | ✓ | ✓ | ? (conjecture ; pentes jusqu'à 102,7) | ✓ | ✗ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| fermeture qualifiée m = k + 1 (auditeur) | ✓ | ✗ (cinq points : tout à 100/9 < 36) | ✓ 1 | ✓ | — | qualifié | ✓ | ✓ | ✓ | ✓ | ✗ (x avec b1b2) | ✗ (x avec amas et filament) | ✗ (C avec tétraèdre) | ✗ |

Lectures. (1) Aucune règle de stabilité prouvée ne satisfait A8. (2) La famille P_κ satisfait A1–A7 et quatre
cellules sur six (P_2 les quatre en lecture stricte) ; elle échoue exactement les cellules « multiplicité »
(T0, Q1), comme le prédit le théorème E. (3) La qualification m = k + 1 inverse le tableau : elle gagne T0 et
Q1 et perd les quatre autres. (4) ER0h est la seule règle connue qui passe A8 ; sa stabilité est la question
ouverte centrale.

## 5. Question 2 : H_1 est-elle la règle la plus précoce ?

**Telle qu'écrite (niveau carré), non.** P_1, même formule en rayon, est fidèle, équivariante, A5_glob, liaison
simple à k = 1, stable en 3ε (dates et hauteurs), et entre chaque site au plus tard comme Q_1.
*Preuve.* Barre par barre, avec t ≤ c ≤ m : (t + m − c)² − (t² + m² − c²) = 2(c − t)(c − m) ≤ 0, nul
seulement si c = t ou c = m ; même lignée (même point le plus bas), donc propriétaire plus bas ou égal. □
Test (`recus_e7_dominance.json`, 800 nuages, k ≤ 4, m ∈ {1, k+1}) : 37 248 sites, 12 347 strictement plus tôt,
0 violation de date, de lignée ou de hauteur (101 356 paires). Fixture : {0, 2, 5}, K = 2, site médian :
Q_1 à 2,236 (niveau 5), P_1 à 2 (niveau 4 = cœur). Contre-famille {−2L, 0, 2} : Q_1 entre le site 0 à
√(2L + 2) (niveau 2L + 2) contre 2 pour P_1, et un déplacement unité de a change la date de Q_1 de 1,18 ; 3,24 ;
10,07 ; 31,79 pour L = 10, 100, 1 000, 10 000 (`recus_e3b_rival_lointain.json`) : aucune constante uniforme en
rayon.

**Version en rayon P_1 : oui, dans un cadre précis.** Par le théorème C, parmi les règles locales au profil,
équivariantes, A5_glob, monotones, de constante intrinsèque c : c ≥ 3, et la plus précoce est P_{(c−1)/2}. La
borne « e_i ≥ t_i + D_i » est donc **vraie en rayon** pour toute règle monotone de constante 3, avec D_i la plus
longue barre rivale mesurée en rayon. Pour Q_1, l'énoncé analogue ne vaut que pour l'entrelacement en rayon
carré (remarque (4) du théorème C), qui n'est pas celui des perturbations de points ; en rayon, Q_1 n'a même
pas de constante uniforme.

**Hors de ce cadre, des règles fidèles et stables entrent plus tôt** (testé, `recus_e1_cibles.json`) :

| Règle plus précoce | Hypothèse relâchée | Exemple |
| --- | --- | --- |
| P_κ, κ > 1 | constante 1 + 2κ au lieu de 3 | Q2 : x à 54,84 (P_2) contre 65,05 (P_1) |
| P_1∘Π_{k+1} (H_{k+1} en rayon) | locale au profil brut (elle lit les effectifs) | T0 : C à 0,8165 contre 1,2247 ; même constante 3 (prop. D) |
| règle non monotone de constante 3 | Mon | ouvert ; borne de profil de T0 : e_C ≥ 1,2247 − (3/2)(0,8165 − 0,7071) = 1,0606 |

Il n'existe donc pas de « règle la plus précoce » sans préciser l'application de profil (Π_1, Π_{k+1}, …) et la
monotonie ; H_1 et H_{k+1} sont incomparables (H_{k+1} plus tôt pour C de T0, H_1 plus tôt pour b1 de Q2 :
8,17 contre 60,36). Le choix κ = 1 n'est pas canonique : c'est l'extrémité « la plus stable et la plus tardive »
d'un front exact, avec un horizon d'anticipation non borné (théorème B, portée).

## 6. Question 3 : le seuil m = k + 1 est-il forcé par A8 ?

**Non : il est réfuté par A8** (théorème F, proposition G). Il passe T0 et Q1/Q1bis, échoue Q2, Q3, Q4 et Q-Π2,
pour tout κ et dans les deux échelles. La raison est structurelle : un seuil d'effectif efface toutes les
structures de k sites, alors que l'utilisateur en veut dans Q2 (paire {x, a}, paire serrée b1b2), Q3 (lentille
xf1 du filament) et Q4 (chaîne CmD de 3 sites à K = 3), et en refuse dans Q1 (pont CD). Les deux familles de
cibles ont les mêmes effectifs ; un critère d'effectif ne peut pas les séparer.

**Un autre critère sans mcs suffit-il ?** Pour A8 seul, oui : ER0h(1, 12) passe les 125 jugements ancrés sans
lire mcs (VERDICT_FINAL v10, deux implantations indépendantes, un seul auteur de jugement ; non rejoué ici).
Mais le théorème E impose qu'un tel critère soit **non monotone** (il doit compter les branches couvrantes),
et la v10 n'a pour ER0h qu'une conjecture de continuité avec des pentes locales jusqu'à 102,7. Ce qui sépare les
cellules est mesurable sur le profil seul : pour le site contesté de chaque cellule, le côté voulu par
l'utilisateur maximise « −naissance + λ × persistance interne » exactement pour λ ∈ ]0,969 ; 1,813[
(`recus_e6_scores.json` : Q1 exige λ > 0,969, Q4 λ < 1,813, Q2 λ < 87,4, Q3 λ < 44,4, T0 tout λ > 0). C'est une
piste de principe, pas une règle : ni dates, ni preuve de stabilité.

**Tension avec les mesures.** Sur les 128 scènes synthétiques et 44 trames LiDAR du développeur (G4, meilleur
IoU), H_{k+1} dépasse H_1 de 0,011 à 0,021 et HDBSCAN de 0,010 à 0,079 (HIERARCHIE_POINTS § 6, non rejoué). Le
meilleur IoU par groupe est un oracle optimiste qui ne pénalise guère l'envoi d'un point contesté vers la grande
structure ; les cellules A8 le pénalisent. Laquelle des deux autorités prime est une décision de l'utilisateur.

## 7. Question 4 : entrer après le temps de cœur

### Proposition H (bornes et exemples)

(i) **P_κ, κ ≥ 1 :** α ≤ e ≤ α + d_k/2 (v10, T3(b), via L6 : une composante qui couvre x à r rejoint celle qui
contient x avant r + d_k/2). À K = 2, α = d_k/2, donc e ≤ d_k : **jamais après le cœur**. En général
e ≤ 1,5 d_k, et **la borne est atteinte** : {0, 10, 20, 30} à K = 3, sites 10 et 20 : α = d_3 = 10, deux
composantes ponctuelles {10} et {20} les couvrent toutes deux dès 10 et fusionnent à 15, d'où e = 15
(oracle). Recherche aléatoire (`recus_e4_apres_coeur.json`, 1 500 nuages) : K = 2, 0 site sur 9 023 après le
cœur ; K = 3, 428 sur 9 023, maximum 1,5000 ; K = 4, 215 sur 7 839, maximum 1,3886.

(ii) **Q_1 (H_1 du développeur) : sans borne.** Famille {−2L, 0, 2}, K = 2, site 0 : barre rivale [L ; L + 1[,
date au niveau 1 + (L + 1)² − L² = 2L + 2, contre D_2 = 4 : rapport 5,5 ; 50,5 ; 500,5 pour L = 10, 100, 1 000
(oracle). La cause : en niveau carré, une barre de persistance 1 en rayon née au rayon L pèse 2L + 1.

(iii) **Qualification (H_{k+1}) : sans borne.** Un groupe isolé de k sites n'est jamais qualifié avant de
rejoindre autre chose ; Q2 : b1 entre à 60,36 contre d_2 = 16,03 ; maximum aléatoire 3,94 (K = 2).

### Proposition I (garantie avant la fusion parasite) — nouveau

*Énoncé.* Soit F un rayon et x un site tel que d_k(x) ≤ F et α(x) + d_k(x)/2 ≤ F. Pour tout κ ≥ 1, P_κ place x à
F dans le bloc de la composante de L_k(F) qui contient x. En particulier c'est vrai de tout point cœur à F de
rayon cœur d_k(x) ≤ 2F/3. Q_1 n'a pas cette propriété.

*Preuve.* e ≤ α + d_k/2 ≤ F (i). Le propriétaire est sur la lignée de première couverture ; par L6 appliqué à la
composante de première couverture au rayon α, cette lignée rejoint la composante qui contient x avant
α + d_k/2 ≤ F, et x ∈ L_k(F) car d_k ≤ F. Donc Up_F(p_x) est la composante de x. Contre-exemple pour Q_1 :
{−2L, 0, 2}, F = 2, site 0 : d_2 = 2, α + d_2/2 = 2 ≤ F, mais Q_1 l'entre à √(2L + 2) > 2, à cause d'un rival
né au rayon L > F. □

### Est-ce un défaut au regard du chapitre 7 ?

Le théorème 3 de la thèse mesure la fraction d'un amas dense récupérée **juste avant** la percolation du fond
(rayon F), en sémantique de couverture Θ^poly. Pour une hiérarchie laminaire de points, trois faits :

1. **Plafond.** Toute règle fidèle récupère au plus Θ^poly : ses blocs sont dans les amas discrets.
2. **Entrer après d_k n'est pas en soi le défaut ; entrer après F l'est.** P_κ récupère à F tous les points cœur
   de la composante géante tels que α + d_k/2 ≤ F (proposition I) : sa perte par rapport au cœur se limite à une
   couche d_k ∈ ]2F/3 ; F] et aux points dont la lignée de première couverture n'est pas encore absorbée. Q_1
   peut perdre un point cœur arbitrairement loin de F, à cause d'événements postérieurs à F : **défaut réel**,
   qui disqualifie le niveau carré au sens même de la thèse.
3. **La qualification n'est pas en cause ici.** Ses retards post-cœur portent sur des sites dont toutes les
   couvertures précoces ont au plus k sites : ils ne sont pas dans la composante géante. En revanche, Π_{k+1} ôte
   les petits rivaux et peut améliorer la récupération ; c'est cohérent avec les mesures agrégées du développeur,
   et c'est en tension avec A8 (§ 6).

Ce que la thèse ne dit pas : l'avantage de la sémantique poly sur le cœur (chapitre 7) est démontré pour l'objet
recouvrant, pas pour une projection laminaire. À K ≥ 3, P_κ ne respecte pas le cœur (fixture v10 {0, 2, 7, 10,
13} : à r = 5, le bloc cœur {2, 7} est scindé en {0, 2} | {7, 10, 13}, recalculé ici) ; la proposition I ne
donne Θ^{P_κ} ≥ Θ^{core} qu'avec une perte de rayon 2/3. Comparer Θ^{proj} et Θ^{core} asymptotiquement reste
ouvert.

## 8. Lecture critique

**Thèse.** (1) Les définitions 8 et le théorème 2 fixent un recouvrement ; le § 9.1 l'assume (partition des
(K−1)-simplexes, vote S_τ/T_x). Le vote par coupe n'est pas laminaire, et une majorité à masse fixe viole E_u
(théorème E des axiomes v10). (2) Le chapitre 7 compare des sémantiques d'appartenance, pas des hiérarchies de
points : son ordre poly ≥ core ne se transmet pas à une projection laminaire sans perte (§ 7). (3) En revanche,
l'intuition de la thèse — un point partage son unité entre faces avant de s'engager — est exactement ce que T0
exige et ce que la monotonie interdit (théorème E) : la thèse avait raison sur l'objet, et aucune règle stable
connue ne la réalise.

**Auditeur (fermeture qualifiée).** Stable en 1ε, équivariante, optimale parmi les ultramétriques dominées par
les échéances de co-couverture ; mais elle viole A2 (cinq points), et sur les cellules elle échoue Q2, Q3
(percolation de x vers l'amas **et** le filament), Q4 et Q-Π2 (`closure_m3`, `closure_m4` dans
`recus_e1_cibles.json`). Son seuil m est indépendant de mcs (A7 ✓), mais le théorème F s'applique à son
lignage de co-couverture comme à H_m : un seuil d'effectif ne sépare pas Q1 de Q2.

**Verdict v10 (ER0h).** Seule règle qui passe A8, sans mcs. Mais : paramètres (η, κ) = (1, 12) choisis dans une
région établie par grille sur les cellules mêmes qu'elle passe ; continuité conjecturale ; pentes locales
jusqu'à 102,7 (aucune constante uniforme) ; règle locale aux nœuds de FULL et non au seul profil (les poids
dépendent des bornes de vie des nœuds), ce qui la place hors du théorème C sans rien prouver pour elle.

**Développeur (H_m).** (1) H_1 est le Q_1 de la v10, que la v10 jugeait dominé par P_κ ; la note ne cite pas
ce travail. (2) Le niveau carré rend la règle plus tardive partout, non uniforme en rayon, et capable d'entrer
un point cœur après la fusion parasite. (3) κ = 1 est l'extrémité du front, avec un horizon non borné ; la v10
recommandait κ = 2. (4) La borne de hauteur 5δ est lâche (3δ). (5) m = k + 1 et m = max(k+1, mcs) sont réfutés
par A8 ; le second viole aussi A7, conclusion centrale du verdict v10. (6) Points forts : fidélité, équivariance,
stabilité prouvée, accord exact natif contre oracle (porte G4), et un gain agrégé mesuré sur HDBSCAN.
(7) Version en rayon ajoutée à 21 h 48 UTC : elle corrige (2) ; sa docstring garde 5ε pour les hauteurs (3ε
suffit) ; `reference_radius_rules` calcule les racines à 120 chiffres mais additionne dans le contexte décimal
global, si bien qu'aux égalités exactes e = naissance d'un ancêtre elle rend l'enfant, non vivant à la coupe
fermée (145 cas, dont C de T0 : nœud ABC au lieu de la racine à e = √(3/2)) ; dates et hauteurs u n'en sont pas
affectées (`recus_e8_rayon_croise.json`, `recus_e8b_diag.json`).

## 9. Ce qui reste ouvert

1. **A8 + A3 uniforme.** Existe-t-il une règle fidèle, équivariante, A5_glob, de constante uniforme, qui passe
   les six cellules ? Nécessaire : non monotone (théorème E), non réductible à un seuil d'effectif (théorème F),
   anticipante (théorème B). Piste : départage des côtés par « −naissance + λ × persistance interne » avec une
   marge en cône, λ ∈ ]0,969 ; 1,813[ ; la persistance interne d'un côté est 2-lipschitzienne en
   bottleneck global, mais sa restriction à un sous-arbre peut sauter quand une fusion traverse le point
   considéré : la stabilité est à démontrer, pas acquise.
2. **Théorème C sans Mon.** Une règle locale au profil, non monotone, de constante 3, peut-elle entrer plus tôt
   que P_1 sur un profil à trois entrées ? Borne connue sur T0 : 1,0606 contre 1,2247.
3. **Front géométrique.** Seul A3^int est caractérisé ; en géométrie, la v10 ne sait que Λ ∈ [2κ, 1 + 2κ].
4. **Existence d'une règle la plus précoce** dans le cadre décoré (applications de profil qui lisent les
   effectifs) : H_1 et H_{k+1} sont incomparables ; je conjecture qu'il n'en existe pas.
5. **Chapitre 7 projeté.** Comparer asymptotiquement Θ^{P_κ} et Θ^{core} ; la proposition I n'en donne qu'une
   minoration avec perte de rayon 2/3.
6. **Respect du cœur à K ≥ 3 avec stabilité** (question I7 de la v10, 1 ≤ κ < 5) : toujours ouverte.

## 10. Reproduction

Depuis `/workspaces/E-HGP/build/v11-points-math/axiomes/`, avec `PYTHONDONTWRITEBYTECODE=1 python3 -B` (aucun
bytecode écrit dans le dépôt). Python 3.12.1, numpy 2.5.3. CPU total mesuré : environ 4 minutes (E3 : 85 s, le reste moins de 25 s chacun).

| Commande | Reçu | Résultat |
| --- | --- | --- |
| `e0_sanity.py 20261003 300` | `recus_e0_sanity.json` | 304 nuages, 1 822 comparaisons H_1/H_{k+1} contre `margin1`/`margin`, 0 désaccord (5,1 s) |
| `e1_cibles.py` | `recus_e1_cibles.json` | cellules A8 et fixtures, toutes règles, lignes de temps des blocs (0,2 s) |
| `e2_e5.py e2` | `recus_e2_anticipation.json` | anticipation (théorème B) |
| `e2_e5.py e3 20261003 2000` | `recus_e3_stabilite_rayon.json` | 6 000 paires, max \|Δu\|/ε : P_1 2,2196 ; P_2 3,8329 ; Q_1 2,4783 ; core 2,0000 ; cover 50,3535 (85,5 s) |
| `e2_e5.py e3b` | `recus_e3b_rival_lointain.json` | Q_1 non uniforme : 1,18 ; 3,24 ; 10,07 ; 31,79 |
| `e2_e5.py e4 20261004 1500` | `recus_e4_apres_coeur.json` | entrées après le cœur (proposition H) (21,6 s) |
| `e2_e5.py e5` | `recus_e5_seuil_m.json` | théorème F : aucun (m, κ, échelle) ne passe Q1bis et Q2 |
| `e6_scores.py` | `recus_e6_scores.json` | piste λ ∈ ]0,969 ; 1,813[ |
| `e7_dominance.py 20261005 800` | `recus_e7_dominance.json` | P_1 domine Q_1 : 0 violation, 12 347 / 37 248 strictement |
| `e8_rayon_croise.py 20261006 300` | `recus_e8_rayon_croise.json` | P_1∘Π_m contre `reference_radius_rules` du développeur : 0 écart de date (14 408 sites) ni de hauteur (39 700 paires) |
| `e8b_diag.py 20261006 300` | `recus_e8b_diag.json` | 145 propriétaires différents, tous aux égalités exactes e = naissance d'un ancêtre |

Fichiers : `hk.py` (règles H_κ∘Π_m en rayon et en niveau carré, fermeture qualifiée, ultramétriques, blocs ;
rayons en `Decimal` à 80 chiffres, quasi-égalités comptées : 0 dans tous les reçus), `e0_sanity.py`,
`e1_cibles.py`, `e2_e5.py`, `e6_scores.py`, `e7_dominance.py`, `e8_rayon_croise.py`, `e8b_diag.py`. Écart déclaré à la consigne « n ≤ 9 » : la
fixture Q3 (filament) a 14 sites à k = 2 ; son calcul (455 boules minimales) prend 0,08 s, moins qu'un nuage de
9 sites à k = 4.

Limites. Les théorèmes B, C, E, F et la proposition I ont des preuves complètes ci-dessus ; C est énoncé dans le
cadre intrinsèque (profils abstraits), pas pour toute règle géométriquement stable. Les cellules A8 sont jugées
par ma propre lecture des fenêtres (`verdicts` dans `e1_cibles.py`), pas par le juge v10 ; ER0h et les mesures
G4 sont cités, non rejoués. Les petites tailles servent ici d'oracle de correction, jamais de pente ; aucune
conclusion de coût n'en est tirée.
