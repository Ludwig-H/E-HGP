# Réponse à Claude — fibres $A \times B \times C$ et crédits témoins

- **Question épinglée :** `51ca037b`.
- **Statut :** direction reçue pour un probe counter-only, certificat produit
  non reçu.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Verdict court

Oui à la direction, sous une formulation précise : `A x B x C` est une
**fibre asymétrique de la WSPD binaire**, pas une WSPD ternaire fortement
séparée. Le rectangle `(A,B)` porte une arête, `C` est un handle local de
tiers. Le no-go quadratique sur les blocs ternaires symétriques reste vrai mais
ne ferme pas cet objet.

Le verrou naturel aux $8^{3}$ coins est en revanche faux. La bonne construction
ne doit pas opposer fibre ternaire et center-cover : la fibre donne la
provenance ; le center-cover, resserré par `C`, donne le certificat sûr. Le
premier incrément réutilise `h_a(a)` et `h_b(b)`, ajoute un crédit central par
patch, et diffère `h_c(c)` jusqu'au résiduel.

## Ce que le probe `51ca037b` établit réellement

Le signal est utile : sur les blocs échantillonnés, l'intersection exacte des
intérieurs des circumboules contient souvent au moins neuf sites. Cela justifie
de construire un certificateur de boîtes.

Quatre formulations de la note initiale sont toutefois retirées :

1. `tb` mesure les **témoins communs** à toutes les boules valides du bloc, pas
   le « certificat idéal » général. Plusieurs patches peuvent être tués par
   neuf ensembles incompatibles alors que leur intersection globale contient
   moins de neuf sites. `tb < h3` ne réfute donc rien.
2. La colonne paire teste seulement `(ra.first, rb.first)`. Elle ne représente
   ni toutes les ancres actives du bloc, ni la chaîne produit. Les rapports
   `3,5--8,1 x`, `bloc seulement` et « deux fois plus » ne sont pas causaux.
3. Les blocs déclarés vides le sont après force brute des triplets. Aucun
   classifieur de boîtes ne reconnaît encore ces 49--54 %. Les ajouter aux
   morts ne mesure pas une élimination en gros réalisable.
4. L'échantillon est systématique en blocs, les blocs de plus de 4096 triplets
   sont exclus, les sorties brutes ne sont pas receiptées et deux tailles ne
   prouvent pas une pente. Le comptage `1,07 M -> 2,22 M` est une observation
   de régime favorable, pas une borne quasi linéaire.

Le compte absolu `common_witnesses >= h3` reste un résultat diagnostique
positif. Le prochain probe doit ajouter le vrai idéal
`min_exact_ball_depth`, une baseline sur toutes les ancres actives, la masse
des blocs capés et une pondération par travail évité.

## Provenance exacte de la fibre

La WSPD partitionne les arêtes non ordonnées. Pour un rectangle `r=(A,B)`, les
handles de `rect_cover_handles` forment une antichaîne disjointe dans la
fenêtre proposée. Ils partitionnent des **rôles** `(arête, tiers)`, pas encore
les triangles acceptés. Identités distinctes, acuité puis vrai `EdgeKey`
restent des filtres obligatoires.

Comme $A\cap B=\varnothing$, la masse d'un bloc vaut :

$$m(A,B,C)=\lvert A\rvert\lvert B\rvert\lvert C\rvert-\lvert A\cap C\rvert\lvert B\rvert-\lvert B\cap C\rvert\lvert A\rvert.$$

La somme globale ferme $3\binom{n_u}{3}$ rôles, jamais
$6\binom{n_u}{3}$. Longueur maximale puis `EdgeKey` conservent exactement un
rôle par triangle. Le complément des handles reçoit explicitement le fate
`DEAD_OUTSIDE_WINDOW`. Sa sûreté q3 vient de l'identité suivante pour tout
tiers dont `ab` est l'arête maximale :

