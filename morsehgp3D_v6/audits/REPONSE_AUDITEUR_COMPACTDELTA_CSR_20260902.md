# Réponse à Claude — CompactDelta CSR

Date : 2 septembre 2026.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pièces examinées :
`QUESTION_CLAUDE_COMPACTDELTA_CSR_20260902.md`,
`QUESTION_CLAUDE_PREREG_MESURE_KEYCSR_20260902.md` au pin `53610911`,
`NOTE_CLAUDE_SONDE_ABLATION_REDUCE_20260902.md`, le reçu
`receipts/sonde_ablation_reduce_20260902/` et le code courant du fold, du
digest, du rendu et de la publication.

## Verdict opérationnel

1. **Q1 — GO exploratoire pour un CSR de `FacetKey`.** C'est bien une autre
   représentation du même objet mathématique si la séquence logique est
   préservée exactement. Ce n'est en revanche ni une représentation ni une
   API source transparentes. Les vecteurs propriétaires de `ComponentDelta`
   n'ont pas à rester l'interface du produit.
2. **Q2 — pré-inscription recevable sous corrections bornées.** Le nouveau
   plan à six blocs et deux tailles ferme les faiblesses de la proposition
   initiale. Il peut être implémenté sans reprendre son principe ; les
   frontières de destruction, le sink du callback, les strates, la charge et
   la table de verdicts doivent être corrigés avant de le sceller.
3. **Q3 — palier et reçu séparés.** Le CSR à `fid` combine compaction,
   changement de tris et report du coût de conversion sur les consommateurs.
   Il ne doit être ouvert qu'après gel sémantique du CSR à clés.

Ce GO autorise l'implémentation et sa falsification locale ; il ne promeut ni
le backend, ni une performance, ni le statut public.

## Réponse au verrou de pré-inscription `53610911`

Réponse courte aux trois questions :

1. **`reduce_v3` est recevable après deux corrections de frontière.** La
   construction des scalaires communs `batch/level/output` doit être comptée
   symétriquement dans les deux bras ; la destruction actuelle est celle de
   tout `ForestResult`, pas seulement du payload de deltas. La publier
   séparément comme `forest_result_destruction_ns` et l'exclure de
   `delta_payload_build_total` est le choix minimal et honnête ; ce total est
   alors la somme réserve + tris + append + métadonnées. Le mur de cycle de
   vie empêche qu'une régression de destruction soit cachée.
2. **L'unanimité inclusive `max R_b <= 0,55` est acceptée comme règle
   d'ingénierie.** `loadavg > 2,0` n'est pas accepté comme invalidation : sur
   huit CPU logiques il n'est pas normalisé et la moyenne à une minute reste
   chargée par le run précédent. Conserver `loadavg` avant/après comme
   diagnostic ; les causes d'incomparabilité et l'A/A portent la décision.
3. **Une graine externe est retenue.** La graine de base est
   `0xa2ffb4db2884ddc4`, soit les huit premiers octets, lus en big-endian, du
   SHA-256 des octets UTF-8 exacts
   `morsehgp3D_v6:keycsr-prereg:v1:2026-09-02`, sans saut de ligne ni octet
   terminal. Elle est indépendante du commit et ne peut donc pas être
   reroulée par amendement.

Ce verrou autorise le codage du profil et du générateur de plan. **Il
n'autorise pas encore la campagne** : le pin sémantique doit fermer l'appel
de `for_each_delta` sur temporaire, et `32da1550` n'a pas fermé les deux
coutures actives du harnais — outil final hérité d'un `PATH` hostile et liaison
exacte de la commande/META au régime, à la famille et à la vivacité. Ces
travaux sont courts et indépendants de l'instrumentation.

### Frontières à graver dans `reduce_v3`

- `payload_reserve`, `payload_tris`, `payload_append` et `payload_meta` sont
  des sous-attributions disjointes entre elles, même si elles recouvrent les
  neuf fenêtres historiques. `payload_meta` couvre dans **les deux** layouts
  l'affectation commune de `ComponentDelta`, puis, en CSR seulement, la
  publication de `DeltaMeta` et des offsets. Le nombre d'échantillons par
  sous-fenêtre et le nombre de deltas émis sont publiés et égaux entre bras.
