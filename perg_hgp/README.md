# PERG-HGP

Le package expose deux objets distincts :

- `PowerCover3D`, hiérarchie spatiale (H_0) du champ K-ième distance de
  puissance, recommandée pour les données 3D ;
- `PERGHGPClusterer`, prototype historique d’atlas Rank-Gabriel, chargé
  paresseusement avec les dépendances optionnelles `atlas`.

## Installation

```bash
pip install -e .               # PowerCover CPU : NumPy + SciPy
pip install -e '.[atlas]'      # prototype historique
pip install -e '.[test]'       # suite complète
```

## Exemple CPU : régularisation à échelle locale

```python
import numpy as np
from perg_hgp import PowerCover3D, PowerCoverConfig

X = np.random.default_rng(0).normal(size=(2_000, 3)).astype("float32")

config = PowerCoverConfig(
    K=10,
    kappa=1.0,  # non utilisé comme cible en mode local_distortion
    m_reg=64,
    regularization_mode="local_distortion",
    distortion_budget_relative=0.08,
    distortion_budget_absolute=0.25,
    local_scale_k=10,
    min_resolved_radius=0.50,
    max_relative_spatial_error=0.50,
    max_grid_cells=5_000_000,
    grid_shape=(1, 1, 1),  # ignoré lorsque min_resolved_radius est fourni
    candidate_k_initial=10,
    candidate_k_max=512,  # ignoré par l'oracle de puissance CPU 4D
    neighbor_audit_queries=0,
    require_neighbor_audit=False,
)

hierarchy = PowerCover3D(config, backend="cpu").fit(X)
print(hierarchy.report.to_dict())
print(hierarchy.accuracy_at_scale(0.50))

inner_cells, outer_cells = hierarchy.components_at_cut(0.75)
coverage = hierarchy.site_components_at_cut(
    0.75, site_start=0, site_stop=2_000
)
print(coverage.summary())

# Partition plate conservatrice : seuls les sites confirmés et rattachés à
# une unique composante sont affectés; -1 désigne le bruit/l'indécision.
labels = hierarchy.labels_at_cut(0.75, min_cluster_size=20)
```

Le CPU construit le voisinage brut avec cKDTree, puis interroge le top-K de
puissance exactement pour les sites stockés grâce au relèvement
((z_i,a_i)\mapsto(z_i,\sqrt{a_i})\in\mathbb R^4). Il n’utilise donc ni le
plafond de candidats ni le garde RBC.

## Régularisation locale et garantie

Le mode `local_distortion` résout, sur le support dur de `m_reg` voisins :

\[
\max_{q\in\Delta} H(q)
\quad\text{sous}\quad
\sum_jq_j\lVert x_i-x_j\rVert^2\le B_i^2,
\]

avec

\[
B_i=\min(B_{\rm abs},\gamma R_K(x_i)).
\]

Il produit une perplexité locale (kappa_i=\exp H(q_i)), sans relancer tout le
pipeline avec une suite de `kappa` globaux. Le chemin CUDA utilise un RawKernel
fusionné par ligne : coûts en registres, 28 bissections utiles en float32 et
accumulation directe de (z_i,a_i,s_i,\kappa_i), sans tenseurs globaux de
probabilités ou d’offsets.

Le barycentre est sommé en coordonnées locales `x_j - x_i`, puis le poids
additif est fermé par l’identité de variance
`E||X_j-x_i||² = ||z_i-x_i||² + a_i`. Cette formulation évite la cancellation
float32 observée lorsque les rayons locaux deviennent très petits à 10--30 M.
Après promotion des sites stockés en float64, le ratio est recalculé sur tous
les points. Un dépassement du budget relatif arrête désormais le run au lieu
de publier une borne multiplicative invalide.

Pour les très rares sites dont le barycentre float32 reste hors budget après
stockage, le backend contracte explicitement le site de puissance vers le site
identité `z=x`; si nécessaire il replie le centre exactement sur `x` et arrondit
le poids additif vers le bas. Cette projection ne peut qu’abaisser la
distorsion. Son nombre, le nombre de replis identité et le facteur minimal sont
exposés dans `RegularizationDiagnostics` afin de ne pas confondre cette garde
numérique avec la solution de Gibbs non modifiée.

