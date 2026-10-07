# Les niveaux d'un polyèdre dans la hiérarchie, et entre les ordres

6 octobre 2026, rédigé de 22 h 14 à 22 h 45 UTC (heures lues par `date -u`). Rôle : théoricien « niveaux » du
workflow « généraliser le complexe alpha à l'ordre K ». Aucun commit, aucun push, aucune écriture hors de `build/`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

Sources lues en entier : audit `d2be6bdc7` (`receipts/audit_hartigan_delaunay_20261006/README.md`), réponse du
développeur `ee2df0362` (`audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_20261006.md`), audit `28d70f8ab`
(`receipts/audit_hartigan_robustesse_20261006/README.md` et `ombre_et_niveaux/README.md`). À 22 h 35 UTC,
`origin/main` = `5ca12e8dd` : aucun commit d'audit postérieur à `28d70f8ab`. Thèse, parties I–II (texte extrait) :
Fait 2 (MST), déf. 8, 20–22, 27–31, th. 2–6, prop. 6, § 6.1 (six points), ch. 7 (th. 3), § 9.1.

**En bref.** (1) La famille $A_{k,v}(r)$ d'un nœud se stocke sous forme d'événements : chaque cellule de la mosaïque
reçoit **un** propriétaire, le nœud où elle apparaît ; les sous-complexes $Z_v$ sont laminaires et $A_{k,v}(r)$ en est un
filtre par date (E1, prolongement exact du § 9.1). (2) Les **niveaux d'un nœud** sont les paliers de son propre module
$H_1$, $H_2$ ; la coupe de forme canonique est le milieu du plus long palier ; elle est certifiée au bruit $\delta$ si le
palier dépasse $4\delta$ : identité, couverture (à une frange publiée près) et homologie persistante sont alors stables
(E2–E4) ; la coupe de couverture $A_v(d_v^-)$ est celle du chapitre 7 (E5) ; la racine a une borne canonique, $R(P)$
(lemme 0). (3) L'offset est une **enveloppe de couverture** : sa trace est l'amas discret, un contact d'offsets annonce une
fusion dans $(r,2r]$ sans en être une, et il ferme ou crée des trous (E6–E9). (4) Entre ordres, les objets forment un
**ordre partiel** ; régions, offsets et couvertures sont emboîtés, **pas** les représentants (fixture $pqst$) ; le domaine
d'isolement d'un objet donne ses niveaux entre ordres et sa robustesse aux aberrants ; aucun représentant homotope ne
garde un trou fermé (E10–E13). (5) Fait nouveau sur l'exemple de la thèse : à $K=2$, la fusion finale des six points
crée **deux trous** (vivants jusqu'à $r=2$), tués par l'application vers l'ordre 1. (6) À $K=1$, tout redonne le complexe
alpha, l'arbre couvrant minimal et le Single-Linkage (tableau du § 4, E14).

Statuts employés : **prouvé** (preuve complète ici, ou chez l'auditeur et citée), **vérifié borné** (oracle exact
sur un nuage fini), **conjecture**. Rien n'est « exact » au sens du registre. Vérifications rejouables :
`verif/oracle_niveaux.py`, `verif/experiences.py` (→ `verif/resultats.json`), `verif/paliers.py`
(→ `verif/paliers.json`), `verif/fer_a_cheval.py`, `verif/six_perturbe.py`, empreintes dans `verif/SHA256SUMS` ;
tous verts en Python normal et `-O`. L'oracle est
**indépendant de toute mosaïque** : il calcule le nerf $N^W_K(r)$ des régions témoins de la thèse (familles de
K-parties dont la réunion a une plus petite boule englobante de rayon $\le r$), avec plus petites boules englobantes
exactes en rationnels ou dans $\mathbb{Q}(\sqrt{3})$, puis la persistance (gudhi) sur les **rangs entiers** des niveaux
carrés exacts, en corps $\mathbb{Z}/2$ et $\mathbb{Z}/3$ (barres identiques). Petites tailles : oracle de correction
seulement ; **rien n'est mesuré à l'échelle** dans cette note.

## 0. Cadre, notations, ce qui est repris sans être refait

$P\subset\mathbb{R}^3$ fini, $n$ sites distincts de poids 1 (contrat FULL unitaire), $1\le k\le n$, boules fermées,
$d_k(y)$ distance au $k$-ième site le plus proche, $\Omega_k(r)=\{y : d_k(y)\le r\}=L_K(r)$ (thèse, § 6.3), niveau
stocké $a=r^2$. Hiérarchie d'ordre $k$ : arbre de fusion $T_k$ en **convention FULL** : un nœud $v$ vit sur
$[b_v,d_v)$ en rayon ; à $d_v$ il est remplacé par son parent né à $d_v$ (multi-fusions non binarisées) ; la racine a
$d_v=+\infty$. $C_v(r)$ est sa composante pour $r\in[b_v,d_v)$ ; pour $r\ge b_v$, $\mathrm{anc}_r(v)$ est le nœud vivant à
$r$ dont la composante contient $C_v(b_v)$ ; $w\preceq v$ signifie « $w$ descendant de $v$ ou $v$ ». $M_k$ est la mosaïque
d'ordre $k$ (subdivision régulière duale aux domaines pleins $V_Q$), de cellules $\sigma$, de dates $a_\sigma$ ;
$A_k(r)=\{\sigma : a_\sigma\le r^2\}$ ; les sommets $c_Q$ portent leurs labels $Q$. $E_v(r)=P\cap(C_v(r)\oplus\bar{B}_r)$ est
l'amas discret (déf. 8). $R(P)$ est le rayon de la plus petite boule englobante de $P$.

Repris de l'auditeur, cités et non refaits :

- **(A1)** $\lvert A_k(r)\rvert\simeq N^C_k(r)\simeq N^W_k(r)\simeq\Omega_k(r)$, naturellement en $r$, sans position générale ;
  identification des $\pi_0$ par les labels ; $\pi_0\Gamma_K=\pi_0 N^W_K$ (`d2be6bdc7`, §§ 0 et 1.1).
- **(A2)** $E_v(r)$ est la réunion des labels $Q$ des sommets de $A_{k,v}(r)$ (§ 1.3).
- **(A3)** $\lvert A_{k,v}(r)\rvert\subseteq C_v(r)\oplus\bar{B}_r$ et $C_v(r)\subseteq\lvert A_{k,v}(r)\rvert\oplus\bar{B}_r$ (§ 2).
- **(A4)** $C_v(r)\oplus\bar{B}_r$ est la réunion des $C_Q(r)\oplus\bar{B}_r$ sur les $Q$ attribués à $v$ (§ 6).
- **(A5)** une bijection $\varphi:P\to P'$ avec $\lvert\varphi(p)-p\rvert\le\delta$ donne $\lVert d_k^P-d_k^{P'}\rVert_\infty\le\delta$, donc
  $\Omega_k^P(r)\subseteq\Omega_k^{P'}(r+\delta)\subseteq\Omega_k^P(r+2\delta)$ ; une fusion de sites casse la bijection (`28d70f8ab`, § 1.1).
- **(A6)** $m$ ajouts : $\Omega_k^P(r)\subseteq\Omega_k^{P\cup O}(r)\subseteq\Omega_{k-m}^P(r)$ ; $m$ suppressions :
  $\Omega_{k+m}^P(r)\subseteq\Omega_k^{Q}(r)\subseteq\Omega_k^P(r)$ (§ 1.2).
- **(A7)** l'ombre $S_v(r)$ et ses propriétés (§ 4) ; **(A8)** la coupe $A_v(d_v^-)$, et l'absence de plus grand rayon avant
  la mort pour $\Omega$ (§ 5, $P=\{0,2,10\}$).

Hypothèses : **aucune position générale**, sauf mention ; tout est vrai pour un $P$ fini quelconque, donc sur la grille
de 1 mm avec ses cosphéricités ; arithmétique exacte ; pas de simulation de simplicité. Les énoncés de robustesse
E2, E3 et E5 (iii) supposent une **bijection** des sites (pas de fusion de retours en un site).

**Lemme 0 (prouvé) — finitude, continuité à droite, borne canonique de la racine.** (i) Il existe
$t_1<\dots<t_N$ tels que $A_k(r)$, donc $\pi_0$ et l'homologie de $\Omega_k(r)$ avec leurs applications, soient constants
sur chaque $[t_i,t_{i+1})$ et sur $[t_N,\infty)$. (ii) Pour $r\ge R(P)$, $\Omega_k(r)$ est contractile pour tout $k$ ; aucun
événement $H_0$, $H_1$, $H_2$ n'a lieu au-delà de $R(P)$.

*Preuve.* (i) $M_k$ est finie et $A_k(r)$ ne change qu'aux $\sqrt{a_\sigma}$ ; (A1) transporte. (ii) Le centre $c$ de la plus
petite boule englobante est à distance $\le R(P)\le r$ de tout site, donc appartient à toutes les régions témoins
$W_Q(r)$ : le nerf $N^W_k(r)$ est un simplexe plein, contractile, et le lemme du nerf conclut. ∎
Cela répond à la réserve de l'auditeur (« la racine exige une borne d'échelle finie déclarée ») par une borne
**canonique** : $R(P)$. Pour $K=1$, c'est le fait classique que l'union des boules est contractile dès qu'elles
contiennent toutes $c$.