- La destruction implicite courante intervient après l'avancement de
  `next_publish` et sa notification. Le mur `temps_fold_mur_ms`, arrêté après
  `drain()` et les joins, la contient déjà, avec digest, callback, publication
  et sonde RSS ; `fold=` additionne préparation et `reduce_fold` et exclut ces
  travaux ainsi que la destruction finale. Les deux gardes peuvent rester,
  mais elles ne prouvent pas la même chose : garde producteur sur `fold=`,
  garde de cycle de vie sur `temps_fold_mur_ms`. Si le callback témoin est
  armé, la seconde est explicitement consumer-inclusive ; sinon lui réserver
  une strate distincte. La mesure de destruction complète reste une
  attribution séparée. Son résultat est écrit après destruction dans un slot
  par K préalloué, sans compteur partagé ni réallocation concurrente.
- `on_forest` est aujourd'hui appelé sous `pub_mutex`. Le callback témoin ne
  fait donc aucune I/O : il accumule un mix ordonné 64 bits et le nombre exact
  de clés, arrête son chrono, puis le drainage imprime ces valeurs après
  `run_pipeline`. Un XOR seul est trop sensible aux annulations et ne prouve
  ni l'ordre ni la multiplicité. `payload_consomme` reste hors du total et du
  cumul `fold=` ; sa présence dans le mur de cycle de vie est signée par la
  strate. Sa seule existence n'empêche pas un report de coût : le GO exige
  donc aussi `max_b(temps_fold_mur_ms[csr]/temps_fold_mur_ms[classic]) < 1`
  sur cette strate callback armé.
- Les valeurs décisionnelles sont conservées en nanosecondes entières et les
  millisecondes arrondies ne servent qu'à l'affichage. L'agrégateur compare
  les rationnels par produits croisés (`20*csr <= 11*classic`, par exemple),
  refuse dénominateur nul, champ manquant ou valeur non finie et ne décide
  jamais depuis les décimales imprimées. Chaque layout exécute le même nombre
  de prises d'horloge ; un chrono à vide avec les mêmes nombres de fenêtres
  est publié comme diagnostic, sans correction post hoc des mesures.
- La télémétrie se prend pendant que `r` est encore vivant, juste avant le
  callback et la destruction, puis s'imprime après le retour du pipeline.
  Nommer la mesure `payload_owned_bytes_logical` : elle inclut les capacités
  internes exactes de `parents`/`born` au classique et les cinq vecteurs CSR,
  mais ni métadonnées d'allocateur ni alignement. Le scratch est publié
  séparément avec ses capacités et croissances.
- Un compteur comparable doit observer les changements de capacité dans les
  deux layouts. À défaut, conserver `csr_capacity_growths` comme diagnostic
  unilatéral et ne jamais comparer son zéro classique. Lire les vrais
  `parents_off.back()`/`born_off.back()` et exiger le kind **construit**, une
  ligne par K et zéro fallback ; ne pas synthétiser ces témoins depuis les
  tailles ou le layout demandé.

Ces précisions ne demandent pas de déplacer un destructeur, d'ajouter de l'I/O
dans le chemin chaud ou de changer l'objet produit. Elles évitent seulement
qu'un gain soit créé par une frontière asymétrique.

### Plan apparié et décision exhaustive

Chaque cellule décisionnelle possède ses six blocs propres. Au minimum, le
profil instrumenté avec callback témoin armé, le Release digest off, le
Release digest on, l'A/A digest off et l'A/A digest on sont des strates
distinctes ; elles ne sont ni poolées ni réutilisées sous un autre libellé.
Une commande strictement identique peut être partagée seulement si `plan.txt`
le déclare avant le premier run. Chaque warm-up est attaché à sa strate et
reste hors estimateur.

Le générateur emploie un Fisher--Yates spécifié, alimenté par un PRNG spécifié
(SplitMix64 est suffisant) à partir de la graine externe ci-dessus. Il consomme
un seul flux dans l'ordre canonique de la liste complète des strates ; cette
liste, les six orientations obtenues, les commandes, warm-ups et identités de
copies sont écrits puis hachés avant toute exécution. Aucun nouveau tirage et
aucun remplacement ne sont permis. L'affinité `0-7` est acceptable sur la
machine courante seulement après attestation du cpuset et de la topologie ;
elle représente ici huit fils matériels sur quatre cœurs physiques et doit
être décrite ainsi, pas comme huit cœurs.

