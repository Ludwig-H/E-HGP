# Règles à masses laminaires : vote de la thèse et ER0h, contre $H_m$

3 octobre 2026, soir (heure lue : `date -u` = 22:00 UTC à la fin des calculs). Label `majorite_vote`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only (oracles exacts en entiers, tailles d'oracle seulement)
mode=analyse_mathematique_regles_a_masses
public_status=not_claimed
GCP non utilisé. Aucune commande git. Aucune construction ni test natif. Dépôts lus seulement.
Écritures : uniquement sous build/v11-points-math/majorite_vote/.
```

Toutes les tailles sont des tailles d'**oracle** ($n\leq 9$, plus Q3 à 14 sites et $K=2$, soit 455 boules minimales,
exception assumée parce que la mission demande Q3) : aucune conclusion de coût ou d'échelle n'en est tirée. Rayon $r$,
niveau $r^{2}$ ; les tableaux de hiérarchies sont en rayon (convention des questions de l'utilisateur et de la v10),
les preuves en niveau (convention de `HIERARCHIE_POINTS.md`). Tout nombre cité sort d'un reçu de `recus/`
(empreintes `recus/SHA256SUMS`) ou d'une formule démontrée ici.

## En bref

1. **Cadre commun.** Toute règle à masses « une majorité, une fois » est une *médiane sur l'arbre* : une mesure de
   crédits $\nu_x$ portée par la région de couverture $R_x$, son point médian le plus bas, une date au-dessus. C'est
   une pendaison fidèle (proposition M, prouvée) : laminaire, sans réunion avant FULL, équivariante dès que la
   mesure l'est. Le vote **par niveau** (argmax à chaque coupe) n'en est pas une et **n'est pas laminaire** : témoin
   exact à quatre sites du plan, $K=2$.
2. **Le vote de la thèse rendu hiérarchique** (majorité stricte à dénominateur figé sur la partition de l'unité
   $S_\tau/T_x$) est une pendaison fidèle, mais : il dépend de $F_K$ et de $p$ (décisions retournées sur les deux
   triangles) ; il est **discontinu** sans cône (témoin exact à cinq sites, égalité exacte $1/2$–$1/2$) et, avec
   faces de Gabriel, **même avec cône** (témoin à six sites, $K=3$, bascule de Gabriel) ; il mesure la « masse des
   faces » que l'utilisateur a écartée : **60 à 70/125** jugements ancrés avec Gabriel, **20 à 25/125** avec toutes
   les faces.
3. **ER0h (verdict v10)** est une pendaison fidèle, équivariante, indépendante de mcs, à dates bornées
   $\alpha\leq t\leq 2\sqrt{1+\eta}\,\alpha$ (lemme 4a, prouvé), et passe **125/125**. Mais **aucune borne uniforme à
   la H3** : famille exacte de trois sites où $\vert\Delta u\vert/\delta\geq\frac{64\kappa}{129}\rho^{2}-1$
   (proposition S, prouvée) ; valeurs exactes 304,6 ; 1 205 ; 4 805 ; 19 205 ; 76 805 ; 307 200 pour
   $\rho=5\ldots160$ ; en unités v10 (saut de rayon par déplacement unité) 146,8 → 9 597 ; hors diagonale
   ($u(x,g_1)$, cinq sites) 394 → 11 878. Les « pentes jusqu'à 103 » de la v10 ne sont **pas bornées**.
4. **Pourquoi.** L'héritage proportionnel répartit le poids d'un parent lourd au prorata de masses d'enfants légers :
   rapport mal conditionné, amplifié par le cône absolu $\kappa\sqrt{A}$. Or cet héritage est **le seul** cohérent
   sous dédoublement d'une fusion multiple (proposition U, prouvée, équation de Luce puis de Cauchy) : ER0 (sans
   héritage, 36 et 50 discontinuités sur 3 000) et ER-hv à $\theta=2/5$ (21 sur 3 000, témoin exact) le confirment.
5. **Meilleure version trouvée : ER0h à cône relatif (ER0hr).** Mêmes votes, crédits et majorité que ER0h ; remise
   $\kappa'\,\mu\,(\sqrt{e}-\sqrt{A})$ au lieu de $\kappa\sqrt{A}\,\mu$. **125/125** pour $\kappa'\in\lbrace 6,10,16,24\rbrace$,
   rapport ≈ $\kappa'/2$ (4,25 ; 4,50 ; 5,00) sur les trois familles où ER0h explose, 0 discontinuité sur 3 000
   configurations adverses. **Pas de borne uniforme non plus** : pire rapport aléatoire 69,5 (ER0h 152,9 ; $H_m$
   2,6), mécanisme résiduel = somme de $N$ poids (un cas à six votes et 0,4 % de marge). Statut : candidate, continuité
   conjecturée.
6. **Contre $H_m$.** $H_m$ est uniformément stable (H3 ; mon test aléatoire : pire 2,62). Mais **aucun $m$ ne réalise
   les réponses de l'utilisateur** : toute règle qui pend dans la lignée du premier point qualifié échoue Q1 si
   $m\leq k$ ou Q2 si $m\geq k+1$ (proposition O, prouvée). Juge v10 rejoué : $H_1$ **35/125**, $H_{k+1}$ et
   $H_{\max(k+1,\mathrm{mcs})}$ **70/125** (échecs Q2, Q3, Q-Π2, Q4), ER0h, ER0hr, ER0hv **125/125**. Autre défaut
   exact : le retard de $H_1$ n'est pas relatif à l'échelle propre ($e/\alpha^{2}=r+2$ pour une paire serrée face à
   une paire lointaine à distance $r$), et $H_{k+1}$ cache toute paire jusqu'à l'arrivée d'un troisième site.
7. **Lecture.** Les réponses de l'utilisateur exigent une information de **multiplicité dans le temps** (deux
   lentilles longues contre un pont) que $H_m$, fait de suprema ultramétriques, ne voit pas ; une telle information
   est une somme, donc de sensibilité proportionnelle au nombre d'événements. Je ne démontre pas d'impossibilité
   générale ; je montre l'arbitrage sur chaque règle connue.

## 1. Sources et code

| Lu | Usage |
| --- | --- |
| `morsehgp3D_v11/docs/HIERARCHIE_POINTS.md` (version du 3 oct., § 6 ajouté pendant ce travail) | cadre F1–F2, règle $H_m$, H1–H5 |
| `morsehgp3D_v11/docs/MATHEMATIQUES.md` § 7 | P1–P5, contre-exemple $\lbrace 0,2,4\rbrace$ |
| `morsehgp3D_v11/bench/points_reference.py`, `reference/hgp11_ref/definition.py` | oracle de la définition (étage A) ; `reference_rules` pour core, cover, $H_m$ |
| `v10-verrou-points/juge_final/VERDICT_FINAL.md`, `verdict/er0h.py`, `verdict/cellules.py`, `verif_echelle_relative/{vfull,ver,ver_var,vrad}.py`, `echelle_relative/MEMO.md` § 2.6 | définition d'ER0h, juge des cellules (importé, non modifié), variantes ER-h, ER-hv |
| `v10-verrou-points/revision_cible/QUESTIONS_UTILISATEUR.md`, `juge_final/REPONSES_UTILISATEUR_20261001.md` | Q1–Q4, Q1bis, Q-Π2 |
| `v11-points-math/these/ch9_partition_stricte.txt`, `parties_I_II.txt` (Déf. 28–29) | vote du § 9.1, Prop. 7, simplexes de Gabriel |

Code écrit ici (aucun code de règle importé, sauf le juge v10 et ses variantes pour les témoins) :

| Fichier | Contenu |
| --- | --- |
| `surd.py` | sommes exactes de racines (regroupement par classes de carrés, signe certifié) |
| `arbre.py` | arbre abstrait (naissances, parents, $c_x(v)$) lu sur l'oracle v11 ou sur `vfull` v10 ; signature structurelle |
| `regles.py` | $H_m$ ; ER0h (crédits explicites) ; ER0 ; ER-hv ; ER0hr (cône relatif) ; vote de la thèse (faces toutes ou Gabriel, $p$ pair, avec ou sans cône) ; argmax par niveau ; ultramétriques exactes |
| `recoupe.py` | recoupes : arbre v11 = arbre v10 ; $H_m$ = oracle du développeur ; ER0h = `ver_var` v10 |
| `comparaison.py`, `comparaison_r.py` | hiérarchies exactes sur les fixtures |
| `cellules_regles.py` | juge v10 des 125 jugements ancrés appliqué à toutes les règles |
| `pente.py`, `rapports_aleatoires.py`, `retard_hm.py` | familles de stabilité, rapports aléatoires, retards de $H_m$ |
| `argmax_laminarite.py`, `sauts.py`, `sauts_cible.py`, `sauts_er.py`, `sauts_er_r.py` | non-laminarité ; recherches adverses de discontinuités |

Recoupes (`recus/recoupe.json`, `python3 -B recoupe.py 300 20261003`) : 847 couples nuage–ordre ($n=4$ à 8, $K=2$ à 4,
plus les fixtures) ; arbres v11 et v10 identiques 847/847 ; $H_m$ écrit ici = `reference_rules` du développeur sur
1 694 ultramétriques ; ER0h écrit ici = `ver_var` v10 sur 847 ultramétriques ; 0 désaccord. ER-hv : 320 comparaisons,
0 désaccord. Après ajout des variantes : `recus/recoupe_apres_variantes.json`, 116 couples, 0 désaccord.

## 2. Cadre : médianes sur l'arbre

Notations de `HIERARCHIE_POINTS.md` § 2 : $T_k$ arbre de FULL vu comme espace de points $(v,a)$ avec
$b_v\leq a<d_v$ ($d_v$ naissance du parent, $+\infty$ à la racine) ; $c_x(v)$ premier niveau de la vie de $v$ où son
amas discret contient $x$ ; $V_x$ l'ensemble de ces nœuds (clos vers le haut, $c_x(\mathrm{parent})=b_{\mathrm{parent}}$) ;
$R_x=\lbrace (v,a) : c_x(v)\leq a<d_v\rbrace$ ; $A_x=\min_v c_x(v)=\alpha_k(x)^{2}$. Pour un point $p=(v,a)$, son
**cône inférieur** est $\downarrow p=\lbrace (w,b) : b\leq a,\ \mathrm{anc}_a(w)=v\rbrace$.

**Lemme 4a (prouvé).** Deux composantes qui couvrent $x$ à des niveaux $\leq a$ sont réunies au niveau $4a$.

*Preuve.* Une composante $C$ couvre $x$ au niveau $a$ si et seulement si une $k$-partie $F\ni x$ appartient à $C$
avec $\beta(F)\leq a$. Sens direct : un témoin $z\in C$ à distance $\leq\sqrt{a}$ de $x$ a au moins $k$ sites dans
$\overline{B}(z,\sqrt{a})$, qui contient aussi $x$ ; une $k$-partie $F\ni x$ de cette boule a $\beta(F)\leq a$ et
$z\in W_F(a)$, convexe, donc $F$ est dans la composante de $z$. Réciproque : le centre de la boule minimale de $F$ est
dans $W_F(a)\subseteq C$ à distance $\leq\sqrt{a}$ de $x$. Tout $y\in F$ vérifie $\vert y-x\vert\leq 2\sqrt{a}$, puisque
$x$ et $y$ sont dans une boule de rayon $\sqrt{a}$. Pour deux telles parties $F_1,F_2$, $F_1\cup F_2\subseteq\overline{B}(x,2\sqrt{a})$,
donc $\beta(F_1\cup F_2)\leq 4a$, et les échanges de T1 relient $F_1$ à $F_2$ par des $(k+1)$-parties de
$F_1\cup F_2$, toutes de niveau $\leq 4a$. □

**Proposition M (médiane sur l'arbre, prouvée).** Soit $\nu_x$ une mesure positive finie, somme d'atomes portés par
des points de $R_x$. (i) À niveau fixé, les cônes $\downarrow(v,a)$ des nœuds vivants sont disjoints, et
$\downarrow p\subseteq\downarrow p'$ si $p'$ est sur la remontée de $p$. (ii) L'ensemble des points $p$ tels que
$2\nu_x(\downarrow p)>\nu_x(T_k)$ est une remontée $\mathrm{Up}(p^{*})$ non vide ; son point le plus bas
$p^{*}=(O,T_{1/2})$ appartient à $R_x$. (iii) Toute règle qui pend $x$ en $(\mathrm{anc}_{t^{2}}(O),t^{2})$ avec
$t^{2}\geq T_{1/2}$ est une pendaison fidèle : blocs laminaires, aucune réunion avant la fusion FULL des propriétaires,
chaque bloc inclus dans l'amas discret de son nœud (F1). (iv) Si $\nu_x$ et la date ne lisent que l'arbre, les
niveaux et la couverture, la pendaison est équivariante (isométries, renumérotation) sans aucun départage.

*Preuve.* (i) Un atome $(w,b)$ avec $b\leq a$ a un unique ancêtre vivant à $a$ ; la monotonie suit de la transitivité
des ancêtres. (ii) À au plus un nœud par niveau revient plus de la moitié, et la masse croît le long d'une remontée ;
au-dessus du plus haut atome, la racine porte tout. $\downarrow p^{*}$ contient un atome $q\in R_x$ et $p^{*}$ est sur
la remontée de $q$ ; $R_x$ est stable par remontée. (iii) $(\mathrm{anc}_{t^{2}}(O),t^{2})$ est sur la remontée de
$p^{*}$, donc dans $R_x$, et F1 s'applique. (iv) Une majorité stricte ne demande aucun départage. □

Les règles suivantes sont toutes de cette forme ; elles ne diffèrent que par $\nu_x$ et par la date.

## 3. Énoncés précis

### 3.1 Le vote plat de la thèse (§ 9.1, proposition 7) : hors du cadre

Faces $\tau\in F_K$ ($K$-parties « construites », Gabriel dans la version standard, Déf. 28–29), score
$S_\tau=\sum_{\sigma\supset\tau,\vert\sigma\vert=K+1}\psi(\rho(\sigma))$, $\psi(t)=t^{-p}$, normalisation
$T_x=\sum_{\tau\ni x}S_\tau$, vote $V_x(c)=\sum_{\tau\ni x,\ell(\tau)=c}S_\tau/T_x$ sur les clusters **sélectionnés**,
étiquette $\mathrm{argmax}_c V_x(c)$. Ce n'est pas une pendaison : il n'y a ni date ni nœud, la sortie dépend d'une
antichaîne choisie après condensation, et l'argmax demande une règle de départage, non équivariante (réflexion de
$\lbrace 0,2,4\rbrace$, P4).

### 3.2 Argmax à chaque niveau : non laminaire (prouvé par un témoin exact)

On attribue à chaque niveau $s$ le nœud vivant qui porte le plus de $S_\tau/T_x$ parmi les faces nées de $x$. Témoin
minimal trouvé (`recus/argmax.json`, `python3 -B argmax_laminarite.py 200 3`, 405 témoins sur 200 nuages, toutes
variantes) : sites $P_0=(0,0,0)$, $P_1=(0,3,0)$, $P_2=(2,1,0)$, $P_3=(3,2,0)$, $K=2$, $p=2$, faces toutes ou de
Gabriel. Paires nées à $1/2$ ($P_2P_3$), $5/4$ ($P_0P_2$), $2$ ($P_1P_2$) ; premier triangle à $5/2$. Avec
$\psi=1/\beta$ : $S_{23}=4/13+2/5$, $S_{02}=2/5+4/13$, $S_{12}=4/5$. Au niveau $1/2$, $P_2$ et $P_3$ votent pour le
nœud de $P_2P_3$ : bloc $\lbrace P_2,P_3\rbrace$. Au niveau $2$, trois composantes disjointes portent les faces de
$P_2$ et l'argmax de $P_2$ passe au nœud de $P_1P_2$ ($4/5$ contre $0{,}708$), tandis que $P_3$ reste dans celui de
$P_2P_3$ : le bloc $\lbrace P_2,P_3\rbrace$ est coupé avant toute fusion. La remarque 1 du § 5 du développeur est
donc un fait, à quatre points.

### 3.3 Le vote de la thèse rendu hiérarchique : $V_{1/2}$ et $V_{1/2}^{\kappa}$ (dans le cadre)

$\nu_x=\sum_{\tau\in F_x}(S_\tau/T_x)\,\delta_{(v(\tau),\beta(\tau))}$, où $v(\tau)$ est le nœud de la composante de
$\tau$ à sa naissance (il couvre $x$, donc l'atome est dans $R_x$). $V_{1/2}$ pend $x$ au point médian
$(O,T_{1/2})$ ; $V_{1/2}^{\kappa}$ ajoute le cône d'ER0h avec $A=\min_\tau\beta(\tau)$. Choix libres : $F_K$
(toutes les $K$-parties et toutes les cofaces, ou Gabriel) et $p$ ; ici $p\in\lbrace 0,2\rbrace$ pour rester
rationnel. À $K=1$ la seule face de $x$ est $\lbrace x\rbrace$ : liaison simple, comme le dit la thèse. Si
$T_x=0$ (aucune face de Gabriel, convention $1/T_x=0$), $x$ n'a pas de vote : il faut un repli (ici, poids uniformes).

### 3.4 ER0h et ses variantes (dans le cadre)

Définition du verdict v10 (§ 2.2), réécrite en mesure : bande $E_x=(1+\eta)A_x$ ; votes $v\in V_x$ avec
$c_x(v)<E_x$, poids $\omega_v=\min(d_v,E_x)-c_x(v)$, $W=\sum\omega_v$ ; masses de sous-arbre $S(v)$ ; héritage
$M(r)=S(r)$ aux racines de vote et $M(c)=M(q)\,S(c)/(S(q)-\omega_q)$ pour un enfant de vote $c$ de $q$ (lemme M de la
v10) ; $\nu_x=\sum_{\ell}M(\ell)\,\delta_{(\ell,c_x(\ell))}$ sur les feuilles de vote. Avec $G(s)$ la masse de la
lignée de $O$ et $J$ ses sauts après $T_{1/2}$ tels que $G(e^{-})<W$, $\mu(e)=2G(e^{-})/W-1$ :

$$ t_x=\max\left(\sqrt{T_{1/2}},\ \max_{e\in J}\left(\sqrt{e}-\kappa\sqrt{A_x}\,\mu(e)\right)\right),\qquad o_x=\mathrm{anc}_{t_x^{2}}(O). $$

| Variante | Mesure $\nu_x$ | Remise du cône |
| --- | --- | --- |
| ER0 | chaque vote garde $\omega_v$ à son début | $\kappa\sqrt{A}\,\mu$ |
| ER0h | héritage proportionnel | $\kappa\sqrt{A}\,\mu$ |
| ER-hv($\theta$) | l'enfant ne reçoit que $\min(1,S(c)/(\theta W))$ de sa part, le parent garde le reste | $\kappa\sqrt{A}\,\mu$ |
| **ER0hr** (proposé ici) | héritage proportionnel | $\kappa'\,\mu\,(\sqrt{e}-\sqrt{A})$ |

ER0hr interpole linéairement, en rayon, entre $\sqrt{T_{1/2}}$ (pour $\mu\geq 1/\kappa'$) et $\sqrt{e}$ (pour
$\mu\to 0$) : la remise est proportionnelle à l'attente déjà écoulée depuis la première couverture, pas à l'échelle
$\sqrt{A}$.

**Corollaire D (prouvé).** Pour ER0, ER0h, ER-hv et ER0hr : pendaison fidèle et équivariante (proposition M),
indépendante de mcs, non définie à $K=1$ ($A_x=0$, bande vide), et $\alpha_x\leq t_x\leq 2\sqrt{1+\eta}\,\alpha_x$.
*Preuve de la borne.* $t_x\geq\sqrt{T_{1/2}}\geq\sqrt{A_x}$. Tous les votes couvrent $x$ avant $E_x$ ; par le lemme 4a
ils sont réunis à $4E_x$, où la lignée porte $W$ : $T_{1/2}\leq 4E_x$ et tout saut $e\in J$ vérifie $e\leq 4E_x$. Les
deux remises sont positives ($\mu>0$ car $G$ croît depuis $T_{1/2}$, et $\sqrt{e}\geq\sqrt{A}$). □ La v10 avait ce
résultat (théorème 4) ; la preuve ici passe par le lemme 4a. Rien de tel pour $V_{1/2}$ : sans bande, ses faces vont
jusqu'au diamètre et ses dates dépassent la racine (Q3 : jusqu'à 2 192 pour une racine à 796).

### 3.5 Rappel de $H_m$

$t_x$ niveau le plus bas de $R_x^{(m)}$ (couvertures d'au moins $m$ sites), $p_x$ un point à ce niveau,
$e_x=t_x+\sup_{q\in R_x^{(m)}}(m(p_x,q)-h(q))$, propriétaire $\mathrm{anc}_{e_x}(p_x)$. Ce n'est pas une médiane : la
lignée est celle du **premier** point qualifié, et la date un supremum de durées de co-couverture.

## 4. Propriétés comparées

| Propriété | Argmax par niveau | $V_{1/2}$ (Gabriel) | $V_{1/2}^{\kappa}$ (toutes faces) | ER0 | ER0h | ER0hr | $H_m$ |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Pendaison fidèle, laminaire | **non** (§ 3.2) | oui (M) | oui (M) | oui | oui | oui | oui (F1) |
| Équivariance sans départage | non | oui ; non si $F_K$ vient d'une triangulation ambiguë | oui | oui | oui | oui | oui (H2) |
| Choix libres | $F_K$, $p$, départage | $F_K$, $p$ | $p$, $\kappa$ | $\eta$, $\kappa$ | $\eta$, $\kappa$ | $\eta$, $\kappa'$ | $m$ |
| Défini à $K=1$ | oui | oui | oui | non | non | non | oui |
| Retard borné par $2\sqrt{1+\eta}\,\alpha$ | — | non | non | oui | oui | oui | **non** (§ 6.3) |
| Continuité | — | **non** (§ 5.4) | aucun saut trouvé (1 500) | **non** | conjecture | conjecture | oui (H3) |
| Borne uniforme $\vert\Delta u\vert\leq C\delta$ | — | non | non mesurée | non | **non** (prop. S) | non (pire 69,5) | **oui**, $C=5$ |
| Jugements ancrés de l'utilisateur (125) | — | 60 à 70 | 20 à 25 | 125 | 125 | 125 | 35 ($m=1$), 70 ($m=k+1$) |

Dépendance aux choix, faits exacts (`recus/comparaison.json`) : sur la base T0_P1 des deux triangles, $V_{1/2}$ avec
$p=0$ ne forme **rien** avant la racine, avec $p=2$ et toutes les faces il forme AB | EF, avec $p=2$ et Gabriel
ABC | DEF ; sur Q3, $x$ rejoint l'amas à 547,5 ($p=0$) ou à 453,5 ($p=2$). La décision du vote de la thèse change
avec $F_K$ et avec $p$. Pour ER0h, la région de la v10 ($\eta\in[0{,}788;1{,}126]$ à $\kappa=12$) reste à respecter ;
pour ER0hr, $\kappa'=4$ échoue T0 (85/125) et $\kappa'\in\lbrace 6,10,16,24\rbrace$ passe.

Non-localité des faces « toutes » : $S_\tau$ somme $\psi(\rho)$ sur **toutes** les cofaces, lointaines comprises ;
pour une densité uniforme en dimension 3, la contribution des points entre $r$ et $2r$ est d'ordre $r^{3}r^{-p}$, donc
la somme est dominée par le loin dès que $p\leq 3$ (remarque d'ordre de grandeur, pas un théorème sur nuage fini).
Gabriel rend le score local, mais discontinu.

## 5. Stabilité

$\delta$ désigne, pour deux nuages de **même combinatoire** de FULL, le plus grand décalage des niveaux de nœuds
appariés et des débuts de couverture appariés. Ces décalages définissent un $\delta$-entrelacement avec transport des
couvertures (cadre de H3) ; le rapport mesuré $\vert\Delta u\vert/\delta$ minore donc le rapport au meilleur
entrelacement. L'appariement des nœuds ne lit pas les niveaux (naissance : sites couverts à la naissance ; fusion :
ensemble des enfants) et refuse toute non-bijection (`pente.py`).

### 5.1 Proposition S : ER0h n'a aucune borne uniforme (prouvée)

**Énoncé.** Pour $\rho\geq 2$ entier, $L=8\rho^{2}$, $h=8\rho$, $X_\rho=\lbrace x=(0,0,0),\ y_1=(L,h,0),\ y_2=(L,-h,0)\rbrace$
et $Y_\rho$ obtenu en remplaçant $y_2$ par $(L,-h,1)$, $K=2$, $\eta=1$, $0<\kappa\leq 31$. Les deux arbres ont la même
combinatoire, $\delta=1/4$, et ER0h vérifie $e_x^{X}-e_x^{Y}\geq\frac{16\kappa}{129}\rho^{2}-\frac{1}{4}$, donc
$\vert\Delta u(x,x)\vert/\delta\geq\frac{64\kappa}{129}\rho^{2}-1\to\infty$.

*Preuve.* Notons $R^{2}=L^{2}+h^{2}=64\rho^{2}(\rho^{2}+1)$. Avec trois sites et $K=2$, la seule $(K+1)$-partie est le
triangle : FULL a trois naissances (les paires) et une fusion ternaire au niveau de la boule minimale du triangle,
qui est sa boule circonscrite puisqu'il est aigu (dans $Y$ : carrés des côtés $4h^{2}+1$, $R^{2}+1$, $R^{2}$, chacun
inférieur à la somme des deux autres).
Dans $X$ : paires $xy_1$, $xy_2$ à $o=R^{2}/4=16\rho^{2}(\rho^{2}+1)$, paire $y_1y_2$ à $h^{2}$, fusion à
$b=R^{4}/(4L^{2})=16(\rho^{2}+1)^{2}$ ; $s=b-o=16(\rho^{2}+1)$. Dans $Y$ : $o_1=o$, $o_2=o+1/4$, $y_1y_2$ à $h^{2}+1/4$,
fusion à $b'=b+\varepsilon'$ où, par la formule de Héron sur les carrés des côtés,
$\varepsilon'=\frac{R^{2}(3h^{2}+1-h^{4}/L^{2})}{4(4h^{2}L^{2}+R^{2})}=\frac{(\rho^{2}+1)(192\rho^{2}-63)}{4(256\rho^{4}+\rho^{2}+1)}\in\left]0;\frac{1}{4}\right]$.
Les débuts de couverture sont ces niveaux (paires de $x$, paire $y_1y_2$, fusion) : $\delta=1/4$, mêmes nœuds.
ER0h pour $x$ : votes $c_1=xy_1$, $c_2=xy_2$ (feuilles), $P$ la fusion ($b<2o=E_x$). Dans $X$, $c_1$ et $c_2$ reçoivent
exactement $W/2$ chacun : aucune majorité stricte avant $b$, $t_x=\sqrt{b}$. Dans $Y$, $s_1=b'-o_1>s_2=s_1-1/4$ :
$c_1$ porte $s_1W/(s_1+s_2)>W/2$ dès $o_1$, $T_{1/2}=o_1$, un seul saut en $b'$ avec
$\mu=\frac{1/4}{2s+2\varepsilon'-1/4}\geq\frac{1}{8s+2}$, et $t_x'=\max(\sqrt{o},\sqrt{b'}-\kappa\sqrt{o}\,\mu)$.
Fenêtre : $\kappa\sqrt{o}\,\mu\leq\kappa\rho\sqrt{\rho^{2}+1}/(31(\rho^{2}+1))\leq\kappa/31\leq 1$ et
$\sqrt{b'}-\sqrt{o}\geq s/(8(\rho^{2}+1)+1/8)\geq 1{,}98$ : le cône est actif et
$t_x'^{2}=(\sqrt{b'}-\kappa\sqrt{o}\mu)^{2}$. Alors
$b-t_x'^{2}=-\varepsilon'+\kappa\mu\sqrt{o}\,(2\sqrt{b'}-\kappa\mu\sqrt{o})\geq-\frac{1}{4}+\kappa\mu\,o\geq-\frac{1}{4}+\frac{16\kappa\rho^{2}}{129}$,
en utilisant $2\sqrt{b'}-1\geq\sqrt{o}$ et $\kappa\mu o\geq\kappa\,16\rho^{2}(\rho^{2}+1)/(129(\rho^{2}+1))$. □

Ordre exact (`recus/pente.json`, `python3 -B pente.py`) : le rapport vaut $\kappa R L/h^{2}+O(1)\approx\kappa\rho^{2}$.

| $\rho$ | $L$ | ER0h | ER0hr ($\kappa'=10$) | ER-hv (1/20) | ER0 | $H_1$ | $H_{k+1}$ |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 200 | 304,58 | 4,281 | 1 | 1 | 1,770 | 0,770 |
| 10 | 800 | 1 204,6 | 4,258 | 1 | 1 | 1,755 | 0,755 |
| 20 | 3 200 | 4 804,6 | 4,252 | 1 | 1 | 1,751 | 0,751 |
| 40 | 12 800 | 19 205 | 4,251 | 1 | 1 | 1,750 | 0,750 |
| 80 | 51 200 | 76 805 | 4,250 | 1 | 1 | 1,750 | 0,750 |
| 160 | 204 800 | 307 200 | 4,250 | 1 | 1 | 1,750 | 0,750 |

Bornes certifiées (encadrement des racines à $2^{-200}$ près ; largeur nulle à l'affichage).

**Unités de la v10** (saut de rayon pour un déplacement unité). Même figure, $y_2$ déplacé de $1$ selon $x$ (premier
ordre), $L=48\rho^{4}$, $h=L/\rho$ : ER0h saute de **146,8 ; 596,8 ; 2 397 ; 9 597** pour $\rho=5,10,20,40$ ; ER0hr
2,19 à 2,25 ; ER0 et ER-hv 0,24 à 0,25 ; $H_1$ 0,56 à 4,47 (la constante de H3 en rayon dépend de $\sqrt{\Lambda}/t$,
limite 2 déjà notée par le développeur). Rapports au niveau : ER0h 304 → 19 204, ER0hr 4,50 à 4,55, les autres $\leq 0{,}501$.

**Hors diagonale.** Selle à cinq sites : $x=0$, $g_1=(a,b,0)$, $g_1'=(a,b,e)$, $g_2=(a,-b,0)$, $g_2'=(a,-b,-e)$ ;
$Y$ déplace $g_2,g_2'$ de $(1,0,0)$, ce qui garde les angles droits en $g_1$ et $g_2$. $x$ est couvert par $xg_1$ et
$xg_2$, qui rejoignent $G_1=\lbrace g_1,g_1'\rbrace$ et $G_2$ à $o_i+e^{2}/4$ ; $G_1$ et $G_2$ fusionnent à
$b^{2}+e^{2}/4$. Dans $Y$, ER0h pend $x$ dans le nœud qui contient $g_1,g_1'$ avant la fusion :
$\vert\Delta u(x,g_1)\vert/\delta$ = **394 ; 926 ; 2 901 ; 11 878** quand $a/b\to\sqrt{3}$ ($(a,b,e)=\lambda\,(1700,1000,100)$,
$\lambda(1720,1000,100)$, $\lambda(1728,1000,50)$, $\lambda(1731,1000,20)$ avec $\lambda=50$, 400, 2 000, 20 000) ;
ER0hr 5,04 ; 5,02 ; 5,01 ; 5,00 ; $H_{k+1}$ 1. Le propriétaire de $x$ dans $Y$ couvre $g_1$ : la variation porte
sur un bloc, pas seulement sur une date d'entrée.

**Réponse à la question posée.** Les « pentes locales jusqu'à 103 » mesurées par la v10 ne sont pas bornées : la
pente d'ER0h croît comme $\kappa(L/h)^{2}$ dans une famille de trois sites, en niveau comme en rayon, sur la
diagonale comme hors diagonale. La v10 écrivait « aucune constante uniforme n'est revendiquée » : c'est confirmé et
précisé (mécanisme, ordre exact, trois sites suffisent).

### 5.2 Mécanisme et proposition U : l'héritage proportionnel est forcé

Dans la proposition S, le parent $P$ (poids propre $\approx o$) cède tout à deux enfants de masses $s_1,s_2$ d'ordre
$s=o/\rho^{2}$ ; la part $s_1/(s_1+s_2)$ varie de $\Delta/(2s)$ quand un début bouge de $\Delta$, et le cône absolu
convertit la marge en date avec un gain $2\kappa\sqrt{oe}$ indépendant de $s$. Peut-on changer de parts ?

**Proposition U (prouvée, cadre abstrait).** Considérons les redistributions sans rétention où chaque vote cède tout
son total à ses enfants de vote selon des parts strictement positives, symétriques, continues, ne dépendant que des
masses de sous-arbre des enfants. Si la mesure de crédits est continue sous la dégénérescence D2 (une fusion de trois
enfants devient, par une perturbation arbitrairement petite, deux fusions binaires aux niveaux $b$ et $b+\epsilon$,
dans l'un ou l'autre ordre), alors les parts sont proportionnelles : $f_c=S(c)/\sum S$.

*Preuve.* Soit $h(a,b)$ la part d'un enfant de masse $a$ face à un frère de masse $b$, $h(a,b)+h(b,a)=1$. Quand
$\epsilon\to 0$, le nœud intermédiaire a la masse de ses enfants. Les deux ordres de D2 imposent, pour la part du
premier de trois enfants de masses $a,b,c$ : $h(a,b+c)=h(a+b,c)\,h(a,b)$. Posons $q(a,T)=h(a,T-a)$ pour $0<a<T$ ; il
vient $q(a,T)=q(a,m)\,q(m,T)$ pour $a<m<T$. Fixons $T_0$ et $\varphi(a)=q(a,T_0)>0$ : $q(a,m)=\varphi(a)/\varphi(m)$.
La complémentarité $q(a,a+b)+q(b,a+b)=1$ donne $\varphi(a)+\varphi(b)=\varphi(a+b)$ sur $]0;T_0[$, équation de
Cauchy à solution positive, donc linéaire : $q(a,m)=a/m$ et $h(a,b)=a/(a+b)$. Pour $r$ enfants, on dédouble un enfant
après l'autre. □

Deux compléments, avec témoins exacts. (D1) Si une feuille éphémère coupe la vie d'un vote, tout ce que le haut du
vote retient est daté à la coupure au lieu du début : la rétention doit tendre vers 0, ce que viole ER0 (discontinue,
v10 et ici 36 puis 50 sauts sur 3 000). (D2 avec rétention) ER-hv compose ses rétentions le long d'une chaîne dédoublée
: témoin exact $\theta=2/5$, $P=\lbrace(0,0,0),(0,2,0),(0,2,2),(1,2,0),(2,3,0)\rbrace$, $K=2$, site 1 déplacé de
$(0,0,1)$ : le crédit de la feuille passe de 718 750 à 436 372 (sur $W\approx 10^{6}$), la majorité est perdue, le
site 2 entre à 1 118,03 au lieu de 1 000 ; saut 118,03 puis 1 180,34 à l'échelle 10 (`recus/sauts_er.json`,
21 discontinuités sur 3 000). Aucune discontinuité trouvée pour $\theta=1/20$ (3 000), mais le mécanisme D1
s'applique dès qu'un enfant pèse moins de $\theta W$ : je conjecture des sauts pour tout $\theta>0$.

Conséquence : dans la classe « votes de nœuds, crédits par redistribution, majorité, cône absolu », la continuité
force l'héritage proportionnel, et celui-ci donne la pente non bornée de la proposition S. Le seul levier qui reste est
la date.

### 5.3 Le cône relatif (ER0hr)

À la jonction mal conditionnée de la proposition S, l'attente $\sqrt{e}-\sqrt{A}$ est d'ordre $s/\sqrt{o}$ : elle
compense exactement le $1/s$ de la part, et $\Delta e\approx\kappa'\Delta/2$. Mesures exactes : rapports 4,25
(isocèle), 4,50 (radiale), 5,00 (selle) contre ER0h non borné. Juge des cellules : **125/125** pour
$\kappa'=6,10,16,24$, 146 jugements forcés et 21 dérivés passés (`recus/cellules_ER0hr.json`). Recherche adverse :
**0 discontinuité** sur 3 000 configurations pour $\kappa'=10$ et $\kappa'=6$, avec ER0 comme témoin positif (50
sauts) dans la même campagne (`recus/sauts_er_relatif.json`). Continuité : la remise ne lit que $A$, $e$ et
$\mu(e)$ ; elle est continue en $\mu\to 0$, sous l'échange de deux sauts (les deux ordres donnent le même maximum
$\sqrt{e}-\kappa'\mu_{\mathrm{avant}}(\sqrt{e}-\sqrt{A})$), et sous le passage du seuil de majorité ($T_{1/2}$
n'entre que par le plancher $\sqrt{T_{1/2}}$) ; les autres transitions (D1, D2) sont celles d'ER0h, que l'héritage
proportionnel rend continues pour la mesure. Ce n'est pas une preuve complète : statut **conjecture**.

**Limite mesurée.** 3 994 paires aléatoires de même combinatoire ($n=4$ à 7, $K=2$ ou 3, un site déplacé de 1,
`recus/rapports_aleatoires.json`) : pire rapport ER0h 152,9 (72 paires au-delà de 20), ER-hv(1/20) 152,9, ER0hr 69,5
(16 au-delà de 20), $H_1$ 2,62, $H_{k+1}$ 2,00. Le pire cas d'ER0hr ($K=3$, sept sites) n'a **aucun héritage actif** :
six votes feuilles, marge $\mu=0{,}0041$ ; un déplacement unité change les six poids d'environ $\delta=500$ et la marge
de 0,0029 ; gain $2\kappa'\sqrt{e}(\sqrt{e}-\sqrt{A})\approx 1{,}2\cdot 10^{7}$. Ordre de grandeur
$\kappa'\,2N\,(e-A)/W$ : toute règle dont la marge est une somme de $N$ poids a une sensibilité qui croît avec le
rapport $(e-A)/\bar\omega$. Je conjecture pour ER0hr une borne locale $\vert\Delta e\vert\leq C(\kappa',\eta)\,N_x\,\delta$
(non prouvée) et je ne revendique aucune constante uniforme.

### 5.4 Continuité du vote de la thèse

| Règle | Discontinuités trouvées | Témoin minimal exact |
| --- | --- | --- |
| $V_{1/2}$, Gabriel, sans cône | 6 sur 120 | $P=\lbrace(0,1,1),(0,2,1),(3,0,0),(3,0,1),(3,2,0)\rbrace$, $K=2$, $p=2$, site 0 déplacé de $(1,0,0)$ |
| $V_{1/2}$, toutes faces, sans cône | 1 sur 120 | même témoin |
| $V_{1/2}^{\kappa}$, Gabriel | 5 sur 120 | $P=\lbrace(0,1,0),(0,2,0),(1,1,1),(2,1,0),(2,1,1),(3,3,2)\rbrace$, $K=3$, site 0 déplacé de $(1,0,0)$ |
| $V_{1/2}^{\kappa}$, toutes faces, $p=0$ et $p=2$ | 0 sur 1 500 | — |

Premier témoin : dans $X$ dilaté par $D$, les deux composantes qui portent les faces du site 0 ont **exactement**
$1/2$ chacune (fractions exactes), donc aucune majorité stricte avant la racine ; dans $Y$ la composante 7 porte
$109183586237945561982534514027/218367136481150054695941428054>1/2$ et le site 0 entre plus tôt. Saut 71,05 ;
706,43 ; 7 060,23 pour $D=10^{3},10^{4},10^{5}$ : saut$/D$ = 0,0711 ; 0,0706 ; 0,0706, constant, alors que le
déplacement normalisé $1/D$ tend vers 0. Second témoin : la 4-partie $\lbrace 1,2,3,4\rbrace$ est de Gabriel dans
$X$ et ne l'est plus dans $Y$ ; $u(0,4)$ passe de 1 224,74 à 1 117,59 avec ou sans cône ; saut$/D$ = 0,107 aux trois
échelles. Statut : **testé** (trois dilatations exactes ; la persistance pour tout $D$ n'est pas démontrée).

## 6. Comparaison avec $H_m$ sur les fixtures exactes

### 6.1 Hiérarchies (rayons ; blocs d'au moins deux sites ; `recus/comparaison*.json`)

| Fixture | $H_1$ | $H_{k+1}$ | ER0h(1, 12) | ER0hr(1, 10) | $V_{1/2}$ Gabriel, $p=2$, cône |
| --- | --- | --- | --- | --- | --- |
| Deux triangles exacts v11 (racine 1,225) | AB \| EF dès 0,816 | **ABC \| DEF** dès 0,816 | **ABC \| DEF** dès 0,816 | idem | idem |
| T0_P1 (racine 1 931,827) | AB \| EF ; ABC \| DEF à 1 931,816 seulement | **ABC \| DEF** dès 1 154,684 | **ABC \| DEF** dès 1 154,684 | idem | idem |
| T0_S_plan (racine 1 897,367) | AB \| EF dès 1 250 | **ABC \| DEF** dès 1 250 | **ABC \| DEF** dès 1 250 | idem | AB \| EF, puis ABC \| DEF à 1 318,7 |
| Cinq points (racine 6) | 12 \| 34 dès 2,261 | 12 \| 34 dès 3,333 ($100/9$ en niveau) | 12 \| 34 dès 2 | dès 2 | dès 3,333 |
| $\lbrace 0,2s,4s\rbrace$, $s=1000$ | médian à la racine 2 000 | racine | racine | racine | racine |
| $\lbrace 0,2s,4s+1\rbrace$ | gx dès 2 000,25 | racine 2 000,5 | gx dès 1 994,495 | gx dès 1 995,494 | racine |
| Q1, pont 1 700 (racine 1 787,36) | CD à 1 707,98 (a) | **ABC \| DEF** dès 1 154,684 | **ABC \| DEF** dès 1 407,584 | dès 1 438,354 | dès 1 155,747 |
| Q2 (racine 75,26) | {x,a} dès 67,37, tard | {x,b1,b2} dès 60,36 (b) | **{x,a}** dès 50 | dès 50 | {x,b1,b2} dès 60,36 |
| Q3 (racine 796,117) | x dans le filament à 744,18, tard | x dans l'amas dès 590,54 (b) | **{x,f1}** dès 694,42 | dès 695,43 | x dans l'amas dès 453,47 |
| Q4, $K=3$ (racine 1 334,166) | PQR \| P2Q2R2 dès 899,5 ; CmD à 1 262,27 | CPQR \| DP2Q2R2 dès 866,03 (b) | PQR \| P2Q2R2 dès 866,03 ; **CmD** dès 1 157,28 | CmD dès 1 197,71 | CPQR \| DP2Q2R2 dès 1 078,17 |

Gras : réponse de l'utilisateur. Sur $\lbrace 0,2s,4s+1\rbrace$ (déplacement 1, $\delta\approx 2000$ en niveau), le
saut de niveau du site médian vaut 1 000 pour $H_1$ et 21 990 pour ER0h : rapport 11, dans le régime « cône » déjà
mesuré par la v10 (F2, pente 15,4 en rayon).

### 6.2 Les 125 jugements ancrés (juge v10 rejoué, `recus/cellules_*.json`)

| Règle | Total | T0 (40) | Q1 + Q1bis (20) | Q2 (5) | Q3 (25) | Q-Π2 (5) | Q4 (30) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ER0h(1, 12) | **125** | 40 | 20 | 5 | 25 | 5 | 30 |
| ER0hr($\kappa'=6$ à 24) | **125** | 40 | 20 | 5 | 25 | 5 | 30 |
| ER-hv($\theta=1/20$, 1/5, 2/5) | 125 | 40 | 20 | 5 | 25 | 5 | 30 |
| ER0(1, 12) | 125 | 40 | 20 | 5 | 25 | 5 | 30 |
| $H_{k+1}$ et $H_{\max(k+1,\mathrm{mcs})}$ | 70 | 40 | 20 | 0 | 0 | 0 | 10 |
| $V_{1/2}$ Gabriel, $p=2$, sans cône | 70 | 40 | 20 | 0 | 0 | 0 | 10 |
| $V_{1/2}^{\kappa}$ Gabriel, $p=2$ | 60 | 30 | 20 | 0 | 0 | 0 | 10 |
| $H_1$ | 35 | 0 | 10 | 0 | 0 | 5 | 20 |
| $V_{1/2}^{\kappa}$ toutes faces, $p=2$ | 25 | 0 | 10 | 0 | 0 | 5 | 10 |
| $V_{1/2}^{\kappa}$ toutes faces, $p=0$ | 20 | 0 | 5 | 0 | 0 | 5 | 10 |

Toutes les règles passent les 146 jugements forcés. Le juge et les cibles sont ceux de la v10 (importés tels quels) ;
ER0h y redonne 125/125, comme le verdict.

### 6.3 Ce que $H_m$ ne peut pas faire, et un défaut d'échelle

**Proposition O (prouvée).** Soit une pendaison fidèle qui, pour un seuil $m$ commun à tous les sites, pend chaque
site dans la lignée de son point le plus bas de $R_x^{(m)}$ (c'est le cas de $H_m$, H1). Si $m\leq k$, elle échoue Q1 ;
si $m\geq k+1$, elle échoue Q2.

*Preuve.* Faits exacts de FULL$_2$ (oracle v11 et Γ$_K$ v10, identiques). Q1 : $R_C$ a un unique point le plus bas,
la paire CD née à $850^{2}=722\,500$ ; la lignée de CD ne rencontre celle de ABC qu'à la racine $3\,194\,656=1787{,}36^{2}$
(la boule minimale de ACD est la boule diamétrale de AD) ; pour $m\leq 2$, C reste hors du bloc de ABC sur
$[1698;1787{,}36[$, où la cellule exige ABC | DEF. Q2, $m=3$ : $xa$, $xb_1$, $xb_2$ ne couvrent que deux sites ; le
premier point qualifié est le nœud de $\lbrace x,b_1,b_2\rbrace$ né à $60{,}36$ ; sa lignée ne contient $a$ qu'à
75,122 ; à 61 le bloc de $x$ ne peut pas être $\lbrace x,a\rbrace$, que la cellule exige. Pour $m=4=n$, aucun nœud
n'est qualifié avant 75,122. □ Le même argument met $x$ dans l'amas en Q3 ($m=3$ : l'amas couvre neuf sites dès
453,47, la lentille $xf_1$ deux) et C dans CPQR en Q4 ($m=4$). Les réponses Q1 et Q2 diffèrent par la **durée de la
multiplicité** (lentilles AC et BC séparées pendant 15 % du rayon ; $xb_1$, $xb_2$ réunies après 0,25 %), information
qu'aucune règle « première couverture qualifiée » ne lit.

**Retard non relatif de $H_m$ (prouvé, forme close).** Paire serrée $x=(0,0,0)$, $y=(2,0,0)$ ($A_x=1$) et paire
lointaine $z_1=(-r,0,0)$, $z_2=(-r-1,0,0)$, $K=2$. Paires $z_1z_2$ à $1/4$, $xy$ à 1, $xz_1$ à $r^{2}/4$ ; triangles
alignés : $xz_1z_2$ à $(r+1)^{2}/4$, $xyz_1$ à $(r+2)^{2}/4$ (racine). Pour $H_1$, le rival $(xz_1,r^{2}/4)$ rejoint la
lignée de $xy$ à $(r+2)^{2}/4$ : $D=r+1$ et $e_x=r+2$, soit $e_x/A_x=r+2$ (12 ; 102 ; 1 002 ; 10 002 pour
$r=10\ldots 10^{4}$, `recus/retard_hm.json`). Pour $H_3$, la paire $xy$ n'est jamais qualifiée : $e_x=(r+1)^{2}/4$. ER0h
et ER0hr : $e_x=1$ (les rivaux sont hors de la bande). Variante perpendiculaire : $H_1$ donne 2, $H_3$ donne
$r^{2}/4+1$. Le retard de $H_m$ est une durée absolue de co-couverture, pas une durée relative à l'échelle propre du
site ; et $H_{k+1}$ cache toute paire (en Q2, $b_1b_2$, née à 8,016, n'apparaît qu'à 60,36).

## 7. Ce qu'une règle à masses apporte que $H_m$ n'a pas, et inversement

| | Règles à masses (ER0h, ER0hr) | $H_m$ |
| --- | --- | --- |
| Réponses ancrées de l'utilisateur | 125/125 : multiplicité × temps de couverture (Q1 contre Q2), précocité (Q2), durée (Q3, Q4) | 35 ou 70/125 ; impossible pour tout $m$ (prop. O) |
| Horizon de l'attente | borné : $t\leq 2\sqrt{1+\eta}\,\alpha$ (corollaire D) | non borné relativement à $\alpha$ (§ 6.3) |
| Partition de l'unité avant engagement (idée du § 9.1) | oui : parts $M(\ell)/W$ par branche, réutilisables par une tête fractionnaire | non |
| Stabilité uniforme | non (prop. S ; ER0hr pire 69,5 au hasard) | oui, H3 ($\leq 3\delta$, $\leq 5\delta$) ; pire mesuré 2,62 |
| Continuité | conjecture (ER0h : 0 saut sur 3 000 ici et 29 750 en v10 ; ER0hr : 0 sur 3 000) | prouvée |
| Paramètres | deux ($\eta$ ; $\kappa$ ou $\kappa'$), réglés sur les cellules | un ($m$), sens combinatoire, transporté exactement par P5 |
| $K=1$ | non défini | liaison simple |
| Coût, port | votes par point 41 à 299 en moyenne sur LiDAR, rationnels jusqu'à 4 790 bits (v10) | port natif fait, porte G4 conforme (développeur) |
| Petits groupes près de $K$ | faiblesse mesurée par la v10 (0,26 contre 0,41 pour cover à $K=5$) | non mesuré sur ce critère |

## 8. Lecture critique

**Thèse.** Le § 9.1 a raison sur l'objet (recouvrement, partition des $(K-1)$-simplexes) et sur l'idée de répartir
l'unité d'un point avant de l'engager ; ER0h en est une réalisation. Mais : la proposition 7 est plate et ne se
hiérarchise pas par argmax (non-laminarité à quatre sites) ; sa version à majorité figée est discontinue (égalité
exacte à cinq sites, bascule de Gabriel à six) ; ses poids dépendent de $F_K$ et de $p$ au point de retourner la
décision sur les deux triangles ; surtout, ils mesurent une **masse de faces** qui donne l'option (b) en Q2, Q3 et Q4,
que l'auteur a lui-même écartée le 1er octobre. La partition de l'unité est la bonne idée, la mesure $S_\tau/T_x$ n'est
pas la bonne mesure.

**Verdict v10.** ER0h passe bien 125/125 et je ne trouve aucun saut (3 000 configurations de plus). Trois corrections :
(1) la pente n'est pas seulement « non bornée a priori » : elle est non bornée, d'ordre $\kappa(L/h)^{2}$, sur trois
sites ; (2) la cause n'est pas que l'héritage soit un quotient en général, mais le gain absolu $\kappa\sqrt{A}$ du cône
appliqué à une marge mal conditionnée ; le cône relatif l'enlève sans perdre une cellule ; (3) la proposition U montre
que l'héritage proportionnel n'est pas un choix parmi d'autres : c'est le seul cohérent sous dédoublement, ce qui
explique les sauts d'ER0 et d'ER-hv.

**Proposition du développeur.** H1–H3 sont justes à ma lecture, et mon test aléatoire est compatible (pire 2,62).
Mais $H_{k+1}$ contredit trois des cinq groupes de réponses ancrées (Q2, Q3, Q4) et Q-Π2, et la proposition O montre
qu'aucun $m$ ne corrige cela ; $m=\max(k+1,\mathrm{mcs})$ réintroduit mcs dans la projection, ce que la v10 avait
écarté (Q1bis, Q-Π2), sans gain ici (70/125 comme $m=k+1$). Le retard de $H_m$ n'a pas d'horizon relatif. La limite 3
de son § 5 (« deux rivaux pèsent pareil ») est exactement ce qui sépare Q1 de Q2. Ses mesures (meilleur bloc contre
HDBSCAN) jugent la présence des objets, pas la sémantique des cellules : les deux critères sont nécessaires.

**Auditeur.** Sa fermeture qualifiée n'est pas une pendaison fidèle (réunion avant FULL, cinq points) ; elle sort du
cadre de ce rapport et je ne l'ai pas rejouée.

## 9. Recommandation

1. La meilleure version laminaire des règles à masses trouvée ici est **ER0hr($\eta=1$, $\kappa'=10$)** : mesure de
   crédits d'ER0h (seul héritage cohérent, prop. U), majorité stricte figée (prop. M), cône **relatif**. Elle garde
   tout ce qu'ER0h a prouvé (fidélité, équivariance, indépendance de mcs, dates bornées) et ses 125/125, et retire le
   mécanisme de la proposition S. Obligations avant tout port : preuve de continuité ; borne locale en $N_x$ ou
   contre-exemple ; catalogue v2 complet (215 entrées, non rejoué ici faute de budget) ; petits objets LiDAR (R9 de
   la v10) ; coût aux tailles d'intérêt.
2. Le vote de la thèse ne doit pas servir de projection hiérarchique : il sert, après engagement, de tête fractionnaire
   (parts $M(\ell)/W$), comme la v10 le suggérait.
3. $H_{k+1}$ reste le témoin uniformément stable. Il convient de le publier **à côté** d'une règle à masses, pas à sa
   place, tant que les réponses ancrées de l'utilisateur font foi : une règle stable qui contredit ces réponses
   répond à une autre question.
4. Question ouverte pour l'utilisateur, si l'on veut une stabilité uniforme démontrée : accepter que Q2 ou Q1 tombe
   (prop. O), ou accepter une constante locale en $N_x$.

## 10. Limites

- Oracles bornés seulement ($n\leq 9$, plus Q3 à 14 sites) : aucune pente d'échelle, aucun coût.
- Continuité d'ER0h et d'ER0hr : conjectures appuyées sur 3 000 configurations adverses chacune, pas des preuves.
- Proposition U : cadre abstrait (redistributions sans rétention, parts positives) ; la réalisation géométrique d'un
  saut pour toute règle non proportionnelle n'est montrée que par les témoins ER0 et ER-hv.
- Témoins de discontinuité du vote : trois dilatations exactes, persistance non démontrée pour tout $D$.
- Les 125 jugements sont ceux de la v10 ; je n'ai pas rejugé le catalogue v2 (cibles extrapolées) ni les mesures dev
  et LiDAR.

## 11. Reproduction

```bash
cd /workspaces/E-HGP/build/v11-points-math/majorite_vote
python3 -B surd.py && python3 -B fixtures.py                      # autotest exact, coordonnées = catalogue v10
python3 -B recoupe.py 300 20261003 > recus/recoupe.json           # 41 s
python3 -B comparaison.py > recus/comparaison.json                 # 1 s
python3 -B comparaison_r.py > recus/comparaison_relatif.json
python3 -B cellules_regles.py ER0h,H1,Hk1,Hmcs > recus/cellules_H_ER0h.json
python3 -B cellules_regles.py VOTEg2,VOTEg2nc,VOTEa2,VOTEa0,ER0 > recus/cellules_VOTE.json
python3 -B cellules_regles.py ER0hv20,ER0hv5,ER0hv25 > recus/cellules_ER0hv.json
python3 -B cellules_regles.py ER0hr4,ER0hr6,ER0hr10,ER0hr16,ER0hr24 > recus/cellules_ER0hr.json
python3 -B pente.py > recus/pente.json                             # familles isocèle, radiale, selle
python3 -B argmax_laminarite.py 200 3 > recus/argmax.json
python3 -B sauts.py 120 11 > recus/sauts.json
python3 -B sauts_cible.py 1500 29 > recus/sauts_cible.json         # 92 s
python3 -B sauts_er.py 3000 41 240 > recus/sauts_er.json           # 178 s
python3 -B sauts_er_r.py 3000 43 150 > recus/sauts_er_relatif.json # 140 s
python3 -B rapports_aleatoires.py 7 150 > recus/rapports_aleatoires.json
python3 -B retard_hm.py > recus/retard_hm.json
(cd recus && sha256sum -c SHA256SUMS)
```

Temps CPU total de ce travail : environ 12 minutes. `sauts_er.py` juge ER0, ER0h, ER-hv (1/20 et 2/5) et
$H_{k+1}$ ; `sauts_er_r.py` est la seconde campagne (ER0 témoin, ER0hr à $\kappa'=10$ et 6). Le fichier
`bench/points_reference.py` du développeur a changé pendant ce travail (ajout de règles en rayon) ; les fonctions
utilisées ici (`reference_rules`, `reference_ultrametric`) sont inchangées et la recoupe a été rejouée après.
