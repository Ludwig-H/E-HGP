# Protocole apparié G4 des survivantes S2 — 27 septembre 2026

Ce dossier prépare l'exécution du
[comparateur publié](../b_q34_survivors_compare_20260927/README.md)
à `8dc572498fa8fed24f24d17e3ef9dec30233e818`. Il s'agit d'une expérience
S2, ng00 sans sol à 1 mm, hors registre, `public_status=not_claimed`.
Cette qualification locale n'utilise pas GCP et ne produit aucune mesure
GPU. Le responsable principal est seul propriétaire du lancement cloud,
après publication du protocole et vérification des sources publiées.

## Cycle de vie conservé

Port explicite du
[protocole résident](../b_q34_resident_session_20260927/README.md)
publié à `03decc16c`. Le moteur et les protocoles antérieurs restent inchangés.
Le helper `gcp-migration/full_probe_session_v7.py` est chargé seulement si
son SHA-256 vaut
`177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8`.
Ses démarrage, double garde, récupération et arrêt ciblé restent ceux du
helper. L'adaptation porte sur `validate_snapshot`. Les AST de `wait_owned`
et `owned_controller` sont comparés à ceux du protocole CUDA historique,
dont `session.py` est épinglé à
`20e1b9fc6a9aea435ac6876b1eef1723a76750d8f460367fa9ada50ad7fa01a0`.

La cible fixe est le projet `devpod-gpu-exploration`, la zone
`us-central1-b`, l'instance G4 spot `ehgp-v7-4fa0e0789a7d5bb06b787d35`.
Le wrapper est inerte sans `--execute`. Il ne crée pas d'instance et
n'installe pas CUDA. La garde invitée est de 30 minutes, la garde GCE de
3 600 secondes avec action STOP. Le budget utile global est de **450 s**,
compilation, portes et quatre processus de trame compris, avec une marge
de fermeture d'au moins 300 s avant l'échéance gardée.

Le wrapper attend 600 s, envoie ensuite SIGINT au contrôleur si nécessaire
et le joint jusqu'au retour de son `finally`. Il ne tue pas ce nettoyage.
La clôture exige l'arrêt ciblé certifié puis une lecture indépendante
`TERMINATED`, pour le même `lastStartTimestamp`. L'arrêt d'une autre
génération ne clôt pas cette session.

## Sources et compilation propres à la session

`package.py` lit les blobs d'un commit Git complet, vérifie leur identité
et produit un paquet reproductible. Aucun octet mouvant du worktree n'est
pris comme source de compilation distante. Le responsable vérifie que le
commit a été publié avant le lancement ; la seule présence locale d'un
objet Git ne prouve pas sa publication. Les cinq scripts exécutants
`common/package/session/worker/compile_contract` doivent correspondre aux
octets de ce commit. Le manifeste est exhaustif ; liens, traversées,
membres inconnus et doublons sont refusés.

La VM configure un répertoire neuf `survivors_build`, Release CUDA 120,
avec `MHGP9_GEN_LIBRARY` vide : aucune ancienne archive GEN n'est réutilisée.
Avant le build, `compile_contract.py` exécute `-M` avec les options réelles
de chacune des **31 unités** : cinq du comparateur, une de la porte high-u64
et 25 de GEN. Les trois unités de la cible historique configurée
`mhgp9_q34_filtered_resident` sont explicitement exclues du build.

`compile_before.json` conserve chaque commande et son flux de dépendances,
les sources/en-têtes hachés, compilateurs/outils CUDA, configuration CMake,
options/flags et fichiers `.rsp`, ainsi que les archives externes résolues
au lien. La résolution respecte l'ordre des `-L` et la préférence `.so`
avant `.a` dans chaque répertoire ; un lien CUDA dynamique inattendu est
refusé. Après compilation, `compile_after.json` ferme les 31 `.o` et leurs
`.o.d`, l'archive `baseline/libresident_gen.a` et les deux exécutables.
La même clôture est vérifiée après les mesures. Les dépendances sous la
racine source doivent appartenir au manifeste du commit. Ce contrôle ne
prétend pas constituer un environnement système hermétique.

Le préflight local conservé dans
`/workspaces/E-HGP/build/v9-survivors-session-preflight-20260927` a produit
les deux exécutables et sa clôture en lecture retrouve 31 objets et
1 135 dépendances, sans reconstruction. Ses fichiers `audit_preflight_*`
restent les traces d'une préparation mutable, **sans autorité de
qualification**, sans mesure GPU et sans transfert à la future compilation
VM. Les preuves de compilation du comparateur publié restent distinctes.

## Recette distante et sortie attendue

Après contrôle des gardes, inventaires CPU/GPU et versions des outils :

