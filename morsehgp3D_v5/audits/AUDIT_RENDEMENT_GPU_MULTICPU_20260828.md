# Audit ciblé — pourquoi les gains GPU et multi-CPU plafonnent

- **HEAD relu :** `63deda74` ; dernier chemin de performance reçu au pin `82f613d3`. Le HEAD ajoute une instrumentation q4 intrusive qui n'est pas encore mesurée.
- **Reçu principal :** `receipts/campagne_g4_v5_20260828_grille/`, G4, 48 fils, 50 k et 200 k.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Verdict

La réponse courte est : **le GPU actuel accélère une fraction devenue petite du
pipeline, après avoir payé sa matérialisation sur CPU ; le multi-CPU rencontre
ensuite des réductions séquentielles, des traînes indivisibles et beaucoup de
trafic mémoire.** Les meilleurs gains récents viennent de certificats qui
suppriment du travail, pas de l'ajout de matériel.

Deux nuances sont nécessaires :

1. Le GPU n'est pas uniformément sans gain. Au reçu courant, `terrain` gagne
   10,8 % sur le mur interne et `scanline` 2,2 %, tandis que `uniform` et
   `eight_clusters` régressent. Un passage unique et non contrebalancé ne suffit
   toutefois pas à établir un gain reproductible.
2. Le faible scaling multi-CPU sur G4 **n'est pas encore mesuré**. Les 221
   sorties suivies à 48 fils et les 36 sorties à 8 fils diffèrent de machine,
   de taille ou de pin ; aucune campagne ne balaie 1 à 48 fils sur le même cas.
   `ouvriers=48` prouve la création de fils, pas leur occupation ni un speedup.

## Ce que dit réellement le reçu GPU courant

Les murs ci-dessous viennent des sorties CPU/GPU appariées à 50 k. La dernière
colonne est un plafond d'Amdahl volontairement trop favorable : elle suppose que
**tout** le temps CPU `rects q3 + rects q4`, travail hôte inclus, disparaît.

| famille | CPU (s) | GPU (s) | CPU / GPU | q3 + q4 dans le mur CPU | plafond idéal |
|---|---:|---:|---:|---:|---:|
| `uniform` | 56,615 | 57,517 | 0,984× | 8,9 % | 1,10× |
| `terrain` | 16,432 | 14,650 | 1,122× | 51,6 % | 2,06× |
| `eight_clusters` | 62,157 | 63,898 | 0,973× | 23,7 % | 1,31× |
| `scanline_single_pass` | 12,629 | 12,348 | 1,023× | 48,0 % | 1,92× |

La q3 device est plus lente dans les quatre familles. La q4 gagne sur `terrain`
et `scanline`, mais perd sur `uniform` et `eight_clusters`. Le routage adaptatif
par `cover >= 256` ne change pas la conclusion sur un passage : 62,936 s pour
`eight_clusters` et 12,128 s pour `scanline`, contre 62,157 s et 12,629 s sur
CPU. Il choisit selon la taille du cover, pas selon le travail restant après
les certificats.

Le protocole `v5_campaign_remote.sh` exécute une seule série CPU, puis une seule
série GPU, toujours dans cet ordre. Il ne contrebalance pas les passages, ne les
répète pas et ne publie ni dispersion ni intervalle. Ces chiffres décrivent le
pin ; ils ne séparent pas un petit effet du bruit de machine.

## Pourquoi le GPU plafonne

### 1. Le port ne couvre que les corps q3/q4

`mhgp5_cuda` remplace uniquement `q3_override` et `q4_override`. Index, trois
WSPD, q2, RLE, préfiltre, census, expansion, fold et digests restent sur CPU.
Sur `uniform` 50 k, même un q3+q4 instantané ne peut donc procurer qu'environ
1,10×. À 200 k, le fold et le digest dominent encore `uniform`; le GPU courant
ne les touche pas.

### 2. Les certificats CPU ont retiré précisément le travail favorable au GPU

