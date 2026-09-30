# De la tour FULL à une hiérarchie laminaire de points : masses conservées et sélection

30 septembre 2026. Approche « masses conservées (chapitre 9) et sélection » du verrou posé par l'utilisateur :
comment passer de la tour FULL complète à la hiérarchie laminaire de points la plus pertinente possible.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=verrou_points_masses_selection
public_status=not_claimed
```

Aucun moteur modifié. Le worktree partagé est lu seulement. GCP non utilisé. Code, fixtures et sorties exactes
dans ce dossier ; exports natifs et brouillons dans `/tmp/mhgp10-verrou-points/masses_selection/`.

## 0. Réponse courte

1. **Oui, à une condition.** Une hiérarchie dure et laminaire se dérive canoniquement d'une masse conservée si
   l'unité de masse est la **branche de FULL** et si la masse s'**accumule continûment** pendant la couverture.
   Chaque point répartit une masse unité sur son arbre de couverture (définition 8 de la thèse), au prorata de sa
   persistance couverte en λ = r^(−z). Il rejoint la première branche qui porte au moins la moitié de sa
   persistance totale, passé et futur compris, puis il suit les ancêtres (§ 3). La hiérarchie obtenue est
   laminaire, fidèle aux amas discrets et **continue en la configuration** (théorèmes 5 et 7 bis, prouvés).
2. **Non, avec une activation par marches.** Théorème 8 : sur la famille à trois sites de F2, toute majorité à
   masse fixe activée par marches, quels que soient les poids (niveau, mort, type d'incidence) et l'univers
   d'atomes (boules du catalogue, faces du § 9.1, unités de branche), soit ne forme jamais la paire précoce, soit
   saute. Aucune fonction de poids de cette classe n'est donc à la fois précoce et continue. La durée de branche
   par marches passait toutes les fixtures connues : le théorème fournit sa fixture d'échec, {0, 937 / 938, 2 000}.
3. **Le prix de la stabilité est un théorème.** Théorème 9 : sur la famille à trois sites de F2, une règle fidèle
   aux amas discrets, équivariante et L-lipschitzienne ne peut pas réunir le point du milieu à sa première
   couverture tant que son avance relative sur la branche concurrente est inférieure à 1/(2L−1). La règle
   progressive paie ce prix continûment ; z règle le compromis.
4. **Toutes les fixtures connues passent pour z\* ≈ 1,845 ≤ z ≤ 6.** Le bas de la fenêtre vient des cinq sites.
   Le haut vient de F2 999/1001 avec la tolérance 2ε de core : à z = 7 et 8, ρ(0, m) change de 46,5 et 110 pour
   un déplacement de 2, continûment mais avec amplification. F1, F2, les huit paires, le contact
   coquille/intérieur, les entrées internes K3/K5, le fantôme K5 et la quasi-égalité K3 des bras A5/A6 passent
   pour z = 2, 3, 4. La règle ne dépend ni du catalogue, ni de Kmax.
5. **La sélection n'a pas besoin d'une hiérarchie de points.** Condensation et EOM se font sur FULL avec les masses
   fractionnaires ; un vote à l'antichaîne choisie donne la partition (proposition 7 transposée). Projeter d'abord
   change la sélection, dans les deux sens (fixtures S1, S2). La hiérarchie majoritaire est cohérente avec le vote.
6. **Les masses du § 9.1 ne sont pas la bonne unité** pour une hiérarchie : biais de degré (les paires précoces
   échouent), perte des entrées internes en convention Gabriel (T_x = 0), activation par marches.
7. **Multi-K** : tout se transporte sur une tranche monotone (r croissant, K décroissant). L'union libre des
   ordres reste impossible (F3) : il faut choisir une tranche avant de parler de masses.

## 1. Objets et notations

X ⊂ R³ est un nuage de n sites distincts. K ≥ 2 est fixé. À K = 1, les amas discrets forment déjà une partition
(remarque 3 de la définition 8) : il n'y a rien à projeter. β = r² désigne un **rayon carré** exact.

L'arbre FULL_K est l'arbre de fusion des composantes de `L_K(β) = {y : |X ∩ B̄(y, √β)| ≥ K}`. Un nœud v a une
naissance b_v et une mort d_v = b_parent(v), avec d = +∞ pour la racine. Coupe fermée : v est vivant à β si
b_v ≤ β < d_v. Un plateau exact est un seul événement. `anc(v, β)` est l'ancêtre de v vivant à β.

**Définition 1 (couverture).** La branche v couvre x à β ∈ [b_v, d_v) si dist(x, C_v(β)) ≤ √β, où C_v(β) est la
composante représentée par v. On note c_x(v) le premier tel β, V_x l'ensemble des v pour lesquels il existe, et
T_x = {(v, β) : v ∈ V_x, c_x(v) ≤ β < d_v} l'**arbre de couverture** de x. À la coupe β, la famille des v
vivants de V_x avec c_x(v) ≤ β est exactement l'ensemble des amas discrets X ∩ δ_r(C) qui contiennent x.

**Lemme 1.** V_x est clos vers le haut et c_x(parent(v)) = b_parent(v) pour v ∈ V_x.

*Preuve.* Si v couvre x à β < d_v, la composante du parent à b_parent contient C_v(β), donc
dist(x, C_parent) ≤ √β ≤ √b_parent. ∎

**Lemme 2 (calcul sans énumération).** Soit W_x l'ensemble des témoins forts de x : boules critiques de
population fermée ≥ K, de p + q_min ≤ K, contenant x (intérieur ou coquille). Le nœud d'un témoin w est la
composante de son centre à son propre niveau ℓ(w) (le `ball_node` natif). Alors, pour tout nœud v,
`c_x(v) = max(b_v, min{ℓ(w) : w ∈ W_x, nœud(w) ⪯ v})` dès que cet ensemble est non vide, et v ∉ V_x sinon.

*Preuve.* Le lemme de couverture du catalogue (addendum de l'auditeur continu, preuve complète) dit, à chaque
coupe β : x est couvert par la composante C ssi un témoin fort de niveau ≤ β a son centre dans C. Le centre
d'un témoin w est dans `anc(nœud(w), β)` pour β ≥ ℓ(w). Donc v couvre x à β ∈ [b_v, d_v) ssi un témoin
vérifie ℓ(w) ≤ β et nœud(w) ⪯ v. Le plus petit tel β est la formule annoncée. Un témoin placé sous un enfant a
un niveau < b_v : le maximum rend alors b_v, ce qui est la continuation du lemme 1. ∎

Contrôle : les arbres et couvertures obtenus par Γ_K exhaustif et par export natif plus lemme 2 coïncident sur
les 34 nuages de fixtures (§ 6) : même multi-ensemble (naissance, mort), même signature (c_x(v), d_v) pour chaque
point, mêmes attaches progressives, 318 contrôles exacts.

## 2. L'unité de masse : la persistance couverte d'une branche

**Définition 2.** Soit φ(β) = β^(−z/2) = r^(−z), l'échelle λ de la tête EOM, avec φ(+∞) = 0. L'**unité de
participation** de x dans la branche v ∈ V_x est `w_x(v) = φ(c_x(v)) − φ(d_v) > 0`. La masse totale
`W_x = Σ_{v ∈ V_x} w_x(v)` est la longueur, en unités λ, de l'arbre de couverture T_x.

Lecture : x participe à l'amas discret porté par v du rayon √c_x(v) au rayon √d_v. L'unité w_x(v) est la durée
de cette participation en unités de densité : c'est exactement le terme λ_max − λ_min de la stabilité de
Campello et al. La masse unité de x se répartit au prorata de ses participations, passées et futures. C'est la
généralisation, à toutes les branches et avec leurs continuations, du contrôle « durée couverte des feuilles » de
l'audit (sections 10 et 11) ; la majorité à dénominateur fixe de l'audit (section 8) en est le cas des atomes
activés par marches.

**Proposition 3 (télescopage des continuations).**
(a) Le long d'une chaîne de nœuds de V_x où chacun a exactement un enfant couvert, la somme des unités vaut
φ(c) − φ(d_haut) : elle ne dépend pas du découpage.
(b) Insérer une continuation dans une branche v, par fusion à γ avec une branche g qui ne couvre pas x, ne change
ni W_x ni aucune masse accumulée du § 3. Si g couvre x, W_x augmente de w_x(g) ≤ φ(b_g) − φ(d_g), la persistance
de g en λ, qui tend vers zéro avec la persistance du fantôme.
(c) Exclure un ancêtre retire φ(b) − φ(d) à W_x ; compter chaque continuation comme une unité nouvelle double
sa masse. Ce sont les deux défauts relevés à la section 11 de l'audit ; l'unité de branche n'a ni l'un ni l'autre.

*Preuve.* (a) φ(c) − φ(d_1) + φ(d_1) − φ(d_2) + … = φ(c) − φ(d_haut). (b) Le découpage de v en v′ = [b_v, γ) et
v″ = [γ, d_v) remplace w_x(v) par w_x(v′) + w_x(v″) = w_x(v) si c_x(v) < γ, et par w_x(v″) = w_x(v) sinon ; la
masse accumulée d'un nœud vivant (définition 3) est une somme télescopique de ces termes. Le second énoncé est
la définition de w_x(g). (c) Immédiat. ∎

**Proposition 4 (indépendance du catalogue et de Kmax).** V_x, c_x et w_x ne dépendent que de FULL_K et de la
relation géométrique de couverture. Deux univers de témoins qui certifient les mêmes couvertures donnent les
mêmes unités. L'univers fort ne dépend pas de Kmax.

*Preuve.* Définition 1 et lemme 2 ; l'admission p + q_min ≤ K ne fait pas intervenir Kmax. ∎

Conséquences vérifiées : dans le contact coquille/intérieur, le nuage de base a 15 incidences de témoins forts et
le nuage déplacé 6, mais les unités de branche changent seulement par la variation continue des niveaux. Sur six
sites alignés, Kmax = 2 et Kmax = 4 donnent des catalogues de 9 et 14 boules, mais les mêmes 10 incidences fortes,
les mêmes unités et les mêmes attaches (fixture KM, § 8).

## 3. Deux activations d'une même masse, et la majorité

**Définition 3 (activations).** Pour v vivant à β, notons base_x(v) la somme des unités des descendants stricts
de v dans V_x.

- **Progressive** : `A_x(v, β) = base_x(v) + (φ(c_x(v)) − φ(β))_+`. La masse entre au fil de la couverture.
- **Par marches** : `S_x(v, β) = Σ w_x(u)` sur les u ⪯ v de V_x avec c_x(u) ≤ β. Chaque unité entre entière à
  sa première couverture ; c'est le mode des atomes de l'audit (majorités A3/A4, durée de feuille).

La **réserve** `ρ_x(β) = 1 − Σ_{v vivant} A_x(v, β)/W_x` est la part future de la participation. Pour toute
coupe, la masse active des nœuds plus les réserves vaut n : la masse totale est conservée.

**Définition 4 (attache majoritaire).** Progressive : `t(x) = min{β : ∃ v vivant, A_x(v, β) ≥ W_x/2}` et o(x)
est cette branche. Par marches : même définition avec S_x(v, β) > W_x/2. À β ≥ t(x), le bloc de x est
anc(o(x), β) ; avant t(x), x est un singleton de complétion, jamais un bloc collectif « bruit ».

Forme close. Sur une chaîne de continuations issue d'un nœud a avec base_x(a) < W_x/2, le franchissement a lieu à
`φ(t) = base_x(a) + φ(c_x(a)) − W_x/2`, s'il est avant la fin de la chaîne ; sinon la majorité est acquise par
saut à une fusion. Pour un point couvert par une seule lignée, t = 2^(2/z) c_x : le retard de confiance vaut
2^(1/z) en rayon (1,41 à z = 2, 1,26 à z = 3, 1,19 à z = 4), contre au plus 2 pour core.

## 4. Résultats positifs

**Théorème 5 (exclusivité, existence, laminarité).** Pour les deux activations :
(i) à tout β fini, au plus une branche vivante porte A_x ≥ W_x/2 (resp. S_x > W_x/2) ;
(ii) si v est vivante à β et β′ ≥ β, alors A_x(anc(v, β′), β′) ≥ A_x(v, β) ;
(iii) t(x) est fini et atteint ;
(iv) les partitions P_β (blocs anc(o(x), β), singletons avant t(x)) sont emboîtées.

*Preuve.* (i) Les masses accumulées des branches vivantes portent sur des parties disjointes de T_x. Leur somme
vaut W_x − R(β), où R(β) est la longueur de la partie de T_x postérieure à β. Si une branche vivante couvre x,
R(β) ≥ φ(β) > 0 ; sinon toutes les masses sont nulles. Deux branches à ≥ W_x/2 donneraient une somme ≥ W_x.
Par marches, deux masses > W_x/2 dépasseraient W_x. (ii) Le sous-arbre de anc(v, β′) contient celui de v et
A_x est croissante en temps et en sous-arbre. (iii) La racine est dans V_x (lemme 1) et
A_x(racine, β) = W_x − φ(β) → W_x. La fonction F(β) = max_v A_x(v, β) est croissante et continue à droite
(continue pendant la vie des nœuds, sauts vers le haut aux fusions en coupe fermée) : {F ≥ W_x/2} est un
intervalle [t, +∞). Par marches, F est en escalier continue à droite. (iv) C'est le dispositif d'ancrage : deux
points réunis à β ont des propriétaires de même ancêtre à β, donc à tout β′ ≥ β. ∎

L'égalité « ≥ W_x/2 » est licite en progressif : le futur de T_x est strictement positif, donc le seuil 1/2 reste
exclusif (i). Une réunion à égalité exacte entre deux branches ne peut pas se produire à temps fini.

**Proposition 6 (fidélité à la définition 8).** Pour β ≥ t(x), x appartient à l'amas discret X ∩ δ_r(C) de la
composante C = anc(o(x), β). Chaque bloc est donc contenu dans l'amas discret de sa composante.

*Preuve.* A_x(o(x), t(x)) ≥ W_x/2 > 0. Une masse accumulée positive dans v impose que v couvre x : si
base_x(v) > 0, un enfant couvre x, donc v aussi (lemme 1) ; sinon c_x(v) ≤ β. La couverture passe ensuite aux
ancêtres (lemme 1). ∎

**Théorème 7 (continuité à combinatoire fixée).** Fixons la forme de l'arbre, les ensembles V_x et les égalités
c_x(v) = b_v des continuations. Si les niveaux b_v, d_v et c_x(v) dépendent continûment d'un paramètre p, alors
t(x) et les hauteurs de réunion `u(x, y) = max(t(x), t(y), b_LCA(o(x), o(y)))` de la règle **progressive**
dépendent continûment de p.

*Preuve.* Les masses base_x(v) et W_x sont des sommes finies de valeurs de φ en ces niveaux : elles sont
continues. Soit t0 = t_{p0}(x) et v0 = o(x).

1. *Croissance stricte.* Si A_x(v, β) > 0, v couvre x (preuve de la proposition 6), donc A_x(v, ·) croît
   strictement en β, de dérivée −φ′(β) > 0. F ne peut donc pas rester égale à W_x/2 sur un intervalle.
2. *t_p ≤ t0 + ε.* Comme v0 couvre x à t0, il existe ε′ ≤ ε avec t0 + ε′ < d_v0 et
   A_{p0}(v0, t0 + ε′) > W_{p0}/2 + γ, γ > 0. À β fixé strictement intérieur à la vie de v0, A_p(v0, β) est
   continue en p ; l'inégalité reste vraie près de p0.
3. *t_p ≥ t0 − ε.* Si les niveaux se déplacent d'au plus δ, toute branche vivante à β pour p a une masse au plus
   égale, à κ(p) près avec κ → 0, à celle de sa lignée pour p0 à β + δ (monotonie (ii) du théorème 5). Donc
   F_p(t0 − ε) ≤ F_{p0}(t0 − ε/2) + κ(p) < W_p/2 pour p proche de p0, car F_{p0}(t0 − ε/2) < W_{p0}/2.
4. *Propriétaire.* À t0, toute autre branche vivante u vérifie A_x(u, t0) ≤ W_x/2 − φ(t0) (preuve de (i)) :
   elle reste sous le seuil près de p0. Le propriétaire est donc v0, ou, si le franchissement a lieu exactement
   à une fusion de v0 avec la limite d'un enfant c, c ou v0 à des dates voisines. Dans ce dernier cas,
   b_LCA(c, o(y)) et b_LCA(v0, o(y)) ne diffèrent que si o(y) ⪯ c, et alors les deux hauteurs valent t0 à la
   limite. u(x, y) est donc continue. ∎

Le théorème est faux pour l'activation par marches, pour cover et pour les bandes (théorème 8). Il s'étend
aux changements de combinatoire de FULL :

**Théorème 7 bis (continuité globale).** Pour K ≥ 2 et z > 0 fixés, les dates t(x) et les hauteurs u(x, y) de la
règle progressive sont des fonctions continues de la configuration, sur l'ouvert des nuages de n points distincts
étiquetés.

*Preuve.* On décrit tout sans nœuds, par Γ_K (théorème 2 du manuscrit). Les sommets sont les K-parties F, nées à
β(F), le carré du rayon de leur plus petite boule. Soit m(F, F′) le niveau minimax d'un chemin de Γ_K entre F et
F′ (maximum des naissances et des niveaux de (K+1)-parties le long du chemin, minimisé sur les chemins). C'est une
ultramétrique ; elle vaut le niveau où les composantes de F et F′ fusionnent. Les β(F) sont continus en la
configuration, et m l'est aussi : minimum, sur un ensemble fini fixe de chemins, de maxima de fonctions continues.

1. *Couverture.* Par le théorème 2, une composante couvre x à β ssi elle contient un sommet F ∋ x né avant β.
   Pour un ensemble S de K-parties contenant x, notons Λ(S, β) = ∫_0^β N_S(s) |dφ(s)|, où N_S(s) compte les
   classes de {F ∈ S : β(F) ≤ s} sous la relation m ≤ s. La masse accumulée d'une composante Q à β est
   A(Q, β) = Λ(Q ∩ V, β), V = {F ∋ x}, et W_x = Λ(V, +∞).
2. *Forme close.* Si S est dans une seule classe à β, Λ(S, β) = Σ_{F ∈ S} φ(β(F)) − Σ_{e ∈ MST(S)} φ(e) − φ(β),
   pour l'arbre couvrant minimal de S sous m (règle de l'aîné : chaque arête tue une barre). La multiplicité triée
   des poids d'un arbre couvrant minimal est une fonction continue des poids ; donc Λ(S, β) et W_x sont continues
   en la configuration, uniformément sur les horizons bornés inférieurement.
3. *Monotonie.* m étant global, ajouter un sommet à S ne réunit jamais deux classes : N_S croît avec S, donc Λ aussi.
4. *Encadrement.* Si les niveaux et m bougent d'au plus δ entre p et p0, toute classe Q_p à β est contenue dans une
   classe Q′ de p0 à β + δ. Par 2 et 3, A_p(Q_p, β) ≤ A_{p0}(Q′, β + δ) + κ(δ), avec κ(δ) → 0. Donc
   G_p(β) ≤ G_{p0}(β + δ) + κ(δ), pour G = max des masses des composantes, et symétriquement.
5. *Dates.* G_{p0} croît strictement là où elle est positive (la composante qui la réalise couvre x). Avec
   l'encadrement et la continuité de W_x, la preuve des étapes 2 et 3 du théorème 7 donne t_p → t_{p0}.
6. *Hauteurs.* u(x, y) = max(t_x, t_y, m(F_x, F_y)) pour des sommets F_x, F_y des composantes propriétaires. À
   t0, toute autre composante a une masse ≤ W_x/2 − φ(t0) ; par l'encadrement, le propriétaire pour p est dans la
   lignée de celui de p0 à un temps voisin, donc m_{p0}(F_x(p), F_x(p0)) est voisin de t0 au plus. Par l'inégalité
   ultramétrique, m(F_x, F_y) ne change que si elle est dominée par max(t_x, t_y) : u est continue. ∎

La preuve utilise l'univers de toutes les K-parties comme objet mathématique ; le calcul, lui, n'en a pas besoin
(lemme 2). La question d'une constante de Lipschitz uniforme reste ouverte (C2, § 14).

**Remarque (les deux paramètres).** Le seuil θ = 1/2 est le plus petit seuil exclusif : θ < 1/2 autorise deux
propriétaires, θ > 1/2 retarde toutes les entrées sans rien gagner en exclusivité. L'exposant z est le seul
réglage. Quand z → +∞, pour une configuration fixée dont la première couverture de x est unique, t_z(x) tend vers
α_K(x)² et o_z(x) vers la composante de première couverture : W_x ≈ φ(α_K(x)²) car les autres unités sont
d'ordre φ(c_2) = o(φ(α_K(x)²)), c_2 étant la deuxième date de couverture, et t_z = 2^(2/z) α_K(x)² (1 + o(1)).
La famille progressive, continue pour chaque z fini, **tend donc vers A5**, dont la discontinuité n'apparaît qu'à
la limite. Quand z → 0, le retard 2^(2/z) diverge : les entrées deviennent arbitrairement tardives.

## 5. Résultats négatifs

**Théorème 8 (dichotomie des règles datées par événements).** Soit H > 0, e un vecteur unitaire, K = 2, et
X_δ = {0, (H − δ)e, 2He} pour δ ∈ [0, H) ; notons m le site du milieu. L'arbre FULL_2 a trois nœuds : la
feuille gauche née à α_L = ((H − δ)/2)², la feuille droite née à α_R = ((H + δ)/2)², la racine née à μ = H².
Considérons une règle laminaire dont les dates d'attache sont des niveaux d'événements {α_L, α_R, μ} et qui est
invariante par la réflexion qui échange 0 et 2He (étiquettes comprises). Alors :
(a) ou bien 0 et m ne sont réunis qu'à μ, pour tout δ ;
(b) ou bien le rayon de réunion ρ(0, m) est discontinu en δ.

Cette classe contient cover sans départage par identifiant, toutes les bandes η ≥ 0 (A5 est η = 0), et **toute
majorité à masse fixe activée par marches**, quels que soient les poids fonctions du niveau, de la mort et du type
d'incidence, et quel que soit l'univers d'atomes : boules du catalogue (A3, A4), faces du § 9.1 dans les deux
conventions, unités de branche. Pour une majorité par marches, la majorité ne peut devenir vraie qu'à
l'activation d'un atome ou à une fusion, donc à un niveau de {α_L, α_R, μ}.

*Preuve.* À δ = 0, la réflexion échange les deux feuilles ; une règle invariante ne peut attacher m à l'une
avant μ, donc t(m) = μ. La hauteur ρ(0, m)² est le maximum de dates de {α_L, α_R, μ} et d'une naissance
d'ancêtre commun : elle appartient à {α_L, α_R, μ}. Sur [0, δ1], ces trois fonctions sont continues et
μ − α_R ≥ min_{[0, δ1]}(μ − α_R) > 0. Si ρ(0, m)² < μ en un δ1, la fonction passe de la courbe μ (en 0) à une
courbe inférieure : elle saute d'au moins ce minimum. ∎

Certificats exacts (§ 7, grille g = 2H = 128 000, un point déplacé d'une unité) : A4 et A5 sautent de 64 000 à
31 999,5 entre δ = 0 et 1 ; la bande η = 1/8 saute de 64 000 à 30 117,5 entre δ = 3 764 et 3 765 ; la majorité
de branche par marches saute de 64 000 à 30 015 entre δ = 3 969 et 3 970 ; cover saute au passage de l'égalité
par son départage ; A3 (uniforme) ne réunit jamais la paire, même pour δ = H − 1, où m est à une unité de 0.

*Modèle réel et grille.* Les théorèmes 7 bis, 8 et 9 portent sur des positions réelles : « discontinu » y a
son sens exact (δ → 0). Sur la grille entière finie, toute fonction est lipschitzienne ; les certificats ci-dessus
mesurent donc une **amplification** : un déplacement d'une unité produit un saut d'environ H/2. Réciproquement,
la continuité du théorème 7 bis se lit sur la grille comme une variation bornée par le module de continuité, sans
croissance avec l'échelle (quasi-égalité K3 : 0,53 à S = 1 024 comme à 2 048 ; fantôme : variation normalisée
divisée par deux quand N double).

**Réponse à la question 1.** Dans la classe « poids fonction du niveau du témoin, de sa mort et du type
d'incidence », avec un dénominateur fixe et des atomes activés à leur niveau, **aucune fonction de poids n'est à
la fois précoce et continue** : les deux paires et les cinq sites exigent une réunion précoce quand l'avance est
nette, la quasi-égalité A5/A6 et F2 exigent la continuité, et le théorème 8 interdit les deux ensemble sur la
seule famille F2. Les poids uniformes, 1/β et les faces échouent déjà sur des fixtures connues. La durée de
branche par marches les passe toutes, mais échoue sur la nouvelle fixture {0, 937 / 938, 2 000} (saut de 531,5).
Chaque poids a son propre point de saut dans la famille F2, que la dichotomie entière de `f2_family.py` localise :
avec la famille entière comme porte, l'ensemble des poids qui passent tout est vide.

**Théorème 9 (compromis stabilité / rappel de frontière).** Soit une règle R qui associe à toute configuration
étiquetée une ultramétrique ρ_X en rayon sur les étiquettes, et qui vérifie :
(F) *fidélité* : à tout rayon r, chaque bloc est contenu dans l'amas discret X ∩ δ_r(C) d'une seule composante ;
(E) *équivariance* : ρ ne dépend ni du repère, ni des noms d'étiquettes ;
(L) *stabilité* : si |x_i − y_i| ≤ ε pour tout i, alors |ρ_X(i, j) − ρ_Y(i, j)| ≤ Lε.
Alors, sur X_δ (K = 2), `ρ(0, m) ≥ H − Lδ`. En particulier, 0 et m ne sont réunis au rayon de première
couverture (H − δ)/2 que si δ ≥ H/(2L − 1).

*Preuve.* Pour r < H, les composantes de L_2(r) sont la région gauche (autour de [m − r, r] sur l'axe) et la
région droite (autour de [2H − r, m + r]), disjointes car r < 2H − r. La gauche est à distance 2H − r > r du
site 2He : aucune composante ne couvre à la fois 0 et 2He, donc ρ(0, 2He) ≥ H par (F). À δ = 0, la réflexion
échange 0 et 2He en fixant m ; par (E), ρ(0, m) = ρ(2He, m). L'inégalité ultramétrique donne
H ≤ ρ(0, 2He) ≤ max(ρ(0, m), ρ(m, 2He)) = ρ(0, m). Déplacer m de δ change ρ(0, m) d'au plus Lδ par (L). ∎

Portée. La paire {0, m} est un amas discret d'une seule composante pour tout rayon de [(H − δ)/2, H). Le
théorème dit que toute règle stable la retarde, d'autant plus que L est petit. Core vérifie (F), (E) et
L = 2 (preuve de l'audit continu, § 2.2) ; sur X_δ il réunit à H − δ, jamais à la première couverture. Cover,
A5, les bandes et les majorités par marches ne sont lipschitziennes pour aucun L (théorème 8). La règle
progressive réunit à H pour δ ≤ δ\*(z), puis descend continûment :

| z | δ\*(z)/H (racine de 2^z[(1−u)^(−z) − (1+u)^(−z)] = 1) | pente locale maximale mesurée sur F2 |
| ---: | ---: | ---: |
| 1 | 0,2361 = √5 − 2 | 2,27 |
| 2 | 0,0620 | 4,09 |
| 3 | 0,0208 | 8,02 |
| 4 | 0,00781 | 15,97 |

La zone de report vaut environ H/(z 2^(z+1)) et la pente environ 2^z : **z est le cadran du compromis**. Nous
n'avons pas trouvé cet énoncé tel quel dans les références lues (§ 12). Il est dans l'esprit de la
proposition 44 de Rolle et Scoccola (discontinuité de la liaison simple robuste à κ ≥ 2 fixé), mais il porte sur
la projection des points, pas sur l'arbre.

**Proposition 10 (les masses du § 9.1 comme unité de hiérarchie).** Pour les poids de la thèse,
S_τ = Σ_{σ ⊃ τ} ψ(ρ(σ)) sur les cofaces et w_xτ = S_τ/T_x, avec ψ = r^(−2) :
(a) *biais de degré* : sur les paires {0, 1, L, L+1}, K2, la face locale {0, 1} n'a que des cofaces de rayon
≈ L/2, comme les faces-ponts. En convention Gabriel, le point 1 porte 1/3 de sa masse sur la face {0, 1} et
2/3 sur le pont ; en convention « toutes les faces », environ 1/3 sur chacune de ses trois faces. La majorité ne
forme jamais les paires avant la fusion : échec sur les huit fixtures de paires, dans les deux conventions ;
(b) *perte des entrées internes* : en convention Gabriel, le point x des fixtures K3 et K5 n'appartient à aucune
face (son seul témoin fort a une population K + 1) ; T_x = 0, il ne reçoit aucune masse et n'est jamais attaché ni
étiqueté ;
(c) *activation par marches* : le théorème 8 s'applique.

*Preuve.* (a) et (b) sont des calculs exacts de la porte (§ 6). (c) Les faces entrent à leur naissance. ∎

Obtention sans énumération : les faces de Gabriel sont les boules du catalogue de population exactement K, et
leurs cofaces celles de population K + 1 (vérifié par le commit v9 `268ad5a80`, sous position générale) ; la
convention « toutes les faces » demande en revanche les C(n, K) parties et reste interdite en production. Le
problème de (a) et (b) n'est donc pas le coût : c'est l'unité.

La thèse n'utilise ces masses que pour condenser et voter à une sélection fixée (proposition 7), pas pour
construire une hiérarchie : ce résultat ne contredit pas le manuscrit, il en borne l'usage.

**Les deux ingrédients sont nécessaires (ablation exacte).** Unité de branche seule, ou activation progressive
seule, ne suffisent pas :

| Variante (z = 2) | Contact : variation de ρ(C, A), M = 10 → 2 048 | Fantôme K5 : variation normalisée, N = 64 → 256 | Seuil {0, 937 / 938, 2 000} |
| --- | --- | --- | --- |
| unités de branche, marches | 0 | 0,0073 → 0,0018 | saut 531,5 |
| témoins du catalogue non dédupliqués, durée progressive | 5,8 → 1 133 (∝ M) | 134,6, constante | 2,2 |
| unités de branche, progressive | 0 | 0,0109 → 0,0027 | 4,0 (pente continue) |

Les témoins non dédupliqués comptent plusieurs fois une même couverture et changent avec les certificats : le
contact (15 → 6 incidences) déplace la date d'une quantité proportionnelle à M. Les marches sautent au seuil.

## 6. Porte exacte : toutes les fixtures connues

`fixtures.py` construit Γ_K exhaustif (référence figée `hgp10_ref.py`) sur 34 nuages, exécute quatorze règles et
vérifie quatorze constats attendus. Normal et `python3 -O` donnent la même sémantique ; 4 411 transitions de
coupes sont contrôlées laminaires. Avec `--native`, les nuages sont aussi exportés par `export_frontier` et
comparés (§ 1).

| Fixture | Exigence | Règle progressive z = 2, 3, 4 | Échecs reproduits |
| --- | --- | --- | --- |
| F1 {0, 2, 4} | laminarité, report du point contesté (équivariance) | reporté à la fusion | cover attache par identifiant |
| F2 999/1001 | continuité en rayon (borne core 2ε = 4) | ρ(0, m) = 1 000 dans les deux | cover, A4, A5 : 499,5 contre 1 000 |
| Deux paires, lignes et tétraèdres, L = 4, 8, 32, 100 | paires formées avant la première fusion | 8/8 | A3 uniforme, faces § 9.1 |
| Cinq sites | paire {0, 1} formée avant sa fusion à β = 5/4, propriétaire dans sa lignée | x0 attaché à β = 10/9 (z = 2), rayon 0,800 (z = 3), 0,684 (z = 4) | A3, A4, faces ; progressive z = 1 |
| Contact coquille/intérieur, M = 10, 100, 1 024, 2 048 | variation bornée de ρ(C, A) | 0 (z = 2) ; 0,56 (z = 3) ; 0,58 (z = 4) | A4 : 2 461,3 à M = 2 048 ; A3 : 95 925 |
| Entrées internes K3 (β = 25), K5 (β = 105 625) | x attaché à la branche interne | attaché, rayon 6,30 et 409,5 (z = 3) | faces Gabriel : jamais attaché |
| Fantôme K5, N = 64, 128, 256 | variation normalisée → 0 | 0,0097 → 0,0049 → 0,0024 (z = 3) | aucune règle ne diverge |
| Quasi-égalité K3 (A5/A6), S = 1 024, 2 048 | variation bornée sous déplacement unité | 0,53 | A5 : 1 068 puis 2 135 |

Remarques. (0) Fenêtre en z, contrôlée aussi à z = 5, 6, 7, 8 : F2 999/1001 reste inchangé jusqu'à z = 6, puis
change de 46,5 (z = 7) et 110 (z = 8) ; quasi-égalité K3 (0,53), contact (≤ 0,582) et cinq sites restent bons.
(1) Le seuil z\* ≈ 1,845 des cinq sites est la racine de
`φ(1/4) − φ(5/4) = 3 φ(1/2) − 2 φ(2/3)` : la branche {0, 1} doit accumuler la moitié de la persistance totale de x0
avant sa fusion. (2) La majorité de branche par marches passe aussi ces fixtures : aucune ne se trouve au seuil
où elle saute ; le théorème 8 fournit la fixture manquante (§ 7). (3) Le contact ne change rien aux unités de
branche : la quantité qui changeait (15 → 6 incidences) est un certificat, pas une couverture.

## 7. Famille F2 : sauts certifiés et continuité

`f2_family.py` balaie δ ∈ [0, H) sur la grille g = 128 000 (H = 64 000), puis localise chaque saut par dichotomie
entière jusqu'à deux nuages distants d'une unité de grille. Les constats du § 5 en sortent :

| Règle | Égalité δ = 0 | Réunion précoce possible | Saut certifié (δ, rayon avant → après) |
| --- | ---: | --- | --- |
| cover A1 | 32 000 (départage) | oui | entre δ = −1 et 0 : 64 000 → 32 000 |
| A5 (bande η = 0), A4 (1/β) | 64 000 | oui | δ = 0 → 1 : 64 000 → 31 999,5 |
| bande η = 1/8 | 64 000 | oui | δ = 3 764 → 3 765 : 64 000 → 30 117,5 |
| branche, marches, z = 2 | 64 000 | oui | δ = 3 969 → 3 970 : 64 000 → 30 015 |
| A3 (uniforme) | 64 000 | **non** (même δ = 63 999) | aucun |
| core | 64 000 | oui (à H − δ) | aucun, pente 1 |
| branche, progressive, z = 1..4 | 64 000 | oui | aucun ; pentes 2,27 / 4,09 / 8,02 / 15,97 |

Le saut de la majorité par marches se produit exactement au seuil δ\*(2) où la règle progressive commence à
descendre continûment : les deux règles ont la même condition de majorité, seule la date diffère. Nouvelle
fixture minimale à graver : **{0, 938, 2 000} contre {0, 937, 2 000}** à l'échelle de F2 (δ = 62 et 63).

## 8. La sélection sans hiérarchie de points

**Proposition 11 (condensation fractionnaire).** Masse d'un nœud : m_v(β) = Σ_x A_x(v, β)/W_x (progressive) ou
Σ_x S_x(v, β)/W_x (marches). Elle croît pendant la vie de v. À sa mort, les deux coïncident :
m_v(d_v⁻) = Σ_x (base_x(v) + w_x(v))/W_x, car toutes les unités du sous-arbre sont acquises. La condensation
N-aire (enfants gros si m ≥ mcs, mcs réel), puis l'EOM avec `S(C) = ∫ m_C dλ` sur la vie condensée de C
(égalité → parent, racine exclue), sont bien définies et rendent une antichaîne de nœuds de FULL.

*Preuve.* Pour des masses unités entières entrant à des dates fixes, ∫ m_C dλ = Σ_p (λ_max,C(p) − λ_min,C(p)),
la stabilité σ(C) de Campello et al. telle que l'écrivent McInnes et Healy. La définition est linéaire en la
masse : elle s'étend telle quelle aux masses fractionnaires. La condensation ne lit que les masses de fin de vie
des enfants, identiques dans les deux activations. L'EOM ascendante choisit une antichaîne par construction. ∎

**Proposition 12 (vote à l'antichaîne).** Pour l'antichaîne choisie, de sommets top(c), posons
`V_x(c) = Σ_{u ∈ V_x, u ⪯ top(c)} w_x(u)/W_x`. Les sous-arbres sont disjoints, donc Σ_c V_x(c) ≤ 1. L'étiquette
est l'argmax unique s'il est positif ; sinon x est bruit (abstention aux égalités, sans départage par
identifiant). Le résultat est une partition : c'est la proposition 7 du manuscrit, avec des unités de branche au
lieu des faces.

**Proposition 13 (cohérence hiérarchie / vote).** Si le propriétaire majoritaire progressif de x vérifie
o(x) ⪯ top(c) pour un cluster choisi c, alors V_x(c) > 1/2 et le vote donne c.

*Preuve.* top(c) est o(x) ou l'un de ses ancêtres, donc d_top(c) ≥ d_o(x) > t(x). V_x(c) majore la masse accumulée
dans le sous-arbre de top(c) à tout β < d_top(c). Juste après t(x), cette masse dépasse strictement W_x/2 : o(x)
couvre x, sa masse croît strictement (théorème 7, étape 1). ∎

Contrôle : zéro violation sur 194 cas de fixtures (mcs ∈ {1, 2, 3}, z ∈ {2, 4}, deux activations) et sur les
scènes dev (§ 9).

**Proposition 14 (hiérarchie tirée en arrière).** Étant donnés l'antichaîne et le vote, attacher chaque point
étiqueté c au début de la vie condensée de c, ou à sa première couverture par la chaîne de c si elle est
postérieure, puis suivre les ancêtres, donne une hiérarchie laminaire et fidèle. Entre l'entrée du dernier point
étiqueté et la première mort d'un cluster choisi, ses blocs sont exactement les classes du vote. Elle ne voit
rien sous la sélection et hérite des discontinuités de l'EOM (égalités S(C) = Σ R) et du vote. Elle est
canonique **relativement à la sélection**, pas davantage. (La chaîne de c couvre x avant d_c : V_x(c) > 0 donne un
nœud couvrant sous top(c), et le lemme 1 fait couvrir top(c).)

*Preuve.* Dispositif d'ancrage (théorème 5, (iv)) ; fidélité : la chaîne condensée de c couvre x à sa date
d'attache par construction, puis lemme 1. ∎

**Fixtures S1 et S2 (non-équivalence).** Deux amas symétriques et des points partagés sur le plan médian, K2,
z = 2 (`selection_fixtures.py`) :

| Fixture | Condensation fractionnaire + vote | Projection progressive, puis condensation dure |
| --- | --- | --- |
| S1 : tétraèdres, D = 3, deux points partagés, mcs = 4 | 0 cluster : chaque point réserve sa queue future | 2 clusters |
| S2 : triangles, D = 5, quatre points partagés, mcs = 7/2 | 2 clusters, masse exacte 9 176 959 019/2 421 129 760 ≈ 3,790 chacun à la fusion | 0 cluster : les points à égalité exacte sont reportés au LCA ou à la racine |

**Réponse à la question 2.** Une hiérarchie de points n'est pas nécessaire pour sélectionner. L'arbre FULL,
les masses fractionnaires et un vote final suffisent ; tous les amas discrets restent comptés jusqu'à la
décision. Projeter d'abord n'est pas neutre : S1 et S2 montrent des effets dans les deux sens. Un seuil mcs
n'a pas le même sens en masse fractionnaire (une masse est réservée au futur) et en points. La hiérarchie de
points reste utile quand le produit doit publier un dendrogramme sur les points ; la règle progressive est alors
cohérente avec la sélection (proposition 13).

## 9. Scènes dev

Quatre scènes dev de la campagne frontière (`bhc_n600_*` : deux amas de 224 cœurs et 60 halos, un couloir de
12 points, 20 points de fond ; graines dev figées par son manifeste), K2 et K5, exports natifs. Représentants :
plus petit D_K par classe ; r_sp = première fusion parasite de leurs lignées core. Jitter apparié delta4 des
fondations (ε = √48 ≈ 6,93, 2ε ≈ 13,86), panel fixe de 2 000 paires tiré sur la base. mcs = 24 = √n. Ce ne sont
ni des scores préenregistrés ni une campagne : quatre nuages et une graine chacun.

**K = 2** : rappel du halo avant la fusion parasite (classes 0 / 1), part reportée, jitter delta4 (q99 et max de |Δρ| en rayon ; part au-delà de 2ε).

| Règle | base | couloir_dense | halo_rare | loin | jitter δ4 : q99 / max / >2ε (moyenne des scènes) |
| --- | --- | --- | --- | --- | --- |
| core | 0.05 / 0.12 ; 0.193 | 0.07 / 0.05 ; 0.333 | 0.25 / 0.20 ; 0.088 | 0.80 / 0.87 ; 0.062 | 7.67 / 10.4 / 0.000 |
| cover | 0.18 / 0.27 ; 0.075 | 0.12 / 0.10 ; 0.132 | 0.50 / 0.30 ; 0.058 | 1.00 / 1.00 ; 0.015 | 9.15 / 45.2 / 0.005 |
| bande 1/8 | 0.17 / 0.23 ; 0.095 | 0.10 / 0.08 ; 0.150 | 0.50 / 0.30 ; 0.062 | 0.98 / 1.00 ; 0.018 | 14.32 / 55.0 / 0.008 |
| marches z2 | 0.10 / 0.17 ; 0.133 | 0.08 / 0.05 ; 0.173 | 0.40 / 0.20 ; 0.070 | 0.93 / 0.98 ; 0.030 | 6.03 / 31.1 / 0.001 |
| progr. z2 | 0.10 / 0.17 ; 0.153 | 0.08 / 0.05 ; 0.203 | 0.40 / 0.20 ; 0.073 | 0.93 / 0.98 ; 0.035 | 5.24 / 16.1 / 0.000 |
| progr. z3 | 0.15 / 0.20 ; 0.142 | 0.10 / 0.07 ; 0.182 | 0.50 / 0.25 ; 0.068 | 0.97 / 0.98 ; 0.025 | 5.16 / 18.1 / 0.000 |
| progr. z4 | 0.17 / 0.20 ; 0.128 | 0.10 / 0.10 ; 0.172 | 0.50 / 0.25 ; 0.067 | 0.98 / 1.00 ; 0.020 | 4.82 / 17.4 / 0.000 |

Sélection (ARI exact contre les labels, bruit compris ; clusters choisis) :

| Méthode | base | couloir_dense | halo_rare | loin |
| --- | --- | --- | --- | --- |
| fractional_step_z2 | 0.669 (2) | 0.592 (2) | 0.879 (2) | 0.946 (2) |
| fractional_step_z3 | 0.669 (2) | 0.592 (2) | 0.879 (2) | 0.946 (2) |
| fractional_gradual_z3 | 0.669 (2) | 0.592 (2) | 0.879 (2) | 0.946 (2) |
| hard_core_eom_z3 | 0.378 (3) | 0.267 (3) | 0.850 (2) | 0.893 (2) |
| hard_cover_A1_eom_z3 | 0.666 (2) | 0.125 (9) | 0.879 (2) | 0.946 (2) |
| hard_band_eta1/8_eom_z3 | 0.191 (5) | 0.120 (9) | 0.879 (2) | 0.942 (2) |
| hard_branch_gradual_z3_eom_z3 | 0.648 (2) | 0.619 (3) | 0.876 (2) | 0.939 (2) |
| hard_core_eom_z2 | 0.592 (2) | 0.396 (2) | 0.850 (2) | 0.893 (2) |
| hard_cover_A1_eom_z2 | 0.666 (2) | 0.635 (3) | 0.879 (2) | 0.946 (2) |
| hard_branch_gradual_z3_eom_z2 | 0.648 (2) | 0.619 (3) | 0.876 (2) | 0.939 (2) |

Violations de cohérence (proposition 13) : 0 sur 16 sélections fractionnaires.
Tailles : base : 3597 nœuds, 4046 incidences, 313425 unités, r_sp = 115.8, 90 s; couloir_dense : 3499 nœuds, 3951 incidences, 371412 unités, r_sp = 83.2, 94 s; halo_rare : 3421 nœuds, 3876 incidences, 247614 unités, r_sp = 122.5, 88 s; loin : 3516 nœuds, 3976 incidences, 243110 unités, r_sp = 231.9, 61 s

**K = 5** : rappel du halo avant la fusion parasite (classes 0 / 1), part reportée, jitter delta4 (q99 et max de |Δρ| en rayon ; part au-delà de 2ε).

| Règle | base | couloir_dense | halo_rare | loin | jitter δ4 : q99 / max / >2ε (moyenne des scènes) |
| --- | --- | --- | --- | --- | --- |
| core | 0.32 / 0.32 ; 0.170 | 0.03 / 0.07 ; 0.247 | 0.55 / 0.25 ; 0.073 | 0.97 / 0.97 ; 0.040 | 7.33 / 9.9 / 0.000 |
| cover | 1.00 / 1.00 ; 0.032 | 0.32 / 0.32 ; 0.150 | 1.00 / 1.00 ; 0.032 | 1.00 / 1.00 ; 0.015 | 4.83 / 43.9 / 0.002 |
| bande 1/8 | 0.87 / 0.98 ; 0.050 | 0.27 / 0.23 ; 0.182 | 1.00 / 0.95 ; 0.037 | 1.00 / 1.00 ; 0.027 | 9.91 / 36.3 / 0.006 |
| marches z2 | 0.70 / 0.78 ; 0.088 | 0.18 / 0.13 ; 0.202 | 0.70 / 0.80 ; 0.053 | 1.00 / 1.00 ; 0.030 | 7.70 / 13.3 / 0.000 |
| progr. z2 | 0.70 / 0.78 ; 0.088 | 0.18 / 0.13 ; 0.202 | 0.70 / 0.80 ; 0.053 | 1.00 / 1.00 ; 0.030 | 6.67 / 10.0 / 0.000 |
| progr. z3 | 0.80 / 0.93 ; 0.060 | 0.22 / 0.15 ; 0.195 | 0.75 / 0.85 ; 0.048 | 1.00 / 1.00 ; 0.023 | 5.53 / 10.1 / 0.000 |
| progr. z4 | 0.88 / 0.98 ; 0.047 | 0.25 / 0.20 ; 0.183 | 0.90 / 0.90 ; 0.040 | 1.00 / 1.00 ; 0.022 | 4.93 / 9.6 / 0.000 |

Sélection (ARI exact contre les labels, bruit compris ; clusters choisis) :

| Méthode | base | couloir_dense | halo_rare | loin |
| --- | --- | --- | --- | --- |
| fractional_step_z2 | 0.959 (2) | 0.659 (2) | 0.955 (2) | 0.930 (2) |
| fractional_step_z3 | 0.959 (2) | 0.659 (2) | 0.955 (2) | 0.930 (2) |
| fractional_gradual_z3 | 0.959 (2) | 0.659 (2) | 0.955 (2) | 0.930 (2) |
| hard_core_eom_z3 | 0.698 (2) | 0.543 (2) | 0.880 (2) | 0.945 (2) |
| hard_cover_A1_eom_z3 | 0.959 (2) | 0.659 (2) | 0.955 (2) | 0.930 (2) |
| hard_band_eta1/8_eom_z3 | 0.578 (4) | 0.642 (2) | 0.959 (2) | 0.946 (2) |
| hard_branch_gradual_z3_eom_z3 | 0.906 (2) | 0.618 (2) | 0.935 (2) | 0.939 (2) |
| hard_core_eom_z2 | 0.698 (2) | 0.543 (2) | 0.880 (2) | 0.945 (2) |
| hard_cover_A1_eom_z2 | 0.959 (2) | 0.659 (2) | 0.955 (2) | 0.930 (2) |
| hard_branch_gradual_z3_eom_z2 | 0.906 (2) | 0.618 (2) | 0.935 (2) | 0.939 (2) |

Violations de cohérence (proposition 13) : 0 sur 16 sélections fractionnaires.
Tailles : base : 14632 nœuds, 43855 incidences, 1241094 unités, r_sp = 256.3, 358 s; couloir_dense : 13875 nœuds, 41686 incidences, 1412919 unités, r_sp = 151.7, 350 s; halo_rare : 14567 nœuds, 44066 incidences, 1196340 unités, r_sp = 258.8, 228 s; loin : 14511 nœuds, 43622 incidences, 1149927 unités, r_sp = 438.6, 163 s

Lecture, sans revendication statistique :

- **Rappel frontière avant fusion parasite** : cover > bande ≥ progressive z4 > z3 > z2 = marches > core, sur
  presque toutes les scènes. La règle progressive récupère l'essentiel de l'avantage de cover sur core (K5 :
  halo 0,88 / 0,98 à z = 4 contre 0,32 pour core et 1,00 pour cover sur `base`).
- **Stabilité sous jitter** : la règle progressive reste dans la borne 2ε de core pour 100 % des paires à K5
  (maximum 10,1 contre 9,9 pour core) ; à K2 quelques paires la dépassent (maximum 18,1). Cover et la bande
  atteignent 36 à 55 et dépassent 2ε sur 0,2 à 0,8 % des paires : c'est l'effet mesuré des théorèmes 8 et 9.
  Core ne dépasse jamais 2ε : contrôle vivant du théorème de l'audit continu.
- **Sélection** : la condensation fractionnaire et le vote donnent exactement le même résultat aux deux z et dans
  les deux activations. Elle n'est jamais la pire : ARI moyen 0,772 (K2) et 0,876 (K5), contre 0,654 et 0,876
  pour cover à z = 3 (effondrement à 0,125 et neuf amas sur `couloir_dense` K2), 0,771 et 0,850 pour la
  projection progressive, 0,683 et 0,767 pour core à z = 2. Elle ne bat pas toujours la meilleure projection
  (cover z = 2 : 0,635 contre 0,592 sur `couloir_dense` K2).
- **Cohérence** : zéro violation de la proposition 13 sur les 32 sélections fractionnaires.
- **Égalités douces** : 74 321 au total. Sur l'exécution instrumentée par site d'appel (scène `base`, K2,
  2 016 cas), toutes comparent des valeurs Decimal identiques : auto-comparaison du gagnant du vote (1 952) et
  masses entières de fin de vie égales à mcs (64). Les autres exécutions n'ont pas été instrumentées ; elles
  passent par les mêmes sites. Aucune quasi-égalité non certifiée n'a été observée.


## 10. Coût

Mesures sur la scène dev `bhc_n600_base` : à K2, 3 597 nœuds FULL, 4 046 incidences de témoins forts, mais
Σ_x |V_x| = 313 425 unités de branche (522 par point) ; à K5, 14 632 nœuds, 43 855 incidences et 1 241 094 unités
(2 068 par point). Parcourir les chaînes est donc proscrit à l'échelle.

La majorité n'en a pas besoin. Le **squelette** de T_x ne garde que les nœuds à zéro ou au moins deux enfants
couverts ; le long des chaînes, tout télescope (proposition 3). Il a au plus 2 E_x segments, E_x entrées, et
E_x ≤ D_x témoins de x. Algorithme par point :

1. trier les nœuds de témoins par ordre DFS et ajouter les LCA des voisins (arbre virtuel), O(D_x log D_x) ;
2. calculer base et W_x sur les segments, O(E_x) ;
3. tester le franchissement en forme close par segment ; retrouver le nœud vivant par sauts d'ancêtres,
   O(log profondeur).

Total O(D log D + n log H), D le volume d'incidences fortes (borne de l'auditeur relative au catalogue). Pour la
condensation, les masses de nœuds s'obtiennent par mises à jour de chemins (tableaux de différences et une passe
ascendante), O(n + H + D). Le prototype Python de ce dossier parcourt encore les chaînes pour les masses de nœuds :
aucun coût natif, 8k/16k/32k ou G4 n'est mesuré ici.

## 11. Plusieurs ordres

**Proposition 15 (tranche monotone).** Soit une tranche t ↦ (r(t), K(t)) avec r croissante et K décroissante. Les
ensembles L_K(t)(r(t)) sont emboîtés et la couverture est monotone le long des ancêtres :
dist(x, C′) ≤ dist(x, C) ≤ r ≤ r′ si C ⊆ C′. Les définitions et les théorèmes 5, 7, les propositions 3, 6 et
11 à 14 se transportent mot pour mot à l'arbre de la tranche. *Preuve :* aucune de leurs preuves n'utilise K
autrement que par l'emboîtement et la monotonie de la couverture. ∎

L'union libre des ordres n'est pas un arbre (F3) : une hiérarchie « de toute la tour » est une tranche, choisie
avant les masses. Rolle et Scoccola donnent la stabilité des tranches linéaires de pente négative. Une famille de
hiérarchies, une par ordre K, reste publiable ; chacune suit ce mémo.

## 12. Littérature (énoncés vérifiés dans le texte ou la notice)

- A. Rolle, L. Scoccola, *Stable and Consistent Density-Based Clustering via Multiparameter Persistence*, JMLR
  25(258), 2024 (arXiv:2005.09048v4, lu). Proposition 44 : la liaison simple robuste RSL_{κ,α}, κ ≥ 2, est
  discontinue pour les distances de Gromov–Hausdorff–Prokhorov et d'entrelacement de correspondances.
  Résultat B : pour une droite de pente σ < 0, d_CI(λ-link(M), λ-link(N)) ≤ max(2|σ|, 1) d_GHP(M, N).
  Résultat C : λ-link est CI-consistante, donc Hartigan-consistante, pour toute densité continue à support compact.
- K. Chaudhuri, S. Dasgupta, *Rates of convergence for the cluster tree*, NIPS 23, 2010 ; K. Chaudhuri,
  S. Dasgupta, S. Kpotufe, U. von Luxburg, *Consistent procedures for cluster tree estimation and pruning*, IEEE
  TIT 60(12):7900–7912, 2014 (lu). Définition 2.3 : consistance de Hartigan ; algorithme 1 : liaison simple
  robuste.
- J. Eldridge, M. Belkin, Y. Wang, *Beyond Hartigan Consistency: Merge Distortion Metric for Hierarchical
  Clustering*, COLT 2015 (lu). Définition 6 : hauteur de fusion ; définition 11 : distance de distorsion de
  fusion ; théorème 5 : stabilité L∞ de l'arbre vrai. Nos hauteurs u(x, y) sont ces hauteurs de fusion.
- G. Carlsson, F. Mémoli, *Characterization, Stability and Convergence of Hierarchical Clustering Methods*, JMLR
  11:1425–1470, 2010 (notice) : existence et unicité (la liaison simple) sous leurs axiomes, là où Kleinberg
  obtient une impossibilité ; dendrogrammes vus comme ultramétriques, stabilité en distance de Gromov–Hausdorff.
- J. Kleinberg, *An Impossibility Theorem for Clustering*, NIPS 15, 2002 (notice) : aucune fonction de partition
  ne vérifie à la fois invariance d'échelle, richesse et cohérence. Notre théorème 9 est un compromis quantitatif
  d'une autre nature : fidélité aux recouvrements, équivariance et constante de Lipschitz.
- J. Culbertson, D. Guralnik, P. Stiller, *Functorial hierarchical clustering with overlaps*, Discrete Applied
  Mathematics, 2018 (arXiv:1609.02513, notice) : cadre fonctoriel pour des recouvrements hiérarchiques ; les amas
  discrets de la définition 8 en sont un exemple, et la projection vers des partitions est notre objet.
- R. Campello, D. Moulavi, J. Sander, *Density-Based Clustering Based on Hierarchical Density Estimates*, PAKDD
  2013, pp. 160–172 (notice) ; L. McInnes, J. Healy, *Accelerated Hierarchical Density Clustering*,
  arXiv:1705.07321 (lu) : arbre condensé, σ(C) = Σ (λ_max − λ_min), sélection EOM.
- F. Chazal, L. Guibas, S. Oudot, P. Skraba, *Persistence-Based Clustering in Riemannian Manifolds*, J. ACM
  60(6), 2013 (notice) : ToMATo, fusion guidée par la persistance des modes.
- D. Cohen-Steiner, H. Edelsbrunner, J. Harer, Y. Mileyko, *Lipschitz functions have L_p-stable persistence*,
  FoCM 10(2):127–139, 2010 (notice) : la stabilité de persistances totales exige des hypothèses de croissance.
  Notre masse W_x est une persistance totale en échelle φ ; sa continuité (théorème 7 bis) vient de la finitude
  de l'univers des K-parties, pas d'une borne uniforme : une constante de Lipschitz reste ouverte (C2).
- H. Blumberg, M. Lesnick, stabilité de la multicouverture en distance de Prohorov, FoCM 24(2), 2024 (lu par
  l'auditeur de la littérature v10) : argument pour garder la tour, pas certification d'une tête.

## 13. Recommandation

1. **Unité de masse** : la persistance couverte d'une branche, en λ = r^(−z) avec le z de la tête. Calcul par
   témoins forts et arbre virtuel, sans énumération de K-parties, indépendant du catalogue et de Kmax.
2. **Sélection** : condensation et EOM sur FULL avec ces masses fractionnaires, puis vote à l'antichaîne. Pas de
   hiérarchie de points dans la boucle. Publier la marge du vote et les abstentions.
3. **Hiérarchie de points**, si le produit en publie une : majorité **progressive** à θ = 1/2, z dans la fenêtre
   [2, 6] (z = 3 par défaut, comme la densité K-NN en 3D). Publier la réserve et les dates. Garder core (stable,
   L = 2, lent) et cover (précoce, discontinu) comme contrôles.
4. **Ne pas porter** : les majorités par marches (théorème 8), les faces du § 9.1 comme unité de hiérarchie
   (proposition 10), un argmax recalculé par coupe (F5), une union libre des ordres (F3).
5. **Graver** les fixtures nouvelles : {0, 938, 2 000} / {0, 937, 2 000} (saut par marches), S1, S2, et la
   racine z\* des cinq sites.
6. **Mesurer ensuite**, sur scènes dev plus grandes puis 8k/16k/32k : rappel avant fusion parasite, masse
   reportée, jitter apparié, ARI de la sélection fractionnaire contre la projection dure, coût natif du squelette.

## 14. Questions ouvertes

- **C1 (continuité globale)** : résolue par le théorème 7 bis.
- **C2 (constante uniforme).** Existe-t-il L(z) tel que la règle soit L(z)-lipschitzienne en rayon ? Sur F2, la
  pente locale vaut environ 2^z ; aucune borne générale n'est établie.
- **Règle optimale.** Sur F2, la borne du théorème 9 est atteinte par la fonction δ ↦ max((H − δ)/2, H − Lδ),
  compatible avec (F), (E) et la pente L. En faire une règle définie sur tout nuage demande une marge canonique
  entre branches ; la majorité en fournit une en unités de masse, sans optimalité prouvée.
- **Choix de z et de mcs.** Statistiques, à préenregistrer ; un seuil mcs ne se transpose pas entre masses
  fractionnaires et points (S1, S2).
- **K = 1 et multiplicités.** Hors domaine ici (φ(0) = +∞ ; tour pondérée refusée).

## 15. Entrées proposées pour le registre des preuves

Le registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` est dans le worktree partagé, en lecture seule pour ce
travail. Entrées proposées au développeur, avec leurs fixtures permanentes :

