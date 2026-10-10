# Compagnon des mutants de recherche exacte

10 octobre 2026. Complément d'intégration au reçu immuable
[`g_population_egalite`](../g_population_egalite/README.md), sans modification
de celui-ci. Proposition non intégrée ; aucune compilation, exécution native
ou mesure de temps. Cadre : exploration v12 hors registre, `public_status=not_claimed`.

Le retour immédiat sur égalité supprime exactement deux ancres du manifeste
`tests/mutants/tower.json` à 73 mutants. Le correctif du moteur doit donc être
accompagné de `proposition.patch` ci-présent, qui ne modifie que les champs
`cherche`, `remplace` et `note` de ces deux entrées :

- `recherche_sans_egalite_des_sites` : remplacer le retour sur comparaison
  exacte nulle par un retour sur seule égalité du hash au milieu courant.
  Même porte `mhgp12_tower_index_index_reference`.
- `recherche_premiere_place_du_seau` : remplacer toute la boucle par une
  comparaison exacte de la première fiche. Aucun maintien artificiel d'une
  borne constante dans une boucle, donc aucune boucle infinie introduite.
  Même porte `mhgp12_tower_index_weak_key_resolution`.

Les identifiants, portes, ordre, plancher 73, métadonnées et 71 autres entrées
restent identiques. Le lecteur officiel applique chaque mutation isolément,
avec ses éventuels remplacements `aussi` dans leur ordre : **73 mutants,
77 remplacements dont 4 secondaires, chaque ancre présente une seule fois**.
Avant le compagnon, le même lecteur refuse exactement les deux entrées citées.

Le modèle indépendant force tous les hashes à zéro, sur trois populations
de deux sites et cinq requêtes. Le mutant hash seul rend quatre réponses
incorrectes : mauvaises naissances présentes ou faux succès sur absence.
Le mutant première fiche provoque deux faux échecs sur des populations
présentes. La recherche exacte correspond toujours à l'oracle linéaire et
termine en au plus deux itérations ; le mutant première fiche n'a pas de
boucle. Les rangs distincts accompagnant les réponses sont conservés dans
le résultat attendu ; aucune donnée de nuage réel n'est utilisée.

Les portes existantes couvrent ces causes : `index_reference` compare les
naissances et rangs exacts sous masque nul ; `weak_key_resolution` compare
notamment les compteurs des routes de résolution. Un faux échec de première
sonde peut retrouver plus tard la même cible FULL mais changer ces compteurs :
le modèle ne prétend pas démontrer une erreur de cible finale dans ce cas.
Il démontre la réponse incorrecte de la recherche. **Aucun mutant natif n'est
déclaré tué par ce reçu.**

La composition a également été appliquée dans une copie temporaire de
`ae8f8107cc5a13356da89addf90808b5aaad9d67`, dans l'ordre recherche exacte,
présent manifeste, puis
[`t1_support_population`](../t1_support_population/README.md).
Le patch T1 publié est épinglé par son commit et son SHA-256
`e77781e64c92f4540850a0408dca3e5a7257c51c915518c17562b5a40d9c789b`.
Les postimages des deux fichiers produit correspondent aux propositions
publiées. Les 36 fichiers nécessaires sont épinglés ; les sources du premier
contrôle à `2aaed1847` ont aussi les mêmes empreintes sur cette base de
composition. Le lecteur officiel rend alors :

```
manifeste_ok module=tower mutants=73 plancher=73
manifeste_ok module=catalogue mutants=38 plancher=38
```

Les **77 ancres tour et 38 ancres Catalogue** sont présentes une seule fois
au moment de chaque remplacement. C'est une compatibilité statique de la
composition, pas sa qualification native. La preuve et les préconditions
mathématiques T1 restent celles du reçu lié ; pas de nouveau gain annoncé.

Rejeu Python seul, depuis ce dossier :

```
python3 -B -S check.py /chemin/du/depot
python3 -B -S -O check.py /chemin/du/depot
```

Ces deux relectures vérifient les empreintes, appliquent les patches seulement
dans des copies temporaires, appellent le lanceur officiel avec `--check`
uniquement et confrontent le résultat à `results.json`. `capture.json` donne
les pins exacts ; `SHA256SUMS` ferme ce petit reçu. Les portes et campagnes
natives prévues par les deux propositions restent nécessaires avant adoption.
