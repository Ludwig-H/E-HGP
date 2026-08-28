# État courant audité de MorseHGP3D v5 — 28 août 2026

- **Dernier pin produit inspecté :** `55c1f105`, qui épingle le correctif de
  complexité trouvé par le miroir, sa sonde et les mesures documentaires. Il
  prolonge le fold à créneaux et les raccords hôte G0/G1 de `c19dc60d`, le
  pool/sonde de `194a0bc2` et le réducteur vivant durci de `bc66ade7`. Les
  constats historiques G0 portent sur `fe54ccca`, la campagne device/SCALE sur
  `c95cfa95`, G1 q3 sur `dd928111`/`839cf1ec` et G1 q4 sur `556c421e`.
- **Pin de réception G0/G1 q3 :** `0656bf4c`, sans code produit.
- **Worktree observé avant ce delta :** `HEAD=origin/main=819cac3c`; aucun code
  produit n'est postérieur à `55c1f105`. `2192ec9e`–`3636d5b0` corrigent la
  proposition de raffinement dans la question active ; `fd2844fb`, `57deaaa6`
  et `4ecb57d4` ajoutent puis requalifient une question d'exposants et des
  instruments exploratoires dans `rect_probe.cpp`, sans modifier le produit.
  `905c5361` ajoute une mesure transitoire désormais consolidée dans la question
  active et retirée du tip ; `b550ec0d` tranche V42, `ad42438e` borne le taux
  de récupération et `819cac3c` corrige dans l'instrument la population q4 du
  compteur `k=1`. La rétractation `a54bd528` retire justement le claim des
  31 jours, mais ses nouveaux temps 10 M restent fondés sur un débit non mesuré
  appliqué à une autre unité ; elle est consolidée dans la question active puis
  retirée du tip.
  Le probe non suivi
  `.codex_fold_contract_probe.cpp`, appartenant à un autre travail, reste hors
  preuve. La réponse transitoire de Claude au miroir est consolidée ici puis
  retirée du tip, conformément à la convention du dossier.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Verdict utile à Claude

**Orange constructif : `c19dc60d` garde la bonne architecture.**
Notification du ticket, démarrage transactionnel, domaine `1..8`, latch
causale, TLS non allouant et fermeture en place sont de vraies corrections ; il
ne faut ni les jeter ni redessiner la lane. La réception du **contrat de sûreté
G0** demande maintenant quatre raccords locaux : compter une soumission seulement
après `queue_.push_back` réussi, garantir un `exception_ptr` fatal non nul sans
allocation dans `close_fatal`, rendre les scénarios de fermeture causaux sans
`sleep_for`, puis relier une exception CUDA typée à `close_fatal` avant que le
worker puisse prendre un autre lot. Ces points ne bloquent ni G1 ni le fold,
mais précèdent tout claim de confinement device.

La sonde fold historique reste correctement libellée micro-banc. Son régime
`--dump/--from` exécute, lui, un seul réducteur par processus et constitue la
bonne couture de miroir. Il a déjà été utile : sur le témoin massif de Claude,
le parcours des capacités de tous les deltas à chaque lot a fait passer une
réduction à 981 s. `55c1f105` déplace ce parcours vers les balayages
bornés et supprime ainsi le coût quadratique par préfixes ; garder cette
correction. Les nombres exacts appartiennent à une sortie brute ou un reçu,
tandis que le commentaire source peut simplement documenter la complexité
évitée.

Le bras vivant sans rejeu est une **ablation de l'état L2**, pas encore le
miroir d'un objet résident complet. Le bras avec rejeu n'a en revanche besoin
d'aucune copie de `keys` : `reduce_fold_live(FoldPrepared&&)` ne déplace que
`fp.r` et laisse `fp.keys` valide jusqu'au retour de `measure`. Rejouer
directement depuis `fp.keys` après la réduction évite la copie annoncée de
274 Mo et ferme ce biais sans changer l'API produit. Il reste à partager le
replayer T5 strict, comparer une signature commune et faire échouer la sonde sur
tout refus ou désaccord.

L'artefact n'est pas encore un reçu stable : un compteur natif suivi de la
représentation mémoire de `ForestEvent`, sans magic, schéma, K, taille de
record, boutisme, digest ni contrôle de fin de fichier, peut provoquer une
allocation massive avant validation. Un wire champ par champ doit être borné
contre la taille réelle du fichier avant allocation. Le RSS est celui du
processus dédié — lecture, préparation, sortie et rejeu compris — et non celui
du réducteur seul ; si le fold n'établit pas un nouveau pic, le différentiel
reste inconclusif.

Dans `docs/ECHELLE.md`, remplacer donc « RSS attribuable » par « pic de
processus apparié ». Le −8,6 % compare le résident au vivant **sans partition**,
alors que le bras vivant avec rejeu dépasse le résident ; il isole une ablation
utile, pas encore un gain mémoire de l'objet complet.

La session 13 fournit par ailleurs une baseline CUDA cohérente. La session au
pin `839cf1ec` compile et exécute nominalement G0 et G1 q3 : huit exécuteurs au
total au lieu de 98, digests appariés et baisse H2D q3 visible. Les pins G1 q4
complètent ensuite le wire sans remplacer SoA. Les contrats
fail-closed/non-vacants G1 restent à fermer sans reprendre ces travaux.

Le pin `c19dc60d` répond déjà aux deux demandes G1 les plus rentables : bornage
**en valeur** avant lancement et instrumentation qui rend la branche observable.
Il sépare désormais aussi géométrie absente et géométrie déclarée vide ; conserver
ce patch. Le libellé des lots sans seed reste un nettoyage non bloquant. La
réception device est, elle, encore fail-open : le validateur accepte un faux
pilote qui ignore `--gpu-wire` et n'imprime aucun compteur de branche, la
campagne n'exécute pas les deux mutants de retombée SoA et q4 n'a aucun
`PointId` adverse au-delà du bit 31. Fermer ces trois témoins reçoit le wire sans
refonte ni nouvelle matrice SCALE.

Le pin `bc66ade7` consolide le bon cœur L2 : égalité de vie par lot,
balayages structurels, vacuité finale, plafond par alias, cas adverse court et
timeouts explicites. Il ne faut revenir sur aucune de ces corrections. Les
finitions avant L3 sont locales : replayer T5 strict partagé, comparateur de
fixtures complet, deux invariants de listes et attribution exacte des témoins
mémoire. Une fixture du décalage arrière ne reste utile que si l'ancien hash est
conservé comme repli ; le chemin actif à créneaux n'en a plus besoin. Ces
finitions bornent le claim, pas l'utilité du réducteur.

