# État courant audité de MorseHGP3D v5 — 28 août 2026

- **Dernier pin technique relu :** `700a38c7`, correction de la publication du fold et durcissements SCALE, au-dessus du smoke test de rejeu `f4b554fe` et des raccords `476a55fd`, `c95cfa95`, `4816ea27`.
- **Réponse de conception active :** V17–V30 sont traités dans [QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md](QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md). L'ordre instrument → G0 → G1 → G2 est accepté et ne doit pas être retardé par les obligations propres à L7.
- **Worktree produit au moment du verdict :** postérieur à `700a38c7`, sale sur un chantier G0 concurrent (`executor_pool.hpp`, sa porte, CMake et registre de mutants), ainsi que sur le probe `.codex_fold_contract_probe.cpp` de l'autre audit. Tous restent préservés, non reçus et hors du commit d'audit.
- **Pins de performance conservés :** `82f613d3` pour les campagnes CPU 50–200 k et `63deda74` pour les étapes device à 50 k.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Verdict utile à Claude

**Orange constructif : `700a38c7` ferme le défaut d'ordre `kPublished` et plusieurs trous du pilote SCALE. `f4b554fe` est un bon smoke test de cohérence, mais ne ferme pas encore le décodeur indépendant T5. Claude peut poursuivre G0 en parallèle ; la session G4 n° 13 annoncée reste à recevoir et tout claim device demeure prématuré.**

La série ferme l'essentiel des défauts de vie du fold : slot possédé avant le lancement, drainage explicite, arbitrage des défauts au tour de K, domaine de `fold_inflight`, mesure du pic et portes à fautes injectées. `476a55fd` capture l'allocation du message d'invariant, `700a38c7` appelle le hook terminal avant d'ouvrir K+1 et ajoute le scénario N5, `c95cfa95` imprime les dénominateurs nécessaires à V29, et `f4b554fe` rejoue réellement 733 029 deltas sur 5 194 737 facettes. Elle apporte en outre un oracle indépendant substantiel pour la grille, des compteurs device mieux nommés, une porte de préfixe étendue et un pilote SCALE testable hors GCP. Ce sont des progrès fonctionnels réels, pas seulement de la documentation.

Le P0 de code `kPublished` est reçu au pin. La porte T5 reste provisoire avant le fold streamé, sans bloquer G0/G1/G2. Les autres corrections du fold sont des durcissements courts ; elles ne doivent pas détourner Claude du pool, du wire par indices et de la compaction stable.

## Avant le fold streamé — transformer le smoke T5 en décodeur borné

Le cœur de `mhgp5_delta_replay` est utile : union-find frais, minimum de bloc recalculé, comparaison finale et deux corruptions d'intégration orthogonales réellement détectées. Mais `facet_keys`, `deltas` et `final_canon_fid` proviennent tous du même `ForestResult`. Supprimer de façon cohérente une facette ou une fusion des trois objets peut donc rester vert ; un ordre vide peut aussi être masqué par les seuls planchers agrégés. Le nom « autorité indépendante du fold » est trop fort à ce pin.

Cinq raccords font de ce travail le décodeur demandé sans le réécrire :

1. Borner d'abord le claim à `(catalogue,deltas) -> final_canon_fid` sous flux accepté et compteurs de violations de rôles nuls. Si la porte doit aussi certifier la source du catalogue, énumérer alors indépendamment depuis les `ForestEvent` toutes les K-facettes attendues, avec un comparateur lexicographique local, puis comparer ce catalogue à `facet_keys`.
2. Remplacer `born_count + alive_root` par les états `unseen`, `introduced`, `alive`, `absorbed`. Une facette est introduite exactement une fois, soit dans `born`, soit comme parent singleton implicite. « Jamais born ⇒ canonique final propre » est faux dès K=1 : un singleton implicite peut fusionner ensuite.
3. Valider un delta entièrement avant toute union : `parents` et `born` strictement triés et uniques, racines pré-delta distinctes et vivantes, clé présente, sortie égale au minimum, et refus du no-op `parents=[A], born=[]`. Seulement ensuite unir et changer les états. La contre-fixture minimale `parents=[A,A], born=[], output=A` passe actuellement à tort.
4. Juger `batch`, `level`, `batch_levels` et `batches` : batch dans le domaine et monotone, niveau égal à `batch_levels[batch]`, compteurs de violations nuls. Figer le snapshot des canoniques vivants au début du lot, valider tous ses deltas contre ce snapshot avant la moindre union et interdire une chaîne intra-lot artificielle.
5. Ajouter de petites fixtures synthétiques pour doublon, clé hors catalogue, sortie non minimale, parent ressuscité, naissance après singleton implicite, chaîne de deux deltas dans le même lot, lot/niveau invalide et catalogue incomplet, avec mutants locaux du décodeur.

