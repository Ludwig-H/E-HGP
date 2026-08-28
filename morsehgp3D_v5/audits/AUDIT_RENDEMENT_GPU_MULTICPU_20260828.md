# Audit de résolution — rendement GPU et multi-CPU

- **Dernier pin reçu :** `ab2c2563` ; mesures CPU au pin `82f613d3` et
  instrumentation device au pin `63deda74`.
- **Worktree observé :** postérieur à `cce4b2b3`, encore non committé. Il
  ajoute la sûreté du fold, des métriques CUDA et `SCALE_THREADS` ; les
  remarques sur ce code guident Claude mais ne constituent pas une réception.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Réponse et direction

Le faible gain n'est pas mystérieux :

- le GPU ne remplace que les corps q3/q4, après construction des covers,
  ancres, seeds et formes sur CPU ;
- les mêmes sites sont rematérialisés et recopiés pour chaque ancre ;
- q4 revient trois fois sur le CPU pour filtrer et compacter ;
- jusqu'à 48 producteurs CPU créent autant d'exécuteurs/streams éphémères,
  puis leurs lots résiduels sont vidés séquentiellement ;
- côté CPU, `--threads` n'est pas un budget global et le dernier reduce/digest
  par K conserve une longue traîne séquentielle et mémoire.

La prochaine décision utile ne vient pas seulement d'une nouvelle campagne.
Trois expériences architecturales bornées répondent directement aux suspects :

1. **un petit pool GPU persistant**, indépendant des producteurs CPU ;
2. **la géométrie globale résidente**, avec seulement des indices u32 envoyés
   par cover ;
3. **la compaction q4 stable sur device**, sans les deux retours intermédiaires.

G0 et G1 ne partent pas d'un soupçon arbitraire : le reçu n° 12 a déjà désigné
la contention des exécuteurs et le wire répété des covers. Son ancien instrument
ne séparait toutefois pas proprement leurs parts causales dans le mur ; G0 et G1
sont donc les ablations qui les départagent. G1 retire le payload site répété en
le remplaçant par des indices, mais l'hôte téléverse encore un `Q3BatchSeed` de
112 octets ou un `Q4BatchSeed` de 288 octets par seed. La dominance de ce second
terme n'est pas reçue. Si elle est ensuite établie, la conception qui le retire
est la lane **par rectangles avec index résident** déjà décrite dans
[`docs/analyses/seeds_20260827/design.txt`](../docs/analyses/seeds_20260827/design.txt) :
le device reçoit handles et ancres, puis reconstruit covers et seeds. G1 est la
couture réversible qui prouve d'abord l'index et l'arithmétique de cette cible.

Le reçu n° 12 établit que le kernel q3 est petit et implique fortement le wire
des covers et la contention ; la causalité q4 et la part propre du wire par seed
restent ouvertes. Recevoir d'abord l'instrument corrigé sur une petite session
G4 permettra donc d'ordonner G0, G1 et G2 au lieu de supposer leur gain. Pour le
multi-CPU, il faut séparer la sensibilité au budget d'ouvriers d'un véritable
speedup 1→N sous cpuset.

## Base chiffrée conservée

Au reçu n° 12, les murs CPU/GPU appariés à 50 k restent mixtes :

| famille | CPU (s) | GPU (s) | CPU / GPU |
|---|---:|---:|---:|
| `uniform` | 56,290 | 58,142 | 0,968× |
| `terrain` | 16,423 | 14,605 | 1,124× |
| `eight_clusters` | 61,920 | 64,618 | 0,958× |
| `scanline_single_pass` | 12,635 | 12,445 | 1,015× |

Le reçu prouve compilation/exécution CUDA, quatre digests CPU/GPU égaux et le
mutant device tué. Un passage CPU puis GPU, non contrebalancé et non répété, ne
prouve pas un gain reproductible. Il montre que le port conserve l'objet sur
ces quatre entrées. Les variations de sous-étapes q4 sur un passage désignent
des expériences causales, pas encore un levier reçu de bout en bout.