## 1. Question 1 — Les niveaux d'un nœud

### 1.1 Propriétaire d'une cellule : la partition laminaire (prolongement du § 9.1)

**Définition 1.** Pour une cellule $\sigma$ de $M_k$, $\nu(\sigma)$ est le nœud vivant au rayon $\sqrt{a_\sigma}$ dont la
composante contient $\sigma$ ($\sigma$ est convexe, donc dans une seule composante). On pose $\Sigma_v=\nu^{-1}(v)$ et
$Z_v=\bigcup_{w\preceq v}\Sigma_w$.

**E1 (prouvé) — la famille d'un nœud sous forme d'événements.**
(i) Les $\Sigma_v$ partitionnent $M_k$.
(ii) $Z_v$ est un sous-complexe fermé par faces, et la famille $(Z_v)$ est laminaire : $Z_w\subseteq Z_v$ si $w\preceq v$,
$Z_v\cap Z_w=\varnothing$ si $v$ et $w$ sont incomparables.
(iii) Pour $r\in[b_v,d_v)$ : $A_{k,v}(r)=\{\sigma\in Z_v : a_\sigma\le r^2\}$.
(iv) $A_{k,v}(b_v)$ est la réunion disjointe des $A_{k,w}(d_w^-)$ des enfants $w$ et des cellules de $\Sigma_v$ datées
$b_v^2$ ; ensuite $A_{k,v}(r)$ ne gagne que les cellules de $\Sigma_v$ de dates $\le r^2$.
(v) $E_v(r)$ est la réunion des $Q$ tels que $c_Q\in Z_v$ et $a_{c_Q}\le r^2$.

*Preuve.* (iii) Soit $\sigma$ active à $r$. Par (A1), la composante de $A_k(r)$ qui contient $\sigma$ est l'image de celle qui la
contient à $\sqrt{a_\sigma}$, c'est-à-dire la composante de $\mathrm{anc}_r(\nu(\sigma))$. Donc $\sigma\in A_{k,v}(r)$ si et seulement si
$\mathrm{anc}_r(\nu(\sigma))=v$, soit $\nu(\sigma)\preceq v$ puisque $v$ est vivant à $r$. (i) $\nu$ est une application.
(ii) Si $\tau$ est une face de $\sigma$, $a_\tau\le a_\sigma$ et, au rayon $\sqrt{a_\sigma}$, $\tau\subset\sigma$ est dans la composante de
$\sigma$ : $\mathrm{anc}(\nu(\tau))=\nu(\sigma)$, d'où $\nu(\tau)\preceq\nu(\sigma)$. Les ensembles de descendants d'un arbre sont
emboîtés ou disjoints. (iv) Une cellule de $Z_v$ datée avant $b_v^2$ a un propriétaire vivant avant $b_v$, donc descendant
d'un enfant $w$, et elle est dans $A_{k,w}(d_w^-)$ car $d_w=b_v$ ; une cellule datée $b_v^2$ a pour propriétaire le nœud
vivant à $b_v$, c'est-à-dire $v$. (v) Par (A2) et (iii). ∎