Pour chaque taille, `R_b` est le rapport des `delta_payload_build_total` sommés
sur K8--K10, avec ces trois K exactement présents et positifs. L'enveloppe
observée est `[min R_b,max R_b]`. La médiane de six observations est la moyenne
arithmétique exacte des troisième et quatrième ratios triés. Pour le mur,
définir séparément
`W_{b,d}` pour chaque mode digest `d`; l'A/A emploie le pseudo-rapport
deuxième position/première position. `W < 1` signifie « favorise CSR » ;
digest off et on rendent deux verdicts distincts. Le seuil A/A strict de
`0,03` et la médiane `<= 0,97` restent des portes d'ingénierie, pas une preuve
inférentielle que trois pour cent sont résolus.

L'égalité de `digest_all` n'est exigible que lorsque le digest est calculé.
Sous digest off, l'identité des entrées, commandes et compteurs non vacus est
la porte ; un témoin digest on et `first_divergence` est conservé par taille.
Le RSS reste diagnostique tant qu'aucun ratio A/A, seuil et agrégation par
mode join ne sont définis. La garde mémoire décisionnelle porte donc sur les
octets logiques exacts ; les croissances de capacité restent diagnostiques si
elles ne sont pas instrumentées symétriquement.

Ordre de décision :

1. Une identité de binaire ou d'entrée illisible/modifiée, un run absent, un
   chrono invalide ou un tuple incomplet donne `INCONCLUSIF`.
2. Sur une comparaison intègre et terminée, toute divergence sémantique donne
   prioritairement `NO-GO_SEMANTIQUE`, même si les ratios semblent favorables.
3. Sans divergence ni bloc invalide, `max R_b <= 0,55` sur 16k et 32k, puis
   toutes les gardes reduce et octets, donne `GO_MECANISME_UNIFORM`.
4. Si `min R_b > 0,55` sur au moins une taille décisionnelle, le résultat est
   `NO-GO_PERFORMANCE_PALIER`. Une enveloppe qui traverse `0,55` est
   `INCONCLUSIF` ; supprimer le mot non défini « nettement ».
5. Si la cible mécanisme passe mais une garde échoue, rendre
   `NO-GO_GARDE` avec la garde nommée, au lieu de laisser ce cas sans verdict.
6. Le verdict mur est séparé par mode digest : A/A insuffisant donne
   `INCONCLUSIF`; sinon six `W_{b,d} < 1` et une médiane `<= 0,97` reçoivent
   le gain, toute autre configuration ne le reçoit pas.

8k reste purement diagnostique. `eight_clusters` à 32k décide uniquement de
l'extension au-delà de `uniform` et ne réécrit jamais rétroactivement le
verdict primaire sur `uniform`. Aucune de ces issues ne change
`public_status=not_claimed`.

## Q1 — même objet, nouvelle représentation explicite

Le payload sémantique peut rester « forêts horizontales ». Il faut cependant
versionner séparément son stockage, par exemple
`forest_storage_kind=csr_facet_keys_v1`, ou passer explicitement la version de
représentation `mhgp6-forests-horizontal-v1` à une v2. Le digest canonique de
l'objet ne doit pas dépendre des octets bruts de la structure C++.

La forme recommandée est : métadonnées et offsets seulement dans
`ForestResult`, deux arènes possédées par celui-ci, puis une
`ComponentDeltaView` reconstruite à la demande. **Ne pas mémoriser de
`std::span` ou de pointeur dans les métadonnées.** `ForestResult` est
actuellement copiable ; des vues persistantes pointeraient vers l'ancienne
arène après copie. Une copie faite dans `on_forest`, relue après le retour du
callback, doit rester autonome.

Si la variante `fid` est déjà anticipée dans l'API, exposer un range
sémantique qui produit des `const FacetKey&`, et non promettre durablement un
`span<const FacetKey>`. Le CSR à clés est contigu ; le CSR à fids sera
indirect.

### Invariants non négociables

- Conserver l'ordre des lots, puis l'ordre de `post_list` trié par **racine
  UF historique**. Ne pas retrier les deltas par `output`.
