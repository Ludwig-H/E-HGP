# GEN_v2 — Générateur du catalogue critique de morsehgp3D_v10 (conception finale)

28 septembre 2026. Révision de `GEN_v1.md` après la critique adverse (`crit_GEN/`). Sous-système : génération des boules critiques (supports de 2 à 4 positions, centre dans l'intérieur relatif de l'enveloppe du support, intérieur strict I, coquille complète U, admission). Document de conception, pas de code produit. Chaque chiffre nouveau a un reçu rejouable sous `design/gen_v2_probe/` (§ 15).

```text
phase=exploration_v10_conception_hors_registre
backend=cpu_reference (puis cuda_g4, subordonné)
profile=quantized_u18_input_only (poids natifs ; voie large i256 pour B > 19)
mode=audit_independant_math_and_architecture
public_status=not_claimed
GCP non utilisé
```

Priorité de complexité fixée par l'utilisateur : **le régime LiDAR** (trames SemanticKITTI sans sol de 30 à 60 k sites, grille de 1 mm sur 18 bits, K5 puis K10, contrats de 1 s puis de 100 ms sur G4). Les bancs synthétiques 8k/16k/32k servent au contrôle de pente et de familles adverses ; les petits nuages ne servent que d'oracle.

---

## 0. Réponse à la critique

### 0.1 Constats bloquants et majeurs

| constat | verdict | correction dans v2 | preuve ou reçu |
|---|---|---|---|
| B1 — la règle de stagnation à 3 niveaux fait refuser les amas, les filaments, les grilles et le banc synthétique | **fondé** ; la règle n'avait aucune mesure | règle **supprimée**. Un nœud se découpe si et seulement si $\lvert L(Q)\rvert>M(K)$ et côté $>2^{-T}$ mm. Au côté minimal, la feuille est « forcée » (énumérée jusqu'à 128 sites), au-delà refus. Racine fixe $[0,2^{B})^{3}$ | 98 exécutions à racine fixe B = 18 (T = 6, sauf 5 grilles à T = 0) sur toutes les familles (amas 8k/16k/32k K5, amas K10 à 8k et 16k, filaments, coquilles, uniforme, terrain, 21 entrées du banc synthétique, 9 LiDAR emboîtées K5 et K10, 000200, brute avec sol, grilles, doublons) : **0 refus**, 0 feuille forcée hors des grilles entières (§ 11.4) |
| M1 — la prémisse « borne basse fausse avec poids » est fausse ; la fenêtre pondérée n'est pas un intervalle ; voie pondérée jamais validée | **fondé** | admission **unique et positionnelle** $p_{w}+q_{\min}\le K+1$ ($q_{\min}$ en positions), alignée sur TOWER_v1. Lemme W prouvé (§ 3.8). Ordres critiques = ensemble non intervalle, calculé par la tour ; fixtures F8a et F8b | preuve § 3.8 ; oracle indépendant pondéré sur 600 nuages (§ 12.1) ; J-KM2 enregistrement par enregistrement sur deux entrées pondérées (trois comparaisons, § 11.7) |
| M2 — oracle non indépendant ; « 27 cas » sans reçu | **fondé** | oracle indépendant Fraction pondéré (dérivé de celui du critique), comparaison **enregistrement par enregistrement** ; l'allégation des 27 cas est retirée | 1 200 nuages, 28 800 exécutions de la sonde (4 configurations, 6 valeurs de K), 3 699 196 enregistrements comparés, **0 écart** |
| M3 — porte de coût « 1,15 par compteur et par boule » rouge sur les propres mesures | **fondé** | porte redéfinie : exposants de **travail** (arbre contre n, feuille contre boules, total contre n + boules) ; compteurs individuels en diagnostic ; LiDAR jugé en absolu | exposants mesurés ≤ 1,126 partout, arbre seul 1,21 sur la série emboîtée s02, publiée en diagnostic (§ 12.6) |
| M4 — projections de 70 à 190 ms (K5) et 0,3 à 0,75 s (K10) fondées sur un gain non mesuré | **fondé** | microbancs faits : arbre ingénierié ×4,3 à ×5,7 **à listes identiques**, feuille ×1,5 à ×1,8 **à catalogue identique** ; projections refaites sur ces mesures | 3,7 à 5,8 µs CPU par boule, soit ≈ 0,15 à 0,25 s à K5 et ≈ 0,5 à 0,8 s à K10 sur G4 en CPU seul (± 30 % selon le processeur, § 11.9). v1 était optimiste d'un facteur 2 à 3 ; l'estimation du critique (150 à 300 ms à K5) est confirmée |
| M5 — juges biaisés (J2) ou circulaires (J5) aux ordres K−1 et K | **fondé** | J2 remplacé par le **juge des boîtes** (borne de rayon $d_{K}(q_{0})+\mathrm{diam}(Q)$, ARCH § 12.4) ; J5 remplacé par **J-KM2** : égalité $\mathrm{cat}(K)=\mathrm{restrict}(\mathrm{cat}(K+2))$ et Euler sur $\mathrm{cat}(K+2)$ | égalité vérifiée enregistrement par enregistrement sur 4 comparaisons (3 entrées, dont 2 pondérées) ; coût mesuré de K+2 : ×1,9 à K5, ×1,35 à ×1,46 à K10 (§ 11.7) |

### 0.2 Constats mineurs

