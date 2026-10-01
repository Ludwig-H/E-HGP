# Collecteur R2 : causalité et inventaire, 1er octobre 2026

Paquet TEXT-ONLY, controlflow Python uniquement. Aucun moteur, compilation, CLI natif, GCP ou écriture de source
partagée. Le collecteur original complet est figé dans `sources/mutants_entrees_cli.py`, SHA
`2555ba3c31eec86b62f3f11798ecc03a8afb2061ed8c921567dc670c4a892d51` ; original vérifié avant/après les captures.
Il n'importe aucun autre helper local. La bibliothèque standard/runtime Python reste une dépendance extérieure,
explicitement déclarée et l'exécutable Python est épinglé dans `receipt.json` ; aucun compilateur n'est requis.

`controlflow.py` appelle le **main réel**, avec `build_copy`, `judges` et l'`open` du collecteur remplacés en RAM.
La fixture virtuelle contient l'unique motif MA1 exact de la table d'origine. Les témoins avant/après rendent0.
Il ne teste donc pas l'exécution C++ d'un mutant : il teste la classification d'un résultat fourni au harnais.

Quatre cas, chacun sous Python normal et `-O` (8 captures) :

- contrôle `judge_1` : produit1, un mutant compté tué, collecteur0 ;
- `signal_11` : produit−11, également compté tué, collecteur0 ;
- `timeout` : `delai_1500s`, également compté tué, collecteur0 ;
- `unknown_id` : identifiant absent, aucun mutant, deux témoins, collecteur0.

Ainsi le collecteur contredit son contrat « signal ou délai jamais un succès » et accepte une sélection vide.
Cela ne prétend pas que les captures natives déjà publiées utilisent ces défauts. Les autres remarques statiques
sur isolants, G4 simulé et inventaire des lots ne font pas partie de cette preuve.

Lecture hash-first, **sans mutation**, avec le SHA du manifeste fourni extérieurement :

    python3 -B read.py CHEMIN_PAQUET SHA_MANIFEST
    python3 -B -O read.py CHEMIN_PAQUET SHA_MANIFEST

Contre-rejeu fonctionnel explicite (8 subprocess Python, timeout10s chacun, aucun appel natif/aucune écriture) :

    python3 -B read.py CHEMIN_PAQUET SHA_MANIFEST --replay-controlflow

Le lecteur contrôle les23 fichiers manifestés, l'inventaire physique complet25 (manifeste+sidecar inclus),
les sources, les8 commandes/captures exactes, les témoins, les codes et les résultats sémantiques ; empreintes
recontrôlées après lecture/rejeu. Le chemin du runner seul s'adapte à une relocation, l'argv initial exact reste
archivé. Le lecteur n'exécute pas `capture.py`, ne réécrit aucun fichier et ne charge aucun moteur.

Trois mutations causales du lecteur, en copies privées avec manifeste **réépinglé** : manifeste vide,
cas omis, code de sortie falsifié. Elles sont refusées avec code2 ; seuls les petits résultats sont conservés dans
`mutations.json`, pas les arbres mutés. `capture.py` est le recorder de la phase ouverte, interdit après clôture ;
il ne fait pas partie de la commande de lecture. Le paquet est immuable une fois le SHA final annoncé.
