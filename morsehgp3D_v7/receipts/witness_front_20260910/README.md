# Front hôte optionnel par lots — preuve CPU, 10 septembre 2026

Statut : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La porte fraîche passe en O2 et sous ASan/UBSan : 431 010 contrôles, 1 728 cas, 88 560 lignes, 311 968 requêtes, 15 rejets et une réutilisation non-vide puis singleton. Les deux arguments invalides sortent 2. Trois mutants compilent sans diagnostic puis sont rejetés avec le code 1 et leur raison causale exacte. Le front scalaire reste le défaut ; aucun backend CUDA n'est exercé ici.

## Périmètre

Le front délègue uniquement les comptages de témoins, conserve l'ordre stable des vagues et le bilan des masses de paires sur l'hôte, puis publie sa sortie transactionnellement. Le corpus compare à la voie scalaire les rectangles, masques, comptages et statistiques appariées, sur s=8/10/12 et plusieurs tailles de lot. Aucune matrice globale de paires ou de nœuds visités n'est créée.

Le fournisseur CPU emprunte exactement la même instance immuable de `CloudIndex`. Un futur fournisseur GPU devra lier son index préparé à une identité vérifiée ; une réponse bien formée d'un autre nuage n'est pas un témoin. La génération maximale reste réservée au sentinelle et ne peut être envoyée. `WitnessFrontWork` est par appel, y compris pour un singleton ; `GenerateStats` reste cumulatif. Le pic de vague initial est mesuré par le nouveau champ sans changer la sémantique historique de `GenerateStats`. Un échec tardif ne publie ni sortie ni statistiques partielles ; les statistiques ne représentent donc pas le travail physique abandonné.

Les appels directs au fournisseur CPU supposent des `NodeRef`, spans et pointeurs valides ; le front testé les construit. La validation d'association et de forme des lignes n'est pas en elle-même un oracle géométrique.

## Mutants causaux

| Retrait privé | Compilation | Exécution | Diagnostic exact |
| --- | --- | --- | --- |
| Liaison à l'index avant requête | 0 | 1 | `binding.wrong_cloud_rejected_before_query` |
| Réservation de MAX : retour à la seule garde MAX, qui enverrait MAX depuis MAX−1 | 0 | 1 | `generation.reserved_before_first_query` |
| Remise à zéro du travail sur retour singleton | 0 | 1 | `work.singleton_clears_per_call_only` |

Les deux derniers tests ont été renforcés pendant cette qualification : le test MAX−1 exige la génération inchangée avant toute requête ; la réutilisation singleton exige les cinq compteurs par appel nuls sans effacer le bilan cumulatif. La correction singleton provient de la contre-lecture de l'agent GPU.

## Vérification et sources

```bash
python3 -B morsehgp3D_v7/receipts/witness_front_20260910/verify.py
python3 -B -O morsehgp3D_v7/receipts/witness_front_20260910/verify.py
```

Ces lecteurs ne recompilent pas et n'utilisent ni Git ni GCP. Ils vérifient l'inventaire, les hashes, les sources avant/après, la fermeture réellement consommée via les fichiers `.d`, tous les codes et les diagnostics. `source_snapshot/` conserve les 30 fichiers projet consommés une seule fois ; chaque mutant ne conserve que son en-tête modifié et son patch. Les fichiers ELF ne sont pas publiés, leurs hashes le sont. `capture/record.py` est le script original et conserve les commandes absolues historiques : ce n'est pas un chemin nécessaire au lecteur portable. Les dépendances système ne sont pas archivées ; la version du compilateur est capturée.

Pins actifs qualifiés :

- `src/pipeline/witness_front.hpp` : `fb6f1bcca0a6dee13d9c786250bdd26e4c388b8d6e7caac615f7a33972937b81`.
- `src/spindle/witness_batch.hpp` : `66f31ead8b358dbbb09274b1a0e4fbfcc4777604827527f5c0b2220c1602cd52`.
- `tests/witness_front_gate.cpp` : `f283bca024d593abc47305c5c40f32af1be47d89ff09d84da9225b7e48f1c4a7`.

Aucun chronométrage contractuel : cette porte ne certifie ni toute la tour à 50k sous une seconde, ni 100 ms, ni les dizaines de millions de points, ni une accélération GPU. GCP non utilisé.
