# Contre-audit : admission FULL parallèle et vagues q3/q4

27 septembre 2026, reprise `fd1a2c7ee`. Lecture indépendante des nouveaux
prototypes, sans modification de moteur, source gelée, reçu ou build.
Conclusion sur les captures closes ci-dessous ; aucune qualification
CUDA n'en découle. Aucun benchmark lourd ni GCP lancé par cet audit.

## FULL : propriété des écritures

La voie normale de
[parallel.hpp](../b_full_parallel_parent_20260927/parallel.hpp) respecte
la séparation indispensable entre validation et émission.

1. Forme CSR et domaine sont contrôlés ; une continuation dans n'importe
   quel lot déclenche le repli général avant les objets de la voie rapide.
2. Sans continuation, `V=A` et `prior_count=batch_begin[b]`. Les champs
   de `batch[A]` appartiennent à des lots disjoints.
3. `first[V]` est initialisé avant la phase atomique. Les occurrences
   `p<V` réduisent le minimum par `atomic_ref<u64>` ; l'alignement requis
   est vérifié. Aucun accès ordinaire ne chevauche cette phase.
4. Après jointure, les validations ordinaires lisent ce minimum stable.
   Les erreurs sont privées par identité de worker. Les phases actions
   et en-têtes sont jointes avant réduction du minimum canonique.
5. Après admission seulement, les sorties sont allouées puis remplies.
   Les nœuds et contributions ont des plages disjointes. Chaque parent
   admis n'a qu'une occurrence : un seul écrivain par successeur.

Des opérations atomiques `relaxed` suffisent à cette réduction de valeurs
indépendantes : les jointures établissent la visibilité nécessaire entre
phases. Il ne s'agit pas de lire un minimum partiel pour décider en cours
de calcul. Les niveaux exacts, l'ordre des slots et les priorités de refus
restent ceux du prototype scalaire contre-audité.

Le scheduler cyclique découpe des grains disjoints. Le nombre de jobs
utilise quotient/reste, et la progression vérifie `jobs-job<=count` avant
l'addition. Il n'introduit pas un compteur partagé susceptible de déborder.
Les exceptions de worker restent privées ; une erreur de lancement
joint les threads déjà créés avant propagation. Les callbacks de l'encodeur
ne bloquent pas en attente d'un worker qui pourrait ne pas avoir été lancé.

Un défaut a été signalé avant gel : le mutant `IgnoreDuplicate` pouvait
admettre deux écrivains de successeur et créer une course au lieu d'une
divergence sémantique. Il est désormais limité explicitement à W1, et
la combinaison mutant/W>1 est refusée. Le chemin normal ne présentait
pas cette course grâce à l'admission préalable.

Le gate ajoute un rendez-vous de quatre threads distincts et un compteur
de workers ayant réellement traité des occurrences parents. Cela évite
de qualifier le parallélisme par le seul nombre de threads demandés.
Les fixtures de contention gardent une erreur de population avant les
réemplois futurs, afin de vérifier aussi le premier refus sous concurrence.

Après clôture, les lecteurs de la capture FULL parallèle r2 et de sa
capture TSan séparée ont été exécutés par cet audit en modes normal et
`-O`, sans relancer les binaires : **quatre PASS**. R2 ferme 14 commandes,
372 entrées et 2 232 comparaisons par build, 65 entrées acceptées,
307 refusées, 1 722 passages spécialisés, 420 replis et 90 refus avant
sélection de voie. Les trois mutants par build sont réfutés sémantiquement.
Le gate publie 13 644 créations de threads, 114 672 occurrences parents
vérifiées et quatre workers réellement actifs sur la réduction des parents.

La capture TSan ferme quatre commandes, avec gate identique à Release et
stderr vide. Son lecteur distingue bien un vrai PASS d'une indisponibilité
du runtime ; l'état observé ici est `TSan_qualified=true`. Cela qualifie
la recherche de courses sur ce corpus, pas une absence universelle de
courses ni le reste de la chaîne FULL. Le digest commun de r2 est
`3935095278205354091`. Les archives, builds et sources restent ceux des
[reçus propres](../../receipts/full_parallel_parent_20260927/README.md).

## FULL : portée exacte du travail et du parallélisme