| Énoncé | Statut proposé | Preuve ou fixture |
| --- | --- | --- |
| les unités de branche télescopent et ne dépendent ni du catalogue ni de Kmax | `proved_here` | propositions 3, 4 ; 34 nuages Γ/natif ; KM |
| la majorité progressive est exclusive, laminaire et fidèle à la définition 8 | `proved_here` | théorème 5, proposition 6 |
| la majorité progressive est continue à combinatoire fixée | `proved_here` | théorème 7 |
| la majorité progressive est continue en la configuration (changements de combinatoire compris) | `proved_here` | théorème 7 bis ; perturbations exactes, jitter dev |
| la majorité progressive est lipschitzienne avec une constante uniforme L(z) | `proof_obligation` (C2) | pente ≈ 2^z sur F2 |
| une majorité à masse fixe activée par marches, pour un choix de poids (niveau, mort, type), passe toutes les fixtures | `false_in_general` | théorème 8 ; {0, 937 / 938, 2 000} |
| une règle fidèle, équivariante et lipschitzienne peut réunir un point à sa première couverture | `false_in_general` | théorème 9 : ρ ≥ H − Lδ sur F2 |
| les masses de faces du § 9.1 donnent une hiérarchie à rappel frontière précoce | `false_in_general` | proposition 10 ; paires ; K3/K5 Gabriel |
| sélectionner sur la hiérarchie projetée équivaut à sélectionner sur les masses fractionnaires | `false_in_general` | S1, S2 |
| le vote à l'antichaîne est cohérent avec la majorité progressive | `proved_here` | proposition 13 ; 194 cas, zéro violation |

