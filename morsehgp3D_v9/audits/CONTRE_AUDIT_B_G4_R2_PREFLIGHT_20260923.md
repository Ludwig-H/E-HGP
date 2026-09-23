# Contre-audit B — paquet G4 R2 : préflight du protocole

23 septembre 2026. Lecture **sans accès invité ni mutation GCP** du paquet
LIVE `v9tower.20260923T003034Z`, préparé au commit `0b29b6c3` (arbre
`3294f8d0…`). Archive SHA-256
`ced2182d87ac55a182e8e57901b188fa1909f578591bccb0b3eae08d85fba244`,
worker SHA-256
`cc6e15898c6c18b87acf38537354bbfc53c61732075c14802fee33bd4f6a8255`.
Le manifeste et les sources empaquetées concordent ; ce constat ne valide
pas les sorties de calcul. Ne pas confondre l'étiquette de préparation
`prepared_not_executed` du paquet avec le cycle de vie LIVE de la session.

## Plan réellement envoyé

Treize cas s8, grille u18 à 1 mm, trames sans sol 08/000000,
08/000100 et 08/000200 : K5 et K10 à W48/FULL statique48,
deux répétitions chacun, plus 08/000000 K10 à W24/statique24.
Ce sont trois trames d'**une seule séquence** et non les différentes
séquences du contrat final. Aucun GPU n'est exécuté. Le plan n'est ni
un essai s10/s12 ni une preuve RAW/float32.

## Rejet certain de chaque sortie normale

La source de sonde empaquetée écrit dans `tower_work` les champs
`meb_accounting` (chaîne) et `meb_supports_by_size` (tableau). Le worker
empaqueté, `gcp-migration/tower_worker_v9.py:309–311`, exige au contraire
`_count` entier non négatif pour **tous** les champs de `tower_work`.
Une sortie normale `mhgp9_tower_probe_v3` de ce paquet atteint donc
`ValueError: probe counters tower_work` après la phase de calcul.
La porte Python utilise une fausse sonde qui omet ces deux champs.
Un rejeu local indépendant sur une vraie sortie v3 a retrouvé ce refus
après mise en concordance **en mémoire** du seul libellé d'entrée
`grid=1mm` ; cette manipulation ne change ni la géométrie ni le JSON
sur disque et ne remplace pas la démonstration par sources empaquetées.

Le worker classe alors le cas `probe_failed`, conserve ses fichiers bruts,
et poursuit le plan tant que le budget utile le permet. Sa session ne
produira donc pas de reçu `completed` si un cas normal atteint ce parseur ;
le lecteur hôte refuse en plus le statut final `probe_failed`. Les chronos
bruts éventuellement récupérés resteront **exploratoires**, jamais une
qualification G4 des contrats. Le budget utile de la session est 1 500 s,
avec plafond 600 s par cas ; l'échec de schéma ne raccourcit pas ce budget.

Au contrôle local de 00:38 UTC, le handoff est encore `targeted_running`.
L'issue réelle, la récupération et l'arrêt ciblé sont à contrôler à la
clôture ; aucune réussite ou panne algorithmiques n'est déduite ici.
Le contrôleur prévoit un arrêt ciblé en `finally`, même sur échec du
worker, mais la preuve de cet arrêt dépend du reçu final.

## Clôture observée à 00:43 UTC

Le contrôleur a finalement rendu `status=worker_failed`, `worker_status=
probe_failed`, code 1. L'archive récupérée a le SHA-256
`98f557d7e7b4e6356a8210a31fb6bde3f53b11931b93934a8ad47a561c6569a0` ;
le reçu hôte note `capture_received=true` et
`targeted_shutdown_certified=true`. `guarded_stop` sort avec code 0 et
son groupe de processus fermé. L'instance n'a pas été laissée active.
Le worker invité déclare sources, binaire et dépendances compilées stables,
`FULL_executed=true`, GPU faux ; **les treize cas** ont code de sonde 0,
mais **treize `probe_failed`**, tous et seulement avec la raison annoncée
`ValueError: probe counters tower_work`. Le reçu hôte garde
`FULL_executed=false` parce qu'aucun résultat n'a été accepté par son
lecteur : ne pas en conclure que la sonde invitée n'a pas calculé.
Une contrelecture indépendante a recoupé les **111 membres** de l'archive
au manifeste SHA, les **106 membres non-données** aux blobs Git de
`0b29b6c3`, les trois fichiers d'entrée aux octets u32 commis, puis les
hashes stdout/stderr des **46 commandes invitées** et **14 commandes
hôte** ; tous concordent. `lifecycle.txt` reste à l'état historique
`targeted_running`, mais le reçu final et `guarded_stop` certifient l'arrêt
de cette génération précise. Ne pas utiliser ce fichier mutable seul
comme autorité de coût.

