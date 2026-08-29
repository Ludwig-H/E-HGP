# État courant audité de MorseHGP3D v5 — 29 août 2026

- **HEAD fonctionnel relu :** `7bf28488`, publié sur `main` et `origin/main`.
  Le dernier delta du chemin produit reste `2d052921` ; le HEAD épingle
  maintenant le harnais, le contrat du futur center-cover et le probe
  diagnostique `A x B x C` v2, sans pinner le raccord d'enveloppe. Le prédicat
  idéal au seuil est reçu ; ses proxys de travail et ses sorties ne sont ni
  causaux, ni receiptés.
- **Dernier pin du chemin produit reçu :** `72090f79`. Le chemin produit de
  l'enveloppe q3/q4 et ses portes restent dans un worktree concurrent non
  commité. Ils sont jugés ci-dessous comme snapshot, jamais attribués au HEAD ;
  le harnais est désormais épinglé séparément à `66997d56`.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.
- **Frontière :** aucun résultat GPU, aucune promotion de registre, aucun
  claim d'exactitude publique ou de passage à 10–30 M. La v4 reste un
  différentiel borné, jamais une implémentation ni une preuve héritée.

## Verdict utile à Claude

Le raccord d'enveloppe est bon dans son principe et dans son placement CPU :
il conserve le cover historique, compacte paresseusement au premier scan et
réemploie le tampon du counting sort. L'appariement exercé est solide. Il doit
maintenant être épinglé avec ses portes, pas encore optimisé ni mesuré.

La base continue d'éviter la mosaïque de Delaunay d'ordre supérieur. Le filtre
réduit les sites de cœur/profondeur ; il ne retire ni visites de handles, ni
ancres, ni pire exposant q4. Le verrou d'échelle global reste donc ouvert.

La correction de cap de l'utilisateur est reçue : **q2 n'est pas le problème
architectural à traiter**. La WSPD binaire partitionne correctement les paires ;
l'explosion naît lorsque q3/q4 développent chaque produit vivant `A x B` en
ancres, puis les tiers ou les couples de carriers. Une auto-jointure de deux
WSPD globales a été testée puis rejetée : elle recrée des millions
d'interactions dès `n=256`. La WSPD locale d'arête opposée proposée au pin
`5afcfce0` est requalifiée en **ablation q4 conditionnelle** : elle ne traite ni
q3, ni le facteur `|A||B|`, et une partition compacte de couples sans décision
sémantique peut être redondante avec le terminal shallow. Le center-cover de
blocs vient donc en premier ; la route locale ne survit que si elle évite un
travail effectivement exécuté avant ce terminal.

Le reçu `echelle_par_lane_20260829` confirme que les rescans sont un poste
majeur sur `terrain`, mais son addendum sur-interprète le routage. Le seuil
`pretest_query_min_points` porte sur les points de handles d'un rectangle, pas
sur le nombre exact de lignes d'une ancre ; le seuil 60--100 est un croisement
de modèle, pas une mesure du shallow. Les chiffres 3,9 % / 87 % et leur
ventilation ne sont pas présents dans les sorties jointes. L'enveloppe réduit
directement `scan_sites`, et un prune de bloc supprime tous les rescans aval :
center-cover et shallow restent complémentaires.

La v3/v4 proposait déjà un switch statique entre scan et arrangement. Pour le
routage du terminal, l'incrément neuf défendable est un
`adaptive_online_dispatch` : après création de l'`AnchorLineSet`, scanner un
préfixe canonique, compter le travail vraiment exécuté puis acheter le shallow
pour le reliquat lorsque son devis receipté est atteint. q3 porte la première
ablation ; q4 exige de garder les carriers du préfixe comme témoins, de masquer
seulement leur droit d'émission primaire et de fermer le ledger exact-once. Le
mode initial reste shadow/counter-only, sans reroutage produit ; le RLE ne
remplace pas la preuve de partition.

Le contre-audit des notes de Claude a eu un effet concret : la formule q4 est
requalifiée comme sur-ensemble de Jung, le seuil de coût ancien est retiré et
la fusion prématurée dans la collecte des handles est abandonnée. Ces décisions
sont intégrées à la question active ; les deux notes redondantes sont retirées
du tip.