`O(B+A+P+C)` décrit les visites et écritures **logiques**. Ce n'est pas
une borne uniforme sur les essais `compare_exchange_weak`. Un draft
invalide peut répéter massivement un parent ; les retries dépendent alors
du nombre de workers et du calendrier. Le modèle C++ permet aussi des
échecs parasites du CAS faible. Aucun compteur de retries n'est publié
par ce prototype.

Sur les drafts admis sans continuations, chaque parent est référencé une
seule fois : il n'y a pas de contention réelle entre occurrences sur une
même cellule. Cela ne transforme pas la primitive weak en une borne
déterministe sur toutes les exécutions abstraites C++.

Les copies/initialisations de certains vecteurs de sortie restent
sérielles. Le remplissage de `batch` découpe les lots ; validation et
scatter découpent les actions, et leurs boucles intérieures ne subdivisent
pas une grosse multifusion. Les threads sont créés/joints à chaque phase.
Il serait donc excessif d'annoncer toutes les étapes massivement
parallélisées. La capture appariée distincte sur les vrais drafts observe
ensuite, pour 08/000000 sans sol, 163,973 ms natifs contre 120,793 ms W4 :
sommes des médianes des encodeurs par K, pas temps mur FULL. Les rotations
natif/W1/W4 sont équilibrées et les sorties vérifiées contre la forêt
native. Ce levier reste distinct des vagues S2. La forte variabilité,
l'hôte local partagé et les trois sorties simultanément vivantes dans le
harnais interdisent une extrapolation directe à G4 ou à un pic mémoire du
moteur intégré. Voir les [reçus dédiés](../../receipts/full_parallel_real_drafts_20260927/README.md).

## Vagues : invariants relus

Le design et l'implémentation gelée de
[waves.hpp](../b_q34_arena_waves_20260927/waves.hpp) distinguent correctement
les objets suivants : préfixe brut avant filtre rectangle, P ouvert après
ce filtre, E réellement envoyé au filtre ponctuel, et S survivantes.
Le préfixe brut n'est pas `expanded_pairs`, qui conserve le sens natif P.

Les segments représentent soit une bande collective, soit un produit
fallback intégral. Un rectangle fermé ne réapparaît pas comme fallback.
La capacité Q ne borne que le scratch ; le curseur doit épuiser E même
avec Q=1. Les rangs originaux sont retrouvés via les permutations des
facteurs, puis utilisés pour la clé cartésienne avant tri de S. Le tri
doit précéder toute consommation par S3.

Les rejets de voie natifs se reconstruisent par `Pq-Eq` plus les rejets
ponctuels parmi Eq. La somme des voies n'est jamais la masse union.
Sur l'index natif à feuilles exactes, les rejets Pool sont déjà des rejets
du filtre ponctuel complet : la bonne gate compare donc S champ à champ,
dans l'ordre, avec le batch CPU natif, pas seulement un digest FULL.

Le stockage des résultats d'une vague pending doit permettre de reprendre
une réserve de sortie échouée sans recalcul de géométrie. Une exception
en milieu de `fill` ne constitue pas un tel checkpoint ; l'auteur a ajouté
un état d'échec interdisant cette reprise partielle.

## Points trouvés avant gel des vagues

- Deux boucles d'ordre dans `finish()` utilisaient `i=1; i!=S`. Pour S=0,
  elles lisaient hors limites. Elles ont été corrigées en `i<S` avant
  toute capture ; les gates doivent appeler `finish()` aussi sur sortie
  vide, pas simplement constater que `step()` est terminé.
- Une mesure après `keyed.reserve()` ne voit plus l'ancien buffer qui
  coexistait avec le nouveau pendant la réallocation. Le pic doit compter
  cette coexistence, ou être nommé empreinte aux points observés plutôt
  que pic réel. L'auteur a ajouté, avant gel, la capacité ancienne à la
  capacité nouvelle dans le calcul du pic transitoire du curseur.
- Le mutant qui réouvre une voie Pool avant S2 peut laisser S identique,
  puisque le filtre global referme cette voie. Sa réfutation naturelle
  est donc une incohérence d'Eq/requêtes physiques ; ne pas la présenter
  comme un résultat géométrique S incorrect.

La mémoire du curseur est distincte de celle de `Prepared` et de son pic
de construction : copie des requêtes, vecteur de plans à préparer, scratch
de l'arène et index partagé. La borne `O(R+F+C+D+Q+S)` n'est pas un RSS
et ne prouve pas que F, E ou S croissent sous-quadratiquement avec n.