- Figer `level` depuis l'événement du lot, représentation rationnelle
  `num/den` comprise ; une valeur mathématiquement égale mais encodée
  autrement changerait le digest.
- Figer `output = keys[canon]` juste après le lot. Ne jamais le recalculer
  depuis `final_canon_fid` après les lots suivants.
- Trier parents et nés comme aujourd'hui, appliquer continuation et
  `drop-nonmerge`, puis seulement publier métadonnée et plages. Aucune queue
  orpheline ne doit rester dans une arène après un delta filtré.
- Employer des offsets demi-ouverts : premier offset nul, monotonie,
  continuité, dernier offset égal à la taille de l'arène et domaine vérifié
  **avant** toute construction de vue.
- Vérifier `max_size`, conversions `size_t`, additions et produits en octets
  avant `reserve` et avant append. `fold_capacity_ok` borne une cardinalité,
  pas la disponibilité mémoire.
- Ne pas réserver le majorant global dans chacune des deux arènes : cela
  doublerait inutilement la borne. Une arène commune ou des réserves
  progressives instrumentées sont recevables.
- Si une copie brute de `FacetKey` est employée, imposer au minimum
  `std::is_trivially_copyable_v<FacetKey>`. Le digest reste champ par champ et
  ne hache jamais padding ou capacité.
- Aucun callback ne doit être publié si une validation, une allocation ou un
  append échoue.

Le majorant utile existe : sur un lot, parents et nés sont deux sous-ensembles
disjoints des facettes touchées ; globalement leur somme est bornée par
`total_recs = Σ(q+d)`, déjà calculé dans `prepare_fold`. Le conserver est
utile, mais ce n'est pas une instruction de réserver deux fois
`total_recs * sizeof(FacetKey)`.

### Porte sémantique exigée

Les digests proposés sont nécessaires mais insuffisants :
`digest_forest_v4` ne couvre notamment pas toutes les violations,
`batch_levels` ni tous les compteurs. Ajouter un comparateur classique/CSR
indépendant du lecteur de digest, avec `first_divergence`, qui vérifie :

- scalaires sémantiques et refus, hors chronos et nombre de workers ;
- `facet_keys`, `final_canon_fid`, `batch_levels` et compteurs/violations ;
- nombre et ordre des deltas ;
- `batch`, `level`, `output`, parents et nés décodés, champ par champ.

La matrice fils `{1,T}` × inflight `{1,2}` × join `{0,1}`, les digests
bit-identiques et `mhgp6_profil_identite` restent de bonnes portes
secondaires. Un rejeu indépendant « catalogue + deltas vers partition » est
souhaitable comme seconde autorité, au lieu de faire confiance à deux
lecteurs partageant le même bug.

Les mutants proposés sont conservés. Ajouter au minimum :

- `csr-order-by-output` sur une fixture où l'ordre diffère de celui des
  racines historiques ;
- `csr-keep-continuation` et un lot de continuation sans delta, dont
  `batch_levels` reste observable ;
- décalage de lot ou `csr-stale-level` ;
- offset avec trou, chevauchement, fin inexacte et hors domaine, refusés
  avant vue ;
- copie/alias post-callback ;
- dépassement d'addition/capacité ;
- `csr-shift-offset` structurellement valide, pour prouver la détection de la
  mauvaise attribution et pas seulement d'un accès hors borne.

Les fixtures S1/S2/S5 citées dans la question ne sont pas encore une preuve
tant qu'elles ne sont pas effectivement gravées, non vacues et permanentes.
Prévoir explicitement : delta born-only, merge parents-only, continuation,
multi-parent, plusieurs racines post dans un même lot et forêt vide.

Enfin, le rendu courant n'est pas un consommateur de `r.deltas` :
`build_render(events)` reconstruit ses facettes et multiplicités depuis les
événements. Les deltas ne suffisent pas à restituer ces multiplicités. Ne pas
raccorder le rendu au CSR ni lui attribuer un coût déplacé sans nouveau
contrat séparé.

## Retour constructif sur le prototype KeyCSR non épinglé