La restriction supplémentaire par un handle $C$ est désormais un **GO
counter-only**. La v2 teste exactement au seuil si tous les supports valides
d'un bloc non capé sont profonds, et sa baseline couvre désormais toutes les
ancres. En revanche `valid_forms * rectangle_candidates` n'est ni le nombre de
rescans exécutés, ni le travail évité : les ratios 99,7/99,5 %, 78,9/76,2 % et
les facteurs de résidu 70/48 sont rétractés, comme la priorité `EMPTY` qui en
était déduite. Les blocs capés lourds sont exclus, les blocs mixtes sont mal
crédités et la chaîne réelle filtre puis s'arrête au seuil.

Le premier incrément sûr est moins cher que le rescan envisagé : calculer une
fois par `(A,B)` les crédits `g_AB[j]` des 64 patches hors `A union B`, puis
laisser chaque `C` masquer seulement les patches dont `AB/AC/BC` peuvent
encore contenir zéro. `g_AB[j]` s'additionne à `h_a(a),h_b(b)` mais n'est pas
le vrai $h_0$ extérieur à `C`; il ne se compose donc pas avec un futur
$h_c(c)$ sans union d'IDs ou repartition. Le test de puissance aux seuls
`8^3` coins reste réfuté par la fixture u16. Un cap, une tangence ou une borne
ambiguë produit `pending`, jamais un prune.

## Enveloppe q3/q4 reçue mathématiquement

- Avec `d=b-a`, `D2=|d|2`, `w=2z-a-b`, `S=|w|2-D2` et
  `Xi=|d×w|2`, q3 emploie le prédicat fermé exact
  `S <= 0 || 3*S*S <= 4*Xi` sous ses préconditions d'ancre aiguë.
- q4 emploie `S <= 0 || S*S <= 2*Xi`, sur-ensemble sûr de Jung pour les
  tétraèdres strictement bien centrés émis par la lane. Il reste intersecté
  avec le cover historique coefficient 3.
- La lentille fermée de l'ancre est incluse dans l'enveloppe q3, elle-même
  incluse dans Jung q4. Seeds et complétions historiques sont donc préservés.
- Les produits sont formés en `i128`; les frontières restent fermées. Le
  `Q_min` par distance aux intervalles est seulement un minimum continu sûr,
  pas le minimum exact du réseau u16 à parité fixée.

La dérivation, les fixtures et la réponse V49–V52 consolidée vivent dans
`QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md`.

## Réorientation WSPD q3/q4

Le contrat actif transmis à Claude est désormais :

- garder la WSPD binaire comme tape extérieur et `origin_rect_id` immuable,
  q2 restant entièrement inchangé ;
- reconnaître que `Q3FiberTask(A,B,C)`, WST3, WST4, `CellPair` et
  `Sym2(G) disjoint_union Cross(G,N)` sont des antériorités v3/v4, pas une
  nouvelle généralisation ;
- rejeter l'auto-jointure de deux WSPD globales comme hot path ;
- appliquer d'abord le center-cover exact du Théorème 5 directement aux blocs
  `A x B`, avec verdicts q3/q4 indépendants et aucune ancre matérialisée ;
- terminaliser seulement les ancres résiduelles par lignes signées, puis
  comparer en ablation q4 un range-report direct à `LocalOppositeEdgeWspd` ;
- séparer `support_lines` de `census_lines` et conserver le range-report global
  nécessaire au rang ;
- tuer seulement par certificat universel exact ; ambiguïté ou capacité
  atteinte conserve le parent et sa continuation.

Le no-go des blocs ternaires symétriques fortement séparés reste valide. La
WSPD locale partitionne les couples de cellules en `O(k_r)` blocs seulement
sous grille commune ou octree 2:1 et paramètres fixes ; cette borne ne prouve
aucun prune. Elle ne borne ni les splits `MIXED` de `A x B`, ni les visites du
center-cover, ni la somme des lignes de census. En attendant leur réception,
la frontière directe reste l'oracle q3/q4 borné.

