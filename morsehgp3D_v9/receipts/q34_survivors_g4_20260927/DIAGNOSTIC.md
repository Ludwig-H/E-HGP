# R1 : échec de pré-épinglage, sans nouvelle mesure GPU

27 septembre 2026. Le [reçu clos](r1/README.md) reste **failed**.
Sa lecture normale et `-O` passe, ainsi que la contrelecture indépendante :
cela valide la conservation de l'échec, pas le prototype CUDA.

La configuration CUDA12.9.41 réussit, puis `compile_contract.generated`
exige à tort la présence d'au moins un fichier `.rsp`. Le CMake distant
3.22.1 n'en produit pas, contrairement au préflight local3.28.3. L'erreur
est conservée dans [prepin.stderr](r1/vm/prepin.stderr). Elle précède les
commandes de dépendances `-M`, le build, les portes CUDA et les quatre
mesures prévues. Sources inchangées ; budget utile consommé2,325s.

La même exigence apparaît dans `validate_closed`. Le selftest synthétique
contenait un `flags.rsp` non référencé par les commandes : il confortait
l'hypothèse de format au lieu de tester aussi une configuration directe.
Les nombreux refus locaux ne couvraient donc pas cette variation réelle.

Correction minimale future : accepter les options directes, fermer tous
les fichiers indirects **effectivement référencés** par compilation/lien,
y compris références imbriquées et NVCC `--options-file`, et vérifier les
références dans les preuves reçues. Refuser absence, modification, cycle
ou chemin hors périmètre. Tester à la fois configuration directe et
indirecte ; maintenir les autres contrôles de sources, options, unités,
dépendances, objets, archives et binaires. Utiliser une capture distincte,
sans modifier la preuve R1 ni son protocole gelé.

G4 SPOT arrêtée et relue TERMINATED pour la même génération :
12:54:28,609 → 12:57:04,535 UTC, soit155,926s d'allocation observée.
Ce temps n'est pas une facture. Aucun gain ou défaut géométrique déduit,
aucune relance automatique : [priorité au profilage FULL](../../docs/PROFILAGE_AVANT_REFONTE_20260927.md).
