# Relecture des qualifications G4 des fondations v11

Source de publication figée : `6a22a9118a9dc692d3b5a34b8fd97673746697c4`. Source réellement testée en reprise3 : `a971806679a1c68519249bb28c5dac9533a43f59`. Entre ces deux commits, seuls des documents, reçus et le lecteur de reçus changent ; aucun code moteur, matrice, juge ou manifeste de mutants ne change. Les fichiers copiés sont identiques au LIVE au début et à la fin de cette lecture ([sources](sources.json), [recoupe finale](sources_after.json)).

Cadre : `exploration_v11_hors_registre`, `cpu_reference`, `quantized_u18_input_only`, audit de qualification des fondations, `not_claimed`. GCP non utilisé par cet audit. Aucun build, CTest produit, benchmark, signal ou calcul GPU lancé. Les deux lecteurs seulement relisent des archives closes en Python normal/−O : [lecteur développeur](reader_runs.json), [contrôle indépendant portable](review_runs.json). Ils passent et conservent les deux campagnes échouées.

Les trois archives rapatriées correspondent aux hashes annoncés. Leurs 91 entrées de `results/MANIFEST.sha256` sont toutes présentes et valides. Les paquets source locaux sont octet pour octet égaux à des `git archive --format=tar.gz COMMIT -- morsehgp3D_v11 gcp-migration/v11_worker.sh` indépendants. Les hashes du plan rendu et les champs source/paquet/génération du worker concordent. [Observations et calculs](observations.json) ; les archives originales sont copiées dans [sources](sources/morsehgp3D_v11/receipts/developpement_20261002/).

| Campagne | Source | Verdict global | Échecs conservés |
| --- | --- | --- | --- |
| reprise1 | `92c5af705f1d4b0183b2bf8af4bb515f58f9682d` | `failed_remote`, code 1 | Collecteur UTF-8 ; construction TSan en échec |
| reprise2 | `d4eeb5157f039812b7da045daf4c6380e191da15` | `failed_remote`, code 1 | Deux portes de harnais ; constructions 21/24 bits ; trois mutants core invalides |
| reprise3 | `a971806679a1c68519249bb28c5dac9533a43f59` | `completed`, code 0 | Aucune porte échouée ou non jouée parmi les sélections annoncées |

Chaque reçu brut local haché confirme le même projet/zone/nom et la même génération au démarrage et à la fermeture, l'état `TERMINATED`, les deux garde-fous et la libération du verrou. En reprise3 : génération `2026-10-02T03:36:40.466-07:00`, arrêt observé `03:41:51.438-07:00`, durée GCE 3600 s et commande 1500 s. Ceci relit la fermeture de ces sessions ; aucune interrogation nouvelle de l'état GCP. Le lecteur développeur reste LIVE car il exige les bruts locaux ; `review.py` relit les seules copies déposées ici.

Le compteur courant n'est pas «140 portes». Les inventaires uniques de `tests.json` correspondent exactement aux noms et issues JUnit :

| Configuration | Sélection/passes | Portée |
| --- | ---: | --- |
| GCC Release 18 | 205/205 | 130 portes de base +75 `mhgp11_reference_*` |
| GCC ASan+UBSan, GCC TSan, profils 21 et24 | 130/130 chacun | Même inventaire ; les75 références Python indépendantes sont seules exclues |
| Poison | 131/131 | Base 130 + `mhgp11_core_poison` |
| Style | 2/2 | Normal/−O, également présents dans la base |
| Mutants | 9/9 | Six contrôles de manifestes +trois campagnes |

La base 130 comporte 69 portes support, 29 core, 20 cloud, 10 num et 2 style. Ce sont des portes CTest, souvent liées ou jumelles : ni 867 faits indépendants ni 205 tests de FULL. La VM déclare GCC 11.4, CMake/CTest 3.22.1, Python 3.10.12, 48 fils ; Clang est absent et facultatif. Les oracles numériques normal/−O déclarent réellement 18/21/24 bits selon la configuration, chacun 504 géométries, 160 paires entières, 50 dégénérescences et 6164 contrôles. Les six groupes natifs numériques cumulent 145 contrôles. Le contrôle LiDAR est uniquement une sentinelle de dossier ; les données sont vides. Le sched probe est absent.

Le journal mutant final fournit 103 identités uniques correspondant exactement aux manifestes : 78 core, 9 num, 16 cloud. Causes : 98 `code`, 3 `ligne`, 2 `construction` attendues (`refus_construit_une_valeur`, `identifiants_confondus`) ; donc 101 juges exécutés, aucun signal/délai. Les trois mutants core invalides de reprise2 restent visibles dans le journal précédent. Le lanceur distingue encore `lancement_impossible` d'une mutation jugée ([kill_cause et judge_copy](sources/morsehgp3D_v11/tests/mutants/run_mutants.py), lignes260–308) ; la vieille réserve «interpréteur absent compté tué» n'est pas réouverte.

Les deux précédentes P2 ont des statuts différents :

- **Interruption globale corrigée** : [matrice](sources/morsehgp3D_v11/tools/g4_matrix.py), lignes762–777 et883–887, propage `received` au code final et interdit le résumé conforme après signal. La porte `test_g4_matrix.py`, lignes45–77, joue le vrai `main` avec appels externes remplacés ; son résultat 29 contrôles normal/−O est conservé sur G4. Cette preuve ne simule pas une interruption GCE réelle.
- **Isolation après sortie normale toujours ouverte, explicitement assumée** : `Steps.run`, lignes272–289, attend seulement le parent ; `schedule` libère l'allocation à son retour (lignes741–744). `run_probes` déclare `isolation=not_certified` (lignes695–701). Le worker ferme le groupe en fin de commande ; le reçu final confirme `group_closed=1`. Aucun chrono de la campagne ne prouve la quiescence entre configurations ni une performance isolée. [Déclaration courante](sources/morsehgp3D_v11/docs/DEVELOPPEMENT.md), lignes93–96.

Une amélioration de traçabilité reste utile au prochain run, sans annuler les accords fonctionnels : `default_build=false` délègue toutes les constructions à la matrice. Le [worker](sources/gcp-migration/v11_worker.sh), lignes604–629, ne collecte les hashes des exécutables et le CMakeCache que dans sa construction par défaut. Les trois reçus bruts ont donc `provenance.binaries_sha256={}`, compilateur/cache vides ; la matrice rapporte ses commandes et versions mais aucun hash d'exécutable. Les sources testées sont bien attribuées, les artefacts binaires exacts par configuration ne sont pas figés. Collecter ces hashes et les flags effectifs par configuration lors de la prochaine capture évitera cette lacune, sans campagne supplémentaire destinée seulement à augmenter les compteurs.

Qualification acquise : primitives exactes et propriétaire aux profils déclarés, harnais et référence bornée selon leurs sélections. Restent hors portée catalogue, index, tour FULL produit, points/EOM produit, HDBSCAN, LiDAR, GPU et contrat 100 ms. Le préflight de notre extraction avait omis la valeur par défaut `attendu=porte` ; cet échec de collecteur audit est conservé séparément, corrigé avant clôture, sans changement du produit ni des reçus développeur.