Il ne ferme pas la fibre asymétrique `WSPDRect x Handle`, où $C$ n'est pas
séparé. Le couple `(rectangle,handle)` partitionne les rôles paire--tiers du
cover ; acuité, identités et owner restent à décider. La masse retire les
diagonales lorsque $C$ recouvre $A$ ou $B$, et le complément du cover reçoit
le fate `DEAD_OUTSIDE_WINDOW`. Le join des histogrammes est borné par le seuil
neuf seulement sur un domaine cartésien ; acuité et owner couplent sinon les
rôles. Leur calcul courant reste `O(|A|^2+|B|^2)`.

Une ablation distincte `Lca3Forest` possède un ledger exact
`sum |A||B||C| = C(n,3)` et au plus `48(n-1)` blocs littéraux sous le radix
Morton48 actuel. Elle n'est pas une WSPD ternaire : sa paire LCA n'est pas
l'arête maximale, donc ni le spindle ni le cover owner courant ne peuvent y
être réutilisés. Elle reste un comparateur combinatoire après la fibre mesurée,
pas une nouvelle route produit.

La borne honnête cible
`O(n log n + R + K log n + C + I + A + V + sum_e(m_e log m_e) + h*M + Z)`.
`R,K,C,I,A,V,E,M,Z` sont des compteurs obligatoires ; `Z` compte les sommets de
niveaux proposés, pas toutes les représentations de supports d'une même boule.
Les masses de rôles `3*C(n_u,3)` et `6*C(n_u,4)` ont leur ledger séparé. Une
borne supérieure à 95 % de pente reste un falsificateur empirique, jamais une
preuve d'exposant. Le linéaire demeure un objectif conditionnel, pas un claim.

Le premier incrément demandé n'est pas un reroutage produit : un probe
`q34_fiber` counter-only exécute un parcours témoin par rectangle, réutilise
ses crédits sur les handles, ferme les masses de positions et de rôles, exige
`anchors_materialized=0` et rejoue chaque prune à petit `n`. Ensuite seulement
vient `AnchorLineSet`, puis l'ablation de WSPD locale q4. Les seuils à
`smax=11` restent neuf intérieurs pour tuer q3 et huit pour tuer q4. La note
active détaille coefficients homogènes, tangences, concurrences, digest,
portes de coût, fixtures et mutants.

Ce probe a maintenant un contrat d'implémentation borné : deux grilles
entières distinctes de 64 patches à l'échelle 32, sans flottant ; parcours
partagé par deux masques mais antichaînes locales ; scission obligatoire d'un
nœud témoin contenant `A` ou `B` ; aucun cumul avec `AliveRect::core`, aucun
héritage de crédits après split. Le profil refuse les positions dupliquées :
le ledger sémantique compte les positions, jamais `node_weight`. Le lemme q3
analogue au Théorème 5 doit être gravé avant tout prune q3 autoritaire.

Claude a répondu au pin `ac43ab1a` avec deux filtres q3 par groupes. Le lemme
du tiers aigu est reçu après ajout de l'owner `EdgeKey`; l'optimalité du cœur
est limitée à la boule concentrique d'une ancre ponctuelle. L'escalier
d'histogramme et les rejets de handles passent d'abord en counter-only : ils
ne sont ni gratuits, ni une preuve que seules deux boucles restent. Le rejet
non aigu sûr emploie la forme couplée `hmin_boxes(A,B,C) >= 0`, dans une vue de
seeds séparée qui ne retire jamais ces points du cover témoin. La réponse
V53--V56 est fusionnée dans la note active ; la question séparée est retirée
du tip après migration, son commit restant dans l'historique.

## Réception du snapshot d'enveloppe

### Fermé dans le worktree observé

- build Release complet avec `-Wall -Wextra -Wpedantic -Werror` ;
- registre `80/80/80`, Python requis et détection des portes CMake
  multiligne ;
- appariement OFF/ON sur six familles : ordre brut à un fil, catalogue RLE,
  digests, événements avec niveaux, `batch_levels` et cardinalités par K ;