Jusqu'à ces raccords, conserver `f4b554fe` comme smoke test résident u32 de cohérence. Après eux, l'appeler « décodeur indépendant du fold » est exact ; l'autorité sémantique de la forêt reste aux juges/oracles prévus et le futur wire massif u64 reste à recevoir séparément.

## Requalifications du pin

### Fold et mesures

`700a38c7` déplace correctement le hook `kPublished` avant l'ouverture de K+1. N5 force K=3 à terminer sa réduction avant la publication de K=2 et prouve que la faute du hook K=2 empêche toute publication supérieure ; ce P0 est reçu.

Six durcissements ciblés restent utiles : le `catch` principal de réduction à la ligne qui affecte encore inconditionnellement `sp->exc` doit préserver une faute antérieure de `kReduceBegin` ; N3 doit exiger K=2 et K=3 `kNotPublished` sans `kPublished` quand `on_forest(K=2)` lève ; N5 doit exiger K=1/K=2 publiés et jamais `kNotPublished`, callbacks ordonnés/non chevauchés ; `published_complete` peut maintenant juger l'ordre strict des hooks ; N1 doit exiger un pic de trois ; le mutant A doit synchroniser `callback_K1_entered` vers `kStageABegin(K=2)` avant d'attendre `kStageAFailed`. Pour tuer l'écrasement d'exception sans nouveau mutant, faire lever le hook de K=3 à `kReduceBegin`, puis laisser le mutant B lever : l'exception finale doit rester celle du hook.

La porte nominale et ses mutants passent, mais aucun reçu TSan reproductible n'est acquis. GCC TSan échoue de manière intermittente avant les tests avec `unexpected memory mapping`; Clang TSan bute sur le builtin SHA du projet. Ce constat ne révèle ni n'exclut une race. Les phrases « rejouée sous TSan » du plan et « TSan propre » de `GPU.md` dépassent donc les preuves versionnées : joindre un reçu sur runner compatible ou les requalifier en protocole prévu. Cela ne bloque pas le chemin CPU nominal.

Les portes concurrentes doivent recevoir un `TIMEOUT` CTest global. Dans N1, la poignée maintient réellement trois workers B vivants ; exiger `peak_fold_inflight == 3`, et non seulement `>= 2`, rend la non-vacuité exacte sans prétendre mesurer trois appels simultanés à `reduce_fold`.

`pic_mesure_en_vol` mesure des workers B vivants, y compris attente de publication et hooks, pas des appels `reduce_fold` simultanés. `rss_mb[4]` reste un maximum d'échantillons après callback, pas le pic RSS du fold. `t_fold_wall_ms` inclut drainage et hooks. Ces libellés suffisent s'ils restent exacts ; un second compteur autour de `reduce_fold` est nécessaire pour revendiquer une réduction réellement simultanée.

Le domaine public `fold_inflight=1..16` dépasse le Kmax utile et ne possède pas encore de préflight mémoire agrégé. Conserver 1..3 comme domaine mesuré dans le prochain pilote, puis ouvrir davantage à partir d'un majorant de fenêtre et d'un pic RSS reçu ; ce n'est pas un blocage du code nominal.