Dans le mode dur `fixed_kappa` avec `kappa=1`, la solution est analytiquement
`z_i=x_i, a_i=s_i=0` par la convention d’auto-voisinage. Le backend saute donc
entièrement l’index et les requêtes KNN de régularisation dans ce cas exact.

Le kernel fusionné ne recalcule pas un résidu de simplexe indépendant : son
champ `simplex_residual_max` vaut donc `null`. L’acceptation GPU vérifie la
parité du kernel avec l’oracle CPU sur plusieurs régimes, mais ne transforme
pas une mesure absente en zéro. Pour un cap absolu, le diagnostic
`distortion_budget_violation_max` enveloppe également l’erreur d’entrée
\(\varepsilon_X\).

Si `local_scale_k == K` et le ratio observé (gamma_{\rm obs}<1/2), la borne
multiplicative ((1\pm2\gamma_{\rm obs})) est publiée relativement au champ
K-NN du nuage quantifié. Sur CUDA elle reste conditionnelle à l’ordre RBC ;
\(\varepsilon_X\) est rapporté séparément vis-à-vis de l’entrée.

## Grille et niveau minimal résolu

Avec `min_resolved_radius=r_min`, la forme anisotrope est calculée pour garantir

\[
2H\le \eta r_{\min},
\qquad \eta=\texttt{max_relative_spatial_error}.
\]

L’étendue globale détermine alors le nombre de cellules, pas leur échelle. Si
ce nombre dépasse `max_grid_cells`, `GridResolutionBudgetError` arrête le run.
Le rapport compte aussi les sites dont (R_K(x_i)<r_{\min}) : ces niveaux de
forte densité ne sont pas résolus par cette grille.

Cette politique est une grille uniforme anisotrope pilotée par un plancher
local conservateur. Une vraie discrétisation locale exige des cellules
variables, des erreurs (H_Q+\delta_Q) et deux MSF distincts ; elle reste une
étape future.

## Sorties et appartenance des observations

`PowerCoverHierarchy` contient le champ, les forêts base/inner/outer, les
diagnostics, les bornes et le budget mémoire. `site_components_at_cut()` teste
la vraie intersection

\[
\operatorname{dist}^2(z_i,Q)\le r^2-a_i
\]

entre boule de puissance et cellule active. La relation CSR peut être
multivaluée. Les statuts signifient :

- `confirmed` : mêmes composantes après appariement inner→outer non ambigu ;
- `possible` : appartenance ou branche ambiguë entre les enveloppes ;
- `excluded` : aucune intersection même dans l’enveloppe extérieure.

Cette implémentation est une référence CPU bornée par `max_sites` et
`max_active_cells`, utilisable par tranches. Elle ne constitue pas encore le
kernel d’affectation 30 M.

`labels_at_cut(..., policy="confirmed_unique")` fournit une vue plate stricte
et déterministe de cette relation. Un site reçoit une étiquette uniquement si
son statut est `confirmed` et si sa relation extérieure contient exactement
une composante. Les relations multivaluées, `possible` et `excluded` deviennent
du bruit `-1`; les groupes plus petits que `min_cluster_size` sont ensuite
supprimés et les composantes restantes recodées en entiers contigus, par
identifiant de composante croissant.

Cette sortie est un **post-traitement conservateur**, pas une garantie de
partition plate ni une sélection par persistance : les garanties mathématiques
restent celles des inclusions H_0 inner/outer. Le calcul porte volontairement
sur tous les sites afin que `min_cluster_size` soit global et transmet les
mêmes limites CPU que `site_components_at_cut`. Il est donc impossible de
l’utiliser tel quel sur 30 millions de sites; cela exige le futur kernel GPU
de relation boule--AABB en deux passes.

## Routage CUDA explicite

Le voisinage brut utilise RBC seulement si celui-ci peut fournir tout le
support demandé ; sinon `CuvsBruteForceIndex` est choisi explicitement. Pour
les requêtes de puissance, RBC n’est retenu que s’il peut honorer le
`candidate_k_max` configuré plus un candidat de garde. Les petits nuages qui ne
satisfont pas cette capacité passent donc en brute force sans réduire le cap.
La capacité RBC est bornée par `min(floor(sqrt(N)), 1024)` avec RAPIDS 26.02 ;
le plus grand cap compatible avec le rang de garde est donc 1 023. Le rapport
enregistre le backend, le cap demandé, le cap effectif — limité uniquement par
\(N\) — et la raison du choix.