| constat | traitement |
|---|---|
| format : 17 o fixes, pas 12 | format aligné sur ARCH v2 : 11 o fixes + 4 o par identifiant + ≤ 4 o de table des niveaux (§ 8) ; `n_shell` tient sur u8 car $\lvert U\rvert\le M_{\mathrm{hard}}=128$ |
| bornes : recensement q3 = $288E^{6}$, q4 = $144E^{5}$ ; E doit inclure le côté de boîte | table refaite (§ 5.2) ; **deux repères** : les centres et le recensement n'utilisent que des différences de sites (E sans la boîte), les tests de boîte ont leur propre étendue $E'\ge s$ |
| rejet des quadruplets coplanaires absent du pseudo-code | ajouté (§ 6.3) |
| lemme K : les 13 dalles ne sont qu'un test suffisant | reformulé (§ 3.6) |
| § 9.1 : critère de fin de la phase en largeur insatisfiable | « ou nœud terminal » ajouté (§ 10.1) |
| § 5.2 point 6 : « une passe » mal définie (3 valeurs par axe, gardes différentes) | revendication retirée ; filtrage par enfant, mesuré (§ 11.5) ; la garde elle-même disparaît (lemme D-loc, § 3.2) |
| F10 non porteuse | retirée comme fixture de marge ; les extrêmes u18 restent des fixtures de bornes |
| mémo : appartenance au plan en i256 fréquente sur les grilles | mémo **par masque de coquille U** (lemme U, § 3.7) : aucune arithmétique |
| J4 énumère $2^{u}$ sous-coquilles | J4 plafonné à $u\le20$, refus explicite du juge au-delà |
| une trame brute avec sol existe | mesurée : K5 2 822 052 boules, **K10 11 387 391** (92,3 par site), aucun refus (§ 11.2) |
| K = 2–3 et K = 11–16 non mesurés | mesurés ; table M(K) révisée (§ 7) |
| « +48 % » et « 99,98 % » non justifiés | « +48 % » retiré (la règle pondérée à part n'existe plus) ; 572 / 1 407 885 = 0,041 % de coquilles étendues, donc 99,96 % de régulières |
| conversion U192 → double fidèle | spécifiée (§ 6.4) |
| le digest impose le niveau réduit | digest **sans niveau ni rang** : suite canonique de ($S^{*}$, I, U, poids, $q_{\min}$) ; les rangs sont exclus parce que J-KM2 compare des catalogues de rangs différents (§ 6.5) |
| GPU : `__int128` natif depuis CUDA 11.5 ; IMAD émulé | branche d'émulation retirée ; coût des multiplications 64 et 128 bits compté (§ 10.2) |
| P-COST-CPU indéfini sous SMT ; P-PERM vert par construction | définitions données (§ 12.7) ; P-PERM gardé comme porte de la préparation, avec un mutant qui le rend non vacant |

### 0.3 Ce que les sondes de cette révision ont appris en plus

1. **La garde G est redondante.** Si $S_{0}\subseteq Y$, tout site exclu par G l'est par D ; de même pour D′ (§ 3.2). L'arbre n'a plus qu'**un** prédicat. Listes terminales identiques mesurées (empreintes `e3baefa5…` à K5, `552daaab…` à K10).
2. **Dominance sans multiplication.** En coordonnées locales à la boîte, y domine x si et seulement si $A(x)-A(y)>2s\sum_{i}\max(0,x'_{i}-y'_{i})$, avec $2s$ puissance de deux, donc un décalage (§ 3.2). Noyau vectorisé sans branchement : 1,5 cycle par test mesuré.
3. **Bissectrice = non-dominance mutuelle.** Le test bissectrice–boîte de la feuille se lit dans la matrice de dominance, sans calcul (§ 3.6).
4. **Lemme Z sous forme fermée i64** : $\lvert u_{k}Z_{1}-v_{k}Z_{0}\rvert\le2s(\lvert\nu_{i}\rvert+\lvert\nu_{j}\rvert)$ avec $\nu=u\times v$ (§ 3.6).
5. **Mettre les sites à l'échelle $2^{T}$ casse l'i128.** Une version intermédiaire de la sonde, qui multipliait les coordonnées de sites par $2^{6}$, a émis **une boule q3 fausse** sur `clusters_8000` (510 759 au lieu de 510 758) : le recensement d'un triangle de 30 m débordait. D'où les deux repères du § 5 et le mutant M13 (reçu `out/mutant_M13_scaled_sites.txt`).
6. **Une boule critique est déterminée par sa coquille U** (lemme U, § 3.7). Le mémo des coquilles étendues devient un ensemble de masques de bits.
7. **T ne coûte rien là où les listes décroissent.** À T = 6, l'arbre est identique à celui de T = 0 (mesuré sur `clusters_8000` et 000200 : mêmes nœuds, même catalogue) ; les niveaux sous-millimétriques ne servent qu'aux treillis. Sur la grille $20^{3}$ au pas 1, T = 6 supprime les 7 726 feuilles forcées et divise le coût par 1,7.

---

## 1. Verdict

1. **Voie retenue : boîtes de centres à listes certifiées** (prototype `cble` de L13, durci). Chaque boule critique appartient à l'unique feuille demi-ouverte qui contient son centre. Chaque feuille porte une liste **K-certifiée** qui contient la boule K-NN fermée de tout point de la feuille. L'énumération locale de cette liste est complète et son recensement exact (théorème C). La preuve tient sans position générale et avec multiplicités.
2. **Rejetées** : le front WSPD de la v9 (quasi quadratique sur amas, cubique sur coquilles) et la marche sur le ≤J-niveau (position générale, Delaunay, séquentielle). Le WSPD disparaît ; la directive « s ≥ 8 » devient sans objet pour le générateur.
3. **État de la preuve de correction** : lemmes D, D-loc, L, C, U, W, Z, M, K, S, O écrits (§ 3). Validation : oracle indépendant pondéré, 3,70 M enregistrements, 0 écart ; 12 épingles L13/v9 sur 12 retrouvées à l'identique (§ 12.3) ; J-KM2 vert sur 4 comparaisons.
4. **Coût mesuré, LiDAR d'abord** (sonde ingénieriée, un à quatre fils sur un hôte partagé chargé) : 3,7 à 5,8 µs CPU par boule à K5 comme à K10. Soit 000200 : 1,41 M boules à K5, 5,48 M à K10 ; brute avec sol : 2,82 M et 11,39 M. **Linéaire** sur toutes les familles adverses (exposant de travail total ≤ 1,08 sur les familles à doublement pur, ≤ 1,126 sur le LiDAR emboîté).
5. **Contrats** (§ 11.9) : **1 s à K5 atteignable en CPU seul** (générateur ≈ 0,15 à 0,25 s). **1 s à K10 non garanti en CPU seul** (générateur ≈ 0,5 à 0,8 s, plus la tour) : il faut la feuille GPU ou le filtre flottant certifié O-F1. **100 ms** : hors d'atteinte à K10 ; à K5, seulement avec générateur GPU et tour de recherche.
6. **Le poste dominant est désormais la feuille i128** (triplets et quadruplets : 86 % de la feuille à K10), pas l'arbre (15 à 30 %).
7. **Non prouvé** : aucune borne de pire cas sur le nombre de nœuds ni de feuilles forcées (la sortie elle-même peut être $\Omega(n^{2})$) ; Euler pondéré (O-W2) ; le filtre flottant O-F1 n'est qu'une option à décider sur microbanc.

---

## 2. Objet exact produit

### 2.1 Entrée

- Points $(x_{i},\mathrm{pid}_{i})$, coordonnées entières $0\le x<2^{B}$, $B\le24$ (profil produit $B=18$), `PointId` u32 arbitraires.
- **Sites pondérés** : les points de même coordonnée forment un site de poids $w\ge1$ (spécification § 2). Index de site = rang de Morton des coordonnées, donc canonique. CSR site → `PointId` triés. $W=\sum w$.
- $K_{\max}\le16$ pour le générateur. Si $W<K$, le générateur travaille à $K_{g}=W$ (§ 6.1).

### 2.2 Boule critique, support canonique, identités

Pour $B=(c,r)$, $r>0$ : $I=X\cap B^{\circ}(c,r)$, $U=X\cap\partial B(c,r)$ (positions), $p_{w}=w(I)$, $u_{w}=w(U)$.

- **Support** : $T\subseteq U$ de 2 à 4 positions affinement indépendantes, avec $c\in\mathrm{relint}\,\mathrm{conv}(T)$.
- $q_{\min}(B)$ = plus petit cardinal (en **positions**) d'un support.
- **Support canonique** $S^{*}$ : le plus petit support de cardinal $q_{\min}$ dans l'ordre lexicographique des index de sites triés.
- **Deux identités exactes** : $S^{*}$ détermine $B$ (v1) ; **U détermine B** (lemme U, § 3.7). La clé canonique est $S^{*}$ (au plus 4 index, 16 o), sans arithmétique.
- **Coquille régulière** : $U=S^{*}$ ; **étendue** : $\lvert U\rvert>q_{\min}$. Mesuré : 0,041 % des boules à K5 sur 000200, 0,024 % à K10, un tiers sur les grilles entières.

### 2.3 Admission (unique)

$$\mathrm{adm}_{K}(B)\iff p_{w}+q_{\min}\le K+1$$

avec $p_{w}$ pondéré et $q_{\min}$ en positions, **pour toutes les coquilles, pondérées ou non**. Le lemme W (§ 3.8) prouve que c'est un sur-ensemble sûr : toute boule qui a un ordre critique au plus K est admise. La tour (TOWER_v1 § 2.3) lit la fenêtre $[\max(1,p_{w}+q_{\min}-1),\min(K,W,p_{w}+u_{w})]$, y calcule les ordres **effectivement** critiques par son quotient local (ils ne forment pas un intervalle, fixtures F8a et F8b) et compte puis ignore une boule inerte. La règle v1 « $p\le K-1$ si la coquille est pondérée » est **supprimée**. Elle contredisait TOWER et gonflait le catalogue. $\mathrm{adm}_{K}$ est monotone en K, d'où J-KM2.

Les positions de poids $w\ge2$ sont des minima de rayon nul aux ordres $1..w$. Elles ne sont pas émises ; la tour les lit dans la table des poids.

### 2.4 Ce que le générateur ne fait pas

Ni tour, ni quotient de coquille, ni niveau réduit sur le chemin chaud. Ni mosaïque de Delaunay d'ordre supérieur, ni catalogue de cellules $\propto\binom{n}{k}$. Les seules structures globales sont la sortie et les listes des nœuds terminaux, de taille $O(\text{sortie})$ mesurée.

---

## 3. Théorie (à inscrire au registre)

### 3.1 Notations

$X$ positions distinctes de $\mathbb{R}^{3}$, poids $w:X\to\mathbb{Z}_{\ge1}$. Pour $c\in\mathbb{R}^{3}$ et $1\le k\le W$ : $d_{k}(c)=\min\lbrace\rho\ge0:w(X\cap\bar{B}(c,\rho))\ge k\rbrace$ et $N_{k}(c)=X\cap\bar{B}(c,d_{k}(c))$ (boule k-NN **fermée**, ex æquo compris). Une boîte est $Q=l+[0,s]^{3}$ (fermée) ; les feuilles utilisent $[l,l+s)$ pour la propriété des centres. $L\subseteq X$ est **K-certifiée pour Q** si $N_{K}(c)\subseteq L$ pour tout $c\in Q$. Pour $x,y\in X$ : $f_{xy}(c)=\lVert x-c\rVert^{2}-\lVert y-c\rVert^{2}$, affine en c.

### 3.2 Dominateurs (pondérés), forme locale, redondance de la garde

**Lemme D.** Soit $\mathrm{Dom}_{Q}(x;Y)=\lbrace y\in Y:\max_{c\in Q}(\lVert y-c\rVert^{2}-\lVert x-c\rVert^{2})<0\rbrace$. Si $w(\mathrm{Dom}_{Q}(x;Y))\ge K$, alors $x\notin N_{K}(c)$ pour tout $c\in Q$.

*Preuve.* Pour $c\in Q$, ces $y$ sont strictement plus proches que $x$ et pèsent au moins $K$, donc $d_{K}(c)<\lVert x-c\rVert$. ∎

**Lemme D-loc (forme sans multiplication).** Soient $x'=x-l$ et $A(x)=\lVert x'\rVert^{2}$ (unités de la boîte). Alors

$$y\in\mathrm{Dom}_{Q}(x)\iff A(x)-A(y)>2s\sum_{i=1}^{3}\max(0,x'_{i}-y'_{i}).$$

*Preuve.* $\lVert y-c\rVert^{2}-\lVert x-c\rVert^{2}=A(y)-A(x)+2c'\cdot(x'-y')$ avec $c'=c-l\in[0,s]^{3}$. Le maximum d'une forme linéaire sur le cube est atteint au coin choisi par les signes, et vaut $2s\sum_{i}\max(0,x'_{i}-y'_{i})$. ∎

Les boîtes sont dyadiques, donc $2s$ est une puissance de deux et le membre droit est un décalage. Un seul produit par site et par nœud ($A$) ; tout est exact en i64 si $B+T\le29$ (§ 5.2).

**Corollaire G ⊂ D.** La garde de v1 exclut x si et seulement si tout $y\in S_{0}$ domine x. Si $S_{0}\subseteq Y$ et $w(S_{0})\ge K$, D exclut aussi x. De même, la coupe par distances D′ implique D. **Conséquence** : le filtre d'un nœud est D sur Y, seul. *Mesure* : les trois variantes (G puis D comme le prototype, D′ puis D avec sortie anticipée, D seul sans branchement) donnent les **mêmes** listes terminales sur 000200 à K5 et K10 (empreintes commutatives identiques, `out/` § 15) ; et les 28 800 exécutions de l'oracle (§ 12.1) rendent le même catalogue que l'oracle quelle que soit la variante d'arbre.

### 3.3 Proposition L (listes certifiées par induction)

$L(\text{racine})=X$ est K-certifiée. Si $L(Q)$ l'est pour $Q$ et $Q'\subseteq Q$, alors $L(Q')=\lbrace x\in L(Q):w(\mathrm{Dom}_{Q'}(x;Y))<K\rbrace$ est K-certifiée pour $Q'$, **quel que soit** $Y\subseteq X$. *Preuve* : contraposée du lemme D. ∎ Le choix de Y (§ 6.2) ne touche que la force du filtre.

### 3.4 Théorème C (recensement local exact)

Soient $L(Q)$ K-certifiée et $B=(c,r)$ avec $c\in Q$. Notons $p^{*}=w(X\cap B^{\circ})$, $U^{*}=X\cap\partial B$, et $\hat{p}$, $\hat{U}$ les mêmes quantités sur $L(Q)$. (a) Si $p^{*}\le K-1$ : $\hat{p}=p^{*}$ et $\hat{U}=U^{*}$. (b) Si $p^{*}\ge K$ : $\hat{p}\ge K$.

*Preuve* (inchangée depuis v1). (a) $w(B^{\circ})<K$ donne $r\le d_{K}(c)$, donc $X\cap\bar{B}(c,r)\subseteq N_{K}(c)\subseteq L(Q)$. (b) $d_{K}(c)<r$, donc $N_{K}(c)\subseteq B^{\circ}$, et $N_{K}(c)\subseteq L(Q)$ pèse au moins K. ∎

**Corollaire A.** L'admission ne dépend que de $p^{*}\le K-1$, de $U^{*}$ et de $c$ : elle se décide exactement sur $L(Q)$, et les supports sont inclus dans $L(Q)$.

### 3.5 Corollaire U (émission unique)

Les feuilles demi-ouvertes partitionnent $[0,2^{B})^{3}\supseteq\mathrm{conv}(X)$. Une boule admise est émise dans exactement une feuille, et une seule fois grâce au mémo (§ 3.7). L'unicité globale est vérifiée par l'absence de $S^{*}$ adjacents égaux dans l'ordre canonique (invariant, code 3). *Mesure* : 0 doublon sur les 98 exécutions d'échelle.

### 3.6 Filtres de feuille et survie du support canonique

Soit $B$ admise de centre $c\in Q$ et $T$ l'un de ses supports.