Le fold actif va plus loin : il remplace le hash vivant par un tableau de
créneaux de taille `peak_live_exact`, une table directe `fid -> créneau`, puis
réutilise l'alias résolu dans `slot_alias`. La construction par intervalles est
cohérente avec l'ordre naissance-avant-mort du lot ; les indices d'arène restent
valides après les relocalisations small-to-large. Garder cette piste. Le nouveau
chemin de débordement retourne maintenant avant tout `unite`, vide les sorties
partielles et expose `resource_exhausted` ; le mutant et les deux cas de
frontière sont des progrès réels. Mathématiquement, les vies fermées
`[FIRST, LAST]` forment un graphe d'intervalles parfait : allouer toutes les
naissances avant les morts du lot puis réutiliser la liste libre au lot suivant
atteint exactement le chevauchement maximal. Il n'y a pas de verrou
algorithmique caché à résoudre ici.

Le pin `c19dc60d` ferme le marquage exact libre/vivant, les valeurs littérales
des deltas de frontière, le préfixe `resource_exhausted`, la vacuité des deux
sorties et le compte de débordement. `55c1f105` ferme en plus le défaut
quadratique de la mesure. Quatre coutures locales, dans cet ordre, suffisent
maintenant :

1. **Sécuriser l'oracle avant de l'étendre.** Dans la liste de composante,
   tester `x < nslots` avant toute lecture de `av[x]`, puis borner
   `av[x].fid` avant l'accès à `slot_of_fid`; capturer `next` seulement après
   ces gardes. Parcourir ensuite **tous** les `slot_of_fid` pour détecter une
   entrée stale. Un mutant de partition à cardinalité conservée doit tuer ce
   témoin précis.
2. **Rendre les deux petites portes causales.** La fixture de capacité vérifie
   déjà le bon refus local, mais le binaire peut encore rendre 4 grâce à un
   désaccord synthétique ultérieur : terminer immédiatement sur ce verdict
   dédié. Pour les frontières, comparer `liv.batches` à la valeur littérale
   `c.batches`, graver les vecteurs `batch_levels` attendus et inclure `level`
   dans le rendu des deltas.
3. **Finaliser les capacités une seule fois, exactement.** `cfree`, `slot_mark`,
   les scratchs imbriqués et les deltas imbriqués sont déjà comptés : ne pas
   les réimplémenter. Il manque seulement `born_at`/`died_at`, détruits avant le
   relevé, et `batch_levels`. Un helper commun appelé après la dernière
   croissance, y compris avant un refus anticipé si ces métriques y sont
   promises, donne les maxima exacts sans échantillonnage. Publier les cinq
   postes comme capacités de conteneurs séparées, jamais comme somme
   allocator-précise.
4. **Comparer le même objet sans copie.** Conserver le bras sans rejeu comme
   ablation, puis faire du bras avec rejeu le miroir complet en utilisant
   `fp.keys` directement, le replayer strict et un digest commun. Versionner le
   wire avant d'en faire un reçu.

Le L2 complet reste O(F + I), et seul L3 peut retirer les tableaux
d'occurrences. Contrebalancer ensuite les répétitions sous cpuset et conserver
les sorties brutes. Les tableaux 8 k/16 k historiques restent des
micro-mesures ; ils ne deviennent pas rétroactivement des mesures miroir, et
deux tailles ne prouvent pas un facteur asymptotiquement constant.

**Réponse V35 :** retenir le vivant comme **baseline L2 candidate** d'une future
voie d'échelle, à sélectionner lorsque le futur préflight prouvera que le
résident ne tient pas ; aujourd'hui L2 reste O(F + I), hors chemin produit et
sans ce routeur. Le surcoût historique d'environ 1,6 vient du micro-banc ; le
miroir courant rapporte 2,52 pour le bras **sans rejeu**, donc pour un objet
incomplet. Ce sont des budgets à réduire, pas une destination ; remesurer le
miroir complet après suppression de la copie et raccord du replayer strict. Ne
pas remplacer encore l'arène de composantes : `cv` est déjà un vecteur contigu
à indices recyclés. Profiler séparément lifetime, naissances/morts, unions,
tris, rejeu et `slot_alias`; n'essayer une seconde disposition par créneau que
si les accès `cv` ressortent effectivement. La suppression L3 de
`firstb`/`lastb`/`slot_of_fid` peut apporter davantage.

Sur le seul
[run reçu `uniform n=50000`](../receipts/campagne_g4_v5_20260828_instrument_scale/out/contrat_uniform_n50000.txt),
la part mur du fold vaut
`25245,8 / 56290,8 = 44,85 %` ; le calcul conditionnel d'Amdahl à environ +27 %
est correct, sans devenir une part générale des autres familles ou du nouveau
fold.

