# Contrat de `PowerCover3D`

## 1. Objet calculé

Pour chaque observation (x_i\in\mathbb R^3), un support dur de `m_reg`
voisins est construit. Une loi (q_i) produit :

\[
z_i=\sum_jq_{ij}x_j,
\qquad
a_i=\sum_jq_{ij}\lVert x_j-z_i\rVert^2,
\]

et la distance de puissance

\[
d_i(y)^2=\lVert y-z_i\rVert^2+a_i
=\sum_jq_{ij}\lVert y-x_j\rVert^2.
\]

Le champ est le K-ième rayon :

\[
g(y)=r_{K,\mathrm{reg}}(y)
=K\text{-ième plus petite valeur de }\{d_i(y)\}_i.
\]

Le backend construit une approximation cubique de la hiérarchie des composantes
connexes des sous-niveaux de (g). Il ne reconstruit pas le complexe HGP
combinatoire complet.

## 2. Deux régimes entropiques

### Cible de perplexité fixe

Le mode `fixed_kappa` résout :

\[
\min_{q\in\Delta}\sum_jq_j\lVert x_i-x_j\rVert^2,
\qquad H(q)\ge\log\kappa.
\]

Il reste disponible pour la reproductibilité. Les optima sur plusieurs minima
ex æquo peuvent avoir une entropie strictement supérieure à la cible ; ils ne
sont plus comptés comme une erreur de solveur.

### Budget de distorsion local

Le mode recommandé `local_distortion` inverse le problème :

\[
\max_{q\in\Delta}H(q),
\qquad
\sum_jq_j\lVert x_i-x_j\rVert^2\le B_i^2,
\]

avec

\[
B_i=\min(B_{\rm abs},\gamma R_L(x_i)),
\qquad L=\texttt{local_scale_k}.
\]

Si un seul des deux plafonds est configuré, il définit seul \(B_i\) ; le
contrat exige qu’au moins un budget absolu ou relatif soit fourni.

La solution reste une loi de Gibbs et fournit une perplexité locale
(kappa_i=\exp H(q_i)). Un budget nul donne une loi uniforme sur les minima
exacts ; un budget qui admet la loi uniforme utilise tout le support.

Sur CUDA, un noyau fusionné traite une ligne KNN par bloc, résout la température
en 28 bissections float32, puis accumule directement (z_i,a_i,s_i,kappa_i).
Il évite les grands tenseurs `Q × m_reg` de probabilités et
`Q × m_reg × 3` de voisins/offsets. Une passe float64 par chunks recalcule ensuite
une borne supérieure déclarée de (s_i) pour les sites stockés.

Ce noyau ne publie pas de résidu de simplexe indépendant. Pour
`solver_backend="cuda_fused_budget_v1"`, `simplex_residual_max` vaut donc
`null`; seul le solveur vectorisé CPU publie cette mesure. Les smoke tests GPU
comparent le résultat fusionné à l’oracle CPU, mais une valeur absente n’est
jamais interprétée comme un résidu nul.

## 3. Garantie locale de régularisation

Posons (R(y)=r_K(y)), (D_i(y)=\lVert y-x_i\rVert) et
(s_i=d_i(x_i)). On a :

\[
|d_i(y)-D_i(y)|\le s_i.
\]

Si `local_scale_k == K` et

\[
s_i\le\gamma R(x_i),\qquad 0\le\gamma<\tfrac12,
\]

alors, pour tout (y),

\[
\boxed{(1-2\gamma)R(y)\le g(y)\le(1+2\gamma)R(y).}
\]

Pour la borne supérieure, les K voisins de (y) satisfont
(R(x_i)\le R(y)+D_i(y)\le2R(y)). Pour la borne inférieure,
(d_i(y)<(1-2\gamma)R(y)) implique (D_i(y)<R(y)), ce qui concerne au plus
K−1 sites.

Le rapport utilise le ratio maximal observé et arrondi vers l’extérieur, pas
seulement le (gamma) demandé. Sur CUDA, cette garantie est conditionnelle à
l’ordre KNN RBC et à l’enveloppe flottante déclarée. Si une échelle (R_K=0)
porte une distorsion positive, le statut devient `unbounded_zero_scale` et
aucune borne multiplicative n’est publiée.

La borne additive historique reste toujours disponible :

\[
|g(y)-r_K(y)|\le s^\uparrow,
\qquad s^\uparrow\ge\max_i s_i.
\]

