# État courant audité de MorseHGP3D v5 — 28 août 2026

- **Dernier pin produit inspecté :** `194a0bc2`, qui consolide le pool G0 et
  ajoute la sonde resident/vivant. Il succède au réducteur vivant L2 durci de
  `bc66ade7`. Les constats historiques G0 portent sur `fe54ccca`, la campagne
  device/SCALE sur `c95cfa95`, G1 q3 sur `dd928111`/`839cf1ec` et G1 q4 sur
  `556c421e`.
- **Pin de réception G0/G1 q3 :** `0656bf4c`, sans code produit.
- **Worktree observé :** Claude durcit G1 après `194a0bc2` : valeurs des indices,
  compteurs de branche et d'octets, mutant de retombée SoA, CLI et portes q3/q4.
  Il expérimente aussi un cache `slot_alias` dans le fold vivant. Ces deltas non
  épinglés restent exclus du verdict versionné ; la porte hôte de contrat G1 est
  toutefois verte. Le probe `.codex_fold_contract_probe.cpp` d'un autre audit
  reste lui aussi non intégré.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Verdict utile à Claude

**Orange constructif : `194a0bc2` garde la bonne architecture.** Notification
du ticket, démarrage transactionnel, domaine `1..8` et latch causale sont de
vraies corrections ; il ne faut ni les jeter ni redessiner la lane. La réception
complète demande encore un petit durcissement local : marqueur TLS non allouant
et refus de tout nesting, fermeture fatale à garantie forte, puis fixtures
causales. `close_fatal` est aujourd'hui une primitive hôte exercée, pas encore
le poison CUDA q3/q4. La sonde fold du même pin mesure un second réducteur depuis
le callback d'un pipeline qui a déjà exécuté le résident : elle reste un
micro-banc incrémental, pas le miroir CPU/RSS annoncé.

La session 13 fournit par ailleurs une baseline CUDA cohérente. La session au
pin `839cf1ec` compile et exécute nominalement G0 et G1 q3 : huit exécuteurs au
total au lieu de 98, digests appariés et baisse H2D q3 visible. Les pins G1 q4
complètent ensuite le wire sans remplacer SoA. Les contrats
fail-closed/non-vacants G1 restent à fermer sans reprendre ces travaux.

Le worktree G1 actif répond déjà correctement aux deux demandes les plus
rentables : bornage **en valeur** avant lancement et témoin de la branche
réellement exécutée. Conserver ce patch. Il reste à aligner le libellé des lots
vides, supprimer une activation mutante q4 redondante, puis recevoir les deux
mutants sur un vrai device ; cela ne justifie aucune refonte du wire.

Le pin `bc66ade7` consolide le bon cœur L2 : égalité de vie par lot,
balayages structurels, vacuité finale, plafond par alias, cas adverse court et
timeouts explicites. Il ne faut revenir sur aucune de ces corrections. Les
finitions avant L3 sont locales : replayer T5 strict partagé, comparateur de
fixtures complet, micro-fixture du backshift, deux invariants de listes et
attribution exacte des témoins mémoire. Elles bornent le claim, pas l'utilité
du réducteur.

Le fold actif va plus loin : il remplace le hash vivant par un tableau de
créneaux de taille `peak_live_exact`, une table directe `fid -> créneau`, puis
réutilise l'alias résolu dans `slot_alias`. La construction par intervalles est
cohérente avec l'ordre naissance-avant-mort du lot ; les indices d'arène restent
valides après les relocalisations small-to-large. La porte différentielle
complète rend déjà 6/6 et le nominal local tombe à 19,8 s, contre 72–83 s lors
de la relecture de `bc66ade7`. C'est un signal très favorable, pas encore un
benchmark apparié : garder cette piste.