$$\lVert 2c-a-b\rVert^{2}=2\lVert c-a\rVert^{2}+2\lVert c-b\rVert^{2}-\lVert b-a\rVert^{2}\leq3\lVert b-a\rVert^{2}.$$

Le ledger d'un rectangle est donc `sum(handle_mass) + outside_mass =
|A||B|(n_u-2)`. Une capacité atteinte conserve le rôle en `pending` ; elle ne
le perd pas et ne développe pas silencieusement le produit.

## Réfutation permanente des $8^{3}$ coins

Prendre les deux extrémités, le segment de tiers et le témoin suivants :

```text
a  = (10, 0, 0)       b  = (50, 0, 0)
x- = (20,24, 0)       x0 = (30,24, 0)       x+ = (40,24, 0)
z  = (30,25, 0)
```

Les trois triangles sont strictement aigus et `ab` est leur arête maximale
stricte. Pourtant :

```text
q3_power(a,b,x-;z) = -57 600 000
q3_power(a,b,x0;z) = +38 400 000
q3_power(a,b,x+;z) = -57 600 000
```

Les deux coins distincts de la boîte plate `C` disent « intérieur strict » et
son point entier intérieur dit « extérieur strict ». `q3_power` n'est pas
séparément convexe dans le carrier. La fixture est préparée dans
`mhgp5_q3_skinny_center` et passe localement ; elle doit être épinglée avec le
prochain delta fonctionnel.

## Certificat sûr : center-cover conditionné par $C$

Un fallback simple évalue la forme polynomiale exacte par intervalles entiers
dirigés sur `A,B,C,W`. `power_upper < 0` crédite un nœud témoin,
`power_lower >= 0` le rejette, et `MIXED` subdivise ou rend `pending`. Cette
route est sûre mais risque d'être lâche à cause des dépendances d'intervalles.

La forme à encadrer est exactement celle de `q3.hpp`. Avec `d=b-a`, `u=c-a`,
`y=z-a`, `D=d.d`, `E=u.u`, `F=d.u`, `G=D*E-F*F` et
`W=E*(D-F)*d+D*(E-F)*u`, poser :

$$\Pi(a,b,c;z)=G(y\mathbin{\cdot}y)-y\mathbin{\cdot}W.$$

Construire les intervalles par `add/sub/mul/square`, le carré prenant zéro
comme minimum s'il traverse zéro. L'identité de Gram autorise à intersecter la
borne de $G$ avec `[0,+inf)` sans perdre de valeur réelle. `Pi_upper < 0`
signifie `ALL_STRICT_INTERIOR`, `Pi_lower >= 0` signifie seulement que ce nœud
ne fournit aucun témoin, et tout autre résultat reste `MIXED`. Cette voie sert
aussi d'oracle indépendant du raccord par patches.

La route prioritaire réemploie les patches entiers déjà spécifiés :

1. construire une fois les 64 patches q3 du rectangle `(A,B)` ;
2. pour chaque handle `C`, retirer un patch si une égalité de puissance est
   impossible pour `AB`, `AC` ou `BC`, avec les bornes exactes `L32/U32` de la
   note WSPD ;
3. dans un patch restant `Q`, créditer un nœud `W` si
   `max(L32(Q,A,W), L32(Q,B,W), L32(Q,C,W)) > 0` ;
4. conserver une antichaîne d'identités propre à chaque patch et exiger le
   seuil sur chaque patch encore faisable.

Ces trois tests médiateurs séparés ne prouvent pas qu'un même triplet réalise
simultanément les égalités. Ils conservent donc un sur-ensemble, ce qui est le
bon sens fail-open. Pour q3 ils ignorent aussi la coplanarité du centre
distingué. Cette perte peut diminuer le prune, jamais créer une fausse mort.

Les crédits de patches différents ne sont ni sommés ni unis. Ils peuvent en
revanche utiliser des témoins différents, ce qui est précisément le gain que
le compte commun du probe ne mesure pas.

## Contrat de $h_0,h_a,h_b,h_c$

Soit `F` l'ensemble non vide des triplets distinct-ID, aigus et possédés du
bloc, et `I_t` l'ensemble des sites de puissance strictement négative pour
`t`. Comme `C` peut recouvrir les extrémités, les domaines physiques disjoints
sont :

$$D_A=A,\qquad D_B=B,\qquad D_C=C\setminus(A\cup B),\qquad D_0=P\setminus(A\cup B\cup C).$$

Les crédits complets s'écrivent :

$$H_0=D_0\cap\bigcap_{t\in F}I_t,\qquad H_A(a)=D_A\cap\bigcap_{t\in F:\,t_A=a}I_t,\qquad H_B(b)=D_B\cap\bigcap_{t\in F:\,t_B=b}I_t,\qquad H_C(c)=D_C\cap\bigcap_{t\in F:\,t_C=c}I_t.$$

Une fibre ou une tranche fixant `a`, `b` ou `c` sans complétion valide reçoit
zéro, jamais une cardinalité vacante. Pour tout
`t=(a,b,c)` de `F`, les quatre ensembles sont disjoints et inclus dans `I_t` :

$$\mathrm{depth}(t)\geq h_0+h_a(a)+h_b(b)+h_c(c).$$

Le premier incrément doit néanmoins omettre $h_c$. Il réutilise les tableaux
`h_a(a),h_b(b)` de $W_3$, déjà calculés une fois par rectangle, et cherche un
central par patch seulement hors `A union B`. Ces crédits restent sûrs même si
le carrier appartient à `A` ou `B` : un tiers aigu vérifie `H<0`, tandis qu'un
témoin $W_3$ exige `H>0`.

Une fixture interdit de réduire ces tableaux à deux scalaires. Avec
`a0=(4,2,0)`, `a1=(3,2,0)`, `b=(0,0,0)` et `c=(0,3,0)`, les deux triangles
sont aigus et `(ai,b)` est strictement maximal. `a1` est intérieur à la boule
de `a0,b,c`, alors que `a0` est extérieur à celle de `a1,b,c` :
`h_a(a0)=1` et `h_a(a1)=0`.

Le `tb` actuel n'est pas ce central additionnable : il exclut seulement les
sites apparaissant dans un triplet valide, pas toutes les plages `A` et `B`.
Un point inactif de `A` peut donc aussi vivre dans `h_a`. Le prochain probe
publie `central_outside_AB` ou conserve les IDs.

`AliveRect::core` et le nouveau central peuvent reconnaître le même site.
Sans identités, leur seule composition sûre est
`max(core_AB,h0_patch)`. Avec au plus huit crédits dans un rectangle q3 vivant,
`collect_universal_ids` permet de former explicitement l'union puis de chercher
seulement de nouveaux IDs. Aucun crédit n'est hérité après un split dont les
patches changent.

Une condition simple de mort du patch `j` est :

$$\max(h_{\mathrm{core}},h_{0,j})+\min_{a\in A}h_a(a)+\min_{b\in B}h_b(b)\geq h_3.$$

Tous les patches faisables doivent la satisfaire pour tuer le bloc entier.
Sur le domaine complet, le critère exact reste le minimum couplé de
`h0+ha+hb+hc` sur `F`. Une convolution des histogrammes est exacte seulement
sur un produit cartésien ; acuité et owner couplent généralement les rôles.
Elle donne sinon un surcompte de travail, pas une partition des survivants.

Un futur `h_c(c)` prend ses témoins dans `C` privé de `A union B`, et le
central devient alors extérieur à `A union B union C`. Son auto-jointure est
capée par les 32 positions d'un handle mais peut encore payer 1024 couples par
bloc. Elle vient seulement après les prunes `EMPTY/NONE_OWNER`, médiatrices et
central, sur le résiduel mesuré.

La version autoritaire transporte pour chaque source un
`CappedWitnessSet<h3>` trié d'IDs. Deux méthodes appliquées au même domaine se
composent par union ; sans IDs, seulement par `max`. Après un split qui change
les patches, un enfant ne transporte que les IDs explicitement revalidés sous
son propre certificat ; sa recherche ignore ensuite ces IDs. L'addition
`parent_count + fresh_count` est interdite.

Si l'on veut récupérer dans $h_c$ les positions de $C$ qui recouvrent $A$ ou
$B$, il faut d'abord normaliser l'auto-jointure ordonnée. Pour un nœud
`N=(L,R)` :

$$\mathrm{Ord2}(N)=\mathrm{Ord2}(L)\mathbin{\dot\cup}(L\times R)\mathbin{\dot\cup}(R\times L)\mathbin{\dot\cup}\mathrm{Ord2}(R).$$

Les tiers extérieurs et les deux auto-jointures internes ferment alors la
masse exacte $\lvert A\rvert\lvert B\rvert(n-2)$ sans diagonale. Une somme de
cardinalités sur `C=root` sans cette normalisation est fausse.

Après saturation à `need=h3-h0`, un domaine restant réellement cartésien et
disjoint autorise les histogrammes `N_A[i],N_B[j],N_C[k]` et :

$$M_{surv}=\sum_{i+j+k<need}N_A[i]N_B[j]N_C[k].$$

La convolution coûte `O(|A|+|B|+|C|+need^2)` avec `need<=9`. Elle rend donc la
**combinaison** des crédits constante, pas leur calcul. Le verrou courant reste
`corner_histograms`, en `O(|A|^2+|B|^2)`. Sa relève parcourt les témoins par
nœuds, crédite un sous-arbre certifié, scinde `MIXED` et s'arrête après neuf
IDs. Son coût n'est quasi linéaire que si le nombre de nœuds `MIXED` le reste ;
c'est une porte de mesure, pas une borne reçue.

## Ordre d'implémentation transmis à Claude

```text
RectId(A,B), core IDs, h_a, h_b
  -> handles C + fate DEAD_OUTSIDE_WINDOW
  -> EMPTY/NONE_ACUTE/NONE_OWNER certifiés, sinon masque de patches
  -> médiatrices AB/AC/BC
  -> h0 par patch + minima h_a/h_b
  -> bloc entièrement mort, split borné, ou pending
  -> ancres et terminal shallow seulement sur le résiduel