Les tests d'ancre, morceaux de corde, prétests et grille tuent les ancres/seeds
avant le balayage. Entre l'ancien reçu et le pin courant, le volume device
`eight_clusters` q3 est passé de 18,2 milliards à 126,5 millions de seeds,
environ 144 fois moins ; q4 passe d'environ 1,5 milliard à 91 millions. C'est
une bonne optimisation produit, mais elle réduit le levier du kernel.

La sonde q3 indique en outre que 98 % des seeds tués le sont après 8 à 15 sites
sur CPU. Un warp device évalue au minimum son premier paquet de 32 sites. Cette
amplification logique est certaine ; son coût dans le mur ne l'est pas encore.

### 3. Le « GPU » commence par construire le travail sur CPU

Le chemin q3 construit sur l'hôte les covers, compte deux fois les seeds aigus,
copie quatre tableaux SoA du cover, forme `AffineSeed` et calcule même la clé et
le niveau de chaque `BallCandidate` avant de connaître le verdict du device
(`q3_lane_batched.hpp:180-287`). Une seed morte a donc déjà payé une part
importante du chemin exact.

La q4 fait également sur l'hôte prétests, cover, grille, lentille, comptages et
formes de toutes les seeds, puis recopie coordonnées et identifiants du cover
dans chaque lot (`q4_lane_batched.hpp:193-365`). Le nuage et l'index ne sont pas
résidents globalement sur le device ; les mêmes points peuvent être recopiés
pour de nombreuses ancres.

### 4. La q4 est un aller-retour CPU/GPU en trois actes

Chaque lot q4 effectue :

- kernel K1, retour de tous les verdicts et synchronisation ;
- sélection CPU des seeds vivants et calcul des offsets ;
- kernel K2, retour d'un octet **par paire** et synchronisation ;
- comptage et compaction CPU ;
- kernel K3, retour des profondeurs et synchronisation.

Il n'y a ni compaction device entre K1/K2/K3, ni double buffering d'un même
exécuteur. La q3 fait aussi H2D, kernel, D2H et synchronisation à chaque lot.
Les buffers hôte sont des `std::vector` ordinaires, pas une mémoire épinglée
prévue pour recouvrir sûrement copies et calcul.

### 5. Quarante-huit producteurs ne forment pas un ordonnanceur GPU

Chaque ouvrier CPU possède un exécuteur `thread_local`, son flux et ses tampons.
Les équipes `std::thread` sont recréées pour la lane puis détruites : les flux
et allocations ne survivent pas à l'équipe. Les reliquats des 48 lots sont en
outre vidés après la jointure depuis le fil appelant, donc séquentiellement.

Cette architecture prouve l'égalité de chaque lot, mais ne borne ni le nombre
optimal de flux, ni la contention, ni la traîne. Il faut mesurer avant de
conclure que 48 flux sont trop nombreux ; le code actuel ne fait en tout cas ni
agrégation globale des petits lots, ni pilotage par charge device.

### 6. Le compteur historique `kernel_ms` ne permet pas le diagnostic

La q3 sait mesurer H2D, kernel et D2H séparément, mais ne remonte que le kernel
à la CLI. La q4 place son événement après les copies initiales et englobe les
deux retours, les synchronisations et les boucles CPU entre ses trois kernels.
La CLI additionne ensuite ces deux grandeurs de sens différent sur jusqu'à 48
flux et les imprime sous un seul `kernel_ms`.

Cette somme n'est ni un mur GPU ni un temps kernel homogène. Les phrases
historiques « la cause n'est pas le kernel » ne sont donc pas démontrées au pin
courant. Matérialisation et orchestration sont des suspects étayés par le code,
pas encore une décomposition quantitative.

### Réception du nouvel instrument `63deda74`

Le commit répond utilement à la demande en exposant les coupures q3 et q4, mais
il ne produit pas encore les **murs** annoncés dans son sujet :

- `sg3.wall` et `sg4.wall` additionnent les durées de tous les appels `scan()`
  exécutés par des ouvriers concurrents. Ce sont des temps-exécuteur cumulés,
  pas le chemin critique de la lane. Les soustraire à `rects q3/q4`, comme le
  propose le commentaire CLI, est dimensionnellement invalide ; renommer
  `executor_ms_sum` et mesurer séparément `lane_wall_ms` ;