- routes de prétest cover/requête, compteurs séparés et compaction q3/q4 non
  vacante ;
- fixtures strictes non axiales, frontière `i128`, point Jung q4 extérieur à
  q3 et oracle indépendant par produit vectoriel ;
- chemins batched normaux, tout hôte, mixtes et surdimensionnés q3/q4 ;
- réemploi de `cover_tmp`, remapping stable de la lentille q4, garde u32 avant
  matérialisation, autorité unique de `pretest_query_min_points`, parsing CLI
  exact et sonde compilée comme cible produit.

### Dents avant pin et mesure

1. **Pinner le delta complet.** Source, fixtures, CMake et statut doivent
   entrer dans le même commit cohérent, puis être reconstruits et rejoués sur
   ce pin.
2. **Rendre « oversized » causal.** Les exécutions auditées empruntent la route
   (`3657` ancres q3, `2961` q4), mais `expect-route=device` n'exige pas
   `anchors_oversized > 0`. Ajouter un plancher explicite.
3. **Déclarer les capacités d'override.** Une option imprimée active ne peut
   être ignorée silencieusement par un exécuteur externe ; propager ou refuser
   la combinaison.
4. **Finir le harnais de reçu.** `66997d56` pinne le protocole, refuse
   l'écrasement, force les vrais digests, compare les bras, grave le statut et
   l'environnement, et son auto-fixture nominale passe `5/5`. Le handler
   `INT/TERM` écrit un statut s'il manque, mais ne quitte pas explicitement le
   script. Sous `setsid`, un `TERM` pendant le premier run a laissé exécuter les
   quatre runs : codes `143,0,0,0`, sortie finale 3 et `statut=failed`, jamais
   `interrompu`. Séparer `on_signal`, sortir en 130/143, attendre ou tuer le
   descendant ciblé et vérifier qu'aucun ne survit.
5. **Comparer l'objet, pas les métadonnées.** La signature conserve toute la
   ligne `famille=`, donc `--threads=1` contre `--threads=2` produit un faux
   `DESACCORD` code 3 alors que les digests et comptes sémantiques sont
   identiques. Hasher seulement les digests et cardinalités, puis ajouter ce
   bras multithread au contrôle positif. Le selftest doit aussi vérifier les
   champs du reçu et la cause : son cas `--smax=99` produit déjà un objet vide,
   donc le code 3 ne tue pas isolément la garde `runs_non_nuls`. Enfin écrire
   `ordre_joue` ; avec trois bras le reçu annonce actuellement AB/BA pour un
   ordre ABC/CBA. Reconfigurer aussi avec un cache frais et hasher les options
   ou le `CMakeCache.txt` : le répertoire `build/recu_$nom` peut actuellement
   préexister alors que le reçu ne grave que compilateur et `Release`. Exposer
   séparément q3/q4 avant `none/q3/q4/both` reste bloqué par le booléen global
   du raccord.
6. **Réparer le budget de la porte post-séparation.** Dans la campagne à deux
   workers, `mhgp5_postsep_refine_mutant_h1` expire à `300,10 s` alors que la
   porte nominale jumelle finit en `302,74 s`. Le rejeu isolé est vert en
   `153,94 s` avec le code 4 attendu : le mutant est bien tué, mais le timeout
   de 300 s ne supporte pas la concurrence de la campagne canonique.
7. **Réparer les normes du brouillon mathématique.** Le worktree observé écrit
   trois fois `leftVert` sans antislash dans la nouvelle dérivation
   d'enveloppe (`D`, `S`, puis `Xi`) ; `rightVert` reste seul. Le rendu KaTeX
   est invalide alors que `python tools/check_docs.py` reste vert, car ce motif
   amputé n'est pas encore contrôlé. Corriger le Markdown avant pin et ajouter
   ce cas au validateur.

Le filtre reste OFF par défaut. Aucun tableau de mur antérieur au refactor ne
sert de reçu. Mesurer ensuite `none/q3/q4/both` exige d'abord des commutateurs
internes par lane, car l'API courante ne possède qu'un booléen global.