La nouvelle porte `mhgp5_delta_replay` repart bien d'un union-find frais, ne lit pas `final_canon_fid` pendant le rejeu, puis compare la partition reconstruite au tableau final. Ses planchers et ses deux mutants empêchent un vert entièrement vide, mais les limites P0 détaillées plus haut interdisent encore de la recevoir comme décodeur indépendant. Elle ne prouve ni la compacité mémoire ni le fold streamé lui-même.

### Grille de cellules

Le code et l'oracle sont assez solides pour continuer : les sept CTests ciblés sont inclus dans la relecture indépendante, l'oracle compare le comptage optimisé à une évaluation i128 et tue ses mutants. Cela reçoit une intégration CPU conservatrice au pin, pas encore la formulation canonique du théorème 10.5.

Les sept corrections courtes déjà listées dans [QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md](QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md) ne sont pas toutes intégrées. La série ajoute bien F11 et l'oracle au plan de tests. Les écarts encore visibles comprennent : la couverture justifiée par la seule norme des vecteurs, le libellé des 4 799 488 comparaisons, le facteur déjà inclus dans `rhs`, l'affirmation trop forte sur FMA, la dernière borne d'arrondi et les gardes binary64 réduites à `FLT_EVAL_METHOD == 0`. Ces six corrections documentaires doivent précéder la formule « théorème reçu », mais ne justifient ni une réécriture de l'algorithme ni l'arrêt des raccords GPU.

### Instrumentation GPU et campagne SCALE

Les portes hôte de l'instrument passent et le schéma sépare maintenant copies, événements kernels, attente, octets et tailles de lots. La session 13 a été annoncée au pin `c95cfa95`, antérieur aux durcissements SCALE de `700a38c7`; aucun de ses résultats, profil d'occupation ou reçu nvcc n'est encore reçu. Les événements inter-stream incluent la contention de planification entre leurs bornes ; ils ne sont pas des coûts intrinsèques isolés.

Avant de prendre ce schéma comme autorité, ajouter une version explicite telle que `gpu_instrument_schema=v2` et faire exiger par le validateur exactement une ligne courante par lane, tous les champs obligatoires, finis et non négatifs. Un résidu temporel négatif ne doit pas être écrêté silencieusement à zéro : publier l'écart signé et faire échouer la porte au-delà d'une tolérance préenregistrée. Les octets H2D doivent aussi séparer au moins géométrie résidente/sites, seeds, ancres et intermédiaires ; sinon l'ablation G1 ne pourra pas attribuer les 112/288 octets annoncés par seed.

Le cycle de vie reste incomplet si l'exécuteur `thread_local` du fil appelant est détruit après l'impression finale. Le contexte G0 doit posséder et drainer explicitement ses exécuteurs avant le reçu. De même, la jauge de flux et son reset doivent appartenir à une invocation, pas à un état statique partagé entre pipelines concurrents, et toute erreur CUDA observée après enqueue doit empoisonner puis drainer le slot avant réutilisation.

### Aide au chantier G0 actuellement non committé

Le helper `ExecutorPool` en cours possède déjà les bonnes briques nominales : un exécuteur local et persistant par worker, file bornée, résultat producteur-local, réémission de l'exception au soumetteur et compteurs atomiques cohérents. Ses trois portes hôte passent et tuent les deux mutants ; une répétition indépendante 30 fois reste verte et le registre est cohérent à 66 noms. C'est une bonne base, mais pas encore une primitive reçue :