Lecture. C'est le § 9.1 transposé : « l'arbre est une partition des $(K-1)$-simplexes » devient « l'arbre est une
partition des cellules de la mosaïque ». Chaque cellule est rangée **une fois**, avec sa date, dans le nœud où elle
apparaît ; la famille entière $\{A_{k,v}(r)\}$ de tous les nœuds tient dans la taille de $M_k$ ; une coupe est une **vue**
(filtre de $Z_v$ par la date), jamais une copie. Les sommets de $M_k$ sont les K-parties à domaine plein ; leur
propriétaire est l'ancêtre, à la date $a_{c_Q}\ge\mathrm{MEB}(Q)^2$, du nœud de naissance de $Q$ dans $\Gamma_K$ (car
$C_Q(r)\subseteq W_Q(r)$, convexe) : les deux partitions sont compatibles. Les masses $m_\tau$ et les votes du § 9.1 restent
sur les K-parties ; raffiner ou réduire la mosaïque ne les change pas. Le coût est celui de la mosaïque (de l'ordre de
$10^3$ faces par point à $k=5$ selon le workflow précédent ; **non mesuré ici**), donc en aval et borné, jamais dans le
calcul de la hiérarchie.

**Cas K = 1.** $M_1$ est la mosaïque de Delaunay (cellules polytopes en cas cosphérique). Les feuilles sont les sites,
nés à 0 ; la cellule qui fusionne est une arête de l'arbre couvrant minimal ; $Z_v$ est le sous-complexe alpha de la
composante. Au contraire de $K\ge 2$, $E_v(r)=P\cap C_v(r)$ est **constant** sur $[b_v,d_v)$ : un site n'entre que par une
fusion, puisqu'il est sa propre feuille dès $r=0$. Pour $K\ge 2$, la couverture croît pendant la vie (A8, `growth_ABCZ`).

### 1.2 Module d'un nœud, paliers, coupe canonique

**Définition 2.** Pour $q\in\{1,2\}$ et un corps $F$, le module du nœud est $M_v^q(r)=H_q(C_v(r);F)\cong H_q(A_{k,v}(r);F)$ pour
$r\in[b_v,d_v)$, avec les applications d'inclusion (l'équivalence naturelle (A1) respecte les composantes). Soit
$\bar{d}_v=d_v$, et $\bar{d}_v=R(P)$ pour la racine (lemme 0). Les **événements internes** sont $b_v$, $\bar{d}_v$ et les
extrémités des barres de $M_v^1$, $M_v^2$ dans $(b_v,\bar{d}_v)$ ; les **paliers** sont les intervalles entre événements
consécutifs ; sur un palier, toutes les applications de $M_v$ sont des isomorphismes. **Les niveaux du nœud sont ses
paliers.** Ils se calculent par une réduction de persistance du complexe filtré $Z_v$ (E1), sans autre donnée.

**Définition 3.** Marge d'une coupe : $q_v(r)$ = distance de $r$ à l'ensemble des événements. **Coupe de forme
canonique** : $r^*_v$, milieu (en rayon) du plus long palier, le plus petit en cas d'égalité ; marge publiée $q^*_v$ = la
moitié de sa longueur $L_v$. **Coupe de couverture** : $A_v(d_v^-)=\{\sigma\in Z_v : a_\sigma<d_v^2\}$ (A8).

**E2 (prouvé) — sandwich d'identité et de couverture.** Soit $\varphi:P\to P'$ une bijection avec
$\lvert\varphi(p)-p\rvert\le\delta$. Si $b_v\le r-\delta$ et $r+\delta<d_v$ :
(i) la composante $C'$ de $\Omega_k^{P'}(r)$ qui contient $C_v^P(r-\delta)$ vérifie $C_v^P(r-\delta)\subseteq C'\subseteq C_v^P(r+\delta)$ ;
(ii) $C'$ ne rencontre aucune autre composante de $\Omega_k^P(r-\delta)$ ; deux nœuds distincts de $P$ satisfaisant
l'hypothèse au même $r$ ont des partenaires distincts ;
(iii) $\varphi(E_v^P(r-\delta))\subseteq E^{P'}_{C'}(r)\subseteq\varphi(E_v^P(r+\delta))$.

*Preuve.* (i) Par (A5), $C_v^P(r-\delta)\subseteq\Omega^{P'}(r)$ est connexe, d'où $C'$ ; $C'\subseteq\Omega^{P'}(r)\subseteq\Omega^P(r+\delta)$ est
connexe et contient $C_v^P(r-\delta)\subseteq C_v^P(r+\delta)$, donc $C'\subseteq C_v^P(r+\delta)$. (ii) Si $C'$ rencontrait une autre
composante $D$ de $\Omega^P(r-\delta)$, $D\cup C'$ serait connexe dans $\Omega^P(r+\delta)$, donc $D\subseteq C_v^P(r+\delta)$ : $v$ aurait
fusionné avec $D$ avant $r+\delta<d_v$, ce qui le tue. Deux nœuds de même partenaire contrediraient (ii).
(iii) Si $x\in E_v^P(r-\delta)$, un $y\in C_v^P(r-\delta)\subseteq C'$ vérifie $\lvert y-x\rvert\le r-\delta$, donc $\lvert y-\varphi(x)\rvert\le r$.
Si $x'\in E^{P'}_{C'}(r)$, un $y'\in C'\subseteq C_v^P(r+\delta)$ vérifie $\lvert y'-x'\rvert\le r$, donc
$\lvert y'-\varphi^{-1}(x')\rvert\le r+\delta$. ∎

La **frange de couverture** $E_v(r+\delta)\setminus E_v(r-\delta)$ est l'ensemble des seuls sites dont l'appartenance peut changer
sous ce bruit : elle se publie et se mesure.

**E3 (prouvé) — homologie certifiée d'une coupe.** Mêmes hypothèses sur $\varphi$. Si $b_v\le r-2\delta$, $r+2\delta<d_v$ et
qu'aucun événement interne de $v$ n'est dans $(r-2\delta,r+2\delta]$, alors, pour tout degré $q$ et tout corps,
$$\mathrm{rang}\left(H_q(C'_{r-\delta})\to H_q(C'_{r+\delta})\right)=\dim H_q(C_v^P(r)),$$
où $C'_s$ est la composante de $\Omega_k^{P'}(s)$ qui contient $C_v^P(s-\delta)$.

*Preuve.* Comme dans E2, $C_v^P(r-2\delta)\subseteq C'_{r-\delta}\subseteq C_v^P(r)\subseteq C'_{r+\delta}\subseteq C_v^P(r+2\delta)$. L'application
$f:H_q(C'_{r-\delta})\to H_q(C'_{r+\delta})$ se factorise par $H_q(C_v^P(r))$, donc $\mathrm{rang} f\le\dim H_q(C_v^P(r))$. La composée
$H_q(C_v^P(r-2\delta))\to H_q(C'_{r-\delta})\to H_q(C'_{r+\delta})\to H_q(C_v^P(r+2\delta))$ est l'application interne du nœud, un
isomorphisme par hypothèse ; elle se factorise par $f$, donc $\mathrm{rang} f\ge\dim H_q(C_v^P(r-2\delta))=\dim H_q(C_v^P(r))$. ∎

