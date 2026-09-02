# ALERTE — sonde d'ablation `reduce` : reconnaissance utile, attribution non causale

Date : 2 septembre 2026. Coupe auditée : `81623528`, avec la campagne locale
`receipts/sonde_ablation_reduce_20260902/` désormais terminée. Cadre :
`phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Verdict utile à Claude

La décomposition avant écriture d'un palier est la bonne étape. Les mutants
destructifs donnent des **bornes exploratoires** sur trois groupes de travail
et les portes qui les tuent protègent le produit. Le reçu final doit être
conservé comme reconnaissance négative/diagnostique, mais il ne peut pas
encore attribuer causalement un écart à la copie, au tri ou aux lectures de
clés, ni choisir une implémentation `CompactDelta`.

Trois coutures suffisent ; il n'est pas nécessaire de rouvrir le pipeline.

1. **Le plan à trois répétitions n'est pas équilibré.** Le lanceur alterne
   seulement l'ordre direct et inverse. Avec la valeur courante `REPS=3`, les
   ordres sont direct/inverse/direct : les positions médianes des quatre bras
   restent respectivement 1, 2, 3 et 4. Une dérive intra-bloc est donc
   confondue avec l'ablation, et la différence de médianes non appariées de
   l'agrégateur ne la retire pas. Employer au minimum quatre blocs avec chaque
   bras une fois à chaque position (plan latin/Williams), puis agréger les
   différences appariées par bloc.
2. **La “clé factice” ne retire pas seulement une lecture aléatoire.** Elle
   remplace le multiensemble de `parents`/`born` par une clé constante, puis
   trie ces clés identiques. Elle change donc aussi le coût et la distribution
   du tri dans `materialisation_tri_copie`. Les médianes finales confirment ce
   débordement : `post_remplissage` baisse de 73,9 %, 73,5 % et 75,0 % selon
   la taille, tandis que `materialisation_tri_copie` baisse aussi de 15,4 %,
   14,4 % et 15,3 %. Renommer ce bras en
   borne composite, ou pré-matérialiser hors fenêtre les **mêmes valeurs** dans
   le même ordre et conserver exactement le multiensemble trié. Tout bras
   candidat à une optimisation doit ensuite comparer le `ForestResult`
   complet au témoin, contrairement aux mutants destructifs qui doivent
   continuer à diverger.
3. **Le reçu est fail-open et le binaire reste mutable.** Le SHA-256 n'est lu
   qu'au début ; le binaire partagé `build/v6/mhgp6_profile_sonde` est exécuté
   directement, sans copie privée ni contrôle avant/après chaque tuple. Le
   hash observé après publication est encore celui du `META`
   (`74a46046...`), donc aucune contamination n'est constatée sur ce run,
   mais le protocole ne l'aurait pas détectée. En outre `REPS=0` publie
   un reçu vide, l'agrégateur accepte une matrice absente et transforme des
   fenêtres manquantes en zéros, et l'échec de génération de `SHA256SUMS`
   n'est pas fatal.

## Fermeture minimale avant une seconde mesure

- copier le binaire dans le `.partial`, graver son hash, puis vérifier ce hash
  avant et après chaque tuple ;
- refuser une liste de tailles vide, `REPS <= 0` et un nombre de blocs non
  compatible avec le plan équilibré ;
- exiger l'ensemble exact `bras × tailles × répétitions`, les dix lignes K,
  toutes les fenêtres finies et les codes nuls ; ne jamais substituer zéro à
  une mesure absente ;
- rendre fatals l'agrégateur, la génération puis la vérification finale de
  `SHA256SUMS`, avec une porte de vacuité et un mutant de binaire remplacé ;
- conserver le run terminé sous un libellé explicite du type
  `exploratory_noncausal_upper_bounds`, sans en tirer un choix de palier.

Cette fermeture est locale et CPU. Elle ne demande ni nouvelle session G4 ni
modification du statut public.

## Clôture factuelle du reçu terminé

La mécanique de cette exécution particulière est intacte : 115 fichiers,
114 entrées SHA-256 couvrant exactement les autres fichiers réguliers, 36
cellules exactes (`4 bras × 3 tailles × 3 répétitions`), tous les codes nuls,
dix lignes K et neuf fenêtres par sortie. Les copies du lanceur et de
l'agrégateur égalent bit à bit `81623528`; le résumé est reproductible bit à
bit depuis les sorties. Le hash du binaire partagé est resté identique après
publication. Ces faits reçoivent le paquet, pas son attribution causale.

Le signal utile est une priorité de falsification. La suppression destructive
de la copie borne à 53,8 %, 57,0 % et 59,2 % la part retirée de la fenêtre
`materialisation_tri_copie`; la suppression du tri la borne à 27,5 %, 25,8 %
et 25,4 %. Mais le gain apparent du premier bras sur le mur instrumenté vaut
4,8 %, -0,9 % puis 7,1 % : signe suffisant qu'ordre, charge et autres étages
dominent encore la comparaison. À 16k, la différence des médianes dit même
`+669 ms`, alors que les trois différences appariées valent `-847`, `+669` et
`-3634 ms`, de médiane `-847 ms` : l'agrégation courante peut inverser le
signal. Une représentation sans copie est donc la
première hypothèse sémantiquement valide à **falsifier**, avec différences
appariées et égalité complète du `ForestResult`, pas un palier déjà choisi.

`META.txt` grave en outre `worktree_sources_modifies=1` sans embarquer le diff
correspondant. La copie du protocole rejoint le pin et le worktree observé ne
montre qu'un changement de mode sur ce lanceur, mais le reçu seul ne prouve
pas cette explication. Cela renforce son statut exploratoire et interdit d'en
faire un benchmark de référence.

Le libellé embarqué reste
`sonde_locale_non_decisionnelle (attribution decomposee...)`. Le mot
« attribution » est trop fort au regard du plan ; le verdict extérieur de ce
rapport prime et classe le reçu `exploratory_noncausal_upper_bounds`.

## État du WIP de fermeture de la sonde

Photographie : worktree non commité au-dessus de `101c33cf`. Le nouveau
lanceur ferme réellement l'essentiel du protocole nominal : carré de Williams
`ABCD/BDAC/CADB/DCBA`, répétitions positives multiples de quatre, plan gravé
avant exécution, différences appariées par bloc, binaire privé seul exécuté et
hashé autour de chaque tuple, ensemble ordinaire des cellules/K/fenêtres,
agrégateur et génération/vérification du manifeste fatals. La porte directe,
son exécution sous `python3 -O` et le CTest passent localement.

Cinq contre-fixtures publient néanmoins encore un reçu avec code nul :

1. **Hash vide.** Un faux `sha256sum` qui rend zéro sans sortie produit
   `binaire_sha256=` et des lignes `avant= apres=` vides. Centraliser une
   primitive de hash fatale et exiger exactement 64 hexadécimaux pour source,
   copie, protocoles et chaque tuple.
2. **Taille dupliquée.** `N_LIST="64 64"` écrase les tags de cellules ; le
   reçu annonce 32 runs mais ne porte que 16 statuts. Refuser les doublons
   dans le lanceur et l'agrégateur, puis comparer compte annoncé et cardinal
   exact des artefacts.
3. **Plan latin non Williams.** Un carré cyclique garde l'équilibre des
   positions mais répète quatre transitions au lieu de couvrir les douze ;
   l'agrégateur l'accepte. Vérifier les quatre lignes canoniques ou l'ensemble
   exact des successions ordonnées, pas les seules positions.
4. **Champ dupliqué.** `touch=nan touch=<fini>` passe parce que le dictionnaire
   écrase la première valeur. Refuser tout champ profil, META, statut ou plan
   dupliqué et toute ligne `profil_reduce` malformée au lieu de l'ignorer.
5. **Manifeste imbriqué invisible.** Un `out/sub/SHA256SUMS` créé par le
   binaire est publié mais absent du manifeste, car `! -name SHA256SUMS`
   exclut tous les basenames. Exclure seulement `./SHA256SUMS`.

L'inventaire `out/` doit en plus exiger exactement un triplet
`txt/err/status` par cellule et aucun fichier ou répertoire supplémentaire :
un `.err` manquant et des artefacts arbitraires passent encore. Ajouter ces
cinq dents, l'échec de **génération** du manifeste et l'inventaire exact aux
mutants permanents. Le plan et l'appariement sont reçus comme progrès ; la
fermeture fail-closed ne l'est pas encore.

## Signalement mis à jour sur le WIP de revalidation adjacent

Photographie : worktree non commité au-dessus de `38281dc7`. Trois fermetures
sont réelles : la normalisation retire maintenant exactement `./`, le hash du
manifeste initial domine le contrôle final, et liens/types spéciaux sont
refusés. Les doublons de codes session sont aussi comptés avant extraction.
Les mutants rehash, symlink, fichier intrus et codes dupliqués du selftest
courant sont verts.

Cette progression ne ferme pas encore la revalidation :

1. **Le validateur n'est pas authentifié.** Le second argument reste un chemin
   arbitraire. L'exécution avec `/dev/null` rend effectivement `0`, ne produit
   aucun résumé et affiche pourtant « recu intact ». En mode normal, exiger la
   cible attendue et graver son hash ; si l'injection d'un faux validateur est
   nécessaire aux tests, la réserver à un mode selftest explicite qui ne peut
   jamais publier un verdict de revalidation.
2. **Tout basename `SHA256SUMS` est exclu.** `find ... ! -name SHA256SUMS`
   retire aussi `out/SHA256SUMS` et `marques/SHA256SUMS` de l'inventaire. Seul
   `./SHA256SUMS` à la racine doit être exclu. Ajouter les deux contre-fixtures
   imbriquées.
3. **Les répertoires ne sont liés qu'avant l'appel.** Le contrôle final
   recompare les fichiers et les types irréguliers, pas l'ensemble exact des
   répertoires. Un validateur hostile peut laisser un répertoire vide ajouté
   ou supprimé. Graver puis revalider aussi cet ensemble.

`selftest_revalidate_v6.sh` est vert, mais ne contient aucune de ces trois
dents. Ce constat vise uniquement le WIP ; il ne remet pas en cause
l'intégrité déjà vérifiée du reçu.

## Réception critique du pin `1cb60655`

Claude a correctement transformé les cinq faux positifs historiques en dents
permanentes. Au pin courant :

- construction de `mhgp6_profile_sonde` : succès ;
- porte directe normale et sous `python3 -O` : 11/11 scènes vertes dans les
  deux cas ;
- CTest ciblé `mhgp6_profile_sonde_refuse_inconnu` et
  `mhgp6_sonde_ablation_gate` : 2/2 en 7,18 s.

Cette fermeture est reçue dans sa portée. Elle ne rend toutefois pas encore le
harnais fail-closed : quatre familles de faux positifs distinctes restent
reproductibles ou directement exposées par le chemin exercé.

1. **TOCTOU sémantique entre agrégation et scellement.** Le lanceur produit
   `resume.txt`, puis seulement ensuite construit et vérifie `SHA256SUMS`. Un
   faux `python3` qui exécute le vrai agrégateur puis modifie le `statut=` de
   `META.txt` publie avec code 0 un reçu où META et résumé se contredisent.
   Après scellement de l'ensemble, réagréger avec la copie archivée, comparer
   bit à bit au premier résumé, puis revérifier tous les hashes. Ajouter ce
   mutant avant toute nouvelle mesure.
2. **Comparaison d'inventaire fail-open.** Le pipeline
   `diff | grep | head | tr` ne distingue pas « différence » et erreur de
   `diff`. Avec un faux `diff` qui rend 2 et deux intrus hachés, le lanceur
   rend 0 et publie. Comparer les deux ensembles en Python, ou capturer
   explicitement `diff` avec `0=égal`, `1=écart`, `2=erreur fatale`, puis
   conserver une dent `diff` défaillant. L'inventaire doit aussi lier les
   répertoires : un dossier vide inattendu à la racine est actuellement
   publié sans apparaître dans le manifeste.
3. **Le protocole n'est pas lié causalement à ses métadonnées.** L'agrégateur
   rend encore 0 après promotion arbitraire de `statut`, remplacement de
   `sha256_lanceur` par 64 zéros, modification de
   `protocole_lanceur.sh`, ou suppression de `runs_effectues` et
   `runs_attendus` ; le statut promu passe aussi sous `python3 -O`. Exiger le
   statut exploratoire exact, exactement les quatre bras, les deux compteurs,
   et recalculer les hashes des protocoles archivés. Verrouiller aussi la ligne
   de profil `profil_kind=reduce_v2 fold_join=1 ...`, refuser toute ligne
   profil malformée et exiger l'unicité de `temps_mur_ms` et `rss_max_kb`.
4. **Frontière CLI trop large.** Le binaire délègue au registre générique et
   accepte avec code 0 `--inject=`, `--inject=,` et le mutant étranger
   `render-active-only`; `ablation-inconnue` rend bien 2. Cette cible doit
   autoriser exactement les trois ablations nommées. Ajouter les trois CTests
   de rejet et conserver celui du nom inconnu.

La correction la plus économique est donc une seule passe de fermeture :
contrat META/profil exact dans l'agrégateur, inventaire sans pipeline ambigu,
réagrégation du jeu scellé et whitelist CLI. Les scènes positives existantes
restent utiles ; il suffit de leur ajouter ces mutants. Le reçu concret
`b79e29a5` conserve sa valeur diagnostique. Le prototype sémantique KeyCSR
peut avancer indépendamment en local ; une nouvelle mesure réutilisant ce
harnais et tout claim de performance attendent cette fermeture.

## Réception critique du pin `fc8e28b1`

Le nouveau pin ferme effectivement l'essentiel de la passe précédente. Les
empreintes prises avant agrégation sont reliées au manifeste final, les codes
1 et 2 de `diff` sont traités sans ambiguïté, les compteurs sont obligatoires,
la racine et `bin/` sont exacts, les protocoles archivés sont rehachés et la
grammaire des lignes `profil_reduce` est nettement durcie. Sur les fichiers du
pin restés stables pendant les rejeux :

- porte directe : 18/18 scènes en mode normal et 18/18 sous `python3 -O` ;
- CTest ciblé `mhgp6_profile_sonde_refuse_inconnu` et
  `mhgp6_sonde_ablation_gate` : 2/2 en 21,01 s.

Ce progrès est reçu. Une seule famille de contrat bloque encore une **mesure
réutilisable** : le régime exécuté n'est pas relié causalement au régime
annoncé. Sur des copies du reçu nominal, l'agrégateur rend encore 0 après
chacune des mutations suivantes :

- ligne de sortie remplacée par `profil_kind=not_reduce fold_join=0` ;
- `--fold-join=1` remplacé par 0 dans `commande=` **ou** `fold_join=1`
  remplacé par 0 dans `META.txt:parametres` ;
- champ `liveness=0.000` retiré d'une ligne `profil_reduce` ;
- `statut` prolongé par `PUBLIC_STATUS=exact benchmark_reutilisable`, ou
  `identite_cible=mhgp6_profile_sonde_evil`, car les deux contrôles emploient
  `startswith`.

La fermeture minimale est cohérente avec le dessin actuel : exiger une unique
ligne `profil_kind` avec `reduce_v2`, `fold_join=1`, inflight et layout
attendus ; comparer chaque `commande=` à l'argv canonique de son tuple ; lier
les mêmes valeurs dans `META.txt:parametres` ; rendre `liveness` obligatoire et
refuser les champs profil hors schéma ; comparer exactement `statut` et
`identite_cible`. Une seule contre-fixture composée peut couvrir ces liaisons,
puis deux dents courtes gardent les deux comparaisons exactes.

La CLI de test accepte toujours `--inject=` vide, `--inject=,` et
`render-active-only`. C'est un durcissement P2, pas un blocage supplémentaire
du harnais : le lanceur et l'agrégateur restreignent déjà les injections
émises. Une cible d'ablation dédiée serait plus simple à expliquer, mais la
liaison exacte de l'argv suffit pour cette mesure.

Le reçu concret `b79e29a5` garde sa valeur diagnostique et le prototype
sémantique KeyCSR reste indépendant. Seule une nouvelle publication de mesure
avec ce harnais attend la dernière liaison de régime.

## Réception critique du pin `32da1550`

La v4 apporte le bon durcissement, sans remettre en cause les calculs déjà
reçus. Après scellement, le lanceur réagrège avec l'interpréteur gravé hors du
`PATH`, compare le second résumé au premier et revérifie le manifeste. Le
mutant qui modifie temporairement les sorties pendant la première agrégation,
puis les restaure, est désormais refusé. Le statut complet et la ligne
`profil_kind` sont aussi comparés plus strictement ; le reçu historique v2
reste accepté avec son avertissement explicite de claim borné.

Sur les trois fichiers identiques au pin :

- porte directe : 21/21 scènes en mode normal et 21/21 sous `python3 -O` ;
- CTest ciblé `mhgp6_profile_sonde_refuse_inconnu` et
  `mhgp6_sonde_ablation_gate` : 2/2 ;
- syntaxe et réagrégation v4 : vertes.

Cette progression est reçue. L'intitulé « contrat meta/profil exact » reste
toutefois un peu plus large que ce que le juge prouve : une seule famille, la
liaison causale du régime, conserve quatre faux positifs indépendants. Sur une
copie du nominal, l'agrégateur rend encore 0 après chacune des mutations
suivantes :

- `--fold-join=1` remplacé par 0, ou retiré, dans `commande=` ;
- `fold_join=1` remplacé par 0 dans `META.txt:parametres` ;
- `liveness=0.000` retiré des dix lignes d'une sortie ;
- `identite_cible=mhgp6_profile_sonde_intrus`, encore accepté par
  `startswith`.

Les variantes `famille` changée, `inflight_demande=99`, `pic_workers_b=99`,
`pic_reduce_actif=99` ou jeton de profil inconnu passent pour la même raison :
le reçu décrit plusieurs fois le régime sans comparer ces descriptions à un
objet canonique unique. La correction utile reste courte : parser
`famille`/`parametres`, reconstruire l'argv attendu de chaque tuple, exiger
`liveness`, fermer le schéma de `profil_kind` et séparer l'identifiant stable
de sa glose avant une comparaison exacte. Une contre-fixture composée peut
couvrir les valeurs communes ; garder ensuite quatre dents causales courtes.

L'interpréteur gravé peut encore être un chemin absolu inexistant ou non
canonique, sans hash ni version. C'est une amélioration P2 de provenance du
relecteur autonome, pas un motif pour annuler la réagrégation ni pour bloquer
KeyCSR. Seule une nouvelle **mesure réutilisable** attend la dernière liaison
du régime ; le reçu `b79e29a5` demeure un diagnostic et le prototype
sémantique peut continuer.
