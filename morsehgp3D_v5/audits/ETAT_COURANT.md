# État courant audité de MorseHGP3D v5 — 28 août 2026

- **Dernier pin fonctionnel inspecté :** `556c421e` ; les constats G0 portent
  sur son introduction à `fe54ccca`, la campagne device/SCALE versionnée sur
  `c95cfa95`, G1 q3 sur `dd928111`/`839cf1ec` et G1 q4 sur `556c421e`.
- **Worktree produit observé :** postérieur à `556c421e`, sale sur le nouveau
  réducteur vivant (`fold_live.hpp`, sa porte, CMake et mutants). Ce chantier
  est préservé et audité provisoirement, mais ne fait pas partie du verdict du
  HEAD. Le reçu G0/G1 q3 corrigé reste local et non versionné au HEAD ; il est
  prêt à être versionné séparément par son propriétaire. Le probe
  `.codex_fold_contract_probe.cpp` d'un autre audit reste lui aussi non intégré.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Verdict utile à Claude

**Orange constructif : la direction est bonne.** La session 13 fournit une
baseline CUDA cohérente. La nouvelle session au pin `839cf1ec` compile et
exécute nominalement G0 et G1 q3 : huit exécuteurs au total au lieu de 98,
digests appariés et baisse H2D q3 visible. Les pins G1 q4 complètent ensuite le
wire sans remplacer SoA. Il ne faut jeter aucun de ces travaux ni redessiner la
lane. Quatre coutures hôte G0 et les contrats fail-closed/non-vacants G1
ci-dessous restent toutefois à fermer.

Le prochain pin facile à recevoir est un **G0 sûr**, puis un petit pin hôte G1
qui borne les indices et rend chaque branche index falsifiable. La session qui
vient d'être exécutée est une bonne mesure exploratoire et prouve que le chemin
q3 index a effectivement réduit ses copies ; elle ne remplace pas ces portes et
ne couvre pas q4, absent de son pin source. Aucun nouveau G4 n'est nécessaire
avant leur fermeture locale. T5, le fold massif et le protocole CPU sous cpuset
restent indépendants et ne doivent pas détourner ce travail court.

## Ce qui est reçu et réutilisable

### Campagne 13 au pin `c95cfa95`

Le reçu versionné contient 121 fichiers : 39 triplets
`.status`/`.txt`/`.status.time`, `RECU.txt` et trois auxiliaires. Les 38 runs
nominaux sortent à 0, le mutant device à 4, les statuts sont complets et les
hashes du payload et du protocole se reconstruisent. Les six comparaisons
CPU/GPU ou CPU/GPU-adaptatif disponibles ont les mêmes digests détaillés. Cela
reçoit une **preuve fonctionnelle bornée** sur les quatre familles, la seed, le
matériel et le pin annoncés.

Le journal local non versionné montre aussi que la cible exacte
`devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a` était
`SPOT`, munie des deux coupe-circuits puis certifiée `TERMINATED`. Cette lecture
est utile pour la sécurité de la session, mais le commit ne contient pas la
preuve d'arrêt elle-même.

Le validateur courant a depuis ajouté les quatre contrats `gpuidx`. Il rejette
donc ce reçu historique, qui ne les contient pas. Cela n'annule pas son rejeu au
validateur épinglé, mais impose désormais un `campaign_schema` explicite et un
dispatch par version : la liste mouvante du HEAD ne doit pas redéfinir les
obligations d'un reçu déjà figé.

### Campagne G0/G1 q3 locale au pin `839cf1ec`

Le dossier local `receipts/campagne_g4_v5_20260828_g0_g1/` contient 29
triplets : 28 codes 0 et le mutant device à 4. Hashes et statuts se
reconstruisent, le validateur **du pin source** rend
`campaign_status=complete`, et les quatre bras CPU/GPU/GPU-index ont les mêmes
digests détaillés. Le validateur du HEAD attend désormais q4 index et refuse ce
schéma plus ancien ; un `campaign_schema` versionné doit sélectionner les
obligations du pin au lieu de les faire dériver avec HEAD.

Deux preuves nominales sont acquises. G0 construit huit exécuteurs au total,
deux pools de quatre avec `flux_pic=4`, contre 98 créations dans la baseline.
G1 q3 exerce des lots index non vides et réduit les octets H2D de 43 à 65 % sur
les quatre familles, avec mêmes sorties. Cela prouve le câblage emprunté sur ces
runs, mais ni la sûreté générale de G0 ni G1 q4, absent du pin source.

Les temps restent exploratoires : un passage SoA puis index, sans
contrebalancement. La note locale corrigée indique que le temps H2D q3 baisse
de 16 à 73 % ; sur `uniform`, q3+q4 vaut 6,015 s et 10,3 % du mur, les 6,251 s
incluant q2 ; les parts « 9–11 % » et « fold 43 % » ne valent que pour
`uniform`. Les bras `gpuidx` sont q3-index/q4-SoA. Sur ce passage q3-index
reste plus lent que q3 CPU sur les quatre familles ; q4 device gagne sur
`terrain`, `scanline` et légèrement les clusters, mais perd sur `uniform`. Le
setup synchrone de `GpuGeometry` n'entre ni dans `h2d_ms` ni dans
`lane_wall_ms`, bien que ses octets alimentent `h2d_octets`.