Le support KNN contient canoniquement l’observation elle-même. Si son identifiant
est absent mais qu’un doublon de coordonnées exact est présent, celui-ci sert
de représentant ; son identifiant est remplacé par celui de l’observation et
sa distance est forcée à zéro avant le calcul de \(R_K(x_i)\) et de Gibbs.
Aucun voisin seulement « presque égal » n’est accepté comme auto-voisin.

RBC est utilisé comme générateur de support : chaque requête sur-échantillonne
jusqu’à quatre fois le nombre de rangs demandé (dans les plafonds `sqrt(N)` et
1 024), recalcule les distances à partir des paires requête/identifiant, puis
rerange et tronque. Ce choix corrige une omission top-64 détectée à 10 M.

L’audit strict ne prend pas les distances `sqeuclidean` de cuVS pour autorité :
sur Blackwell, leur formulation en produits scalaires subit une cancellation
mesurable près de l’auto-voisin. Un balayage indépendant calcule directement
les différences en float64, par blocs de mémoire bornée, et fusionne ses
top-K. cuVS reste enregistré comme diagnostic. Comme tout audit échantillonné,
ce résultat constitue une preuve empirique sur les requêtes testées, pas une
preuve numérique universelle de RBC.

Le premier palier du garde de puissance vaut désormais 16 par défaut. Sur le
run local 10 M/K=10, les 884 736 centres ont tous été certifiés à ce palier :
le champ passe de 24,68 s à 5,56 s et le fit de 178,59 s à 159,23 s. Les
requêtes non séparées continuent d’escalader 16→32→… jusqu’au cap configuré ;
ce réglage change donc le coût, pas le critère de certification.

## Dtypes et sauvegarde

La géométrie de calcul normalisée est float32. Après calcul, les centres,
poids additifs, distorsions, rayons et forêts sont convertis en float64 dans
les unités d’entrée. Le run refuse les étendues dont les carrés de distances
ne sont pas représentables en float64, ainsi que tout poids positif qui
sous-déborderait lors de la dénormalisation.

`hierarchy.save(path, include_sites=...)` écrit le schéma 3 et un `run_id`
commun au JSON et à chaque NPZ. `PowerCoverHierarchy.load(path)` vérifie ce
`run_id`, les formes, les comptes, les décalages inner/outer et les valeurs
finies. Le loader migre aussi les artefacts schéma 2 antérieurs à la séparation
des deux termes d’erreur numérique. Un ancien `power_sites.npz` est supprimé lorsqu’une nouvelle
sauvegarde est demandée sans sites, ce qui empêche de réutiliser silencieusement
des observations d’un autre run.

## CUDA / Blackwell, 30 M et K=10

[Le notebook dédié](../PERG_HGP_Blackwell_30M.ipynb) :

- exécute tous les tests CPU ;
- vérifie Blackwell, NVRTC, CuPy, cuML, cuVS et RMM ;
- teste le solveur local CUDA fusionné et le MSF GPU ;
- audite RBC sur plusieurs géométries, y compris les ex æquo ;
- lance (N=30\,000\,000), (K=10), sans fallback global de `kappa` ;
- sauvegarde \(\varepsilon_X,s^\uparrow,H,\delta_{\rm num}\), temps, VRAM,
  RSS, histogrammes et fichiers d’échec ;
- renvoie `CONDITIONAL_PASS`, `INCONCLUSIVE` ou `FAIL`.

Une campagne G4 exécutée le 12 juillet 2026, ses résultats JSON/CSV légers et
les comparaisons HDBSCAN/HGP-CGAL sont documentés dans
[`benchmarks/results/G4_20260712_REPORT.md`](benchmarks/results/G4_20260712_REPORT.md).
Les gros nuages `.npy` et les logs restent ignorés par Git.

Voir [POWER_COVER_3D.md](POWER_COVER_3D.md) et les deux audits datés du
11 juillet 2026 pour les preuves, réserves et priorités restantes.
