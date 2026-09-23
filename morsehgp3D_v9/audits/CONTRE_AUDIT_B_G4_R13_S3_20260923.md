# Contre-audit B — G4 R13, certificats S3 dans la tour

23 septembre 2026. Relecture indépendante du [reçu R13](../receipts/g4_tower_r13_20260923/README.md)
publié par `3dfedcae1`, paquet construit au commit produit
`46c50432c`. Cadre `exploration_v9_hors_registre`, u18/grille 1 mm,
trois trames **sans sol de la seule séquence 08**, s8, `not_claimed`.
Le G4 SPOT a été **utilisé puis arrêté** : le reçu hôte donne
`completed`, capture reçue et arrêt ciblé certifié ; la VM a été relue
`TERMINATED`. Aucun second G4 n'a été lancé par cet audit.

## Ce que la réception établit

Les **326** lignes de `SHA256SUMS` passent. Le manifeste, le snapshot et
les journaux de 169 sources issus du commit déclaré concordent ; les
290 membres de la capture correspondent aux fichiers extraits. Une
relecture avec `validate_snapshot` puis `validate_received` du protocole
épinglé a donné `completed`, **18/18 cas**, en Python normal puis `-O`.
Ces deux lectures LIVE n'ont pas de journal versionné supplémentaire ;
la capture publique et son lecteur restent les pièces rejouables.

Les six cas principaux GPU S2+S3 ont tous un jumeau moteur, et les six
couples de condensés FULL/catalogue sont identiques aux **épingles CPU
indépendantes de C**. Les 18 cas sont `complete_relative`, Euler « holds » ;
les 12 comparaisons du lecteur sont égales et aucun lot n'est non
apparié. Le préflight synthétique 1 500 sites juge **55 523/55 523**
arêtes décidées sur GPU par leur masque CPU et leurs comptes sommés ;
le préflight à ardoise 64 juge **52 119** arêtes et reporte **3 404**
arêtes au moteur, avec les mêmes condensés et comptes. Sur LiDAR, les
sept cas S3 (six principaux et W24) ont 1 504 warps lancés, un temps
device positif et **zéro report** ; les deux autres cas GPU (ablation
S2 seule) exécutent le filtre, pas les certificats. Le juge par arête
est **désactivé sur les trames LiDAR chronométrées**.

Cette preuve reste relative : le condensé FNV64 a une possibilité de
collision et ne compare pas littéralement tous les éléments du
catalogue ; deux chemins également incomplets peuvent conserver un
condensé identique. Les petits oracles indépendants et les épingles
réduisent le risque, mais n'établissent pas la complétude mathématique
globale. Le binaire construit sur G4 n'est pas joint à la capture ;
source, commande de build, dépendances et SHA du binaire sont consignés.
Le lot CUDA **vide avec pointeurs nuls** n'a toujours pas été exercé sur
carte. Aucun résultat de cette session ne qualifie trame brute avec
sol, float32 natif, plusieurs séquences, s10/s12, passage à l'échelle
8k/16k/32k ou dizaines de millions de points.

## Performance : gain réel, contrat encore loin

Les durées sont `chain_total` de la sonde sur les points u32 déjà
préparés ; lecture, segmentation, préparation Python et condensés de
contrôle sont hors de ce nombre. Une seule exécution par cas : aucun
p95 ni intervalle de dispersion ne sont mesurés.

| Trame sans sol | K | GPU S2+S3 | Moteur | q3/q4 GPU | Tour GPU | Chaîne hors q3/q4 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 08/000000 | 5 | 2,202 s | 3,266 s | 1,184 s | 0,745 s | 1,017 s |
| 08/000100 | 5 | **1,738 s** | 2,563 s | 0,909 s | 0,584 s | 0,829 s |
| 08/000200 | 5 | 2,347 s | 3,538 s | 1,229 s | 0,784 s | 1,119 s |
| 08/000000 | 10 | 7,825 s | 10,498 s | 3,836 s | 3,039 s | 3,990 s |
| 08/000100 | 10 | **5,914 s** | 7,710 s | 2,919 s | 2,260 s | 2,995 s |
| 08/000200 | 10 | 7,967 s | 10,751 s | 4,075 s | 2,724 s | 3,892 s |

L'ablation **dans R13**, sur 08/000000/W48, donne GPU S2 seul →
GPU S2+S3 : **2,340→2,202 s** à K5 et **8,408→7,825 s** à K10, soit
138 et 583 ms de gain net observé. Les baisses R12→R13 traversent
deux paquets et deux sessions ; elles comprennent notamment le
préchauffage du contexte/index GPU et ne doivent pas être attribuées
entières à S3. Le lot CPU sans GPU est plus lent que le moteur seul
sur ces deux K (3,841 contre 3,266 s ; 11,023 contre 10,498 s).

