# Publication des reçus : deux corrections, une collision

Pin `07ee13ef6bebc0b6b85da90207f3f067ffff1755`, correctif `2f7b41380`.
Le [témoin](check.py) appelle le vrai `main` de `microbancs/outils/recu_session.py`,
sans remplacer sa logique. Répertoires temporaires et identité synthétique
`audit@example.invalid` seulement ; aucune publication historique modifiée.
[Résultat](result.json) identique avec Python normal et `-O`, sources et témoin
hachés avant/après. Aucun appel GCP.

**CST-0219 clos dans sa portée.** Le nom contenant l'identité est anonymisé ;
l'identité disparaît aussi du manifeste, désormais écrit avant le contrôle final.
Le manifeste est vérifié contre tous les octets publiés. Une fuite binaire
non expurgeable rend le code 3 et retire les fichiers créés.

**CST-0221 clos dans sa portée.** Une destination non vide est refusée avec
le code 2 avant toute écriture : les deux fichiers antérieurs sont inchangés.
Une destination qui est un fichier est également préservée. Lors du refus
d'une fuite, une destination initialement vide reste présente et vide ; une
destination créée par l'appel est supprimée. Pas de concurrence entre écrivains
dans ces témoins.

**CST-0224 ouvert : l'anonymisation n'est pas injective.** Les deux fichiers
sélectionnés `run-audit@example.invalid.json` et `run-<compte>.json`, de contenus
distincts, donnent le même chemin de sortie. Le vrai publieur retourne 0 et
annonce trois fichiers (deux résultats plus le reçu), mais ne conserve qu'un
résultat plus le reçu. Le manifeste est cohérent avec cet arbre amputé : il
ne détecte pas la perte. Les sources restent intactes. Le contrôle positif
avec deux noms distincts publie bien les deux résultats.

Correction attendue : calculer et vérifier l'unicité de tous les chemins
anonymisés avant de publier, puis refuser une collision, ou adopter une
convention sans collision et conserver la correspondance. Aucune perte dans
les campagnes réelles C/D n'est déduite de ce témoin.

Rejeu au pin : `python3 -B check.py` puis `python3 -B -O check.py` depuis ce
dossier, ou chemins absolus ; les captures doivent coïncider avec `result.json`.