- **Bissectrice = non-dominance mutuelle.** Pour $a,b\in L(Q)$, le plan bissecteur rencontre $\bar{Q}$ si et seulement si ni $a$ ne domine $b$, ni $b$ ne domine $a$. En effet $f_{ab}$ est affine : de signe constant strict sur $\bar{Q}$ si et seulement si l'un domine l'autre, sinon elle s'annule sur $\bar{Q}$ (convexe). Le test se lit donc dans la matrice de dominance de la feuille.
- **Lemme Z (forme fermée).** Pour $a,b,d$ non alignés, soient en unités de boîte $u=b'-a'$, $v=d'-a'$, $\nu=u\times v$, $Z_{0}=2(A(a)-A(b))+2s(u_{1}+u_{2}+u_{3})$ et $Z_{1}=2(A(a)-A(d))+2s(v_{1}+v_{2}+v_{3})$. La droite des points équidistants de $a,b,d$ rencontre $\bar{Q}$ si et seulement si, pour tout axe $k$ avec $(u_{k},v_{k})\ne(0,0)$ et $\lbrace i,j\rbrace$ les deux autres axes : $\lvert u_{k}Z_{1}-v_{k}Z_{0}\rvert\le2s(\lvert\nu_{i}\rvert+\lvert\nu_{j}\rvert)$.
  *Preuve.* $F=(f_{ab},f_{ad})$ vérifie $2F(l+c')=(Z_{0},Z_{1})+\sum_{k}t_{k}\,2s(u_{k},v_{k})$ avec $t_{k}\in[-1,1]$ quand $c'$ parcourt le cube : l'image est un zonogone de générateurs $g_{k}=2s(u_{k},v_{k})$. $0$ y appartient si et seulement si, pour chaque normale d'arête $n_{k}=(-v_{k},u_{k})$, $\lvert n_{k}\cdot(Z_{0},Z_{1})\rvert\le\sum_{j}\lvert n_{k}\cdot g_{j}\rvert$. Or $n_{k}\cdot g_{j}=2s(u_{k}v_{j}-v_{k}u_{j})$, qui vaut au signe près une composante de $\nu$ (nulle pour $j=k$). Les $g_{k}$ engendrent $\mathbb{R}^{2}$ car $\nu\ne0$. ∎ Borne : $\lvert u_{k}Z_{1}-v_{k}Z_{0}\rvert\le72E'^{3}$, donc exact en i64 si $E'\le2^{18}$ (§ 5.2).
- **Lemme M (union des dominés).** Soit $\mathrm{Dom}_{Q}(s)$ l'ensemble des sites qui dominent $s$ sur $Q$. Pour $s\in T$, tout $y\in\mathrm{Dom}_{Q}(s)$ vérifie $\lVert y-c\rVert<\lVert s-c\rVert=r$ : il est strictement intérieur à $B$. Donc $w(\bigcup_{s\in T}\mathrm{Dom}_{Q}(s))\le p_{w}$.
- **Lemme K (enveloppe, test suffisant).** $c\in\mathrm{relint}\,\mathrm{conv}(S^{*})\subseteq\mathrm{conv}(L(Q))$. S'il existe une direction $n$ parmi 13 ($e_{i}$, $e_{i}\pm e_{j}$, $(1,\pm1,\pm1)$) telle que les intervalles $n\cdot L(Q)$ et $n\cdot\bar{Q}$ sont disjoints, alors $\mathrm{conv}(L(Q))\cap\bar{Q}=\emptyset$ et aucune boule admise n'a son centre dans $Q$. C'est un test **suffisant** de séparation, pas un test complet (le théorème de l'axe séparateur exigerait aussi les produits vectoriels d'arêtes) ; il est plus faible que l'exactitude, donc sans risque.

**Lemme S (survie du support canonique, seuils positionnels).** Posons $\theta(T)=K+1-\lvert T\rvert$ ($\lvert T\rvert$ en positions). Pour $B$ admise de centre $c\in Q$, $S^{*}$ passe tous les filtres : bissectrices, droite, centre dans $[l,l+s)$ ; union des dominés de poids $\le p_{w}\le K+1-q_{\min}=\theta(S^{*})$ ; sortie anticipée du recensement, qui ne se déclenche que si $p_{w}>\theta(T)$. Une présentation non canonique $T$ peut être écartée sans perte, puisque $S^{*}$ est énuméré dans la même feuille. ∎ Les seuils des masques (paires $\le K-1$, triplets $\le K-2$, quadruplets $\le K-3$) sont ces $\theta$, appliqués aux poids ; ils sont justes avec multiplicités.

### 3.7 Lemme U (une boule critique est déterminée par sa coquille)

**Énoncé.** Si $B=(c,r)$ a un support, alors $c$ est l'unique point de $\mathrm{aff}(U)$ équidistant de $U$. Deux boules critiques de même coquille $U$ sont donc égales.

*Preuve.* $c\in\mathrm{conv}(S^{*})\subseteq\mathrm{aff}(U)$ et $c$ est équidistant de $U$. L'ensemble des points équidistants de $U$ est un sous-espace affine de direction $\mathrm{aff}(U)^{\perp}$ : il coupe $\mathrm{aff}(U)$ en au plus un point. ∎

**Conséquences.** (1) Le mémo de feuille des coquilles étendues est un ensemble de **masques** $U$ (bits sur la liste de la feuille), comparés par égalité : ni clé i128, ni centre rationnel, ni i256. (2) **Raccourci q4** : si un masque du mémo contient les quatre sites d'un quadruplet, ce quadruplet est soit coplanaire (pas un support), soit sur la sphère unique de cette boule (même boule). On l'écarte avant de calculer le centre. *Mesure* : 0 désaccord sur les 3,70 M enregistrements ; sur la grille $32^{3}$, 762 759 rejets par le mémo et 226 982 par le raccourci q4.

### 3.8 Lemme W (admission pondérée positionnelle)

Pour $B$ critique et $j\ge1$, soit $\Sigma^{w}_{j}=\lbrace v\in S^{2}:w(U\cap H_{v})\ge j\rbrace$ avec $H_{v}=\lbrace x:v\cdot(x-c)>0\rbrace$. Pour $A\subseteq U$, soit $R(A)=\lbrace v:A\subseteq H_{v}\rbrace$, ouvert sphériquement convexe, donc connexe s'il n'est pas vide.

**Énoncé.** Pour $1\le j\le q_{\min}-2$, $\Sigma^{w}_{j}$ est non vide et connexe. Donc l'ordre $p_{w}+j$ est π0-régulier en $B$ (modèle local de TOWER § 2.4), tout ordre critique de $B$ est au moins $p_{w}+q_{\min}-1$, et $\mathrm{adm}_{K}$ contient toute boule qui a un ordre critique au plus K.

*Preuve.* (i) *Gordan.* Si $S\subseteq U$ et $\lvert S\rvert<q_{\min}$, alors $c\notin\mathrm{conv}(S)$. Sinon $c$ serait dans l'intérieur relatif d'un simplexe à sommets affinement indépendants pris dans $S$ (Carathéodory dans la face minimale qui contient $c$), c'est-à-dire un support de cardinal au plus $\lvert S\rvert$, avec au moins 2 points puisque $r>0$. Donc $R(S)\ne\emptyset$.
(ii) *Positions.* $\Sigma^{\mathrm{pos}}_{j}=\bigcup_{\lvert A\rvert=j}R(A)$. Pour $j\le q_{\min}-2$, chaque $R(A)$ est non vide. Si $\lvert A\cap A'\rvert=j-1$, alors $R(A\cup A')\ne\emptyset$ (cardinal $j+1<q_{\min}$) et $R(A\cup A')\subseteq R(A)\cap R(A')$. Le graphe de Johnson est connexe, donc $\Sigma^{\mathrm{pos}}_{j}$ est connexe.
(iii) *Poids.* $\Sigma^{w}_{j}=\bigcup_{w(A)\ge j}R(A)\supseteq\Sigma^{\mathrm{pos}}_{j}$. Si $\lvert A\rvert\ge j$, $R(A)\subseteq\Sigma^{\mathrm{pos}}_{j}$. Si $\lvert A\rvert<j$, on complète $A$ en $A\cup C$ de cardinal $j$ ($\lvert U\rvert\ge q_{\min}>j$) : $R(A\cup C)$ est non vide et contenu dans $R(A)\cap\Sigma^{\mathrm{pos}}_{j}$. Chaque morceau connexe rencontre donc le connexe $\Sigma^{\mathrm{pos}}_{j}$. ∎

(Esquisse du critique, revérifiée et rédigée ; la tour pondérée de TOWER_v1, validée contre $\Gamma_{K}$ sur copies sur 3 100 cas, utilise la même admission.)

**Les ordres critiques ne forment pas un intervalle.** Ordres relatifs $j=k-p_{w}$ où $\Sigma^{w}_{j}$ est vide ou non connexe :
- F8a, paire antipodale de poids (3,1) : $\Sigma_{1}$ = sphère moins un grand cercle (deux hémisphères, non connexe) ; $\Sigma_{2}=\Sigma_{3}$ = hémisphère côté du poids 3 (connexe) ; $\Sigma_{4}=\emptyset$. Ordres critiques **{1, 4}**.
- F8b, triangle aigu de poids (3,1,1), $q_{\min}=3$ : $\Sigma_{1}$ = sphère moins les deux pôles (connexe) ; $\Sigma_{2}$ = cercle des directions moins les deux arcs où un seul point de poids 1 est positif (non connexe) ; $\Sigma_{3}$ = hémisphère de $a$ (connexe) ; $\Sigma_{4}$ = deux arcs séparés par l'arc « $a$ seul » (non connexe) ; $\Sigma_{5}=\emptyset$. Ordres critiques **{2, 4, 5}**.

L'obligation v1 « O-W1 » (fenêtre intervalle) est **fausse** et retirée. Le générateur n'a besoin que de la borne basse, prouvée ; le calcul exact des ordres est celui de la tour.

### 3.9 Lemme O (oracle K-NN pour la tour)

Soit $Q$ la feuille terminale (énumérée ou ignorée) qui contient $c$. (i) Pour tout $k\le K$, $N_{k}(c)\subseteq N_{K}(c)\subseteq L(Q)$. (ii) Si $w(F\cap B^{\circ}(c,r))<K$, il existe $x\in X\setminus F$ avec $\lVert x-c\rVert<r$ si et seulement s'il en existe un dans $L(Q)$, et le plus proche de ces intrus appartient à $L(Q)$. (iii) Une boule du catalogue est rangée dans la feuille de son centre. *Preuves* : v1 § 2.7, inchangées.

### 3.10 Terminaison, feuilles forcées, refus