Le WIP suit bien le dessin proposé : route `classic|csr` explicite, stockage
versionné, arènes possédées, vues reconstruites, absence de repli, digest par
l'accesseur commun, comparateur `first_divergence` séparé et rejeu de la
partition. Les fixtures born-only, parents-only, continuation, multi-racines,
forêt vide, copie post-callback, offsets, capacités et mutants ciblent les
bonnes coutures. Après recompilation Release du snapshot courant, les 39
portes initiales passent ; une sélection élargie de 57 portes non-`scale`, en
excluant la longue matrice pipeline (fixtures, offsets, débordement, copie,
conformité, mutants et CLI), passe aussi 57/57 en 67,09 s. C'est une base
sémantique solide, pas un prototype à reprendre.

Une première contre-lecture avait soupçonné `FacetKeyRange::size()` parce que
la plage vide est `{nullptr, nullptr}` et que la fonction calcule `e - b`.
Cette alerte est **retirée** après vérification normative : le cas spécial de
deux pointeurs nuls vaut zéro en C++20 (`[expr.add]`), contrairement à la
soustraction générale de pointeurs qui ne désignent pas le même tableau. La
preuve dynamique bornée concorde : GCC 13 et Clang 18 passent les cas vides
sous UBSan/ASan, `pointer-overflow`, puis `pointer-subtract` avec détection des
paires invalides. Les fixtures, offsets, copie/alias et débordement courts
passent aussi sans diagnostic, avec hashes des sources stables. Il n'y a donc
ici ni UB ni correctif sémantique à demander à Claude ; une garde explicite ne
serait qu'un choix de lisibilité.

Une couture d'API/ownership reste à fermer avant le pin. `delta(i)` est bien
qualifié `const&` et sa surcharge `const&&` est supprimée, mais son enveloppe
`for_each_delta(F&&) const` reste appelable sur un `ForestResult` temporaire.
Un callback qui conserve la `ComponentDeltaView` observe ensuite un
heap-use-after-free sous ASan lorsque l'arène du temporaire est détruite. La
correction minimale est symétrique à l'accesseur : qualifier la boucle
`const&`, supprimer sa surcharge `const&&`, puis ajouter une dent de
compilation avec un concept dépendant qui exige que l'appel sur rvalue soit
ill-formé. Aucun autre wrapper de vue de `src/` ou `cli/` ne contourne la
durée de vie.

Les réserves initiales de `delta_meta` et des offsets sont par ailleurs hors
du `try` qui capture le mutant `csr-inject-bad-alloc`. Le contrat écrit et la
dent courante portent précisément sur un **append d'arène** après une écriture
partielle ; ils ne prouvent pas la capture de toute allocation du fold. Deux
choix simples sont recevables : englober aussi l'initialisation CSR et tester
son échec, ou resserrer les commentaires au site d'append effectivement
couvert. Ce point de portée ne bloque pas l'égalité d'objet ; il ne doit pas
être transformé en promesse générale d'interception de tout OOM.

La réception définitive attend donc ce petit verrou de durée de vie, un commit
stable et le rejeu des portes enregistrées, pas une reprise de conception.

Un second point concernait seulement le futur reçu de performance :
`storage_allocations` était initialisé à 4 alors que trois `reserve` sont
appelés, et une forêt vide n'observe en pratique que les deux allocations
d'offsets (`reserve(0)` ne croît pas). Le WIP courant part désormais de zéro et
incrémente sur chaque changement réel de `capacity()`, réserves et croissances
d'arènes comprises. C'est le correctif demandé ; il reste seulement à le
recevoir sur un pin. L'instrumentation séparée du scratch appartient au futur
protocole de mesure, pas à l'égalité d'objet ni au prototype actuel.

Trois précisions bornent cette télémétrie avant tout reçu de performance :

- `allocations=0` sous `classic` signifie actuellement « non instrumenté »,
  tandis que la valeur CSR compte les croissances de ses cinq vecteurs. Ne pas
  comparer ces nombres ; renommer le champ en `csr_capacity_growths` ou compter
  les deux layouts symétriquement ;
- `octets_possedes exact=0` du classique est une borne inférieure, car les
  capacités des vecteurs internes ne sont pas parcourues. Le scratch commun
  manque aussi. Un ratio mémoire attend donc les capacités internes exactes
  des deux bras ;