## 4. Quantification de l’entrée

La géométrie est recentrée, normalisée isotropiquement puis convertie en
float32. Le rapport mesure et enveloppe :

\[
\varepsilon_X=\max_i\lVert x_i-\widetilde x_i\rVert.
\]

Les entiers hors de ([-2^{53},2^{53}]) sont refusés, car le contrat public
float64 ne pourrait pas représenter exactement l’entrée. `epsilon_X` est
séparé de la distorsion entropique : il ne doit jamais être absorbé dans un
diagnostic Gibbs.

En revanche, lorsqu’un budget absolu est demandé, le diagnostic opérationnel
`distortion_budget_violation_max` enveloppe

\[
\max(0,s^\uparrow+\varepsilon_X-B_{\rm abs}),
\]

car le contrat doit porter sur les observations d’entrée et non seulement sur
leur copie quantifiée.

Le calcul normalisé est float32, puis les centres, poids additifs, distorsions,
rayons et forêts publics sont promus en float64 dans les unités d’entrée. Les
étendues plus petites que la racine du plus petit float64 positif, ou plus
grandes que la racine du plus grand float64 fini, sont refusées : leurs poids
de puissance carrés ne seraient pas représentables. Un poids positif qui
sous-déborderait à la dénormalisation provoque également un arrêt demandant de
changer les unités.

## 5. Oracle top-K de puissance

### CPU exact

L’identité

\[
d_i(y)=\left\|(y,0)-(z_i,\sqrt{a_i})\right\|_{\mathbb R^4}
\]

permet un cKDTree exact en dimension 4 pour les sites stockés. Ce chemin gère
les ex æquo sans plafond de rang euclidien 3D.

### CUDA conditionnel

RAPIDS RBC renvoie des candidats euclidiens 3D. Pour le premier candidat omis :

\[
\min_{i\ \mathrm{omis}}d_i(y)^2
\ge d_{\rm guard}(y)^2+\min_i a_i.
\]

Le nombre de candidats double jusqu’à séparation du K-ième pouvoir. Au plafond,
un chevauchement limité aux enveloppes numériques peut être accepté comme un
intervalle étroit sur la **valeur** du rang K ; cela traite les grandes
multiplicités exactes. Si la borne omise est matériellement plus basse, la
requête reste incertaine et le mode strict lève
`PowerKNNCertificationError`.

Les champs `conditional_guard_*` ne sont pas une preuve RBC exhaustive. Le
notebook audite des requêtes contre cuVS brute force, mais cet échantillon ne
certifie pas les requêtes restantes.

### Routage par taille

Le backend n’autorise aucun changement d’algorithme caché et ne réduit pas le
cap configuré pour satisfaire RBC :

- pour le support de régularisation, RBC est retenu seulement s’il peut fournir
  la totalité du support demandé ; sinon cuVS brute force est choisi ;
- pour le champ de puissance, RBC doit pouvoir honorer
  `candidate_k_max` — limité seulement par \(N\) — plus le candidat de garde ;
- cette capacité doit respecter à la fois \(\lfloor\sqrt N\rfloor\) et la
  frontière `k <= 1024` du kernel RBC RAPIDS 26.02 ;
- si l’une de ces limites est dépassée, l’index cuVS exhaustif est utilisé avec
  le même cap.

Le plan effectif (`power_candidate_plan`) et le statut des deux index sont
conservés dans `neighbor_audits`. Pour le run 30 M, le cap 1 023 et son rang de
garde 1 024 tiennent dans la capacité RBC ; avec les mêmes défauts, les petits nuages
passent explicitement en brute force. L’exhaustivité algorithmique de cuVS ne
constitue toujours pas une preuve d’arrondi dirigé.

Avant Gibbs, l’identifiant propre est recherché dans la réponse KNN. S’il est
absent, un point de coordonnées strictement identiques peut servir de
représentant ; il est canonisé sous l’identifiant propre avec distance nulle.
En l’absence de l’un ou de l’autre, le run échoue. Cette règle corrige les
petites distances propres positives observées avec certaines piles RBC sans
accepter un voisin seulement proche.

## 6. Grille pilotée par un rayon local minimal

Pour un cube fermé (Q), de centre (c_Q), demi-diagonale (H), et une marge
numérique (delta_{\rm num}) :

\[
g(c_Q)-H-\delta_{\rm num}\le g(y)
\le g(c_Q)+H+\delta_{\rm num}.
\]

