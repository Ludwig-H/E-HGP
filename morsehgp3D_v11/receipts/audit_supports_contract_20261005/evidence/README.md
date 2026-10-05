# Suivi L0/S0+S1 — sources 5ad, demande 9290

Revue favorable des corrections adoptées. Aucun nouveau défaut matériel trouvé sur ce périmètre. Sources Git `5adf6a59f3d3b99fdf947e676e8548d4102ab48f`, document complémentaire seul `9290cf3bfe51147ede73d178dce8bf62e31c82d8`. Captures avant/après : aucune source capturée n'a changé. Aucun produit, acteur, audit ou reçu antérieur n'a été modifié.

[SORTIES.md](sources/git/morsehgp3D_v11/docs/SORTIES.md),416–437, reprend exactement le flux d'octets V2 proposé : namespace B/K/XYZ sans PointId, structure canonique, naissance SiteIdx àK1/S* sinon. SP peut recalculer cette empreinte ; FUL1 seul ne publie pas S*. Le SHA de l'artefact et la signature restent distincts. Les incidences (b,F) sont nommées à410, l'état `published_complete` à498–503, et l'invariance est correctement nuancée à514–526. C'est un contrat, pas une qualification de l'écrivain, du lecteur ou de la façade native.

La [consolidation](sources/reports/l0_consolidation.md),152–172, est recoupée avec les pièces fermées : les quatre logs déclarés Python3.10/3.12 normal/−O sont identiques ; 50faits, 210nuages,951ordres,15062boules,16943supports,12441nœuds,48074coupes. L'oracle publié a SHA826f3f94ce9eeb66e6ed04edce8266dd70b52498fd929c2ec7f7dbc01538a541. Les 13patches de mutants sont uniques/applicables à ce source et les logs portent leurs causes et verdicts nommés. Le CTest conservé comporte90Passed distincts, dont30portes S1 : porte principale/refus et leurs jumelles,26mutants normal/−O. Les28payloads du SHA256SUMS du rapport ont été rehachés et sont copiés ici, inventaire interne compris.

[check_l0_s1.py](check_l0_s1.py) : **208 gardes bornées**, normal/−O identiques, codes0, stderr vide. Il joue uniquement les fonctions AST exactes `floors` et `run_mutant` avec des métadonnées/faux calculs, puis lit sources et reçus. Les compteurs corrompus, planchers, digest faux, table vide, témoin intact en échec, patch périmé, survivant et mauvaise cause sont rejetés selon leurs codes. Il ne calcule aucun nuage, MEB ou hiérarchie. Premier échec de notre parseur privé conservé dans [reader_preflight](reader_preflight/README.md) : regexp sans chiffres, omettant arity2/3/4 ; corrigé avant fermeture, aucun défaut du produit.

[Demande §E](sources/git_delta/morsehgp3D_v11/audits/REPONSE_CLAUDE_SUPPORTS_20261004.md),166–178 : la matrice proposée est pertinente **sur une source native intégrée et figée**. [MATRIX_ADVICE.json](MATRIX_ADVICE.json) conserve les13noms/fixtures/causes et les portées exactes : GCC Release u21/u24 distincts ; ASan/UBSan et TSan pour les configurations effectivement jouées ; mutants tower/supports/io/api/cli causaux ; scale diagnostique et trames entières K5 séparées ; frontière coquille24admise/25refusée. u18 reste un profil explicite à requalifier séparément pour les nouveaux ports, sans transfert des anciennes fondations.

Les lacunes déclarées restent importantes avant L1 : suite permanente S1 limitée àK≤5 et coquille≤12, pas d'écrivain/lecteur natif qualifié. La confrontation indépendante high-K du rapport [v_oracle](sources/reports/l0_verif_oracle.md),32–41, reste distincte d'une porte permanente. Pour le futur oracle long K≤3/coquille24, remplacer seulement le parcours2^m deN_j ne borne pas `_minimal_nonseparable` (supports.py:241–267), qui étend les sous-parties séparables jusqu'àm : conserver budget/label long et toute censure, calculer lesN_j nécessaires jusqu'àK+1 sans tronquer la famille Q_b. Aucun coût natif ou contrat100ms n'est établi par la revue.

Rejeu autonome, bibliothèque standard seulement :

```sh
python3 -B -S check_l0_s1.py
python3 -B -O -S check_l0_s1.py
```

[BILAN.json](BILAN.json) et [RUNS.json](RUNS.json) donnent les comptes, sorties et limites. L'inventaire racine exclut uniquement son propre fichier `SHA256SUMS`, pas les inventaires des sous-dossiers. Le ledger exclut seulement lui-même et l'inventaire racine.
