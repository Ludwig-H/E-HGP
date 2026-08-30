# État courant audité de MorseHGP3D v5 — 30 août 2026

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur

Dernier pin source relu : `e2ac9da2`. Il reprend le build autonome et la garde
`--s` de `eba24b9a`, puis committe la lecture des sites `P>0` par la corde dans
les deux routes CPU, le mutant `chord-skip-positive`, une régression de masse
`terrain n=2000` et les corrections de portée de `PROFIL_SEPARATION.md`. Le
kernel CUDA n'est pas modifié par ce pin.

Le dernier commit concurrent relu, `1e4e0845`, ne change pas ces sources q4 :
avec `b1045ae5`, il ajoute le ledger causal puis les fixtures exactes proposées
pour `EndpointCredit`. Le worktree contient en revanche un brouillon
concurrent non commité dans `mutants.hpp`, `sector_kill.hpp` et `generate.hpp`,
qui raccorde `EndpointCredit` aux prétests q4. Ce brouillon est relu séparément
ci-dessous, sans être substitué au pin source. Les autres changements non
commités sont des propositions documentaires conservées pour Claude. Les
commits `d421bd69`, `30c1dc30`, `3b729c01` et `be8dfd26` portent les
contre-lectures q4 actives. Toute modification source ultérieure périme la
fraîcheur du verdict, pas les contre-exemples mathématiques ni les constats
attachés aux pins nommés.

## Verdict

Le chantier q3/q4 progresse et plusieurs optimisations proposées sont
réutilisables. Deux frontières restent toutefois non négociables : la forêt
publiée est encore le sous-flot Gabriel horizontal surqualifié en objet Gamma,
et les nouvelles sondes doivent comparer une autorité produit réelle plutôt
que deux transcriptions qui peuvent partager le même défaut.

Ce verdict bloque les claims d'exactitude et de performance, pas
l'exploration. L'ordre utile est : rendre le pin autonome, nommer honnêtement
l'objet, fermer la corde q4, rendre ses compteurs falsifiables, puis recevoir
la fusion WSPD.

## P0 — la sémantique de forêt reste surqualifiée

