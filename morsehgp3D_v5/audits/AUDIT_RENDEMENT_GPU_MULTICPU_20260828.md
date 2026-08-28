# Audit de résolution — rendement GPU et multi-CPU

- **Pins inspectés par sujet :** `194a0bc2` pour le pool G0 et la sonde fold,
  `bc66ade7` pour le réducteur vivant ; la baseline device/SCALE versionnée
  provient de `c95cfa95`, G0 historique de `fe54ccca` et G1 q3/q4 des trois
  commits `dd928111`–`556c421e`. Le nouveau reçu device exécute G0 et G1 q3 au
  pin `839cf1ec` ; il est versionné avec cet audit à `0656bf4c` et ne reçoit
  pas G1 q4, postérieur.
- **État courant et priorités :** voir
  [`ETAT_COURANT.md`](ETAT_COURANT.md). Les sections G1–G3 ci-dessous restent
  des guides de conception ; les anciennes conditions de campagne sont
  requalifiées par le reçu de session 13 et le contre-audit G0.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

Le reçu de la nouvelle session au pin `839cf1ec` confirme nominalement huit
exécuteurs G0 et le wire index q3 avec digests égaux et H2D réduit. Elle ne
couvre pas G1 q4 et ne ferme pas les défauts de sûreté du pool ; son dossier
versionné contient un manifeste d'artefacts et un reçu de sûreté expurgé. La
réception exacte et le plus petit ordre de correction sont centralisés dans
`ETAT_COURANT.md`.

## Réponse et direction

Le faible gain n'est pas mystérieux :

- le GPU ne remplace que les corps q3/q4, après construction des covers,
  ancres, seeds et formes sur CPU ;
- les mêmes sites sont rematérialisés et recopiés pour chaque ancre ;
- q4 revient trois fois sur le CPU pour filtrer et compacter ;
- dans la baseline de session 13, jusqu'à 48 producteurs CPU créaient autant
  d'exécuteurs/streams éphémères ; G0 les borne désormais, mais les lots
  résiduels restent vidés séquentiellement ;
- côté CPU, `--threads` n'est pas un budget global et le dernier reduce/digest
  par K conserve une longue traîne séquentielle et mémoire.

La prochaine décision utile ne vient pas seulement d'une nouvelle campagne.
Trois expériences architecturales bornées répondent directement aux suspects :

1. **un petit pool GPU persistant**, indépendant des producteurs CPU ;
2. **la géométrie globale résidente**, avec seulement des indices u32 envoyés
   par cover ;
3. **la compaction q4 stable sur device**, sans les deux retours intermédiaires.

G0 et G1 ne partent pas d'un soupçon arbitraire : les reçus n° 12 et 13
désignent la multiplication des exécuteurs et le wire répété des covers. Les
sommes instrumentées se recouvrent et ne séparent pas causalement leurs parts
dans le mur ; G0 et G1 restent donc les ablations qui les départagent. G1 retire
le payload site répété en
le remplaçant par des indices, mais l'hôte téléverse encore un `Q3BatchSeed` de
112 octets ou un `Q4BatchSeed` de 288 octets par seed. La dominance de ce second
terme n'est pas reçue. Si elle est ensuite établie, la conception qui le retire
est la lane **par rectangles avec index résident** déjà décrite dans
[`docs/analyses/seeds_20260827/design.txt`](../docs/analyses/seeds_20260827/design.txt) :
le device reçoit handles et ancres, puis reconstruit covers et seeds. G1 est la
couture réversible qui prouve d'abord l'index et l'arithmétique de cette cible.

La session 13 reçoit la baseline comme preuve fonctionnelle bornée : digests
appariés, métriques présentes et mutant device tué. Le reçu suivant exerce
réellement le pool G0 et le wire q3 index, avec digests égaux. Il ne ferme ni la
sûreté hôte G0, ni les lots mono-wire, ni la preuve de branche du protocole, ni
q4 index. L'ordre utile devient donc G0 sûr, fermeture hôte G1, une petite
réception CUDA commune au HEAD, puis G2 ou G3 selon l'ablation. Pour le
multi-CPU, il faut séparer la sensibilité au budget d'ouvriers d'un véritable
speedup 1→N sous cpuset.