1. Configuration neuve, pré-épinglage `-M`, build `-j8`, clôture compilée.
2. `mhgp9_survivors_device_gate --cuda`, dans `device_gate/`.
3. `mhgp9_survivors_compare --gate --cuda` : 52 lots et 208 opérateurs.
4. ng00 W4, `--first baseline`, puis W4, `--first candidate`.
5. ng00 W48, `--first baseline`, puis W48, `--first candidate`.

Ces quatre dernières commandes sont quatre processus distincts, chacun
ABBA ou BAAB. Les clés du reçu sont `w4_baseline`, `w4_candidate`,
`w48_baseline`, `w48_candidate`. Tout échec interrompt les commandes
suivantes. Les deux portes doivent attester `cuda_executed=true` avant
toute mesure. Le gate high-u64 conserve 24 cas, dont six refus, six lots
vides et six singletons ; ses autres catégories doivent être non vacuelles.

Chaque trame paie un front natif W1 et conserve un oracle natif W4 commun
aux quatre passages. W4/W48 désigne la préparation d'arène. K5/s8 et
Q=Qr=262144 sont fixes. Ces capacités de scratch ne plafonnent aucune
recherche. L'entrée entière ng00 compte 39 885 sites, 478 620 octets,
SHA-256 `0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf`,
hash programme `9245360528374966039`. Segmentation et préparation hors
ligne sont exclues du chronométrage S2.

Les quatre passages et les quatre processus doivent avoir le même travail
exact : masses, compteurs physiques, géométrie et arène. Le validateur
exige notamment R=3 133 819, P=23 686 751, E=9 122 704, S=2 043 612,
digest `5324876275161635233` et 537 798 656 visites de paires. Il vérifie les
partitions par voie, les nombres de vagues et les bornes mémoire. Le
programme compare aussi chaque sortie orientée et ordonnée, masques
compris, à l'oracle natif commun ; le digest ne remplace pas cet oracle.

`first_cuda_call` vaut vrai seulement à la position 0 ;
`first_implementation_call` vaut vrai aux positions 0 et 1. Les autres
positions sont des répétitions dans le même contexte de processus. Les
allocations privées sont renouvelées, sans `cudaDeviceReset`. Ces quatre
passages ne sont donc pas quatre démarrages CUDA froids. Dans la porte
appariée complète, ces compteurs valent respectivement un et deux.

`adapter` est la borne englobante de l'opérateur, avec sortie native encore
retenue ; comparaison et destruction de cette sortie sont publiées
séparément. `adapter_plus_native_release_noncontiguous` est une somme non
contiguë. Le total du harnais inclut préparation commune et oracle ; il
n'est pas le temps d'un opérateur ni d'une tour. Les sous-champs emboîtés
ne sont pas additionnés une seconde fois. Les ledgers mémoire ne sont
ni un RSS ni un pic global de processus ; la baseline n'exporte pas le
ledger de sortie ajouté au candidat. Voir le contrat détaillé du comparateur.

## Qualification hors ligne

Les selftests bloquent tout sous-processus réel et n'utilisent que des
reçus synthétiques et des fichiers temporaires. La capture
[checks/r2/receipt.json](checks/r2/receipt.json) exécute six commandes :
selftest, helper historique et wrapper inerte, chacun en normal et `-O`.
Les tests du protocole comptent **73 positifs et 204 refus**, cinq scénarios
d'attente/annulation, dont des défauts d'écriture PID et d'installation des
handlers. Les tests du helper comptent 11 positifs et 22 refus. Les
mutations incluent transport, ordre des portes, compteurs, chronos,
sources, `.rsp`, objets manquants et résolution des archives CUDA.
Les captures épinglent aussi l'interpréteur, cette notice et tous les scripts
consommés. Les lecteurs LIVE normal/`-O` rejugent les sources actuelles.
La première capture `checks/r1` a passé les mêmes six commandes ; elle
reste historique après correction typographique de cette notice, qui fait
elle-même partie des sources épinglées. La capture r2 est l'autorité LIVE.

```sh
python3 -B morsehgp3D_v9/audits/b_q34_survivors_session_20260927/run_local.py morsehgp3D_v9/audits/b_q34_survivors_session_20260927/checks/r2
python3 -B morsehgp3D_v9/audits/b_q34_survivors_session_20260927/run_local.py morsehgp3D_v9/audits/b_q34_survivors_session_20260927/checks/r2 --readback
python3 -B -O morsehgp3D_v9/audits/b_q34_survivors_session_20260927/run_local.py morsehgp3D_v9/audits/b_q34_survivors_session_20260927/checks/r2 --readback
```

Après publication, le responsable peut préparer un paquet neuf avec
`package.py --commit COMMIT_COMPLET --output DOSSIER_NEUF`, puis exécuter
`session.py --execute --package PAQUET --session-dir DOSSIER_NEUF`.
Les tests locaux ne remplacent aucune porte GPU de cette recette. Aucun
gain S2, contrat 100 ms, tour FULL ou résultat G4 nouveau n'est acquis par
la qualification du protocole.