Deux coutures préservent la valeur de l'expérience. Au L2 courant,
`slot_of_fid` occupe `4 * facettes` octets et `slot_alias` jusqu'à
`11 * sizeof(u32) = 44` octets par événement du plus gros lot ; tous deux sont
omis de `allocated_bytes`. Ils ne disparaîtront qu'au futur wire L3 : les
inclure aujourd'hui, ou renommer la métrique et publier séparément état
persistant, mapping et scratch. Les sidecars `firstb`, `lastb` et
`slot_of_fid` sont bien en O(F), mais le chemin L2 complet ne l'est pas en F
seul : `FoldPrepared` garde `events`/`ev_fid`, donc O(I) occurrences, et
`slot_alias` ajoute O(I_max_lot) de scratch. Annoncer O(F + I) entrée comprise,
ou publier séparément préparation, sidecars O(F), état vivant O(pic) et scratch
du plus gros lot. Le futur flux pourra seul retirer ces tableaux du réducteur.
`assign(..., kNil)` écrit aussi les onze slots à chaque lot alors que seuls les
slots consultés sont relus ; un `resize` réutilisant la capacité suffit. Enfin,
un débordement de créneau ne doit pas retourner `kNil` vers `unite`, qui
déréférencerait cet indice : lever immédiatement une contradiction interne.
Comparer séparément « hash -> table directe » puis « second lookup -> cache »
permettra d'attribuer le gain. Le ratio 1,9–3,5 issu de la sonde callback reste
un signal de micro-banc, pas la mesure miroir que son commentaire annonce.

La vérification `slots == peak_live_exact` est aujourd'hui tautologique : le
premier champ est affecté depuis le second. `peak_aliases == peak_live_exact`
reste une bonne cohérence de sortie, mais découle déjà de l'égalité de vie par
lot ; la dent indépendante manquante est la bijection des créneaux. Prolonger
le balayage structurel borné pour marquer les créneaux vivants puis libres et
refuser tout hors-borne, doublon ou créneau manquant. Un mutant
`slot-cap-minus-one` doit produire le code 4 par contradiction interne capturée
avant tout accès sentinelle. Deux fixtures littérales isolent la convention de
frontière : deux événements K=1 disjoints au même niveau donnent quatre
facettes `FIRST == LAST` et `peak=slots=4` ; les mêmes événements sur deux
niveaux successifs donnent quatre facettes totales mais `peak=slots=2`. Ce
second cas vérifie la réutilisation entre lots ; un mutant de libération avant
émission est nécessaire si l'on veut certifier précisément la phase. Enfin,
qualifier le créneau comme champ du **wire L3 proposé** :
`FacetOccurrenceWire` n'est pas encore une structure enregistrée, et
`free_slots` reste bien une liste libre.

Le prochain pin facile à recevoir est un **durcissement ciblé de `194a0bc2`**,
puis un petit pin hôte G1 qui borne les indices et rend chaque branche index
falsifiable. La session déjà exécutée est une bonne mesure exploratoire et
prouve que le chemin q3 index a effectivement réduit ses copies ; elle ne
remplace pas ces portes et ne couvre pas q4, absent de son pin source. Aucun
nouveau G4 n'est nécessaire avant leur fermeture locale. T5, le fold massif et
le protocole CPU sous cpuset restent indépendants et ne doivent pas détourner ce
travail court.

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

### Campagne G0/G1 q3 versionnée à `0656bf4c`, source `839cf1ec`

Le [reçu](../receipts/campagne_g4_v5_20260828_g0_g1/RECU.txt) contient 29
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
contrebalancement. La note corrigée indique que le temps H2D q3 baisse
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

Le [reçu de sécurité](../receipts/campagne_g4_v5_20260828_g0_g1/SAFETY_RECEIPT.txt)
transcrit les codes remote/scp à 0, les deux coupe-circuits, le modèle `SPOT` et
l'arrêt exact `TERMINATED`, et le lie au
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

### Relecture constructive du worktree G1 actif

Le patch ferme dans son principe les points 1 à 3 : `GpuGeometry::count` borne
chaque valeur, les fixtures `index == count` et `UINT32_MAX` refusent, les
compteurs distinguent index et SoA, le mutant `wire-index-force-soa` possède une
signature dédiée, et q3 parse enfin `--inject`. La porte CPU
`mhgp5_batch_contract` rend 0. L'agrégation des statistiques de lot reste sous
le mutex de lane ; aucune nouvelle race hôte n'est visible.