**Corollaire.** Si $L_v>4\delta$, la coupe canonique montre exactement les trous et cavités qui persistent au moins $2\delta$
dans les données perturbées autour de $r^*_v$ ; tout bruit $\delta<L_v/4$ est **certifiable** pour ce nœud. Un nœud dont
aucun palier ne dépasse $4\delta$ (en particulier si $d_v-b_v\le 4\delta$) n'a **aucune coupe de forme certifiée au bruit
$\delta$** : on le garde dans l'arbre et on le condense à l'affichage (analogue de `min_cluster_size`, mais en rayon et
avec une preuve). Ce n'est pas une stabilité géométrique du dessin, que l'auditeur a réfutée en général (triangle de
`28d70f8ab`, § 1.1) : c'est la stabilité de l'identité, de la couverture à la frange près et de l'homologie de la coupe.

**E4 (prouvé) — propriétés de la coupe canonique.** (a) Elle ne dépend que de la filtration : aucun paramètre, $\delta$ ne
sert qu'à la certifier. (b) Elle est équivariante par isométrie, par homothétie (tous les événements sont multipliés par
$\lambda$) et par réétiquetage. (c) Parmi toutes les coupes, elle maximise la marge, donc le bruit certifiable par E3 : une
coupe $r$ a une marge au plus égale à la demi-longueur de son palier, donc au plus $L_v/2$. (d) Pour la racine, la borne
$R(P)$ est canonique (lemme 0). (e) **Limite** : l'argmax n'est pas stable si deux paliers ont des longueurs à moins de
$4\delta$ l'une de l'autre ; publier donc **tous les paliers de longueur supérieure à $4\delta$** (les niveaux certifiés du
nœud), chacun avec sa coupe médiane et sa marge, plutôt qu'un seul jeton.

*Preuve.* (a), (b), (d) par construction ; (c) ci-dessus. ∎

**E5 (prouvé) — le critère du chapitre 7 et la coupe de couverture.** (i) $E_v(r)$ est croissant et constant par morceaux
sur $[b_v,d_v)$ ; son supremum $E_v(d_v^-)$ est la réunion des $Q$ avec $c_Q\in Z_v$ et $a_{c_Q}<d_v^2$, atteint à la dernière
date de sommet avant $d_v^2$. (ii) Le théorème 3 du chapitre 7 mesure la fraction récupérée « juste avant la percolation du
fond » : dans la hiérarchie finie, c'est $E_v(d_v^-)$ pour le nœud $v$ dont la mort est la fusion parasite. Choisir *quel*
nœud est l'amas est la sélection (§ 9.1, excès de masse), pas la représentation. (iii) Sous un bruit $\delta$, E2 garantit
$\varphi(E_v(r-\delta))\subseteq E'(r)$ pour tout $r<d_v-\delta$ : la couverture garantie est $E_v((d_v-2\delta)^-)$, et le **prix de la
robustesse** $E_v(d_v^-)\setminus E_v((d_v-2\delta)^-)$ se publie. (iv) Coupe de forme et coupe de couverture n'ont pas le même
but : forme stable (E3, E4) contre rappel maximal avant fusion (ch. 7) ; on publie les deux, nommées, avec leur côté
ouvert ou fermé (A8).

*Preuve.* (i) $C_v$ et $\bar{B}_r$ croissent ; (A2) et E1 (v) ; finitude. (iii) E2 (iii) avec $r\uparrow d_v-\delta$. ∎

**Emboîtement le long de la hiérarchie (R2), à ordre fixé (prouvé).** Comme $r^*_v<d_v=b_{\mathrm{par}(v)}\le r^*_{\mathrm{par}(v)}$ et
$Z_v\subseteq Z_{\mathrm{par}(v)}$, les jetons canoniques sont emboîtés comme sous-complexes :
$A_{k,v}(r^*_v)\subseteq A_{k,\mathrm{par}(v)}(r^*_{\mathrm{par}(v)})$ ; de même les ombres, les couvertures et les offsets. Pour une réduction
certifiée **globale** $L$ (auditeur, § 4.2, certificat sur toute la plage), $L\cap A_{k,v}(r)$ hérite de cet emboîtement ; des
réductions nœud par nœud ne l'héritent pas.

### 1.3 Exemples exacts (vérifiés bornés)

**Les six points du § 6.1** (unité : le $r$ de la thèse ; côtés $2$ ; $A,B=(-1-\sqrt{3},\pm 1)$, $C=(-1,0)$, $D=(1,0)$,
$E,F=(1+\sqrt{3},\pm 1)$ ; calcul dans $\mathbb{Q}(\sqrt{3})$, `verif/resultats.json`). $R(P)=\sqrt{5+2\sqrt{3}}\approx 2{,}909$.

| K | Nœuds et vies (rayons) | Barres $H_1$ | Paliers et coupe canonique |
| --- | --- | --- | --- |
| 1 | 6 feuilles $[0,1)$ ; racine née à 1 (6 enfants) | deux trous $[1,2/\sqrt{3})$ (triangles ABC, DEF) | racine : $[1;1{,}155)$, $[1{,}155;2{,}909]$ ; coupe $2{,}032$, marge $0{,}877$ |
| 2 | 7 feuilles nées à 1 (AB, BC, AC, CD, DE, EF, DF) ; ABC et DEF nés à $2/\sqrt{3}$ (fusions triples) ; CD vit sur $[1;\sqrt{2+\sqrt{3}})$ ; racine née à $\sqrt{2+\sqrt{3}}\approx 1{,}932$ (3 enfants) | **deux trous nés à la fusion finale**, $[\sqrt{2+\sqrt{3}},2)$, autour de C et D ($d_2(C)=d_2(D)=2$) | ABC : $[1{,}155;1{,}932)$, coupe $1{,}543$, marge $0{,}389$ ; racine : $[1{,}932;2)$ puis $[2;2{,}909]$, coupe $2{,}455$ |
| 3 | ABC, DEF nés à $2/\sqrt{3}$ ; ACD, BCD, CDE, CDF nés à $\sqrt{2+\sqrt{3}}$ ; deux fusions triples à 2 ; racine à $1+\sqrt{3}$ | un trou né à la fusion finale, $[1+\sqrt{3},R(P))$, autour de l'origine | racine : un seul palier, marge $0{,}089$ |