Les fixtures prioritaires demandées couvrent Q=1 et plusieurs frontières,
fallbacks survivants, rejet Pool réel, masque mixte réduit à une voie,
permutation non identité, ordre de groupes différent de l'ordre original,
fermetures créant des trous d'ordinal, reprise pending sans nouvelle
géométrie et refus d'un résultat incomplet.

Deux trous du préflight ont été fermés avant gel : les formules
`fallback_ranks` et `original_ordinal`, effectivement employées par `fill`,
sont exercées pour un produit 2^32 et un ordinal 2^33+16 ; une fixture
native dédiée impose E>0/S=0 sur six capacités. Le premier test qualifie
ces formules u64, pas le parcours effectif de milliards de paires. L'injection
`step(true)` simule un `bad_alloc` au point pré-commit, même si aucune
croissance de capacité n'aurait été nécessaire ; elle teste la reprise
sans rejeu, pas l'interception d'une panne de l'allocateur système.

## Vagues : clôture indépendante

Les [reçus des vagues](../../receipts/q34_arena_waves_20260927/README.md)
ont été relus par cet audit avec `run.py --readback` en modes normal et
`-O` : **deux PASS**, sorties identiques. Quinze commandes ferment les
gates Release et Clang ASan/UBSan/LSan : 175 lots, 1 050 consommations,
355 632 requêtes ponctuelles et 195 948 survivantes comparées au natif,
ordre et masques compris. Ces valeurs cumulent les capacités et fixtures,
elles ne décrivent pas une trame distincte.

La couverture positive comprend 1 878 rejets Pool union / 3 622 par voie,
78 426 coupures de bandes, 4 104 inversions d'ordre, 216 consommations
E=0, six E>0/S=0 et 139 reprises pré-commit. Les six exécutions mutées
sont réfutées par les causes annoncées : quatre incohérences de requêtes
physiques et deux ordinals dupliqués. Le checker spécifique et son reçu
de huit corruptions ont également été relus ; aucune réécriture de reçu
ni nouvelle exécution de gate n'a été nécessaire à cette contrelecture.

Pas de défaut bloquant trouvé dans ce chemin gelé. Le curseur demeure
mono-consommateur ; les gates ne qualifient ni CUDA, ni un appel concurrent
à `step`, ni FULL après raccord. Une exception dans `fill` ou dans la
finalisation engagée empoisonne le curseur ; un appel prématuré à `finish`
est seulement refusé avant cette finalisation. Seule la reprise pré-commit
est exercée comme checkpoint.

## Décision de port GPU : une couture précise, pas une variante de plus

**Porter d'abord ce consommateur établit l'exactitude et la mémoire du
raccord ; cela ne justifie pas d'activer « préparation CPU + S2 GPU »
comme optimisation acquise.** La préparation collective précédente coûte
environ 277 ms W4 sur l'hôte local. On ne peut pas la comparer directement
au CPU G4, mais rien ne prouve que quelques dizaines de ms économisées
dans S2 la financeraient. Déplacer ou réduire cette préparation reste une
étape nécessaire avant de présumer un bénéfice net.

Le Pool réduit P vers E, pas S : puisque S reste exactement la sortie du
filtre ponctuel complet, ce raccord ne diminue pas, à lui seul, les entrées
de S3/S4 ni les objets FULL. Les 100 ms exigent aussi le levier FULL et les
autres coûts aval. Aucun nouveau micro-ajustement q2 n'est proposé ici.

### Transport minimal et unités

Le prochain port doit transporter les mêmes objets, sans reconstruire
d'ancien `Plan` ni développer P/E sur l'hôte :

- Index partagé : coordonnées par rang, nœuds, et liens escape requis
  par l'aval. La préparation Pool GPU devra aussi disposer des IDs
  originaux par rang pour les départages exacts des témoins ; l'actuel
  `FilterInput` S2 ne les porte pas, contrairement à `LanesInput`.
- Rectangles : plages originales A/B, masque ouvert, `raw_base` u64 et
  indice d'arène/tag fallback. Conserver les rectangles fermés dans la
  convention du préfixe brut, sans leur attribuer de segment vivant.
- Arène : offsets de facteurs/classes, rangs locaux groupés, crédits
  compactés et bandes. Segments : rectangle, bande/tag fallback et fin
  cumulée u64. Ces tableaux sont proportionnels à R/F/C/D, jamais P/E.
- Vague : ordinal u64, deux rangs et masque conservé jusqu'à compaction.
  Sortie : seules les S survivantes, avec leur ordinal jusqu'au tri.