`docs/SPECIFICATION_MORSEHGP3D.md` et
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` réservent au flot Gabriel brut un
rôle de proposition ou de connectivité positive. Le contrat Gamma exige aussi
les incidences silencieuses. La faute doit être localisée précisément :
`docs/MATHEMATIQUES.md` § 1.2 identifie explicitement « la forêt HGP » au K-MST
Gabriel et `src/forest/fold.hpp` appelle ses deltas « payload hiérarchique
complet ». `README.md` et `docs/ARCHITECTURE.md` ne nomment ni Gabriel ni K-MST,
mais surqualifient la portée en l'identifiant aux dix forêts horizontales HGP.
La sortie combine en outre `tower_scope=profile_complete_k10` et
`vertical_maps=none`.

La fixture `gabriel-point-set-counterexample-5-points-v1` reste décisive : les
cofaces non-Gabriel `ACD` et `ACE` attachent silencieusement `AC` au niveau
carré `33/2`, puis `ABC` la réutilise au niveau `83886/3563`. Gamma et le flot
Gabriel brut n'ont alors ni la même généalogie ni le même temps de fusion. Le
juge racine `tests.oracle.test_gabriel_counterexample` retrouve ce défaut en
arithmétique rationnelle indépendante.

Fermeture constructive attendue :

1. Publier la source actuelle comme
   `proof_basis=gabriel_positive_connectivity` et
   `forest_semantics=verified_events_only`, avec une portée horizontale.
2. Ajouter `reconstruction_contract_id` et `require_exact`, puis refuser
   atomiquement `require_exact=true` sur cette source.
3. Graver la fixture cinq points dans une porte v5 Gamma contre produit.
4. Ajouter ensuite une source sparse d'incidences silencieuses, sans faire de
   l'oracle exhaustif borné l'architecture produit.

## P1 — `eba24b9a` ferme le build, pas tous les claims associés

`eba24b9a` suit désormais `src/core/parse.hpp`, route `--s` par
`std::from_chars` dans les deux CLI et refuse les valeurs numériques sous 8.
Le build autonome qui manquait est donc rétabli sur la voie CPU : une archive
Git propre du commit configure et construit toutes les cibles CPU sous
`-Werror`, et les portes ciblées du profil passent.

Le raccord parse exactement `--s` seulement. `--n`, `--coord`,
`--seed`, `--smax`, `--threads`, `--cell-min-sites` et les options CUDA restent
sur `atoi`/`atoll`. La ligne candidate de `docs/PROVENANCE.md` doit donc dire
« parsing exact de `--s` », ou Claude doit convertir et tester toutes les
options qu'elle revendique. Les suffixes, le vide et les débordements doivent
rester des rejets de code 2.

`INT64_MAX` est volontairement accepté : c'est une frontière arithmétique, pas
un profil de coût, et son front peut être quadratique. Le signe explicite
`--s=+8` est refusé par `from_chars` ; cette grammaire doit être documentée ou
son choix modifié puis testé. Les CTests actuels vérifient le code CLI, pas le
texte exact de stderr.

Le bypass sous `s=8` est décrit comme « compilé test-only », mais le booléen de
contournement reste présent dans toute compilation ; seul le champ d'options
est conditionné. Il faut soit conditionner aussi la branche, soit borner le
claim à un opt-in interne non exposé par les entry points produit. Enfin,
`docs/PROFIL_SEPARATION.md` ne doit pas dire que tout cœur est vide sous 8 ni
qu'aucun rectangle n'est généré : les singletons donnent une contre-fixture et
la bibliothèque refuse explicitement le profil produit.

La fermeture restante est documentaire et de portée : dire exactement
« parsing exact de `--s` », ou convertir les autres options avant de parler des
« entiers de CLI ». Le build et les portes CPU de `--s` doivent être rejoués
depuis le pin ; la CLI CUDA n'a pas été recompilée localement sans `nvcc`.

## P1 — q4 : fermer la vérité avant de mesurer la vitesse

### `e2ac9da2` récupère les témoins `P>0`, mais dépend encore de leur ordre

Au pin précédent, un site certifié `P>0` quittait les routes scalaire, shaped et
device avant `ChordPieces::update`. `e2ac9da2` corrige utilement ce placement
dans les deux routes CPU. Sa régression trouve sur `terrain n=2000` exactement
`24727` morts de corde contre `21691` sous le mutant, avec `9401` candidats q4
dans les deux bras. Le défaut historique était donc bien fail-open et du
travail est réellement évité sans modifier cette sortie finale bornée.

La couture de contrôle reste toutefois ouverte : le pin teste encore
`chord.dead(h4)` après le `continue` des sites positifs. Si un tel site complète
le dernier morceau, la décision dépend de l'ordre. La fixture indépendante à
cinq points rend, sur la route shaped nominale,
`dead=1,dead_by_chord=1` lorsque le positif vient en premier, mais
`dead=0,dead_by_chord=0` lorsqu'il vient en dernier ; le mutant rend les deux
ordres vivants. En parallèle, les trois CTests `mutants_gate`,
`chord_positive` et `chord_positive_mutant` passent. La porte de masse est donc
une bonne régression d'intégration, mais une autorité insuffisante pour cette
couture.

Le kernel CUDA conserve l'ancien saut complet. La fermeture CPU tient à un
test supplémentaire après l'update du seul cas positif, avant son `continue` ;
les sites non positifs gardent le cœur prioritaire puis le test de corde
historique. Il faut graver les deux permutations dans la porte exacte, puis
former `my_piece` avant la branche du cœur dans le kernel.

### Le brouillon `EndpointCredit` q4 est sûr mais ne réalise pas encore son contrat

Le worktree propage utilement `h_a+h_b` vers le pré-scan W4 et les secteurs.
La disjonction employée est correcte : le crédit vit dans `A union B`, tandis
que `cnt_out` ne lit que son complément. Mais le commentaire annonce
`min_k max(cnt[k],cnt_out[k]+base)>=h`, alors que le code calcule seulement
`min_k cnt[k]>=h` **ou** `min_k(cnt_out[k]+base)>=h`. Ce dernier reste fail-open
et sûr, mais il perd les cas où certains secteurs sont fermés par le compte pur
et les autres par le crédit. Pour réaliser le contrat annoncé, prendre le
maximum secteur par secteur avant le minimum et graver une fixture croisée où
aucune des deux branches globales ne suffit seule. Le commit documentaire
`1e4e0845` donne désormais la fixture géométrique q3 à cinq positions qui
réalise ce croisement, puis une fixture distincte de fausse mort pour le mutant
de double comptage. Elles sont préférables à un test limité aux tableaux du
helper et doivent devenir les autorités CTest.

Le raccord n'est pas encore homogène : `q3_lane_batched.hpp` et
`q4_lane_batched.hpp` construisent les mêmes histogrammes mais ne transmettent
pas `EndpointCredit` à leurs prétests, routes hôte ou prétest manuel avant le
lot device. Aucun nouveau wire n'est requis : construire le crédit par ancre
côté hôte, le passer aux fallbacks et employer la même fonction de prétest
avant matérialisation. Sans cela, la production et les lanes par lots divergent
dès que le nouveau kill devient non vacant. Le mutant `sector-credit-inbox`
n'a encore aucune porte visible ; il faut une mort fausse par double comptage,
des planchers sur les nouvelles morts et la parité production/batch. Ce
brouillon ne corrige par ailleurs aucune des trois coutures de corde ci-dessus.

Le rejeu local rend ces deux manques non vacants. `mhgp5_mutants_gate` échoue :
83 mutants sont déclarés et injectés, mais 82 seulement possèdent une porte
CTest en code 4 ; `sector-credit-inbox` est l'orphelin. La fixture secteur
historique passe et ne discrimine donc pas le nouveau contrat. Surtout, les 15
portes batch ciblées q3/q4 choisies — familles, ordre, route mixte, tout-hôte et
ancre surdimensionnée — échouent toutes. Les vecteurs finaux restent égaux,
mais la production tue davantage par secteurs et les compteurs aval divergent :
sur `uniform n=1200`, q3 donne `12066/11821` morts secteur et
`1384153/1392373` seeds production/lots ; q4 donne `24196/23945` et
`1290674/1298238`. Le chemin tout-hôte échoue lui aussi, ce qui localise le
défaut dans la propagation du contrat, pas dans le kernel device.

### La sonde de corde actuelle n'est plus une autorité indépendante

`q4_chord_probe` construit son flux `emitted` en appelant aujourd'hui le
produit avec grille et K=4 actifs, puis son replay manuel omet la grille et
recalcule K=1/2/4/8. `wrong=0` est donc tautologique pour K=4, masqué pour K=2
et partiellement masqué pour K=8. La sonde accepte aussi une exécution vide :
`uniform n=2` rend zéro seed, zéro complétion et un code 0.

Le reçu historique reste un diagnostic de `f8f5b4ff`, commit qui introduit la
sonde et les sorties. Le pin imprimé `635951d6` ne contient pas ce fichier ; il
provenait d'une configuration antérieure et ne suffit pas comme provenance du
binaire.

Réparation : bras de vérité avec corde et grille désactivées, partition
explicite `grille -> cœur -> corde -> faces_D`, comparaisons par support, et
planchers sur seeds accessibles, K=4 exercé et complétions. La source, la
commande, le stdout et le hash du binaire doivent être épinglés ensemble.

### Les nouveaux compteurs sont diagnostics, pas encore mesures d'étage

`eba24b9a` ajoute des compteurs utiles, mais plusieurs noms excèdent ce qui
est réellement compté :

- `q4_sector_sites` additionne la taille offerte du cover, alors que le scan
  sectoriel peut sortir avant de la parcourir ;
- `q4_grid_scan_sites` compte une masse éligible au pré-scan, pas les sites
  consultés par la construction et les requêtes de grille ;
- `q4_chord_cell_tests` compte une requête par seed avec grille, pas les
  cellules visitées ;
- `pretest_sites` compte la taille de la requête, sans les sorties anticipées
  ni le double passage W4/secteurs.

La soustraction u64 qui imprime les seeds survivants peut sous-dépasser et
masquer un double comptage. Il faut une identité contrôlée en arithmétique
large, un compteur explicite `q4_faces_reaching_completions` et un bit
`seed_ledger_ok` dans le verdict. Les CTests q4-stage actuels forcent
`--pretest-query-min=0` : les nouvelles voies W4, secteur et grille peuvent
donc rester à zéro tout en passant. Ajouter un bras cover et un bras grille
forcée avec planchers, plus un mutant de ledger.

Les temps sont imbriqués : `boucle_seeds` contient cœur et complétions, puis
`completions` contient profondeur. Ils ne se somment pas. Les libeller
`inclusive`/`dont`, vérifier l'échec de l'horloge et mesurer son coût avant de
tirer un ratio d'étage.

Le helper nommé `mhgp5_thread_cpu_ns` lit en réalité
`CLOCK_PROCESS_CPUTIME_ID`, sans traiter l'échec de `clock_gettime`; le rapport
mur/CPU n'isole pas exclusivement la contention. Enfin, les champs de profil
et leur agrégation existent dans `GenerateStats` hors `MHGP5_PROFILE_Q4` : seule
leur alimentation est neutralisée. Le commit ne peut donc pas les qualifier de
« sans effet sur le binaire produit » sans mesure ou conditionnement complet.

Le ledger causal ajouté dans `b1045ae5` est la bonne prochaine sonde, avec une
garde : son identité `N_scan=seeds[1]-seeds_killed_cells[2]` suppose
`invariant_jneg==0`. Sinon un `Jb<0` saute le scan mais finit actuellement dans
`seeds_killed_core`; il faut soit refuser le reçu si l'invariant est non nul,
soit séparer `jneg` et écrire `N_scan=seeds-cells-jneg`. Le titre et les phrases
« ne change pas » / « pas un artefact de famille » de `docs/Q4_MUR_UNITE.md`
restent incompatibles avec la requalification mono-graine de ce même document.
Les exposants locaux de compteurs à deux tailles ne doivent pas devenir des
claims asymptotiques sur une lane ou une famille.

### Deux corrections mathématiques aux audits

Deux angles distincts avaient été confondus. Le critère ponctuel de W4 impose
`angle(azb)>125,264 deg`. La pointe du fuseau a une demi-ouverture
`54,736 deg`, donc une ouverture complète `109,471 deg`. Le `125,26 deg`
antérieur était non nommé, pas numériquement faux ; les audits qui le
rétractaient catégoriquement sont corrigés.

Le futur minimum exact sur corde peut bien conserver au plus `2h4=16` racines,
mais le compte constant `c0` doit aussi recevoir les racines hors corde actives
partout : `B=0,P<0`, `B>0,P/B<alpha` et `B<0,P/B>beta`. Les cas opposés hors
corde ne témoignent jamais. Sans cette classification, sur la corde `[-1,1]`,
le site `P=-2,B=1` serait oublié alors qu'il est actif partout.

Le filtre ponctuel de complétion `2P^2<=JB^2`, sa frontière stricte et les
fixtures associées restent reçus mathématiquement. Ils réduisent une constante
après énumération de D ; ils ne remplacent pas une porte de face avant cette
énumération.

## P1 — fusion WSPD : bonne idée, porte produit encore absente

Le fait de structure est prometteur : les décisions de scission WSPD ne
dépendent pas de la lane et le compteur de témoins accepte déjà un masque.
Les six CTests de `9940668e` passent, mais ils comparent deux transcriptions
locales de la descente et non la transcription fusionnée à
`alive_rectangles`.

`collinear_seven` falsifie déjà l'équivalence au chemin produit. La famille
retourne neuf points même avec `--n=600`; le produit applique alors
`smax_effective=9` et publie `rect_alive=30/30/29`. La sonde force `smax=11`
dans ses deux bras, publie `30/30/30` et passe. Elle prouve l'accord de ses deux
copies, pas leur accord avec le produit.

Les claims mémoire doivent également être retirés. `size*sizeof(T)` est une
charge utile minimale, pas un pic : il ignore `capacity`, la coexistence
`vague/suivante`, les tampons parallèles et les copies de concaténation. Le
reçu `uniform n=32000` donne en outre `1094102/2908394/3233183` rectangles ;
trois listes représentent environ 115,8 Mo décimaux contre 51,7 Mo pour le
maximum séquentiel, pas « 26 Mo contre 12 Mo ».

Avant raccord produit :

1. Ajouter d'abord au chemin produit le ledger global de masse de paires. Le
   ledger `pair_mass==expected_pair_mass` n'existe aujourd'hui que dans
   `wspd_wavefront`, appelé uniquement par les tests ; `alive_rectangles`
   vérifie son raffinement post-séparation à partir d'une masse `base`, mais ne
   retrouve jamais la masse globale attendue. Cette égalité est nécessaire,
   non suffisante : une porte par multiensemble de petites paires doit empêcher
   deux erreurs compensatoires.
2. faire appeler directement `alive_rectangles` par le bras A avec
   `smax_effective=min(smax,input_count)` et imprimer effectifs demandés/réels ;
3. ajouter `scanline_overlap_multiecho`, plusieurs graines et des planchers
   non vides, puis deux mutants qui forcent le code 3 ;
4. épingler les sorties brutes et le hash du binaire ;
5. mesurer les capacités simultanément résidentes ou un HWM attribuable aux
   tailles 8k/16k/32k ;
6. seulement ensuite raccorder la fusion, conserver l'ordre exact des listes
   et requalifier les compteurs par lane.

Le raccord à `postsep_refine_levels>0` reste un contrat séparé.

## P1 — stratégie sous-quadratique : plan conditionnel reçu, borne ouverte

`STRATEGIE_SOUS_QUADRATIQUE_Q3_Q4_20260830.md` fournit une direction utile :
front WSPD prouvé à `s=8`, requêtes saturées de facteurs, fermeture
hiérarchique par `h_c`, puis sous-complexe de faibles profondeurs pour les
ancres lourdes. Elle sépare correctement les gains de constante, la sortie et
les termes d'incidence. Elle ne résout pas le P0 Gamma et reste attachée au
sous-flot horizontal `verified_events_only`.

La contre-revue a imposé quatre corrections dans le document : le grand-livre
paie explicitement chaque ancre résiduelle `|E|`; la profondeur historique
`delta_e^(3)` sur `cover3` reste distincte de `delta_e^full`; les concurrences
et incidences de droites ont leur propre terme ; et `h_c` ne ferme un nœud de
carriers qu'avec un seuil uniforme prouvé sans visiter ses feuilles. Le
coarsening du front est borné à `postsep_refine_levels=0`.

Le choix Morton recommandé pour le premier incrément est de réutiliser le cube
`Q` et de contracter ses au plus sept préfixes internes dans un quotient octree
indépendant. Les invariants suggèrent le lemme de simulation
`R_shadow<=49*R_oct<=49*C_3(8)*m`, mais le facteur 49 n'est reçu qu'après avoir
prouvé l'injection terminale et n'est pas la constante WSPD totale. Le charging
`C_3(8)` et le nouveau ledger global de coupe
`R_cut=rectangles_emis+rectangles_tues_core` doivent encore entrer dans une
porte avant `R_cut=O(m)`.

Une spécification de prototype exact est dérivée pour les facteurs : `ALL` est
équivalent aux 64 couples de coins sur le produit continu partenaire--témoin,
donc seulement suffisant pour les positions discrètes d'un nœud ; `NONE`
dispose d'un certificat entier conservateur, et `MIXED` descend jusqu'aux
feuilles pour rendre le compte exact. Son coût est
`O(V_R+C_R+|A|+|B|+P_R)`, où `V_R` compte tous les nœuds classifiés,
`C_R<=64*V_R` les couples de coins et `P_R` les ancres survivantes de ce seul
filtre. Le crédit `ALL` conserve d'abord l'unité de `corner_histograms`, une
position unique par feuille : employer `node_weight()` compterait les buckets
dupliqués et changerait le contrat. Chacun de ces termes peut encore être
quadratique. Le premier moteur plan suit la même discipline :
`PlanConflictGrid` shadow q3 mesure `K_conf`, puis le shadow q4 mesure
`P_grid`; aucun des deux compteurs n'est une preuve d'exposant.

Le claim global reste NO-GO tant que `R=O(n)`, le constructeur exact des
niveaux peu profonds et chaque terme du grand-livre ne sont pas prouvés. Les
cinq tailles et trois graines peuvent réfuter ou borner une classe annoncée ;
elles ne remplacent aucune de ces preuves.

## Ordre de travail conseillé à Claude

1. Fermer `e2ac9da2` avec les deux ordres de la fixture q4 et la route device ;
   garder CUDA non reçu tant que `nvcc` n'a pas rejoué la porte.
2. Finir le raccord `EndpointCredit` secteur par secteur, puis le recevoir sur
   les routes production, batch et device avec mutant non vacant.
3. Livrer le patch de vérité Gabriel/Gamma et le refus `require_exact`.
4. Limiter exactement le claim de parsing à `--s` dans la documentation.
5. Fermer le ledger q4 et rendre les nouveaux compteurs non vacants.
6. Recevoir le front `Q` par quotient octree, shadow et ledger global, puis
   seulement raccorder la fusion des lanes et mesurer la mémoire.
7. Ajouter les requêtes `h_a/h_b` `ALL/NONE/MIXED` en shadow contre
   `corner_histograms`, avec garde cardinal et compteurs `V_R+C_R+P_R`.
8. Livrer `PlanConflictGrid` q3 counter-only, puis q4 avec budget/fallback ;
   n'ouvrir le constructeur orienté de faibles profondeurs qu'après ces
   profils exacts.

## Vérifications indépendantes de cette passe

- archive Git propre de `eba24b9a` : configuration et build CPU complets sous
  `-Werror` ; build Release canonique du workspace : succès à 100 % ;
- portes CPU/API `s`, fold, séparation, q4-stage et fusion WSPD : 29/29. Les
  dix sondes s'exécutent, sans réparer les autorités contestées ci-dessus ;
- au pin `e2ac9da2`, les CTests `mutants_gate`, `chord_positive` et
  `chord_positive_mutant` passent 3/3 en 4,37 s ; la régression imprime
  `24727/21691` morts de corde et `9401/9401` candidats sous nominal/mutant ;
- harness shaped indépendant à cinq points, compilé sous `-Werror` : positif
  premier `dead/chord=1/1`, positif dernier `0/0`, mutant `0/0` dans les deux
  ordres ;
- `python3 -m unittest tests.oracle.test_gabriel_counterexample -v` : 4/4 ;
- `mhgp5_q4_chord_probe --family=uniform --n=2` : code 0 avec tous les
  planchers utiles à zéro ;
- sonde/produit `collinear_seven n=600` : `30/30/30` contre `30/30/29` ;
- sélection CTest hors `scale*` et `slow` : 281/282 sous le timeout par défaut ;
  `mhgp5_postsep_refine_mutant_h1` a atteint 300 s tandis que son témoin a pris
  311,5 s. Le même wrapper mutant isolé réussit sous une enveloppe de 600 s :
  c'est une insuffisance du budget sur machine partagée, mais la commande CTest
  globale reste techniquement rouge ;
- validation manuelle de tous les Markdown d'audit et
  `python3 tools/check_docs.py` : succès sur 222 fichiers ;
- CUDA non compilé ni exécuté localement, faute de `nvcc`.

GCP non utilisé.