- fermer d'abord la durée de vie du ticket : le worker pose `done=true`, libère `t.mu`, puis notifie une `condition_variable` stockée dans le ticket de pile. Un réveil spurieux peut faire revenir le producteur, détruire le ticket puis laisser le worker notifier l'objet détruit. Notifier sous `t.mu`, avant sa libération, ou donner au ticket une propriété partagée ; graver une couture déterministe de destruction immédiate ;
- refuser explicitement toute taille hors `{1,2,4,8}` au lieu de la clamper ; exercer les quatre valeurs et les refus 0/3/9. Forcer exactement N jobs simultanés par latch, sans dépendre du scheduler : sous `taskset -c 0`, le plancher courant `peak>=2` tombe légitimement à 1 ;
- construire chaque `Executor` dans un `try` avec handshake d'initialisation. Si un constructeur d'exécuteur ou un lancement de `std::thread` échoue partiellement, poser l'arrêt, réveiller, joindre les fils déjà créés puis relancer ; aucun fil joignable ne doit atteindre le destructeur du vecteur ;
- documenter et détecter la réentrance : `submit_and_wait()` depuis un job du même pool bloque certainement à N=1 et peut saturer tous les workers à N>1. G0 n'en a pas besoin ; un marqueur `thread_local` peut la refuser immédiatement, puis la porte vérifie que le pool demeure utilisable ;
- distinguer une exception CPU déclarée récupérable, qui peut rester locale au job, d'une erreur device fatale. Pour cette dernière, passer `running -> poisoned`, fermer l'admission, mémoriser la première exception, annuler la queue en complétant tous ses tickets, réveiller `cv_`, `cv_space_` et les tickets, laisser finir les jobs déjà actifs puis retirer chaque exécuteur sans réutilisation. Exiger après `close/drain` : `submitted = succeeded + failed + cancelled` et `active = queued = 0` ;
- brancher réellement ce pool dans q3 et q4. Le test hôte du helper ne reçoit pas G0 à lui seul. Aujourd'hui les résidus sont flushés par la boucle séquentielle après `parallel_items`; effectuer ce flush final depuis T producteurs concurrents ou ajouter un finalizer par shard, puis exiger pic `>=2` sur un plancher de résidus non vides ;
- ajouter des fixtures d'échec du constructeur, démarrage partiel, réentrance, queue pleine avec soumetteur bloqué, poison sans réutilisation, rejet après poison et drainage normal, plus une comparaison q3/q4 avec scanner factice des sorties, compteurs et ordre. Compter constructions et destructions d'exécuteurs ; mesurer le cap avec `queue_cap=1` et un mutant d'ignorance de la limite ; ajouter les includes directs manquants et un `TIMEOUT` aux portes.

Cette fermeture conserve le jalon petit : elle ne demande ni géométrie résidente ni nouveau kernel. Elle transforme seulement le pool hôte en propriétaire fiable des exécuteurs actuels et rend l'ablation 1/2/4/8 causale.

`700a38c7` aligne les faux producteurs sur `tower_scope=profile_complete_k10`, exige les deux `smax`, refuse axes dupliqués et familles inconnues, grave une troncature à l'échéance et s'arrête au premier code non nul. C'est reçu comme durcissement hôte. Le validateur accepte toutefois encore l'ancien format du fold sans pic et n'exige pas `pic == 1` lorsque `fold_inflight == 1`; réserver le legacy à une commande explicite de relecture historique et exiger le format courant dans toute nouvelle campagne.

Un défaut du même pin est reproduit hors fixture : si la ligne `famille=...` manque mais que `tower_scope` est présent, le `elif` rattaché au mauvais `if` appelle `ident.group(...)` sur `None` et termine par `AttributeError`, au lieu de rendre un refus borné. Séparer les deux contrôles et graver les cas « identité absente/tower présent » et « identité fausse/tower absent ». Les 20 tests Python passent parce qu'ils ne couvrent pas encore ce mutant.

Avant une campagne complète, la session doit calculer elle-même `SCALE_DEADLINE_EPOCH` depuis le minimum de ses deux coupe-circuits, réserver une marge de rapatriement et borner tout override ; elle ne fait actuellement que transmettre une valeur optionnelle fournie de l'extérieur. Le plan par défaut de 192 runs à 7200 s représente encore 16 jours théoriques et ne tient pas dans la session de quatre heures. Le protocole A doit imposer réellement son cpuset via `taskset` ou équivalent ; enregistrer seulement l'affinité observée ne mesure pas un speedup 1→N. Enfin, sérialiser les variables distantes avec `printf %q` ou un fichier d'environnement et parser les axes en tableaux quotés. Le pilote annoncé de 14 runs est une bonne taille exploratoire, mais avec une répétition et sans cpuset il ne doit pas être nommé speedup ni « contrebalancé ».

