# État courant audité de MorseHGP3D v5 — 31 août 2026

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur

Dernière consolidation amont relue : `81089f0b`; son dernier pin fonctionnel
est `fc53472f`. Le présent audit ne modifie aucune source fonctionnelle.
Il conserve les corrections q3/q4 de `9f504e52`, versionne dans `c53229b9` le
reçu terrain construit depuis ce pin, reçoit sa contre-relecture dans
`1bd91360`, puis propage le paramètre de hauteur au champ scanline. `a78d0338`
versionne le reçu scanline et une interprétation documentaire sans modifier
ces sources ; `92605016` reçoit la contre-relecture scanline/`linked_arcs` dans
un audit seulement. Malgré son sujet de commit, il n'ajoute encore ni fixture
`linked_arcs_u16`, ni porte CTest, ni oracle versionné. Le kernel CUDA reste
corrigé en lecture seulement :
`MHGP5_ENABLE_CUDA=OFF` et aucun `nvcc` n'est disponible.

Le reçu committé `canopee_q4` est `complete` au sens « 18 processus ont rendu
0 », mais son binaire sha256 `21794af5...` est épinglé à `38fa88af`, avant le
raccord d'`EndpointCredit` et avant `bump_amp_cap`. Il décrit des cohortes
indépendantes ; il ne reçoit ni la sonde actuelle ni une causalité. Le reçu
versionné `terrain_deux_echelles` est terminal : `source_commit=9f504e52`,
binaire `b319abda...`, 27/27 codes zéro, 27/27 identités internes fermées et
aucun stderr non vide. Son autorité reste `diagnostic_unpaired` et garde le
défaut de couplage des distributions décrit ci-dessous. Le reçu versionné
`scanline_relief` est également terminal : `source_commit=fc53472f`, binaire
`2d202f0e...`, 18/18 codes zéro, identités internes fermées et stderr vides.
Sa portée reste `diagnostic_unpaired`, une répétition sans alternance, sans
tape, lineage ni digest d'entrée, uniquement sur `scanline_single_pass`.
Cette consolidation ne modifie que les audits centraux et le worktree suivi est
propre à sa publication. Toute modification source ultérieure périme la
fraîcheur du verdict, pas les contre-exemples mathématiques ni les constats
attachés aux pins nommés.

## Verdict

Pour q3/q4, la cible architecturale candidate la mieux étayée est désormais
**sortie-sensible** et conditionnelle : WSPD binaire `A x B` comme
owner/certificat à `s=8`, facteurs et cœur composés
avec provenance, fibre `C` hiérarchique, requête globale de seuil par `BallKey`
sur la petite route, puis lift shallow streamé pour les fanouts lourds ; q4 ne
cherche `D` qu'à partir d'intersections orientées entre un seed aigu vivant et
une complétion admissible dans les zones non certifiées profondes. Les probes
jetables indépendants et de route produit rencontrent déjà une famille
géométrique à nombre quadratique de boules critiques ; sa transposition au
contrat doit encore être gravée par la fixture oracle+pipeline. Le pipeline
courant appelle le fold dense, dont le payload conserve toutes les
`facet_keys` et un `final_canon_fid` de même cardinal : après réception de
cette fixture, l'obstruction portera donc aussi sur **cette forêt explicite**,
même si elle finit connexe. Une garantie « sous-quadratique pour toute entrée
et sortie explicite » n'est plus un objectif mathématique recevable. Cela ne
borne ni une future API implicite/reconstructible, ni Gamma, ni une asymptotique
infinie propre au seul domaine u16 fixe.

Deux frontières indépendantes restent non négociables : la forêt publiée est
encore le sous-flot Gabriel horizontal surqualifié en objet Gamma, et les
nouvelles sondes doivent comparer une autorité produit réelle plutôt que deux
transcriptions ou deux familles aléatoires non appariées. Ce verdict bloque les
claims d'exactitude et de performance, pas l'exploration ni les shadows bornés.

## P0 — la sémantique de forêt reste surqualifiée