**Réponse sur le coût de recherche q2/q3/q4 : la crainte est confirmée, mais
elle dépend fortement de la géométrie.** Au meilleur grand reçu disponible
(`82f613d3`, CPU 48 fils, 200 k), les corps q2/q3/q4 prennent 1,103 / 7,869 /
12,323 s sur `uniform`, 0,940 / 47,667 / 83,786 s sur `eight_clusters`, et
0,483 / 24,551 / 214,544 s sur `scanline`. Entre 50 k et 200 k, q3/q4 sont
presque linéaires sur `uniform`, d'exposants observés 1,61/1,56 sur les
clusters, mais 2,07/2,76 sur `scanline`. q2 ne cherche pas de `x`; q3 peut
rescanner le cover pour chaque `x`, et q4 ajoute les `y` puis parfois un
nouveau scan. Sur `scanline`, les rectangles restent presque linéaires, mais
les ancres puis les `x` aigus approchent déjà une pente quadratique : le verrou
est dans `|A| × |B|`, puis `x × cover` et q4 `x × y × cover`, pas dans la WSPD
seule. Le
[tableau, ses limites et la correction proposée](QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md#complément--la-crainte-quadratique-est-confirmée-sur-scanline)
est intégré à la question active. Ces 200 k prouvent un verrou au pin mesuré,
pas la performance de `55c1f105` : instrumenter les itérations puis rejouer
50/100/200 k au pin courant avant toute extrapolation. Il n'existe encore
aucune mesure 1 M ou 10 M.

**Architecture constructive reçue pour étude : WSPD de paires, puis arrangement
local de centres par ancre.** Une WSSD standard couvre les simplexes pour une
approximation, mais ne fournit ni partition exacte ni rang ; les produits
symétriques de trois ou quatre boîtes ont déjà montré leur mélange jusqu'aux
feuilles. Pour une ancre diamètre `(a,b)`, chaque tiers devient une droite
orientée dans le plan médiateur. q3 interroge le point marqué de cette droite à
profondeur au plus 8 ; q4 énumère seulement les intersections de profondeur au
plus 7. La borne locale visée devient `O(m log m + m*K)` et au plus `8*m`
sommets q4 à `smax=11`, au lieu de `binom(m,2)`. Elle ne borne pas le nombre
global d'ancres ni `M=sum(m)` : mesurer d'abord `m`, `M`, `Z`, quantiles,
fallbacks et scratch, puis construire un oracle exhaustif borné. Le producteur
shallow échoue sa porte s'il forme d'abord toutes les intersections. Les
[formules, dégénérescences et étapes R0–R3](QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md#réponse-à-louis--généraliser-la-wspd-mais-par-les-centres)
sont transmises à Claude ; aucun q5 n'est requis en dimension trois.

Les mesures Claude `954ec1af` à `ff5931fd` renforcent le bon diagnostic sans
encore recevoir une architecture : le nombre de rectangles vivants est proche
du linéaire aux tailles sondées, mais leur masse d'ancres et le travail q4 par
seed se dégradent selon la famille. Deux erreurs de normalisation sont fermées
dans la question active : le tableau q4 divisait par les rectangles q3, et
`jung_cert_skip` total était présenté comme travail par ancre. La sonde de
classes de rayon reste exploratoire : ses 159 M seeds `scanline` q3 16 k sont
contrefactuels, tandis que le chemin reçu n'en énumère que 13,6 M ; elle
n'imprime pas encore la masse post-filtres annoncée. Ne déduire ni `cover`
proportionnel à `D^3`, ni un rectangle entier tuable, ni un verrou GPU de cette
sonde sans sorties brutes, compteur résiduel et certificat exact par bloc.
Réutiliser d'abord `q3_cert`/`q4_cert`, puis ajouter les visites de prétest,
handles, grille et profondeur q4, ventilées après chaque filtre. Le détail
constructif et les conservations sont dans la requalification V29.

**Réponse V36–V41 : garder le signal, refuser les claims.** Les extrapolations
10 M de `fd2844fb` sont arithmétiquement cohérentes sous un débit inventé et une
pente figée, mais ne prouvent pas que trois régimes tiennent huit heures. Le
seuil proposé ne définit pas non plus la sous-quadraticité et ferait déjà
échouer clusters et `scanline` sur leurs autres compteurs/intervalles. Séparer
diagnostic de pente et budget produit. Fermer seulement la coupure **aveugle**
par `Dmax`, jamais tout certificat dépendant du rayon. La session 11 fournit en
outre déjà 100/200 k `scanline` : le dernier doublement donne 3,220 sur
`jung_cert_skip` et 3,305 sur le mur q4, ce qui réfute directement la projection
1,5 h fondée sur la seule pente 32→50 k.

La descente prolongée de `57deaaa6` trouve un signal q3 non vacant, mais sa
profondeur 40, son ratio estimé et son commentaire ne sont pas recevables comme
algorithme. `separated` n'est pas héréditaire, tandis que le compte sémantique
de témoins universels est monotone sur un sous-produit ; tester les deux enfants
avant effet, conserver par garde `core=max(parent,fresh)`, exclure q2, borner
`L` à 0/1/2/3, puis graver ledger u128, multiensemble trié, digests, événements
et niveaux. Les 92,1 M sites de cover évités sont un prorata contrefactuel et ne
se comparent pas aux 27,3 M nœuds visités comme un speedup 3 pour 1. Les
257 810 « sous-rectangles » incluent 173 190 racines, et le binaire rejoué
annonce encore `pin_configure=0b3f3fd6` : aucun reçu `57deaaa6` n'existe. La
[réponse détaillée](QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md#réponse-auditée--v36-à-v41)
et les fixtures sont consolidées dans les deux questions actives.

La réception `4ecb57d4` retire correctement le faux plafond et accepte les deux
rails. Elle ne transforme toutefois pas les ratios de la sonde en verdict de
temps : ils ne classent pas la priorité q4. La prochaine
série épingle la règle `coord(n)`, pas une valeur `coord` constante. V42 est
tranché comme porte bornée : multiensemble littéral sur petits arbres, ledger
u128 seul à l'échelle, mutants perte/doublon/effet avant rollback.

La mesure `905c5361` ne ferme pas davantage le choix. Avec ses seuls totaux,
la récupération `k=1` de `scanline` q3 est dans `[70,581 %, 100 %]`, pas égale
à 70,6 % : il manque l'intersection exacte entre morts du raffinement et morts
des prétests. Le zéro `k=1` q4 borne seulement la partie post-prétests ; il
n'interdit pas une économie amont. Les rapports covers estimés / nœuds visités
ne sont toujours pas des verdicts de temps.

**Ablation immédiate multi-CPU :** ne pas commencer par un nouveau pool général.
Sur le témoin qui porte `490143/1231555/1353144` rectangles à 48 workers,
remplacer les `parallel_items(nrect, ...)` des lanes par le
`parallel_ranges(...)` déjà reçu réduit statiquement 3 074 986 `fetch_add` à
1 296. Il conserve l'objet post-RLE, les digests et les compteurs sémantiques
additifs, mais pas l'ordre brut, les frontières, histogrammes/maxima de lots,
octets ou timings device. Les paquets de 1,3 k à 3,5 k rectangles peuvent aussi
créer une traîne non volable : instrumenter tickets et temps occupé, puis
mesurer le mur à 1/2/3/8/48 fils avant de retenir ce découpage. Remplacer ensuite
les treize équipes no-op d'`expand.hpp` par un `make_chunk_plan` pur. Ces deux
petits essais sont indépendants du fold et diront si création de threads et
contention atomique expliquent une part du plafond avant d'engager l'équipe CPU
persistante.

`c19dc60d` a déjà épinglé ensemble les avancées source G1, fold à créneaux et G0
hôte : ne pas les refaire. Leurs trois **suivis** restent indépendants : fold
après sûreté du scan, causalité et pics ; G0 après admission comptable, secours
fatal et barrières ; G1 après selftest fail-closed, `PointId` q4 adverse et
exécution device minimale des deux mutants. Aucun nouveau G4 n'est nécessaire
avant les fermetures locales du fold et de G0. T5, le fold massif et le
protocole CPU sous cpuset restent indépendants.

## Ce qui est reçu et réutilisable

### Campagne 13 au pin `c95cfa95`

Le [reçu versionné](../receipts/campagne_g4_v5_20260828_instrument_scale/RECU.txt)
garde 38 runs nominaux au code 0, le mutant device au code 4 et six
comparaisons CPU/GPU aux digests détaillés égaux. C'est une preuve fonctionnelle
bornée de la baseline, jamais un speedup. Le validateur du pin l'accepte ; celui
du HEAD demande depuis quatre contrats `gpuidx`. Cette dérive doit être traitée
par un `campaign_schema` et un dispatch versionné, sans réécrire les obligations
du reçu historique. La certification `SPOT`/`TERMINATED` n'existe que dans le
journal local non versionné et ne devient donc pas une preuve archivée.

### Campagne G0/G1 q3 versionnée à `0656bf4c`, source `839cf1ec`

Le [reçu](../receipts/campagne_g4_v5_20260828_g0_g1/RECU.txt) garde 28 runs au
code 0, le mutant device au code 4 et des digests égaux. Il reçoit nominalement
huit exécuteurs G0 au total et le wire q3 index non vide, avec moins d'octets
H2D ; il ne reçoit ni la sûreté générale du pool ni q4 index. Son validateur
épinglé l'accepte, tandis que celui du HEAD exige un schéma plus récent. La
prochaine version doit donc figer commandes, trois artefacts par run,
`timing_scope`, témoins de pool et branche réellement exécutée. Le
[reçu de sécurité](../receipts/campagne_g4_v5_20260828_g0_g1/SAFETY_RECEIPT.txt)
lie les deux coupe-circuits, le modèle `SPOT` et l'arrêt `TERMINATED` à une
transcription expurgée elle-même liée au hash du journal brut local. Les mesures
et corrections chiffrées restent dans
[`AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md`](AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md).

### Architecture G0 au pin `fe54ccca`

Les choix structurants sont appropriés : nombre d'exécuteurs borné, un
exécuteur persistant par worker, producteurs bloquants, sorties conservées dans
le shard d'origine et raccord effectif aux lanes q3/q4. La porte hôte nominale
et ses deux mutants passent. G0 reste donc une bonne base à corriger, pas un
prototype à remplacer.

### G1 q3/q4 aux pins `dd928111`–`556c421e`

Le chemin généré ne montre pas de faute arithmétique, d'ordre ou de durée de
vie nominale. Sous le profil u16, chaque coordonnée affine a une valeur absolue
au plus 131 070, `D2` est inférieur à `2^34` et la puissance q reste inférieure
à `2^36` : i64 puis `double` sont exacts dans cette plage. q3 conserve le corps
DI128 ; q4 récupère le même `PointId`, les mêmes offsets locaux de lentille et
le même ordre de cover que le SoA. La géométrie est copiée avant les jobs puis
seulement lue, et `submit_and_wait` conserve aujourd'hui sa durée de vie.

Les deux premiers points définissent le contrat fonctionnel G1 ; le troisième
répare une porte CUDA déjà déclarée. Les trois derniers sont des durcissements
de coût à mesurer ensuite, pas des conditions artificielles de réception :

1. **Réception — indices.** Le validateur de lot ne borne que la **taille** de `site_index`, pas ses
   valeurs. Avant tout lancement, exiger chaque index strictement inférieur à
   `GpuGeometry::count`. En q3 seulement, exiger aussi la présence conjointe
   index/géométrie d'ancre ; q4 borne ses indices contre la géométrie résidente.
   Les fixtures `index == count` et `UINT32_MAX` doivent lever sans lancement.
   Le builder normal produit bien des indices valides : il s'agit d'un contrat
   fail-closed, pas d'une divergence des sorties actuelles ;
2. **Réception — branche.** La porte imprime le wire demandé, sans prouver le wire exécuté. Un mutant
   remplaçant le branchement index par SoA garderait verdicts et digests verts,
   et le faux GPU du selftest ignore aujourd'hui `--gpu-wire` tout en étant
   accepté. Ajouter `index_lots`, `soa_lots` et les octets catégoriels, puis
   exiger `index_lots == lots > 0`, `soa_lots == 0` et
   `site_soa_bytes == 0`. Un mutant `wire-index-force-soa` et un selftest
   `GPU_IGNORE_WIRE=1` doivent être refusés. Pour q4, employer aussi des
   `PointId` non monotones au-delà du bit 31 afin de tuer `PID=dense_index` ;
3. **Suite CUDA — mutant.** La porte q3 ne parse pas `--inject`, bien que CMake passe
   `--inject=route-ignore-threshold` au CTest attendu à 4 : sur device il
   rendrait 2 par argument inconnu. Raccorder le registre de mutants et exercer
   ce CTest, absent de la commande `gpu_lane` de la nouvelle session. Les
   planchers q3/q4 supplémentaires sont des durcissements séparés ;
4. **Performance — double wire.** Le gain `32 -> 4` en q3 et `60 -> 4` en q4 ne concerne encore que la copie
   PCIe par lot. `Q3Batch` conserve 32+4 octets/site, `Q4Batch` 60+4, et chaque
   exécuteur réserve les deux familles de buffers même en SoA. Passer les lots
   et réserves à une représentation étiquetée exclusive ; le mode SoA ne doit
   payer aucun index et le mode index ne doit conserver aucun payload SoA ;
5. **Performance — contexte.** q3 puis q4 téléversent chacun `xyz + PointId`, soit 16 octets/point deux fois
   et 320 Mo à 10 M de points. Créer une seule `GpuBackendContext` par run,
   possédant une géométrie partagée et les deux pools, avec
   `resident_uploads == 1`. Déclarer la géométrie avant les pools ou fermer et
   drainer explicitement ceux-ci ;
6. **Mesure — setup.** Les octets de cette géométrie sont ajoutés à `h2d_octets`, mais son upload
   précède `lane_wall_ms` et n'entre pas dans `h2d_ms`. Graver séparément
   `resident_h2d_bytes`, `resident_h2d_ms` et un mur incluant le setup. Une
   spécialisation SoA/index ou un chargement local q4 ne se justifie qu'après
   profil ; l'audit n'impose pas cette forme de code.

L'ordre interne est donc : sûreté d'index testable sur l'hôte, puis preuve de
branche et CTest mutant à recevoir sur G4 ; ensuite seulement lots mono-wire,
contexte partagé et métriques de setup selon leur effet mesuré. Ces derniers ne
bloquent ni les digests déjà obtenus ni la réception fonctionnelle bornée.

### Relecture constructive du pin G1 hôte `c19dc60d`

Le patch implémente les points 1 à 3 : `GpuGeometry::count` borne chaque valeur,
les fixtures `index == count` et `UINT32_MAX` refusent, les compteurs distinguent
index et SoA, le mutant `wire-index-force-soa` possède une signature dédiée, et
q3 parse enfin `--inject`. Le contrat d'indices est exercé sur CPU et la porte
`mhgp5_batch_contract` rend 0 ; branche et mutants restent à recevoir sur
device. L'agrégation des statistiques de lot reste sous le mutex de lane ;
aucune nouvelle race hôte n'est visible.

La sentinelle de géométrie est maintenant fermée proprement : la vue porte
`geom_declared` séparément de `n_geom_points`, et les fixtures q3/q4 refusent un
lot indexé non vide contre une géométrie déclarée de zéro point. Le doublon
`mutants_enable` q4 a également disparu. Ces deux remarques sont closes.

Trois finitions évitent encore de sur-vendre ce bon patch. `lots` est incrémenté
avant le retour d'un scan sans seed, tandis que
ses compteurs de branche restent à zéro. Le générateur produit ne soumet
actuellement aucun lot vide, donc cela ne bloque pas G1 ; déplacer `m.lots = 1`
après le retour anticipé, ajouter `noop_lots`, ou définir `lots` comme « lots
transférés » rend simplement l'API et son commentaire cohérents. Le champ q3
`site_index_bytes` additionne indices **et** géométrie d'ancre, alors que le CLI
l'appelle `octets_sites_index` et exclut l'upload résident. Le renommer en
`wire_index_bytes`, ou séparer ces trois catégories, évite une métrique ambiguë ;
la porte peut alors exiger aussi que les octets de la branche choisie soient
strictement positifs et exacts. La réception reste device : sans nvcc local, ni
les deux nouvelles portes mutantes, ni la conservation de `PointId` adverse au
delà du bit 31 ne sont encore rejoués. Surtout, le faux pilote de campagne qui
ignore `--gpu-wire` rend toujours `violations=0` sans imprimer une seule ligne de
branche ; le validateur doit refuser ce cas avant de transformer les compteurs
en reçu.

Le témoin q4 manquant peut rester petit : affecter `id = UINT32_MAX - i` aux
entrées du gate index et tuer un mutant `wire-index-pid-is-dense` qui remplace
`ix.point_id(u)` par `u`. Pour fermer la comptabilité, exiger aussi q3
`lots == flushes`, `launches == lots` et q4 `lots == flushes`,
`lots <= launches <= 3 * lots`, avec H2D/D2H strictement positifs. Enfin, le
selftest campagne rend encore `violations=0` avec un faux GPU qui ignore
`--gpu-wire` et n'imprime aucune ligne de branche. Le validateur doit exiger,
pour chaque contrat `gpuidx`, exactement une ligne q3 et q4 avec
`index_lots == lots > 0`, zéro lot/octet SoA et des octets index positifs ; la
campagne device suivante exécute aussi les deux mutants de retombée SoA au code
4. Cela reçoit le raccord sans rejouer la matrice SCALE.

### Autres jalons

- `700a38c7` ferme l'ordre de publication `kPublished` et durcit le pilote
  SCALE.
- `f4b554fe` est un smoke test substantiel du rejeu
  `(catalogue, deltas) -> partition`, sans être encore une autorité sur la
  production du catalogue ou sur tout le `ForestResult`.
- La porte de préfixe renforcée à `fb7e9d40` ferme les trois anciens
  durcissements de libellé, tie excess et tampering ciblé.

Ces travaux n'empêchent ni G0 ni G1. Les détails de passage à l'échelle restent
dans [AUDIT_PASSAGE_ECHELLE_20260828.md](AUDIT_PASSAGE_ECHELLE_20260828.md).

### Grille V15 — noyau reçu, six coutures toujours ouvertes

La réception bornée de `d090f2cb` reste réutilisable et il n'est pas utile de
rouvrir l'algorithme. En revanche, aucun pin ultérieur n'a fermé les six raccords
de [`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`](QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md) :
le théorème canonique justifie encore le losange par le seul
`|P_k| >= rho`, appelle 4 799 488 comparaisons `(grille, cellule)` des « paires
(site, cellule) » et affirme qu'une contraction FMA ne peut qu'abaisser
l'erreur ; `cell_grid.hpp` écrit encore `|G*rhs|` alors que `rhs` porte déjà
`G`, prétend la capacité u32 garantie par le profil, et
`kCellLocateEvalOk` ne vérifie que `FLT_EVAL_METHOD == 0`. Corriger ces six
énoncés/gardes au même pin, puis rejouer les sept portes reçues, suffit pour
soumettre la fermeture ; aucune nouvelle campagne G4 ni nouvelle grille n'est
requise.

### Réducteur vivant `bc66ade7` — garder le cœur, fermer les coutures locales

Le pin conserve l'égalité nominale du résident et ajoute les raccords utiles :
`alias == live_exact[b]` avant et après les morts, `idx.used == aliases`,
balayages des listes, vacuité finale, maximum de déplacements par alias,
chaîne d'absorptions adverse et sorties mutantes rapides. Les CTests portent
désormais des timeouts explicites. Les fixtures D/E opposent réellement ordre
des racines logiques et ordre des clés. C'est une réception sensiblement plus
forte ; ne pas refaire l'architecture.

Quelques claims doivent encore rester étroits :

- `delta_equal` et `compare_order` couvrent déjà `level`, `batch_levels`, lots,
  compteurs et deltas complets : ne pas ajouter un second comparateur. Seule la
  petite table **littérale** de frontières omet encore `level`, les vecteurs
  `batch_levels` attendus et la comparaison de `liv.batches` à `c.batches`.
  A-300 et E-50 restent justement deux stresses différentiels ;
- le replayer local de `fold_live_gate.cpp` reconstruit une connectivité finale,
  mais ignore `output`, lots, niveaux et clés absentes du catalogue. Il n'a ni
  mutant ni rejet propre. Extraire le replayer strict de la porte T5 dans un
  utilitaire d'oracle commun, puis l'appliquer à `(catalogue résident,
  deltas vivants)` ;
- l'empreinte constante atteint l'internement des `FacetKey`. Le chemin actif à
  créneaux a supprimé `LiveIndex` : aucune fixture de décalage arrière n'est à
  ajouter tant que ce hash ne revient pas comme repli compilé. Le hash constant
  des lifetimes externes reste dû à L3 ;
- les cinq postes remplacent utilement `g_alloc_bytes`. `55c1f105` compte déjà
  `cfree`, `slot_mark`, scratch et deltas imbriqués. Un finaliseur unique après
  la dernière croissance doit seulement conserver `born_at`/`died_at` avant
  leur destruction et ajouter `batch_levels` ;

Les balayages structurels sont un oracle, pas un coût produit. Le stride
plancher peut en exécuter jusqu'à 127 malgré le plafond commenté à 64 : utiliser
un quotient plafond. Surtout, tester `x < nslots` **avant** `av[x]`, puis
`av[x].fid < slot_of_fid.size()` avant l'indexation, et balayer finalement tout
`slot_of_fid` pour détecter les entrées stale. Les backlinks, comptes et
free-lists sont déjà contrôlés ; les conserver sans construire un nouvel
oracle générique. Aligner aussi la formule publiée et celle exécutée : la marge
`ceil(log2(F + 2)) + 1` est sûre mais plus lâche que la borne de doublement
`floor(log2(F))` par alias.

Les deux fixtures de frontière FIRST/LAST sont maintenant présentes et gravent
leurs deltas. Compléter seulement leur objet littéral par `batches` et
`batch_levels`. Pour les mutants de coût et de capacité, le code 4 est
mathématiquement légitime seulement après égalité complète des sorties et
compteurs synthétiques ; une divergence sémantique doit garder priorité sur le
plafond, et le refus de capacité doit posséder son propre verdict causal.

Le mutant `free-on-absorb` ne recycle plus ses alias, mais recycle encore leur
record de composante. Son code 4 Release court est reçu ; l'absence de
diagnostic ASan/UBSan est rapportée par Claude, mais n'a pas été rejouée par cet
audit. Le qualifier d'injection diagnostiquement sale tant que ces alias
gardent un indice placé dans `cfree`.

Enfin, `reduce_fold_live` reste hors du chemin produit et aucun gain CPU/RSS
n'est reçu. Le nouveau régime `--dump/--from` fait maintenant lire le même flux
d'événements à deux processus et chacun exécute un seul réducteur : la couture
de mesure est donc correcte. Le diff ne copie plus le catalogue dans l'ablation
sans rejeu, mais le copie encore précisément dans le bras qui doit rendre
l'objet complet. Cette copie est inutile : `fp.keys` reste valide après
`reduce_fold_live(std::move(fp))`. L'utiliser directement, imposer le replayer
strict et publier la même signature d'objet dans les deux bras ferme le miroir
sans modifier le pipeline produit. Le pic `ru_maxrss` reste un pic de bout en
bout — entrée et préparation comprises — ce qui est précisément la mesure
utile.

Sur `uniform n=8000`, Claude documente à `55c1f105`, sans sortie brute ni reçu :
résident 4 097 ms et 1 166 Mo RSS, vivant sans rejeu 6 557 ms et 1 065 Mo, puis
vivant avec rejeu 2 700 ms de préparation, 3 718 ms de réduction, 3 872 ms de
rejeu et 1 330 Mo. Pour ce dernier bras, les composantes totalisent 10 290 ms,
pas les 10 569 ms transcrites dans sa réponse ; vérifier la sortie brute avant
de retenir un total. Le signal de décision reste utile : l'ablation L2 économise
8,6 % du pic de processus au prix d'une réduction 2,52 fois plus lente, tandis
que le seul bras qui restitue aujourd'hui la partition dépasse le résident. Ces
valeurs restent des mesures Claude non reçues ; elles motivent le miroir strict
et la suppression de la copie, pas un claim de gain mémoire du réducteur
complet.

Le protocole doit lancer deux invocations fraîches du binaire avec
`digest=true` et exiger `fold_inflight=1`, `reduce_calls == orders`, les mêmes
cardinalités, `digest_balls`, chaque `digest_forest[K]`, `digest_all`, niveaux
de lots et signature complète des événements. Le replayer commun refuse clé
inconnue, lot ou niveau incohérent, parent mort/non canonique, naissance
dupliquée ou ressuscitée et `output` différent du minimum. Pour la mémoire,
`ru_maxrss` n'attribue un delta au fold que si ce fold établit un nouveau pic ;
sinon le verdict mémoire est `inconclusif`, avec le RSS courant avant/après
comme témoin complémentaire, jamais comme substitut.

## Chemin minimal pour recevoir G0

### P0 initial — quatre corrections hôte fermées par `194a0bc2`

Les quatre points ci-dessous décrivent le défaut de `fe54ccca` et la couture
demandée ; `194a0bc2` les ferme dans leur principe. La relecture du pin révèle
ensuite un dernier P0 plus local, détaillé plus bas.

1. **Ticket.** Dans `ExecutorPool::run`, poser `done=true` et appeler
   `notify_all()` pendant que `Ticket::mu` est encore détenu, puis ne plus
   toucher le ticket. Au pin `fe54ccca`, un réveil spurieux pouvait laisser le producteur
   détruire son ticket de pile avant la notification : c'est une UB réelle.
2. **Démarrage.** Construire l'`Executor` sous capture d'exception et faire
   remonter son état au constructeur du pool. Le constructeur ne doit revenir
   que lorsque les N workers sont prêts. En cas d'échec d'un exécuteur ou d'un
   lancement de thread après un démarrage partiel : fermer, réveiller, joindre
   tous les threads déjà créés, puis relancer l'exception. Ces chemins pouvaient
   terminer le processus.
3. **Réentrance.** Refuser immédiatement, par marqueur `thread_local`, un
   `submit_and_wait()` appelé depuis un job du même pool. G0 n'a pas besoin de
   réentrance ; le blocage N=1 est reproduit, donc un rejet clair est préférable
   à un ordonnanceur plus complexe.
4. **Porte déterministe.** Remplacer le test de pic dépendant du scheduler par
   N jobs maintenus dans une latch avant libération. Sous `taskset -c 0`,
   l'ancienne porte répétée 100 fois ne sort à 0 que 22 fois ; 78 échecs portent
   sur `N=4 peak_active=1`. Tester ainsi N=1, 2, 4 et 8 et ajouter un `TIMEOUT`
   CTest.

Le domaine le plus cohérent avec la CLI est **tout entier de 1 à 8**. Il n'est
pas utile d'interdire 3 simplement parce que le sweep mesure 1, 2, 4 et 8.
En revanche, le constructeur ne doit pas clamper silencieusement 0 ou 9 : il
doit les refuser, et une fixture doit accepter 3.

### P1 — avant la prochaine réception CUDA

- Distinguer l'exception de job hôte récupérable de l'erreur device fatale.
  Pour le cas fatal, la solution minimale est un pool à usage unique qui ferme
  l'admission, mémorise la première erreur, complète ou annule les tickets en
  file, réveille toutes les attentes et draine les actifs sans réutiliser leurs
  exécuteurs. Invariant final : `submitted = succeeded + failed + cancelled`,
  `active = queued = 0`.
- Documenter que le propriétaire détruit le pool seulement après jonction de
  tous les producteurs, ce qui est déjà le contrat des lanes. G0 n'a pas besoin
  d'une API générale autorisant destruction et soumissions concurrentes : ce
  contrat est maintenant documenté et suffit.
- Le cap, son pic et une porte `queue_cap=1` existent déjà. Il reste seulement
  à rendre causal le cas « file pleine + admission retenue » et à en exiger la
  comptabilité exacte ; ne pas refaire l'API.
- Les includes directs sont présents et la compilation autonome est exercée.
  Le sweep scanner q3/q4 N=1/2/4/8 est une intégration/performance ultérieure,
  pas un verrou du contrat de sûreté G0.

Ce paquet reste volontairement petit. Une machine à états très générale, un
reorder buffer ou des lots asynchrones ne sont pas requis pour recevoir G0.

### Relecture constructive du pin G0 `194a0bc2`

Le pin ferme correctement l'essentiel : notification sans accès
au ticket après réveil, attente des workers prêts, refus de réentrance, domaine
continu `1..8`, latch causale et timeouts CTest. La porte ciblée rend `0/4/4`,
le nominal répété sous CPU 0 rend 30/30 et ASan/UBSan rend 3/3. Avec Clang 18,
le nominal et les deux mutants rendent aussi leurs codes attendus sous TSan
sans diagnostic ; le runtime TSan de GCC 13 échoue en revanche avant le test
sur `unexpected memory mapping`, donc ce second environnement ne prouve rien.
Un probe séparé où le deuxième des quatre constructeurs d'Executor lève rend
l'exception exacte, quatre tentatives, zéro vivant et trois destructions, en
Release comme sous ASan/UBSan. Le cœur du correctif est à garder.

Le pin `c19dc60d` ferme maintenant les deux défauts les plus dangereux de cette
relecture : le vecteur TLS est remplacé par un pointeur non allouant qui refuse
tout nesting, y compris entre pools, et `close_fatal` dépile la deque sans
tableau temporaire. Conserver ces corrections. Quatre finitions locales, et
seulement elles, restent au chemin critique G0 :

1. déplacer `submitted_++` **après** `queue_.push_back(&t)` réussi. Une
   allocation de la deque peut encore laisser un soumis qui n'est ni réussi,
   ni échoué, ni annulé ;
2. garantir un porteur fatal non nul même si `close_fatal(nullptr)` ne peut pas
   allouer son `runtime_error` de repli. Sinon les tickets annulés reçoivent un
   `exception_ptr` nul et peuvent sembler réussir ; employer un porteur
   préalloué/non allouant et graver ce cas ;
3. rendre contre-pression et fermeture fatale causales. Bloquer les workers,
   observer la file réellement pleine et au moins une admission retenue, puis
   fermer ; `sleep_for(100/200 ms)` ne doit jamais décider du scénario. Exiger
   les nombres exacts de succès, annulations et refus, ainsi que
   `peak_queued == cap` et un témoin d'admission retenue ;
4. typer l'exception CUDA et appeler `close_fatal` dans le worker ou son wrapper
   **avant** qu'il puisse dépiler un autre lot. Les exceptions hôte ordinaires
   restent récupérables et la première erreur device gagne.

Le mutant série prend aujourd'hui 15,6 s parce que la latch attend N arrivées
alors que le mutant n'a qu'un worker. Attendre `pool.executors()` arrivées puis
comparer le pic au N demandé conserve la preuve nominale et tue le mutant
immédiatement, sans réduire le timeout de sûreté.

Le chemin P1 `close_fatal` est donc une bonne primitive hôte sous le contrat de
durée de vie des producteurs. Sa garantie forte ne couvre toutefois aujourd'hui
que l'entrée non nulle : le repli de `close_fatal(nullptr)` peut échouer à
allouer, laisser `fatal_` nul et acquitter les tickets annulés sans exception.
Fournir un porteur non allouant, puis remplacer les attentes de 100/200 ms par
deux barrières : deux jobs actifs, puis file réellement pleine, avant la
fermeture. Enfin, tant que q3/q4 ne classent ni n'appellent ce chemin, le
qualifier de mécanisme hôte exercé, pas encore de gestion reçue d'une erreur
device réelle.

Le raccord device doit se faire **avant que le worker puisse dépiler un nouveau
lot**. Aujourd'hui `cuda_check` transforme toute erreur CUDA en
`std::runtime_error`, `broken_` ne couvre que les réservations et q3/q4
n'appellent jamais `close_fatal` : une capture dans le producteur, après
`submit_and_wait`, arriverait trop tard. Introduire une exception device typée
qui conserve le `cudaError_t`, sans classification par texte, puis faire la
transition fatale dans la capture du worker ou dans un wrapper de job qui
ferme et relance avant son retour. Les `invalid_argument`, erreurs de fixture et
exceptions hôte ordinaires restent récupérables ; la première erreur device
gagne et tous les workers actifs sortent après leur lot courant.

La machine d'état minimale tient sous `mu_` : `Open`, puis
`Queued -> Running -> Succeeded|Failed`, ou une transition unique
`Open -> Fatal(first_error)` qui rejette les admissions et transforme chaque
`Queued` en `Cancelled(first_error)`. Le déplacement actuel de la deque vers
`active_` et celui d'`active_` vers le compteur terminal sont séparés du mutex ;
un snapshot vivant peut donc voir un job en limbe. Cela ne bloque pas G0 si
`counters()` est documenté et exercé **après quiescence seulement**. Ne rendre
les transitions atomiques sous `mu_` que si une future barrière consomme
réellement ces compteurs pendant l'activité.

La porte fatale peut alors être exacte et rapide : maintenir exactement deux
jobs actifs, attendre quatre tickets en file et deux admissions bloquées,
fermer, puis libérer les actifs. Elle exige
`submitted=6, succeeded=2, failed=0, cancelled=4, active=queued=0`, zéro corps
annulé exécuté, deux résultats normaux et six producteurs voyant la première
erreur (quatre annulations, deux admissions refusées). Un second `close_fatal`
avec un autre marqueur et une soumission tardive prouvent `first-error-wins`.
Les délais ne servent que de coupe-circuit ; aucun `sleep_for` ne décide du
scénario.

Le run direct du pin illustre la dent manquante : le nominal a observé par
chance `6` erreurs et `2` succès, tandis que `pool-drop-exception` a observé
`2` erreurs et `6` succès ; dans les deux cas, toute la sous-porte fatale a
imprimé `ok`. Le mutant ne rend finalement 4 que grâce à l'exception ordinaire
ultérieure. La fermeture et sa propagation ne sont donc pas encore un témoin
causal du mutant.

Le compteur de file peut rester un détail P2, mais son type doit suivre celui
du cap : aujourd'hui `queue_cap` est un `size_t` alors que `peak_queued` tronque
en `u32`. Avec le cap produit par défaut, inférieur à 16, cela ne bloque pas G0 ;
borner l'API ou publier la métrique en `size_t` évite un futur wrap silencieux.
Le libellé « déterministe » doit enfin devenir « latch causale à échéance » :
les cinq secondes rendent la porte bornée, pas indépendante d'une famine totale
de l'ordonnanceur. Épingler la commande et la version Clang du rejeu TSan avant
de transformer l'observation locale en reçu.

## Signaux de performance, sans claim

Les campagnes ne disent pas « aucun GPU » : q3 reste derrière le CPU sur les
quatre familles, tandis que q4 gagne nettement sur `terrain` et `scanline`,
légèrement sur les clusters et perd sur `uniform`. Après G1, tester un routage
par intensité `travail / octets` est donc plus utile qu'un backend GPU global.

Le pilote CPU montre aussi une forte réponse à `--threads` jusqu'à 32, puis une
traîne dominée par `reduce` et le digest. Cela justifie le réducteur vivant et
un futur protocole cpuset physique/SMT ; cela ne justifie pas plus de
producteurs aujourd'hui.

Ces chiffres restent à un passage, non contrebalancés. Le pilote SCALE n'impose
pas de cpuset ; les sommes H2D, `issue`, réservations et vies d'exécuteurs se
recouvrent ; `cycle_de_vie_ms_sum` inclut travail et attente. Les ratios sont des
signaux de conception, jamais un speedup reçu ni le « protocole A ». Les
valeurs, calculs et corrections détaillés restent dans
[AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md](AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md).

## Ordre recommandé, sans détour

1. Conserver le contrat G1 hôte de `c19dc60d` : bornes d'indices, géométrie
   absente/vide, compteurs/mutants de branche et parseur `--inject`. Le libellé
   des no-op peut suivre sans nouvelle cible. `PointId` q4 adverse, selftest et
   vraie exécution des mutants ferment ensuite la réception device.
2. Suivi fold séparé : garder le tableau de créneaux et le refus avant `kNil` ;
   compléter le balayage de tout `slot_of_fid`, la causalité propre de
   `slot-cap-minus-one`, les niveaux/lots littéraux, l'échantillonnage des cinq
   catégories mémoire et le miroir strict.
3. Suivi G0 de `c19dc60d`, sans refonte : garder le TLS non allouant et la fermeture
   en place, déplacer la comptabilité après admission réussie et rendre les
   fixtures causales ; puis brancher le poison typé dans q3/q4 avant tout claim
   de confinement d'une erreur CUDA.
4. Pour L2/L3 : garder `bc66ade7`, partager le replayer strict, compléter les
   fixtures/listes et attribuer chaque métrique à son témoin ; puis ouvrir L3
   sans autre refonte du réducteur.
5. Après les pins hôte, une réception CUDA **minimale** de q4 index et des
   nouvelles dents suffit ; ne pas rejouer la matrice 50 k déjà acquise.
6. Mesurer ensuite la double représentation, le setup résident et le contexte
   partagé ; n'optimiser que le poste visible.
7. G2 seulement si cette ablation montre que les retours intermédiaires q4
   dominent encore ; sinon attaquer G3, qui retire aussi le wire par seed.
8. En parallèle : préparer le pilote CPU cpuset
   physique/SMT à trois répétitions ; renforcer T5 avant le fold streamé.

## Validation indépendante

- Pin `194a0bc2` : construction ciblée Release et trois CTests pool verts ;
  nominal 0,35 s, mutants série 15,35 s et exception 0,35 s. Le même source
  nominal a rendu 30/30 sous CPU 0, et 3/3 sous ASan/UBSan. Avec Clang 18, le
  nominal et les deux mutants rendent leurs codes attendus sous TSan sans
  diagnostic. Ce rejeu TSan reste une observation locale non reçue ; le
  runtime GCC 13 TSan échoue avant le test sur un mapping inattendu.
- Deux observations locales non reçues réfutent les derniers cas non couverts : allocation
  du vecteur TLS forcée en échec → handler 77 ; cycle `A(1) -> B(1) -> A` →
  timeout. Le probe de constructeur d'Executor fautif confirme en revanche la
  jonction transactionnelle : exception exacte, zéro objet vivant, en Release
  et ASan/UBSan.
- Pin `c19dc60d`, G1 hôte : construction ciblée Release réussie ;
  `mhgp5_batch_contract`, registre de mutants, instrumentation hôte et lanes
  batched q3/q4 rendent **5/5**. CUDA est désactivé localement ; aucune porte
  device de ce delta n'est annoncée comme exécutée.
- Pin `c19dc60d`, fold à créneaux : **7/7** CTests fold nominal + six mutants verts au
  dernier passage, en 54,15 s. `slot-cap-minus-one` rend 4, publie le refus exact
  et reste propre sous ASan/UBSan, mais ses cas synthétiques ajoutent encore des
  désaccords génériques suffisants au code 4. Ces exécutions reçoivent la sûreté
  du chemin exercé et l'égalité différentielle, pas encore la causalité du
  mutant, le mapping complet, les pics des cinq postes ni le miroir strict.
- Rejeu indépendant de ce checkpoint : construction ciblée de
  `mhgp5_batch_contract_gate`, `mhgp5_executor_pool_gate`,
  `mhgp5_fold_live_gate` et `mhgp5_fold_live_probe` réussie ; cinq CTests ciblés
  (batch, pool nominal/deux mutants, capacité fold) rendent **5/5** en 16,11 s.
  Un smoke `--dump/--from` sur `uniform n=200` rend les mêmes 7 459 facettes et
  2 385 deltas dans les deux bras, sans encore comparer leur objet complet.
- Contre-rejeu du pin `c19dc60d` : batch hôte, pool nominal/deux mutants, fold
  nominal et ses six mutants rendent **11/11** en Release (51,32 s mur avec
  deux jobs CTest). Les mêmes sources, avant leur pin, rendaient pool + six
  mutants fold **9/9** sous ASan/UBSan (16,46 s). Cela couvre les chemins
  exercés, pas les corruptions de partition encore sans mutant ni le nominal
  fold massif sous sanitizer.
- Sources ensuite épinglées par `55c1f105` : le nominal fold passe directement en
  20,62 s sur 58 ordres, 5 194 737 facettes et 733 029 deltas, avec zéro
  désaccord et zéro violation de vie, structure, partition ou vacuité. Un second
  CTest sous charge concurrente passe en 87,55 s ; les six mutants passent
  **6/6** en 0,17 s après reconstruction ciblée. Ces verts confirment la
  correction fonctionnelle exercée du déplacement de mesure ; la dispersion du
  nominal interdit d'en déduire un speedup.
- Build Release CPU complet au pin `556c421e` : succès ; construction ciblée
  des portes fold au pin `bc66ade7` : succès.
- CTests API, pool, T5, préfixe et lanes q3/q4 batched : 41/41 verts en
  158,04 s au pin `556c421e`.
- Répétition de l'ancienne porte du pool `fe54ccca` sous un seul CPU : 22 succès
  et 78 échecs sur 100, défaut de déterminisme historique reproduit.
- La campagne versionnée est complète selon son validateur épinglé ; hashes,
  codes et digests ci-dessus ont été rejoués indépendamment.
- Le selftest campagne courant affiche `violations=0`, alors que son faux GPU
  ignore `--gpu-wire` et omet wire, étapes et octets : ce vert reproduit la
  permissivité à corriger, il ne reçoit pas G1.
- Pas de nvcc ni de device CUDA local. La nouvelle session compile/exécute G0
  et G1 q3 au pin `839cf1ec`; G1 q4 de `556c421e` reste sans réception device.
- Fold `bc66ade7` : **7/7** CTests fold/fixtures verts. Le nominal rend 0 sur
  58 ordres, 5 194 737 facettes et 733 029 deltas, sans désaccord, violation de
  vie, structure ni vacuité ; il dure 72–83 s selon la charge locale. Les cinq
  mutants rendent 4 en 0,03–0,07 s et la fixture rend 0 en 0,04 s. La porte de
  sûreté `fold_inflight=3` mesure indépendamment un pic exact de 3, ce qui
  réfute tout plafond structurel à 2. Les claims CPU/RSS restent non reçus.

GCP non utilisé par cet audit. Les deux sessions appartenaient à Claude et leurs
journaux locaux montrent l'arrêt ciblé `TERMINATED`. La nouvelle campagne lie
cette observation à un reçu expurgé versionné et au hash du journal ; la session
13 ne possède encore que sa preuve locale non versionnée.