Avec `min_resolved_radius=r_min`, la forme anisotrope est dérivée pour imposer :

\[
2H\le
\texttt{max_relative_spatial_error}\times r_{\min}.
\]

La taille physique des cellules dépend donc du niveau local minimal que
l’utilisateur veut résoudre ; l’étendue globale ne sert qu’à compter les
cellules. Le run refuse :

- plus de `max_grid_cells` ;
- une forme dont les centres consécutifs ne sont pas représentables sous le
  contrat float32 portable CPU/CUDA.

Le rapport publie la fraction des sites ayant (R_K(x_i)<r_{\min}). Les
niveaux correspondants ne sont pas résolus par ce plancher.

Cette grille reste uniforme anisotrope. Elle ne fournit pas encore les erreurs
locales (H_Q,\delta_Q) d’un octree et ne doit pas être appelée adaptative. Une
grille variable imposera des activations par cellule et deux MSF distincts,
car inner et outer ne seront plus de simples translations.

## 7. Deux contrats d’entrelacement

Pour toutes les requêtes enveloppées :

\[
\varepsilon_{\rm base}
=\varepsilon_X+s^\uparrow+H+\delta_{\rm num},
\]

\[
\varepsilon_{\rm env}
=\varepsilon_X+s^\uparrow+2(H+\delta_{\rm num}).
\]

La première borne concerne `base_forest`. La seconde conserve les inclusions
inner/continu/outer au même seuil. Elles portent sur le rayon et sur (H_0),
pas sur une erreur additive de densité ni sur des labels plats.

Les cubes fermés utilisent la 26-connexité. Une arête ((u,v)) naît à
(max(g_u,g_v)) ; un MSF conserve toutes les composantes de chaque sous-niveau.

## 8. Appartenance observation–composante

À un seuil (r), le site (i) appartient à une composante cubique si sa boule

\[
B_i(r)=\{y:\lVert y-z_i\rVert^2+a_i\le r^2\}
\]

intersecte une de ses cellules. Le prédicat exact boule–AABB est :

\[
\operatorname{dist}^2(z_i,Q)\le r^2-a_i.
\]

`site_components_at_cut()` calcule cette relation CSR sur CPU par tranches et
compare inner/outer. `confirmed` exige un appariement inner→outer non ambigu ;
une relation confirmée peut néanmoins contenir plusieurs composantes, ce qui
est légitime pour (K\ge2).

Ce chemin est limité par `max_sites`, `max_total_cells`, `max_active_cells` et
`max_candidate_cells_per_site`. Le kernel GPU deux passes, le merge tree
enrichi et la sélection plate restent à implémenter pour 30 M.

## 9. Mémoire et validation 30 M

L’estimateur inclut les tableaux persistants, les chunks KNN/Gibbs, les
candidats de puissance et le MSF implicite. Il inclut désormais
(kappa_i) et les échelles locales temporaires. Il ne mesure pas exactement
les workspaces RAPIDS/RMM/CUB, la compilation NVRTC ni la fragmentation.

Le notebook `PERG_HGP_Blackwell_30M.ipynb` fixe (N=30\,000\,000), (K=10),
`m_reg=64`, vérifie le noyau local fusionné et le MSF, collecte VRAM/RSS/temps,
et sauvegarde les deux bornes. Son succès reste `CONDITIONAL_PASS` tant que
RBC est seulement audité par échantillon.

Sans artefacts exécutés et versionnés, le dépôt ne revendique ni débit 30 M,
ni validation Blackwell, ni amélioration SemanticKITTI.

## 10. Sérialisation cohérente

Le schéma 2 de `PowerCoverHierarchy.save()` génère un `run_id` aléatoire par
sauvegarde et l’inscrit dans `run_report.json`, `cubical_hierarchy.npz` et,
s’il est demandé, `power_sites.npz`. `load()` exige l’égalité de ces identifiants
et vérifie également manifeste, formes, comptes de requêtes, décalages,
quantification et finitude. Il refuse ainsi un répertoire contenant des
artefacts issus de deux générations différentes.

Une sauvegarde sans sites retire, après publication du nouveau manifeste, un
ancien `power_sites.npz` éventuellement présent. Il s’agit d’une reprise de
résultat complète après `fit`, pas encore d’un checkpoint intermédiaire des
étages index, régularisation, champ et MSF.