`docs/SPECIFICATION_MORSEHGP3D.md` et
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` réservent au flot Gabriel brut un
rôle de proposition ou de connectivité positive. Le contrat Gamma exige aussi
les incidences silencieuses. La faute documentaire restante doit être localisée
précisément. `docs/MATHEMATIQUES.md` § 1.2 distingue correctement l'objet K-MST
visé du flot Gabriel actuellement rendu, et `src/forest/fold.hpp` dit
explicitement que ses deltas ne forment pas un payload hiérarchique complet et
ne peuvent pas porter `require_exact=true`. En revanche, `README.md` annonce
encore « le même objet que la v4 — les dix forêts horizontales HGP » et
`docs/ARCHITECTURE.md` nomme encore son objet « les dix forêts horizontales
HGP », tout en déclarant dans les deux cas
`forest_semantics=verified_events_only`. Cette auto-contradiction, avec
`tower_scope=profile_complete_k10` et `vertical_maps=none`, est le P0 à fermer.

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

Le pin applique maintenant la bonne couture : scalaire et shaped testent
la mort après l'update positif et avant son `continue`; le kernel forme
`my_piece` pour tout site avant de brancher sur `P>0`, puis conserve la priorité
du cœur à égalité de lane. La correction nominale est cohérente en lecture.
Elle n'est que partiellement reçue : aucune fixture scalaire/shaped à cinq
points et deux permutations n'est enregistrée, et le kernel n'a été ni compilé
ni exécuté sans `nvcc`. CUDA reste donc `not_received`, non « fermé ».

La porte commitée active la corde dans un mode
`q4_core_shaped_gate --ordre-corde` et inverse tout le cover. La paire nominale
/ mutant passe `2/2`; le replay direct compte `25675` morts de corde dans les
deux ordres nominaux, puis `25091` morts et `426` désaccords d'ordre sous
`chord-dead-skip-positive`, qui sort bien au code 4. C'est une bonne régression
d'intégration CPU shaped. Le plancher CMake vaut désormais `20000` et le
nominal le dépasse : la vacuité du premier brouillon est fermée. Ce nuage
aléatoire ne remplace toutefois pas la fixture exacte cinq points. Sous
`__CUDA_ARCH__`, le système de mutants hôte est
désactivé ; le kernel se reçoit donc soit contre l'oracle CPU exact, soit avec
un bit test-only explicitement transmis, jamais grâce à cette porte shaped.

Deux limites supplémentaires bornent son message de commit. Les `426` sont les
désaccords du reversal de cette réalisation, pas le nombre universel de seeds
que l'ancien code manquait, et `morts_corde` additionne les attributions des
deux replays plutôt que des seeds uniques. La porte n'appelle pas
`process_anchor_q4` : l'injection scalaire passe encore la porte de masse avec
`24700` morts de corde et un code 0. Enfin, le plancher est testé avant le fate
mutant ; si le mutant tombait sous `20000`, la porte rendrait 3 au lieu du code
4 contractuel. Le mutant doit rendre 4 si son invariance **ou son plancher**
échoue, tandis que le nominal vacant garde un code distinct.

### `EndpointCredit` : raccord présent, provenance end-to-end encore à graver

Le pin propage `h_a+h_b` vers le pré-scan W4 et les secteurs. La
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
avant le raccord. Les routes scalaires et batch q3/q4 construisent désormais
le token depuis `sc.ha/sc.hb` et le transmettent au prétest puis aux secteurs ;
le second rejeu shaped/corde/secteur/q4-batch porte le sous-ensemble ciblé à
`14/14` en `47,02 s`. Le commit `b8082040` rapporte une suite CPU complète
`303/303` à son pin ; après le pin fonctionnel `fc53472f`, seules les portes
ciblées nommées dans cette passe ont été rejouées.

La fixture croisée fait toutefois encore confiance à
`EndpointCredit.base=1`. Elle ne calcule pas ce crédit via
`corner_histograms`/l'autorité de coins et ne vérifie pas les huit comptes purs
annoncés. Le remplacement unique `o=(10,1020,1024)` rend la provenance réelle :
les rangs Morton de `a/e/i/o/b` valent `2/1/0/4/3`, `NodeRef(2)` porte le range
`[0,2]`, `leaf_ref(3)` le range `[3,3]`, et `wspd_wavefront(s=8)` émet ce
produit. `alive_rectangles(q3,h=2)` le garde avec `core=0`, puis
`corner_histograms` rend `ha=[0,0,1]`, `hb=[0]`. La porte doit consommer ces
objets réellement produits, jamais réinjecter les ranges ou `base=1` après les
avoir seulement assertés. Le cover brut contient aussi `a,b`; après exclusion
des extrémités, les sites sectoriels sont `e,i,o` et donnent les comptes
annoncés. Cette géométrie n'a pas de carrier aigu : elle reçoit le fate et les
compteurs du prétest sectoriel, pas une divergence de sortie finale. Un carrier
aigu constitue une fixture séparée si cette dernière portée est souhaitée.
La précondition `base<hh` doit aussi être encodée ou documentée : les helpers
ignorent un appel direct `base>=hh`; la route produit l'évite seulement parce
que la porte d'histogramme a déjà tué l'ancre.

Les parités batch ne comparent pas encore tout `GenerateStats`. Les builders
ne remplissent pas `hist_killed_rows`, `hist_killed_thresh` et
`hist_survivors`, et leurs gates ne comparent pas ces champs. Partager
l'énumérateur ou ajouter ces trois comptes, puis exiger
`rows+thresh+survivors=anchors` ferme le ledger plutôt que le seul aval.

Surtout, le token vaut toujours seulement `h_a+h_b`. `ar.core` sert au seuil
de la porte histogramme, puis n'est pas transmis aux secteurs. Le chemin reste
fail-open, mais ne réalise pas encore `h_core+h_a+h_b`. La fermeture propre est
bornée : appeler une fois `collect_universal_ids` par `AliveRect`, avec
`cap=ar.core`, et recertifier des **indices `upos` uniques** hors `A union B` —
ce helper ne retourne pas des `PointId`. Si `r_core<=h_core` indices sont
retrouvés et exclus du résidu, le minorant sûr et graduel devient
`h_a+h_b+r_core+max(h_core-r_core,h_c_residual)`. Son analogue sectoriel est
`max(cnt[k],h_a+h_b+r_core+max(h_core-r_core,cnt_res_after_Ucore[k]))`, où
`U_core` est la petite liste recertifiée et non la fibre de carriers `C`.
`r_core=0` donne le nouveau fallback sûr avec cœur scalaire et
`r_core=h_core` la somme totalement disjointe ; le produit courant, lui, perd
encore `h_core` après la porte histogramme. Une recertification partielle ne
doit donc pas jeter tout le crédit. Cette même provenance prépare le tape
résiduel de `h_c`. La précondition produit de positions distinctes reste
obligatoire.

### `CREDIT_SECTEUR.md` ferme le lemme, pas encore le coût

La nouvelle note de Claude apporte une clarification juste : au stage exact
`W_q`, chaque témoin du crédit est déjà dans le compte complet, donc la forme
résiduelle peut seulement anticiper la même mort ; au stage sectoriel, le
polygone suffisant peut ne pas recompter ce témoin universel dans chaque
secteur, et le minorant `max(cnt[k],cnt_out[k]+base)` devient utile. Cette
séparation mathématique doit être conservée.

La réponse de Claude reçoit aussi honnêtement le second défaut de corde. Au pin
fonctionnel `fc53472f`, les routes scalaire et shaped constatent la mort après
la mise à jour positive et avant le `continue`, avec la priorité historique
préservée. Le vrai `q4_kernels.cuh` forme désormais lui aussi `Bz/my_piece`
avant de brancher sur `P>0`. Le titre « verrou de corde fermé » reste néanmoins
prématuré jusqu'à la fixture exacte cinq points sur les routes scalaire/shaped
et au rejeu CUDA : le kernel n'a été ni compilé ni exécuté dans cet
environnement.

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

### Canopée q4 : familles indépendantes, pas encore d'expérience causale

`9f504e52` répond utilement à deux objections : la sonde transmet maintenant
le même `EndpointCredit` au prétest et à `process_anchor_q4`, et
`--bump-amp-cap` sépare l'amplitude des six bosses du saut rare de canopée. Le
plan expérimental naturel devient donc un factoriel `2 x 2` — bosses
nominales/bornées, saut nominal/borné — aux mêmes tailles et graines. Cela peut
dire quelle anisotropie porte le coût q4 ; cela ne reçoit toujours pas une
paire causale.

Deux détails du patch doivent être corrigés avant ce factoriel. Pour
`bump_amp_cap=1`, `amp_hi=max(2,cap)` vaut 2 : le prétendu cap est dépassé. Et
le commentaire « de plus en plus volumique » ne découle pas de la croissance
du relief ou du nombre d'altitudes : le nuage reste un graphe de hauteur 2D,
avec jitter et 2 % d'outliers. Dire « dilation non stationnaire de reliefs
macroscopiques » est plus précis qu'« anisotropie croissante » lorsque rayons
et amplitudes grandissent ensemble ; les valeurs `137 -> 284` doivent être
retirées du contrat source ou épinglées dans un reçu.

En effet, les deux plafonds changent directement les bornes de
`uniform_int_distribution`. Le tirage de hauteur est intercalé avec ceux des
centres et rayons des bosses ; selon les rejets internes, modifier sa borne peut
décaler toute la suite du MT. Le plafond de canopée peut à son tour modifier la
consommation du générateur, puis les collisions changent la boucle
d'acceptation et les `PointId`. Le bon objet de mesure est un tape latent tiré
**une fois** : paramètres bruts des bosses, `(x,y)`, jitter, événement de
canopée et lift brut. Les quatre bras dérivent leurs `z` de ce même tape par
écrêtage, gardent le même lineage et la même cardinalité, puis la paire entière
est refusée si un écrêtage crée une collision. Les digests du tape, du nuage
ordonné et du lineage rendent cette causalité vérifiable.

Les deux options doivent aussi refuser un cap négatif ou supérieur au plafond
nominal et imprimer la graine effective `uint32`. `canopy_lift_cap` est réservé
à `terrain` ; la loi de hauteur actuellement nommée `bump_amp_cap` est supportée
sur `terrain` et les deux familles `scanline_*`, et doit être refusée ailleurs.
`38fa88af --entrees-differentes` évite honnêtement de comparer comme un objet
les compteurs de nuages distincts ; il ne restaure aucun appariement. Le reçu
doit ranger ces hashes comme `signatures_compteurs`, pas comme catalogue ou
forêt, et publier les deltas seulement entre bras issus du même tape.

Garder ce générateur latent dans `bench/` évite aussi d'élargir l'API nominale
de `src/cloud/families.hpp`. Si l'option reste dans `src/`, une porte doit au
minimum établir `cap0 == famille nominale` bit pour bit et toute CLI produit ou
de conformité doit la refuser.

Enfin, `mhgp5_q4_stage_probe` est construit par `mhgp5_executable`, donc avec
`MHGP5_TESTING=1`, alors que `recu_local.sh` l'annonce encore comme « cible
PRODUIT ». Il contourne aussi `run_pipeline`, le census complet et la forêt :
ses trois identités ne sont que des partitions comptables internes. Cette
sonde doit être déclarée explicitement `probe`, ou être recompilée par la
fabrique produit appropriée ; dans les deux cas son verdict reste diagnostic.
Son `total` commence après l'index et `alive_rectangles`, le temps du wrapper
inclut ces étapes sans devenir un temps produit complet, et `rss=0` signifie
« mesure absente », jamais mémoire nulle. Elle doit refuser
`input_count!=n || unique_count!=n` et publier demandé/réel.

La garde de worktree doit inclure tout `morsehgp3D_v5/bench/`, être répétée
après le build et à la fin, et épingler les hashes des sources compilées. La
campagne `canopee_q4` illustre exactement cette course : elle est terminale,
mais son binaire ancien ne reçoit pas le commit qui la publie. La campagne
`terrain_deux_echelles` emploie la sonde de `9f504e52` et est aussi terminale :
27/27 codes zéro et identités internes fermées. Son statut sémantique reste
`diagnostic_unpaired`, `public_status=not_claimed` ; elle ne reçoit ni une
causalité ni un exposant. Elle joue seulement `nominal`, `canopée bornée` et
`les deux bornées` : le bras `bosses bornées seules` manque. Ce n'est donc pas
encore le factoriel `2 x 2` qui sépare les deux effets principaux de leur
interaction.

Le couplage est en outre dissymétrique sur cette exécution. En rejouant
`terrain_cloud`, les `(x,y)` au même rang du bras nominal et du bras canopée
ne coïncident, pour la graine 3, que sur `2407/8000`, `10135/16000` et
`2412/32000` positions : changer la borne du tirage perturbe donc justement la
graine anormale. Le passage canopée vers les deux plafonds conserve ici les
`(x,y)`, centres et rayons, mais `bump_amp_cap=30` **retire une nouvelle loi**
uniforme `[15,30]` ; il n'écrête pas les amplitudes nominales. Sans tape ni
lineage gravé, cela reste un contraste de lois observé, pas une paire causale
permanente.

La note commitée dépasse déjà cette autorité. « Le mur n'est pas algorithmique
», « q4 redevient linéaire » et « l'écart venait de la hauteur » sont des
conclusions causales ou asymptotiques tirées de trois tailles et de familles
différentes. Le résultat positif défendable est plus étroit : dans ces cohortes,
borner les deux échelles est **associé** à des compteurs presque proportionnels
à `n`. Cela ne prouve ni une pente future, ni le coût complet de q4, ni que la
famille bornée modélise un LiDAR réel. Une épaisseur « bornée par la physique »
doit être mesurée sur le corpus cible, pas postulée depuis ce générateur.

L'explication géométrique doit elle aussi nommer le bon objet. Le cover q4
historique vérifie `|2z-a-b|^2<=3D^2`, ce n'est pas la boule diamétrale. Dans le
plan horizontal `z=z0`, son rayon exact vaut
`r3^2=(3D^2-(2z0-z_a-z_b)^2)/4`. Pour `z_a=z0=0`, `z_b=H` et séparation
horizontale `d`, on obtient `r3^2=3d^2/4+H^2/2`, pas `H^2/4`. Le prétest
diamétral de coefficient 1 annule au contraire le terme `H`, et la lentille qui
porte les seeds est un troisième domaine. Le modèle publié peut donc expliquer
une masse offerte au scan du cœur, mais ne prédit pas seul les seeds, les
complétions ou le temps. La mesure utile stratifie par ancre `(abs(dz),d_xy)`
les tailles de cover et lentille, seeds aigus, tests de cœur et complétions.
L'aire de tranche continue `pi*H*min(t,H-t)` n'implique `Theta(H)` que pour une
épaisseur `t=Theta(1)>0` et une densité bidimensionnelle effectivement présente
dans la tranche ; un graphe de hauteur discret ne fournit pas cette densité.

La « seconde anisotropie jumelle » demande aussi une correction. Dans chaque
bosse, rayon **et** amplitude croissent comme `coord`; leur rapport et leur
pente restent donc auto-similaires. Figer seulement l'amplitude à 30 tandis que
le rayon continue de croître aplatit progressivement la famille : c'est un
contre-régime utile, pas la suppression causale d'une anisotropie déjà isolée.

Enfin, la table de la note mélange compteur et chronométrie. Les exposants
`1,49--2,34` de la ligne « covers » se reproduisent depuis le champ temporel
interne `profil ... covers=<ms>`, pas depuis `q4_covers_built`, dont les pentes
sont proches de 1. Ce temps provient d'une machine partagée, sans alternance,
et le reçu interdit lui-même une conclusion fine. La phrase q3
`1,89--1,96, sous 2` ne décrit que le premier doublement : sur le second, les
trois graines donnent environ `2,045`, `2,093` et `2,191`. Les colonnes doivent
nommer l'unité exacte et publier les deux pas, y compris lorsqu'ils contredisent
le récit.

### Extension scanline : bonne contre-famille, même prudence

`fc53472f` propage `bump_amp_cap` aux cinq calottes **et** aux quatre plateaux
de `ScanlineField`; le nom ne décrit donc plus ce qu'il contrôle. La voie
nominale `cap=0` reste inchangée, mais la fixture des douze digests ne teste que
cette inertie par défaut, pas le nouveau paramètre.

Le claim « même anisotropie de hauteur » est mal posé. Dans le champ nominal,
rayons ou largeurs horizontales **et** hauteurs croissent comme `coord` : la
géométrie normalisée reste approximativement autosimilaire. Tirer ensuite les
hauteurs dans `[15,30]` tout en laissant les supports horizontaux croître
aplatit progressivement le champ. Ce contraste peut localiser une sensibilité
de q4 au relief vertical ; il ne montre ni un bug de famille ni le coût d'un
processus LiDAR stationnaire.

La campagne terminale désormais versionnée ne joue que
`scanline_single_pass`, nominal contre nouvelle loi, une répétition et sans
alternance. Un replay du
MT/libstdc++ courant
retrouve toutefois un vrai couplage latent pour les neuf paires : même état
après `ScanlineField`, mêmes centres/rayons/plateaux, trous, jitter et `(x,y)`
rang par rang ; seule la hauteur est remappée par quantile de la loi nominale
vers `[15,30]`. C'est plus fort que des cohortes indépendantes, mais ni un
clamp ni un contrat durable : le reçu ne grave aucun tape, lineage ou digest de
ce couplage et l'API de distribution ne le garantit pas abstraitement. Pour
`scanline_overlap_multiecho`, le lift des échos continue en plus jusqu'à
`coord/10`, donc une seconde échelle verticale reste active. Une attribution
propre exige de graver le tape et, pour l'overlap, des bras séparés
champ/échos.

Enfin, la sonde accepte encore le paramètre via `atoi`, l'ignore silencieusement
sur certaines familles et le reçu l'appelle cible produit malgré
`MHGP5_TESTING=1`. Employer un nom de loi explicite, parser/refuser les valeurs
hors contrat et ajouter une fixture positive par famille avant tout nouveau
claim. Le résultat défendable restera une association de compteurs sur des
contre-familles, pas une preuve que « les deux seules familles non linéaires »
ont une cause commune.

La section 6 versionnée dans `a78d0338` dépasse donc le reçu lorsqu'elle conclut
« la même anisotropie » et « imputable ». Le nominal dilate un nombre fixe de
reliefs macroscopiques de façon approximativement autosimilaire ; la loi
`[15,30]` les aplatit relativement. Le fait reçu est seulement que les trois
exposants sécants 8k→32k de `tests_cœur` deviennent inférieurs à un dans les
cohortes bornées observées. `overlap_multiecho` n'a pas été joué et conserve un
lift d'écho allant jusqu'à `coord/10`.

Les compteurs terminaux localisent néanmoins bien le prochain travail. Entre
8k et 32k, les exposants sécants de `tests_cœur` sur les trois graines valent
`1,712--2,077` au nominal contre `0,710--0,830` sous la nouvelle loi. Cela
décrit trois tailles, pas une complexité sublinéaire. À `n=32000`,
nominal/borné donne un ratio de seeds de `4,70/5,38/4,08` et un ratio de
`tests_cœur` de `15,42/18,75/8,36`, tandis que les nombres de covers ne
diffèrent que de
`1,12/1,13/1,06` et les candidats émis de `1,05/0,98/0,79`. Le résidu n'est
donc pas le nombre de rectangles : il se forme dans la masse par cover, les
seeds et le scan de fibre. Les `45,58/50,69/26,43` itérations par seed énuméré
au nominal, contre `13,89/14,54/12,91`, mélangent encore entrée après grille,
taille de tape et arrêt précoce ; ajouter `core_entered` et l'histogramme
conditionnel avant d'appeler cela une longueur de préfixe. Ce diagnostic
renforce la priorité `h_core/h_a/h_b -> h_c -> shallow global`, pas une
réécriture du front WSPD.

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

## P1 — stratégie sortie-sensible : plan conditionnel reçu, borne ouverte

`STRATEGIE_SOUS_QUADRATIQUE_Q3_Q4_20260830.md` fournit une direction utile :
front WSPD exact dans sa sémantique de séparation/ownership à `s=8`, mais avec
borne linéaire et charging du quotient encore ouverts ; requêtes saturées de
facteurs, fermeture hiérarchique par `h_c`, puis sous-complexe de faibles
profondeurs pour les ancres lourdes. Elle sépare correctement les gains de
constante, la sortie et les termes d'incidence. Elle ne résout pas le P0 Gamma
et reste attachée au sous-flot horizontal `verified_events_only`.

La contre-revue a imposé quatre corrections dans le document : le grand-livre
paie explicitement chaque ancre résiduelle `|E|`; la profondeur historique
`delta_e^(3)` sur `cover3` reste distincte de `delta_e^full`; les concurrences
et incidences de droites ont leur propre terme ; et `h_c` ne ferme un nœud de
carriers qu'avec un seuil uniforme prouvé sans visiter ses feuilles. Le
coarsening du front est borné à `postsep_refine_levels=0`.

Le choix Morton recommandé pour le premier incrément est de réutiliser le cube
`Q` et de contracter ses au plus sept préfixes internes dans un quotient octree
indépendant. La multiplicité locale sept ne prouve pas le facteur heuristique
`7*7` : il manque encore connexité des microarbres, feuilles singleton,
fanout huit, simulation du split/tie-break et injection terminale exact-once.
La cible reste `R_shadow=O(R_oct)=O(m)`, sans constante numérique reçue. Le
charging propre à `s=8` et le nouveau ledger global de coupe
`R_cut=rectangles_emis+rectangles_tues_core` doivent entrer dans une porte avant
tout claim `R_cut=O(m)` ; même `R_cut<=R_shadow` reste conditionnel à cette
projection.

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

La nouvelle contre-famille ferme une ambiguïté importante : même une WSPD
linéaire ne borne ni la masse des handles de rectangles, ni la somme des tapes
par ancre. `rect_cover_handles` contient toujours `A union B` aux coefficients
produit 3/4 ; la construction candidate `x_i=L^i` donne une masse quadratique
si la fixture reçoit ses facteurs singletons et le ledger exact-once, dans le
modèle à précision croissante. Plus directement, une ancre singleton `a`, une
calotte de `k` extrémités de même rayon et `M` sites obliques situés dans chaque
lentille mais hors `W2` font survivre `h_core/h_a/h_b` tout en imposant
`sum_e m_e>=k*M`. Les marges et la réalisation sont détaillées dans la
stratégie. Sous u16, ces familles sont des contre-fixtures finies dépendantes
de la profondeur, pas des asymptotiques infinies. Le grand-livre doit désormais
publier `H_rect`, la vraie répétition
`H_scan=sum_R anchors_surv(R)*handle_mass(R)` et `M_anchor`; `R=O(n)` ne peut
plus les masquer.

Il existe en plus un verrou que ni `h_core`, ni `h_a/h_b`, ni `h_c` ne peut
éliminer pour un catalogue explicite : une famille réellement quadratique de
boules critiques. La construction liée
d'Edelsbrunner--Pach possède, pour `N=2n+2`, exactement `2n(n+1)` triangles
critiques vides et `n^2` tétraèdres critiques vides ; ce sont respectivement
des clés q3 et q4 distinctes de profondeur zéro. La source primaire est
[Maximum Betti Numbers of Čech Complexes](https://pub.ista.ac.at/~edels/Papers/2024-01-MaxBettiCech.pdf),
§ 3.1 et lemme 3.5. L'owner WSPD choisit qui émet chaque clé, il ne réduit pas
leur nombre. Le théorème porte sur la géométrie exacte ; la portée produit v5
reste conditionnée par la future porte oracle+pipeline. Si cette porte émet les
événements attendus, le fold dense courant en interne toutes leurs facettes et
conserve `facet_keys` puis `final_canon_fid`, donc la barrière atteint aussi ce
payload `verified_events`. Elle ne s'étend pas automatiquement à un futur
payload implicite/reconstructible, au fold vivant non raccordé ou à Gamma.

Cette géométrie possède déjà un replay entier préparatoire dans le profil : la
fixture proposée `linked_arcs_u16`, de tailles `N=6,10,18,34`, donne exactement
`12/40/144/544` clés q3 et `4/16/64/256` clés q4 après RLE. À `N=34`, toutes
les marges d'acuité, de centre intérieur et de vide sont strictes. La stratégie
porte les coordonnées littérales et les marges. Il faut maintenant graver ce
replay contre l'oracle exact **et** la route produit, avec profondeur zéro,
coquille égale au support et owner exact-once. L'oracle géométrique et le ledger
produit sont deux preuves distinctes. `92605016` versionne ces constats et les
résultats de probes jetables indépendants : une énumération géométrique OBig et
des appels séparés de la route produit reproduisent les comptes, l'exact-once
pré-RLE, le census et l'équivariance attendue. Ce ne sont toujours ni un oracle
exécutable versionné ni une porte permanente ; « receive linked arcs » ne ferme
donc pas encore ce point. Le domaine u16 fixe ne fournit pas une
asymptotique infinie : cette série montre seulement un ratio sortie/`N^2`
constant jusqu'à 34 points. La borne `Omega(N^2)` appartient à la famille
exacte à précision croissante. Elle suffit à imposer un contrat sortie-sensible
à toute généralisation de précision qui matérialise ce catalogue ou cette
forêt explicite ; une représentation implicite/reconstructible est un autre
contrat. Les événements seuls imposent déjà au payload forestier courant au
minimum `(n+1)^2+2n=N^2/4+N-2` facettes K=2 et
`2n(n+1)=N^2/2-N` facettes K=3, soit `3N^2/4-2` clés au total, car
`ForestResult` conserve aussi un `final_canon_fid` par facette. Pour u16, cette
cible reste une doctrine prudente et falsifiable, pas un théorème asymptotique
inventé.

Le ledger de sortie sépare en outre `B`, nombre de `BallKey`, de
`S_shell=sum_key |U_B|`, masse des coquilles complètes. Dans le profil courant
`complete_regular`, une sortie acceptée impose `shell_cap<=12`, donc
`S_shell<=12B` ; une coquille `Theta(n)` concerne une généralisation non
plafonnée ou provoque ici un refus transactionnel `resource_exhausted`.
L'expansion des supports est encore un troisième contrat. Le constructeur
shallow doit grouper les concurrences exactes et ne jamais employer de
perturbation symbolique, qui changerait coquille et intérieur.

Le sweep de `h_c` est également précisé. Sur toute la corde du carrier, chaque
site donne le signe affine `P-mu*B`; un sweep groupé aux racines, extrémités et
frontières rend le minimum exact **relativement au tape**. Sa composition avec
les autres crédits reste un minorant de la profondeur totale. Pour `r_core` indices
`upos` cœur recertifiés et retirés du tape résiduel, employer
`h_a+h_b+r_core+max(h_core-r_core,h_c_residual)` ; les témoins non universels des facteurs
sont abandonnés. Sans aucune provenance, `r_core=0` redonne
`h_a+h_b+max(h_core,h_c_ext)`. Le `h_c` d'un carrier
retire uniquement son rôle d'appariement ; le point reste témoin, conflit,
census et éventuelle complétion.

Pour la V1, le contrat le plus petit est complete-or-none : sous positions
distinctes, la collecte doit retrouver exactement `h_core`; sinon elle signale
un invariant et revient au maximum sûr sans exclure d'ID. `h_c` et
`PlanConflictGrid::base_C` lisent le même tape, donc leur combinaison
résiduelle est `max(h_c,base_C)`, jamais une somme. Les tokens et grilles sont
tagués par lane, rectangle/ancre et `tape_id`.

Ne pas fusionner les deux contrats de lane : q3 demande le compte résiduel au
centre ponctuel de `abc`, tandis que q4 demande le minimum uniforme sur toute
la corde des centres possibles avant de fermer la fibre contre tous les `D`.
Les champs et compteurs doivent donc distinguer `h_c_q3_point` de
`h_c_q4_chord`. Ce dernier dit seulement si la corde entière est morte ; une
corde survivante peut alterner entre plusieurs intervalles profonds et shallow.
La V1 rasterise toute la corde, ou le sweep doit retourner **tous** les
fragments shallow exacts avec leurs frontières.

Enfin, une grille dynamique n'est pas une preuve. En q3, le coût exact contient
`K_conf=sum_C q_C*c_C` sur les centres uniques ; le modèle
`G≈sqrt(k_unique)` suppose une corrélation bornée entre requêtes et conflits,
que des concentrations adaptées au maillage réfutent. En q4, avec `r_core`
indices cœur recertifiés et retirés du tape avec `A union B`, le minorant d'un
seed `c` dans une cellule est
`h_a+h_b+r_core+max(h_core-r_core,h_c(c),base_C)`. La forme soustractive
`base_C<h4-(h_core+h_a+h_b)` n'est valide que pour `r_core=h_core`, après une
garde signée contre l'underflow ; l'employer avec un cœur inconnu le compterait
deux fois.

Le budget q4 doit aussi respecter les rôles. Un tétraèdre bien centré garantit
au moins une face `abv` aiguë, pas deux. Pour chaque cellule, `U_C` contient les
classes de droites uniques éligibles **unairement** comme complétions pour
l'ancre — source, lentille, non-axialité et intersection de la droite clippée
avec la cellule — avant distance `|c-d|`, owner, exact-once ou bien-centrage,
qui dépendent du couple. `A_C subset U_C` contient celles portant un seed aigu
survivant. Le bon preflight est
`P_role=sum_C[binom(|U_C|,2)-binom(|U_C minus A_C|,2)]`, pas
`sum_C binom(|A_C|,2)`. `h_c` retire une ligne de `A_C`, jamais de `U_C`.
Cette masse peut rester quadratique. La validation des provenances facture le
produit orienté sur classes distinctes
`sum_C sum_{L != M}|SeedProv[L,C]|*|CompletionProv[M,C]|` avant quotient
exact-once ; `L=M` a déterminant nul. Les shadows publient les incidences seed
et complétion séparément, puis tombent atomiquement sur le fallback sans aucune
émission partielle.

Une voie de repli plus courte existe déjà dans le code et doit être mesurée
avant d'implémenter tout l'arrangement : `pipeline/census.hpp` fournit
`ball_depth_at_least`, prédicat global exact de seuil par `BallKey`, bornes de
boîte et arrêt au seuil. Son booléen est exact ; lorsque le seuil est atteint,
le `count` retourné est écrêté par l'arrêt précoce et ne doit pas être publié
comme profondeur exacte. Le générateur q3 scanne actuellement le cover par seed, puis le
pipeline répète une requête globale après RLE ; q4 ne possède au générateur
qu'un minorant `cover3`, avant la même décision globale. Un shadow peut donc
remplacer le rescan d'un support survivant par cette requête reçue, comparer les
fates et publier visites de nœuds/feuilles contre tests de sites. Il neutralise
la contre-famille `ancre x témoins` pour le **coût de décision**, mais ne réduit
ni les ancres, ni les supports proposés, ni les `BallKey` à sortir. La garantie
reste sortie-sensible et le radix peut visiter `Theta(n)` nœuds par requête :
aucun exposant sous-quadratique ne découle de ce seul raccord. Un prototype
streamé peut trier chaque chunk en runs, mais ne décide encore rien : une
fusion/RLE **globale** doit d'abord réunir une clé présente dans plusieurs
chunks, retrouver son arité minimale et son représentant canonique, puis
appeler une seule fois la requête. Décider par chunk serait faux si deux
occurrences de la même clé portent des arités différentes. Tout repli reste
transactionnel et précède l'émission.

Le claim global reste NO-GO tant que `R=O(n)`, le constructeur exact des
niveaux peu profonds et chaque terme du grand-livre ne sont pas prouvés. Les
cinq tailles et trois graines peuvent réfuter ou borner une classe annoncée ;
elles ne remplacent aucune de ces preuves.

## Ordre de travail conseillé à Claude

1. Conserver le plancher `20000` du replay shaped, graver en plus la fixture
   exacte cinq points, puis rejouer le kernel ; garder CUDA non reçu tant que
   `nvcc` n'a pas passé la porte.
2. Graver dans une fixture produit non vacue la dérivation déjà raccordée
   `corner_histograms -> EndpointCredit -> secteurs`, avec `o.z=1024`, puis
   ajouter les indices `upos` bornés du cœur au crédit résiduel, recevoir
   l'héritage postsep et fermer les trois compteurs `hist_*` des routes batch.
3. Graver `linked_arcs_u16` contre un oracle OBig/i128 indépendant et le
   produit : événements q3/q4, facettes K=2/K=3, exact-once pré-RLE,
   permutation physique, réétiquetage équivariant et mutant de troncature i64.
   Accepter alors explicitement le contrat sortie-sensible et interdire tout
   gate de temps universel qui oublierait les sorties demandées.
4. Benchmarker le shadow `ball_depth_at_least` après déduplication bornée des
   clés. Il précède `PlanConflictGrid` : c'est la réutilisation exacte la plus
   courte et elle dira si un arrangement est réellement nécessaire.
5. Recevoir le front `Q` par quotient octree, shadow et ledger global, puis
   seulement raccorder la fusion des lanes et mesurer la mémoire.
6. Ajouter les requêtes `h_a/h_b` `ALL/NONE/MIXED` en shadow contre
   `corner_histograms`, avec garde cardinal et compteurs `V_R+C_R+P_R`.
7. Construire le sweep oracle de `h_c`, puis sa fibre hiérarchique et seulement
   ensuite `PlanConflictGrid` q3/q4 si le shadow global laisse une queue lourde.
8. Fermer le ledger q4 et rendre les nouveaux compteurs non vacants.
9. Refaire le factoriel canopée/bosses à partir d'un tape latent commun ; le
   reçu terminal versionné demeure un diagnostic non apparié.
10. En parallèle documentaire, livrer le refus Gabriel/Gamma
    `require_exact=true` et limiter le claim de parsing à `--s`.

## Vérifications indépendantes de cette passe

- sur les sources `fc53472f`, rebuild ciblé de `mhgp5_families_fixture` et
  `mhgp5_q4_stage_probe`, puis 5/5 CTests verts : `mhgp5_families_fixture`,
  `mhgp5_mutants_gate`, `mhgp5_q4_chord_ordre`, son mutant et
  `mhgp5_q4_stage_uniform` ;
- le commit `b8082040` rapporte une suite CPU complète 303/303 à son pin ; elle
  n'a pas été rejouée intégralement après `fc53472f` et ne reçoit pas CUDA ;
- replay entier préparatoire non reçu `linked_arcs_u16` : comptes
  q3/q4 `12/4`, `40/16`, `144/64`, `544/256`, clés toutes distinctes et
  coquille égale au support ; une énumération OBig indépendante jetable et des
  probes produit séparés les reproduisent, mais la porte versionnée reste à
  construire ;
- `python3 tools/check_docs.py` : 225 fichiers Markdown actifs validés ;
- `python3 tools/check_implementation_status.py` : 20 phases et portes
  validées ;
- le reçu `canopee_q4` est terminal `complete`, 18/18 codes 0, mais reçoit le
  binaire antérieur `38fa88af` et seulement des cohortes indépendantes ;
- `terrain_deux_echelles` est terminal sur la sonde de `9f504e52` : 27/27
  codes zéro, 27/27 identités internes fermées, aucun stderr non vide ; il
  est versionné, mais ses bras ne sont pas tous appariés ;
- `scanline_relief` est terminal sur `fc53472f`, uniquement pour
  `scanline_single_pass` : 18/18 codes zéro et identités internes fermées ; le
  reçu est versionné dans `a78d0338` mais reste diagnostic ;
- CUDA non compilé ni exécuté localement, faute de `nvcc`.

GCP non utilisé.
