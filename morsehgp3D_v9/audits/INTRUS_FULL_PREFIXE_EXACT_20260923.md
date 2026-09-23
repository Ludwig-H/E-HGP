# FULL : préfixe exact d'intrus réutilisable par BallKey

23 septembre 2026. Proposition mathématique et lecture du moteur v9
`5ab4326c`, sans modification de celui-ci ni mesure de vitesse nouvelle.
Le [premier reçu G4](../receipts/g4_tower_r1_20260922/README.md)
`08/000000`, K10, `tower_static_threads=48` donne 7 096 687 requêtes
d'intrus et 403 429 577 nœuds visités (56,85 par requête) ; ces sommes
couvrent **tous les ordres K2..10**, pas K10 seul. La sonde ne publie ni
le nombre de BallKeys distinctes interrogées ni leur récurrence. Le cache
ci-dessous est une piste conditionnelle à cette mesure, pas un gain acquis.

## Lemme de préfixe

Pour un index `CloudIndex` immuable et une clé exacte `b`, ordonner les
positions uniques par leur rang Morton `u`. Soit `I_b` la suite croissante
des `u` tels que `b.power(upos[u])<0`. Pour toute facette triée `S` de
taille K, `intruder_work(b,S)` rend exactement le premier élément de
`I_b \ S`, ou −1 s'il n'en existe pas. En effet, son DFS empile le fils
droit puis le gauche ; l'arbre radix sépare les intervalles consécutifs
`[first,last]`. Le rejet `lo>=0` ne retire aucun intérieur ; sur `hi<0`,
le parcours de la plage croît aussi en `u`. Les contacts `power=0` restent
exclus. Sources : `src/tower/forest/full_ball_tower.hpp:547–569`,
`src/tower/tree/cloud_index.hpp:180–211`,
`src/tower/pipeline/census.hpp:80–114`.

Supposons qu'une recherche normale ait rendu `z`. **Tout intérieur de
rang inférieur à `z` appartient nécessairement à `S`**. On peut donc
former, sans autre parcours de l'arbre, le préfixe *complet* de `I_b`
jusqu'à `z` : tester exactement `b.power(upos[u])<0` pour les seuls
`u∈S, u<z`, puis ajouter `z`. Cela demande au plus K tests de puissance
supplémentaires. Lors d'une prochaine requête `(b,S')`, si le préfixe
contient un élément hors de `S'`, son premier tel élément est **la réponse
identique** au parcours original. Sinon, exécuter ce parcours original,
obtenir `z'>z` et étendre le préfixe avec les seuls intérieurs de
`S'∩(z,z')`, puis `z'`. L'invariant reste exact. Un préfixe de K+1
intérieurs suffit à toutes les facettes de cet ordre par le principe des
tiroirs ; pour plusieurs ordres, borner l'entrée à `Kmax+1=11` IDs.
Si le parcours rend **−1**, il a épuisé l'index : tous les intérieurs
éventuels appartiennent à `S`. Le port le plus simple propage −1 sans
créer ni modifier d'entrée de cache ; les appelants FULL actuels le
traitent ensuite comme un échec d'invariant. Le lemme d'extension
ci-dessus ne s'applique qu'aux réponses `z≥0`.

Ne pas mémoriser simplement le dernier intrus : si `I_b=(2,5,9)`,
`S=(2,7)` rend 5 mais `S'=(7,8)` doit rendre 2, même si 5 n'appartient
pas à `S'`. Le préfixe complet issu du premier appel est `(2,5)` ; il
rend 2 correctement. Un cache `(BallKey, intrus)` sans les intérieurs
antérieurs modifie donc la chaîne de résolution, même si une éventuelle
équivalence de terminales était espérée.

L'[oracle combinatoire autonome](check_full_intruder_prefix_20260923.py)
énumère `n=2..7`, toutes les suites non vides d'intérieurs et toutes les
paires de facettes de taille `1..min(4,n)` : **1 336 782 paires PASS** en
Python normal et avec `-O`. Il vérifie construction, hit et extension du
préfixe. Cette vérification finie ne remplace pas le gate différentiel
sur le vrai `CloudIndex` et ses bornes exactes.

