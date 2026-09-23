# Contre-audit B — préflight G4 R14 et attribution GPU v19

23 septembre 2026. Lecture **statique** des scripts et des chronos du
paquet publié `eb8028cc4`, sans exécution GCP. Il ne s'agit pas d'une
contre-mesure R14 : aucun reçu R14 n'est encore qualifié par cette note.

## Deux corrections avant de lire la nouvelle attribution de temps

1. La ligne **R-27** de `audits/README.md` dit que le préflight local
   d'au moins **2 Gio libres sur `/workspaces`** a été ajouté avant
   toute session G4. Dans `gcp-migration/tower_session_v9.py`,
   `run_session` vérifie les fichiers et hashes puis crée le répertoire
   hôte ; il peut ensuite atteindre `guarded_start` vers la ligne 418.
   Aucun contrôle `disk_usage`/`statvfs`/`df` n'est présent dans ce
   contrôleur, le snapshot, le worker, le selftest ou le script de
   démarrage épinglé. Le contrôle annoncé n'est donc **pas intégré au
   chemin publié**. R13 avait atteint 318 Mio libres ; un état libre
   favorable à une heure donnée ne remplace pas une garde au prochain
   lancement. Ajouter le contrôle
   avant `guarded_start`, avec refus typé et selftest sous espace simulé,
   ou désigner/épingler explicitement un lanceur externe qui l'exécute.

2. Les nouveaux champs `filter_kernel_ms` et `filter_transfer_ms` ne
   sont **pas** une décomposition « noyaux purs / copies pures ».
   `tower_chain.cpp` additionne `rect_ms+scan_ms+pair_ms+select_ms`
   pour le premier. Dans `gpu/filter_runner.cu`, l'intervalle `scan_ms`
   contient une copie D2H synchrone des queues, allocation temporaire
   du scan et interrogation de mémoire ; `select_ms` contient aussi
   copie D2H et allocations. `upload_ms`/`download_ms` incluent
   allocations et initialisations en plus des copies, en particulier
   le `certificate_transfer_ms` de S3. Les événements CUDA mesurent
   bien ces **intervalles de pipeline**, mais le nom « kernel » leur
   prête une attribution matérielle qu'ils n'ont pas. Le lecteur
   `tower_worker_v9.py::validate_batch` exige seulement un temps
   noyau positif et `kernel+transfer≤device+0,05 ms` : deux champs
   arbitrairement sous-déclarés laissent un résidu non attribué sans
   refus. Si les intervalles restent tels quels, les renommer et
   contrôler `|total−Σ intervalles|` sous tolérance ; sinon ajouter
   des événements séparés pour noyaux, copies et préparation.

Ces écarts ne changent pas nécessairement le **temps total de chaîne**
ni les résultats géométriques. Ils empêchent seulement d'attribuer
rigoureusement le gain R14 à « calcul GPU » plutôt qu'aux copies,
allocations et synchronisations.

## Portée du plan par défaut et portes présentes

`tower_snapshot_v9.py::default_plan` prévoit 18 cas : les six couples
GPU/moteur (3 scènes de la seule séquence 08 × K5/K10), puis six cas
entrelacés S2 seul ou S2+S3 sur 08/000000. Ce sont des **sous-nuages
entiers sans sol**, profil grille 1 mm, `s=8`, W48. Aucune trame brute
avec sol, autre séquence, `s=10/12` ou float32 n'est mesurée par ce
plan. Un plan personnalisé accepte `s=10/12`, mais les condensés
absolus préépinglés du lecteur ne couvrent que les six cas `s=8` ;
garder cette différence de preuve visible. Une réception `partial`
sans tous les jumeaux et répétitions n'autorise aucune attribution
de gain S3.

Les gardes de la mesure relative restent substantielles : tailles et
SHA des entrées, ordres `1..K`, jumeaux GPU/moteur, travail des
certificats, temps device observés, limite 1 500 s utile/600 s par cas,
arrêt/fermeture ciblés. Ce préflight ne remet pas ces portes en cause.
L'exactitude du statut demeure **`complete_relative`**, pas une preuve
de complétude globale, et la qualification du contrat exige ensuite
trames brutes entières et sans sol de plusieurs séquences, s8/10/12,
K5/K10 et une vraie tour GPU sous une seconde.
