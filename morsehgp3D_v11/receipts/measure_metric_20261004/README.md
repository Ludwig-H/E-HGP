# Preuve bornée — distances de mesures et entrée immédiate

Résultat : à k=2,m=3 en comptes unitaires, toute règle à entrée immédiate lorsqu'il n'existe aucun rival qualifié
ne peut être uniformément lipschitzienne entre W_p des probabilités empiriques et les dates des IDs communs en
norme SUP, pour p fini et N non borné. Les mesures normalisées définissent le métrique, jamais les seuils de la règle.

[PREUVE.md](PREUVE.md) donne le lemme de lentille dans R³, le seuil fermé s₂/2, le couplage et les limites.
La grille u21 R=N,η=1 fournit des témoins finis jusqu'à N=299593 ; elle n'exclut aucune constante propre à un
profil fini. L¹/Lᵖ pondérée en sortie et W∞ ne sont pas exclus par cette preuve. Le paragraphe UOT est conditionnel
à un plafond de coût d'insertion déclaré.

Le helper indépendant utilise seulement Fraction et de petites listes (64 sites au maximum). Il contrôle
cardinalités, coquilles fermées, identités de convexité, marginales, coûts de couplage et bornes de coordonnées.
Il ne construit ni catalogue, ni FULL, ni nuage de 299k sites et n'importe aucun code produit/oracle. La preuve
générale vient de l'argument mathématique ; ces gardes vérifient ses identités, sans remplacer une preuve.

```
python -B check_scalars.py > checks.json
python -B -O check_scalars.py > checks_optimized.json
```

Les deux sorties sont identiques octet pour octet et toutes les gardes passent, sans asserts. Versions et
commandes sont conservées. Aucun natif, fit, build ou GCP n'est lancé ; aucune note active ni source produit n'est
modifiée. Les quatre sources publiques ab1a739d1 sont épinglées avant/après pour situer les conventions de la
règle ; elles restent inchangées. Le helper reste autonome et sa validité ne dépend pas de ces fichiers.

Le SHA256SUMS racine inventorie chaque fichier du reçu, à l'exclusion de lui-même.
