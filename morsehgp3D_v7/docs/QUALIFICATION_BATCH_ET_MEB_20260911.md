# Lots complets et réduction du travail MEB

11 septembre 2026, complément après `324f6192`. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La sonde complète par lots et son test census→FULL sont maintenant compilés en CUDA strict, après qualification locale. Une optimisation MEB indépendante est aussi vérifiée. **Aucune de ces nouvelles sources n'a encore tourné sur GPU ; aucun nouveau temps 50k n'est disponible.** Les trois tentatives G4 de cette étape sont closes, sans benchmark exécuté.

## Le raccord complet, pas seulement une primitive

Le [paquet sonde et gate](../receipts/terminal_batch_probe_20260911/README.md) conserve les sources privées, captures et échecs. Il utilise le Builder actif `83f1c78e`, le terminal semé `d73de05f` et le transport résident `a62bd1d5`. La sonde `21d0a5dd` mesure la capture froide, les callbacks par K, le reste du constructeur et la fermeture. Le reste n'est pas appelé « calendrier pur » : il contient aussi préparation, tri, semis et restitution.

La gate compacte `98e426f2` construit les vrais census CPU, puis compare cache scalaire, statique CPU et batch dans le même processus. Elle contrôle directement les identités terminales avant toute normalisation de composantes, puis les banques/populations, niveaux, nœuds, parents, successeurs, contributions et verticales. La référence CPU reste distincte du terminal composé. Ce n'est pas une nouvelle preuve universelle de complétude WSPD.

| Épreuve locale | Résultat propre à cette source |
| --- | --- |
| Gate O2 et ASan/UBSan ROOT, LSan actif | 372 536 contrôles par build ; 10 fixtures, 20 paires physiques, 46 732 nœuds et 28 666 contributions comparés |
| Terminales directes | 10 326, dont 1 752 à K9 et 1 758 à K10 ; 59 lots et neuf contextes tous fermés |
| Semis après échange | Q=2 911 recherches, H=715 hits ; même travail payé entre statique et batch |
| Refus causaux | BallId admissible mais erronée après readback, puis panne après un premier lot payé : code 4, aucune tour partielle publiée ; CLI invalide : code 2 |
| Sonde entière et gate compilées/liées sous NVCC | SM120, avertissements hôte/device fatals, zéro diagnostic ; aucune dépendance Boost/oracle dans ces unités, aucun ELF CUDA exécuté |

Le premier échec de compilation de la fixture, dû à une conversion rétrécissante de PointId, est conservé. La corruption de BallId est injectée sur hôte après transfert ; elle n'est pas présentée comme un mutant arithmétique exécuté dans le kernel. Les compteurs de transport locaux sont émulés, pas des débits GPU.

## Réduction exacte q2 et confinement prioritaire

La [variante MEB privée](../receipts/meb_diameter_20260911/README.md) ne reprend pas la récursion de l'auditeur. Pour tester un support de deux points, seule la première paire de distance maximale doit être essayée. Si une telle boule contient tous les sites, toute autre paire maximale y est antipodale et définit la même boule ; si la première échoue, les autres échouent aussi. Conserver le premier maximum sur égalité préserve le support canonique. La [preuve complète](../receipts/meb_diameter_20260911/sources/current/PROOF.md) et les tests restent liés à leurs sources.

Le confinement teste les deux extrémités en premier, puis tous les autres sites dans leur ordre initial. Cela ne supprime aucun point et ne double aucun élément de coquille. Les parcours de supports q3/q4 restent inchangés. Le balayage des distances est un travail supplémentaire, facturé séparément.

Ce prépass porte sur au plus dix sites d'une facette, soit 45 paires, pas sur les n points du nuage. Il ne construit aucune table globale de paires, mosaïque de Delaunay ou incidence Gamma supplémentaire. Cela borne ce coût local, sans démontrer une borne sous-quadratique de toute la sortie FULL dans tous les régimes.

O2 et SAN ROOT donnent les mêmes 52 488 contrôles et 6 416 comparaisons MEB : 161 cas avec juge rationnel Gram indépendant, puis 6 255 appels réellement rencontrés dans trois census n32, s8/10/12, K1..10. Les cas à diamètres égaux, permutations, triangle aigu, bords u16 et contre-exemple K7 sont permanents. Quatre mutants réfutent dernier maximum, distance non facturée, ancien ordre sous la nouvelle comptabilité et double coquille. Ce troisième mutant est comptable, pas une réfutation de l'ancien algorithme.