Sur `uniform`, q3+q4 ne représente que 8,9 % du mur CPU : même leur disparition
donnerait un plafond idéal proche de 1,10×. Sur `scanline` 200 k, q4 représente
au contraire environ 80 % du mur ; c'est la cible principale du chantier.

## P0 — finir correctement l'instrument avant toute campagne

Le travail courant apporte de bons éléments : événements CUDA séparés,
compteurs d'octets, histogrammes de lots, attente/réservation hôte, compteur
de concurrence et suppression de la barrière H2D intrusive. Aucun de ces
points ne bloque un pin explicitement `cpu_reference`; les raccords suivants
bloquent seulement la réception de l'instrument et tout nouveau reçu G4.

### Le validateur SCALE accepte encore l'ancien format et ignore le pic

`run.hpp` imprime désormais :

```text
temps_fold_mur_ms=... (etages A et B, fold_inflight=N, pic_mesure_en_vol=P)
```

`validate_v5_campaign.py` reconnaît maintenant cette ligne, mais son alternative
regex accepte toujours `N ordre(s) en vol`, ne capture jamais `P`, et les tests
Python exigent même la compatibilité legacy. Ainsi `P=0`, `P>N` ou `N=1,P>1`
restent verts alors qu'ils contredisent la mesure annoncée.

Le parseur de cette campagne doit exiger les deux champs nommés, vérifier
`1 <= pic_mesure_en_vol <= fold_inflight`, imposer `P=1` pour `N=1` et comparer
le budget demandé. Ajouter les rejets legacy, `P=0` et `P>N`.

### Les portes ont maintenant leur raccord local, à recevoir sur un pin

Le worktree mouvant enregistre désormais `gpu_instrument_gate`,
`fold_inflight_safety_gate`, `cell_grid_oracle` et leurs mutants dans CMake et
`mutants.hpp`. Une construction Release puis le CTest ciblé donnent localement
14/14 réussites en 50,57 s, y compris codes 4 et refus code 2. C'est la
correction attendue, mais pas encore une réception sur pin propre ; aucun de ces
tests n'exécute nvcc ou un device CUDA.

### Le cycle de vie CUDA échappe à la décomposition

Les `Q3DeviceExecutor` et `Q4DeviceExecutor` sont `thread_local` dans des
ouvriers éphémères. Création de stream/événements, `cudaMalloc`, `cudaFree` et
destruction du stream sont incluses dans le mur de lane mais absentes de
`executor_ms_sum`. Ce dernier commence également après une partie de la
validation et de la préparation des vecteurs.

À court terme, ajouter `executors_created` et `lifecycle_ms` ou élargir
explicitement le chrono. La correction architecturale est le pool persistant
décrit ci-dessous.

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

### Le nouveau schéma n'est ni versionné ni jugé par la campagne

La ligne historique `gpu=1 kernel_ms=...` conserve son nom alors que le champ
passe d'un temps mêlant retours et boucles hôte à la somme des seuls kernels.
Les deux nouvelles lignes `gpu_q3_etapes` et `gpu_q4_etapes` ne portent aucune
version. Surtout, `validate_v5_campaign.py` ne les exige pas : une campagne peut
rester verte si elles manquent, contiennent `nan`, des octets faux ou une
partition hôte incohérente.

Correction minimale avant G4 : imprimer `gpu_instrument_schema=v2`, employer un
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

## G0 — pool GPU borné et persistant

Le contrat actuel de `scan()` est **synchrone**. La première version de G0 n'a
donc besoin ni de lots possédés asynchrones, ni de file de completion, ni de
reorder buffer :

- créer hors de `parallel_items` un pool de 1, 2, 4 ou 8 exécuteurs, chacun
  possédant stream, événements et buffers réutilisables ;
