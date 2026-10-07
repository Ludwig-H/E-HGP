# Reçu de la passation v11 → v12 : rapports bruts de l'audit final

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Instantané audité : `ac081a06f`, dernier commit moteur de la v11. **GCP non utilisé** : aucune session, aucune
compilation, aucune mesure nouvelle.

## Objet

Ce reçu conserve les pièces de l'[audit final](../../docs/AUDIT_FINAL_V11.md) et de la
[passation](../../PASSATION.md). Ces deux documents sont la synthèse ; les pièces ci-dessous en sont les sources
brutes.

## Méthode

Six lectures indépendantes ont été faites en **lecture seule** sur l'instantané `ac081a06f`, une par domaine. Chacune
a pu déléguer des relectures, qu'elle a recoupées par sondage. Aucune n'a écrit dans le dépôt.

| Fichier | Domaine |
|---|---|
| `rapports/A_mathematiques_exactitude.md` | objet mathématique, preuves, oracle, doctrine numérique |
| `rapports/B_catalogue_gpu.md` | catalogue (étage `domain`) et voie GPU |
| `rapports/C_tour_forets.md` | tour FULL et étage des forêts |
| `rapports/D_sorties_hierarchies.md` | sorties, hiérarchies dérivées, comparaison à HDBSCAN, polyèdres |
| `rapports/E_tests_outillage_g4.md` | tests, portes, mutants, mesure, sessions G4 |
| `rapports/F_canal_audit.md` | canal d'audit externe et dialogue avec le développeur |

**Légendes propres à chaque rapport** : [F] fait vérifié, [I] ou « inférence » pour ce qui est inféré, [M] pour une
note de mémoire du développeur, [F-hd] pour un fait vérifié hors dépôt.

**Retouches** : seul le chemin local de l'instantané a été remplacé par `morsehgp3D_v11/` dans l'en-tête du rapport A,
et un saut de ligne final a été ajouté. Le texte est sinon tel que rendu.

**Corrections.** Les rapports n'ont pas été réécrits après coup. Les points sur lesquels la synthèse les corrige sont
listés au § 16 de l'audit final :
- enveloppes M3/E4 toujours appliquées sur la voie CPU ;
- réservoir chaîné gardé ;
- temps au gel ;
- comparaison à la v10 étage par étage.

## Intégrité

`SHA256SUMS` couvre tous les fichiers de ce dossier, sauf lui-même :

```sh
cd morsehgp3D_v11/receipts/passation_20261007 && sha256sum -c SHA256SUMS
```

Aucune donnée ni coordonnée LiDAR, aucune identité de compte.