Couvertures à $K=2$ : à $r=1$, les sept paires ; à $r=2/\sqrt{3}$, $\{A,B,C\}$, $\{C,D\}$, $\{D,E,F\}$, le site C étant
couvert par deux nœuds (fig. 6.5) ; à la racine, les six sites. Le dendrogramme $H_0$ est exactement celui de la thèse
(fig. 6.2–6.4). **Ce que la thèse ne dit pas et que l'oracle établit** : à $K=2$, la clôture de la hiérarchie au rayon
$AD/2$ crée simultanément **deux trous**, parce que quatre contacts (ACD, BCD, CDE, CDF) relient trois composantes ; ils
vivent jusqu'à $r=2$. Un même rayon critique fusionne et crée des cycles (phénomène décrit par Reani–Bobrowski, cité
par l'auditeur). Ce n'est pas un artefact de la symétrie : dans une version rationnelle perturbée ($\sqrt{3}\to 7/4$, D
décalé de $1/50$, `verif/six_perturbe.py`), ACD fusionne à $1{,}93797$ et BCD crée un trou à $1{,}94312$ ; CDE et CDF,
restés symétriques, fusionnent et créent un trou au même rayon $1{,}94052$. Le premier contact entre deux composantes
les relie, le second crée le trou. La racine d'ordre 2 a donc deux niveaux : « trois amas reliés en deux anneaux », puis
« bloc plein ». À l'ordre 1, aucun trou n'existe sur $[\sqrt{2+\sqrt{3}},2)$ : l'application verticale tue ces deux classes (E13).

**Anneaux entiers cocycliques** (dégénérés : tous les sites sur un cercle ; aucune position générale). `anneau8` :
$(\pm 1,\pm 2),(\pm 2,\pm 1)$, $R(P)=\sqrt{5}$. `anneau12` : les 12 points entiers du cercle de rayon 5. Barres $H_1$ du nœud
racine, en rayon carré exact :

| Nuage | K = 1 | K = 2 | K = 3 |
| --- | --- | --- | --- |
| `anneau8` | $[1,5)$ ; coupe $1{,}618$, marge $0{,}618$ | $[5/2,5)$ ; coupe $1{,}909$, marge $0{,}327$ | $[9/2,5)$ ; coupe $2{,}179$, marge $0{,}057$ |
| `anneau12` | $[5/2,25)$ | $[9,25)$ | non calculé |
| `anneau8` + centre $(0,0)$ | toutes les barres meurent à $25/16$ au plus tard | $[25/16,5)$ | $[5/2,5)$ |

Le trou de l'anneau meurt toujours au centre ($r=R(P)$) et naît plus tard quand $K$ croît : sa persistance décroît avec
l'ordre. **Bruit** : `anneau8` dont chaque site est déplacé d'exactement $\delta=1/10$ (vecteurs $(\pm 3/50,\pm 4/50)$ et
permutés). Les barres longues sont appariées à $\delta$ près ($K=1$ : $[1;2{,}236)\to[1{,}070;2{,}159)$ ; $K=2$ :
$[1{,}581;2{,}236)\to[1{,}676;2{,}212)$), les autres barres sont de longueur $\le 2\delta$. À $K=3$, la barre de longueur
$0{,}115<4\delta$ n'est **pas certifiable** à ce bruit, conformément à E3 ; à $K=2$, la coupe $1{,}909$ est certifiée
($[1{,}709;2{,}109]$ est dans le palier) et la barre perturbée la contient.

## 2. Question 2 — L'offset $C\oplus\bar{B}_r$

**E6 (prouvé) — rôle et construction.** Soit $\mathrm{Off}_v(r)=C_v(r)\oplus\bar{B}_r$.
(i) $\mathrm{Off}_v(r)$ est la réunion des **boules témoins** $\bar{B}(y,r)$, $y\in C_v(r)$, dont chacune contient au moins $k$ sites :
c'est la région d'où vient la densité de l'amas, son **enveloppe de couverture**.
(ii) Sa trace est l'amas discret : $P\cap\mathrm{Off}_v(r)=E_v(r)$ (déf. 8 ; (A2)).
(iii) Construction exacte : (A4), pièces convexes et leur nerf ; pièces polyédriques intérieures exactes :
$\mathrm{conv}(Q\cup C_Q(r))\subseteq C_Q(r)\oplus\bar{B}_r$ ; l'ombre $S_v(r)$ est contenue dans l'offset (A7).
(iv) Pour $K=1$ : $\mathrm{Off}_v(r)=\bigcup_{p\in E_v(r)}\bar{B}(p,2r)$ et $\Omega_1(r)\oplus\bar{B}_r=\Omega_1(2r)$. Pour tout $K$ :
$\Omega_k(r)\oplus\bar{B}_r\subseteq\Omega_k(2r)$, avec inclusion stricte possible dès $K=2$.

*Preuve.* (i), (ii) par définition et (A2). (iii) Si $y\in C_Q(r)$, tous les sites de $Q$ sont dans $\bar{B}(y,r)$, convexe, donc
$\mathrm{conv}(Q)\subseteq\bar{B}(y,r)$ ; un point $\lambda y'+(1-\lambda)x$ avec $y'\in C_Q(r)$ et $x\in\mathrm{conv}(Q)$ est à distance
$(1-\lambda)\lvert y'-x\rvert\le r$ de $y'$. (iv) À $K=1$, $C_v(r)$ est la réunion des $\bar{B}(p,r)$, $p\in E_v(r)$. En général $d_k$ est
1-lipschitzienne : $d_k(x)\le d_k(y)+\lvert x-y\rvert\le 2r$. Stricte : $P=\{-2,2\}$, $K=2$ : $\Omega_2(1)=\varnothing$ alors que
$\Omega_2(2)=\{0\}$. ∎

**E7 (prouvé, bornes atteintes vérifiées) — un contact d'offsets n'est pas une fusion, il l'annonce.** Si $v\ne w$ sont
vivants à $r$ et que leurs offsets se rencontrent, leur fusion (naissance de leur plus petit ancêtre commun) a lieu dans
$(r,2r]$. La borne $2r$ est atteinte au premier contact : $P=\{0,4\}$, $K=1$, $r=1$ (contact en 2, fusion à $r=2$) ;
$P=\{0,2,4\}$, $K=2$, $r=1$ : $\Omega_2(1)=\{1,3\}$, offsets $[0,2]$ et $[2,4]$ qui **partagent le site 2**, fusion à $r=2$. La
borne $r$ est approchée quand $r$ tend vers le rayon de fusion par valeurs inférieures.

