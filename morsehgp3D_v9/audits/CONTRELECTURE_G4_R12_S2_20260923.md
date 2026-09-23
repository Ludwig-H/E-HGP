# Contrelecture A — R12, filtre q3/q4 par lots sur G4

23 septembre 2026. Reçu [R12](../receipts/g4_tower_r12_20260923/README.md)
publié par `aad3973c`, paquet source `2059189d`. Cadre :
`exploration_v9_hors_registre`, grille entière 1 mm, trois trames **sans
sol** de la seule séquence 08, s8, W48, K5/K10, `complete_relative` ;
`public_status=not_claimed`. Je n'ai ni démarré GCP ni modifié le moteur.

## Réception et portée de l'égalité

Les 270 entrées de `SHA256SUMS` passent. Le lecteur épinglé du paquet
retourne `completed` ; hôte et worker sont achevés, 14/14 cas terminés,
sept passages du filtre GPU réels, arrêt GCE ciblé `TERMINATED`. Les
165 fichiers de sources et d'entrées du snapshot vérifiés correspondent au
manifeste avant/après. Ce reçu établit donc des temps **G4 exécutés**.

Le tableau `cross_worker_comparisons` a huit lignes égales, mais elles
représentent **six couples distincts GPU–moteur**, une répétition GPU–GPU
(0→12) et une répétition GPU–moteur (0→13). `compare_cases` juge une
projection logique : entrée, sites, K, comptes du catalogue, Euler, ordres
et condensé FULL ; elle ne compare pas les clés du catalogue une par une.
La [campagne C](c_omission_20260923/README.md) compare quant à elle les
catalogues **complets** moteur/lot CPU pour 08/000000 sans sol à K5/K10
et trois entrées LiDAR 8k à ces K, avec le code antérieur `a6d81f9c`. Son mutant
`drop-one` est vu. C'est un renfort indépendant du résumé R12, mais ni
un différentiel clé par clé du **GPU** ni une preuve sur les deux autres
trames entières. Le prochain juge direct doit comparer GPU et moteur
champ par champ à 08/000000 K5/K10, puis étendre aux autres scènes ;
conserver les mutations de retrait et de masque dans le chemin GPU.

Le plan effectivement reçu contient W48 pour les 14 cas. Il est validé
comme liste normalisée dans `PACKAGE.json`, mais diffère du plan par
défaut du code `2059189d` sur les deux derniers cas (W24/W1 prévus).
Seul le SHA du `data/session_plan.json` personnalisé est conservé dans
`vm/sources_before.json` : archiver ses octets dans le prochain reçu
rendrait le snapshot régénérable depuis le reçu seul.

## Coût qui subsiste après le filtre

Calcul direct sur `SUMMARY.json`, première répétition de chaque trame/K.
La colonne « sans appel » soustrait **tout** `q34_batch.filter_ms` de la
chaîne ; « survivants + tour » additionne deux phases séquentielles déjà
mesurées. Ce sont des planchers conditionnels **pour le chemin actuel à
travail inchangé**, pas des bornes inférieures de tout algorithme possible.

| trame | K | chaîne GPU (s) | sans appel du filtre (s) | survivants + tour (s) | survivants du lot |
| --- | ---: | ---: | ---: | ---: | ---: |
| 08/000100 | 5 | 2,011 | 1,786 | **1,426** | 1 732 176 |
| 08/000000 | 5 | 2,473 | 2,268 | 1,823 | 2 043 612 |
| 08/000200 | 5 | 2,694 | 2,446 | 1,956 | 2 237 912 |
| 08/000100 | 10 | 6,633 | 6,375 | **5,333** | 3 673 260 |
| 08/000000 | 10 | 8,324 | 8,023 | 6,925 | 4 507 278 |
| 08/000200 | 10 | 8,701 | 8,386 | 7,173 | 4 927 304 |

L'appel du filtre dure 205–315 ms, dont 47–119 ms d'événements sur
l'appareil ; préparer contexte et index pendant q2 peut réduire son coût
hôte, mais même un appel gratuit laisserait toutes les chaînes au-dessus
d'une seconde. Le cœur, certificat et émissions des survivants prennent
0,83–1,18 s à K5 et 3,05–4,32 s à K10 ; la tour seule prend 0,59–0,78 s
à K5 et 2,28–2,97 s à K10. Réduire le travail exact **avant** ces
millions de survivants, ou accélérer leur traitement et la tour avec
catalogue identique, est désormais le levier principal.

## Attribution et prochain diagnostic de croissance

Les commandes R12 activent ensemble `q34_batch_filter=1` et
`q34_gpu_filter=1` face au moteur `0/0`. Le gain de chaîne de 15–25 %
mesure donc **lots + CUDA**, pas CUDA isolément. Le code offre le bras
`1/0` (lot CPU) ; une ablation `0/0`, `1/0`, `1/1` sur les mêmes
entrées, K, s, W et juges complets séparerait coût de l'architecture,
gain de l'appareil et coût de transport. Publier aussi le travail et la
mémoire des rectangles et survivants, que le port matérialise encore.

R12 n'a ni moitiés/quarts ni niveaux de densité. Pour juger la croissance
de **S2** plutôt que celle du binaire v12, reprendre les sept secteurs
physiques par plans capteur et les échantillons emboîtés 1/4, 1/2,
entier, avec nombre réel de sites, formes du cœur, survivants,
catalogue, travail de filtre, CPU, mur et RSS. Faire d'abord le brut
entier et le sans-sol séparément ; l'existant [matrice brute
K5/K10](lidar_raw_k10_sectors_20260923/README.md) et le [rejeu batch
CPU d'un quart](q34_batch_density_quarter_20260923/README.md) donnent
les contrôles d'identité. Ces pentes finies diagnostiquent les régimes
LiDAR ; elles ne prouvent pas à elles seules une borne asymptotique.