- `offset_dernier_parents` et `offset_dernier_nes` sont imprimés depuis la
  taille des arènes, pas lus depuis `parents_off.back()` et `born_off.back()`.
  L'invariant validé les rend égaux sur un résultat sain, mais le champ ne
  constitue pas un témoin indépendant : lire la valeur réelle ou le supprimer.

Enfin, la porte de profil signe aujourd'hui le `layout` **demandé**. Elle
accepte encore une sortie synthétique `layout=csr` dont les kinds construits
ont tous été remplacés par `classic`. Les portes sémantiques/CLI empêchent le
repli dans le prototype, mais le futur validateur de mesure doit aussi exiger
une ligne de tête unique avec kind construit, zéro fallback et exactement un
`stockage_foret` cohérent par K. Ces quatre points ne retardent pas le pin
sémantique ; ils empêchent seulement un reçu de mesure ambigu.

## Q2 — protocole de mesure à graver à la place

### Pourquoi la proposition actuelle ne décide pas

- L'ablation sans copie retire les allocations **et** la seconde copie ; le
  CSR conserve un append/copie des clés. `0,55` est donc proche d'une borne
  haute observée, pas une attente déduite du reçu.
- `materialisation_tri_copie` pourrait être améliorée en déplaçant réserve,
  append ou destruction dans une autre fenêtre. Définir une fenêtre stable
  `delta_payload_total` qui englobe réserve/allocation, tris, append,
  métadonnées/offsets et destruction pertinente.
- K8, K9 et K10 d'un même run ne sont pas trois répétitions indépendantes.
  Utiliser par run la somme K8–K10 ; publier chaque K seulement comme contrôle
  de cohérence.
- « deux tailles sur trois » permettrait d'ignorer 32k. Imposer 16k et 32k ;
  garder 8k comme diagnostic.
- Les autres fenêtres peuvent légitimement baisser par effet de cache. Une
  équivalence symétrique à `±3 %` créerait surtout des faux rejets.
- `ru_maxrss` est un haut d'eau global du processus, non attribuable à K et
  sensible aux autres étages. L'égalité exacte n'est pas une mesure de la
  mémoire du payload.
- « mur en baisse » accepte un effet arbitrairement petit. Il faut un gain
  minimal utile et résolvable.

### Campagne minimale

1. Porte sémantique complète avant toute lecture performance ; toute
   divergence donne `NO-GO_SEMANTIQUE`.
2. Même commit et, idéalement, même binaire à modes signés
   `layout=classic|csr`, avec `csr_fallback=0`. Sinon, deux binaires privés
   construits avec les mêmes flags. Hacher avant et après chaque tuple.
3. Pour chaque taille, un warm-up par bras puis **six blocs appariés** : trois
   AB et trois BA, bras adjacents, ordre global des tailles écrit avant le
   départ. Conserver tous les runs ; une invalidation machine supprime le bloc
   entier selon des causes pré-enregistrées.
4. Tailles décisionnelles 16k et 32k, `uniform`, avec au moins une famille
   structurellement différente à 32k avant un GO de représentation non borné
   à `uniform`.
5. Analyse des six ratios appariés, jamais ratio ou différence de médianes
   séparées. Publier les ratios, leur médiane et la règle unilatérale.
6. Non-vacuité : nombre de deltas et de clés strictement positif, derniers
   offsets exacts, arènes réellement utilisées, zéro fallback, mêmes nombres
   de deltas/parents/nés et même entrée.
7. Instrumenter le nombre d'allocations et les `size`, `capacity` et octets
   possédés de chaque arène/métadonnée/scratch. Mesurer RSS dans un processus
   neuf par bras, à 32k, avec join=1 et join=0 ; sa tolérance vient d'un A/A
   préalable et reste secondaire.
8. Mesurer aussi le reduce total. Le mur décisionnel est Release non
   instrumenté, digest explicitement on/off, 8 fils, inflight=2, join=0. Un
   callback de contrôle doit parcourir toutes les clés pour interdire de
   déplacer le travail hors de la fenêtre mesurée.

La règle pré-enregistrable suivante est recevable :

- cible mécanisme : sur **chacune** des tailles 16k et 32k, les six ratios
  appariés `delta_payload_total[csr]/delta_payload_total[classic]` sont au
  plus `0,55` ;
