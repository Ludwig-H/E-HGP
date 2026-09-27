# Protocole G4 du prototype résident — 27 septembre 2026

Ce dossier prépare une expérience **S2 seulement**, pas une tour FULL.
Il ne modifie ni le moteur, ni le protocole précédent, ni le cycle de vie
des VM. Aucun appel GCP n'a été effectué pour cette qualification locale.
Le responsable principal est l'unique propriétaire d'une éventuelle session
cloud, après publication et vérification des sources.

## Origine et frontière de ce port

Port **explicite**, dans cinq nouveaux fichiers, du protocole
[`b_q34_cuda_session_20260927`](../b_q34_cuda_session_20260927/README.md)
publié à `a7e80d7f9`. Les changements portent sur les sources transportées,
le CMake et le binaire de
[`b_q34_filtered_resident_20260927`](../b_q34_filtered_resident_20260927/README.md),
le schéma de sortie, les recettes et leur validation. Pas de redirection
cachée vers l'ancien binaire, de modification de son reçu, ni de transfert
automatique de qualification.

Le helper `gcp-migration/full_probe_session_v7.py` reste importé avec SHA-256
`177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8`.
Ses démarrage, double garde, récupération et arrêt ciblé ne sont pas
remplacés. Comme dans le protocole source, l'adaptateur fournit uniquement
le contrat de vérification du snapshot à `validate_snapshot` ; les fonctions
de cycle de vie ne sont pas modifiées. Les fonctions locales
`wait_owned` et `owned_controller` sont comparées par AST à celles du
protocole source figé, dont `session.py` porte le SHA-256
`20e1b9fc6a9aea435ac6876b1eef1723a76750d8f460367fa9ada50ad7fa01a0`.

## Séquence fixée

Une seule cible : projet `devpod-gpu-exploration`, zone `us-central1-b`,
instance `ehgp-v7-4fa0e0789a7d5bb06b787d35`. Le wrapper reste inerte sans
`--execute`. Il ne crée pas de VM, n'installe rien et exige CUDA déjà
disponible. Le helper existant pilote le cycle de vie de la cible.

Les commandes distantes sont exactement, dans cet ordre :

1. Lecture de la garde invitée, versions g++/CMake/nvcc, inventaire GPU/CPU.
2. Configuration Release CUDA, architecture 120, puis compilation `-j8`.
3. `mhgp9_q34_filtered_resident --gate --cuda`.
4. Trame ng00 complète, K5/s8, `--qr 262144 --q 262144 --workers 4`.
5. Même trame et paramètres, `--workers 48`.

Un échec de commande ou de verdict coupe immédiatement la séquence.
La mesure W48 n'est jamais exécutée après un échec W4 ; les deux sont
nécessaires pour un reçu final `completed`. Les deux largeurs concernent
**la préparation d'arène uniquement** : front CPU W1 et référence native
CPU W4 restent explicitement indiqués. L'ordre W4 puis W48 n'est pas un
appariement ABBA ni une preuve de gain stable.

Q et Qr sont des capacités de scratch, pas des quotas de recherche.
Toutes les plages sont épuisées. Ces valeurs ne sont pas présentées comme
un optimum GPU.

## Entrée et sortie exigées

Le paquet utilise les objets Git d'un **commit complet publié**, et jamais
les octets mouvants du worktree. Chaque membre régulier est contrôlé ;
liens, traversées de chemin, membres inconnus ou dupliqués sont refusés.
Le manifeste SHA-256 est exhaustif ; les blobs Git sont aussi rejugés par
leur identité SHA-1 Git. Les quatre scripts exécutants doivent être
identiques aux octets publiés. Le paquet contient explicitement les
dépendances historiques d'arène/vagues ; cela ne transfère pas leurs
résultats aux nouvelles sources.

L'entrée est `scene_00_grid/full.u32le`, ng00 sans sol, grille 1 mm,
39 885 sites, **tous conservés**. Taille 478 620 octets ; SHA-256
`0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf` ;
hash d'entrée du programme `9245360528374966039`.
Segmentation et préparation hors ligne ne font pas partie de cet essai S2.

La porte doit annoncer `cuda_executed=true`, des cas portables et CUDA
distincts et toutes ses catégories non vides. Une sortie du stub CPU ne
peut pas devenir une preuve GPU. Les contrôles s'appuient sur le programme
qui compare les masques rectangles, les survivantes ordonnées et leurs
masques, ainsi que les masses et visites natives.

Pour chaque largeur, le validateur exige notamment :

| Champ | Valeur |
| --- | ---: |
| P logique, après filtre rectangle | 23 686 751 |
| P3 / P4 | 17 732 794 / 23 446 295 |
| E, vraies requêtes paires | 9 122 704 |
| E3 / E4 | 6 667 094 / 8 403 884 |
| S ordonné, avec masques | 2 043 612 |
| Digest natif | 5 324 876 275 161 635 233 |

