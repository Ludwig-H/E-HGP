# Contre-vérification de la condensation : contraction des plateaux

Référence source figée : `8bb4618e5c6c7c2bc8a5d7a1c2e189a4249d0047`.
Les sept fichiers C++ de `source/` sont les blobs exacts de ce commit.
Cette capture est une contre-vérification indépendante minuscule d'un problème
déjà couvert par la référence indépendante `developer_rebound/condensation_reference`
(publication `d3e59eb49`), pas un nouveau verrou inédit.

## Résultat et portée

Deux entrées PointDendrogram valides portent douze points de poids unitaire.
A et B ont chacun trois points ; D en a six. Tous naissent au rayon² β=1.
Dans la représentation factorisée, C réunit A et B à β=25, puis la racine
réunit C et D au même β=25. C a donc une durée nulle.
Dans le quotient aplati, la racine réunit directement A, B et D à β=25.

Le noyau figé accepte les deux entrées. Les 144 hauteurs par paire sont
identiques : 1 à l'intérieur de chacun des trois groupes, 25 entre groupes,
0 sur la diagonale conventionnelle. Les coupes ouvertes et fermées aux
cinq valeurs 0,1,4,25,26 sont également identiques. Ces valeurs couvrent
tous les états et les deux plateaux de cette fixture ; le raisonnement par
hauteurs prouve l'identité de toutes les coupes, pas seulement les cinq sondes.

Pour mcs=5, EOM, racine exclue, z=1 puis z=2, la représentation factorisée
sélectionne C et D et affecte les douze points à deux clusters. Le quotient
aplati ne sélectionne aucune branche et produit douze points bruit.
Le noyau choisit explicitement la feuille condensée C de stabilité zéro ;
ce comportement observé est conservé, sans changement silencieux de
politique leaf/EOM ni attribution à une bibliothèque HDBSCAN non appelée.

L'oracle Fraction raisonne sur les composantes strictes de l'ultramétrique :
à β=25, seul D atteint mcs ; D poursuit donc la racine, sans nouveau cluster.
A et B sortent à λ=1/5 (z1) ou 1/25 (z2), D à λ=1.
La stabilité de la racine aplatie est 36/5 ou 156/25.
Les stabilités factorisées sont respectivement (12/5,0,24/5) et
(12/25,0,144/25). Cela établit un défaut d'invariance API à la contraction
des plateaux, pas la réalisation de ces entrées par un nuage 3D ou Γ/FULL.

Deux compilations et exactement deux invocations C++ capturées, normale et
UBSan, code0, diagnostics vides, sorties identiques. Chaque capture a quatre
lignes (deux encodages × deux z). Chaque juge contrôle 576 hauteurs de paire
et 20 comparaisons de coupe. Quatre juges capturés normal/−O passent ;
une première lecture des mêmes archives avait déjà passé avant leur capture
finale. Aucun appel moteur supplémentaire dans les lecteurs.

Le premier préflight a échoué à la compilation sur un avertissement
d'indentation du probe privé : zéro invocation native. Son probe et ses
sorties sont conservés dans `preflight/`. La correction privée sépare les
deux instructions ; aucun des sept fichiers moteur n'a changé. Sources,
compilateur et binaires sont hachés avant/après ; les binaires restent hors
archive. Aucun GCP, aucune nouvelle géométrie, aucun benchmark statistique,
aucun contrat GPU/FULL/100ms qualifié.

Un premier lecteur a refusé la liaison des logs : la création par apply_patch
avait transporté les sorties vides en fichiers d'un octet newline. Cette
lecture en échec est conservée dans reader_preflight.json. Les fichiers ont
ensuite été rendus réellement vides par apply_patch ; les textes capturés
dans les reçus et les résultats natifs n'ont pas changé.

## Plan de condensation sans carré déplacé

Préconditions : les rangs sont des classes de niveaux réellement exactes,
pas des doubles arrondis fusionnés. Contracter les arêtes parent/enfant de
même rang, donc produire un événement N-aire atomique avant tout test mcs.
Une map des représentants se prépare top-down en O(H), sans montée par point.
L'API autorise point_rank=rank(parent(owner)) : après quotient, réattribuer
ce point au parent distinct vivant à ce rang, puis au représentant déjà
préparé. Grâce à la validation et aux rangs désormais stricts entre nœuds,
un seul parent distinct suffit. Cette réattribution est O(n), avant counts ;
sinon une masse sortant au split gonflerait artificiellement un enfant.

Calculer bottom-up counts(v)=masse_directe(v)+Σcounts(enfants).
Un tri global des points par rang de départ décroissant, puis une distribution
stable CSR par owner, fournit les cohortes directes en ordre λ croissant.
Par branche, intégrer masse_active×Δλ ; le cumul remplace, et n'ajoute pas,
l'ancien cumul individuel w(λ_sortie−λ_naissance). Au départ d'une cohorte,
si les survivants sont strictement inférieurs à mcs, terminer le cluster et
éjecter tous les survivants à la même date. Survivants=mcs reste vivant.

À un split structurel atomique : au moins deux enfants de masse ≥mcs créent
de nouvelles branches ; un seul prolonge la branche courante ; aucun termine.
Les petites composantes sortent à la date du split. Lors d'une terminaison
anticipée, ne parcourir que le suffixe des points directs non traités et
les sous-arbres encore intacts : ne jamais reprendre les cohortes sorties.
Les charges d'étiquetage/drop sont ainsi disjointes ; chaque point sort
une fois, chaque nœud est soit traité soit abandonné une fois.
Après préparation, condensation O(H+n), mémoire O(H+n), donc O(H+n log n)
avec ce tri de comparaison. Aucun invariant ne borne ici H par O(n).

L'EOM se calcule bottom-up sur les branches condensées. Une propagation
top-down du premier ancêtre sélectionné évite les montées répétées par
branche et par point, puis label(x)=label(point_cluster[x]) est O(n).
Exclusion de la racine, ties EOM et règle des feuilles de stabilité zéro
restent des politiques explicitement déclarées ; ne pas les déduire de
cette seule fixture. Les poids géométriques/duplications doivent également
avoir une sémantique déclarée avant comparaison à des singletons HDBSCAN.

## Lecture autonome

`python3 -B verify.py`, puis `python3 -B -O verify.py`.
Le lecteur vérifie l'inventaire exact, tous les SHA, les commandes/codes/
dates/captures, les pins avant/après et le préflight conservé avant tout
rejeu. Il ne recompile rien et n'appelle aucun binaire natif.
`record.py` et `run_judges.py` sont des sources historiques, pas des
commandes à relancer pour lire cette archive. Aucun build privé n'est requis.