*Preuve.* Soit $x$ dans les deux offsets, $y_v\in C_v(r)$ et $y_w\in C_w(r)$ à distance $\le r$ de $x$. Tout point $z$ des segments
$[y_v,x]$ et $[x,y_w]$ vérifie $d_k(z)\le r+r$ : les deux composantes sont reliées dans $\Omega_k(2r)$. Elles sont distinctes à $r$,
donc la fusion est postérieure à $r$. ∎

Pourquoi ce n'est pas une fusion : le modèle de Hartigan définit les amas comme composantes de $\Omega_k(r)$ ; l'offset
appartient à une autre échelle (E6 (iv)). À $K=1$, la hiérarchie des offsets est celle de HGP reparamétrée par $r\mapsto 2r$ ; pour
$K\ge 2$ c'en est une autre ($\{0,2,4\}$ : contact à 1, fusion HGP à 2). Les contacts portent l'information de **sites
partagés** (recouvrement des K-polyèdres, fig. 6.5), pas de connexité.

**E8 (prouvé) — emboîtements.** (a) $\mathrm{Off}_v(r)\subseteq\mathrm{Off}_{\mathrm{anc}_{r'}(v)}(r')$ pour $r\le r'$ ; (b) entre ordres,
$\mathrm{Off}^{(k)}_v(r)\subseteq\mathrm{Off}^{(k')}_{\pi(v)}(r')$ dès que $k'\le k$ et $r\le r'$ (notation du § 3) ; (c) idem pour $E_v$. *Preuve :*
les composantes sont emboîtées et $\bar{B}_r\subseteq\bar{B}_{r'}$. ∎

**E9 (prouvé, vérifié borné) — l'offset n'est pas un représentant topologique.** (a) Il ferme des trous : `anneau8`, $K=1$ :
$C$ a un trou pour $r\in[1,\sqrt{5})$, son offset $\Omega_1(2r)$ seulement pour $r\in[1/2,\sqrt{5}/2)$. (b) Il en crée : le fer à
cheval (sept sites d'`anneau8`, $(2,1)$ retiré), $K=1$, $r=1$ : $C$ est connexe et sans trou, son offset $\Omega_1(2)$ a
$\beta_1=1$ (`verif/fer_a_cheval.py`). L'offset est donc une enveloppe (support, couverture, contacts), jamais le
représentant de $C$ ; c'est l'auditeur qui l'avait annoncé, ces deux fixtures le rendent exact.

## 3. Question 3 — Entre les ordres

**Définition 4.** Niveaux $(k,r)$ ordonnés par $(k,r)\le(k',r')$ si $k'\le k$ et $r\le r'$ (le sens « S-Rhomb » : $r$ croît, $k$
décroît). $\Omega_k(r)\subseteq\Omega_{k'}(r')$ induit $\pi_{(k,r)\to(k',r')}$ sur les composantes, fonctoriellement. Un **objet** est
un triplet $(k,v,r)$, $v$ vivant à $r$ dans $T_k$ ; $(k,v,r)\preccurlyeq(k',v',r')$ si $(k,r)\le(k',r')$ et
$\pi(C_v(r))=C_{v'}(r')$.

**E10 (prouvé) — la tour lue sur les labels.** (a) $\preccurlyeq$ est un ordre partiel sur les objets ; s'il relie deux
objets, les régions, offsets et couvertures sont emboîtés. (b) L'application verticale se lit sur les labels :
pour toute K-partie $Q$ active à $r$ et tout $q\in Q$, $\pi_{k\to k-1}$ envoie la composante de $Q$ sur celle de $Q\setminus\{q\}$.
*Preuve.* (a) Fonctorialité des inclusions ; antisymétrie : deux objets comparables dans les deux sens ont même niveau.
(b) $W_Q(r)\subseteq W_{Q\setminus\{q\}}(r)\subseteq\Omega_{k-1}(r)$, ensemble convexe non vide. ∎

**E11 (prouvé, fixture exacte) — ce qui n'est pas emboîté entre ordres.** À ordre fixé, $A$, ombre, couverture et offset
sont emboîtés (§ 1.2). Entre ordres, seuls les régions, offsets et couvertures le sont ; **ni $\lvert A_k\rvert$ ni l'ombre**.
Fixture : $P=\{p=(-1,0),q=(0,-1/2),s=(1,0),t=(0,3/2)\}$, $r=1$. Delaunay $=\{pqt,qst\}$, deux triangles aigus de date
$65/64>1$ ; à l'ordre 2, $c_{pq}=(-1/2,-1/4)$ et $c_{qs}=(1/2,-1/4)$ sont des sommets (domaines pleins) et leur arête, de face
duale $\{x=0,\ y\le 5/12\}$, a la date $\min(1+y^2)=1$. Le point $(-1/4,-1/4)$ est sur cette arête, donc dans $\lvert A_2(1)\rvert$, et
dans l'intérieur strict du triangle $pqt$, absent de $A_1(1)$ : $\lvert A_2(1)\rvert\not\subseteq\lvert A_1(1)\rvert$. L'ombre d'ordre 2 de
cette arête, $\mathrm{conv}(p,q,s)$, n'est pas dans l'ombre d'ordre 1, qui est $\lvert A_1(1)\rvert$. Ce que l'auditeur affirmait sans
témoin (« ni une inclusion géométrique des deux mosaïques barycentriques ») a maintenant une fixture minimale.

**E12 (prouvé) — domaine d'isolement d'un objet, et robustesse de population.** Pour $v$ d'ordre $k$ et $k'\le k$, soit
$\iota_v(k')$ le supremum des $r\in[b_v,d_v)$ tels que la composante de $\Omega_{k'}(r)$ contenant $C_v(r)$ ne rencontre
$\Omega_k(r)$ qu'en $C_v(r)$ (« l'objet est vu seul à l'ordre $k'$ »). (i) Pour chaque $k'$, l'ensemble de ces $r$ est un
intervalle initial $[b_v,\iota_v(k'))$, éventuellement vide ; (ii) $\iota_v$ est croissante en $k'$ et $\iota_v(k)=d_v$ : le
**domaine d'isolement** $D_v$ est un escalier. (iii) Si $(k-m,r)\in D_v$, tout ajout de $m$ sites laisse la composante de
$\Omega_k^{P\cup O}(r)$ qui contient $C_v(r)$ disjointe des autres composantes de $\Omega_k^P(r)$ et contenue dans l'image de
$v$ à l'ordre $k-m$. (iv) Si une composante d'ordre $k+m$ s'envoie dans $C_v(r)$, toute suppression de $m$ sites laisse à
$v$ un partenaire non vide qui la contient.

*Preuve.* (i) Si l'image $J$ à $(k',r)$ contient un point d'une autre composante $D$ de $\Omega_k(r)$, à $r'\in[r,d_v)$ l'image
contient $J$ et l'ancêtre de $D$, distinct de $C_v(r')$ puisque $v$ n'a pas fusionné : l'isolement perdu ne revient pas.
(ii) L'image à l'ordre $k''\in[k',k]$ est contenue dans l'image à l'ordre $k'$. (iii) Par (A6),
$C_v(r)\subseteq\Omega_k^{P\cup O}(r)\subseteq\Omega_{k-m}^P(r)$ ; la composante cherchée est dans l'image $J$ de $v$, et
$J\cap\Omega_k^P(r)=C_v(r)$. (iv) Par (A6), $\Omega_{k+m}^P(r)\subseteq\Omega_k^{Q}(r)$. ∎

Lecture : **la robustesse aux aberrants se lit sur l'axe des ordres** (l'auditeur a montré qu'il n'y a pas d'invariance à
$k$ fixé) : un objet résiste à $m$ ajouts là où il est isolé à l'ordre $k-m$, et à $m$ suppressions s'il contient un
noyau d'ordre $k+m$. Exemple exact : ajouter le centre à `anneau8` ($m=1$). Sur $[\sqrt{5/2},\sqrt{5})$, l'application
$H_1(\Omega_2^P(r))\to H_1(\Omega_1^P(r))$ est non nulle (la boucle enlace l'axe vertical du centre, qui évite $\Omega_1^P(r)$
pour $r<\sqrt{5}$) et elle se factorise par $H_1(\Omega_2^{P\cup O}(r))$ : le trou d'ordre 2 **ne peut pas** disparaître par cet
ajout sur cet intervalle. L'oracle donne en effet le trou $[5/4,\sqrt{5})$ : il naît même plus tôt.

**« Roue ⊂ vélo ⊂ groupe » quand la roue vit à petit $k$.** Une chaîne d'objets
$$(k_w,\mathrm{roue},r_w)\preccurlyeq(k_b,\mathrm{velo},r_b)\preccurlyeq(k_g,\mathrm{groupe},r_g)$$
exige $k_g\le k_b\le k_w$ et $r_w\le r_b\le r_g$. Si la roue n'existe comme nœud qu'à un petit
ordre $k_w$, le vélo et le groupe doivent être pris à un ordre **au plus** $k_w$ : soit dans le même arbre $T_{k_w}$
(ancêtres horizontaux), soit plus bas. Un nœud « vélo » d'ordre $k_b>k_w$ n'est en général pas comparable à la roue (son
noyau dense peut l'exclure) ; ils ne se rencontrent qu'au majorant commun $(\min k,\max r)$. La hiérarchie des objets entre
ordres est un **ordre partiel**, engendré par les arbres $T_k$ et les liens verticaux de FULL, pas un arbre.

**E13 (prouvé, fixtures exactes) — obstruction des trous.** Soit $X(k,r)$ une famille de représentants munis
d'équivalences d'homotopie $X(k,r)\simeq\Omega_k(r)$ compatibles avec toutes les inclusions de la définition 4 (par exemple
la bifiltration rhomboïdale S-Rhomb de Corbet et al., citée par l'auditeur). Si une classe $\alpha\in H_q(C_v(r))$ a une image
nulle dans $H_q(\Omega_{k'}(r'))$ pour $(k,r)\le(k',r')$, son image dans $H_q(X(k',r'))$ est nulle : **aucun représentant
homotope et compatible ne peut montrer, à un niveau supérieur, un trou que la région y a fermé.**
*Preuve.* Les équivalences naturelles induisent un isomorphisme des modules de persistance à deux paramètres. ∎
Fixtures : horizontale, `anneau8` à $K=2$, trou fermé à $r=\sqrt{5}$ ; **verticale à rayon fixé**, `anneau8` + centre : le
trou d'ordre 2 vit sur $[5/4,\sqrt{5})$ alors que toute classe d'ordre 1 est morte dès $5/4$ (le site central le
remplit) ; dans la thèse même, les deux trous d'ordre 2 des six points, sur $[\sqrt{2+\sqrt{3}},2)$, sont tués par
l'application vers l'ordre 1, et le trou d'ordre 3 sur $[1+\sqrt{3},R(P))$ par l'application vers l'ordre 2.

**Ce que « représenter les niveaux d'un objet à travers les ordres » veut dire (proposition).** (1) L'objet est un nœud
$(k,v)$ ; (2) ses niveaux en $r$ sont ses paliers (E2–E4), chacun avec sa coupe et sa marge ; (3) ses niveaux entre ordres
sont ses images sur le domaine d'isolement $D_v$ (E12), chacune représentée par le jeton du nœud image à son propre
ordre ; (4) les relations entre objets sont $\preccurlyeq$ (E10) ; (5) l'inclusion géométrique des représentants n'existe qu'à
ordre fixé (E1, E11) ; entre ordres on transporte l'**identité** par $\pi_0$, jamais par projection ou plus proche
barycentre (auditeur) ; (6) la topologie peut changer le long de $\preccurlyeq$ (E13) : c'est une propriété de l'objet, pas un
défaut du représentant. Le jeton de la roue se garde donc à son ordre et à son palier ; le vélo a le sien ; une vue
« réunion des jetons des enfants » est permise comme vue déclarée, jamais comme représentant revendiqué homotope. Les
applications au-delà de $\pi_0$ (cellulaires entre ordres) demandent le modèle bifiltré S-Rhomb, dont le coût n'est pas
mesuré ; pour la tour FULL, un représentant par ordre et les liens $\pi_0$ suffisent (auditeur, § 5).

## 4. Question 4 — Le tableau d'analogie K = 1 ↔ K

**E14 (prouvé) — l'arbre couvrant minimal sur la mosaïque.** Pour tout $k$ et sans position générale, la forêt couvrante
minimale du 1-squelette de $M_k$, arêtes pesées par $a_\sigma$ et sommets nés à $a_{c_Q}$, a le même arbre de fusion que
$\Gamma_K$ (FULL). *Preuve.* $A_k(r)$ est un complexe polyédral fermé par faces dont chaque cellule de dimension $\ge 2$ a un
bord connexe déjà présent : $\pi_0 A_k(r)$ est celui de son 1-squelette. Le fait 2 de la thèse s'étend aux graphes
filtrés par sommets, puisqu'une arête n'est active qu'avec ses extrémités. (A1) conclut. ∎ Le théorème 5 de la thèse
suppose la position générale pour la filtration de Čech ; E14 non, mais il construit la mosaïque : c'est donc un
**oracle** de cohérence en aval, jamais une voie de calcul de la hiérarchie (invariant d'architecture).

| Rôle | K = 1 | Ordre K | Justification |
| --- | --- | --- | --- |
| Objet de Hartigan | $\Omega_1(r)=\bigcup\bar{B}(p,r)$ ; composantes = Single-Linkage | $\Omega_K(r)=L_K(r)$ (déf. 7) | thèse, partie I |
| Épais, combinatoire | Čech ; 1-squelette $G(P,2r)$ | nerf $N^W_K(r)$ des $W_Q(r)$ ; 1-squelette $\Gamma_K$ (déf. 21) | th. 2 ; (A1) |
| Mince, la forme | alpha : $V_p\cap\bar{B}(p,r)$, Delaunay filtré | $A_K(r)$ : $V_Q\cap W_Q(r)$, mosaïque d'ordre K filtrée par $a_\sigma$ | (A1) ; même construction |
| Squelette $H_0$ | MST euclidien (faits 1–2, th. 1) | K-MST (déf. 30, th. 4–5, prop. 6) ; MST de la mosaïque (E14) | E14 |
| Gabriel | arête de Gabriel | K-simplexe de Gabriel (déf. 28), porté par la mosaïque (th. 6) | thèse ch. 8 |
| Couverture discrète | $E=P\cap C$, partition, constante pendant une vie | $E=P\cap(C\oplus\bar{B}_r)$, recouvrement, croît pendant une vie | déf. 8 ; E1 |
| Partition de l'arbre | sites | K-parties (§ 9.1) ; cellules de la mosaïque (E1) | E1 |
| Offset | $\bigcup_{p\in E}\bar{B}(p,2r)$ ; $\Omega_1(r)\oplus\bar{B}_r=\Omega_1(2r)$ | $\subseteq\Omega_K(2r)$, strict ; contact ⇒ fusion dans $(r,2r]$ | E6, E7 |
| Ombre (rendu sur les données) | égale à $\lvert A_{1,v}\rvert$ | contient $\lvert A_{K,v}\rvert$, même trace que l'offset | (A7) ; cellule = conv des sites |
| Points pondérés, DTM | poids nuls : $f_1=d_1$, puissance = Voronoï | barycentres pondérés (BCY 4.8) ; $f_K\ne d_K$, autre hiérarchie | auditeur |
| Wrap | existe (Bauer–Edelsbrunner) | pas d'analogue par analogie | auditeur |
| Niveaux d'un nœud | paliers du sous-complexe alpha | paliers de $A_{K,v}$, coupe canonique, couverture | E2–E5 |
| Entre ordres | sans objet | ordre partiel, liens $\pi_0$, S-Rhomb ; obstruction des trous | E10–E13 |
| Robustesse | entrelacement de $\delta$ | idem (A5) ; population : décalage de K (A6, E12) | E2, E3, E12 |

**Cas K = 1 de chaque énoncé.** E1 : partition des cellules de Delaunay, couverture constante pendant une vie. E2–E4 :
mêmes énoncés pour les sous-complexes alpha ; les feuilles sont des sites, de coupe $d_v/2$. E5 : la couverture ne
change qu'aux fusions. E6–E9 : l'offset est l'union des boules de rayon $2r$, d'où la même hiérarchie reparamétrée par $r\mapsto 2r$,
mais il peut fermer ou créer des trous (fixtures `anneau8` et fer à cheval, toutes deux à $K=1$). E10–E13 : sans objet à
un seul ordre, sinon l'ordre 1 est le bas de la tour, où toutes les images aboutissent. E14 : l'arbre couvrant minimal
euclidien, via les arêtes de Gabriel.

## 5. Recommandation au workflow

1. Stocker la famille d'un nœud **sous forme d'événements** : (propriétaire, date) par cellule (E1), une seule fois,
   dans la mosaïque ambiante ; les coupes sont des vues.
2. Publier pour chaque nœud ses **paliers** (événements $H_1$, $H_2$ du sous-complexe $Z_v$), la coupe de forme canonique
   (milieu du plus long palier) avec sa marge $q^*_v$ et sa borne de bruit certifiable $L_v/4$ (stricte), et la coupe de couverture
   $A_v(d_v^-)$ avec sa frange ; déclarer « bruit au niveau $\delta$ » les nœuds sans palier de longueur $>4\delta$.
3. Fixer $\delta$ honnêtement : $\sqrt{3}/2$ mm de quantification **seulement si aucun site n'est fusionné**, plus le
   bruit capteur déclaré ; sinon le modèle pondéré.
4. Entre ordres : un jeton par nœud à son ordre, les liens $\pi_0$ de FULL, le domaine d'isolement $D_v$ comme lecture
   des niveaux d'un objet et de sa robustesse aux aberrants ; aucun lien par géométrie.
5. Graver les fixtures : six points (deux trous à la fusion finale d'ordre 2), `anneau8` et son centre (obstruction
   verticale), $pqst$ (non-inclusion entre ordres), $\{-2,2\}$, $\{0,4\}$, $\{0,2,4\}$ (offset), fer à cheval.

## 6. Questions ouvertes

- **Q-a (conjecture).** Les paliers de $Z_v$ calculés sur la mosaïque d'une scène LiDAR sont-ils assez longs (devant
  $4\delta$) pour que les objets (roue, piéton) aient un niveau certifié ? Non mesuré : demande la mosaïque sur des
  découpes à 8 000–32 000 sites, en aval.
- **Q-b.** Un critère de choix entre paliers qui soit lui-même stable (par exemple pondéré par la couverture ou par la
  masse du § 9.1) reste à formuler ; E4 (e) seulement contourne l'instabilité de l'argmax en publiant tous les paliers.
- **Q-c.** Une version de E3 pour la géométrie (pas seulement l'homologie) exige la borne locale
  $\mathrm{dist}(y,C_v(r))\le\kappa(d_k(y)-r)_+$ de l'auditeur ; son calcul certifié par composante n'est pas étudié ici.
- **Q-d.** Les applications cellulaires entre ordres (S-Rhomb) au coût LiDAR : taille non mesurée ; E13 dit ce qu'on y
  gagnerait (des applications d'homotopie) et ce qu'on n'y gagnerait pas (aucun trou fermé n'y survit).
