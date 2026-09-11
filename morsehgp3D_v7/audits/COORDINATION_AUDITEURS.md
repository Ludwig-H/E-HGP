# Coordination entre auditeurs

11 septembre 2026, après **34db4b3e**. Écritures dans le seul
`morsehgp3D_v7/audits/`, sur `main`. Réservation précédente close par
19049adf. Réservation d’index pour cette publication : les six fichiers
`COORDINATION_AUDITEURS.md`, `DIALOGUE_COURANT.md`, `ETAT_COURANT.md`,
`README.md`, `ENTRETIEN.json` et `validation_current.json` uniquement.
Aucun fichier d’une autre session inclus ; réservation close à publication.

## Reçus dense et workers désormais contre-vérifiés

Les lecteurs [dense/pool](../receipts/birth_streaming_20260911/README.md)
et [géométrie parallèle](../receipts/parallel_birth_streaming_20260911/README.md)
passent normal/−O, sur les octets publiés dans 34db4b3e. Sources déjà
contre-lues inchangées : dense 87eb210e, pool 4ec1a206, parallèle f400de79.
Les qualifications O2/SAN sont maintenant attribuées à leurs captures propres ;
TSan reste un échec préalable code66. Les premiers raccords et triplets sont
clos à cette portée, sans nouvelle exécution C++ par notre audit.

## Suite utile : consommer les réponses historiques déjà calculées

`reconstruct` fournit déjà dans `history.marks` la composante fermée de
chaque rôle à son admission. `graph_full::build` ignore cette table et
repose les questions contributives en HLD. Le [dialogue courant](DIALOGUE_COURANT.md)
établit la liaison nécessaire : histoire immuable authentifiée, MarkId,
φ et admission. À 32k, **10 348 964** consultations peuvent devenir des
réemplois de marques ; les requêtes verticales à d’autres dates restent.

Garder la boucle `atlas.program(K)` pour l’ordre physique des contributions :
les marques sont triées par MarkId, pas par ordre du programme. Garder aussi
leur admission, distincte de la naissance du segment. Les marques silencieuses
fournissent les minima de groupe/lot de notre preuve d’export historique ;
elles ne sont pas à filtrer avant cette consommation.

Deuxième doublon : Chains(K) est reconstruit comme lower_chains au K suivant.
Garder précédent/courant ramène **19 préparations à 10** pour K1..10, avec
au plus deux index simultanés. Conserver la validation de chaque histoire et
la durée de vie de leurs propriétaires. Aucun gain de temps n’est déduit.

## Décisions conservées sans répétition

La préparation commune, les semis liés et les gardes par rangs ont été
acceptés comme deltas séparés. Le timeout du runner reste un correctif futur,
sans réécriture des captures scellées. Les [preuves précédentes](README.md)
et fichiers du second auditeur sont intacts ; les notes courantes remplacent
les anciens messages « en préparation ». Commandes et empreintes dans
l’[entretien](ENTRETIEN.json). GCP non utilisé.
