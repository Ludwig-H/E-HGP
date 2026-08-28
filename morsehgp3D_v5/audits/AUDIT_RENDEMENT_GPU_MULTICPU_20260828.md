# Audit de résolution — rendement GPU et multi-CPU

- **Dernier pin reçu :** `ab2c2563` ; mesures CPU au pin `82f613d3` et
  instrumentation device au pin `63deda74`.
- **Worktree observé :** postérieur à `46f9f8c7`, encore non committé. Il
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

Le reçu n° 12 établit seulement que le kernel q3 est petit ; la causalité q4
reste ouverte. Recevoir d'abord l'instrument sur une petite session G4 permettra
donc de choisir et d'ordonner G0, G1 et G2 au lieu de supposer leur gain. Pour le
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
de concurrence et suppression de la barrière H2D intrusive. Quatre raccords
restent à fermer.

### Le validateur SCALE lit encore l'ancien format

`run.hpp` imprime désormais :

```text
temps_fold_mur_ms=... (etages A et B, fold_inflight=N, pic_mesure_en_vol=P)
```

`validate_v5_campaign.py`, son selftest et les tests Python fabriquent encore
`N ordre(s) en vol`. Les tests passent donc sur un faux producteur, tandis que
toute vraie sortie serait rejetée.

Le parseur doit exiger les deux champs nommés, vérifier
`1 <= pic_mesure_en_vol <= fold_inflight` et comparer le budget demandé.
Ajouter une fixture au format produit et un mutant de pic invalide.

### Les nouveaux mutants ne sont pas encore enregistrés

`log2hist-class-shift` et `gauge-no-peak` sont annoncés par
`gpu_instrument_gate.cpp`, mais absents du registre de mutants et de CMake.
La porte nominale compile et passe manuellement ; les deux injections rendent
actuellement le code 2, pas le code 4 contractuel.

Enregistrer les noms, les cibles nominale/mutantes et vérifier que le registre
les reconnaît avant de documenter qu'ils sont tués.

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

La porte CUDA doit exercer les vraies copies/kernels, pas seulement
l'arithmétique du struct :

- `launches == compteur attendu` et `lots == flushes` ;
- q3 : un lancement par lot ;
- q4 : entre un et trois lancements par lot selon les survivants ;
- octets H2D/D2H strictement positifs sur les fixtures non vides ;
- pic de concurrence dans le domaine du nombre d'exécuteurs.

## G0 — pool GPU borné et persistant

Découpler `threads_cpu` de `gpu_executors` :

- les producteurs CPU construisent des descripteurs de lots et les poussent
  dans une file bornée avec backpressure ;
- un pool de 1, 2, 4 ou 8 exécuteurs, créé une fois pour la lane, possède les
  streams, événements et buffers réutilisables ;
- les reliquats sont soumis à la même file, jamais vidés séquentiellement après
  la jointure ;
- chaque lot possède par déplacement tous ses buffers : aucun pointeur ne vise
  une structure de worker réutilisable ;
- la sortie porte `(worker, local_batch_seq)` et passe par un completion/reorder
  borné ; l'ordre brut global n'est exigé qu'à un fil, l'égalité post-RLE étant
  l'autorité multi-fil actuelle ;
- annulation, drain et première exception ne doivent pas se bloquer contre la
  backpressure ; le high-water de file et sa RAM sont mesurés ;
- la géométrie device est partagée en lecture seule entre les exécuteurs.

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
`ix.point_id(u)`. Le wire transporte un **GeometryIndex**, jamais un `PointId`
ni un rang Morton : les identifiants externes sont arbitraires. Pour chaque
cover, envoyer l'ordre de ces indices u32, les paramètres d'ancre et les petits
offsets nécessaires. Le kernel reconstruit `u` et `q` en arithmétique entière
exacte, après élargissement en i64 avant les carrés.

Le terme linéaire par site passe ainsi :

- q3 : environ 32 → 4 octets/site ;
- q4 : le SoA principal 60 → 4 octets/site du cover, auxquels restent au moins
  les indices locaux de `lens_sites`, seeds/ancres et l'upload global amorti.

La q4 récupère coordonnées et identifiants depuis la géométrie résidente.
`x_site`, `lens_sites` et `skip_a/skip_b` restent des offsets **locaux** du
cover ; un tableau séparé relie `an.begin+i` au GeometryIndex global. L'ordre
du cover reste inchangé, donc l'ordre des seeds et émissions reste falsifiable
contre le chemin actuel.

Implémentation sûre :

1. introduire un objet RAII `GpuGeometry` partagé par les exécuteurs ;
2. conserver temporairement les deux wires, SoA actuel et `site_index_u32` ;
3. ajouter à l'ancre les paramètres suffisants (`ua/ub` ou `a+b`, `D2`) et
   comparer deux variantes : préparation affine une fois par `(ancre, site)`
   sur device, puis scans existants, ou recalcul dans le kernel ;
4. comparer d'abord les SoA reconstruits bit à bit, puis verdicts, profondeurs,
   émissions, compteurs et digests ;
5. graver le compteur H2D et une borne par lot incluant le téléversement
   géométrique amorti ;
6. retirer le wire SoA seulement après réception CUDA.

Fixtures prioritaires : coordonnées aux bornes u16, `D2` maximal, identifiants
externes non monotones et bit 31, positions dupliquées avec identifiant
représentatif, index hors géométrie rejeté, cas cocirculaire, permutation
conservant l'ordre du cover, cover vide/minimal et lot surdimensionné.

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

## Ordre de commits proposé à Claude

1. **Finir l'instrument localement** : parseur au format réel, mutants/CMake,
   lifecycle, erreur asynchrone et timelines non additives.
2. **Recevoir ce seul instrument sur une petite session G4** : baseline causale
   avant toute réécriture, puis choisir l'ordre G0/G1/G2.
3. **Exécuter l'expérience choisie avec toggle** : pool persistant, wire indices
   ou compaction q4 ; recevoir d'abord l'égalité à 50 k, puis mesurer sur la
   charge bornée où q4 domine.
4. **Lancer en parallèle le pilote CPU sous cpuset** dès que son protocole local,
   son budget et sa marge de rapatriement sont reçus ; il ne dépend pas des
   refactorings GPU.

Cette séquence évite d'engager trois réécritures sur une causalité encore
ouverte. Elle donne à Claude une baseline interprétable, puis une seule
expérience falsifiable à la fois, tandis que la piste CPU avance indépendamment.

GCP non utilisé pour cet audit.
