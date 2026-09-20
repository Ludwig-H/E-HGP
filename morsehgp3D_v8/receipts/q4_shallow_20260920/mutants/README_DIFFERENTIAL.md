# Compatibilité du chemin local28 dans le build29

Capture [differential_tzfjhtw7](differential_tzfjhtw7/MANIFEST.json) close après le gel des184 sources et la fin du build Release : **40 commandes,20 paires PASS**. Tous les champs JSON concordent hors `timings` ; sources, artefacts, helper et autorité sont identiques avant/après. Les quatre lecteurs historiques/vivants, normal/`-O`, passent ; commandes, sorties et codes figurent dans [DIFFERENTIAL_READBACK.json](DIFFERENTIAL_READBACK.json).

Le helper adapte explicitement le différentiel28, sans modifier ses fichiers ou captures. Vingt paires comparent `mhgp8_q4_local_probe` entre `build/v8_q4_local_r2_20260920` et `build/v8_q4_shallow_20260920` : far/cap × K5/10 × n8k/16k/32k, adversarial × K5/10 × n32/64/128/256. Options fixes : domaine positif, profondeur7, budget de4096 nœuds,512 tests Z intermédiaires, grain32, clipping activé, référence complète `cover`. Aucun cas dense quadratique n'est ajouté.

Tous les champs JSON doivent être égaux, sauf le seul objet `timings` ; l'ancien lecteur local valide chaque sortie contre sa commande. Cela vérifie que le nouveau chemin shallow ne modifie pas la voie locale28 : ni performance ni complétude globale de la nouvelle voie ne sont déduites de ce différentiel.

L'autorité de l'ancien exécutable et du cache est [scale_ipjudaz2](../../q4_local_20260920/scale_ipjudaz2/MANIFEST.json), close sur177 sources. Ses manifeste et clôture sont archivés dans chaque nouvelle tentative. Ce reçu n'épingle pas l'ancienne archive : le protocole ne lui invente pas cette autorité ; les deux archives sont simplement hachées avant/après la nouvelle capture, avec les caches, sondes,184 sources, helper et autorité historique. Les lectures historiques et `--check-live` seront exercées en Python normal et `-O`.

Le contrôle final `git diff --cached --check` signale uniquement une ligne
vide en fin du helper `run_default_differential.py`, déjà épinglé par cette
capture. Elle est conservée telle quelle pour préserver son empreinte ;
ce n'est ni une erreur de test ni un changement du moteur. Les quatre
lectures annoncées ci-dessus ont bien été exécutées et passent.

GCP non utilisé ; aucun contrat FULL/G4 ni borne générale sous-quadratique revendiqués.
