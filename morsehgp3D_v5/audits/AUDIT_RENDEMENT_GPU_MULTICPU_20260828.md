# Audit de résolution — rendement GPU et multi-CPU

- **Dernier pin reçu :** `ab2c2563` ; mesures CPU au pin `82f613d3` et
  instrumentation device au pin `63deda74`.
- **Worktree observé :** postérieur à `ba31c169`, encore non committé. Il
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

La solution utile n'est donc pas seulement une nouvelle campagne. Elle tient
en trois changements bornés :

1. **un petit pool GPU persistant**, indépendant des producteurs CPU ;
2. **la géométrie globale résidente**, avec seulement des indices u32 envoyés
   par cover ;
3. **la compaction q4 stable sur device**, avec un seul retour final.

Ces changements attaquent directement les octets et les synchronisations qui
plafonnent le port actuel. Pour le multi-CPU, il faut d'abord séparer la
sensibilité au budget d'ouvriers d'un véritable speedup 1→N sous cpuset.

## Base chiffrée conservée

Au reçu n° 12, les murs CPU/GPU appariés à 50 k restent mixtes :

| famille | CPU (s) | GPU (s) | CPU / GPU |
|---|---:|---:|---:|
| `uniform` | 56,615 | 57,517 | 0,984× |
| `terrain` | 16,432 | 14,650 | 1,122× |
| `eight_clusters` | 62,157 | 63,898 | 0,973× |
| `scanline_single_pass` | 12,629 | 12,348 | 1,023× |

Le reçu prouve compilation/exécution CUDA, quatre digests CPU/GPU égaux et le
mutant device tué. Un passage CPU puis GPU, non contrebalancé et non répété, ne
prouve pas un gain reproductible. Il suffit cependant à montrer que le port
n'est pas cassé et que certaines familles ont un levier q4.

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
- la sortie porte un numéro de lot et est fusionnée dans l'ordre déterministe
  actuel ;
- la géométrie device est partagée en lecture seule entre les exécuteurs.

Le sweep `gpu_executors={1,2,4,8}` devient alors interprétable. Le nombre de
producteurs CPU peut rester 48 sans créer 48 contextes de travail concurrents
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

Téléverser une fois les positions quantifiées et `PointId` du `CloudIndex`.
Pour chaque cover, n'envoyer ensuite que son ordre d'indices u32, les paramètres
d'ancre et les petits offsets nécessaires. Le kernel reconstruit `u` et `q` en
arithmétique entière exacte.

Le terme linéaire par site passe ainsi :

- q3 : environ 32 → 4 octets/site ;
- q4 : environ 60 → 4 octets/site.

La q4 récupère coordonnées et identifiants depuis la géométrie résidente ;
`lens_sites` peut référencer les positions du cover. L'ordre du cover reste
inchangé, donc l'ordre des seeds et émissions reste falsifiable contre le
chemin actuel.

Implémentation sûre :

1. introduire un objet RAII `GpuGeometry` partagé par les exécuteurs ;
2. conserver temporairement les deux wires, SoA actuel et `site_index_u32` ;
3. ajouter un kernel alternatif qui reconstruit `u/q` ;
4. comparer bit à bit verdicts, profondeurs, émissions, compteurs et digests ;
5. graver le compteur H2D et une borne par lot incluant le téléversement
   géométrique amorti ;
6. retirer le wire SoA seulement après réception CUDA.

Fixtures prioritaires : coordonnées aux bornes u16, `D2` maximal, cas
cocirculaire, permutation conservant l'ordre du cover, cover vide/minimal et
lot surdimensionné.

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
- rapatrier seulement les émissions finales et les compteurs agrégés.

La stabilité est contractuelle : le résultat doit rester dans le même ordre
que le filtrage CPU, indépendamment du découpage en blocs. Une porte compare
chaque tableau intermédiaire sur de petits lots avant de se contenter du
digest final.

G1 précède G2 : réduire le trafic initial permet de mesurer proprement ce que
les retours intermédiaires coûtent encore.

## Multi-CPU — mesurer puis corriger le vrai goulot

`--threads=1` n'est pas un run à un CPU. Le fil principal peut préparer K+1
pendant qu'un fil B réduit K, et plusieurs B peuvent être actifs avec
`fold_inflight > 1`. La phase `SCALE_THREADS` mesure donc une réponse au budget
d'ouvriers, pas encore un speedup 1→N.

Scinder le protocole :

### A. Scaling des ouvriers

- `fold_inflight=1` ;
- digest OFF pour isoler le travail, puis une strate digest ON ;
- processus sous cpuset exact de 1, 2, 4, 8, 16, 24, 32 et 48 CPU ;
- cpuset, affinité, topologie et CPU réellement utilisés gravés ;
- échauffement et au moins trois répétitions contrebalancées.

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

1. **Finir l'instrument** : parseur au format réel, mutants/CMake, lifecycle et
   timelines non additives ; aucun run GCP nécessaire.
2. **Pool persistant** : 1/2/4/8 exécuteurs, résidus dans la file, égalité des
   sorties.
3. **Wire indices** : `GpuGeometry` + `site_index_u32` en parallèle du wire
   actuel ; réception CUDA à 50 k.
4. **Compaction q4 device** : portes des intermédiaires, puis ablation du D2H.
5. **Pilote CPU sous cpuset** : seulement après que le protocole local et son
   budget de session sont reçus.

Cette séquence aide Claude à obtenir un gain réel : elle réduit d'abord les
octets et les synchronisations, puis mesure le plafond résiduel. Elle évite de
lui demander simultanément une réécriture GPU, un nouveau fold et une campagne
longue.

GCP non utilisé pour cet audit.
