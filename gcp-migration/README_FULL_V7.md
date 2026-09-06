# Session SPOT FULL v7 — CPU48

Cette route est distincte du worker historique F et des tests de primitives
device. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Elle ne livre pas de pipeline FULL GPU.

Le [worker invité](full_probe_worker_v7.py) vérifie la double garde publiée
par [start_and_verify.sh](start_and_verify.sh), la cible et sa génération,
les métadonnées invité, le démarrage, les 48 CPU accessibles et l'arrêt
invité encore programmé. Il ne modifie aucune garde et n'appelle pas GCP.
ROOT doit récupérer les preuves puis certifier l'arrêt de la même génération
avec [stop_and_verify.sh](stop_and_verify.sh), même en cas d'échec.

Le [contrôleur hôte](full_probe_session_v7.py) réalise cette séquence pour
la cible existante `devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`.
Il reste inerte sans `--execute`. Il reçoit un répertoire privé neuf,
une clé de session déjà créée, les sources archivées, leur manifeste et
les empreintes attendues. Il inscrit la clé publique avec une durée de
70 minutes, puis laisse le script gardé vérifier son expiration exacte.
L'arrêt invité est fixé à 30 minutes. Aucune création de VM ou de disque,
aucune mutation d'une autre instance. L'archive récupérée exclut le binaire.
L'arrêt ciblé est dans un `finally` indépendant du succès des captures ;
les clés privées restent hors du dépôt et des reçus publiés.

La campagne minimale compile uniquement les dépendances v7 de la sonde
FULL, en C++20 O3 strict. Pas de v6, Boost, CMake ni installation CUDA.
Si nécessaire et explicitement demandé, le bootstrap installe seulement
g++ et GNU time, en mode non interactif et `NEEDRESTART_MODE=l`.
Le manifeste de sources, la marque de garde et le worker sont épinglés.
Le depfile et les empreintes avant/après accompagnent les captures.

Après un smoke n=8 réussi, exécuter n=50 000, WSPD s=8, tour K=1..10,
48 workers de pipeline, cache lazy de 1 000 000 entrées, proposeur MEB
sans quota d'opérations. La construction FULL et la boucle des ordres
restent séquentielles. Si K10 dépasse une seconde ou refuse, K=1..5 est
un second processus complet, pas un temps extrait du préfixe de K10.
Les seules échéances temporelles du worker sont celles de la session
facturable, avec 300 secondes réservées à sa fermeture.

Les sorties brutes, codes de retour, temps externes, RSS GNU time,
groupes de processus et digests sont conservés. Le lecteur refuse les
JSON dupliqués/non finis et recombine le digest horizontal final. Cela
vérifie les déclarations et leur intégrité, pas la géométrie ou la
complétude indépendante. Ni la verticale intégrée ni l'export industriel
ne sont mesurés ; un temps horizontal ne qualifie pas le contrat produit.

Tests purs sans GCP, compilation ni sous-processus :

```bash
python3 gcp-migration/selftest_full_probe_worker_v7.py
python3 -O gcp-migration/selftest_full_probe_worker_v7.py
python3 gcp-migration/selftest_full_probe_session_v7.py
python3 -O gcp-migration/selftest_full_probe_session_v7.py
```

Les sources de la campagne sont celles du
[lot mémoire qualifié](../morsehgp3D_v7/receipts/full_census_payload_20260906/README.md).
Les mesures CPU locales antérieures restent dans leur
[lot séparé](../morsehgp3D_v7/docs/PARALLELISME_FULL_20260906.md).