- q4 ajoute `cudaStreamSynchronize` juste après H2D. Cette barrière n'existait
  pas au pin reçu et peut modifier le recouvrement entre flux. Un instrument ne
  doit pas changer l'ordonnancement qu'il attribue ; employer des événements et
  une récolte différée, ou démontrer par ablation que la barrière est neutre ;
- `q4 h2d` inclut aussi les réservations/allocation ; `k2+d2h` inclut réserve,
  H2D des offsets, kernel et retour ; `k3+d2h` inclut réserve, H2D des candidats,
  kernel et retour. Ces agrégats sont utiles, mais leurs noms ne séparent pas
  encore les coûts visés ;
- q3 expose proprement H2D/kernel/D2H, mais son `wall` inclut réservations et
  orchestration absentes de leur somme. Il faut publier cette différence comme
  overhead local, pas la confondre avec toute la préparation de lane ;
- aucun octet transféré, percentile de lots, pic de scans simultanés ou vrai mur
  global n'est encore gravé. Le commit ne possède pas non plus de reçu nvcc/G4.

Verdict sur `63deda74` : **bon instrument exploratoire, métrique de mur non
reçue**. Il faut corriger les noms et la barrière avant d'utiliser ses nombres
pour choisir une architecture.

## Pourquoi le multi-CPU devient sous-linéaire

### 1. Le fold conserve un noyau séquentiel par ordre

`prepare_fold` parallélise tri et internement, mais `reduce_fold` reste
séquentiel et parcourt un état d'union-find à accès dispersés. Le digest v4
parcourt ensuite séquentiellement clés, partition finale et deltas de chaque K.
`fold_inflight=2` ne parallélise pas une réduction : il autorise seulement deux
ordres B en vol. Le dernier gros K garde donc une traîne mono-fil.

Sur `uniform` 200 k, le mur total est 253,3 s et le mur du fold 115,0 s
(45,4 %), avec 128,2 s de reduce et 66,1 s de digest cumulés. Sur
`eight_clusters` 200 k, génération et fold représentent respectivement 50,4 %
et 30,9 % du mur. Ajouter des fils à la préparation ne supprime aucun de ces
octets ni la traîne du dernier ordre.

### 2. La famille lente ne place pas son goulot au même endroit

Sur `scanline` 200 k, la génération prend 244,1 s sur 267,7 s (91,2 %) et la
q4 par rectangles 214,5 s (80,1 %). Les lanes q2, q3 et q4 sont exécutées les
unes après les autres. À l'intérieur d'une lane, l'unité de travail est un
rectangle entier : ancres, seeds et sites d'un rectangle ne sont pas subdivisés
entre ouvriers. Les rectangles lourds produisent donc une traîne que le tirage
dynamique par rectangle ne peut pas couper.

Le reçu confirme le diagnostic algorithmique : rendre concurrent le fold fait
346 → 258 s sur `uniform` (1,34×, avec +23 % de RSS) mais ne change pas
`scanline` ; la grille, qui retire des balayages, fait 502 → 268 s sur
`scanline` (1,87×). Retirer le travail lourd a plus d'effet que lui ajouter des
fils.

### 3. Une partie importante du coût est du trafic, pas du calcul divisible

Les sorties par ouvrier sont concaténées séquentiellement après chaque passe.
RLE termine par `unique/erase`; préfiltre, census et expansion fusionnent leurs
vecteurs temporaires ; le tri stable parallèle copie un tableau complet et se
synchronise à chaque niveau. Le fold `uniform` 200 k manipule 89,8 millions
d'événements et 562 millions de facettes cumulées.

Le commentaire du reduce attribue environ 30 défauts de cache par événement à
ses tableaux historiques. Cette lecture est cohérente avec le plafonnement,
mais aucune campagne ne publie encore bande passante, LLC misses ou topologie :
une saturation mémoire reste à mesurer, pas à proclamer.

### 4. `--threads` n'est pas un budget global