## 16. Reproduction

Depuis ce dossier, bibliothèque standard seulement (Python 3.12), codes de sortie exacts : 0 si tous les constats
attendus sont reproduits, 1 sinon (constat inattendu ou incohérence levée par `require`, jamais par `assert`).

```bash
python3 -B fixtures.py --out out/fixtures_gamma.json            # 34 nuages, 14 règles, 14 constats, ~12 s
python3 -B -O fixtures.py --out out/fixtures_gamma_O.json       # même sémantique
python3 -B fixtures.py --out out/fixtures_native_crosscheck.json --native /workspaces/E-HGP/build/v10-frontiere/frontier-build/export_frontier
python3 -B f2_family.py --out out/f2_family.json                # sauts certifiés et pentes, ~25 s
python3 -B -O f2_family.py --out out/f2_family_O.json
python3 -B selection_fixtures.py --out out/selection_fixtures.json   # S1, S2, cohérence, Kmax
python3 -B -O selection_fixtures.py --out out/selection_fixtures_O.json
nice -n 5 python3 -B dev_scenes.py --out out/dev_scenes_bhc600.json  # scènes dev, ~35 min sous charge
python3 -B dev_tables.py out/dev_scenes_bhc600.json
python3 -B hashes.py
```

Entrées lues sans modification : la référence figée `hgp10_ref.py` et `frontier_core.py` (copiés dans
`source_snapshot/`, empreintes ci-dessous), l'exporteur `export_frontier` des fondations frontière, les scènes et
perturbations dev de `/workspaces/E-HGP/build/v10-frontiere/work/bench/frontier/` (le script refuse toute scène
ou perturbation dont le manifeste n'est pas `dev`). Exports natifs dans `/tmp/mhgp10-verrou-points/`.

Empreintes (sorties déterministes : deux exécutions successives donnent les mêmes octets ; normal et `−O`
diffèrent seulement par le champ `python_optimize`) :

| Fichier | SHA-256 |
| --- | --- |
| `fixtures.py` | `8512f566aefd64d640867e4dae91d63f302f2b0e464038bfe649d014c469c32c` |
| `f2_family.py` | `a14ebe5c298c4f745ba0d1574a3308ec192640704eeb867f4475eb20dbc0b7b3` |
| `selection_fixtures.py` | `4c01c9cb7d9943eeb0fed616c5bf791f8e4d7f210d1c880ef1ee4b07305e4efe` |
| `dev_scenes.py` | `626b0dc9ca27c9aae4a5a184188e012d394ff05d52b4480f3729cebee0fad03c` |
| `dev_tables.py` | `9b9e19549e4612a2eeb378b85b19658f1364c0a6ede9d4c20d3c00492421d3fd` |
| `hashes.py` | `49b39ffb5f9090a8872fedf1542b6a194dadc8b9e9bd7905ec49162613a6af07` |
| `lib/fullk.py` | `a09a6c85687971f80e0df030effaf4cc112b9641176badf8460296898625a44e` |
| `lib/scale.py` | `7445aec285ce649b32e6b31c62dca96448f4f5b19b0b305b455d545462b2f294` |
| `lib/participation.py` | `47acfd0c482455f9b14fbfaa9e90a9f5ff02627dd15453c3dc99a8651b696549` |
| `lib/rules.py` | `dd4cfc22b1b574cf04a35a44d75025cfed0e38aa14f3acdb24d1add2eb4b3a04` |
| `lib/selection.py` | `509acaa1170c1bcf3a3cfb97eb6220db3a908b0b7669c75efad78183e84bc80e` |
| `source_snapshot/hgp10_ref.py` | `2cb84ad549b1f8982e71f7757794d97b5eadab323fd22d17461da18b76914104` |
| `source_snapshot/frontier_core.py` | `86ba984ff7986bdd44a5d53e9b2c7d3f2c0eb847764938cd28259d18439850b7` |
| `out/fixtures_gamma.json` | `1f43da1afde600225d1863936e365a677ab93eb57b2778a7ab4bdbf1a560cba1` |
| `out/fixtures_gamma_O.json` | `9ed20bb03d04910109372462b12997e53d6bff71d7c81d080ff71a345da1db93` |
| `out/fixtures_native_crosscheck.json` | `9f626abd7c3e53c79e281f2ee49869563392bfcac2d8f01900d61c158e0cd314` |
| `out/f2_family.json` | `9e680db6d0c60b29227d637c38fcdf0bc21954f9d5a9cc0f6ae05cf0ea0c14c3` |
| `out/f2_family_O.json` | `df8bac3bc6495c779c7c7e5d120a8c8cf055ba3dc049917a5f47910e4a2ed52b` |
| `out/selection_fixtures.json` | `27ea4457a844fabd496f16fb127563a27719fc51de05284492df082bdb9ab234` |
| `out/selection_fixtures_O.json` | `ec00f45e68d6120e3331b5916f288ddcca7ebc284962e75c17c53d446ef87ca3` |
| `out/dev_scenes_bhc600.json` | `d1ba8c224dd721f83890dad994b8158f9a3b831300627c89cc5e5058f1bee17a` |
| `/workspaces/E-HGP/build/v10-frontiere/frontier-build/export_frontier` | `b2dc98243c4230761ff6384660978f5972e21d9db62f30621eb8de2be78d6616` |

Arbres de fusion : 4 411 transitions laminaires vérifiées par la porte, 318 contrôles Γ/natif, 194 contrôles de
cohérence, 32 sélections dev sans violation. Aucun moteur modifié, aucun fichier du worktree partagé écrit, GCP
non utilisé.
