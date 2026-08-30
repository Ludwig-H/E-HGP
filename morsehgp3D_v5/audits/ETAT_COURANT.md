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

La tête relue `d42f69ff` ne change pas ces sources q4 : avec `b1045ae5`,
`1e4e0845` et `7aaf0ce5`, elle consolide le ledger causal, les fixtures
attendues et la réception des routes. Le worktree contient en revanche un
brouillon concurrent non commité qui raccorde `EndpointCredit` aux routes
scalaire et batch, prend désormais le maximum secteur par secteur, corrige la
corde scalaire/shaped/kernel et ajoute ses fixtures locales. Ce brouillon est
relu séparément ci-dessous, sans être substitué au pin source. Le kernel CUDA
est corrigé en lecture seulement : `MHGP5_ENABLE_CUDA=OFF` et aucun `nvcc`
n'est disponible. Les autres changements non commités sont des propositions
documentaires conservées pour Claude. Toute modification source ultérieure
périme la fraîcheur du verdict, pas les contre-exemples mathématiques ni les
constats attachés aux pins nommés.

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

Le worktree applique maintenant la bonne couture : scalaire et shaped testent
la mort après l'update positif et avant son `continue`; le kernel forme
`my_piece` pour tout site avant de brancher sur `P>0`, puis conserve la priorité
du cœur à égalité de lane. La correction nominale est cohérente en lecture.
Elle n'est pas encore reçue : aucune fixture produit/shaped à cinq points et
deux permutations n'est enregistrée, `q4_core_shaped_gate` désactive la corde,
et le kernel n'a été ni compilé ni exécuté sans `nvcc`. CUDA reste donc
`not_received`, non « fermé ».

### `EndpointCredit` : helper algébrique reçu, crédit du cœur encore absent

Le worktree propage `h_a+h_b` vers le pré-scan W4 et les secteurs. La
disjonction est correcte : le crédit vit dans `A union B`, tandis que
`cnt_out` ne lit que son complément. Claude a remplacé l'union globale trop
faible par `min_k max(cnt[k],cnt_out[k]+base)` et ajouté le mutant
`sector-credit-global`. La fixture géométrique q3 à cinq positions ferme le cas
croisé ; nominal, mutant et registre passent `3/3`. Ce contrat local n'est donc
plus un verrou.

Le premier état n'était pas homogène : les builders q3/q4 ne transmettaient
pas le crédit aux prétests hôte. Claude construit désormais le même token par
ancre avant toute matérialisation et le passe aux deux routes ; aucun changement
de wire n'était requis.

Le rejeu reçoit cette progression : build ciblé réussi, registre et fixture
locale verts, puis q3/q4 `uniform`, `clusters` et les deux routes tout-hôte
verts, soit `9/9` portes ciblées en `155,65 s`. Ces mêmes parités échouaient
avant le raccord. La propagation scalar/batch de ce sous-ensemble est donc
fermée dans le worktree courant ; les autres routes batch et la suite complète
restent à rejouer après commit. Le second rejeu shaped/corde/secteur/q4-batch
porte ce sous-ensemble à `14/14` en `47,02 s`.

La fixture croisée fait toutefois encore confiance à
`EndpointCredit.base=1`. Elle ne calcule pas ce crédit via
`corner_histograms`/l'autorité de coins, ne vérifie pas les huit comptes purs
annoncés et ne passe pas par un seed vivant. Il reste donc à graver une porte
d'intégration `histogrammes -> crédit -> secteurs`, en plus de la bonne porte
locale désormais reçue. La précondition `base<hh` doit aussi être encodée ou
documentée : les helpers ignorent un appel direct `base>=hh`; la route produit
l'évite seulement parce que la porte d'histogramme a déjà tué l'ancre.

Les parités batch ne comparent pas encore tout `GenerateStats`. Les builders
ne remplissent pas `hist_killed_rows`, `hist_killed_thresh` et
`hist_survivors`, et leurs gates ne comparent pas ces champs. Partager
l'énumérateur ou ajouter ces trois comptes, puis exiger
`rows+thresh+survivors=anchors` ferme le ledger plutôt que le seul aval.

Surtout, le token vaut toujours seulement `h_a+h_b`. `ar.core` sert au seuil
de la porte histogramme, puis n'est pas transmis aux secteurs. Le chemin reste
fail-open, mais ne réalise pas encore `h_core+h_a+h_b`. La fermeture propre est
bornée : appeler une fois `collect_universal_ids` par `AliveRect`, recertifier
les au plus `h_q-1` IDs cœur hors `A union B`, puis former
`base=h_core+h_a+h_b` et `cnt_res` hors facteurs **et** hors IDs cœur. Le
verdict reste `min_k max(cnt[k],cnt_res[k]+base)>=h_q`. Si la recertification
échoue, notamment après héritage postsep, omettre le cœur du crédit et compter
le repli. Cette même provenance prépare le tape résiduel de `h_c`.

