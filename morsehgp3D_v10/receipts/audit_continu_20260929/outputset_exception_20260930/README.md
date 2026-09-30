# Exceptions de sortie : capture autonome du helper OutputSet

Audit du 30 septembre 2026. Origine copiée avant compilation :
`/tmp/mhgp10-audit-geant/verif_raccord_r2/repo`, commit
`bf704f9eb30d97c61cbdc62cade6b8e22450adbe`. Les quatre fichiers de
`source/core/` sont des copies exactes ; le reçu conserve leurs empreintes
avant/après ainsi que celles de l'origine. Aucun moteur/source partagé
n'a été modifié. Ce clone de raccord n'est pas une version publiée qualifiée.

Deux compilations/exécutions minuscules, normale et UBSan non récupérant,
finissent avec code 0 et stderr vide. Ici code 0 signifie que les défauts
attendus ont été effectivement observés, **pas que le helper est correct**.
Les commandes, horodatages, flux et SHA256 des binaires exécutés sont dans
`receipt.json`. Les binaires ne sont pas nécessaires au lecteur d'archive.

La sonde utilise uniquement trois fichiers sentinelles privés. Le contrôle
sans exception ouvre/ferme normalement et le destructeur retire le nom :
4 descripteurs avant/après. Une seule allocation C++ est ensuite forcée à
lever `bad_alloc`, après `fopen` mais avant l'enregistrement de cette sortie :
le compteur passe de 4 à 5, et la sentinelle reste présente, tronquée à zéro
octet. Un callback writer qui lève provoque un second descripteur non fermé :
5 à 6. Dans ce dernier cas le nom est retiré par le destructeur, mais son
descripteur est toujours ouvert. Le noyau les récupère à la fin de chaque
processus de sonde. Les fixtures ne sont pas des fichiers de l'utilisateur.

Cela prouve un défaut de garantie d'exception du helper réel copié, sans
affirmer qu'un callback actuel des CLI lève effectivement. Une protection
RAII du `FILE*` doit précéder toute opération C++ susceptible de lever.
Le fait que reserve tronque des fichiers préexistants est un autre défaut
déjà connu, pas la nouveauté de cette capture. La première allocation
défaillante peut être celle du chemin filesystem plutôt que du vector :
la capture ne localise pas plus finement son allocation.

Rejugement portable, sans build épinglé, sans recompilation ni accès à
l'origine : `python3 -B verify.py`, puis `python3 -B -O verify.py`.
Le lecteur vérifie les hashes avant d'importer le juge, relie les commandes
à leurs flux, compare normal/UBSan et refuse 24 mutations en mémoire,
y compris suppression de chaque champ, statuts menteurs et helper no-op.
Ce ne sont pas des mutants C++ compilés. Le lecteur vérifie l'archive ;
il ne démontre pas l'état LIVE d'un binaire aujourd'hui absent de l'archive.

Les observations bornées ne qualifient ni FULL, ni clustering/statistiques,
ni GPU/G4, ni croissance sous-quadratique. Aucun GCP ni longue campagne.
La capture reste distincte des essais d'intégration et des tests santé
portant sur l'ancien moteur. Pour rejouer le natif, utiliser une nouvelle
copie isolée et adapter `record.py` à une nouvelle origine gelée ; ne pas
écraser cette archive close.
