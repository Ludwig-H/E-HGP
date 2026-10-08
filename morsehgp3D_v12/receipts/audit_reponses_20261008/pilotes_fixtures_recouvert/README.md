# Quatre fixtures d'horloges recouvertes à corriger

8 octobre 2026. Proposition sur les sources publiées `0e62a7232`, sans modification produit.
Les quatre sondes Python simulées de MES-FULL, MES-B, MES-C et Apparié émettent une fin de M1 à6
et une fin de V2 à5. V2 attend M1 : cette fixture dite positive est incohérente avec le produit.
Le [lecteur renforcé proposé](../lf_recouvert_gardes/README.md) la refuse donc à raison.
Cela ne remet pas en cause les 850 passes réelles A déjà relues par ce lecteur.

`proposition.patch` ne change que quatre lignes des tests, de
`[[4,5,6,0,6]] + [[3,4,5,5,6]]*(k−1)` vers
`[[4,5,6,0,6]] + [[3,4,5,6,6]]*(k−1)` : la fin de V des ordres≥2 devient6.
Toutes restent dans la fin globale6, après leur propre M et celui de l'ordre précédent.
V1 reste nul ; aucune relation d'ordre générale entre V et R n'est ajoutée.

Preuve causale pour chaque fixture : sortie originale acceptée par `c3e9e0f4`, refusée par le lecteur
proposé pour « dépendances des fins par ordre » ; sortie corrigée acceptée par les deux.
Les sorties de la branche séquentielle sont identiques octet pour octet avant/après.
Les quatre portes officielles passent ensuite avec **LF renforcé + ces quatre corrections**,
en Python normal et −O, avec sorties identiques. L'Apparié de ces quatre portes est le pilote
publié, sans les correctifs d'admission : leur composition est testée séparément dans
[apparie_livraison](../apparie_livraison/README.md), sans répéter les trois autres portes.

```sh
python3 -B check.py --repo DEPOT
python3 -B -O check.py --repo DEPOT
```

Les deux sorties correspondent à `results.json`. `capture.json` ferme21 sources Python Git,
les préimages/postimages des quatre tests et les deux patches. Le lecteur vérifie leur application
dans un temporaire puis exécute uniquement les fausses sondes Python et les portes officielles.
Les petits fichiers de zéros sont synthétiques. Ces fixtures servent au schéma/aux pilotes,
pas à mesurer des temps ni à qualifier toute propriété physique d'une vraie exécution.
Aucun moteur, compilation, CUDA, GCP ou donnée sous licence ; sources/main et reçus antérieurs intacts.