- dans le callback `scan`, prendre un lease RAII, appeler le scan synchrone,
  puis rendre l'exécuteur ; le lot reste possédé par son ouvrier et ses sorties
  restent dans `louts[t]` ;
- exécuter le flush final des `T` reliquats avec `parallel_items` au lieu de la
  boucle séquentielle actuelle ; le même pool borne la concurrence device ;
- agréger les statistiques une fois par exécuteur après drainage, sans mutex
  par lot ;
- sur exception CUDA, empoisonner l'exécuteur, annuler le pool et réveiller
  tous les leases en attente ; un pool tombé à zéro ne doit jamais bloquer ;
- la destruction draine les leases avant streams, puis la géométrie partagée.

Cette version conserve exactement l'ordre local de chaque ouvrier. Une file
asynchrone avec lots déplacés ne devient utile que si une mesure montre qu'il
faut recouvrir le scan d'un même producteur avec son lot suivant.

Le sweep `gpu_executors={1,2,4,8}` devient alors interprétable. Le nombre de
producteurs CPU peut rester 48 sans créer 48 exécuteurs/streams concurrents
sur un seul GPU.

Premier critère de réussite : mêmes digests et compteurs, moins
`executors_created`, disparition du flush séquentiel et baisse du mur de lane.
Un gain de bout en bout n'est pas exigé de ce commit isolé.

## G1 — géométrie résidente et covers par indices

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

Scinder le protocole :

### A. Scaling des ouvriers

- `fold_inflight=1` ;
- digest OFF pour isoler le travail, puis une strate digest ON ;
- processus sous cpuset exact et topologiquement déclaré de 1, 2, 4, 8, 16,
  24, 32 et 48 CPU ; `--threads=t` ne remplace pas `taskset` ;
- masque effectif gravé et validé dans chaque statut, topologie et CPU réellement
  utilisés ;
- `perf stat` après preflight pour `task-clock`, CPU utilisés, cycles,
  instructions, changements de contexte, migrations et LLC/cache misses ; une
  permission absente est gravée sans invalider la campagne principale ;
- échauffement et ordre miroir à nombre pair de répétitions, ou randomisation
  épinglée si un nombre impair est nécessaire.

### B. Chevauchement des ordres

- cpuset fixe ;
- `fold_inflight={1,2,3}` ;
- mêmes entrées, digests, comptes de travail et pic RSS ;
- ne pas appeler ce tableau « scaling CPU ».

La matrice factorielle actuelle de 192 runs dépasse très largement la session
gardée. Commencer par un pilote d'au plus 24 runs et réserver explicitement le
temps de rapatriement.

Les optimisations CPU à tester ensuite sont :

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

1. **Finir l'instrument localement** : parseur au format réel, mutants/CMake,
   lifecycle, erreur asynchrone et timelines non additives.
2. **Recevoir ce seul instrument sur une petite session G4** : baseline causale
   avant toute réécriture, puis choisir l'ordre G0/G1/G2.
3. **Faire G0 dans sa forme synchrone minimale**, puis recevoir le sweep
   d'exécuteurs ; aucune file asynchrone n'est requise.
4. **Faire de G1 une couture réversible** : géométrie + indices +
   matérialisation device bit-identique. Si les octets par seed dominent
   ensuite, passer à la porte CPU `rect_shaped`; sinon conserver G1 comme
   optimisation bornée.
5. **N'engager G2 que si l'instrument isole les retours q4** comme poste
   dominant et qu'un gain intermédiaire est utile ; comparer G3 seulement si
   le wire par seed devient le poste reçu.
6. **Lancer en parallèle le pilote CPU sous cpuset** dès que son protocole local,
   son budget et sa marge de rapatriement sont reçus ; il ne dépend pas des
   refactorings GPU.

Cette séquence évite d'engager trois réécritures sur une causalité encore
ouverte. Elle donne à Claude une baseline interprétable, puis une seule
expérience falsifiable à la fois, tandis que la piste CPU avance indépendamment.

GCP non utilisé pour cet audit.
