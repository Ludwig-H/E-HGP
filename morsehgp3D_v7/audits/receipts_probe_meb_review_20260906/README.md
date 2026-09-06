# Croissance FULL et audit des lecteurs MEB

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Ce lot ne lance aucune compilation, aucun moteur C++ ni benchmark. GCP non utilisé.

**La borne quadratique concerne bien les feuilles FULL explicites.** La [preuve indépendante](full_output_growth.md) construit, pour chaque K fixé≥2, m² labels minima sur N=2m+K−2 sites : deux arcs paraboliques et K−2 ancres intérieures communes. Les inégalités strictes persistent sous une petite perturbation rationnelle générique ; les fixtures non perturbées ne sont pas qualifiées globalement régulières. K1 reste différent. L’asymptotique exige une précision croissante et ne se transfère pas à l’univers u16 fini ni aux temps uniformes.

| Preuve | Portée exécutée |
| --- | --- |
| [Croissance rationnelle](full_output_growth.py) | 16 modèles, 1 360 labels, 34 720 puissances étrangères et 3 740 puissances d’ancres ; aucun Gamma exhaustif |
| Arcs liés u16 du gate v7 | Lecture entière indépendante : 9/25/81/289 paires strictement Gabriel à N=6/10/18/34 ; ni nouveau passage FULL, ni héritage d’exécution v6 |
| [Bornes de Work](work_bounds_review.py) | Huit modèles et 6 816 états des captures FULL antérieures, dont 1 152 préfixes refusés ; trois faux diagnostics acceptés par les deux lecteurs capturés sont réfutés |
| [Formats du comparateur](capture_format_review.md) | Cinq obstacles reproduits successivement sur un vrai microcas clos, en adaptant seulement en mémoire les obstacles précédents ; arrêt volontaire avant le juge primaire |

Les jugements normal et `-O` sont identiques : croissance `d37a08ec…`, Work `4fb44c48…`, anciens formats `3660b3e4…`. Les scripts épinglent leurs entrées ; aucune lecture d’un ELF n’est requise. Les données mathématiques, diagnostics d’admission et captures de géométrie gardent leurs autorités séparées.

## Empêcher de fausses économies MEB

Pour un ordre frais du dispatcher `f922544b`, les compteurs supplémentaires doivent satisfaire `p ≤ 146 × appels_FULL` et, avec d=c−A, `certified ≤ d ≤ 550 × certified`. F incrémente c et A du même delta. Chaque certificat ajoute à c seul un ordinal positif au plus égal à 550, éventuellement tronqué par la marge L encore positive. Aucun point de levée d’exception n’interrompt cette transaction arithmétique ; les exceptions d’observateur précèdent la certification. Un arrêt asynchrone au milieu ne produit pas de reçu clos.

Les lecteurs capturés `475b9288` / `9f54cb46` acceptent trois états impossibles : A effacé après un repli sans certificat ; p=1 sans appel FULL ; un certificat facturant 551 supports virtuels. Les bornes proposées les rejettent tout en conservant les 6 816 états relus. **Ce sont des lacunes des prédicats de diagnostic**, pas des défauts des forêts ni des corruptions observées dans les captures moteur. Les bornes sont nécessaires ; elles ne prouvent pas que tout état admis soit réalisable.

## Suivi des réparations

La contre-fixture de format reste attribuée au brouillon `be4b8712` et au contrôleur `ee9d4640`. La [réparation `910b30ac`](capture_format_repair_review.md) est contre-vérifiée sur 72 tentatives réelles, 48 paires et 312 ordres, avec 735 fichiers stables et différences recalculées. Les sorties constructeur normal/`-O` sont identiques. Cette lecture datée conserve ses limites : aucun refus réel, K effectif≤8, pas de paquet complet de rejeu portable ni nouvelle qualification des juges. Le défaut de format est clos ; la contre-fixture historique reste intacte.

Le [contrat de performance principal](../../docs/CONTRAT_PERFORMANCE.md) porte les jalons industriels. Le constructeur prépare son protocole de croissance et sa preuve à K2. Ce dossier conserve le complément indépendant à K fixé, les nouvelles paires u16 et les contre-fixtures des lecteurs ; il ne duplique pas leur protocole.
