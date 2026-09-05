# Preuves du certificat horizontal réduit

5 septembre 2026. Autorité principale : sources CPU E. `public_status=not_claimed`, GCP non utilisé. Le [certificat courant](../CERTIFICAT_HORIZONTAL_COURANT.md) assemble la preuve ; les pièces ci-dessous distinguent exécutions du moteur, relectures et fautes attendues.

| Contrôle | Reçu | Portée |
| --- | --- | --- |
| Pipeline E contre Gamma rationnel et lecteur des seuls deltas | [Résumé](pipeline/summary.json), [entrées](pipeline/input.txt) | Deux builds O2/O1 UBSan : 16 appels, 60 ordres, 840 coupes, 200 deltas et 1 124 carrés de naturalité chacun |
| Rejeu des juges sur les mêmes sorties | [Normal](pipeline/replay_normal.json), [optimisé](pipeline/replay_optimized.json) | Sans compiler ni exécuter le produit ; mutant d'attache et faute de type du lecteur distingués |
| Fold normalisé contre hypergraphes par ensembles et lecteur de tokens | [Résumé et commandes](fold/summary.json) | Deux builds : 40 flux, 272 coupes, 128 deltas, sept vrais mutants code 4 chacun |
| Domaine et archive CLI E | [Résultats](domain/results.json), [contrôle](domain/check.json), [pilote](domain/check_cli_domain.py) | 18 cas : six succès, douze refus attendus ; binaire E scellé, pas de build |
| Delta F concurrent | [Lemme de conservation](f_delta/review.json), [octets](f_delta/delta_E_F.patch), [contrôleur](f_delta/verify_source_delta.py) | Revue statique LIFO/masques/comptes et build CLI fermé ; qualification intégrée F encore en attente à l'inspection |

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/horizontal_rational_oracle.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/horizontal_rational_oracle.py --replay
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/horizontal_fold_probe.py
```

Le pilote du pipeline reconstruit un snapshot privé E depuis le commit exact indiqué ; il ne compile pas la préparation F du worktree. La sonde fold n'inclut aucun fichier modifié par F ; ses dépendances `.d` sont scellées. Les commandes avec compilation doivent rester hors des fenêtres de chronométrage du constructeur.

L'[essai initial invalide](pipeline/initial_attempt.invalid.json.gz) conserve les sorties antérieures au gel E et à la correction du juge de métadonnée K1. Il n'est compté dans aucun résultat qualifié. Les autres refus, stdout, stderr, archives de fixtures et hashes sont préservés sans réécriture de leurs octets.
