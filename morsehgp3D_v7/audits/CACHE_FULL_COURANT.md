# Cache FULL paresseux : qualification indépendante

5 septembre 2026, après `6f4b4de5`, sur le header gelé `13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627`. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures exclusivement dans ce dossier, sur `main`.

**Le port paresseux conserve les forêts FULL attendues sur 109 ordres indépendants, y compris avec un cache nul ou saturé.** Les deux copies nominales O2 et ASan/UBSan concordent octet pour octet ; les trois mutations privées sont réfutées pour leurs causes ciblées. Aucun défaut du producteur nominal n’est identifié. Ce résultat qualifie le raccord de la preuve d’alias à la demande au C++ capturé, sous l’autorité extérieure des catalogues complets, exacts et réguliers.

## Matrice exécutée et indépendance

Le [pont d’audit](full_lazy_bridge.cpp) consomme les catalogues rationnels et expose nœuds, niveaux, minima, parents, couvertures et racines aux coupes. Il ne passe ni par le générateur industriel ni par l’oracle C++ du constructeur. Le [juge indépendant](full_lazy_audit.py) réemploie explicitement, sous leurs hashes, les données des cent ordres précédemment établis contre Gamma ; il ajoute neuf ordres issus de deux nuages, dont les MEB et la régularité globale sont recalculées indépendamment. Le réemploi porte sur les fixtures, jamais sur un résultat d’exécution du nouveau code.

Les vingt cas donnent 109 ordres, de K1 à K=n avec n≤7. Deux représentations permutent les entrées/catalogues et changent les écritures rationnelles ; les PointId extrêmes et non consécutifs du corpus historique restent présents. Chacune passe dans EAGER et dans lazy C=0, C=1 et C=100000 : **872 sorties par build**, dont 654 lazy. Les quatre bras totalisent **67 920 coupes**, 6 792 nœuds, 4 336 minima et 5 920 références parentales par build. Les 654 comparaisons croisées concernent chacune un bras lazy face à EAGER ; les comparaisons EAGER avec lui-même ne sont pas comptées.

Les 200 représentations EAGER historiques retrouvent exactement leurs forêts et anciens compteurs. Les nouvelles recherches de directes sont comptées séparément ; aucune compatibilité binaire d’ABI n’est déduite de cette conservation sémantique. Les jugements normal et `-O` concordent. Les deux compilations nominales et leurs exécutions n’émettent aucun diagnostic ; ASan/UBSan garde `detect_leaks=1`. Les [sources, dépendances et clôtures](receipts_full_lazy_20260905/execution_closure.json) séparent ces copies privées des CTests constructeur.

## Ce que le port ferme

La [revue sémantique](receipts_full_lazy_20260905/semantic_review.md) vérifie les trois autorités permanentes : tokens des minima, ancres de toutes les directes après fermeture du lot, successeurs pour normaliser leurs anciens tokens. À K1, les minima sont les offsets des PointId triés ; aux autres ordres, l’ordre des tokens suit exactement celui des naissances du lot. Aucun de ces tokens n’occupe le cache facultatif.

Le miss à un intrus z calcule la MEB et le census de F, puis retrouve F+z par sa clé complète, son niveau égal, son antériorité stricte et son ancre installée. Sa normalisation précède l’emploi comme parent. Il n’y a ni nouveau MEB de F+z, ni naissance tardive, ni ancre du lot courant. Les ancres des connexions sans multifusion restent installées. Un cache plein rend la racine calculée sans insertion ; les dépenses ne sont pas remises à zéro.

La fixture ABC/ABW exerce exactement un miss J1, un MEB et un candidat de support, sans descente. Le nouveau lot partagé ABC/ABW/ABV utilise A=(0,50,0), B=(40,50,0), C=(20,61,0), W=(20,0,0), V=(20,10,30). ABW et ABV sont des directes de niveau 841 qui demandent toutes deux AB. Avec C=0, cette demande est recalculée ; avec un cache suffisant, le second emploi est un hit du même lot sur une racine strictement antérieure. Le contrôle rationnel vérifie tous les sous-ensembles de ce nuage.

E5 étendue conserve sa seconde itération de descente. Le premier candidat partagé du constructeur, avec C=(2,6,0) et V=(2,1,3), reste un négatif permanent : la boule de diamètre CV, au niveau 17/2, porte aussi A et B sur sa coquille. Il n’est pas introduit silencieusement dans les fixtures régulières.

## Travail et résidence : comparaison réellement observée

Les chiffres suivants agrègent les **218 représentations** de chaque bras, sans être des temps ni des expériences indépendantes supplémentaires.

| Bras | Visites strictes ou incidentes | MEB | J1 | Hits cache | Insertions cache | Skips |
| --- | --- | --- | --- | --- | --- | --- |
| EAGER | 5 026 | 22 | 0 | 0 | 0 | 0 |
| Lazy C0 | 2 096 | 94 | 72 | 0 | 0 | 82 |
| Lazy C1 | 2 096 | 88 | 66 | 6 | 36 | 40 |
| Lazy C100000 | 2 096 | 82 | 60 | 12 | 70 | 0 |

Chaque bras comporte douze étapes de descente. La suppression des alias égaux épargne leur installation, mais ajoute des MEB/census au premier besoin : ces petites fixtures montrent bien les deux termes de l’échange. C1 sature réellement, avec 40 skips sur 22 lignes et six hits. Aucun gain de temps ou de RSS n’est déduit de ces compteurs.

