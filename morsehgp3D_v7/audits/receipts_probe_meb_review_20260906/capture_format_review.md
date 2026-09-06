# Le comparateur préparé refuse le format de son contrôleur

6 septembre 2026, captures figées à `10:15:17.801166 UTC`. Le comparateur est une préparation non publiée : première lecture `653ab8ac…`, puis ajout concurrent de modèles de provenance et capture `be4b8712…`. Les cinq incompatibilités ci-dessous sont encore présentes dans cette seconde version. Le contrôleur capturé porte `ee9d4640…`. Ces octets sont conservés [inertement](captured_inputs.json) ; aucune copie n’est exécutée comme contrôleur et aucun ELF n’est ouvert.

Le [reproducteur borné](capture_format_probe.py) appelle réellement `load_arm` du comparateur capturé. Il utilise les métadonnées et flux du seul microcas `n8_s8_k5_lazy_c0_p0`, clos avec code 0 à `10:13:30.602622 UTC`. Ses deux lecteurs avaient rendu des admissions positives ; le verdict de capture est clos à `10:13:30.880165 UTC`. Ces verdicts sont des reçus consultés, pas des juges importés ni relancés par cette preuve.

`Controller.probe()` utilise directement `self.command(..., merged=True)`, pour les microcas comme pour les grandes tentatives. Ce chemin produit bien le format capturé. Aucun appel à un ancien ordonnanceur n’évite le problème.

| Étape | Format effectivement capturé | Attente du comparateur `be4b8712` | Premier arrêt reproduit |
| --- | --- | --- | --- |
| 0 | `source_map_sha256` | `source_sha256` | `KeyError: source_sha256` |
| 1 | Calendrier de sonde dans `protocol.json` | `probe_schema` aussi dans le snapshot | `KeyError: probe_schema` |
| 2 | Calendrier des successeurs dans `protocol.json` | `successor_accounting` aussi dans le snapshot | `KeyError: successor_accounting` |
| 3 | Deux flux scellés : `.raw.txt` et `.stderr` vide | Un seul flux `.raw.txt` | `ValueError: stream_inventory` |
| 4 | Intention avec trois gardes supplémentaires | Inventaire exact de dix anciens champs | `ValueError: intent_inventory` |

Les trois champs supplémentaires sont `sanitizer_virtual_address_reservation=false`, `cpu_limit_seconds=620` et `file_size_limit_bytes=67108864`. Les empreintes avant/après des sources, le digest de leur carte, le miroir command/receipt, l’intention et les deux flux ferment sur cette capture. Il s’agit donc de **faux refus de format**, pas d’un échec scientifique du calcul ou d’un manque de terminal.

Pour isoler chaque obstacle, le reproducteur adapte seulement en mémoire les obstacles précédents : il ajoute les noms attendus depuis les métadonnées réelles, retire l’entrée du stderr déjà vérifié vide, puis retire les trois champs d’intention. Les captures restent inchangées. Avec ces cinq adaptations privées, `load_arm` atteint l’appel du juge primaire ; une sentinelle interrompt volontairement la preuve à cet endroit. **Aucune admission finale artificielle n’est produite.**

Le correctif constructif consiste à consommer le schéma déclaré du contrôleur : conserver `source_map_sha256`, prendre les calendriers dans le protocole auquel les sources sont déjà liées, vérifier les deux flux et le stderr vide, puis accepter et vérifier explicitement les valeurs des trois gardes. Une nouvelle porte doit partir d’une capture du vrai contrôleur ; les seuls modèles `compare_objects()` et cartes de sources synthétiques ne peuvent détecter cette incompatibilité d’entrée. Les anciens reçus ne doivent pas être réécrits.

Les résultats [normal](normal.json) et [optimisé](optimized.json) sont identiques : `3660b3e42339eb6bd049d02837ceb5be9b9bb2412309bcc47428447a63a29abf`. Chaque lecture a été bornée à 60 secondes sur CPU1 ; aucune compilation, aucun moteur ni benchmark lancé. Rejeu depuis la racine :

```bash
python3 -B morsehgp3D_v7/audits/receipts_probe_meb_review_20260906/capture_format_probe.py
python3 -B -O morsehgp3D_v7/audits/receipts_probe_meb_review_20260906/capture_format_probe.py
```

Cette preuve reste attribuée au couple capturé. Une réparation concurrente nécessite sa contrelecture propre ; elle ne rend pas ces erreurs historiques actives à perpétuité. `public_status=not_claimed`. GCP non utilisé.
