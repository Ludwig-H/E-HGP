# État courant audité de MorseHGP3D v5 — 30 août 2026

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur

Dernier pin source relu : `eba24b9a`. Il ferme le header de parsing omis, la
garde CLI de `s>=8` et committe le profiler q4. Les commits ultérieurs
`62959613` et `4c286017` sont des contre-lectures ou stratégies documentaires ;
les autres diagnostics reçus incluent `119b80b0` pour les corrections q4 et
`9940668e` pour la sonde de fusion WSPD.

Au moment du verdict initial, les sources suivies étaient identiques au pin.
Depuis, Claude prépare dans le worktree une correction de la corde q4 dans
`generate.hpp`, `q4_core_shaped.hpp` et le registre des mutants. Cette
proposition non commitée est contre-relue dans l'addendum actif de
`QUESTION_CLAUDE_Q4_APRES_Q3_20260830.md` ; elle ne change pas encore le pin
source reçu. Les autres changements du worktree, notamment documentaires,
restent conservés pour Claude. Les deux faux reçus `cf_*.txt`, qui contenaient
uniquement l'échec d'invocation d'un binaire absent, ont été supprimés pendant
le nettoyage. Toute autre modification source périme la fraîcheur du verdict ;
elle ne périme pas les contre-exemples mathématiques ni les constats sur les
commits nommés.

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

### La corde au pin audité perd des témoins `P>0`

Dans les routes scalaire, shaped et device, un site certifié `P>0` quitte le
scan avant `ChordPieces::update`. C'est correct pour le cœur au centre
`mu=0`, mais pas pour la corde : `P-mu B` peut devenir strictement négatif sur
un morceau extérieur. Le défaut est fail-open ; il ne crée pas de fausse mort,
mais rend `seeds_killed_chord` et les taux historiques ininterprétables comme
mesure du certificat annoncé.

La fixture entière déjà décrite dans
`QUESTION_CLAUDE_Q4_APRES_Q3_20260830.md` ferme ce point avec `P(z_2)>0` et
`B(z_2)<0`. Il faut appeler la mise à jour avant le saut du cœur, porter la
fixture dans le scalaire et le shaped, puis tuer un mutant
`chord-skip-positive`; la parité device vient ensuite.

Le patch actif va dans cette direction, mais son premier état teste encore
`chord.dead(h4)` après le `continue` des sites positifs : la décision dépend
donc de l'ordre si un tel site complète le dernier morceau. Le kernel CUDA
conserve en outre l'ancien saut. Rejoué localement, `mhgp5_mutants_gate` rend le
code 3 : le mutant déclaré n'a aucune porte code 4 ; les portes shaped
désactivent la corde et la parité batch partage le même oubli. La fixture à
sept points et ses deux permutations sont données dans l'addendum de la réponse
Q4, avec le motif de correction qui préserve la priorité du cœur en cas
d'égalité.

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

Le claim global reste NO-GO tant que `R=O(n)`, le constructeur exact des
niveaux peu profonds et chaque terme du grand-livre ne sont pas prouvés. Les
cinq tailles et trois graines peuvent réfuter ou borner une classe annoncée ;
elles ne remplacent aucune de ces preuves.

## Ordre de travail conseillé à Claude

1. Recevoir `eba24b9a` sur CPU et limiter exactement le claim au parsing de
   `--s`; garder CUDA non reçu tant que `nvcc` n'a pas rejoué les portes.
2. Livrer le patch de vérité Gabriel/Gamma et le refus `require_exact`.
3. Réparer la corde q4 tous sites avec fixture et mutant dans les trois routes.
4. Fermer le ledger q4 et rendre les nouveaux compteurs non vacants.
5. Refaire la porte WSPD contre le symbole produit, puis mesurer la mémoire.
6. Après ces portes seulement, choisir entre optimisation q4 et fusion WSPD à
   partir de murs appariés et de reçus bruts.

## Vérifications indépendantes de cette passe

- archive Git propre de `eba24b9a` : configuration et build CPU complets sous
  `-Werror` ; build Release canonique du workspace : succès à 100 % ;
- portes CPU/API `s`, fold, séparation, q4-stage et fusion WSPD : 29/29. Les
  dix sondes s'exécutent, sans réparer les autorités contestées ci-dessus ;
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