Trois finitions évitent de sur-vendre ce bon patch. La valeur sentinelle
`n_geom_points == 0` signifie à la fois « géométrie absente » et « géométrie
déclarée vide » : dans le second cas, le contrôle des indices est sauté alors
que `use_idx` peut devenir vrai. Un booléen de présence ou un `optional<size_t>`
permet de refuser explicitement un lot non vide contre une géométrie de taille
zéro ; graver ce rejet en q3 et q4.

Ensuite, `lots` est incrémenté avant le retour d'un scan sans seed, tandis que
ses compteurs de branche restent à zéro. Le générateur produit ne soumet
actuellement aucun lot vide, donc cela ne bloque pas G1 ; déplacer le comptage
de branche avant ce retour, ajouter `noop_lots`, ou définir `lots` comme « lots
transférés » rend simplement l'API et son commentaire cohérents. Le champ q3
`site_index_bytes` additionne indices **et** géométrie d'ancre, alors que le CLI
l'appelle `octets_sites_index` et exclut l'upload résident. Le renommer en
`wire_index_bytes`, ou séparer ces trois catégories, évite une métrique ambiguë ;
la porte peut alors exiger aussi que les octets de la branche choisie soient
strictement positifs et exacts. Enfin, la porte q4 appelle
`mutants_enable(inject)` deux fois : retirer le doublon. La réception reste
device : sans nvcc local, ni les deux nouvelles portes mutantes, ni la conservation de
`PointId` adverse au-delà du bit 31 ne sont encore rejoués. Le faux pilote de
campagne qui ignore `--gpu-wire` doit toujours être refusé par le selftest avant
de transformer ces compteurs en reçu.

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

### Réducteur vivant `bc66ade7` — garder le cœur, fermer les coutures locales

Le pin conserve l'égalité nominale du résident et ajoute les raccords utiles :
`alias == live_exact[b]` avant et après les morts, `idx.used == aliases`,
balayages des listes, vacuité finale, maximum de déplacements par alias,
chaîne d'absorptions adverse et sorties mutantes rapides. Les CTests portent
désormais des timeouts explicites. Les fixtures D/E opposent réellement ordre
des racines logiques et ordre des clés. C'est une réception sensiblement plus
forte ; ne pas refaire l'architecture.

Quelques claims doivent encore rester étroits :

- les fixtures D/E distinguent maintenant racines logiques et clés, mais leur
  rendu omet `level`, `batch_levels`, refus et compteurs. A-300 et E-50 restent
  différentiels, non littéraux, et le plancher global ne protège pas chaque
  motif. Ajouter un comparateur complet et des planchers nommés, ou réduire le
  libellé à cinq sorties littérales plus deux stresses ;
- le replayer local de `fold_live_gate.cpp` reconstruit une connectivité finale,
  mais ignore `output`, lots, niveaux et clés absentes du catalogue. Il n'a ni
  mutant ni rejet propre. Extraire le replayer strict de la porte T5 dans un
  utilitaire d'oracle commun, puis l'appliquer à `(catalogue résident,
  deltas vivants)` ;
- l'empreinte constante atteint l'internement des `FacetKey`, jamais la table
  `LiveIndex`. Ajouter une fixture directe de décalage arrière avec cluster
  traversant la frontière de table, suppressions tête/milieu/queue, `get`,
  `used_` et réinsertion. Le hash constant des lifetimes externes reste dû à
  L3 ;
- `g_alloc_bytes` est un maximum global, imprimé avec le témoin du maximum
  d'alias. Garder l'ordre témoin avec la valeur ou publier un troisième témoin,
  et échantillonner les capacités après les morts : les free-lists peuvent
  croître au dernier lot. Ce nombre reste celui de structures sélectionnées,
  pas une allocation bout en bout ni du RSS.