Les produits, offsets globaux et masses sont u64 contrôlés. Les rangs
locaux restent u32 dans un domaine validé ; rang, ID original et offset
global ne sont jamais interchangeables. Le domaine GPU actuel réserve
`absent32`, plus strict que le simple maximum u32. Les primitives de scan
appelées avec un compte `int` doivent recevoir des tranches sûres, pas
un cast silencieux ni un plafond global supprimant des données. Les
limites actuelles de compte S aux interfaces S3/S4 restent à traiter
explicitement pour le très massif : une vague S2 ne les abolit pas.

Le propriétaire immuable doit rester vivant jusqu'à la fin de tous les
kernels et transferts. Une vue issue d'un temporaire CPU ou une sortie
partiellement écrite ne peut pas devenir un résultat publié.

### Étapes parallélisables et ordre obligatoire

1. Construire les préfixes des segments, puis distribuer des plages d'E
   aux blocs GPU. Un bloc peut amortir le décodage d'un segment ; chaque
   candidate retrouve toujours ses deux rangs originaux et son masque
   propre. Ni masque6 par bande union, ni disparition des singletons.
2. Exécuter S2 exactement une fois par candidate de la vague. Conserver
   les résultats, effectuer scan/compaction de cette vague, puis écrire
   les survivantes dans des plages réservées. Un échec de réservation
   ne doit pas déclencher un second passage géométrique non comptabilisé.
3. Trier les S survivantes par ordinal original, endpoints et masque
   sous la même permutation, **avant S3**. Payer la mémoire de tri et la
   coexistence éventuelle des formats. Le fait qu'une bande soit ordonnée
   localement ne rétablit pas l'ordre cartésien entre classes.
4. Publier ensemble S et ses bilans : P logique, E physique, masses par
   voie et rejets `(Pq−Eq)+rejets_S2_sur_Eq`. Adapter le raccord natif,
   qui écrit encore `witness.pairs.queries=expanded` dans
   [wspd_q34.cpp](../../src/gen/pipeline/wspd_q34.cpp), au lieu de laisser
   ce champ annoncer P tests quand seuls E ont été exécutés.

L'actuel chemin compact de
[filter_runner.cu](../../src/gpu/filter_runner.cu) réserve encore un
masque, un flag et une position par paire P. C'est précisément la couture
à remplacer ; les buffers par vague ne doivent pas s'ajouter à ces anciens
tableaux cachés. Aucun besoin de modifier le prédicat exact pour ce port.

La préparation collective doit ensuite être réellement portée : sélection
top-K des témoins par facteur avec score exact et départage par ID,
écriture unique des crédits, histogrammes/prefixes des classes, dispersion
stable, comptage puis émission des bandes. La fusion de top-K partiels
est possible en conservant le même ordre total ; elle ne permet pas
d'approximer les scores ni de refaire la géométrie au second scan.

Pour FULL, le raccord distinct reste : CSR du draft et niveaux exacts,
réduction globale « aucune continuation », premières occurrences parents,
réduction canonique des erreurs, **barrière d'admission**, puis écriture
disjointe des nœuds/parents/successeurs/contributions explicites. Repli
général conservé en présence de continuations. Cette écriture ne qualifie
pas automatiquement la génération des drafts ou les liens verticaux.

### Coût total à conserver dans le prochain verdict

Le consommateur visite E et trie S ; la préparation visite les facteurs
et les témoins. La mémoire annoncée O(R+F+C+D+Q+S) doit inclure les buffers
de tri/transfert nécessaires au port, avec le nuage/index à part et leurs
coexistences publiées. Cette écriture n'est pas une borne sous-quadratique
en n : ni F, ni E, ni S ne sont ainsi bornés. Les amas presque quadratiques
restent un contre-régime observé, non corrigé par le format.

Le prochain verdict utile est donc un raccord exact puis une chaîne
appariée : préparation, allocations, transports, tri, fallback et tour
explicite inclus ; même trame entière, masque de sol, grille, K et s.
Publier le lieu réel de la sortie explicite et payer son transfert si le
contrat la consomme côté CPU. Garder segmentation et HGP distincts puis
leur total, ainsi que les coupes capteur pour la croissance. Ni somme de
temps workers, ni encodeur isolé, ni seules paires évitées ne peuvent être
rebaptisés contrat FULL 100 ms.
