# MES-FULL : livraison du pilote capturé

Supplément au [reçu d'admission publié](../mes_full_admission/README.md), laissé
inchangé dans `f6eaa4a66`. Le pilote auparavant capturé en cours est livré en
`c9ac60f20c741d9d493f4900fd8aec4590aaa5c5` **avec les mêmes octets** :
`a89ceec92f76c642a508a0c76d5a0a8a31400cab15a7d27141bc37c557b9f92c`.

Cette identité rattache les témoins d'admission CST-0018 au code livré : preuve
d'isolation manquante, mesures/métadonnées insuffisamment gardées et cohorte
non exigée. Elle ne démontre pas que des durées brutes sont fausses et ne
requalifie aucun verdict de campagne. Les résultats d'une session qui emploie
ces octets doivent être contre-jugés avec des admissions complètes ; aucune
interruption de session ni nouvelle exécution native n'est effectuée ici.

Le lecteur compare les objets Git immuables et la capture historique :

```sh
python check.py --repo DEPOT
python -O check.py --repo DEPOT
```

Les sorties égalent le champ `result` de `capture.json`. Ce supplément ne
réexécute pas les témoins Python historiques, ne modifie pas leur reçu et ne
dépend pas du fichier de travail du pilote. `public_status=not_claimed`.
