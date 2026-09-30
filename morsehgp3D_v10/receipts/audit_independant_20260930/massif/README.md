# Reçu d'audit massif — 30 septembre 2026

Accompagne [l'audit courant](../../../audits/AUDIT_MASSIF_LIDAR_20260930.md). `public_status=not_claimed`. Sources produit au commit `bdc0b8f08ee37ee351996b29c5974f78415bc036`, inchangées sur `origin/main` à `a6b380e9c`. Aucun moteur modifié, aucune allocation massive, aucun GCP.

Trois captures séparées, fermées chacune par leur SHA256SUMS :

- [Représentation et indices](representation/LIMITES.md) : dispositions ABI et calcul du milieu de dichotomie en u32. [Résultat](representation/RESULT.json), [formules](representation/FORMULES.json), [manifest des sources](representation/SOURCE_MANIFEST.json). Les types privés sont copiés, non instrumentés ; la boucle est bornée à 200 pas. Aucun nuage réalisant des milliards de niveaux n'est exécuté.
- [Dimensionnement](dimensionnement/README.md) : extraction des deux lignes synthétiques S5 à 1,024 M sites, faits machine et scénarios purement arithmétiques à 10/30/50 M. Les ratios empiriques ne deviennent ni des bornes ni une qualification ; `--no-points` et les coûts exclus sont déclarés.
- [Sémantique](semantique/receipt.json) : deux fixtures colinéaires complètes Γ2 et une borne de boîte calculées en Fraction. Normal/−O donnent les mêmes sorties. Ces calculs illustrent la perte d'une fusion par point partagé ou d'une naissance dans un grand vide ; aucune exécution native ni campagne statistique.

Les SHA historiques ne sont pas réécrits. Les ledgers de ce sous-dossier fixent les nouvelles preuves ; le contrôle de publication ajoute ses propres empreintes et vérifie les liens des notes actuelles. Les sources et reçus des autres acteurs restent sous leur attribution.