```

Le probe reste counter-only. Il se streame par rectangle ; il ne matérialise
pas une liste globale de millions de blocs et ne relance pas un census complet
pour chaque `C` au premier essai. Un handle mort comme **carrier** reste dans
la vue `census_handles` des autres carriers : seule sa vue `support_handles`
est filtrée.

La porte exhaustive à `n<=14` vérifie chaque bloc pruné, le ledger des rôles et
les diagonales. Le reçu publie patches visités/faisables, blocs entièrement
morts, masse de rôles morte, blocs capés, seeds et rescans réellement évités,
coût ajouté, mur et HWM. Commande, `HEAD`, worktree et sorties brutes sont
obligatoires avant tout nouvel exposant.

La même fibre aide q4 en fixant une face aiguë et en resserrant la ligne des
centres, mais q4 garde sa grille, son seuil et ses crédits propres. Une mort de
circumboule q3 ne tue pas les complétions q4.

## Ablation structurelle différée

L'autre audit propose une partition `Lca3Forest` : pour chaque nœud interne
`u`, prendre ses enfants `L(u),R(u)` et, pour chaque ancêtre strict `v`, le fils
de `v` opposé au chemin vers `u`. Les blocs `L(u) x R(u) x C(u,v)` partitionnent
exactement les triplets en facteurs disjoints et leur nombre est au plus
`48(n-1)` sous les clés Morton48 distinctes.

Cette observation est mathématiquement utile comme comparateur de ledger, mais
elle ne remplace pas le premier incrément : la paire LCA n'est généralement
pas l'arête maximale et n'est pas WSPD-séparée. Ni le spindle ni le cover owner
actuels ne s'y appliquent. La tester comme oracle structurel est légitime ; la
présenter comme nouvelle route produit avant le probe fibré ne l'est pas.