- La profondeur est au plus $B+T$ (24 en u18 avec $T=6$). Il n'y a **aucune règle de stagnation**.
- **Lemme F (listes larges inhérentes).** Si $\bar{Q}$ contient un point $c$, alors $\lvert L(Q)\rvert\ge\lvert N_{K}(c)\rvert$. Une configuration où $N_{K}(c)$ est grand (sphère cosphérique d'intérieur inférieur à K) force donc des listes larges à **toute** échelle : ce n'est pas une faiblesse du filtre. Fixture F11 : les 144 points entiers de $x^{2}+y^{2}+z^{2}=89$, refusés à K2 et K5 (8 feuilles de 144 sites, mesuré). La tour refuserait de toute façon une telle coquille ($u\le16$).
- Aucune borne n'est prouvée sur le nombre de nœuds ni de feuilles forcées. Mesures : 0 feuille forcée hors des grilles entières ; sur les grilles, 0 à T = 6 et K5, 53 176 feuilles de $m\le32$ à K10 (sans refus).

### 3.11 Ce qui reste ouvert

- **Pire cas** : aucune borne (sortie $\Omega(n^{2})$ possible, v7). La linéarité reste un `experimental_target` jugé par les portes de coût.
- **O-W2 (Euler pondéré)** : l'identité d'Euler de la v9 suppose $w\equiv1$. J4 est désactivé et déclaré sur une entrée pondérée ; J-KM2 (restriction) et le juge des boîtes couvrent le pondéré.
- **O-F1 (filtre flottant certifié)**, optionnel : borne d'erreur à priori sur les centres q3 et q4 et sur les orientations, avec repli exact. Il n'est adopté que si un microbanc montre au moins ×1,5 sur la feuille K10 LiDAR (§ 11.9). C'est une décision de conception, jamais un levier d'exécution.

### 3.12 Entrées proposées pour `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (section v10)

| id | énoncé | statut proposé | fixtures, mutants, juges |
|---|---|---|---|
| V10-GEN-1 | lemmes D, D-loc, corollaire G ⊂ D, proposition L | `proved_here` | M1, M2 ; égalité des listes terminales (3 variantes) |
| V10-GEN-2 | théorème C, corollaire A | `proved_here` | oracle indépendant ; F4, F12 |
| V10-GEN-3 | feuilles demi-ouvertes, identité par $S^{*}$, émission unique | `proved_here` | M3, M4 ; F9 ; J6 |
| V10-GEN-4 | bissectrice = non-dominance, lemmes Z (forme fermée), M, K (suffisant), S (seuils positionnels) | `proved_here` | M6, M7, M8, M10 |
| V10-GEN-5 | lemme O | `proved_here` | J3 |
| V10-GEN-6 | lemme W : admission positionnelle sûre avec poids | `proved_here` | F8a, F8b, F8c ; M14 ; oracle pondéré ; J-KM2 pondéré |
| V10-GEN-7 | lemme U : la coquille détermine la boule critique ; mémo par masque | `proved_here` | M4, M15 |
| V10-GEN-8 | Euler pondéré (O-W2) | `proof_obligation` | à écrire |
| V10-GEN-9 | coût linéaire en la sortie sur familles adverses et LiDAR | `experimental_target` | portes de coût § 12.6 |
| V10-GEN-10 | la fenêtre pondérée de v1 (O-W1) est un intervalle | `false_in_general` | F8a, F8b |

**Réouverture formelle.** La piste v3 « cellules de centres » (`morsehgp3D_v3/audits/PISTES_FERMEES.md`) a été fermée pour des raisons d'ingénierie, pas par un contre-exemple. Sa réouverture s'appuie sur un théorème de complétude (V10-GEN-1 à 4, 7), des fixtures et une porte de coût distincte (§ 12.6). Le lemme affine $\Delta_{a,z}(c)$ de l'auditeur A est le même ressort ; l'indexation par centre lui fournit l'invariant de couverture qui lui manquait.

---

## 4. Voies comparées (résumé, chiffres corrigés)

| critère | v9 : front WSPD | marche ≤J-niveau | boîtes de centres (retenue) |
|---|---|---|---|
| exactitude | T2 n ≤ 14, digests | position générale seulement | lemmes § 3 ; 3,70 M enregistrements = oracle indépendant ; épingles v9 |
| sensibilité à la sortie | non (0,44 n² paires sur amas, cubique sur coquilles) | visite 5 à 7 fois plus d'objets que de boules | oui : exposant de travail total ≤ 1,08 (familles pures), ≤ 1,126 (LiDAR emboîté) |
| LiDAR K5, CPU par boule | 88 µs (moteur W48) | — | 3,7 à 5,8 µs (sonde ingénieriée) |
| LiDAR K10, CPU par boule | 63 à 67 µs | — | 5,1 à 5,8 µs (000200 et brute ; 6,3 au plus sur les emboîtées) |
| multiplicités | refus | non | natives (lemme W, oracle pondéré) |
| code | 17,5 kLOC + 6,3 kLOC GPU | 0,3 kLOC | ≈ 2 à 2,5 kLOC estimés (sonde : 0,8 kLOC) |

---

## 5. Domaine et arithmétique

### 5.1 Deux repères

1. **Repère de boîte** (unités $2^{-T}$ mm, entiers, origine au coin bas $l$ de la boîte) : dominance (arbre et feuille), bissectrice, lemme Z, k-DOP, choix de $Y$. Un site y devient $x'=2^{T}x-l$, avec $\lvert x'_{i}\rvert<2^{B+T}$. Étendue de feuille $E'=\max(s,\max_{x}\lVert x'\rVert_{\infty})$.
2. **Repère de sites** (unités d'entrée, origine en un site $a$ du support) : centres, recensement, aigu strict, alignement, orientations, niveaux. N'y interviennent **que des différences de sites**, bornées par $E=\max_{x,y\in L}\lVert x-y\rVert_{\infty}<2^{B}$.
3. **Pont unique** : « centre dans $[l,l+s)$ » s'écrit $l'D\le2^{T}N_{\mathrm{rel}}<(l'+s)D$ par axe, avec $l'=l-2^{T}a$ et $c=a+N_{\mathrm{rel}}/D$.

**Pourquoi.** Mettre les sites à l'échelle $2^{T}$ multiplie les bornes du recensement q3 par $2^{6T}$ ; la sonde l'a montré (§ 0.3, point 5 ; mutant M13).

### 5.2 Prédicats et bornes (profil u18 : B = 18, T = 6, $E<2^{18}$, $E'\le2^{24}$)

| prédicat | repère | forme | borne | type |
|---|---|---|---|---|
| dominance (arbre, feuille), bissectrice | boîte | $A(x)-A(y)>(\sum\max(0,x'-y'))\ll(\log_{2}s+1)$ | $A<3\cdot2^{2(B+T)}=2^{49,6}$ ; membre droit $<3\cdot2^{2(B+T)+2}=2^{51,6}$ | i64 si $B+T\le29$ |
| sélection de Y | boîte | $\lVert2x'-s\mathbf{1}\rVert^{2}$ | $<2^{51,6}$ | i64 |
| k-DOP (13 directions) | boîte | $n\cdot x'$ contre $n\cdot\bar{Q}$ | $3\cdot2^{B+T}$ | i64 |
| lemme Z | boîte | $\lvert u_{k}Z_{1}-v_{k}Z_{0}\rvert\le2s(\lvert\nu_{i}\rvert+\lvert\nu_{j}\rvert)$ | $72E'^{3}$ | i64 si $E'\le2^{18}$ (97,5 % des feuilles LiDAR), sinon i128 ($<2^{78,2}$) |
| aigu strict, alignement | sites | produits scalaire et vectoriel | $3E^{2}<2^{37,6}$ | i64 |
| centre q2 | sites | $N_{\mathrm{rel}}=u$, $D=2$ | $E$ | i64 |
| centre q3 (Gram) | sites | $N_{\mathrm{rel}}=W$, $D=2\lVert u\times v\rVert^{2}$ | $\lvert W_{i}\rvert\le36E^{5}<2^{95,2}$ ; $D\le24E^{4}<2^{76,6}$ | i128 |
| centre q4 (Cramer) | sites | $N_{\mathrm{rel}}=\sum_{r}\lVert d_{r}\rVert^{2}\mathrm{cof}_{r}$, $D=2\det$ | $18E^{4}<2^{76,2}$ ; $12E^{3}<2^{57,6}$ | i128 |
| centre ∈ boîte | pont | $l'D$ contre $2^{T}N_{\mathrm{rel}}$ | q3 : $2^{T}\cdot36E^{5}<2^{101,2}$ et $2^{B+T}\cdot24E^{4}<2^{100,6}$ | i128 |
| intérieur strict du tétraèdre | sites | 4 orientations multipliées par $D$ | $180E^{6}<2^{115,5}$ | i128 |
| recensement q3 | sites | $D\lVert z-a\rVert^{2}-2N_{\mathrm{rel}}\cdot(z-a)$ | $288E^{6}<2^{116,2}$ | i128 |
| recensement q4 | sites | idem | $144E^{5}<2^{97,2}$ | i128 |
| niveau $r^{2}$ | sites | q3 : $\lVert u\rVert^{2}\lVert v\rVert^{2}\lVert u-v\rVert^{2}/(4G)$ ; q4 : $\lVert N\rVert^{2}/(4\det^{2})$ | num $\le972E^{8}<2^{153,9}$ ; dén $\le144E^{6}<2^{115,2}$ | U192 / U128 |
| comparaison de niveaux | — | produits croisés | $<2^{269,1}$ | U320 (U288 suffirait) |

- Chaque borne est un `static_assert` paramétré par $(B,T)$ dans l'en-tête des prédicats. Le type (i64 ou i128) du lemme Z se choisit **par feuille** sur $E'$, compté (`leaf_wide_line`).
- **Limite de l'i128** : le recensement q3 exige $E<2^{19,8}$. Le chemin chaud est donc i128-exact jusqu'à $B=19$. Pour $20\le B\le24$, une feuille d'étendue $E\ge2^{19}$ prend la voie large (§ 5.3), comptée.
- **Palier i64 optionnel** (non retenu en v10.0) : q3 et q4 et leur recensement tiennent en i64 si $E\le2^{9}$ unités. Mesuré sur 000200 K5 : 42,6 % des feuilles ont une étendue de sites au plus 512 mm, 35,8 % au plus 2 048 mm, 21,5 % au plus 16 384 mm, 0,1 % au-delà. Gain partiel, à réévaluer après O-F1.

### 5.3 Voie large (i256)

Uniquement pour $B\ge20$ et les feuilles d'étendue $E\ge2^{19}$. La voie des coquilles étendues n'en a **plus** besoin (lemme U). Un seul type entier signé à 4 mots, jugé contre `boost::multiprecision::cpp_int` (Boost sous `BOOST_ROOT`). Aucun cas en u18.

### 5.4 Flottant

Aucune décision n'utilise de flottant. Seul usage : la clé de tri des niveaux (§ 6.4), à bande d'erreur prouvée et réparée exactement. Un filtre flottant certifié n'entre que par O-F1.

---

## 6. Algorithme

### 6.1 Préparation

1. Contrôle du domaine (`invalid_input`) ; clés de Morton 3×24 bits, tri par base, regroupement des égaux en sites pondérés ; CSR site → `PointId`.
2. Si $W\le K$ : $K_{g}=W$, donc $Y=X$ et aucune exclusion. Le reste est inchangé.
3. Racine **fixe** $[0,2^{B+T})^{3}$ en unités de boîte. Elle est canonique et rend `locate` trivial. Surcoût mesuré sur `clusters_8000` : 187 352 nœuds contre 187 280 avec racine adaptative, soit +0,04 %.

### 6.2 Nœud de l'arbre

```text
noeud(Q = l + [0, s)^3 en unites 2^-T, liste parente Lp croissante) :
  une passe sur x de Lp : x' = 2^T x - l ; A(x) = |x'|^2 ; delta(x) = |2x' - s.1|^2      (i64)
      tampon d'insertion des min(|Lp|, 3K) plus petits (delta, index)                    (depart par index)
  Y = plus court prefixe du tampon de poids >= 3K
  pour y dans Y, pour x dans Lp (vectorise, sans branchement) :
      cnt(x) += w(y) * [ A(x) - A(y) > (sum_i max(0, x'_i - y'_i)) << (log2 s + 1) ]
  L(Q) = { x de Lp : cnt(x) < K }, dans l'ordre de Lp ; boite englobante de L(Q)
  si kDOP(L(Q)) separe Q-ferme           : terminal ignore (liste conservee pour l'oracle)
  sinon si |L(Q)| > M(K) et s > 1        : 8 enfants demi-ouverts, recursion
  sinon si |L(Q)| > M_hard = 128         : refus transactionnel resource_exhausted(wide_leaf)
  sinon                                  : feuille enumeree (forcee si |L(Q)| > M(K))
```

- Pas de garde, pas de D′ (§ 3.2) : le noyau sans branchement (1,5 cycle par test) bat la version à sortie anticipée précédée de D′, qui fait 2,3 fois moins de tests mais branche (mesuré : 2,07 s contre 4,50 s à K5, même charge).
- La sélection par tampon d'insertion remplace `nth_element` plus tri : ×1,27 sur l'arbre. Avant elle, la sélection faisait 36 % du temps de l'arbre (rdtsc).
- Aucune allocation : arène par tâche, une liste par profondeur (au plus $B+T+1$), réutilisée.
- Enveloppe : boîte englobante (3 directions) par défaut, comme le prototype (362 105 nœuds ignorés sur 1 225 736 à K5 sur 000200). Les 13 directions sont mesurées en G1 ; si le gain est neutre, on garde 3 directions (décision écrite, pas un levier).

### 6.3 Feuille

```text
feuille(Q, L = (x_0 .. x_{m-1}) croissante), masques u64 si m <= 64, u128 si m <= 128 :
  repere de boite : x'_i, A_i, E' ; omega(S) = popcount(S) si tous les poids valent 1, somme des poids sinon
  Dom[i] = { j : A_i - A_j > 2s sum max(0, x'_i - x'_j) }                        (m^2 tests i64, noyau 6.2)
  memo = {}                                                     (masques U des coquilles etendues emises)
  paires i < j :
      si j dans Dom[i] ou i dans Dom[j] : suivant                                 (bissectrice hors de Q-ferme)
      d = omega(Dom[i] | Dom[j]) ; si d > K-1 : suivant
      si d <= K-2 : P2[i] += j ; P2[j] += i
      si 0 <= x'_i + x'_j < 2s sur chaque axe : juger({i, j}, milieu)
  triplets i < j < k, k dans P2[i] & P2[j] :
      d = omega(Dom[i] | Dom[j] | Dom[k]) ; si d > K-2 : suivant
      nu = u x v (repere de sites) ; si nu = 0 : suivant                          (alignes)
      si non Z(i, j, k) : suivant                             (lemme Z : i64 si E' <= 2^18, sinon i128)
      si d <= K-3 : H[i,j] += k ; H[i,k] += j ; H[j,k] += i
      si aigu strict : centre q3 (i128) ; s'il est dans [l, l+s) : juger({i, j, k}, centre)
  quadruplets i < j < k < l, k dans H[i,j], l dans H[i,j] & H[i,k] & H[j,k] :
      si omega(Dom[i] | Dom[j] | Dom[k] | Dom[l]) > K-3 : suivant
      si un masque du memo contient {i, j, k, l} : suivant                    (lemme U : meme boule)
      det = 0 : suivant                                                       (coplanaires)
      centre q4 (i128) ; s'il n'est pas dans [l, l+s) ou pas strictement interieur : suivant
      juger({i, j, k, l}, centre)
  juger(T, c) :
      theta = K + 1 - |T| ; recenser sur L : p_w (sortie des que p_w > theta), masques I et U
      si |U| = |T| : emettre(S* = T trie, I, U)                                   (coquille reguliere)
      sinon si U dans memo : retour
      sinon : memo += U ; (q_min, S*) = premier support de U, sous-ensembles de taille 2, 3 puis 4
              en ordre lexicographique ; emettre(S*, I, U)          (p_w + q_min <= K+1 puisque q_min <= |T|)
```

Le raccourci q4 du mémo précède le calcul du centre ; la sonde le fait après (même catalogue, compteur `memo_pre4`).

**Mesures de la feuille** (000200, compteurs par boule, K5 puis K10) : 18,8 et 15,2 paires ; 22,4 et 27,5 triplets examinés ; 16,3 et 19,8 vivants ; 7,1 et 8,3 centres q3 ; 6,3 et 13,2 quadruplets ; 2,07 et 1,78 recensements pour 22,1 et 32,1 sites recensés. Profil rdtsc à K10 : 47 % dans les quadruplets, 39 % dans les triplets, 8 % dans les paires, 6 % dans la dominance.

### 6.4 Niveaux, rangs, ordre canonique

1. Chaque boule reçoit une clé `double` de $r^{2}$. Conversion **fidèle** de U192 (ou U128) vers `double` : normaliser par le nombre de zéros de tête, garder les 64 bits hauts, forcer le bit de poids faible à 1 si un bit inférieur est non nul (bit collant), convertir u64 → `double` (un arrondi au plus proche). Erreur relative au plus $2^{-53}(1+2^{-10})$ par terme, puis un arrondi de division : au total au plus $3{,}01\cdot2^{-53}<2^{-51}$.
2. Tri parallèle par (clé, $S^{*}$), puis balayage : deux voisins dont les clés diffèrent de moins de $2^{-49}$ en relatif forment une **bande**. Chaque bande est retriée par le comparateur exact (U320), puis par $S^{*}$. *Correction* : deux niveaux exacts mal ordonnés par leurs clés ont des clés distantes d'au plus $2^{-50}(1+\epsilon)$ en relatif, et tout ce qui les sépare dans l'ordre flottant est dans la même bande.
3. Rang u32 incrémenté quand le niveau exact change ; les égalités partagent un rang.
4. Ordre canonique **(rang, $S^{*}$)**, indépendant de l'arbre, des fils et de l'ordre d'entrée ; unicité de $S^{*}$ vérifiée au balayage (code 3).
5. `level_rep[rang]` = plus petit $S^{*}$ du rang. Aucun pgcd sur le chemin chaud.

### 6.5 Digest

`catalogue_digest` = SHA-256 de la suite canonique des enregistrements (coordonnées des sites de $S^{*}$, $q_{\min}$, drapeaux, $\lvert I\rvert$, $\lvert U\rvert$, coordonnées et poids de I et de U en ordre de Morton), **sans niveau ni rang**. $S^{*}$ détermine la boule et son niveau, et l'ordre porte l'ordre des niveaux : deux suites égales décrivent le même catalogue, niveaux compris. Les rangs sont exclus parce que J-KM2 compare $\mathrm{cat}(K)$ à la restriction de $\mathrm{cat}(K+2)$, dont les rangs diffèrent ; la restriction préserve l'ordre relatif. Le niveau réduit (pgcd U192, ≈ 1 µs par boule) n'est calculé que par l'exporteur v9. `id_digest` ajoute les listes de `PointId`. *Écart signalé à ARCH v2*, dont J-KM2 hache encore le niveau réduit : le digest sans niveau suffit et coûte ≈ 5,5 CPU·s de moins à K10.

---

## 7. Paramètres : choix et raisons

| paramètre | valeur | raison mesurée |
|---|---|---|
| $M(K)$ | **16** si $K\le6$ ; **24** si $7\le K\le10$ ; **32** si $11\le K\le16$ | K2 sur 000200 : M8 1,99 s, M12 1,58, M16 1,44 ; K3 : 4,85, 2,82, 2,84 ; K5 : M16 5,97 contre M24 6,54 ; K10 : M24 31,1 contre M32 30,0 ; K16 (s00 8k) : M24 34,3, M32 20,9, M40 24,3. La table v1 (12 pour $K\le3$) est abandonnée : M16 est au moins aussi bon. Recalibrage unique sur G4 (P-CAL), jamais un levier d'exécution |
| $Y$ | $\min(\lvert L_{p}\rvert,3K)$ sites les plus proches du centre, puis préfixe de poids $3K$ | L13 : réserve 3K contre 1K, nœuds ÷4 à K10 |
| garde $S_{0}$, coupe D′ | **supprimées** | corollaire G ⊂ D ; listes identiques ; D seul plus rapide |
| $T$ | 6 | grille $20^{3}$ au pas 1 : T = 0 donne 7 726 feuilles forcées ($m\le46$) et 24,9 µs par boule ; T = 6 donne 0 feuille forcée et 14,7 µs. Arbres identiques ailleurs. i64 conservé ($B+T\le29$) |
| stagnation | **aucune** | B1 |
| $M_{\mathrm{hard}}$ | 128 | F11 ; cohérent avec le refus de la tour au-delà de $u=16$ ; plus grande liste légitime mesurée : 46 (grille, T = 0) |
| racine | fixe $[0,2^{B})^{3}$ | canonique ; +0,04 % de nœuds |
| masques | u64 si $m\le64$, u128 si $m\le128$ | aucune feuille u128 dans les 98 exécutions |

---

## 8. Sortie : format (aligné sur ARCH v2 § 5.2)

```cpp
namespace mhgp10::catalogue {
struct SiteTable {                       // positions uniques, ordre de Morton = index canonique
  Buffer<std::array<u32, 3>> pos;
  Buffer<u32> weight;                    // >= 1
  Buffer<u32> pid_off, pid;              // CSR site -> PointId tries
};
struct Catalogue {                       // SoA, ordre canonique (rang, S*)
  Buffer<u32> rank;                      // rang du niveau exact ........................ 4 o
  Buffer<u32> ids_off;                   // CSR dans ids : I trie puis U trie ........... 4 o (garde de depassement)
  Buffer<u8>  n_interior;                // |I| en positions (<= kcat - 1) .............. 1 o
  Buffer<u8>  n_shell;                   // |U| en positions (<= M_hard = 128) .......... 1 o
  Buffer<u8>  qflags;                    // q_min (2 bits) | etendue | ponderee ......... 1 o
  Buffer<u32> ids;                       // index de sites ...................... 4 o par identifiant
  Buffer<u32> level_rep;                 // rang -> boule representante ............... <= 4 o par boule
  ExtSide ext;                           // coquilles etendues : S* en positions dans U (0,02 a 0,06 % LiDAR)
  Ledger ledger;  Status status;  u8 kcat;
};
}
```

- Octets par boule : 11 fixes + 4 par identifiant + au plus 4. Identifiants mesurés par boule sur 000200 : **4,63 à K5**, **8,10 à K10** (brute avec sol : 4,64 et 8,15). Donc 29,5 à 33,5 o, soit **42 à 47 Mo** à K5, et 43,4 à 47,4 o, soit **238 à 260 Mo** à K10 ; ≈ 500 à 540 Mo sur la brute à K10.
- Centre et niveau exacts : recalculés depuis $S^{*}$ à la demande (50 à 100 ns).
- Transaction : count → fill → validate → publish ; aucun préfixe publié en cas de refus.

---

## 9. Ce que le générateur fournit à la tour

1. `SiteTable` et `Catalogue` triés par rang, qui est l'ordre des lots de la tour.
2. **`LeafOracle`** : nœuds terminaux (feuilles énumérées **et** ignorées), triés par code de Morton fin de leur coin, avec leur liste $L(Q)$ en CSR et la CSR feuille → boules centrées. Estimation v1 : $\Sigma m$ ≈ 12 M identifiants (≈ 49 Mo) à K5 sur 000200, ≈ 26 M (≈ 106 Mo) à K10.
3. API exacte (lemme O) : `locate(c)` pour un centre rationnel ; `knn_closed(c, k)` pour $k\le K$ ; `nearest_strict_intruder(c, r², F)` ; `lookup(c, r²)`. Sémantique inchangée depuis v1 § 8.
4. Le générateur ne fournit ni ancres, ni facettes, ni MEB de facettes.

---

## 10. Parallélisme

### 10.1 CPU (48 fils sur G4)

- **Phase en largeur** : développer la frontière jusqu'à au moins $64W$ nœuds **ou** jusqu'à ce que tous les nœuds soient terminaux ; les filtres des nœuds de tête sont parallèles sur $x$. *Constat* : le découpage fixe du prototype (64 boîtes du niveau 2) dégénère avec la racine fixe. Toute la grille $20^{3}$ tombe dans une boîte, et l'exécution y est monofil (mesuré : mur = CPU). La phase en largeur est obligatoire.
- **Tâches** : une par nœud de frontière, en profondeur, ordonnancement dynamique par liste décroissante ; arène par tâche.
- **Déterminisme** : sortie d'une tâche = fonction de sa boîte ; concaténation en ordre de Morton ; ordre final (rang, $S^{*}$) total. Sorties bit-identiques pour W1..W48 (P-THREADS). Rapport d'échec unique par réduction sur (phase, code de Morton).
- **Tri des rangs** : échantillonnage parallèle puis réparation des bandes ; ≈ 20 à 40 ms à 5,5 M boules (estimation).

### 10.2 GPU (RTX PRO 6000, sm_120), ultérieur et subordonné

- **Arbre** : le noyau de dominance (§ 6.2) est sans branchement, en i64, et utilise des décalages. Il se transpose tel quel : un warp par nœud si la liste parente compte au plus 1 024 sites, sinon un bloc. BFS synchrone par niveau, compaction par balayage préfixe. Les niveaux de tête restent sur CPU.
- **Feuille** : un warp par feuille, une voie par site ($m\le32$). Masques u32 ; H en mémoire partagée (32 × 32 u32 = 4 Ko par warp). Émission par atomique agrégée, puis tri canonique sur l'appareil (clé `double` + $S^{*}$, réparation des bandes). Les feuilles de plus de 32 sites reviennent au CPU, comptées.
- **Arithmétique** : `__int128` natif en code appareil depuis CUDA 11.5 ; la branche d'émulation de v1 est retirée. Mais sur cette classe de GPU, les multiplications 64 bits sont émulées par plusieurs IMAD 32 bits, et un produit i128 en coûte une dizaine. La feuille (i128 lourde) sera donc proportionnellement plus chère sur GPU que l'arbre (i64 sans multiplication). Pas de FP64 sur le chemin de décision.
- **Juge du port** : égalité des multiensembles d'enregistrements ($S^{*}$, I, U, rang) contre le CPU.
- **Projection** (non mesurée) : 10 à 40 ms de noyaux à K5, 30 à 100 ms à K10, avec les facteurs IMAD ci-dessus. La v9 a connu ×10 entre projection et mur : ces chiffres ne valent qu'après reçu G4. Le contexte CUDA (121 à 166 ms par processus) impose un processus résident pour tout contrat de 100 ms.

---

## 11. Mesures (reçus `design/gen_v2_probe/out/`)

### 11.1 Conditions

Hôte partagé : AMD EPYC 7763 virtualisé (Zen 3), 4 cœurs / 8 fils, AVX2, charge 3 à 16 selon l'heure, `g++ 13.3 -O3 -march=native`. Les temps sont **indicatifs** et pris sous SMT chargé. Font foi : les compteurs, les catalogues (empreintes `rec_hash`), les refus, et les rapports de temps pris dans la même fenêtre de charge. Sonde : `genprobe2.cpp` (sha256 `6aca988b…`), dérivée de `gen_probe/genprobe.cpp`. Toutes les exécutions d'échelle utilisent la racine fixe B = 18, T = 6, l'arbre `--fast=2` et la feuille `--fastleaf`.

### 11.2 LiDAR (priorité)

| entrée | sites | K | boules (par site) | $q_{\min}$ = 2 / 3 / 4 | étendues | nœuds par site | m moyen / max | forcées | µs CPU par boule |
|---|---|---|---|---|---|---|---|---|---|
| 08/000200 sans sol (sha `a4bbc86d…`) | 45 845 | 5 | 1 407 885 (30,7) | 518 233 / 746 547 / 143 105 | 572 (0,041 %) | 26,7 | 11,4 / 16 | 0 | 3,7 à 5,5 |
| idem | 45 845 | 10 | 5 483 320 (119,6) | 977 534 / 3 019 193 / 1 486 593 | 1 301 (0,024 %) | 35,8 | 18,4 / 24 | 0 | 5,3 à 5,8 |
| 00/000000 brute avec sol (sha `233cc4ea…`) | 123 389 | 5 | 2 822 052 (22,9) | 1 265 445 / 1 370 352 / 186 255 | 1 243 | 24,3 | 11,5 / 16 | 0 | 5,8 |
| idem | 123 389 | 10 | **11 387 391** (92,3) | 2 598 083 / 6 665 118 / 2 124 190 | 2 399 | 31,5 | 18,5 / 24 | 0 | 5,1 |

- Les comptes de 000200 (boules, comptes par $q_{\min}$, coquilles étendues) sont égaux aux épingles L13, elles-mêmes égales au catalogue v9 (`out/l13_pins.txt`). La brute avec sol n'est comparée à rien de v9 : ses comptes deviennent des épingles internes.
- **Brute contre sans sol** : visites d'arbre par site ×0,97 (K5) et ×0,92 (K10) ; travail de feuille par boule ×0,97 et ×0,92. Le sol n'alourdit pas le coût unitaire ; il ajoute des sites.
- **LiDAR emboîté** (sous-nuages 8k ⊂ 16k ⊂ 32k des scènes 00, 01, 02, fichiers v9 `*_nested_*`), K5 et K10 : aucun refus, aucune feuille forcée, $m\le M$. Boules par site à K5 : s00 42,8 → 39,9 → 34,8 ; s01 32,2 → 32,0 → 31,2 ; s02 34,9 → 33,4 → 31,2. Exposants au § 12.6.

### 11.3 Familles 8k / 16k / 32k (K5 sauf mention)

| famille | boules 8k / 16k / 32k | exposant arbre | exposant feuille | exposant total |
|---|---|---|---|---|
| uniforme (v9) | 594 386 / 1 234 454 / 2 531 823 | 1,074 | 1,008 | 1,012 |
| huit amas (v9) | 510 758 / 1 099 180 / 2 305 835 | 1,070 | 1,002 | 0,999 |
| terrain (v9) | 160 174 / 330 597 / 703 057 | 1,126 | 1,086 | 1,077 |
| coquilles (banc, graine 11) | 180 079 / 354 389 / 704 724 | 1,080 | 0,960 | 1,012 |
| filaments (banc, graine 11) | 489 711 / 1 083 686 / 2 340 954 | 1,016 | 0,934 | 0,929 |
| banc synthétique : anisotrope, pont, hétéroscédastique, hiérarchique extrême, sphérique (bruit 0 et 0,1), déséquilibré (graine 0, 8 groupes) | 459 020 à 2 613 873 selon la famille | 0,974 à 1,078 | 0,928 à 0,970 | 0,915 à 0,977 |
| huit amas K10 | 2 491 373 / 5 495 767 / — | — | — | — |

Les 45 exécutions des groupes « familles » et « banc synthétique » : 0 refus, 0 feuille forcée, $m\le M$, 0 doublon de $S^{*}$. La v9 met 57,5 / 179,9 / 1 438,5 s sur les coquilles pour q3/q4 seul ; ici 1,2 / 2,3 / 4,6 s CPU pour tout le catalogue.

### 11.4 Grilles entières (dégénérescence massive)

| entrée | T | boules | étendues | nœuds par site | feuilles forcées (m max) | recensements par boule | µs par boule |
|---|---|---|---|---|---|---|---|
| $20^{3}$ pas 1 | 0 | 234 225 | 78 577 | 1,2 | 7 726 (46) | 27,0 | 24,9 |
| $20^{3}$ pas 1 | 6 | 234 225 | 78 577 | 55,6 | 0 | 3,2 | 14,7 |
| $25^{3}$ pas 1 | 0 / 6 | 471 665 | 157 097 | 1,4 / 59,0 | 15 276 (46) / 0 | 27,7 / 3,2 | 25,6 / 14,6 |
| $32^{3}$ pas 1 | 0 / 6 | 1 015 677 | 336 157 | 1,2 / 61,8 | 32 314 (46) / 0 | 28,4 / 3,1 | 25,8 / 14,5 |
| $20^{3}$ pas 1, K10 | 6 | 1 227 302 | 348 054 | 276 | 53 176 (32) | 2,9 | 23,3 |

Coût linéaire, 3 à 5 fois celui du LiDAR par boule. 72 quadruplets par boule (présentations multiples des coquilles étendues), dont une part est coupée par le raccourci q4 du lemme U.

### 11.5 Microbancs de l'arbre et de la feuille (réponse à M4)

Même binaire, même entrée (000200), un fil, exécutions alternées dans la même fenêtre de charge (8,5 à 10) ; égalité des sorties vérifiée à chaque fois. Reçu : `microbench.sh` → `out/microbench.txt` (les mesures interactives de la session, ×4,35 et ×5,7 sur l'arbre, concordent).

| mesure | référence (sémantique du prototype : G puis D, i128, allocations) | ingénieriée | rapport | égalité |
|---|---|---|---|---|
| arbre seul K5 | 9,49 à 9,57 s | 2,17 à 2,21 s | **×4,3 à ×4,4** | empreinte des listes `e3baefa5…` |
| arbre seul K10 | 25,9 à 26,2 s | 4,66 à 4,80 s | **×5,4 à ×5,6** | `552daaab…` |
| feuille K5 (complet moins arbre) | 9,1 à 9,2 s | 5,4 à 5,5 s | ×1,7 | catalogue `1a584b3f…` |
| feuille K10 | 43,0 à 43,7 s | 26,8 à 26,9 s | ×1,6 | catalogue `28a3f21a…` |
| total par boule, K5 / K10 | 13,3 / 12,7 µs (tout en référence) ; 8,0 / 8,8 µs (arbre ingénierié seul) | **5,4 à 5,5 / 5,75 µs** | ×2,4 / ×2,2 | — |

- Profil rdtsc de la feuille ingénieriée (même reçu) : à K5, dominance 9 %, paires 14 %, triplets 44 %, quadruplets 32 % ; à K10, 6 %, 8 %, 39 %, 47 %.
- Arbre ingénierié, profil rdtsc (avant le tampon d'insertion) : sélection 36 %, noyau 28 %, coordonnées locales 14 %, mise en place de Y 12 %, sortie 10 %. Noyau : 1,5 cycle par test de dominance.
- **Lecture honnête** : l'arbre n'a pas gagné ×13 à ×20, mais il ne pèse plus que 28 à 30 % à K5 et 15 % à K10. La cible v1 de 1,5 à 4 µs par boule n'est pas atteinte (3,7 à 5,8 µs mesurés) ; le reste est dans la feuille i128.
- Par rapport au prototype v2 mesuré en v1 (16 à 24 µs par boule, 2 fils, charge 5 à 10), le gain total est de ×3 à ×6.

### 11.6 Régimes de K (000200 sauf mention)

| K | M | boules (par site) | nœuds par site | µs par boule |
|---|---|---|---|---|
| 2 | 8 / 12 / **16** | 301 745 (6,6) | 36,3 / 15,4 / 9,6 | 6,6 / 5,2 / **4,8** |
| 3 | 8 / **12** / 16 | 575 614 (12,6) | 85,2 / 24,4 / 13,2 | 8,4 / **4,9** / 4,9 |
| 7 | 24 | 2 675 990 (58,4) | 15,8 | 5,4 |
| 12 | 32 | 8 025 829 (175,1) | 20,1 | 5,4 |
| 16 (s00 8k) | 24 / **32** / 40 | 4 897 930 (612) | 408 / 60,9 / 21,9 | 7,0 / **4,3** / 5,0 |

Le régime K = 2–3 de la tête de clustering (L14) coûte environ 1,4 à 2,8 s CPU sur une trame : négligeable devant K5.

### 11.7 Coût de K+2 et J-KM2

- Boules : K7/K5 = 1,90 (000200), 1,98 (s00 32k), 2,18 (uniforme 8k) ; K12/K10 = 1,46 (000200). CPU : ×1,87 (K5 → K7), ×1,35 (K10 → K12). ARCH annonçait ×1,4 à K10 : confirmé. v1 annonçait ×1,6 à 2,2 : juste à K5.
- **Égalité de restriction**, enregistrement par enregistrement (`jkm2_check.sh`, `out/jkm2_restriction.txt`) : s00 8k K5/K7 (342 181 = restriction de 698 752), `terrain2000_w2` K5/K7 et K3/K5 (pondéré), `uniform8000_w4` K5/K7 (pondéré, poids moyen 4) : **égalité** à chaque fois. Le reçu contient aussi F11 à K2/K4, où les deux exécutions refusent (code 3 de la sonde) : la restriction y compare deux catalogues incomplets et n'a pas de valeur de preuve.

### 11.8 Multiplicités à l'échelle

| entrée | sites / W | K | boules | remarque |
|---|---|---|---|---|
| filaments 32k avec son doublon | 31 999 / 32 000 | 5 | 2 340 901 | 53 boules de moins que sans poids (2 340 954) : le site de poids 2 augmente $p_{w}$ |
| uniforme 8k, poids 1 à 7 | 8 000 / 32 068 | 5 / 10 | 144 149 / 367 267 | 0 et 1 coquille étendue ; aucune pondérée spéciale ; 4,9 et 5,7 µs par boule |

### 11.9 Projections G4 (CPU seul) et contrats

Hypothèses **explicites** :
- $t_{b}$ = 3,7 à 5,8 µs CPU par boule, mesuré par fil logique **sous SMT chargé**, donc comparable à un fil G4 quand les 48 sont occupés ;
- mur ≈ boules × $t_{b}$ / 48 × 1,15 (déséquilibre) + 25 à 45 ms (préparation, tri des rangs) ;
- ± 30 % pour le processeur de G4, inconnu ici.

| cas | boules | générateur v10, CPU W48 (projection) | v9 R22, étages générateurs mesurés sur G4 **avec GPU** (comparaison qui n'est pas à armes égales) |
|---|---|---|---|
| LiDAR K5 | 1,41 M | **0,15 à 0,25 s** | ≈ 0,69 s |
| LiDAR K10 | 5,48 M | **0,5 à 0,8 s** | ≈ 1,8 s |
| brute avec sol K10 | 11,39 M | 1,0 à 1,6 s | 2,2 + 1,0 s |

- **Comparaison à armes égales**, par boule et en CPU : moteur v9 W48, 88 µs par boule à K5 et 63 à 67 µs à K10 ; sonde v10, 3,7 à 5,8 µs. Soit ×15 à ×24 par boule, avant tout GPU.
- **1 s à K5 de bout en bout** : atteignable en CPU seul (générateur ≤ 0,25 s ; tour v9 0,29 s ; estimation TOWER 70 à 95 ms).
- **1 s à K10** : générateur 0,5 à 0,8 s plus tour 0,4 à 0,65 s (estimation TOWER) : **non garanti en CPU seul**. Deux leviers, à décider sur mesure : la feuille GPU (projection 30 à 100 ms), ou O-F1, adopté seulement si la feuille gagne au moins ×1,5 à K10.
- **100 ms à K5** : générateur GPU (10 à 40 ms) et tour de moins de 60 ms, ce qui relève de la recherche côté tour. **100 ms à K10** : hors d'atteinte connue (5,5 M boules à produire et à consommer).
- Ces chiffres sont des projections sur une sonde, pas sur le code produit. Ils ne se publient qu'après la tranche G4-CPU (§ 14).

---

## 12. Portes, fixtures, juges, mutants

Codes de sortie exacts (`run_expect.cmake`) : 0 conforme, 1 désaccord du juge, 2 refus avant calcul ou refus transactionnel attendu, 3 invariant violé, 4 mutant tué. Tout crash est refusé. Planchers de couverture `--min-*` partout.

### 12.1 Oracle indépendant (établit la vérité ; exclu de la règle anti-exhaustivité)

- **O1, Python/Fraction** (`indep_oracle_w.py`, dérivé de `crit_GEN/indep_oracle.py`) : énumère tous les 2-, 3- et 4-sous-ensembles, calcule les centres par Cramer en fractions, l'intérieur relatif par élimination de Gauss, recense sur tout le nuage avec poids, et prend $q_{\min}$ et $S^{*}$ par énumération lexicographique de U. Il n'a aucun code commun avec la sonde.
- **O2, C++/`cpp_int`** (tranche G1), écrit par un autre rôle : même énumération, arithmétique multiprécision ; il porte les tailles n ≤ 60 que Python ne tient pas.
- **Comparaison enregistrement par enregistrement** : (p_w, q_min, |U|, $S^{*}$, I et U avec poids), en ensembles, avec les doublons comptés.
- **Campagne de cette révision** (`oracle_campaign.py`, `out/oracle_campaign.jsonl`) : 1 200 nuages en 6 familles (grilles $\lbrace0..3\rbrace^{3}$ et $\lbrace0..4\rbrace^{3}$, sphère $R^{2}=9$ avec intrus, quasi-plan, coins de cube, génériques dans $[0,12)^{3}$), dont 600 pondérés (poids 1, 2, 3, 5). K ∈ {1, 2, 3, 4, 5, 7}. Quatre configurations de la sonde : arbre de référence, arbre D′ à sortie anticipée, arbre sans branchement ; feuille de référence et ingénieriée ; M ∈ {4, 6, 8} ; T ∈ {0, 1, 2} ; racine adaptative ou fixe. Soit 28 800 exécutions et **3 699 196 enregistrements, 0 manquant, 0 en trop, 0 doublon**. Tous les nuages ont des coquilles étendues (251 446 boules étendues), 598 des coquilles pondérées (300 120 boules), et 27 575 exécutions ont des feuilles forcées.
- **Limite** : 9 à 22 positions, W au plus 49. L'échelle est jugée par J1 à J6, jamais par l'oracle.
- **Planchers de G1** : au moins 1 000 nuages, dont 300 à coquilles étendues et 300 pondérés ; K jusqu'à 16 ; O1 et O2 verts.

### 12.2 Fixtures gravées (coordonnées exactes, permanentes)

F1 contre-exemple E5 ; F2 carré cocirculaire ; F3 triangle rectangle ($q_{\min}=2$, coquille étendue) ; F4 coquille à 7 points ; F5 tétraèdre sans face q3 à K3 ; F6 octaèdre régulier ; F7 coins d'un cube ; **F8a paire (3,1)**, ordres critiques relatifs {1, 4} ; **F8b triangle aigu (3,1,1)**, {2, 4, 5} ; F8c paire (3,3), position de poids K, tous les points identiques ; F9 centre sur un plan dyadique à plusieurs niveaux ; F10 extrêmes u18 (coordonnées 0 et 262 143, triangles de côtés proches de $2^{18}$), fixture de bornes et non de marge ; **F11 sphère $R^{2}=89$ (144 points)**, refus `resource_exhausted(wide_leaf)` ; F12 $n\le K$, $n=1$, $n=2$ ; F13 quadrilatère AC/BD à $p=K_{\max}-1$ ; **F14 le triangle q3 de `clusters_8000`** (sommets (20832, 50003, 50014), (50011, 20956, 20989), (50077, 50157, 50037)), faussement émis si les sites sont mis à l'échelle ; **F15 grille $20^{3}$ au pas 1**, 0 feuille forcée à T = 6 ; **F16 petite grille dans un coin de la racine fixe** (phase en largeur, parallélisme non dégénéré).

### 12.3 Différentiel v9 (sujet épinglé, `ce8a649dd`)

- **Épingles L13 retrouvées dans cette session, 12 sur 12** (boules, comptes par $q_{\min}$ et coquilles étendues identiques ; `out/l13_pins.txt`) : uniforme, huit amas et terrain à 8k/16k/32k K5, huit amas 8k K10, 000200 K5 (1 407 885) et K10 (5 483 320). L13 les a trouvées égales au catalogue v9. Restent à rejouer contre L13 : les deux plans 8k/16k/32k et les K10 à 32k. Coquilles, filaments, grilles, banc synthétique et brute avec sol sont des **épingles internes** de cette conception, pas des résultats v9.
- **Clé par clé** : l'outil `v9_catalogue_dump`, construit depuis le worktree v9 épinglé et hors du code v10, compare à l'export v10 depuis $S^{*}$, octet à octet. Entrées de poids 1 seulement.

### 12.4 Juges d'échantillon à l'échelle

- **J1 exactitude émise** : 2 000 boules tirées (graine publiée), recensement brut $O(n)$ ; on vérifie I, U, $S^{*}$, $q_{\min}$, l'admission, le niveau et la feuille du centre.
- **J2 juge des boîtes** (remplace l'ancien J2, ARCH § 12.4). Pour une boîte $Q$ tirée, de centre $q_{0}$ : toute boule admise centrée dans $Q$ a $r\le d_{K}(c)\le d_{K}(q_{0})+\lVert c-q_{0}\rVert$ ($d_{K}$ est 1-lipschitzienne). Ses supports et ses sites intérieurs sont donc dans $S_{Q}=X\cap\bar{B}(q_{0},d_{K}(q_{0})+\mathrm{diam}(Q))$. On énumère brutalement les 2-, 3- et 4-sous-ensembles de $S_{Q}$ (code i128 écrit indépendamment, signes nuls confirmés en BigRat), on garde ceux dont le centre est dans $Q$ (demi-ouvert), on recense sur $S_{Q}$, qui suffit par la même borne, et on compare à $\lbrace B\in\mathrm{cat}:c_{B}\in Q\rbrace$. **Strates** : feuilles ; nœuds ignorés par l'enveloppe ; boîtes des boules du 1 % de plus grand rayon (selles entre amas, ponts) ; boîtes dans les vides ; boîtes aux centres de coquilles étendues. Au moins 200 boîtes par entrée et 30 par strate. Une boîte avec $\lvert S_{Q}\rvert>150$ est sautée et comptée, sous plancher.
- **J3 oracle K-NN** : 10 000 requêtes rationnelles ; $N_{K}(c)$ brut inclus dans $L(\mathrm{locate}(c))$ ; intrus strict égal à celui de l'API.
- **J4 Euler par ordre** $k\le K_{\mathrm{cat}}-2$, entrées de poids 1, coquilles $u\le20$ (au-delà, J4 refuse avec le code 2 et le déclare).
- **J5 = J-KM2** (ARCH v2 § 12.5) : (a) $\mathrm{catalogue\_digest}(\mathrm{cat}(K))=\mathrm{catalogue\_digest}(\mathrm{restrict}(\mathrm{cat}(K+2),\mathrm{adm}_{K}))$ ; (b) Euler sur $\mathrm{cat}(K+2)$ pour les ordres $\le K$ (poids 1). Il juge les ordres K−1 et K, aveugles en v9. Sa restriction vaut aussi pour les entrées pondérées (§ 11.7). Coût mesuré : ×1,9 à K5, ×1,35 à ×1,46 à K10.
- **J6 unicité** de $S^{*}$ et tri canonique vérifié par le comparateur exact sur toutes les paires adjacentes (invariant, code 3).

### 12.5 Mutants (harnais, copies mutées au configure, jamais de `#if` dans les en-têtes produit)

| mutant | effet | tué par |
|---|---|---|
| M1 dominance d'arbre comptée à $K-1$ | exclut des sites de coquille à $p_{w}=K-1$ | J-KM2 (a) ; oracle |
| M2 dominance de feuille avec $\ge$ au lieu de $>$ | exclut des paires équidistantes | oracle ; F2 |
| M3 borne haute de feuille fermée | doublons aux frontières | J6 ; F9 |
| M4 mémo coupé | doublons de coquilles étendues | J6 ; F3, F7 |
| M5 $q_{\min}$ = arité de la présentation | admission fausse | oracle ; F3, F4 |
| M6 sortie du recensement à $p_{w}>\theta-1$ | perd les boules à $p_{w}=\theta$ | oracle ; J-KM2 |
| M7 zonogone strict | perd les droites tangentes à la boîte | oracle ; F9 |
| M8 seuil d'union des dominés à $\theta-1$ | perd des supports | oracle |
| M9 poids ignorés ($\omega$ = popcount sur feuille pondérée) | $p_{w}$ faux | oracle pondéré ; F8 |
| M10 direction k-DOP de signe faux | ignore des feuilles utiles | J2 ; oracle |
| M11 origine locale oubliée dans « centre ∈ boîte » | centre mal rangé | oracle ; F10 |
| M12 bande floue divisée par 4 | ordre canonique faux | J6 |
| M13 sites mis à l'échelle $2^{T}$ dans les centres et le recensement | débordement i128 | épingle `clusters_8000` (**observé** : 510 759 au lieu de 510 758) ; F14 |
| M14 admission avec $q_{\min}$ compté en poids | perd des boules pondérées | oracle pondéré ; F8a |
| M15 mémo sur égalité de $S^{*}$ au lieu de $U$ | doublons ou pertes de coquilles étendues | J6 ; oracle |
| M16 index de site = ordre d'entrée | digest dépendant de la permutation | P-PERM |

Chaque mutant doit être tué (code 4) **par la porte nommée**.

### 12.6 Portes de coût (redéfinies)

Compteurs déterministes (§ 12.8). Deux mesures de travail :
- **arbre** $=$ visites de sites parents (le noyau fait $\lvert Y\rvert$ tests par visite) ;
- **feuille** $\mathrm{WU}=2\,n_{\mathrm{dom}}+20\,n_{\mathrm{paires}}+100\,(n_{\mathrm{triplets}}+n_{\mathrm{q3}})+300\,n_{\mathrm{quad}}+15\,n_{\mathrm{sites\,recensés}}$ (poids en cycles mesurés par rdtsc sur 000200 K5, gelés en G1) ;
- total = feuille + 40 × visites + 1,5 × tests de dominance d'arbre.

Exposant $\alpha$ = pente log-log de 8k à 32k : arbre contre n, feuille contre boules, total contre n + boules.

| porte | périmètre | seuil | mesuré (cette révision) |
|---|---|---|---|
| COST-A | familles à doublement pur : uniforme, huit amas, terrain, coquilles, filaments, 7 familles du banc synthétique ; puis deux plans, doublons lourds et deux arcs (G3) | $\alpha_{\mathrm{arbre}}\le1{,}15$ et $\alpha_{\mathrm{feuille}}\le1{,}15$ | max 1,126 (arbre, terrain) ; max 1,086 (feuille, terrain) |
| COST-B | LiDAR emboîté s00, s01, s02, K5 et K10 | $\alpha_{\mathrm{total}}\le1{,}15$ ; $\alpha_{\mathrm{arbre}}$ publié, alarme (non bloquante) au-dessus de 1,30 | $\alpha_{\mathrm{total}}$ ≤ 1,126 (s02 K5) ; $\alpha_{\mathrm{arbre}}$ jusqu'à 1,209 (s02) |
| COST-C | 000200 et les deux autres trames de conception, K5 et K10 | WU par boule et visites par site ≤ 1,1 × les valeurs de référence (000200, ce document : K5 5 669 et 1 321 ; K10 8 448 et 1 860 ; les deux autres trames sont mesurées et gelées en G3) | référence |
| COST-D | brute avec sol contre sans sol | visites par site et WU par boule ≤ 1,3 × | ×0,92 à ×0,97 |
| COST-E | toute entrée hors F11 | aucun refus ; feuilles forcées publiées | 0 refus |

- Les rapports par compteur et par doublement restent publiés en **diagnostic**. Ils dépassent 1,15 sur des compteurs de petite valeur (terrain : quadruplets par boule 1,19 → 1,44), sans signification algorithmique.
- **Pourquoi ces seuils.** La v9 avait $\alpha\approx1{,}9$ à $2{,}1$ sur amas et plans (×3,7 à ×4,4 par doublement). Un arbre en $n\log n$ donne 1,03 à 1,08 sur cette plage. 1,15 sépare nettement les deux sur les familles pures. La série emboîtée n'est pas un doublement pur : la densité et le régime géométrique de la trame changent. Son exposant d'arbre y est publié, pas bloquant ; la porte qui compte pour le LiDAR est COST-C, en absolu, aux tailles du contrat.
- Famille $\Omega(n^{2})$ (deux arcs, v7) : on y juge le travail **par boule** (COST-A sur $\alpha_{\mathrm{feuille}}$), pas la pente en n.

### 12.7 Portes système (définitions)

- **P-THREADS** : `catalogue_digest` et digest du `LeafOracle` bit-identiques pour W ∈ {1, 2, 8, 48}.
- **P-PERM** : permutation de l'entrée et renumérotation des `PointId` ; `catalogue_digest` inchangé, `id_digest` égal à la bijection près. Vert par construction de l'index de Morton, il protège l'étape de préparation (tri, doublons, CSR) ; M16 le rend non vacant.
- **P-CAL** : balayage unique de M(K) sur G4 (3 trames, K ∈ {2, 3, 5, 10, 12}) avant le gel de la table.
- **P-SCALE** : accélération W1 → W24 → W48 publiée.
- **P-COST-CPU** : (a) **mur** du générateur à W48 sur G4, 3 trames : ≤ 0,25 s à K5 et ≤ 0,8 s à K10 ; (b) **µs CPU par boule** = somme des temps CPU des fils (`CLOCK_THREAD_CPUTIME_ID`) / boules, mesurée à W = 24 (un fil par cœur physique, frères SMT inactifs) : ≤ 4 µs à K5 et à K10. Les deux valeurs sont publiées ; (b) est la mesure de coût unitaire, (a) celle du contrat.
- **P-MEM** : RSS ≤ 1,5 × (catalogue + `LeafOracle`).

### 12.8 Grand-livre (≈ 25 compteurs)

Nœuds, terminaux ignorés, feuilles, feuilles forcées, m max des forcées, refus, $\Sigma m$, m max, visites, tests de dominance d'arbre, exclusions ; dominance de feuille, paires, triplets, vivants, centres q3, quadruplets, rejets du raccourci q4, recensements, sites recensés, rejets par le mémo ; émis par ($q_{\min}$, $p_{w}$), coquilles étendues, coquille maximale, identifiants ; feuilles au lemme Z i128 ($E'>2^{18}$) ; feuilles en voie large i256.

---

## 13. Risques

1. **K10 à 1 s en CPU seul** n'est pas garanti (§ 11.9). La feuille i128 domine (85 % du temps à K10). Parades : feuille GPU (G6) ou O-F1, décidés sur microbanc.
2. **Projections** : sonde et non code produit, hôte chargé, processeur de G4 inconnu. ± 30 % au moins ; la v9 a connu ×10 sur GPU.
3. **Pire cas non borné** : sortie $\Omega(n^{2})$ connue ; nombre de feuilles forcées non borné par un théorème. Parades : portes de coût sur familles adverses, refus explicites.
4. **Exposant d'arbre du LiDAR emboîté** : 1,21 sur s02. Il n'y a pas de doublement pur, et le coût absolu aux tailles du contrat est mesuré (COST-C). Mais une trame plus dense que prévu pourrait coûter plus par site dans l'arbre.
5. **Grilles entières** : 3 à 5 fois le coût LiDAR par boule ; à K10, 53 176 feuilles forcées de 32 sites au plus.
6. **Multiplicités** : validées à petite échelle (oracle, 600 nuages) et par J-KM2 sur 2 entrées pondérées à l'échelle. Euler pondéré (O-W2) reste ouvert.
7. **Alignements inter-sous-systèmes** : ARCH v2 garde encore l'admission pondérée à part et un digest au niveau réduit. Il faut les aligner sur ce document (§ 16).
8. **GPU** : multiplications 64 et 128 bits émulées ; divergence des feuilles ; contexte CUDA hors contrat sans processus résident.
9. **Déséquilibre LiDAR** (densité près du capteur) : phase en largeur et tâches triées par taille.
10. **Échelle de $10^{6}$ points** (≈ $1{,}2\cdot10^{8}$ boules à K10) : tri global des rangs lourd ; tri par région puis fusion, hors v10.0.

---

## 14. Tranches d'implémentation (générateur)

| tranche | contenu | porte de sortie | reçu |
|---|---|---|---|
| G0 | domaine, sites pondérés, Morton, CSR ; oracles O1 (Python) et O2 (`cpp_int`) ; fixtures F1 à F16 | oracles contre fixtures ; codes exacts | `receipts/gen_g0_<date>` |
| G1 | arbre (§ 6.2) + feuille (§ 6.3), un fil, i128, `static_assert` de bornes ; mémo par masques | ≥ 1 000 nuages égaux à O1 et O2 enregistrement par enregistrement, dont 300 pondérés ; épingles L13 ; M1 à M16 tués | idem |
| G2 | rangs, ordre canonique, sortie SoA, digests, `v9_catalogue_dump` ; J1, J4, J6 | différentiel v9 octet à octet | idem |
| G3 | parallélisme (phase en largeur), arènes, vectorisation ; portes de coût 8k/16k/32k ; J-KM2 ; juge des boîtes (J2) | P-THREADS, P-PERM, COST-A à COST-E | idem |
| G4-CPU | session G4 gardée (`start_and_verify.sh`, `stop_and_verify.sh`, TERMINATED certifié) : 3 trames et la brute, K5/K10, P-CAL puis P-COST-CPU | murs et µs par boule publiés | `receipts/g4_gen_cpu_<date>` |
| G5 | `LeafOracle` pour la tour ; J3 ; microbanc O-F1 (adoption si ≥ ×1,5 sur la feuille K10) | juges verts ; RSS publié ; décision O-F1 écrite | idem |
| G6 | GPU : noyau de dominance, puis feuille (warp par feuille) | 24/24 égalités de multiensembles sur G4 | `receipts/g4_gen_gpu_<date>` |

Chaque tranche : un commit sur `main` (code, fixtures, reçu et passation ensemble).

---

## 15. Reçus de cette révision (`design/gen_v2_probe/`)

- `genprobe2.cpp` (sha256 `6aca988b802cfbb077ab53cca950ecbf49a98c00bba13bb8aa15eb8ee529a7fc`) : sonde. Racine fixe, T, arrêt au côté minimal, poids, admission positionnelle, mémo par masques, $S^{*}$ canonique, vidage d'enregistrements, arbre de référence et arbres ingénieriés (`--fast=1|2`), feuille ingénieriée (`--fastleaf`), empreintes commutatives des listes terminales (`term_hash`) et du catalogue (`rec_hash`). Compilation : `g++ -O3 -march=native -std=c++20 -Wall -Wextra genprobe2.cpp -lpthread`.
- `indep_oracle_w.py` (sha256 `93c97054…`) et `oracle_campaign.py` (sha256 `13bb7e79…`) → `out/oracle_campaign.jsonl`, `out/oracle_campaign.log` (1 200 nuages, 0 écart) ; nuages et sorties de l'oracle archivés dans `out/oracle_work.tar.gz`. Empreintes de tous les fichiers : `SHA256SUMS`.
- `run_sweep.py` (sha256 `2c01844c…`) → `out/sweep_{fam,syn,grids,lidar,kreg,kplus2,weighted}.jsonl` (98 exécutions) ; `gates.py` (sha256 `ff844650…`) calcule les exposants du § 12.6.
- `jkm2_check.sh` → `out/jkm2_restriction.txt`.
- `microbench.sh` → `out/microbench.txt` (arbre et feuille, référence contre ingénieriée, profil rdtsc) ; sources des profils dans `profile/` (copies instrumentées de la sonde, dont la version à `nth_element` qui a donné le profil de sélection du § 11.5).
- `out/lidar_k10_leafcmp.jsonl` (feuille de référence contre feuille ingénieriée) ; `out/mutant_M13_scaled_sites.txt` (mutant M13 observé) ; `out/l13_pins.txt` (12 épingles L13 sur 12).
- `inputs/` : banc synthétique régénéré à 8k/16k/32k (graine 0, 8 groupes), filaments 32k avec doublon, uniforme pondéré, terrain pondéré, F11. Entrées externes par chemin et sha256 : trames KITTI hors dépôt, aucun octet versionné.
- Reçus du critique conservés : `crit_GEN/` (stagnation, oracle indépendant d'origine).

---

## 16. Points à trancher hors de ce document

1. **ARCH v2 et TOWER** : adopter l'admission positionnelle unique (suppression de la clause pondérée « $p\le k-1$ » d'ARCH v2 § 5.2) et le digest sans niveau ni rang pour J-KM2.
2. **Contrat K10 à 1 s** : l'accepter comme dépendant de la feuille GPU (G6) ou d'O-F1. Sinon le déclarer hors d'atteinte en CPU seul.
3. **O-F1** : filtre flottant certifié autorisé, sous preuve écrite de borne et à la seule condition d'un gain mesuré d'au moins ×1,5.
4. **COST-B** : accepter que l'exposant d'arbre du LiDAR emboîté soit publié sans être bloquant (1,21 mesuré sur s02), la porte LiDAR bloquante étant COST-C en absolu.