| Travail dans ces seules comparaisons | Nominal | Variante |
| --- | ---: | ---: |
| Supports formés | 437 473 | 293 135 |
| Tests de puissance | 579 018 | 258 574 |
| Distances du nouveau prépass | Sans ce prépass | 158 088 |

Il ne s'agit ni d'un benchmark de tour ni d'une baisse démontrée du nombre de registres CUDA. Les sources et ELF sont épinglés avant/après, mais compilateur, Boost et en-têtes système ne sont pas épinglés dans cette campagne : le paquet ne prétend pas être une fermeture hermétique d'outillage.

Cette variante n'est pas intégrée. Son raccord devra ajouter `diameter_pairs` au travail persistant, protéger sa fusion inter-workers et versionner la comptabilité, sans perdre ce coût dans la sonde ou le transport GPU. Le juge des supports canoniques doit rester indépendant du juge du nouvel ordre de travail. Les anciens snapshots GPU gardent leurs anciens comptes ; un changement CPU seul ne les requalifie pas.

## Tentatives G4 et sobriété

Le [worker préparé et ses tests locaux](../receipts/terminal_batch_worker_20260911/README.md) emportent un instantané immuable de 59 fichiers, sans ELF ni Boost. Il prévoit gate sur carte, paire n200, tour 50k K1..10, repli K1..5, puis paire CPU48/GPU et s8/10/12 selon le temps utile restant. Ce programme n'a pas été exécuté dans les tentatives ci-dessous. Les compteurs, codes, sources et frontières de comparaison sont contrôlés, indépendamment du temps observé.

Les [reçus G4 clos](../receipts/terminal_batch_g4_20260911/README.md) distinguent exactement les trois générations du projet `devpod-gpu-exploration` :

| Cible SPOT | Génération `lastStartTimestamp` | Résultat |
| --- | --- | --- |
| `us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35` | `2026-09-11T06:51:01.668-07:00` | Préemption immédiate, avant gardes invitées et calcul ; arrêt ciblé certifié |
| Même cible US | `2026-09-11T06:53:44.713-07:00` | Préemption immédiate, aucun worker exécuté ; arrêt ciblé certifié |
| `europe-west4-a/ehgp-blackwell-spot` | `2026-09-11T06:59:41.712-07:00` | Doubles gardes vérifiées ; outils existants requis non trouvés au précontrôle ; aucune compilation ni commande de benchmark ; arrêt ciblé certifié |

Le wrapper européen épingle le contrôleur d'origine et ne change que sa cible fixe ; ses tests locaux passent en Python normal et `-O`. La VM européenne existante a été réutilisée sans en créer une nouvelle. Les durées étaient GCE STOP/3600 s et invité 30 minutes ; la fenêtre de 900 s est une politique de travail économique, pas un nouveau plafond algorithmique ni une durée totale de facturation.

L'erreur européenne ne précise pas lequel des outils manque : ne pas la transformer en diagnostic CUDA certain. La [révision locale du diagnostic](../receipts/terminal_batch_tool_discovery_20260911/README.md) consigne désormais les chemins et outils manquants avant refus, sans ajouter d'installation ou relâcher les critères. Tests Python normal/`-O` : 44 contrôles, 239 rejets et 512 combinaisons d'admission ; cinq refus dans le vrai `main()` avec gardes et outils simulés vérifient l'écriture du diagnostic. Elle n'est pas réattribuée à l'ancien reçu. Pas de quatrième redémarrage aveugle. La clé OS Login propre à cette session est révoquée, ses deux fichiers locaux supprimés ; l'inventaire final ne détecte aucune autre VM E-HGP active.

## Contrats et suite

Les dernières tours 50k complètes restent celles du [10 septembre](RESULTATS_TOUR_CACHE_G4_20260910.md) : environ 419 s pour K1..10 et 34 s pour K1..5 sur l'ancien moteur CPU/hybride census. Ni 1 s, ni 100 ms, ni plusieurs dizaines de millions de points ne sont qualifiés. Les tests locaux 8k/16k/32k et s8/10/12 précédents gardent leurs sources propres, sans transfert de temps à ce nouveau prototype.

Prochaine exécution utile : obtenir la disponibilité d'une G4 avec les outils requis, qualifier les vrais lots sur carte, puis mesurer la tour entière. Le calendrier CPU, la génération WSPD et la résidence de sortie restent des chantiers distincts ; accélérer les seules MEB ne les efface pas. L'[auditeur](../audits/NOTE_CLAUDE_COEUR_MEB_20260911.md) étudie en parallèle un proposeur certifié avec repli : ses versions, compteurs et temps ne sont pas confondus avec la variante q2 ci-dessus.
