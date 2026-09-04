# Provenance de la revue arithmétique, 4 septembre 2026

`public_status=not_claimed`. Reçu de LECTURE et de port documentaire,
sans qualification expérimentale nouvelle. Aucun code produit, test,
banc ou CMake n'a été modifié pour cette revue ; aucune compilation,
exécution de porte, mesure ou session GCP n'a été lancée.

Le responsable a lu intégralement les documents de l'overlay
`build/v7_arithmetic_obligations/` avant d'autoriser leur port explicite :

- `GRAND_LIVRE.md` devient [ARITHMETIQUE_PRIMITIVES.md](../../docs/ARITHMETIQUE_PRIMITIVES.md).
- `FIXTURES_PROPOSEES.md` devient [PLAN_PORTES_ARITHMETIQUES.md](../../docs/PLAN_PORTES_ARITHMETIQUES.md).

Seuls les renvois et les localisations de provenance changent dans ces
deux ports. Un renvoi bref est ajouté au § 4 de la cartographie S1 et
une demande de contrelecture au dialogue constructeur/auditeur. Les
rapports de l'auditeur ne sont pas réécrits. La preuve est conditionnelle
aux domaines nommés ; les nouvelles fixtures restent proposées, ni
implémentées ni exécutées.

`sources.overlay.sha256` conserve octet pour octet le manifeste original
des 17 fichiers relus. `sources.sha256` conserve les mêmes 17 hashes, mais
redirige la seule cartographie contextuelle vers sa copie
`QUALIFICATION_S1_PRIMITIVES.before.txt` : le renvoi ajouté dans la version
active ne doit pas rendre cette provenance circulaire. Aucun fichier
algorithmique n'est redirigé ou substitué. Depuis la racine du dépôt :

```bash
sha256sum -c morsehgp3D_v7/receipts/arithmetic_review_20260904/sources.sha256
```

Ce contrôle vérifie les sources examinées, pas leur correction. Les
hashes des documents portés et des artefacts conservés figurent dans
`manifest.json`. Le manifeste historique original pointe volontairement
vers l'ancienne version de la cartographie, qui a maintenant son renvoi.

Les conclusions nouvelles à contre-lire sont : bornes q3 constantes
54M^5/135M^6 justifiant 86/104 bits ; preuve des colonnes U192/U320 ;
précondition supplémentaire INT128_MIN/-1 de floor_div128, non atteinte
par AxisBounds ; cinquième mot U320 non atteint par les niveaux u16 et
absence de porte CMake identifiée pour `level-trunc-hi`. Aucun UB produit
atteignable n'a été établi et aucun statut public n'est promu.