Après S3, les ouvriers CPU reconstruisent encore **611 000 à 750 000
covers** à K5 et **1,258 à 1,555 million** à K10, avant atlas et
émissions q3/q4. Les appels de certificats prennent 170–239 ms à K5,
390–591 ms à K10 ; leur `certificate_device_ms` de 149–557 ms est
un **intervalle d'événements incluant transferts, noyau et retour**
(`filter_runner.cu`, `elapsed(e[0],e[3])`), pas le temps isolé du
noyau. Les **1 504 warps** sont lancés selon une estimation d'occupation,
pas une occupation effective mesurée. La formule « coût du noyau S3 :
189 ms » dans le reçu doit donc être lue comme **coût de la passe
device complète** sur 08/000000/K5 ; le noyau seul n'est pas publié.

Même supprimer fictivement **tout q3/q4** en conservant le reste de
R13 laisserait 1,017 et 1,119 s sur deux trames K5 ; à K10, le résidu
est 2,995–3,990 s et la tour seule 2,260–3,039 s. C'est un diagnostic
conditionnel de l'architecture mesurée, pas une borne contre une autre
architecture. La priorité K5 doit réduire à la fois l'aval des
survivants et la tour ; K10 demande un changement substantiel des deux.
La première grille de [certificats pré-cœur à huit cellules](s2_precore_node_shadow_20260923/README.md)
ne ferme que 0,0818–0,1345 % des formes cœur du plein brut testé : ne
pas la porter telle quelle pour combler l'écart. Le streaming exact
S2/S3, nécessaire pour les dizaines de millions de sites, est un
chantier **distinct** de la réduction du travail total.

## Après R13 : portée du port « 128 registres »

Le commit produit `0b41e4c86`, **postérieur au paquet R13**,
remplace les compteurs d'une arête par des champs u32 et place les
totaux de warp en mémoire partagée. La lecture statique donne une borne
pour les champs par arête sous les gardes d'index actuelles : le DFS
monotone visite au plus `node_count<2^32` nœuds ; les sous-arbres admis
ou rejetés sont disjoints, donc comptent au plus `n<2^32` sites ; les
deux preuves par arête ont chacune au plus
`1+4+…+4^6 = 5 461` cellules. Les tests potentiellement nombreux
restent u64. La barrière avant réemploi du frontier demeure en source.
Ce n'est **pas** encore une preuve device du nouveau noyau. Le chiffre
`128 registres, sans spill, 16 warps/SM` est annoncé depuis une
compilation locale non archivée ; R13 a exécuté l'ancien binaire.

Les additions u64 des totaux par warp et par appel restent **sans
contrôle de débordement** sur tout le domaine de l'API. Une borne
conservatrice du seul terme de tests de cellules est
`2 × 5 461 × min(n, capacity) × edges` : avec le défaut
`capacity=65 536` et `edges≤2^31−1`, elle est inférieure à `2^64`,
mais pas avec une capacité utilisateur proche de `2^32`. Les six
populations R13 sont loin du seuil : cette borne donne moins de
`2,47×10^15` tests pour le pire cas. C'est un verrou de domaine
massif, pas une divergence observée. Un refus typé ou une accumulation
vérifiée est requis pour ne pas produire de compte faux ; pour **servir**
le contrat massif, privilégier des lots bornés avec accumulation hôte
u128 (ou un équivalent exact), le refus seul ne suffisant pas. Une future
réception devrait archiver `ptxas -v`, les attributs CUDA et
l'occupation calculée, puis les sous-temps et les masques/compteurs
par arête contre CPU, avec ardoise réduite, lot vide et contacts.

Précision supplémentaire au [nouvel addendum développeur](../receipts/g4_tower_r13_20260923/ADDENDUM_20260923.md) :
`cudaEventRecord(e[0])` précède **les allocations**, copies et
initialisations du lot S3 (`filter_runner.cu:616–643`). L'intervalle
`certificate_device_ms=e[0]→e[3]` les englobe aussi, ainsi que de
possibles attentes de soumission hôte. « Upload + noyau + download »
n'est donc pas un découpage exhaustif, ni un temps d'activité pure du
noyau.

## Travail physique masqué : covers S3 reconstruits par le CPU

Le flux GPU publie `mask`, `status` et des compteurs agrégés, **pas** les
plages du cover. Après S3, pour chaque arête décidée dont une voie
reste ouverte, `Engine::certified_edge` rappelle
`Q34EdgeCover::make` sur CPU. Le cover complet fermé a pourtant déjà
été construit dans le slab du warp GPU ; ce slab est écrasé à l'arête
suivante. Le nombre `rebuilt_covers` suit ces parcours CPU, mais leurs
visites de nœuds, tests et sites ne sont **pas** ajoutés au ledger
géométrique logique. Ainsi « même travail » dans R13 compare les
certificats logiques, **pas** tous les parcours physiques du chemin
mixte : ne pas utiliser seul ce ledger pour juger sa croissance.