Les masses brutes avant filtre rectangle restent séparées de P. La somme
`planned+fallbacks` doit couvrir tous les rectangles ouverts ; la somme
brute est bornée ici par n(n−1)/2 parce que l'entrée vient de la WSPD native,
pas pour une API de rectangles arbitraires avec répétitions. Les nombres
de vagues doivent couvrir exactement R et E. Les compteurs et durées sont
typés ; booléens déguisés en entiers, NaN, infinis et durées négatives sont
refusés.

Le digest est une vérification supplémentaire de la trame attendue,
**pas un remplacement de la comparaison native champ à champ** exécutée
par la sonde. Une fois rapatrié, chaque stream, recette, intent, statut,
dépendance compilée, provenance et identité du binaire est revérifié.
Les valeurs attendues viennent de la
[sortie ng00 historique](../../receipts/q34_cuda_g4_20260927/r1/vm/ng00.stdout),
pas de temps ou d'une qualification transférés au nouveau prototype.

## Temps et mémoire

`adapter` paie ouverture/init CUDA et copie/index upload, filtrage rectangle,
compaction, arène CPU, consommation GPU, retour des survivantes, tri et
conversion, puis destruction des propriétaires Prepared/Session/Decision.
La sortie S reste possédée après cet intervalle. Aucun contexte CUDA global
n'est détruit par cet adaptateur.

Le front CPU est publié séparément, mais il faut l'ajouter pour juger
front+S2. `total` paie aussi lecture, index, oracle CPU, comparaison et
destructions finales : ce n'est donc **pas** le coût candidat. Le lecteur
contrôle les sommes des phases extérieures non chevauchantes ; les
sous-phases emboîtées ne sont jamais ajoutées une seconde fois.

Les champs de mémoire conservée, pic possédé de construction, allocations
device et transferts ne forment pas un RSS ni une mesure de pic global.
Les allocations communes déjà résidentes sont notamment incluses dans
les pics device rectangle et paire : ne pas additionner aveuglément ces
trois champs. Le sens détaillé des champs appartient au prototype associé.

## Budget et clôture

Le budget utile **global** est de 360 s, compilation + porte + deux essais
compris, en respectant aussi la marge avant l'échéance gardée. La garde
invitée s'étend sur 30 minutes ; la garde fournisseur et son identité de
génération restent celles du helper épinglé. Le wrapper attend 600 s avant
SIGINT puis joint le contrôleur jusqu'à la fin : il ne tue jamais son
`finally`. Les chemins d'échec d'écriture du PID ou d'installation des
handlers sont aussi testés.

La clôture exige l'arrêt ciblé certifié, puis une lecture indépendante
de la même instance `TERMINATED`, avec le **même** `lastStartTimestamp`.
Une session interrompue ou un seul essai réussi ne devient pas un reçu
complet. Les sources, dépendances réellement compilées et binaire sont
hachés avant/après ; sorties et première erreur restent conservées.

## Qualification locale et lancement ultérieur

Les tests purs utilisent exclusivement des archives temporaires et des
reçus **synthétiques**, signalés comme tels. Tous les appels réels à
`subprocess.run/Popen` sont interdits à l'intérieur du selftest ; Git,
contrôleur et fautes sont simulés. Le lanceur de capture utilise seulement
Python pour exécuter ces tests et vérifier le mode inerte. Il ne lance
jamais le protocole cloud.

Capture [checks/r1/receipt.json](checks/r1/receipt.json) close : quatre
commandes, selftests normal/−O identiques (**46 positifs, 117 refus**),
cinq scénarios d'attente/annulation et helper historique simulé
11 positifs/22 refus. Les lecteurs vivants normal/−O passent aussi.
Les sources consommées et l'interpréteur sont épinglés ; les reçus
synthétiques des tests ne contiennent aucune mesure GPU. Cette clôture
porte seulement sur le protocole, pas sur la porte CUDA à exécuter ensuite.

```sh
python3 -B morsehgp3D_v9/audits/b_q34_resident_session_20260927/run_local.py morsehgp3D_v9/audits/b_q34_resident_session_20260927/checks/r1
python3 -B morsehgp3D_v9/audits/b_q34_resident_session_20260927/run_local.py morsehgp3D_v9/audits/b_q34_resident_session_20260927/checks/r1 --readback
python3 -B -O morsehgp3D_v9/audits/b_q34_resident_session_20260927/run_local.py morsehgp3D_v9/audits/b_q34_resident_session_20260927/checks/r1 --readback
```

Après clôture locale et publication, le responsable pourra produire un
paquet neuf avec `package.py --commit COMMIT_COMPLET --output DOSSIER_NEUF`,
puis utiliser `session.py --execute --package PAQUET --session-dir DOSSIER_NEUF`.
Ces commandes ne sont pas exécutées par cet audit. Une compilation CUDA
ou une porte device non passée ne peut pas être remplacée par les tests
purs de ce dossier. Ni gain S2, ni contrat 100 ms/FULL, ni résultat G4 nouveau
ne sont encore acquis ici.