## Autres coutures actives

### G0 — confinement du pool

1. Incrémenter `submitted_` seulement après admission réussie dans la file.
2. Garantir un `exception_ptr` fatal non nul sans allocation dans le fallback.
3. Remplacer les scénarios `sleep_for` par des barrières causales.
4. Relier une exception CUDA typée à `close_fatal` avant toute nouvelle prise
   de lot ; `submit_and_wait` seul ne poisonne pas le pool.

La fermeture hôte explicite est utile. Le confinement général d'une erreur
device n'est pas reçu.

### Fold vivant L2

- borner `x` avant `av[x]`, puis `fid` avant `slot_of_fid`, et parcourir toute
  la table pour détecter une entrée stale ;
- ajouter un mutant de partition à cardinalité conservée et une porte de
  capacité causalement autonome ;
- graver les deltas et `batch_levels` littéraux, niveau compris ;
- ajouter seulement `born_at/died_at` et `batch_levels` au modèle de capacité,
  les autres postes étant déjà comptés ;
- garder le bras sans rejeu comme ablation et rendre le miroir avec rejeu
  strict sur T5.

Le contre-exemple T5 et la borne de wire sont migrés dans `../docs/ECHELLE.md`.
À 10 M, le wire brut FIRST/LAST extrapolé vaut déjà environ 1,60 To tous K ;
l'ancienne ligne 620 Go est retirée.

### Grille et G1

La grille de cellules n'a plus que six coutures documentaires et
d'environnement, listées dans
`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`. Ne pas rouvrir son noyau.

Pour G1, conserver les bornes d'indices, la distinction géométrie absente/vide,
les mutants SoA réellement exécutés, un `PointId` q4 au-delà du bit 31, le
contexte géométrique partagé et une réservation exclusive du wire actif. Le
protocole est condensé dans `QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md` et
`../docs/GPU.md`. Aucune nouvelle matrice G4 avant fermeture locale de G0/G1.

## Validation indépendante du snapshot

- configuration canonique et build Release : succès ;
- campagnes ciblées enveloppe/CLI/mutants et routes batched : `27/27` ;
- registre direct : `80` mutants déclarés, `80` injectés, `80` gardés ;
- campagne complète antérieure label `gate` : `250/251` en `790,97 s`, seul
  le timeout post-séparation décrit ci-dessus ; rejeu isolé vert en
  `153,94 s` ; un rejeu frais a retrouvé ce timeout à `300,12 s` sous
  concurrence puis a été interrompu, il ne constitue pas une campagne
  complète supplémentaire ;
- reçu baseline `echelle_par_lane_20260829` : six runs produit au pin propre
  `a3c15d84`, codes nuls et compteurs exacts ; les ventilations par route et
  le proxy de concentration de l'addendum ne sont pas dans ses sorties ;
- harnais `66997d56` : syntaxe Bash valide ; les cinq scénarios de
  l'auto-fixture rendent les codes attendus et son code final vaut 0 ; le test
  causal `TERM` échoue et la comparaison inter-threads diverge à tort ;
- contrôles documentaires et diff final à rejouer après consolidation.

## Ordre recommandé

1. Fermer la non-vacuité oversized, le timeout et le harnais, puis pinner le
   raccord d'enveloppe avec ses tests ; ne plus l'étendre avant réception.
2. Refaire build et campagne `gate` sur ce pin.
3. Ouvrir `q34_fiber` comme falsificateur counter-only du center-cover de
   blocs ; prouver provenance, ledgers et continuations sans matérialiser une
   ancre.
4. Seulement si ce prune est non vacant, ouvrir `AnchorLineSet`, ses compteurs
   par ancre, l'oracle exhaustif borné puis le shallow ; comparer en shadow
   `all-direct`, `all-shallow` et le dispatch adaptatif. Tester la WSPD locale
   q4 comme ablation, jamais comme prérequis.
5. Fermer ensuite G0/G1, fold vivant et grille selon leur ordre local ; aucun
   de ces chantiers ne doit masquer les compteurs de la nouvelle source.

GCP non utilisé.
