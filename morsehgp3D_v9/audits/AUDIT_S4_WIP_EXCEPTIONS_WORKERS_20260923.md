# S4a : réception v20 corrigée, portes de panne et de croissance

24 septembre 2026. Le [reçu G4 R15](CONTRELECTURE_G4_R15_S4A_20260923.md)
mesure le port S4a construit depuis `8b47a75a9`. Le correctif ultérieur
**`507580243` sur `main`** modifie la réception et les gates, pas les
prédicats géométriques du port. Les trois anomalies de comptabilité et de
réception relevées pendant le WIP sont maintenant **closes à la lecture
du code** : elles ne doivent plus figurer comme défauts actifs.

- `run_lanes_batch_host` capture allocations, initialisation de bloc et
  commit ordonné, réveille les autres workers, les joint avant relance et
  revient sans slab pour zéro arête (`lanes_host.hpp:45,66–141` ;
  `3765080cf`). Le ledger `both_edges` est ajouté avant la séparation
  q3 décidée/reportée (`wspd_q34.cpp:1694–1705`).
- La réception impose `lanes_asked ≤ q3_edges ≤ lanes_asked + deferred`
  (`tower_worker_v9.py:623–627`, depuis `3765080cf`). `507580243`
  retire le faux refus de tout report q3 sous 65 536 sites : un cover
  peut tenir dans son slab de sites mais dépasser ses records ou l'arène.
  Le selftest accepte maintenant ce cas.
- `507580243` applique `validate_preflight_work` au jumeau moteur **dans
  le worker et le contrôleur**. Le census q3 de feuille et le cache de
  témoins y sont exigés quand actifs ; R15 en avait effectivement
  **82 056** et **321 903**. Un mutant de census nul arrête la campagne
  avant le premier cas. Le gate de chaîne compare aussi `q4_emitted` du
  bras à capacité réduite et exige `tails + both > asked`.

La porte de chaîne antérieure a été exécutée localement sur 32 cas
(`--n=1000`, binaire SHA-256 `a576edd6…`, code 0) :
`asked=368886`, `tails=174420`, `both=281530`. S3 CPU ne reporte aucune
arête dans cette porte ; par inclusion–exclusion, au moins **87 064
occurrences d'arêtes-cas** ont simultanément q3 reportée et q4 ouverte.
La nouvelle inégalité du gate préserve cette non-vacuité si la fixture
change. Le correctif ne transforme pas ce gate hôte en essai G4 ou en
preuve des clés non proposées.

## Pourquoi le report sous 65 536 sites était légitime

Le plafond de **65 536** concerne le cover **en sites par arête**, pas
les **4 096 records par arête** ni l'arène globale. Le cas exact du
gate direct ajouté par `507580243` emploie `E=4097` grappes éloignées,
chacune avec `a=(−10,0,0)`, `b=(10,0,0)` et cinq points `(0,u,v)` pour
`(u,v)=(13,0),(0,13),(−13,0),(0,−13),(5,12)`. Chaque triangle
`abx` est strictement aigu et son autre point `y` a une puissance
`69(1−x·y/169)>0` ; il donne cinq records q3 distincts à K5. Les
**28 679 sites** sont sous le plafond de cover, mais les **20 485
records** dépassent l'arène par défaut `4E+4096=20 484` d'une unité :
une arête est correctement reportée et reprend sur CPU. La fixture
fournit directement ces arêtes à l'API S4a ; elle ne prétend pas que la
WSPD les sélectionne.

**Fixture hôte plus compacte proposée, non exécutée.** Poser
`r=1105=5·13·17`, `L=1000`, `a=(−L,0,0)`, `b=(L,0,0)` et prendre tous
les **108 points entiers** `(0,u,v)` du cercle `u²+v²=r²`
(`4·3³=108`, confirmé par énumération). Placer 40 copies aux centres
`(1000+5000c,1105,1105)`, `c=0,…,39`, dans u18. On a
`|ax|²=|bx|²=2 221 025<|ab|²=4 000 000` et
`2|ax|²>|ab|²`, donc acuité stricte et propriété de la plus longue
arête. Le centre transverse est `t x`,
`t=(r²−L²)/(2r²)>0` ; tout autre point `y` du cercle a une puissance
`(r²−L²)(1−x·y/r²)>0`, et les autres grappes sont loin. Chaque
arête fournie peut ainsi produire 108 records, sous son plafond propre.
Sur **4 400 sites/40 arêtes**, le total de 4 320 records dépasse
l'arène par défaut de 4 256 ; 39 arêtes en occupent 4 212, donc une
seule est reportée. Cette variante réduirait fortement le coût du gate
actuel de 4 097 grappes ; elle doit encore être testée contre le moteur
avant substitution.

## Deux limites de preuve encore utiles

`tests/gpu/lanes_port_gate.cpp` force un `bad_alloc` en demandant environ
**512 Gio de records par worker**. Sous overcommit, la réservation peut
réussir puis l'initialisation épuiser la mémoire sans exception
contrôlée ; je n'ai pas lancé ce sous-test. Il ne cible ni l'`assign`
après le premier bloc ni `out.records.insert` au commit. Une injection
d'allocation déterministe et bornée à ces trois points, avec plusieurs
blocs/workers, devrait vérifier jointure, réveil et erreur typée.

Le [reçu local CPU](../receipts/s4a_q3_lanes_local_20260923/README.md)
est publié par `54f6249a4` : son script garde les sources suivies,
les SHA entrée/binaires, statuts, condensés épinglés et décision/juge
q3. Il mesure seulement la trame 08/000000 sans sol entière ; les six
bras K5/K10 ont les mêmes condensés tour/catalogue et des temps sur
hôte partagé. Une contrelecture indépendante a vérifié les 13 SHA du
paquet, quatre SHA d'entrées encore présentes, les six statuts
`complete_relative`, les comptes de records/tests et le catalogue ; le
cinquième SHA d'entrée appartient au binaire auxiliaire
`rank_order_stats`, supprimé après capture. Le lecteur `run.sh` ne juge
pas sémantiquement les trois fichiers `stats_*.txt` ni tous les champs
input/backend/records/ledger, même si les valeurs publiées sont
cohérentes à cette contrelecture. À K5, huit anneaux abaissent les tests
logiques de 5 285,781 M à 145,784 M sur la même trame ; le levier CPU
augmente le temps de chaîne indicatif de 3,48 % K5 et 5,65 % K10 face
à S3. Pour la question de croissance, les
[demi-scènes, quarts physiques et densités emboîtées](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md)
doivent être rejoués sur S4a à sorties appariées. Le
[reçu CPU ciblé](s4a_ground_hot_quarter_20260923/README.md) ferme déjà
le quart chaud à K10 sur trois densités avec une tentative rejetée pour
contention ; les autres secteurs, K5 et G4 restent ouverts. Les
statistiques `--file` ne sont pas une porte d'exactitude LiDAR.
