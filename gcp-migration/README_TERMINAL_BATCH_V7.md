# Session compacte terminal par lots v7

`terminal_batch_worker_v7.py` est un worker invité pour le contrôleur existant
`full_probe_session_v7.py`, dont les octets et les gardes restent inchangés.
Le docstring historique CPU du contrôleur ne décrit pas le worker personnalisé.
Le périmètre exécuté est **census CPU + résolution terminale CUDA + calendrier
et assemblage FULL CPU**, profil u16, `public_status=not_claimed`.
Ce n'est ni une tour entièrement GPU ni un certificat industriel de performance.

## Sources et construction

Le probe privé est épinglé `21d0a5dd`, la gate compacte `98e426f2`, le support
de session `da967163` et l'adaptateur hôte strict `994d9e69`. Les inclusions
relatives privées sont conservées dans `bench/terminal_batch_private/` : aucun
helper actif n'est remplacé. Le générateur de snapshot sélectionne récursivement
les inclusions locales des deux unités, vérifie leurs pins communs et leur
appartenance à la capture qualifiée. Il ajoute le probe historique demandé
par le contrôleur, mais ne l'exécute pas. Aucun ELF ni Boost n'est embarqué.

Recettes historiques de préparation locale du snapshot `snapshot_r1`, worker
`043197e4`, sans cloud ni compilation (ne pas les relancer dans ces destinations
déjà fermées) :

```bash
python3 -B build/v7_terminal_batch_worker_20260911/record_pure.py --out pure_r1
python3 -B build/v7_terminal_batch_worker_20260911/build_snapshot.py --out build/v7_terminal_batch_worker_20260911/snapshot_r1 --gate-qualified-receipt build/v7_whole_batch_device_gate_20260911/san_root_r1/receipt.json
```

Ces scripts `build/` ne sont pas supposés présents dans un clone propre. Leurs
octets et captures sont conservés et extractibles dans
`morsehgp3D_v7/receipts/terminal_batch_worker_20260911/` ; son lecteur décrit le
replay autonome. Ces commandes ne préparent ni ne qualifient la nouvelle
révision diagnostic ci-dessous et ne doivent écraser aucun ancien reçu.

Le manifeste et le tar résultants se passent comme snapshot au contrôleur,
avec `worker.py` et son SHA comme worker personnalisé. ROOT choisit le répertoire
de session frais et conserve les arguments de contrôle exacts avant `--execute`.
Les exécutables invités restent dans `terminal_batch_tooling/`, voisin de
`output/`, donc hors archive récupérée. Les hashes ELF et les dépendances
compilées sont consignés, sans copier les binaires entre distributions.

La compilation invitée utilise NVCC 12.9, O3, SM120, C++20, `fmad=false`,
`cross-execution-space-call` fatal et les quatre avertissements hôte stricts.
Aucun diagnostic de compilation n'est admis. L'adaptateur est copié en mode
0700 et vérifié avant/après compilation. Aucun pilote, paquet ou outil n'est
installé. Les registres/spills observés sur un ancien build ne sont pas
réattribués à ce nouveau build O3.

### Révision diagnostic après l'échec EU du 11 septembre

Le worker historique `043197e4` pouvait refuser avant toute commande avec
`existing tools required, no installation`, sans conserver lequel des outils
manquait. Son échec EU, ses sources, son snapshot et ses reçus restent inchangés :
ils ne permettent pas de déduire rétrospectivement l'outil absent.

La révision suivante range désormais `tool_discovery` dans le résultat avant
le refus : candidats PATH, état des deux chemins CUDA historiques, sélection
effective, liste de tous les outils manquants et résolution du compilateur quand
les outils sont présents. Les replis non consultés restent explicitement tels,
sans lecture supplémentaire quand le PATH ou le premier repli a déjà gagné.
L'erreur nomme les manquants ; une liaison g++ différente
reste un refus distinct. Ce diagnostic de présence n'est pas une vérification de
version ni une certification GPU ; les commandes de version et la gate viennent
toujours ensuite.

Aucun critère n'est assoupli ou ajouté : nvcc du PATH reste prioritaire, puis
`/usr/local/cuda/bin/nvcc` et `/usr/local/cuda-12.9/bin/nvcc` exigent chacun fichier
et accès exécutable ; g++ et nvidia-smi viennent du PATH ; `/usr/bin/time` doit
être un fichier ; le compilateur doit toujours se résoudre comme `/usr/bin/g++`.
Aucun paquet n'est installé, aucun autre chemin recherché. Les nouvelles portes
utilisent uniquement des réponses de lecteurs simulés, confrontent 512 combinaisons
de disponibilité/liaison à la règle historique et passent normalement
comme sous `-O`. Elles ne constituent pas un nouveau run GPU ni une recapture de
l'ancien snapshot.

## Ordre utile et portée des mesures

La gate commence par dix fixtures, vingt comparaisons physiques complètes,
les terminaux directs K9/K10, les compteurs et les nettoyages. Deux fautes
causales sont ensuite réfutées : terminal erroné après transfert et téléchargement
refusé après un premier lot payé. Les refus CLI doivent retourner exactement 2.
Le même exécutable NVCC fournit ensuite deux bras n=200, threads pipeline=48,
résolution statique=1, `--batch=0` puis `--batch=1`. Les digests, données de
calendrier et travaux globaux/par K doivent être identiques.

Vient d'abord n=50 000, toute la tour K1..10, s=8, pipeline=48 et batch GPU.
Si la seconde est dépassée et si la fenêtre restante le permet, le repli porte
sur toute la tour K1..5. Une paire CPU48/GPU est ensuite tentée à K10 selon
le budget restant ; `static_threads=48` et `static_threads=1` sont explicitement
différents, les capacités et le nombre de workers ne sont pas comparés.
Les comparaisons s=8/10/12 utilisent n=8000 si le premier 50k dépasse 30 s ;
sinon elles peuvent rester à 50k sur le dernier K observé. Les objets et le
travail sont comparés aux mêmes n/K ; les candidats amont ne sont pas exigés
identiques. Chaque mesure reste une observation unique, pas une extrapolation.

`total_s` comprend toutes les phases rapportées, dont les copies/construction
du contexte, sa fermeture et le digest. `batch_geometry` distingue les temps
de callback du reste du Builder, sans appeler ce reste un pur calendrier.
Les capacités rapportées ne sont pas un pic de VRAM. Le reçu garde toujours
`contract_qualified=false`, même si un temps observé tombe sous une seconde.

## Coût et fermeture

La cible demeure G4 SPOT à 48 vCPU, GCE `STOP` à 3600 s, arrêt invité à 30 min,
avec 300 s au moins réservées à la fermeture. Le worker ajoute seulement une
fenêtre économique de travail de 900 s. **Ce n'est pas un plafond GCE de 15 min** :
la durée utile peut finir plus tôt et la facturation comprend démarrage et arrêt.
Chaque commande est jointe par le support gardé ; une commande encore active
à l'échéance rend la session incomplète, jamais un résultat tronqué réussi.

Le contrôleur ferme exactement la génération démarrée avec le script d'arrêt
gardé, y compris en échec. Le worker ne certifie pas lui-même `TERMINATED`.
Les autres VM sont seulement signalées. Aucun contrat 50k ou massif ne découle
du succès de cette gate bornée ; aucun dollar courant n'est supposé.
