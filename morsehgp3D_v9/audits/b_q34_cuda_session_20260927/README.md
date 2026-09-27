# Session isolée q34 CUDA — protocole du 27 septembre 2026

Ce protocole qualifie le [prototype par vagues](../b_q34_cuda_waves_20260927/README.md),
pas le moteur FULL. Il ne démarre rien sans `session.py --execute`.
Cadre : `phase=exploration_v9_hors_registre`,
`backend=cuda_experimental`, `profile=quantized_u18_input_only`,
`mode=q34_waves_S2_only`, `public_status=not_claimed`.

## Autorité et périmètre

Le paquet est fabriqué **depuis les blobs d'un commit publié**, jamais depuis
le worktree mutable. Inventaire exhaustif, hashes SHA256, blobs Git et arbre
sont rejugés avant toute mutation. Seule la trame entière 08/000000 sans sol,
grille 1 mm, 39 885 sites, est transportée dans un paquet privé ; aucun octet
KITTI n'est publié dans la v9. Les données proviennent du blob historique v8,
avec le masque déjà figé, sans nouvelle segmentation ni sous-échantillonnage.

Le cycle GCP est exactement `gcp-migration/full_probe_session_v7.py`, épinglé
par SHA256 : `run_session`, collecte, commandes gardées, double coupe-circuit,
récupération et `finally` d'arrêt ne sont pas réécrits. L'adaptateur remplace
uniquement son validateur de snapshot FULL par le validateur **S2** explicite.
Le nom de répertoire `full_host` est un héritage de transport, jamais une
preuve d'exécution FULL. Bootstrap désactivé : pas d'installation CUDA.

Cible unique : projet `devpod-gpu-exploration`, zone `us-central1-b`, instance
`ehgp-v7-4fa0e0789a7d5bb06b787d35`, G4 standard48 SPOT. Pas d'arrêt d'une autre
VM ni d'une génération concurrente. Le script gardé exige `TERMINATED`
avant démarrage et certifie les deux arrêts avant compilation/benchmark.

## Budget et fermeture

- Budget **utile global du worker : 360 s**, compilation et deux sondes
  comprises ; pas 360 s par commande. Dès la première erreur, aucune mesure
  suivante n'est lancée et les sorties initiales sont conservées.
- Garde GCE héritée : 3 600 s, action STOP ; garde invitée : 30 minutes.
  Les 360 s ne sont **pas** une borne sur l'allocation facturable : démarrage,
  transfert, récupération et arrêt s'y ajoutent. Le plafond invité n'est pas
  une invitation à attendre ; l'arrêt ciblé suit immédiatement le travail.
- Après 600 s d'attente, le parent envoie SIGINT au contrôleur puis le joint,
  sans tuer son `finally`. Une erreur d'écriture du PID suit aussi cette
  jointure. Toute erreur avant la relecture finale impose une vérification
  explicite de l'arrêt ; l'absence de reçu n'est jamais un arrêt implicite.
- Retour normal : arrêt certifié par le script gardé, puis lecture GCP
  indépendante `TERMINATED` pour **le même `lastStartTimestamp`**.

## Portes et mesures

Compilation autonome Release : 25 unités générateur, nouvelle sonde et
`runner.cu`, architecture120, `MHGP9_CUDA_WAVES_ENABLE_CUDA=ON`, huit jobs.
Le manifeste de dépendances réellement consommées doit inclure le `.cu`
et `witness_filter.hpp`. Les sources, dépendances et binaire sont rehashés
à la clôture. La recette complète de chaque commande est rejugée côté hôte.

La gate `--gate --cuda` doit réellement exécuter CUDA, couvrir plans et
replis, réductions de voies, sorties vides et réordonnancement, puis comparer
les sorties au natif. Un simple code0, le stub CPU ou un JSON non typé ne
permettent pas de lancer/promouvoir la mesure. Ensuite seulement :

```text
--frame <paquet-privé>/data/scene_00.u32le --cuda --q=262144 --k=5 --s=8
```

Le juge attend n=39 885, P=23 686 751, E=9 122 704, S=2 043 612, l'identité
géométrique épinglée et l'épuisement exact des vagues. Le programme compare
tous les survivants, leur ordre et leurs masques au filtre CPU natif.
Préparation CPU W4, snapshot, transferts, noyaux/compactage, tri/conversion,
destructions et référence sont publiés séparément. Le chrono total de
l'expérience **inclut la référence** ; il n'est pas le coût futur du moteur.
Pas de S3/S4, census, catalogue, hiérarchie ou contrat100ms dans cette mesure.

## Usage et preuves

`selftest.py` teste hors cloud les archives, recettes, refus, reçus et
jointures ; le selftest du contrôleur gardé reste une porte indépendante.
Les tests se relisent aussi avec Python `-O`, sans assertions désactivables.
Qualification hors cloud close : 26 contrôles positifs, 86 refus et cinq
scénarios de jointure ; normal/−O identiques, contrelecture indépendante.
Le selftest historique ajoute 11 contrôles positifs et 22 refus, avec trois
commandes simulées. Aucun de ces tests n'appelle un vrai sous-processus GCP.
Les paquets et sessions vont sous `build/`, privés, jamais dans les reçus
publics : leur parent contient une clé SSH éphémère. Ne publier ensuite que
les pièces nécessaires sans clé, octets KITTI ni adresses de transport.

Une capture G4 future doit avoir son propre dossier de reçus et son verdict
de clôture. La qualification portable et ce protocole ne sont pas un
résultat GPU. Toute correction après gel exige un nouveau commit/paquet,
une nouvelle capture et la conservation de l'échec initial.