Pour éviter K tests sur les clés vues une seule fois, une entrée bornée
peut d'abord garder `(BallKey, S, z)` **non matérialisé**. À la deuxième
rencontre exacte, calculer les tests des sites de `S` avant `z` et
transformer l'entrée en préfixe complet. Avant cette matérialisation,
`z` seul n'est jamais un raccourci de réponse. Une table à correspondance
directe, privée par worker et par index, peut évincer librement : une
collision ne crée qu'un défaut de cache. Si elle survit entre ordres, la
clé doit rester liée au **même** index immuable ; entre nuages, `BallKey`
seule ne suffit pas. La mise à jour doit être construite localement puis
publiée en un seul état valide ; sur échec de capacité, poursuivre sans
cache. Aucun résultat géométrique, ancre ni lot FULL n'est publié par le
cache.

Le coût du cache est un compromis : pour une clé jamais répétée, copie
de clé et de facette, hachage et éviction s'ajoutent au parcours ; pour
une clé récurrente, les hits économisent `AxisBounds` et les visites de
nœuds. Avec une entrée fixe d'environ 80 octets de clé et au plus 11 IDs
de 32 bits, plusieurs milliers d'entrées **par worker** sont déjà des
dizaines de MiB sur 48 workers. Fixer un budget total explicite, puis
mesurer ses collisions et sa résidence réelle. Une table globale
concurrente pourrait partager plus de clés mais ajoute synchronisation
et propriété transactionnelle ; elle ne doit pas être le premier port.

## Raccourci plus simple pour une clé déjà cataloguée

`resolve`/`static_terminal` cherchent la clé dans `by_key` **avant**
`intruder_work`. Si elle y est mais n'a pas de terminal admissible à cet
ordre, le `BallData` du contrat FULL contient déjà sa liste **globale
exacte** d'intérieurs stricts. Chercher le plus petit rang `u` de cette
liste hors de `S` rend la même réponse, sans descente d'arbre ni cache.
Ne pas prendre le premier élément stocké : `ball_census` peut émettre les
IDs en ordre DFS inverse ; l'API de `BallData::interior()` ne garantit
aucun ordre Morton. Sélectionner explicitement le minimum Morton.
Le constructeur vérifie l'appartenance des IDs fournis mais suppose leur
**complétude globale** comme précondition du catalogue ; ce raccourci
n'est valide qu'à cette frontière de confiance, que la chaîne actuelle
recensus exactement avant l'appel FULL.

La MEB de `S` enferme ses K sites, d'où `K≤p+u` pour cette même boule.
Une clé trouvée mais non admissible ne peut donc échouer que sous la
borne basse `K<p+q_min−1`. À K10, tous les enregistrements admis dans
le catalogue satisfont `p+q_min−1≤10` ; les requêtes d'intrus de cet
ordre portent sur des **clés absentes** du catalogue. Prioriser le
raccourci catalogué pour les ordres bas, mais ne pas lui attribuer les
visites K10 sans histogramme par ordre.

## Porte avant implémentation industrielle

Sur la même trame et les mêmes options, instrumenter **par K** les
requêtes d'intrus, BallKeys distinctes, histogramme des fréquences
1/2/3/≥4, doublons observés par worker, nœuds visités par classe de
fréquence et fraction des clés absentes du catalogue. Une capture exacte
de 7,10 M clés coûterait environ 568 Mo bruts pour les seuls cinq `i128` ;
une sonde diagnostique peut échantillonner déterministiquement par hash
de BallKey (tous les appels d'une clé retenue), puis trier ce sous-ensemble
en publiant fraction, variance entre trames et limites de l'estimation.
Ne pas déduire les répétitions des seuls `intruder_queries` ou
`resolver_cache_hits` : ce dernier cache est indexé par **facette** et
la voie statique l'évite.

Si les récurrences justifient le port, comparer cache désactivé/activé
sur les mêmes entrées et ordres. Le gate différentiel doit comparer
**chaque intrus rendu** sur les hits et les replis (y compris cas de
contact, préfixe épuisé, évictions et 48 workers), puis digest, BallIds
terminaux, compteurs de sortie et statuts. Séparer requêtes logiques,
parcours réellement effectués, hits, nœuds économisés, tests exacts
ajoutés, octets du cache et temps CPU/mur. Les compteurs historiques de
travail géométrique doivent payer les tests supplémentaires ; une baisse
de `intruder_nodes` seule ne prouve pas une baisse du temps FULL.

Le [préchauffage géométrique par K](PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md)
et ce cache sont compatibles : `static_terminal` tient déjà pile et
statistiques privées par worker. Les éventuelles répétitions d'une même
BallKey peuvent toutefois se disperser entre workers ; mesurer la
récurrence **dans la même lane** avant de déduire un taux de hits d'un
histogramme global. Les 403 M visites cumulées ne sont pas éliminées par
la seule lecture des `BallData::interior()` du catalogue.
