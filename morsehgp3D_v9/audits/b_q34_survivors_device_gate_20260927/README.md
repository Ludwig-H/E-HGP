# Porte du collecteur S — compilation qualifiée, GPU non exécuté

27 septembre 2026. Suite bornée au [port de sortie résident](../b_q34_resident_survivors_20260927/README.md).
Aucun moteur modifié, aucune session GCP et aucune exécution GPU ici.
Le but est de préparer une petite porte **device réelle** à inclure dans la
prochaine session G4, avant la comparaison appariée sur trame entière.

`device_gate.cu` inclut explicitement le fichier CUDA gelé du port précédent
et appelle ses vrais `append_survivors` et `finish_survivors` dans la même
unité de traduction. Il n'y a ni recopie du collecteur ni seconde liaison
du backend, et cette entrée n'est pas une API produit. Le moteur géométrique
n'est pas appelé par les fixtures de collecte.

Le nouveau CMake compile avec CUDA 12.9, C++17 et architecture 120. La CLI
distingue `--host-gate` (validation portable des fixtures, **pas GPU**) de
`--cuda` (allocation/copieuse/tri/validation réels device). Aucun appel
CUDA d'initialisation ou noyau n'est demandé par le premier mode.

Fixtures : clés de part et d'autre de 2^32 et de 2^63, jusqu'à UINT64_MAX−2,
payloads orientés distincts et masques 2/4/6, vagues vides et clairsemées,
S=0/S=1, quantum 1/7/64, avec et sans téléchargement des clés diagnostiques.
Les réponses sont comparées à une map ordonnée indépendante. Les inversions
entre vagues non vides sont explicitement comptées. Les grandes **valeurs de
clés** ne signifient pas que des milliards de sorties ont été allouées.

Le mode CUDA ajoute six refus : doublon inter-vagues et masque invalide,
pour chaque quantum. La sortie sentinelle doit rester inchangée, toutes les
allocations doivent être libérées, et le ledger doit revenir à zéro. Les
portes positives contrôlent le pic exact par phases, incluant ancien+nouveau
pendant la croissance, Edge+split puis scratch de radix, et les bytes de
copies et de téléchargement. Il s'agit des allocations possédées par cette
porte seule, pas du RSS ou du pic d'une tour.

Les compteurs `cases/items/empty/single/empty_waves/high32/high63`, inversions,
croissances et bytes publiés couvrent les **cas positifs seulement**.
`refused` compte séparément les négatifs ; `peak_bytes` prend le maximum des
allocations possédées des deux groupes. Les transferts des cas refusés ne
sont pas présentés comme inclus dans les bytes positifs.

[Qualification locale r1](checks/r1/summary.json) close : dix commandes,
compilation nvcc 12.9.86, `--host-gate` et deux refus CLI causaux (exit 2).
Lecteurs LIVE normaux et `python -O` PASS. Résultat hôte :
24 cas, 642 éléments, six sorties vides, six
singletons, 1226 vagues vides, 624 clés >2^32−1, 606 clés ≥2^63,
594 inversions entre vagues et 606 inversions au total. Les compteurs
allocations/transferts device et refus CUDA restent zéro dans ce mode.
Cela valide les fixtures et le comparateur portable, **pas le radix device**.

Build qualifié neuf et immuable :
`/workspaces/E-HGP/build/v9-survivors-device-gate-20260927-r1`.
SHA256 du binaire :
`14664e802287141762df0b013de4c2f0d50338259ab52d08b91b737ccb8628ac`.
Le préflight exploratoire antérieur demeure distinct :
`/workspaces/E-HGP/build/v9-survivors-device-gate-20260927-preflight`.
La source CUDA incluse est exactement celle de la qualification r1 du port
précédent, SHA256
`af9259f6104cf0a0f12116af6be2860311b2cc32a989cf6fe70b78529bc86666`.

Le [runner](run.py) porte explicitement le mécanisme du runner précédent
épinglé. Avant le build : 1836 pins de sources/outils/archives, puis le vrai
compilateur rejoué avec `-M` et les flags de `compile_commands.json`, donnant
978 dépendances. Les trois fichiers réponse CMake (`includes_CUDA.rsp`,
`objects1.rsp`, `linkLibs.rsp`) sont épinglés **avant** ce scan, recontrôlés
après puis après compilation. Les chemins de cudart_static/cudadevrt sont
résolus depuis les vrais arguments de lien et les `-L`, et doivent désigner
les archives déjà épinglées. L'objet `.o`, son `.o.d`, les options de lien,
les réponses et le binaire sont fermés dans 989 pins de build. Les
dépendances compilées doivent être incluses dans l'inventaire antérieur.
Cela n'est pas un environnement système hermétique.

Le [test séparé du lecteur](reader_checks/r1/summary.json) passe en normal
et `-O` : un reçu valide et huit falsifications par mode. Refus causaux de
statut échoué, fausse exécution GPU (capture ou gate), compteur flottant, chronologie
prépin/build inversée, inventaire de réponses forgé, objet compilé omis
et crash présenté comme refus CLI. Les stdout altérés sont repinnés ;
seules des copies temporaires des reçus sont modifiées, jamais les builds
ou captures d'origine.

Cette fermeture neuve ne répare pas rétroactivement l'absence de pin propre
du fichier réponse dans l'ancien build CUDA ; voir l'[addendum](../b_q34_resident_survivors_review_20260927/RESPONSE_FILES.md).
Les sources et reçus du premier port restent gelés.

Statut GPU : aucun protocole cloud ni appel `--cuda` exécuté pour cette
porte. Le prochain lanceur doit exiger `mode=cuda` / `cuda_executed=true`
avant de juger une porte GPU réussie. Aucune valeur de scratch ou de pic
device n'est inventée depuis le mode hôte. Aucun gain G4 ou contrat FULL
ne découle de la compilation locale.