## Base chiffrée conservée

Au reçu n° 13, les murs CPU/GPU appariés à 50 k restent mixtes :

| famille | CPU (s) | GPU (s) | CPU / GPU |
|---|---:|---:|---:|
| `uniform` | 56,291 | 57,316 | 0,982× |
| `terrain` | 16,910 | 14,200 | 1,191× |
| `eight_clusters` | 61,812 | 62,709 | 0,986× |
| `scanline_single_pass` | 12,740 | 11,652 | 1,093× |

Le reçu prouve compilation/exécution CUDA, quatre digests CPU/GPU égaux et le
mutant device tué. Un passage CPU puis GPU, non contrebalancé et non répété, ne
prouve pas un gain reproductible. Il montre que le port conserve l'objet sur
ces quatre entrées. Le signal est toutefois actionnable : q3 régresse dans les
quatre cas, tandis que la lane q4 gagne 2,18× sur `terrain` et 1,55× sur
`scanline`, reste presque neutre sur `eight_clusters` et régresse sur `uniform`.
Après G1, le routage doit donc être falsifié par une intensité
`travail / octets`, pas par le seul seuil de sites ; q3 reste sur CPU tant que
son ratio n'est pas inversé.

Sur `uniform`, q3+q4 ne représente que 8,9 % du mur CPU : même leur disparition
donnerait un plafond idéal proche de 1,10×. Sur `scanline` 200 k, q4 représente
au contraire environ 80 % du mur ; c'est la cible principale du chantier.

### Ablation G0/G1 q3 au pin `839cf1ec`

Le reçu suivant borne `--gpu-executors=4` : huit exécuteurs au total pour q3 et
q4, `flux_pic=4`, contre 98 exécuteurs éphémères dans la baseline. Les temps de
réservation cumulés s'effondrent, mais le mur reste du même ordre. C'est un bon
résultat d'architecture : G0 retire une source de variance et de ressources
sans inventer un gain. Il n'exerce toutefois aucun des scénarios d'échec hôte
qui bloquent encore sa sûreté.

L'ablation q3 SoA→index réduit les octets H2D d'un facteur 1,76 à 2,88 et le mur
de lane de 1,5 à 21,3 % selon la famille. Le mur complet varie de +0,8 % à
-2,5 %, sur un seul passage toujours ordonné SoA puis index. Le payload index a
donc un effet réel dans q3, mais Amdahl et les seeds/fold absorbent ce gain. La
note du reçu surestime deux généralités : le temps H2D baisse de 16 à 73 %, pas
de 21 à 73 %, et q3+q4 représente environ 9 % du mur CPU sur `uniform`, mais
24 % sur `eight_clusters`, 49 % sur `scanline` et 53 % sur `terrain`.

## Instrument reçu, interprétation encore bornée

`700a38c7` ferme les défauts connus du validateur SCALE et `c95cfa95` raccorde
les événements CUDA, octets, histogrammes, attente/réservation, concurrence et
vie des exécuteurs. La session 13 exerce réellement nvcc et le device. Ces
points sont reçus comme **instrument exploratoire fonctionnel**, pas comme une
décomposition causale du mur.

Le champ `cycle_de_vie_ms_sum` doit être lu littéralement comme une somme des
durées construction→destruction, incluant activité et attente. Ce n'est pas un
coût de création/destruction. Au pin de la campagne, deux exécuteurs
`thread_local` du fil appelant sont en outre détruits après l'impression ; la
somme publiée ne couvre donc pas les 98 créations annoncées. G0 corrige
l'architecture, mais la campagne `c95cfa95` ne peut pas le recevoir.

### Les timelines ne sont pas additives

Les événements `device_sum=(h2d,kernels,d2h)` décrivent la timeline CUDA par
exécuteur. `host_sum=(issue,wait,prepare,...)` décrit la timeline hôte ;
`wait` recouvre justement une partie du device. Les imprimer séparément et ne
jamais soustraire ou additionner ces deux groupes comme un mur.