Les primitives recréent et joignent leurs équipes à chaque passe, sans pool
persistant. Pendant qu'une préparation A emploie jusqu'à `threads` ouvriers,
des fils B du fold peuvent tourner en plus. `--threads=1` n'impose donc pas un
processus strictement mono-cœur ; `--threads=48` ne garantit pas non plus que 48
cœurs font du travail utile. Le champ `fold=48` n'est que le maximum d'ouvriers
créés par une préparation.

Une contre-sonde locale au HEAD, un passage non contrebalancé sur une machine
4 cœurs / 8 fils matériels, `uniform`, n=4000, K=1..10, digest actif, donne :

| `--threads` | mur (s) | accélération contre 1 |
|---:|---:|---:|
| 1 | 58,04 | 1,00× |
| 2 | 31,40 | 1,85× |
| 4 | 17,70 | 3,28× |
| 8 | 13,57 | 4,28× |

Ce n'est pas un reçu de performance : ordre non contrebalancé, un seul passage,
machine partagée et baseline encore pipelinée par K. Cela réfute seulement la
formulation « aucun grand gain CPU » : le gain existe jusqu'aux cœurs locaux,
puis le rendement supplémentaire chute fortement sur les fils matériels.

## Plan de fermeture utile

### P0 — mesurer avant la prochaine réécriture

1. Ajouter une campagne `SCALE_THREADS` au même pin, même G4, mêmes entrées :
   `threads` dans 1, 2, 4, 8, 16, 24, 32, 48 ; `fold_inflight` dans 1, 2, 3 ;
   digest ON et OFF séparés. Contrebalancer l'ordre, répéter et exiger les mêmes
   digests et compteurs de travail.
2. Graver topologie/affinité et les sorties GNU time complètes, puis au minimum
   task-clock, CPUs réellement utilisés, cycles, LLC misses, migrations et
   bande passante si disponible. Les `.status.time` actuels ne sont pas suivis.
3. Pour le GPU, publier de vrais murs q3 et q4 : préparation, octets H2D,
   K1/K2/K3, octets D2H, compactage, post-traitement, lots p50/p95/max et nombre
   de flux actifs. Ne jamais additionner des flux et nommer la somme « mur ».
4. Faire un banc CPU/GPU contrebalancé et répété, avec échauffement explicite,
   sur les mêmes pins et au moins aux tailles où q4 domine. Le contrat 50 k seul
   est trop favorable à Amdahl sur `uniform` et trop petit pour le cas
   `scanline` 200 k qui motive le port.

### P1 — expériences étroites, chacune ablatée

- GPU q3 : différer clé et niveau aux seules seeds survivantes.
- GPU q4 : faire prefix-sum et compaction sur device entre les kernels, puis ne
  rapatrier que les émissions et compteurs agrégés.
- Tester un tableau global de positions/identifiants résident et un ordonnanceur
  borné à quelques flux, puis pinned + double buffering. Ce sont des hypothèses
  à comparer au chemin courant, pas des gains annoncés.
- CPU : pool persistant avec budget global incluant A et B ; mesurer la charge
  par ouvrier et fractionner les rectangles/ancres au-dessus d'un seuil de
  travail, sans changer l'ordre de fusion.
- Réduire les copies par des offsets précomptés et des fusions déterministes
  parallèles ; paralléliser au moins l'initialisation des gros états. Une vraie
  parallélisation du reduce exige une nouvelle preuve de l'ordre des deltas.

`fold_inflight=3/4` ne doit venir qu'après correction des deux défauts P1 de
sûreté déjà ouverts et une campagne mémoire : cela peut réduire le recouvrement
ou la traîne, mais ne parallélise toujours pas le dernier K.

## Conclusion pour Claude

Il ne faut pas chercher d'abord « un kernel plus rapide » ni « plus de fils ».
Le chantier est : **mesurer la fraction accélérable, supprimer le travail avant
de le déplacer, puis réduire les frontières hôte/device et les traînes
indivisibles**. Le GPU a une cible crédible lorsque q4 reste dominante à grande
taille ; le multi-CPU a déjà un gain réel, mais sa prochaine marche passe par le
budget global, la granularité et le fold, pas par `threads=48` seul.

GCP non utilisé pour cet audit.