Les sorties brutes de la sonde annoncent toutes `complete_relative`.
Un diagnostic en mémoire qui remplace **seulement** les deux valeurs MEB
mal typées par des entiers de test franchit `validate_probe` pour les
treize cas ; ce n'est pas une correction du paquet exécuté ni une
validation de leurs valeurs MEB. Les répétitions de chaque couple
trame/K ont catalogue, ordres et digest identiques ; sur 08/000000,
W24 et W48 K10 rendent le même objet. Les cinq premiers ordres de K10
égalent les ordres K5 de chaque trame, bien que leurs catalogues globaux
diffèrent. Ces contrôles bruts sont informatifs, **sans reçu G4 accepté**.

Médianes des deux sorties brutes W48, en secondes ; le temps de chaîne
comprend q2, q3/q4, FULL et les autres étapes natives, pas segmentation :

| Trame 08/ | Sites | K | q2 | q3/q4 | FULL | Chaîne |
|---|---:|---:|---:|---:|---:|---:|
| 000000 | 39 885 | 5 | 0,457 | 8,492 | 3,807 | 13,252 |
| 000100 | 35 551 | 5 | 0,236 | 6,713 | 3,090 | 10,455 |
| 000200 | 45 845 | 5 | 0,525 | 14,812 | 4,074 | 19,946 |
| 000000 | 39 885 | 10 | 0,780 | 23,972 | 22,985 | 50,071 |
| 000100 | 35 551 | 10 | 0,419 | 17,582 | 17,480 | 37,317 |
| 000200 | 45 845 | 10 | 0,825 | 38,854 | 22,311 | 64,289 |

Le cas additionnel 08/000000/K10 W24/statique24 annonce 56,466 s de
chaîne et le même digest que W48. Ce sont des **chronos exploratoires
sur CPU G4**, non des résultats validés par le protocole, encore loin de
1 s ; ils ne mesurent ni GPU ni plusieurs séquences. Sur 08/000000/K10,
le registre brut compte 17,947 milliards de tests ponctuels d'atlas q4,
12,392 milliards d'insertions d'IDs de **nœuds** frontaliers, 11,309
millions d'appels MEB et 403,430 millions de visites d'index pour les
intrus FULL. Le tri n'explique pas à lui seul ces coûts. Les masses
q4 exigent un changement de travail total, pas seulement W48→GPU.

Pour les six couples trame/K communs au reçu G4 R1 **accepté**, les
sorties brutes R2 ont exactement la même entrée, les mêmes compteurs
`generator`, catalogue, ordres et digest. q3/q4 est plus rapide de
24–33 % selon ces six lignes ; `src/gen` n'a pas changé entre les deux
commits, tandis que la saturation profonde q4 a été activée. C'est un
indice fort en sa faveur, **pas** une ablation contrôlée : deux sessions,
un noyau MEB et une politique FULL différents, et R2 refusé par son
protocole. Le travail restant est massif. Sur 08/000000/K10 R2, même
supprimer *tout* q3/q4 laisserait plus de 26 s dans la chaîne ; supprimer
*tout* FULL laisserait plus de 27 s. Les deux étapes doivent progresser
pour viser une seconde.

Les trois principaux chantiers issus de ces coûts sont :

1. **q4 avant son atlas massif** : mesurer en mode sans effet les gardes
   de blocs certifiés, puis seulement les raccords qui évitent vraiment
   cellules, tests et copies sans perdre de présentation. Préserver un
   minorant au centre du candidat ; retirer un site d'un fragment sans
   ce certificat ne serait pas sûr.
2. **Atlas partagé avec q3** : 78–83 % des arêtes q4 de ces lignes sont
   mixtes, et l'atlas écarte 91–96 % des graines q3 qu'il localise.
   Optimiser q4 en supprimant l'atlas sans remplacer son rejet q3
   déplacerait probablement le coût. Le seuil K−2 des arêtes q4 seules
   est exact mais ne couvre que 17–22 % des arêtes q4 ; sa part des tests
   reste inconnue.
3. **FULL par sous-phase** : séparer tri, MEB, recherche d'intrus et
   publication, puis étudier les préfixes d'intrus et des tâches
   géométriques indépendantes. À K10, les 11,309 M MEB et 403,430 M
   nœuds d'intrus ne peuvent être attribués au tri. La sortie publiée
   de 08/000000 a déjà un plancher de 1,009 Go ; le régime de plusieurs
   dizaines de millions demande aussi un format compact/streaming.

Les centres q4 de faible profondeur sont une piste collective plus
radicale, mais son algorithme exact et son coût global restent à établir ;
balayer naïvement toutes les droites entre elles redéplacerait un carré.
Une simple fusion des deux parcours du cover est de priorité inférieure
aux dizaines de milliards d'opérations d'atlas observées ici.

Avant une autre session facturée : injecter dans `validate_probe` **la
sortie réelle complète** d'une petite sonde v3, autoriser séparément les
deux champs MEB selon leur type et leurs contraintes, vérifier le mode
`atlas_saturate_deep` contre le plan, puis exercer un faux champ et un
tableau malformé. Il faut aussi arrêter une campagne après un défaut
déterministe de protocole plutôt que répéter les mêmes cas.