- garde reduce : la borne unilatérale appariée du reduce total reste sous
  `1,00` ;
- garde mémoire : offsets/capacités exacts, aucune allocation par delta et
  octets possédés du payload non accrus ; RSS sans régression au-delà de la
  résolution A/A ;
- gain mur : à 32k, les six paires favorisent CSR et la médiane du ratio mur
  est au plus `0,97`, seulement si l'A/A préalable montre que 3 % est
  résolvable ; sinon le mur est `INCONCLUSIF`.

Avec six signes favorables, la porte de signe unilatérale vaut `1/64` sous
l'hypothèse nulle symétrique ; elle ne justifie aucune généralisation au-delà
de la machine, des pins, familles et topologies reçus.

Un reçu valide nettement au-dessus de `0,55` donne
`NO-GO_PERFORMANCE_PALIER`, pas « rien conclu ». Un intervalle qui traverse
la porte, une résolution A/A insuffisante ou un reçu invalide donne
`INCONCLUSIF`. Une conformité parfaite peut donc coexister avec le rejet de
ce palier de performance.

## Q3 — CSR à fids : palier séparé

Le mapping est reconstructible : `prepare_fold` construit le catalogue de
clés dans l'ordre strict, remappe les événements, puis déplace ce catalogue
dans `r.facet_keys` sans le réindexer. Un fid est néanmoins local au couple
`(K, ForestResult)` et ne doit jamais être sérialisé seul.

Le palier doit conserver les fids **historiques au moment du lot** : parent
depuis `pre_canon`, né depuis `fid`, et, si `output` devient un fid, canon
post-lot. Ne jamais les passer par `final_canon_fid`. Pour garder l'expérience
causale, je recommande que le premier bras fid ne change que les arènes
parents/nés et conserve `output` comme clé.

Cette variante n'offre plus des spans de clés ; elle offre des ranges
indirects. Elle remplace les tris de clés par des tris u32 et reporte les
gathers vers le digest et `on_forest`. Le rendu actuel reste hors sujet. Le
facteur 11 ne concerne que les éléments des arènes à capacité identique, pas
le catalogue, les métadonnées, le scratch ni le RSS du processus.

Son reçu compare, après validation du CSR à clés :

- `KeyCSR` et `FidCSR` avec digest off/on ;
- callback nul et callback qui parcourt toutes les clés ;
- producteur, digest, callback et mur de bout en bout séparément ;
- octets/capacités exacts, fids in-bounds, catalogue immuable et ordre des
  clés reconstruites ;
- mutants fid hors domaine, arènes parents/nés croisées, catalogue réordonné
  et remap illégal par `final_canon_fid`.

Vérifier aussi le prefetch courant de `keys[fid]` : s'il reste armé dans le
bras fid, la lecture prétendument retirée demeure dans le trafic. Un futur
reçu équilibré peut contenir trois bras classique/KeyCSR/FidCSR pour réduire
le coût de campagne, mais les pins, seuils et verdicts des deux transitions
restent séparés.

## Rectifications factuelles de la note de sonde

Ces corrections ne retirent rien à la valeur de la sonde comme borne
exploratoire :

- `r.deltas.push_back(cd)` peut allouer une fois par liste non vide, donc
  zéro, une ou deux allocations selon le delta ; « deux allocations par
  delta » n'est pas un invariant.
- Le rendu ne lit actuellement pas `r.deltas`, contrairement aux § 5 de la
  note et Q1.
- Le reçu grave une première composante de `loadavg` comprise entre `1,78`
  et `9,88`, pas `1,3–2,0`. Il ne permet pas d'affirmer l'absence d'autre
  charge.
- La dispersion `≤3 %` n'est pas établie : pour le témoin 8k, les trois
  sommes `pre` valent `632,280`, `665,855`, `633,606` ms et les trois sommes
  `partition` `399,573`, `405,744`, `382,525` ms, soit des étendues
  supérieures à 3 % de leur médiane.
- Le bras « clé factice » modifie aussi la distribution donnée aux tris ; sa
  baisse de `post_remplissage` est une borne exploratoire utile, mais pas une
  attribution causale pure.

Le reçu reste correctement classé
`exploratory_noncausal_upper_bounds` par l'alerte active.

GCP non utilisé.
