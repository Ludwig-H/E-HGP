# Contrelecture B — index des selles isolé, essai négatif

23 septembre 2026. Reçu local
[`saddle_index_negative_20260923`](../receipts/saddle_index_negative_20260923/README.md),
commit `9b3491ec` ; GCP non utilisé. Le patch est archivé **hors produit**
et retiré du moteur courant. Il teste une seule partie de D5, la
recherche directe de la boule-selle lors de la phase 0 de FULL : ni
le saut au centre des longues descentes, ni la phase A/compacte.

Le lemme local est juste sous les invariants de support certifié : si
une boule régulière C a un support minimal `S_C` et un intérieur `I_C`,
la facette `S_C∪I_C\{z}`, `z∈I_C`, a la même miniboule C. Le patch
indexe ces facettes par ordre, empreinte puis comparaison exacte des
IDs avant de retourner le même `BallId`. Cela n'implique **aucune** borne
sur le nombre d'entrées, ni sur le coût de création/tri/jointure.

Une paire locale, sur le disque emboîté sans sol 08/000200 à 16k,
grille 1 mm, K10/s8/W8, donne :

| Mesure | Index OFF | Index ON |
| --- | ---: | ---: |
| MEB de résolution | 2 700 241 | 1 688 262 |
| Visites d'intrus | 1 337 551 | 1 337 551 |
| Entrées cumulées, dix ordres | 0 | 10 188 238 |
| Phase 0 | 2 582 ms | 2 752 ms |
| Tour FULL | 3 990 ms | 4 029 ms |

Le million de MEB évités est réel dans les compteurs, mais la phase 0
est **6,6 % plus lente** et FULL environ **1,0 % plus lent** sur cette
unique paire. La chaîne 23 209→23 126 ms varie avec l'amont sous hôte
partagé : ne pas lui attribuer un gain de l'index. Les 10,2 millions
d'entrées sont un **cumul**, non un pic mémoire ; le RSS ON/OFF ne
certifie aucune économie. Une mesure, un seul n, pas une pente
8k/16k/32k ni une qualification G4/full-frame/multi-séquence/GPU.

Le README du reçu dit « chaque coup jugé » et « exactitude confirmée ».
La couture de test du patch compare chaque hit à `static_terminal`
**uniquement** sous `MHGP9_TESTING`, mais les deux seuls JSON publiés
sont des mesures sans trace de cette porte, sans commande/build du
juge et sans compteur `saddle_verified` exporté. La baisse de MEB est
exactement égale aux 1 011 979 hits : si le juge avait fonctionné
pendant ces mesures, il aurait rappelé `static_terminal` et son MEB
à chaque hit. Les objets mesurés ont un digest et des ordres égaux,
pas un payload ON/OFF comparé champ par champ et archivé. Ne pas
transférer une validation de hit-par-hit non reçue à ce chrono.

Conclusion limitée : **fermer cet index isolé**, pas le lemme ni la
refonte D5. Réouvrir seulement si une structure moins chère ou
réutilisée, ou la combinaison avec le saut au centre, paie la création
et la jointure ; comparer alors les racines pré-lot et le payload FULL,
publier temps et mémoire par ordre ainsi que 8k/16k/32k. Sans borne
sur la taille de l'index, cet essai ne prouve pas le sous-quadratique.