Les buffers hôte sont des `std::vector` pageables : `issue_ms` mesure le temps
passé dans les appels CUDA et peut inclure staging ou blocage, pas seulement un
enfilement ([comportement de synchronisation de l'API CUDA](https://docs.nvidia.com/cuda/cuda-driver-api/api-sync-behavior.html)).
En q4, `stage_.resize()` et `deep_.resize()` sont aussi dans ce champ. Les
sortir ou nommer ce périmètre exactement. Le gauge de concurrence
doit appartenir à une invocation, pas être un singleton statique remis à zéro
par deux pipelines potentiellement concurrents. Enfin, toute erreur après une
copie asynchrone doit drainer au mieux, empoisonner l'exécuteur et interdire sa
réutilisation.

### Durcissements requis pour attribuer G1/G2

La ligne historique `gpu=1 kernel_ms=...` conserve son nom alors que le champ
passe d'un temps mêlant retours et boucles hôte à la somme des seuls kernels.
Les deux nouvelles lignes `gpu_q3_etapes` et `gpu_q4_etapes` ne portent aucune
version. Surtout, `validate_v5_campaign.py` ne les exige pas : une campagne peut
rester verte si elles manquent, contiennent `nan`, des octets faux ou une
partition hôte incohérente.

Ce travail ne bloque pas le pin hôte G0. Avant une campagne qui prétend
attribuer un gain à G1 ou G2, imprimer `gpu_instrument_schema=v2`, employer un
nouveau nom tel que `kernels_device_ms_sum` au lieu de recycler `kernel_ms`, et
faire exiger par le validateur exactement une ligne par lane. Tous les réels
doivent être finis et non négatifs; `lots`, `launches`, octets et pic doivent
être non vacants sur la fixture prévue; la fermeture de la décomposition hôte
doit être contrôlée avec une tolérance annoncée. Ajouter quatre faux reçus :
ligne absente, `nan`, octets altérés et somme hôte supérieure au corps du scan.
Le rabat actuel de `rest_ms()` à zéro ne doit pas transformer ce dernier cas en
preuve verte.

Le seul compteur `h2d_bytes` agrège actuellement sites, seeds, ancres, lentilles
et tableaux intermédiaires. Il permet de mesurer le trafic total, mais pas
d'affirmer que les 112/288 octets par seed dominent. Ajouter au minimum les
classes `sites`, `seeds`, `anchors_lens` et `intermediate`, en octets. Leur part
quantifie le payload ; elle n'attribue pas encore un temps, puisque les copies
sont enfilées ensemble. La causalité temporelle vient d'une ablation appariée
G1 puis `rect_shaped`, avec mêmes lots, compteurs et digests, pas de la seule
décomposition d'octets.

La porte CUDA doit exercer les vraies copies/kernels, pas seulement
l'arithmétique du struct :

- `launches == compteur attendu` et `lots == flushes` ;
- q3 : un lancement par lot ;
- q4 : entre un et trois lancements par lot selon les survivants ;
- octets H2D/D2H strictement positifs sur les fixtures non vides ;
- pic de concurrence dans le domaine du nombre d'exécuteurs.

Pour G1, cela exige en plus `index_lots == lots > 0`, `soa_lots == 0` et
`site_soa_bytes == 0`. Le selftest courant accepte pourtant un faux pilote qui
ignore `--gpu-wire=index` et n'imprime ni wire, ni étapes, ni octets. Ajouter un
mutant `wire-index-force-soa` et son faux reçu refusé. La porte q3 doit aussi
parser `--inject` : son CTest de routage négatif attend actuellement 4 mais
l'exécutable rendrait 2 sur l'argument inconnu.

## G0 — pool GPU borné et persistant

`fe54ccca` réalise la forme synchrone minimale : pool construit hors des
producteurs, file bornée, un exécuteur persistant par worker, attente du
producteur et sorties dans son shard d'origine. Cette architecture est reçue
comme direction ; le reçu `839cf1ec` confirme sur device la borne de quatre
exécuteurs par lane. Elle n'a besoin ni de lease RAII imposé, ni de completion
queue, ni de reorder buffer.

Au pin `fe54ccca`, quatre défauts hôte certains empêchaient encore de recevoir
l'implémentation :

- notification d'un `Ticket` de pile après libération de son mutex, avec UB
  possible après réveil spurieux ;
- constructeur d'`Executor` hors capture dans le thread et déroulement non sûr
  après création partielle des threads, deux chemins vers `std::terminate` ;
- soumission récursive au même pool bloquante ;
- porte de pic dépendante du scheduler, reproduite 78 fois en échec sur 100
  sous `taskset -c 0`.

Le pin `194a0bc2` ferme ces quatre points dans la bonne architecture et ne doit
pas être réécrit. Avant sa réception complète, il reste un P0 local :
le vecteur TLS de réentrance peut allouer hors capture et autorise un cycle
`A -> B -> A`. Un marqueur TLS non allouant qui refuse toute soumission depuis
un worker suffit aux lanes actuelles. Les compléments de porte — constructeur
fautif gravé, signatures mutantes causales et contre-pression effectivement
bloquante — sont détaillés dans `ETAT_COURANT.md`.

Garder le domaine CLI continu `1..8`, rejeter 0/9 au lieu de clamper et mesurer
seulement `{1,2,4,8}`. Une erreur hôte explicitement récupérable peut rester
locale au job : il n'est donc pas utile de bloquer ce pin **hôte** sur une
machine à poison générale. Avant réception CUDA, une erreur device fatale doit
en revanche fermer admission et file, réveiller les producteurs puis drainer
sans réutilisation. Le contrat des lanes peut interdire la destruction
concurrente puisque leur propriétaire rejoint déjà tous les producteurs avant
de détruire le pool.

Premier critère de réussite : mêmes digests, vecteurs et compteurs, nombre
d'exécuteurs effectivement construit borné et aucun blocage/terminate injecté.
Le flush final séquentiel doit être parallélisé ou mesuré avant le claim de
rendement G0, mais n'oblige pas à redessiner la sûreté du pool. Un gain de bout
en bout n'est pas exigé du pin hôte.

## G1 — géométrie résidente et covers par indices

`dd928111` à `556c421e` implémentent désormais cette couture sur q3 puis q4,
avec chemins SoA et index parallèles. Les arithmétiques q3 et q4 sont cohérentes
sous le profil u16 ; les conditions de réception non vacante et les bornes
d'indices sont centralisées dans `ETAT_COURANT.md`. La présente section conserve
le contrat de coût visé. Le pin `839cf1ec` reçoit l'exécution nvcc/device q3 sur
des fixtures bornées ; q4 index à `556c421e` reste sans réception device.

Aujourd'hui, chaque ancre recopie environ 32 octets/site en q3 et 60
octets/site en q4 pour des valeurs dérivées de la même géométrie. Or
`AnchorScratch::fill_affine_sites` calcule essentiellement, pour un site `z` :

```text
u = 2*z - a - b
q = dot(u, u) - D2
```

Téléverser une fois `ix.upos[u]` et l'identifiant représentatif
`ix.point_id(u)`. Le premier stockage simple est un SoA i32 + `PointId` u32,
soit 16 octets par position unique : environ 153 Mio à 10 M et 458 Mio à
30 M. Le compactage séparé en u16 abaisse ensuite ce poste à 10 octets/site,
mais n'est pas nécessaire pour tenir en VRAM. Le cardinal géométrique reste
borné par `INT32_MAX`, car `CloudIndex`, `CoverPoint::u` et `NodeRef` sont i32.

Le wire transporte un **GeometryIndex**, jamais un `PointId` ni un rang Morton :
les identifiants externes sont arbitraires. Pour chaque cover, envoyer les
indices u32 dans leur ordre actuel, les paramètres d'ancre et les petits
offsets nécessaires. Un petit kernel matérialise alors **une fois par
`(ancre, site)`** les SoA device existants, après élargissement en i64 avant les
carrés. Recalculer `u/q` dans chaque warp répéterait cette arithmétique pour
chaque `(seed, site)` et n'est pas le premier patch sûr.

Le terme linéaire par site passe ainsi :

- q3 : environ 32 → 4 octets/site ;
- q4 : le SoA principal 60 → 4 octets/site du cover, auxquels restent au moins
  les indices locaux de `lens_sites`, seeds/ancres et l'upload global amorti.

Les tailles réelles empêchent de confondre ce gain local avec le wire total :

```text
q3 actuel : 32*S + 112*J + 8*A
q3 indexé :  4*S + 112*J + environ 24*A

q4 actuel : 60*S + 4*L + 288*J + 88*A
q4 indexé :  4*S + 4*L + 288*J + 88*A
```

Ici `S`, `L`, `J` et `A` comptent sites de cover, entrées de lentille, seeds et
ancres. En q4, le terme linéaire de site vaut donc 4 à 8 octets selon la taille
de la lentille, et les 288 octets par seed peuvent rester dominants.

Le code courant ne réalise pas encore ce coût en mémoire. `Q3Batch` matérialise
simultanément 32+4 octets/site, `Q4Batch` 60+4, et les exécuteurs réservent les
buffers SoA et index dans les deux modes. Le chiffre 4 octets décrit seulement
la copie PCIe du site indexé. Passer à un batch étiqueté exclusif et réserver
uniquement les buffers du wire actif ; sinon même la baseline SoA paie G1.

La géométrie n'est pas encore un contexte de backend : q3 téléverse
`xyz + PointId`, puis q4 les retéléverse, soit 320 Mo cumulés à 10 M de points.
Partager une `GpuBackendContext`, déclarée avant ses pools, donne un seul upload
par run. Graver son temps séparément : ses octets sont actuellement inclus dans
`h2d_octets`, mais son temps est hors `h2d_ms` et hors `lane_wall_ms`.

La q4 récupère coordonnées et identifiants depuis la géométrie résidente.
`x_site`, `lens_sites` et `skip_a/skip_b` restent des offsets **locaux** du
cover ; un tableau séparé relie `an.begin+i` au GeometryIndex global. L'ordre
du cover reste inchangé, donc l'ordre des seeds et émissions reste falsifiable
contre le chemin actuel.

Implémentation sûre :

1. introduire un `GpuBackendContext` commun aux callbacks q3 et q4, contenant
   une `GpuGeometry` en lecture seule puis le pool d'exécuteurs ; déclarer la
   géométrie avant le pool pour que le pool soit drainé et détruit avant elle ;
2. conserver temporairement les deux wires, SoA actuel et `site_index_u32` ;
3. ajouter à l'ancre les paramètres suffisants (`ua/ub` ou `a+b`, `D2`) et
   matérialiser sur device les quatre SoA q3 et les huit champs q4, puis garder
   d'abord `k_scan`, K1, K2 et K3 inchangés ;
4. comparer d'abord les SoA reconstruits bit à bit, puis verdicts, profondeurs,
   émissions, compteurs et digests ;
5. remplacer `fill_affine_sites` côté hôte par une passe sans stockage qui
   calcule exactement `qmax_d` et `umax_d`, encore requis par
   `AffineSeed::bound` ; déplacer cette réduction sur device est une étape
   ultérieure ;
6. graver séparément les octets `geometry_once`, `site_index`, `lens`,
   `anchors`, `seeds` et `intermediate`, ainsi que `materialize_ms` ;
7. retirer le wire SoA seulement après réception CUDA.

Fixtures prioritaires : coordonnées aux bornes u16, `D2` maximal, identifiants
externes non monotones, bit 31 et `UINT32_MAX`, index hors géométrie rejeté,
cas cocirculaire, permutation conservant l'ordre du cover, cover vide/minimal
et lot surdimensionné. Les positions dupliquées ne sont pas une fixture GPU
acceptée : `run_pipeline` doit les refuser avant toute exécution device.

## G2 — compaction q4 stable sur device

Le chemin q4 courant fait :

1. K1, retour complet des verdicts ;
2. sélection CPU des seeds ;
3. K2, retour d'un octet par paire ;
4. compaction CPU ;
5. K3, retour des profondeurs.

Remplacer les deux retours intermédiaires par des scans/sélections stables sur
device :

- compacter les seeds K1 dans l'ordre initial ;
- construire et compacter les paires K2 dans l'ordre `(seed,lens)` ;
- lancer K3 sur le résultat compact ;
- produire un seul **résultat logique final** : selon sa taille, le protocole
  peut employer une capacité préflightée, un compteur scalaire puis le payload,
  ou une copie maximale bornée ; il ne promet pas encore un unique D2H.

La stabilité est contractuelle : le résultat doit rester dans le même ordre
que le filtrage CPU, indépendamment du découpage en blocs. Une porte compare
chaque tableau intermédiaire sur de petits lots avant de se contenter du
digest final.

La porte debug compare K1 verdicts et survivants, K2 états et candidats, K3
profondeurs, émissions et tous les compteurs `Q4CoreCounters`/dead/chord et
`Q4StageCounts`, avec sommes u64 gardées. Le préflight VRAM couvre le nombre
d'exécuteurs multiplié par les capacités de paires/stages. L'ordre G0/G1/G2 se
décide après le reçu de l'instrument, avec un toggle d'ablation par changement.

## G3 — candidat pour retirer le wire par seed

Cette conception est déjà esquissée dans
[`design.txt`](../docs/analyses/seeds_20260827/design.txt). Après réception de
`GpuGeometry`, envoyer par fenêtre seulement :

- les `NodeRef` de `rect_cover_handles` et leurs plages résidentes ;
- les couples `(ua, ub)` qui survivent à l'histogramme ;
- les descripteurs de rectangles et les capacités de sortie.

Le device reconstruit le cover exact dans son ordre stable de 32 classes,
forme les seeds dans les registres et ne retourne que les survivants nécessaires
à la clé/niveau. Cette étape élimine les 112/288 octets par seed, contrairement
à G1. Elle ne doit toutefois pas commencer par un grand kernel CUDA : extraire
d'abord une forme CPU `rect_shaped` qui prouve, ancre par ancre, cover comme
**séquence**, extrema, seeds, verdicts, émissions et compteurs contre la
production. Transcrire ensuite cette forme sur device.

Deux risques restent des mesures, pas des objections générales : le débit des
complétions q4 et la distribution des covers surdimensionnés. Si l'instrument
montre que les retours K1/K2 dominent à court terme, G2 peut fournir un gain
intermédiaire ; si le wire par seed domine, comparer G3 à G2 et lui donner la
priorité seulement sur cette mesure. Le routage
hôte des covers hors capacité et le rejeu d'une fenêtre de sortie pleine sont
fail-closed : jamais de cover tronqué ni de préfixe publié.

## Multi-CPU — mesurer puis corriger le vrai goulot

`--threads=1` n'est pas un run à un CPU. Le fil principal peut préparer K+1
pendant qu'un fil B réduit K, et plusieurs B peuvent être actifs avec
`fold_inflight > 1`. La phase `SCALE_THREADS` mesure donc une réponse au budget
d'ouvriers, pas encore un speedup 1→N.

Elle apporte néanmoins un signal substantiel. Sur `eight_clusters` 16 k, le
mur sans digest passe de 215,041 à 15,284 s entre 1 et 48 fils, soit 14,07× ;
avec digest, de 217,361 à 19,409 s, soit 11,20×. La génération gagne environ
26,1×, mais le mur du fold seulement 5,09×. Son `reduce` ne gagne rien
(5,430 contre 5,995 s) et occupe environ 71 % du cumul de fold à 48 fils ; le
digest reste proche de 4,30 s. Le gain 32→48 tombe à 1,10×/1,08× et le RSS monte
d'environ 3,30 à 4,89/4,96 Gio. Le verrou n'est donc plus le nombre de
producteurs : c'est la phase de reduce séquentielle et le digest fixe. L'état
vivant et le trafic mémoire sont une hypothèse explicative à profiler, pas
encore une cause reçue ; une table de hash et des listes intrusives peuvent
aussi ralentir le chemin court. Le fold vivant durci de `bc66ade7` reste hors
du chemin produit et n'a pas encore été comparé au résident sur un périmètre
attribuable ; la sonde ajoutée par `194a0bc2` relance un second fold depuis le
callback du pipeline et ne fournit donc pas ce miroir CPU/RSS.

La machine expose 48 CPU logiques mais seulement 24 cœurs physiques SMT2, tous
les bras gardent l'affinité `0-47`, et aucun point 24 ni cpuset par bras n'a été
gravé. Les points au-delà de 24 incluent donc le régime SMT, mais le gain 32→48
ne peut pas lui être attribué causalement avec ce protocole. Au point 48 hors
digest, `reduce` vaut 5,995 s sur 15,284 s de mur, soit 39,2 %, contre 5,430 s à
un fil : cette phase ne scale pas et ralentit même de 10,4 %. K8–K10 portent
25 873 054 / 35 767 602 = 72,34 % des facettes et 57,65 % des événements. Cela
localise une forte concentration du volume, pas encore celle du temps sans
chronométrage par ordre.

La lecture du runtime donne trois coûts plus directement réparables qu'un
nouveau grand pipeline A. Sur le run et le pin `c95cfa95`, le décompte statique du source
donne au moins 4 138 créations de threads hors vagues WSPD : 10 fils B, 144 de
lanes, 48 de RLE, 624 de prépasses factices, 624 de vraies équipes chunkées,
2 400 de préparation du fold et 288 de tris. Les 624 prépasses sont treize
lancements de 48 fils exécutant un callback *no-op* sur le découpage uniquement
pour obtenir `nchunks`, pas des équipes sans item. Les trois `run_lane`
distribuent 3 074 842 rectangles ; avec le
ticket terminal de chaque ouvrier, ce chemin exécute 3 074 986 `fetch_add`.
Enfin, 5 918 250 préfiltrages et 5 703 016 census, soit 11 621 266 traversées,
construisent chacun une pile locale `std::vector<NodeRef>`. Le nombre
d'allocations heap — possiblement supérieur avec les croissances — n'est pas
instrumenté et ne doit pas être déduit de ce compte. Ces nombres sont des
diagnostics du pin, pas des lois asymptotiques, mais ils désignent des coutures
concrètes.

Scinder le protocole :

### A. Scaling des ouvriers

- `fold_inflight=1` ;
- digest OFF pour isoler le travail, puis une strate digest ON ;
- processus sous cpuset exact et topologiquement déclaré de 1, 8, 16, 24, 32
  et 48 CPU, en remplissant d'abord les 24 cœurs physiques puis SMT ;
  `--threads=t` ne remplace pas `taskset` ;
- masque effectif gravé et validé dans chaque statut, topologie et CPU réellement
  utilisés ;
- `perf stat` après preflight pour `task-clock`, CPU utilisés, cycles,
  instructions, changements de contexte, migrations et LLC/cache misses ; une
  permission absente est gravée sans invalider la campagne principale ;
- échauffement, ordre miroir et au moins trois répétitions conservées par bras.

### B. Chevauchement des ordres

- cpuset fixe ;
- `fold_inflight={1,2,3}` ;
- mêmes entrées, digests, comptes de travail et pic RSS ;
- ne pas appeler ce tableau « scaling CPU ».

La matrice factorielle actuelle de 192 runs dépasse très largement la session
gardée. Commencer par un pilote d'au plus 24 runs et réserver explicitement le
temps de rapatriement.

Les optimisations CPU à tester ensuite sont :

- séparer un `make_chunk_plan(n, threads)` pur de son exécution. Les treize
  prépasses factices *no-op* se retrouvent exactement dans `expand.hpp` : préfiltre,
  census, comptage, puis une fois pour chacun des dix ordres d'expansion. Le
  plan conserve `workers`, `chunk` et `nchunks` ; l'exécuteur reçoit
  `(chunk_id, begin, end, worker_id)`. Les sorties restent indexées par
  `chunk_id` et fusionnées dans l'ordre courant, donc frontières et digests ne
  changent pas. Une porte indépendante couvre exactement `[0,n)` sans trou ni
  doublon pour `n=0..4096` et `threads={1,2,3,8,48}`, puis l'instrument exige
  treize lancements réels et jamais vingt-six ;
- réutiliser un scratch `NodeRef` par worker dans le préfiltre et le census, au
  lieu d'un vecteur par boule. Le callback doit recevoir `worker_id` ; un même
  worker traite ses chunks séquentiellement et les sorties restent par chunk.
  Chaque primitive doit faire `clear()` avant **tout** parcours, car la sortie
  précoce de profondeur et l'overflow du census laissent volontairement des
  nœuds sur la pile. La fixture enchaîne ces deux sorties puis une requête
  complète et compare à trois piles fraîches. Sous le profil Morton 48 bits,
  `reserve(64)` suffit à la profondeur maximale tout en gardant le vecteur
  défensif. À 48 workers, ces deux phases passent ainsi d'environ 11,62
  millions de constructions de piles, avec un nombre de croissances heap
  encore non instrumenté, à au plus 96 piles réservées une fois ;
- instrumenter `team_launches`, `threads_created`, `atomic_tickets` et
  `worker_busy_ns`, puis distribuer les rectangles par paquets bornés avant de
  toucher au découpage algorithmique ;
- un pool global persistant dont A et B consomment le même budget ;
- subdivision des rectangles lourds par plages d'ancres, avec offsets de
  sortie déterministes ;
- fusion par offsets précomptés au lieu de concaténations répétées ;
- le fold vivant small-to-large décrit dans
[AUDIT_PASSAGE_ECHELLE_20260828.md](AUDIT_PASSAGE_ECHELLE_20260828.md),
  qui remplace les accès UF historiques par un état réellement vivant.

Le reduce ne doit pas être parallélisé par lots avant une preuve de conservation
de l'ordre des racines et deltas. En revanche, réduire sa taille d'état et ses
défauts de cache est compatible avec l'objet actuel.

Deux petits changements attaquent directement les coûts visibles sans changer
l'algorithme : remplacer le ticket atomique **par rectangle** de
`parallel_items` par des paquets dynamiques bornés, puis fusionner `louts` et
les sorties de census par préfixes d'offsets et copies parallèles disjointes.
Aujourd'hui chaque primitive recrée aussi ses fils ; le pool global proposé
doit réutiliser une équipe entre WSPD, lanes et préfiltre. Ces travaux ne feront
pas scaler un reduce aléatoire déjà limité par la bande passante : le fold
vivant est précisément ce qui réduit d'abord son working set.

## Ordre de commits proposé à Claude

1. **Durcir le pin G0 `194a0bc2`**, sans refonte : TLS non allouant avec refus
   de tout nesting, fermeture fatale sans allocation, transitions de compteurs
   atomiques sous mutex, porte causale et poison CUDA typé déclenché avant que
   le worker reprenne un lot.
2. **Fermer le contrat fonctionnel G1 sur hôte** : indices bornés, branche
   prouvée/mutée, `PointId` adverse et CTest `--inject`. Les lots mono-wire, le
   contexte partagé et les métriques de setup sont des optimisations mesurées,
   pas des prérequis sémantiques.
3. **Conserver la nouvelle session `839cf1ec`** comme reçu nominal G0 + G1 q3,
   après correction de sa portée et ajout d'un reçu de sûreté expurgé lié au
   hash du journal ; ne jamais versionner le journal brut ni une clé. Une petite
   réception CUDA de q4 index suffira après les correctifs ; ne pas rejouer la
   matrice 50 k ni imposer un sweep device sans question nouvelle.
4. **N'engager G2 que si l'instrument isole les retours q4** comme poste
   dominant et qu'un gain intermédiaire est utile ; comparer G3 seulement si
   le wire par seed devient le poste reçu.
5. **Réduire en parallèle l'état du fold**, puis lancer le pilote CPU sous
   cpuset physique/SMT avec trois répétitions ; il ne dépend pas des
   refactorings GPU.

La baseline fonctionnelle existe désormais. Cette séquence donne à Claude une
seule couture falsifiable à la fois et évite qu'un défaut de sûreté G0 rende les
mesures G1 ambiguës, tandis que la piste CPU avance indépendamment.

GCP non utilisé pour cet audit.