## Réponse constructive à V17–V30

La réponse détaillée se trouve dans le fichier de question de Claude, afin de ne pas multiplier les audits. Les décisions qui changent immédiatement l'implémentation sont :

- V17 : le cover stable requiert des préfixes exclusifs par chunks dans l'ordre logique et une comparaison vectorielle directe, pas seulement des compteurs ;
- V18 : la conversion DI128 doit conserver 55 bits avec garde, round et collant, traiter `-2^127` sans négation signée et disposer d'un oracle construit autrement ;
- V19 : la notation et la dérivation annoncées pour le minimiseur séparable sont incomplètes ; en distinguant le centre continu du minimiseur entier, un correctif court prouve une erreur strictement inférieure à un demi, puis évalue exactement plancher et plafond en DI128 ; cela ferme seulement L7c ;
- V20 : le seuil de repli CPU qualifie un reçu `device-dominant`, pas sa validité fonctionnelle ; conserver les reçus hybrides en les nommant correctement ;
- V23 : `scanline_overlap_multiecho` déduplique bien ses XYZ et passe V1 ; une vraie entrée à doublon reste refusée avant la lane ;
- V27 : une constante device modifiée par lot est incompatible avec des streams concurrents ; copier un masque immuable avant les launches ou le passer au kernel, avec plusieurs mots puisque le registre dépasse 32 puis 64 mutants ;
- V28 : la fusion/RLE globale des runs est le premier jalon recommandé, sans prétendre qu'elle soit l'unique architecture correcte ;
- V29 : les 214,5 s du reçu réfutent l'hypothèse `seeds ×4`, mais le calcul à partir d'un minorant de seeds n'est qu'un contrôle inférieur, pas encore un modèle prédictif ; `c95cfa95` imprime désormais `seeds` par lane et les complétions q4, le prochain reçu doit ajouter les autres dénominateurs et les temps-fil ;
- V30 : séparer ressources statiques, plafond théorique et occupation dynamique ; `%smid` mesure le placement, pas des blocs simultanés.

Le passage de relais recommandé reste donc simple : finir les raccords CPU sans GCP, livrer G0, G1 et G2 avec leurs égalités locales, puis fermer les obligations L7 au moment où chaque kernel correspondant devient réel.

## État de validation

Dans un worktree détaché propre à `f4b554fe`, la relecture croisée a reconstruit les cibles rejeu T5, fold, API, préfixe et instrument GPU hôte, puis obtenu 21/21 CTests en 98,11 s. Le sous-ensemble T5 reproduit 58 ordres, 733 029 deltas, 5 194 737 facettes et zéro désaccord. Après passage du même worktree à `700a38c7`, les quatre CTests fold passent en 4,50 s et les tests Python SCALE passent 20/20 en 0,126 s ; le crash `ident=None` du validateur a été reproduit séparément. Dans le worktree sale, le prototype G0 passe ses 3/3 portes hôte, mais ces verts ne couvrent ni son démarrage, ni son poison, ni son intégration aux lanes.

`python3 tools/check_docs.py` valide 217 fichiers actifs, `python3 tools/check_implementation_status.py` valide 20 phases et la fonction `validate()` appliquée explicitement aux trois Markdown d'audit modifiés ne remonte aucune erreur avant commit.

Les validations locales de l'audit n'exercent ni nvcc, ni device CUDA, ni TSan. Pour la session n°13 de Claude, le journal local confirme avant démarrage la cible `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, le modèle `SPOT`, l'action `STOP`, `maxRunDuration=14400 s`, l'arrêt invité à 230 minutes et le trap d'arrêt lié à la génération `c95cfa95`. Les portes GPU et contrats 50 k sont verts dans le journal, mais la phase SCALE et l'arrêt ciblé restent en cours : aucun reçu n'est encore reçu par cet audit. Les validations plus anciennes restent bornées à leurs pins : suites CPU à `369f3ac0`, campagne CPU 50–200 k à `82f613d3` et session device à `63deda74`.

GCP non utilisé par cet audit ; session n°13 lancée et détenue par Claude.
