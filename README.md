# E-HGP / PERG-HGP

Ce dépôt regroupe les implémentations historiques HGP/E-HGP, le prototype
d’atlas Rank-Gabriel `PERGHGPClusterer` et le backend spatial `PowerCover3D`
destiné aux nuages 3D massifs.

## État au 11 juillet 2026

`PowerCover3D` calcule une approximation contrôlée de la hiérarchie (H_0) du
champ K-NN régularisé. Le chemin CPU utilise désormais le relèvement euclidien
exact en dimension 4 pour les distances de puissance. Le chemin CUDA conserve
RAPIDS RBC : son garde est conditionnel à l’ordre euclidien renvoyé et aux
enveloppes flottantes déclarées, avec audits contre cuVS brute force.

Le routage CUDA est explicite et ne réduit jamais silencieusement la
configuration : RBC est utilisé seulement s’il peut honorer le support demandé
ou, pour le champ de puissance, `candidate_k_max` plus le candidat de garde.
Sinon cuVS brute force est sélectionné avec le même cap. Avant la
régularisation, l’observation elle-même — ou un doublon de coordonnées exact —
est canonisée comme atome de coût nul, y compris si RBC a renvoyé une petite
distance positive.

Le statut dépend de la taille :

| Régime | Chemin recommandé | Statut |
|---|---|---|
| Quelques milliers à quelques centaines de milliers | CPU, cKDTree 3D + relèvement exact 4D | Testé localement |
| Tailles intermédiaires | CPU exact si la grille reste abordable ; CUDA explicite sinon | Dépend du matériel |
| Jusqu’à 30 000 000, (K=10) | CUDA/RAPIDS + noyaux CuPy fusionnés | Protocole Blackwell prêt, exécution non versionnée |

Le nouveau mode `local_distortion` maximise l’entropie sous

\[
s_i\le \min(B_{\rm abs},\gamma R_K(x_i)).
\]

Si `local_scale_k == K`, si le ratio observé vérifie \(\gamma<1/2\), et
conditionnellement au voisinage calculé, alors sur le nuage quantifié :

\[
(1-2\gamma)r_K(y)\le r_{K,\mathrm{reg}}(y)
\le(1+2\gamma)r_K(y).
\]

L’erreur de quantification d’entrée \(\varepsilon_X\) reste additive. Pour une
grille uniforme de demi-diagonale \(H\) et une marge numérique
\(\delta_{\rm num}\), les contrats publiés sont :

- forêt de base : \(\varepsilon_X+s^\uparrow+H+\delta_{\rm num}\) ;
- enveloppes inner/outer :
  \(\varepsilon_X+s^\uparrow+2(H+\delta_{\rm num})\).

La grille peut être dimensionnée par `min_resolved_radius` plutôt que par un
nombre global arbitraire de cellules. Elle est uniforme anisotrope, refuse tout
dépassement de `max_grid_cells` et ne doit pas être décrite comme adaptative :
un véritable octree avec erreurs \(H_Q,\delta_Q\) reste à construire.

Les calculs normalisés restent en float32 sur CUDA, mais centres, poids de
puissance, rayons publics et forêts sont promus en float64 lors du retour aux
unités d’entrée. Les étendues dont le carré sous-déborde ou déborde le contrat
float64 sont refusées avec une demande explicite de changement d’unités. Si un
budget absolu est demandé, `distortion_budget_violation_max` inclut aussi
\(\varepsilon_X\). Sur le solveur CUDA fusionné, le résidu de simplexe non
recalculé vaut `null`, jamais un faux zéro probant.

`PowerCoverHierarchy.save()` et `load()` utilisent un schéma versionné. Chaque
sauvegarde reçoit un `run_id` commun au manifeste et aux NPZ ; un mélange
d’artefacts provenant de deux générations est rejeté au chargement.

## Installation

Le chemin puissance CPU n’impose plus PyTorch :

```bash
pip install -e perg_hgp
```

Pour l’atlas historique et tous les tests :

```bash
pip install -e 'perg_hgp[test]'
```

La pile CUDA (CuPy, cuML, cuVS, RMM) est installée par le notebook avec la
procédure RAPIDS adaptée à Colab.

## Validation CPU

```bash
PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 python -m pytest -q perg_hgp/tests
```

La campagne locale de cette révision compte 124 tests `perg_hgp` réussis et
1 test E-HGP réussi. Le notebook GPU réexécute toute la suite `perg_hgp` avec
CUDA masqué avant tout benchmark.

## Documents importants

- [contrat PowerCover3D](perg_hgp/POWER_COVER_3D.md) ;
- [README du package](perg_hgp/README.md) ;
- [audit PERG-HGP du 11 juillet](perg_hgp/Rapport_audit_PERG_HGP_2026-07-11.md) ;
- [audit PowerCover3D 30 M / K=10](perg_hgp/Rapport_audit_PowerCover3D_30M_K10_SemanticKITTI_2026-07-11.md) ;
- [notebook Blackwell 30 M](PERG_HGP_Blackwell_30M.ipynb).

## Limites encore ouvertes

- aucune mesure Blackwell/30 M n’est incluse tant que le notebook n’a pas été
  réellement exécuté et ses JSON versionnés ;
- RBC CUDA n’est pas une preuve exhaustive, malgré les audits échantillonnés ;
- l’appartenance canonique boule–composante existe comme référence CPU
  découpable, mais pas encore comme kernel GPU 30 M ;
- il manque encore un merge tree enrichi, une reprise par checkpoints et une
  sélection plate explicitement séparée de la hiérarchie de Hartigan ;
- (K=10) est un choix à taille finie, pas une preuve de consistance ni un
  résultat SemanticKITTI.
