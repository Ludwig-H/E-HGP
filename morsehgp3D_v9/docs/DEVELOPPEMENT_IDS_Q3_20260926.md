# Port q3 → catalogue : protocole v30, option désactivée par défaut

26 septembre 2026. Profil entier u18 / grille 1 mm. Ce document décrit
l'interface et les portes du port ; il ne constitue pas un reçu G4 ni une
qualification du contrat FULL 100 ms.

`ChainOptions::q3_interior_payload=false` reste le défaut. La sonde accepte
`--lever=q3_interior_payload=1` uniquement sur un chemin avec
`q34_batch_q3`. Le producteur conserve les IDs strictement intérieurs des
émissions q3 dans un sidecar possédé ; les records `LaneRecord` restent de
128 octets. q4 et les producteurs sans paquet conservent leur traitement
actuel. Le transport, les réordonnancements et l'import restent à payer
dans la même chaîne FULL explicite, sans modifier son périmètre temporel.

## Publication et lecture

La sonde publie `mhgp9_tower_probe_v30`, le plan
`mhgp9_tower_plan_v7`. Trois compteurs sont ajoutés au catalogue :

- `payload_keys` : clés de représentant canonique q3 régulier importées ;
- `payload_ids` : somme de leurs profondeurs, donc nombre d'IDs importés ;
- `payload_fallback_keys` : clés de représentant canonique q3 recensées
  globalement, notamment CPU tail sans paquet ou coquille étendue.

q2/q4 ne sont pas comptés comme replis de cette option. Pour une chaîne
complète sous option, `payload_keys + payload_fallback_keys` égale le
nombre de boules q_min=3 ; le nombre d'importées ne dépasse pas celui des
supports réguliers q3. Les IDs sont bornés par `(K_effective−2)·payload_keys`.
Sans option, les trois compteurs sont nuls.

Les raisons de complétude disent `…_payload`, ou
`…_sealed_in_process_payload` sous sceau. Elles décrivent le mécanisme
autorisé, pas la preuve qu'il a servi : seul `payload_keys>0` établit un
import effectif. Les raisons historiques sont conservées hors option.
Le juge des voies compare les IDs et l'objet importé au census global
indépendant ; son coût appartient au préflight, pas aux mesures ON/OFF.

## Préflight et campagne dédiée

Le préflight du premier cas ON exige au moins une clé **et un ID**
importés. Il garde le juge des certificats et des voies, le témoin moteur,
le cas de capacité réduite, et ajoute `preflight_payload_off` : mêmes
options et mêmes juges, seule l'option payload est coupée. Les trois
condensés, les comptes d'objet, tout le registre géométrique et les
compteurs du générateur doivent être identiques. Sous juge, les visites
du census de contrôle doivent également correspondre. Les commandes,
sorties et résumés sont relus à la réception avec les mêmes règles.

Avant ces préflights, la même compilation stricte construit
`mhgp9_gpu_interior_payload_gate`, exécuté avec `--device`. Ses cas
fusionnés/non fusionnés, épinglés/pageable et différés vérifient les
associations record/sidecar sur CUDA. Compteurs non vides, commande,
source compilée et stabilité du binaire sont relus ; un échec interdit
les cas LiDAR. Ce contrôle n'est pas un chrono de tour.

Les cas mesurés ne portent pas le juge. Les paires ON/OFF ont le même
nombre de workers et les mêmes autres options ; leurs visites de census
peuvent diminuer, jamais augmenter sous cette comparaison contrôlée.
Chaque cas ON exige dans le plan un OFF identique et un témoin moteur
pour la même entrée/K/s et les mêmes certificats. Les nouvelles mesures
ne remplacent aucune archive historique.

`tower_snapshot_v9.py --payload-plan` sélectionne 26 cas :

- 08/000000 sans sol K5/s8 : ON0, OFF0, OFF1, ON1, moteur ;
- ON/OFF/moteur : 00 K10/s8, 01 et 02 K5/s8, 00 K5/s10 et s12 ;
- ON/OFF/moteur : trame brute b00 K5 et K10/s8.

Tous ont `frames=1`. Le plan historique conserve explicitement payload
désactivé ; il n'est pas lancé en plus de cette campagne. Les entrées
00/01/02 désignent toujours trois trames de la même séquence 08, pas
trois séquences. Aucune conclusion multi-séquence n'en découle.

## Réparation de la publication multi-trame v29

Le correctif WIP du développeur est repris : épingles obligatoires pour
chaque entrée/K du plan réel, vérification de la raison du sceau et borne
basse des contrôles de supports sous sceau.

Pour `--frames=N`, `frames.results` publie désormais **chaque** statut,
raison, K effectif, les trois condensés, six comptes catalogue et les
comptes de tous les ordres. La première entrée doit égaler le corps
publié. `same_object` est recalculé depuis ces preuves, et non admis comme
un booléen d'autorité. Un échec ou une divergence tardifs empêchent la
validation du processus complet même si son condensé de tour est resté
inchangé. Les listes de temps restent associées aux mêmes passages.
Cette réparation n'ouvre pas une nouvelle campagne de latence résidente.

## Portes et clôture

Les autotests Python couvrent les dépendances de l'option, la paire OFF
absente ou différente, les compteurs invalides, la dérive géométrique,
les trois condensés et les erreurs/omissions de passages ultérieurs. Un
scénario cloud **simulé** vérifie le nouveau préflight et la réception,
jusqu'au rejet d'un résumé payload falsifié ; il ne contacte pas GCP.

Le paquet reste construit depuis un commit, jamais depuis des sources
moteur sales. L'autotest intégral de snapshot/réception doit donc être
rejoué après le commit cohérent du port v30, en Python normal et `-O`.
Les compilations strictes, portes natives, comparaisons d'IDs et
`BallData`, puis les reçus G4, restent les autorités de qualification.
Ne pas confondre la présence de cette documentation avec leur réussite.