Sur les 218 comparaisons à cache suffisant, aucune insertion n’est sautée : les demandes J≥2 et les étapes restent celles d’EAGER, et `MEB_lazy = MEB_eager + J1_lazy`. Cette identité n’est exigée ni à C0 ni à C1. La capacité du cache ne remplace pas les limites cumulatives de travail. Les [bornes massives déjà prouvées](MONO_FULL_COURANT.md) restent des bornes de clés, distinctes de cette qualification et de la capacité d’un million configurée dans la nouvelle sonde constructeur.

En succès first-C, le juge indépendant contrôle `cache_inserts = min(C, portal_requests)` et la partition insertions/skips. L’identité des **premières** clés retenues est portée par la lecture du code ; les sorties du pont n’exposent pas le contenu interne de la table. Les minima, ancres, catalogues et certificats demeurent résidents ; leur nombre ne devient pas linéaire en n par cette transformation. Aucun Gamma global, mosaïque de Delaunay ou journal des cofaces silencieuses n’est ajouté.

## Budgets et mutants causaux

Quatre ordres nommés, dans les quatre bras, passent à leurs plafonds exacts : seize réussites, puis **180 refus cap−1** au motif attendu et avec forêt vide. Douze essais lazy refusent séparément `max_aliases=1` comme conflit d’API. Les profils exacts comparent les forêts et tous les compteurs ; les refus cap−1 contrôlent les plafonds, motifs et absence de résultat partiel. Leurs miroirs sur exceptions d’allocation relèvent séparément des reçus constructeur.

Trois copies privées du header isolent les mécanismes suivants :

| Mutation | Réfutation indépendante | Non-vacuité observée |
| --- | --- | --- |
| Supprimer la branche J1 | Refus produit nommé et ordre invalide | 96 sorties lazy touchées |
| Affecter le même token à tous les minima d’un lot | Structure FULL différente malgré un statut produit réussi | 54 témoins de structure incorrecte |
| Ignorer la capacité du cache | Identité first-C violée | 58 sorties dépassent leur capacité |

Les 218 sorties EAGER de chaque mutant restent identiques au nominal. Les codes processus, bruts JSON, identifiants et journaux sont vérifiés avant le jugement scientifique ; un flux cassé ne peut compter comme mutant réfuté. Le juge exige un motif ciblé et un témoin causal. Les sources littérales et patches restent reproductibles, normalement et sous `-O`.

## CTests, digest et prochaine décision

La [contre-vérification constructeur](receipts_full_lazy_20260905/constructor_review.md) ferme séparément 14/14 CTests Release et 14/14 ASan/UBSan : 81 appels lazy, 3 192 coupes, 127 rejets et 434 fautes persistantes d’allocation sur six cellules. Les 582 pins source et les six binaires de chaque build concordent. Le précontrôle initial 12/14 reste conservé. Le JUnit tronque la sortie allocation à 1 024 octets ; son préfixe est vérifié et les journaux complets restent disponibles. Ce détail de capture n’est pas un défaut produit.

La [contrelecture du digest et de la sonde](receipts_full_lazy_20260905/digest_probe_review.md) est favorable : niveaux rationnels normalisés, labels et topologie engagés, identifiants internes éliminés, entrée et ordres liés. La canonicité porte sur le wire d’une forêt valide ; l’égalité de SHA-256 reste une empreinte, pas une injection mathématique. La porte constructeur compare 672 divisions et réductions aux entiers Boost. Le juge de reçus ne relit pas les arènes ; il ne devient pas un nouvel oracle géométrique.

Les quatre lacunes du juge v1 sont corrigées dans le v2 relu. La relation first-C manque dans ce v2 gelé `8d8a612a` ; la [contre-fixture initiale](receipts_full_lazy_20260905/digest_review/replay_helpers.py) et sa corruption coordonnée d’un reçu réel restent conservées. La [porte supplémentaire contre-vérifiée](receipts_full_lazy_20260905/first_c_companion_review.md) ferme cette lacune **par composition avec le juge v2 et le sceau**, sans modifier rétroactivement leur capture. Elle exige l’égalité first-C sur les succès et seulement les bornes prospectives appropriées sur les préfixes refusés.

L’[admission indépendante de la nouvelle sonde](receipts_full_lazy_20260905/probe_admission_review.md) vérifie 24 reçus à n=8, soit 156 lignes d’ordre, onze refus de parsing et les digests entre quatre politiques et trois valeurs de s. Les horizons demandés Kmax=5/10 deviennent 5/8 sur ce nuage. Les 117 lignes lazy satisfont first-C, dont 81 avec portails non nuls. Le [paquet public des CTests](receipts_full_lazy_20260905/publication_binding.md) retrouve séparément les captures privées déjà closes. Ces deux paquets ont leurs inventaires et leurs autorités distincts.

La prochaine décision utile est la comparaison appariée des coûts avec cette même sonde, digest compris. Les grandes campagnes en cours du constructeur, leurs interruptions et leurs reprises restent hors de ce verdict ; aucun de leurs temps n’est épinglé comme résultat qualifié ici.

Ni complétude industrielle des catalogues, ni K9/K10 par oracle, ni verticale, masses, export de tour ou SLO ne sont promus ici. Les compilations et moteurs indépendants sont clos à **17:42:45 UTC**. GCP non utilisé.