| Trame sans sol | K | Covers GPU complets | Rebuilds CPU | Sites de tous les covers GPU | Plages ouvertes, bornes Mo¹ | `edges_ms` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 08/000000 | 5 | 900 377 | 708 686 | 251 972 209 | 5,67–250,93 | 731 |
| 08/000100 | 5 | 777 841 | 610 811 | 195 711 914 | 4,89–192,40 | 541 |
| 08/000200 | 5 | 945 062 | 749 534 | 307 871 336 | 6,00–250,98 | 749 |
| 08/000000 | 10 | 1 934 399 | 1 463 362 | 874 278 487 | 11,71–675,15 | 2 922 |
| 08/000100 | 10 | 1 656 978 | 1 258 362 | 619 045 951 | 10,07–494,77 | 2 244 |
| 08/000200 | 10 | 2 026 783 | 1 554 880 | 972 863 222 | 12,44–654,47 | 3 086 |

¹ Décimaux, `8R≤octets≤2(V−C+4R)` ; `V` est le compteur publié
`ledger.cover_node_visits`. Ces bornes ne sont pas des octets transférés.

Les rebuilds représentent **75,6–79,3 %** des covers complets
certifiés sur ces cas. `edges_ms` inclut aussi atlas, graines,
q3/q4 et callbacks : il **ne mesure pas** le rebuild isolément.
Même retirer fictivement tout `edges_ms` laisserait **1,196–1,599 s**
de chaîne K5 ; la réutilisation seule ne qualifie donc pas la seconde
sur l'architecture R13 inchangée. Les `cover_sites` incluent les
covers fermés, non transférables pour la génération, et ne donnent
aucune distribution de plages des covers ouverts. `retained_ranges`
existe dans le code, mais n'est pas sérialisé dans le reçu. Un payload
u32 de deux bornes par plage coûterait `8×Σranges_ouverts` octets,
plus offsets et statuts ; cette somme reste **inconnue**. La mémoire
CPU de `Range` est de 16 octets/plage sur LP64. Ne pas supposer que
transférer les plages est gratuit parce que leur nombre de sites est
grand ou petit.

Une borne structurelle resserre seulement l'**espace des possibles**.
Dans un parcours d'arbre binaire, `r` runs admis maximaux imposent au
moins `r−1` runs rejetés ; le sous-arbre visité est complet, donc
`V≥4r−3` visites de nœuds pour ce cover. En posant `V` les visites
de tous les covers GPU, `C` leurs constructions et `R` les rebuilds
(covers ouverts), chaque cover fermé visite au moins un nœud :
`Σr_ouverts≤(V−C+4R)/4`. Les six lignes R13 ne bornent ainsi les
`8Σr_ouverts` octets de plages u32 qu'entre **4,89–12,44 Mo de
plancher** et **0,192–0,675 Go de plafond**, selon le cas ; ces
intervalles ne sont ni une distribution ni un transfert mesuré.

Une optimisation éventuelle doit importer **uniquement** les covers
`decided && mask!=0`, complets et fermés :
`|2z−a−b|²≤4|b−a|²`, contacts compris. Ce sont des intervalles
`[first,last)` de **rangs spatiaux**, strictement ordonnés, disjoints
et maximalement joints ; `Σ longueurs=site_count≥2` et les deux
extrémités y figurent. Le cœur diamétral d'une arête fermée ne peut
pas servir à q3/q4. Les consommateurs utilisent ces plages pour
décomposer l'atlas, énumérer des témoins et recenser coquilles et
intérieurs ; égalité de masque, de cardinalité ou de condensé final
ne prouve pas l'égalité des plages. Le payload doit être lié au
**même index immuable et au même ordinal d'arête**, posséder sa mémoire
après le réemploi du slab et garder le travail logique S3 distinct
des parcours physiques. `GpuPreparation::get` ne vérifie aujourd'hui
pas l'identité de l'index demandé ; le chemin R13 n'en utilise qu'un,
mais un import/flux multi-index doit la rendre explicite.

Porte courte **avant** une nouvelle campagne G4 : sur mêmes entrées
08/000000/K5 puis K10, publier pour `decided && mask!=0`
`Σranges`, `Σsites`, p50/p95/p99/max des plages et sites, octets
exportables, ainsi que le temps et les visites du seul rebuild CPU
(W1/W48). Si ce budget est favorable, tuiler l'export avec offsets et
statut transactionnels : manque de place → repli exact au rebuild CPU,
jamais plage partielle. Une porte différentielle compare **plage par
plage** l'import et `Q34EdgeCover::make`, puis flux de supports,
coquilles, catalogue et FULL. L'ablation G4 doit mesurer compaction,
D2H, pic HBM/RSS, rebuild restant et `chain_total` ON/OFF. Un scan en
deux passes qui recalcule le cover sur GPU peut être exact, mais ce
n'est plus la réutilisation du cover déjà calculé par S3 : facturer ce
second parcours. Pour une capture en une passe, un append global borné
peut réserver des plages puis publier `{ordinal,offset,count,sites}`
seulement après succès ; l'ordre physique d'allocation peut varier,
mais la restitution par ordinal doit être stable. `Range` hôte emploie
deux `size_t` : il faut soit l'inflater depuis les deux u32, soit
refondre les consommateurs pour une vue compacte possédée ; aucun
zéro-copie implicite. Si le coût des plages domine, fusionner davantage
d'aval q3/q4 sur GPU est une piste distincte à évaluer.