### `CREDIT_SECTEUR.md` ferme le lemme, pas encore le coût

La nouvelle note de Claude apporte une clarification juste : au stage exact
`W_q`, chaque témoin du crédit est déjà dans le compte complet, donc la forme
résiduelle peut seulement anticiper la même mort ; au stage sectoriel, le
polygone suffisant peut ne pas recompter ce témoin universel dans chaque
secteur, et le minorant `max(cnt[k],cnt_out[k]+base)` devient utile. Cette
séparation mathématique doit être conservée.

La réponse de Claude reçoit aussi honnêtement le second défaut de corde. Dans
le worktree, les routes scalaire et shaped constatent maintenant la mort après
la mise à jour positive et avant le `continue`, avec la priorité historique
préservée. Le vrai `q4_kernels.cuh` forme désormais lui aussi `Bz/my_piece`
avant de brancher sur `P>0`. Le titre « verrou de corde fermé » reste néanmoins
prématuré jusqu'à la fixture à deux ordres et au rejeu CUDA : le kernel n'a été
ni compilé ni exécuté dans cet environnement.

La nouvelle `sector_credit_fixture.cpp` garde bien le mutant au code 4, mais
elle construit encore
`EndpointCredit{1,0,m-1,0,-1}` : la plage B est vide, A contient ancres et
sites, et `base=1` ne provient pas de `corner_histograms` sur deux nœuds WSPD
disjoints. Son second crédit emploie deux boîtes singletons, qui donneraient
réellement `h_a=h_b=0`. Cette porte teste donc un token de helper de confiance,
pas une fausse mort atteignable du produit. La nouvelle fixture croisée, elle,
emploie des ranges disjoints et tue `sector-credit-global`; ses trois portes
passent. Elle doit encore vérifier explicitement que `e` satisfait bien
`universal_over_corners` et, idéalement, exercer
`corner_histograms -> EndpointCredit -> secteurs` sur de vrais `NodeRef`.

Les conclusions de performance de la note dépassent toutefois ses données.
La table mono-graine n'a encore ni source, commande, stdout, hash ni reçu et
précède les fermetures du combinateur et des routes batch ; elle doit être
rejouée avant d'être attribuée au contrat exact courant.
Surtout, « gratuit », « dominant », « ne change pas l'exposant » et
« le mur reste le cœur » ne découlent pas de `0,28--0,53 %` d'ancres mortes en
plus. Le code ajoute une classification et jusqu'à huit compteurs par site, et
une petite population d'ancres peut concentrer une grande part des scans longs.
Dire **sûr et monotone** est acquis ; dire gratuit ou asymptotiquement neutre
reste ouvert.

Après le raccord exact et les fixtures, la mesure utile est donc appariée par
`AnchorKey` : nouvelles morts dues au crédit, somme de `q4_core_site_tests`
évitée, seeds/essais D évités, temps exclusif et HWM. La note peut rester une
hypothèse de travail ; elle ne devient durable qu'avec ce reçu et sans s'appuyer
sur `Q4_MUR_UNITE.md`, dont le compteur fusionne encore cœur et corde.

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

### La nouvelle sonde canopée n'est pas encore appariée

Le paramètre `--canopy-lift-cap` rend enfin le bras borné versionnable, mais son
implémentation change directement la borne de `uniform_int_distribution`.
Elle ne réalise donc pas le contrat « même tirage latent, puis écrêtage » : le
nombre de tirages moteur peut diverger, et une collision créée par le cap décale
ensuite la déduplication et les `PointId`. Générer d'abord le nuage nominal avec
métadonnée de canopée, cloner les mêmes points/IDs, écrêter leur lift et refuser
le couple si une collision apparaît ferme ce point sans inventer une nouvelle
famille de propositions.

Le probe doit aussi refuser un cap négatif, supérieur au plafond nominal ou
appliqué hors `terrain`, imprimer la graine effective `uint32`, et transmettre
le même `EndpointCredit` que le produit. Le mode q4 de `recu_local.sh` ne doit
pas exiger l'égalité de `masses_q4/seeds_q4` entre deux nuages différents ni
l'appeler « identité d'objet » ; il doit recevoir séparément digest d'entrée,
lineage, points modifiés, collisions, commande et source complète du binaire.
Enfin, le dirty gate couvre aujourd'hui le script mais pas
`bench/q4_stage_probe.cpp`, ce qui permettrait d'écrire « propre » pour une
sonde recompilée depuis une source sale.

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