Les balayages structurels sont un oracle, pas un coût produit : le stride
actuel peut en exécuter jusqu'à 127 malgré le plafond commenté à 64, et un
`next` hors arène est déréférencé avant rejet. Employer un stride plafond,
valider les bornes, backlinks, le nombre de composantes non vides contre
`live_comp`, l'index et les free-lists, puis réserver ces parcours au mode de
vérification. Sinon le benchmark du vivant paiera lui-même des dizaines de
scans O(vivant). Aligner aussi la formule publiée et celle exécutée : la marge
`ceil(log2(F + 2)) + 1` est sûre mais plus lâche que la borne de doublement
`floor(log2(F))` par alias.

D reçoit maintenant l'ordre logique annoncé, mais trois triangles disjoints
font naître neuf alias. Ajouter séparément le témoin FIRST = LAST d'un unique
simplex K = 2, avec `peak_aliases == 3` et état final vide. Pour le mutant de
coût, le code 4 est mathématiquement légitime seulement après égalité complète
des sorties et compteurs synthétiques ; une divergence sémantique doit garder
priorité sur le plafond.

Le mutant `free-on-absorb` ne recycle plus ses alias, mais recycle encore leur
record de composante. Son code 4 Release court est reçu ; l'absence de
diagnostic ASan/UBSan est rapportée par Claude, mais n'a pas été rejouée par cet
audit. Le qualifier d'injection diagnostiquement sale tant que ces alias
gardent un indice placé dans `cfree`.

Enfin, `reduce_fold_live` reste hors du chemin produit et aucun gain CPU/RSS
n'est reçu. La sonde sale actuelle réexécute un second fold depuis
`on_forest`, alors que le résident du pipeline est encore vivant : son RSS
compare un résident à un résident plus un vivant. La mesure attribuable doit
faire lire le même flux d'événements à deux processus, chacun exécutant un seul
réducteur ; le bras vivant rattache sans copie le catalogue déplacé après
réduction, exécute le replayer strict, et les deux digests doivent être
identiques.

Le raccord minimal n'est donc pas un autre callback terminal. Ajouter au point
unique où `run_pipeline` appelle aujourd'hui `reduce_fold` une injection de
réducteur réservée au banc, puis imposer `fold_inflight=1`. Le processus
`resident` appelle une seule fois `reduce_fold`; le processus `vivant` appelle
une seule fois `reduce_fold_live`, reconstruit strictement
`final_canon_fid`, rattache le même catalogue, puis laisse le pipeline calculer
le digest normal. Le catalogue ne doit surtout pas être copié avant la
réduction vivante : une sortie réservée au test peut déplacer `fp.keys` après
la réduction, quand celle-ci n'en a plus besoin. Une copie O(F) biaiserait le
pic que le probe cherche précisément à mesurer. Cela donne le même objet publié
et un seul réducteur par processus. Le pic `ru_maxrss` reste un pic de bout en
bout — préparation comprise — ce qui est précisément la mesure utile ; les
temps internes gardent séparément préparation, FIRST/LAST, réduction et rejeu.

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

Un dernier correctif de code et trois finitions de preuve évitent pourtant de
recréer une preuve verte par vacuité :

1. `pool_stack().push_back(this)` est encore hors capture. Sa première
   allocation peut lever dans le worker, sortir de `run()` et appeler
   `std::terminate`, avec ticket non complété ; une injection indépendante le
   reproduit au code du handler 77. Le même stack autorise aussi le cycle
   bloquant `A(1) -> B(1) -> A`, reproduit par timeout. Les lanes n'ont besoin
   d'aucune soumission depuis un worker de pool : remplacer le vecteur par un
   pointeur TLS restauré par RAII, sans allocation, et refuser toute soumission
   quand ce pointeur est déjà non nul. Une composition acyclique entre pools
   pourra être spécifiée séparément si un consommateur réel la demande ;
2. le code 4 mutant est actuellement rendu si **n'importe quelle** attente
   échoue. Séparer les invariants communs du témoin ciblé : `pool-serial` ne
   meurt que par `peak < N`, `pool-drop-exception` seulement par l'exception
   cible avalée, et toutes les autres propriétés doivent rester vraies ;
3. intégrer le probe de constructeur fautif comme fixture permanente. Sans
   seam de lancement de thread, borner le claim au constructeur d'Executor et
   à la relecture du chemin de création partielle, au lieu d'annoncer une
   injection que la porte ne possède pas ;