Le verdict du validateur n'est pas une preuve suffisante de ces faits. Sur une
copie, il reste `complete` après suppression des 29 `.status.time`, retrait des
métriques G0/G1, remplacement des commandes `--gpu-wire=index` par `soa` et
mise à zéro des replis q3 index. Le prochain schéma doit exiger les trois
fichiers par run, la commande et le `timing_scope` exacts, les témoins de pool,
wire/lots/octets et les planchers de replis. Les mesures ci-dessus viennent de
la relecture directe des artefacts, pas du seul verdict vert.

Le `SAFETY_RECEIPT.txt` local transcrit les codes remote/scp à 0, les deux
coupe-circuits, le modèle `SPOT` et l'arrêt exact `TERMINATED`, et le lie au
SHA-256 du journal brut local. `SHA256SUMS` couvre chaque artefact du dossier. Le
journal, qui contient des métadonnées de session, et toute clé restent exclus.
Le propriétaire doit encore révoquer sa clé de session puis nettoyer uniquement
son répertoire temporaire ; cet audit n'effectue aucune mutation GCP.

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

Les deux premiers points ferment le contrat fonctionnel G1 ; le troisième
répare une porte CUDA déjà déclarée. Les trois derniers sont des durcissements
de coût à mesurer ensuite, pas des conditions artificielles de réception :

1. **Réception — indices.** Le validateur de lot ne borne que la **taille** de `site_index`, pas ses
   valeurs. Avant tout lancement, exiger chaque index strictement inférieur à
   `GpuGeometry::count` et la présence conjointe index/géométrie d'ancre ; les
   fixtures `index == count` et `UINT32_MAX` doivent lever sans lancement. Le
   builder normal produit bien des indices valides : il s'agit d'un contrat
   fail-closed, pas d'une divergence des sorties actuelles ;
2. **Réception — branche.** La porte imprime le wire demandé, sans prouver le wire exécuté. Un mutant
   remplaçant le branchement index par SoA garderait verdicts et digests verts,
   et le faux GPU du selftest ignore aujourd'hui `--gpu-wire` tout en étant
   accepté. Ajouter `index_lots`, `soa_lots` et les octets catégoriels, puis
   exiger `index_lots == launches > 0`, `soa_lots == 0` et
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

L'ordre interne est donc : sûreté d'index, preuve de branche et CTest mutant,
tous testables localement ; puis seulement lots mono-wire, contexte partagé et
métriques de setup selon leur effet mesuré. Ces derniers ne bloquent ni les
digests déjà obtenus ni la réception fonctionnelle bornée.

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

### Réducteur vivant local — garder le cœur, réparer la porte

Le prototype courant prend la bonne direction : il sépare racine logique et
conteneur physique, copie le gel pré-lot avant les unions et libère après
émission. Le différentiel nominal trouve zéro désaccord sur 58 ordres,
5 194 737 facettes et 733 029 deltas. Après correction de la fausse borne, la
porte nominale sort 0 avec 5 660 568 relocalisations, zéro invariant et un pic
d'alias de 7,29 % sur 15 grands ordres. Il ne faut donc ni jeter ce code ni
revenir au réducteur résident.

La porte n'est cependant pas recevable dans son état local :

- le commentaire annonce encore `--reloc-ratio`, option qui n'existe pas. Le
  nominal emploie maintenant la vraie borne, mais le mutant physique doit être
  tué par une fixture quadratique explicite et un maximum par alias ;
- `free-on-absorb` bouclait plusieurs minutes dans sa première forme. La forme
  réécrite termine maintenant par un core dump au lieu du code 4 : elle reste
  non mémoire-sûre et aucun `TIMEOUT` CTest ne borne ce cas ;
- T6 compare chaque frontière au pic global `peak_live_exact`, ce qui est trop
  faible. Le compteur exact du lot permet de tester directement
  `live_aliases == live_exact[b]` ;
- la porte compare les deltas mais ne les rejoue pas. Le contrat T5 recevable
  reste `(catalogue externe, deltas) -> partition`, car les deltas seuls ne
  reconstruisent pas les singletons implicites ni `facet_keys` ;
- `live_bytes_peak` additionne l'état logiquement vivant, pas la mémoire
  allouée : capacités d'arènes, free-lists, scratchs, `firstb`/`lastb`, `keys`
  et `ev_fid` sont hors compte.

Le petit correctif utile est précis : remplacer `free-on-absorb` par un mutant
terminant sans crash, poser un `TIMEOUT`, compter les relocalisations **par alias** et
exiger `max_moves <= ceil(log2(nfid))`, puis tuer la racine physique sur une
chaîne d'absorptions adverse. Ajouter ensuite l'égalité vivante par lot et le
rejeu avec catalogue. Renommer enfin la métrique `logical_live_bytes`, ou lui
adjoindre les capacités et le RSS. Ces cinq coutures reçoivent le prototype sans
lui demander une autre architecture. Ne pas relancer toute la matrice de
mutants avant d'avoir rendu `free-on-absorb` terminant.

