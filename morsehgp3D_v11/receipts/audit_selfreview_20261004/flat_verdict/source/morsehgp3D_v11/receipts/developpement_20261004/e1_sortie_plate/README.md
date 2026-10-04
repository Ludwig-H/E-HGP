# Reçu E1 — sortie plate : export, étude LiDAR, qualification native, dev synthétique

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only (synthétique) ; grille de 1 mm (LiDAR)
public_status=not_claimed
```

4 octobre 2026. Décisions et chiffres : [`SORTIE_PLATE.md`](../../../docs/SORTIE_PLATE.md). Toutes les sessions
passent par `gcp-migration/v11_session.py`, cible explicite `ehgp-v7-3b1d496aed430749ea7e049f` (us-central1-c),
arrêt ciblé certifié et `TERMINATED` relu dans chaque `receipt.json`.

## Sessions

| Session | Commit | Contenu | Verdict |
| --- | --- | --- | --- |
| `claudeflat0` | 4fac50118 | porte points stricte ; export des arbres de points de $H^{r}_{k+1}$ et des arbres de `sklearn` (36 exemples organisés, niveaux exacts ; 360 bouts du criblage, niveaux flottants bornés), k = 2, 3, 5, 10 | porte conforme (2 827 nuages) ; 396 scènes sur 396 ; archive de 164 Mo hors dépôt, empreinte dans le reçu |
| `claudeflat1a` | 1de002a38 | porte points stricte ; **porte de la sortie plate en natif** ; dev synthétique à 8 000 points (96 scènes gelées) | portes conformes : 1 654 nuages, 119 088 comparaisons contre l'oracle, 0 désaccord, 32/32 fixtures, 9/9 mutants ; 96 scènes sur 96 |
| `claudeflat1b` | d02ec208b | portes points et sortie plate ; dev synthétique à 16 000 points (96 scènes gelées) | portes conformes (sortie plate : 641 nuages, 46 152 comparaisons, 32/32, 9/9) ; 96 scènes sur 96 |

## Étude LiDAR (hors VM, sur les arbres de `claudeflat0`)

- [`etude/etude_table.json`](etude/etude_table.json) : critère écrit avant lecture (`bench/points_flat_study.py`),
  population des couples (scène, k) où la hiérarchie trouve tous les objets.
- [`etude/etats_par_objet.json`](etude/etats_par_objet.json) : objets trouvés par la hiérarchie, intacts,
  fusionnés, découpés, au bruit, absorbés, par sous-ensemble, règle et ordre (`etude/split_merge.py`).
- [`etude/oracle_lot2.json`](etude/oracle_lot2.json) : antichaîne optimale (vérité connue) contre les règles, sur
  les exemples organisés (`etude/oracle_study.py`).
- Les images et comptes par exemple sont dans `Zoltan/demos/*/*/plat_k<k>.png` et `plat.json`.

Aucune coordonnée ni étiquette KITTI n'est versionnée : noms de scènes, comptes et métriques seulement.

## Choix synthétique sur le dev

Sur les 192 scènes du dev (8 000 et 16 000 points), moyenne équipondérée des cellules du mIoU un-à-un sur k = 2, 3,
5, 10 et mcs = 10, 20, $\sqrt{n}$ : EOM z = 2 de la tour 0,618 (PQ 0,474), z = 3 0,588, z = 1 0,567, feuilles 0,207.
Règle retenue : **z = 2**, fixée dans [`e1_prereg_synthetique_20261004.json`](../../../plans/e1_prereg_synthetique_20261004.json)
avant toute scène de test. Écarts du dev contre `sklearn` : +0,039, +0,058, +0,039, −0,008 à k = 2, 3, 5, 10 ; la
même tête sur l'arbre de `sklearn` gagne +0,033 à +0,051 à tout k : le gain vient surtout de la sélection, et la
hiérarchie de la tour perd à k = 10 (−0,043).

## Relecture

```bash
python3 morsehgp3D_v11/receipts/developpement_20261004/e1_sortie_plate/check.py
```

Le lecteur relit chaque session (statut, arrêt certifié, empreinte de l'archive versionnée, portes, scènes) et
recalcule sur le dev la règle de la tour de meilleur mIoU un-à-un moyen. Code 0 si tout concorde.