4. `peak_queued <= 1` vérifie la borne annoncée mais pas qu'un producteur a
   réellement attendu. Bloquer les deux workers, placer un ticket en file puis
   observer au moins une admission retenue. Un compteur `queue_waits` suffit ;
   exiger aussi `peak_queued == 1` et `jobs_done == submitted` dans ce motif.

Le mutant série prend aujourd'hui 15,6 s parce que la latch attend N arrivées
alors que le mutant n'a qu'un worker. Attendre `pool.executors()` arrivées puis
comparer le pic au N demandé conserve la preuve nominale et tue le mutant
immédiatement, sans réduire le timeout de sûreté.

Le chemin P1 `close_fatal` ajouté ensuite est une bonne primitive hôte, mais sa
fermeture doit elle-même être sans allocation. Le `vector<Ticket*>` rempli sous
`mu_` peut lever après `stop_=true` et avant les notifications, laissant file
et producteurs bloqués. Dépiler la deque en place, poser erreur/done sous le
mutex de chaque ticket, puis notifier, sans tableau temporaire. Déplacer aussi
`submitted_++` **après** le `queue_.push_back` réussi : sinon un `bad_alloc`
crée un soumis qui n'est ni réussi, ni échoué, ni annulé. La fixture fatale
doit remplacer ses attentes de 100/200 ms par deux barrières : deux jobs actifs,
puis file réellement pleine, avant la fermeture. Enfin, tant que q3/q4 ne
classent ni n'appellent ce chemin, le qualifier de mécanisme hôte exercé, pas
encore de gestion reçue d'une erreur device réelle.

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
un snapshot peut donc observer un job en limbe et violer temporairement
l'invariant annoncé. Soit `counters()` est explicitement terminal seulement,
soit, préférable pour les barrières de test, pop+`running++` puis
`running--`+compteur terminal deviennent des transitions atomiques sous ce
même mutex.

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

1. Durcir `194a0bc2` sans refonte : TLS non allouant, fermeture fatale sans
   allocation, comptabilité après admission réussie et fixtures causales ; puis
   brancher le poison typé dans q3/q4 avant la prochaine réception device.
2. Pin hôte G1 fonctionnel : bornes d'indices, compteurs/mutants de branche,
   `PointId` q4 adverse et réparation du CTest `--inject`.
3. Pour L2 : garder `bc66ade7`, partager le replayer strict, compléter les
   fixtures/listes/backshift et attribuer chaque métrique à son témoin ; puis
   ouvrir L3 sans autre refonte du réducteur.
4. Après les deux pins hôte, une réception CUDA **minimale** de q4 index et des
   nouvelles dents suffit ; ne pas rejouer la matrice 50 k déjà acquise.
5. Mesurer ensuite la double représentation, le setup résident et le contexte
   partagé ; n'optimiser que le poste visible.
6. G2 seulement si cette ablation montre que les retours intermédiaires q4
   dominent encore ; sinon attaquer G3, qui retire aussi le wire par seed.
7. En parallèle : préparer le pilote CPU cpuset
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
- Worktree G1 postérieur : construction et CTest
  `mhgp5_batch_contract` verts en 0,04 s. CUDA désactivé localement ; aucune
  porte device de ce delta n'est annoncée comme exécutée.
- Worktree fold à créneaux : **6/6** CTests `mhgp5_fold_live*` verts ; le rejeu
  indépendant courant rend le nominal en 14,21 s, tandis que la plage locale
  non reçue consignée par les auditeurs va jusqu'à 25,78 s ; les cinq mutants
  rendent leur code attendu en 0,01–0,03 s. Le nominal repasse aussi sous
  ASan/UBSan en 125,89 s sans diagnostic. Ces exécutions confirment l'égalité
  fonctionnelle, la sûreté dynamique exercée et les invariants couverts, pas
  encore l'attribution du gain ni le claim mémoire corrigé. Les deux fixtures
  de frontière et le mutant de capacité ci-dessus manquent encore.
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