## Chemin minimal pour recevoir G0

### P0 — quatre corrections hôte ciblées

1. **Ticket.** Dans `ExecutorPool::run`, poser `done=true` et appeler
   `notify_all()` pendant que `Ticket::mu` est encore détenu, puis ne plus
   toucher le ticket. Aujourd'hui un réveil spurieux peut laisser le producteur
   détruire son ticket de pile avant la notification : c'est une UB réelle.
2. **Démarrage.** Construire l'`Executor` sous capture d'exception et faire
   remonter son état au constructeur du pool. Le constructeur ne doit revenir
   que lorsque les N workers sont prêts. En cas d'échec d'un exécuteur ou d'un
   lancement de thread après un démarrage partiel : fermer, réveiller, joindre
   tous les threads déjà créés, puis relancer l'exception. Les chemins actuels
   peuvent terminer le processus.
3. **Réentrance.** Refuser immédiatement, par marqueur `thread_local`, un
   `submit_and_wait()` appelé depuis un job du même pool. G0 n'a pas besoin de
   réentrance ; le blocage N=1 est reproduit, donc un rejet clair est préférable
   à un ordonnanceur plus complexe.
4. **Porte déterministe.** Remplacer le test de pic dépendant du scheduler par
   N jobs maintenus dans une latch avant libération. Sous `taskset -c 0`, la
   porte actuelle répétée 100 fois ne sort à 0 que 22 fois ; 78 échecs portent
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
  d'une API générale autorisant destruction et soumissions concurrentes ; si
  une fermeture explicite est ajoutée, elle doit réveiller `cv_` et
  `cv_space_`.
- Exposer le cap et son pic, puis exercer `queue_cap=1` avec une latch. La valeur
  0 peut garder le sens documenté « auto = 2N ».
- Ajouter les includes directs (`<cstddef>`, `<stdexcept>`, `<utility>` dans le
  pool, `<algorithm>` dans la porte).
- Comparer q3/q4 avec un scanner factice pour N=1/2/4/8 : sorties, ordre et
  compteurs identiques au chemin direct. Le flush final séquentiel des reliquats
  n'invalide pas la sûreté de G0, mais doit être parallélisé ou mesuré avant de
  revendiquer que le pool est pleinement occupé.

Ce paquet reste volontairement petit. Une machine à états très générale, un
reorder buffer ou des lots asynchrones ne sont pas requis pour recevoir G0.

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

1. Pin hôte G0 sûr : quatre P0, portes déterministes et correctifs P1 utiles au
   prochain device.
2. Pin hôte G1 fonctionnel : bornes d'indices, compteurs/mutants de branche,
   `PointId` q4 adverse et réparation du CTest `--inject`.
3. Si le fold vivant courant est poursuivi : remplacer son mutant qui plante,
   borner les CTests, puis recevoir l'égalité vivante par lot et le rejeu T5.
4. Après les deux pins hôte, une réception CUDA **minimale** de q4 index et des
   nouvelles dents suffit ; ne pas rejouer la matrice 50 k déjà acquise.
5. Mesurer ensuite la double représentation, le setup résident et le contexte
   partagé ; n'optimiser que le poste visible.
7. G2 seulement si cette ablation montre que les retours intermédiaires q4
   dominent encore ; sinon attaquer G3, qui retire aussi le wire par seed.
8. En parallèle : préparer le pilote CPU cpuset
   physique/SMT à trois répétitions ; renforcer T5 avant le fold streamé.

## Validation indépendante

- Build Release CPU au HEAD `556c421e` : succès.
- CTests API, pool, T5, préfixe et lanes q3/q4 batched : 41/41 verts en
  158,04 s sur le HEAD.
- Répétition de la porte nominale sous un seul CPU : 22 succès et 78 échecs sur
  100, défaut de déterminisme reproduit.
- La campagne versionnée est complète selon son validateur épinglé ; hashes,
  codes et digests ci-dessus ont été rejoués indépendamment.
- Le selftest campagne courant affiche `violations=0`, alors que son faux GPU
  ignore `--gpu-wire` et omet wire, étapes et octets : ce vert reproduit la
  permissivité à corriger, il ne reçoit pas G1.
- Pas de nvcc ni de device CUDA local. La nouvelle session compile/exécute G0
  et G1 q3 au pin `839cf1ec`; G1 q4 de `556c421e` reste sans réception device.
- Prototype fold local : nominal code 0 sur 58 ordres, 5 194 737 facettes,
  zéro désaccord/invariant et pic d'alias 7,29 % sur les grands ordres. Le
  mutant `free-on-absorb` termine par core dump et les portes n'ont pas de
  `TIMEOUT` ; suite mutante et claim de performance non reçus.

GCP non utilisé par cet audit. Les deux sessions appartenaient à Claude et leurs
journaux locaux montrent l'arrêt ciblé `TERMINATED`. Le dossier local de la
nouvelle campagne lie cette observation à un reçu expurgé et au hash du journal ;
la session 13 ne possède encore que sa preuve locale non versionnée.
