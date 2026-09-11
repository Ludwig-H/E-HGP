# Lot terminal résident avec semis — qualification privée hôte

Cadre `phase=exploration_v7_hors_registre`, `backend=HOST_STUB`, `profile=quantized_u16_input_only`, `public_status=not_claimed`. Ce paquet privé est créé seulement après fermeture O2 **et** SAN du raccord cumulatif. Aucun fichier produit ou audit n'est modifié ; aucune commande Git, compilation CUDA, exécution device ou session GCP ne fait partie de ce paquet. Les sources contiennent une future branche CUDA, mais les présentes captures compilent exclusivement son stub hôte.

## Objet qualifié et sources

Le callback synchrone reçoit, par K, les demandes initiales triées-uniques non résolues par les semis initiaux. Un propriétaire unique conserve l'index, le catalogue et les semis complets exacts ; les positions sont des indices géométriques Morton, jamais des PointId. Le terminal privé effectue MEB, clé/niveau exacts, lookup, intrus et descente, avec le raccourci de semis après échange. Le moteur hôte conserve la construction des demandes, le calendrier et la publication de la forêt FULL.

Les sources finales visibles sont sous `sources/current/`, dont le cœur privé `83f1c78e…`, le terminal semé `d73de05f…`, SeedOwner `f2ac463b…`, la route `a62bd1d5…`, l'adapter atomique `993786f3…` et le juge cumulatif `49b35f00…`. O2 r5 et SAN doivent consommer **la même fermeture complète**, pas seulement ces six fichiers. Les autres scripts/READMEs copiés dans les snapshots sont des matériaux de provenance ; leur présence n'attribue aucun de leurs anciens résultats à ce raccord.

## Campagne cumulative

Chaque compilation finale exerce **28 044 contrôles**, 64 paires de forêts, 84 appels de lot, 176 demandes effectivement résolues et huit hits post-échange. Les 16 géométries comprennent les 14 petits cas historiques, ABEZW et la coquille carrée partiellement occupée ; les variantes de PointId et d'ordre des BallId sont appariées. La construction est rejouée en modes statiques 1 et 4. Les catalogues de cette gate viennent du juge Gram borné, pas d'un nouveau grand census produit.

Les payloads de forêt et les tableaux R/U/S/Q/H/T sont confrontés à la référence CPU semée. Les 176 BallId et ordinaux terminaux sont également comparés **directement** au terminal CPU c03, pas seulement à un digest de forêt. Le juge vérifie que les MEB c03 payées égalent celles du raccord plus les hits semés, et compare séparément le reste du travail à la référence semée. Q/H/T sont comparés en tableaux, mais seuls les huit H agrégés sont imprimés : aucun total Q/T non exporté n'est inventé par le lecteur.

La qualification finale contient **41 rejets transactionnels et un débordement synthétique distinct**. Les 41 comprennent 21 sorties omises/corrompues aux premier/milieu/dernier slots, six fautes de transport, une entrée périmée, quatre échecs de construction partielle, l'échec de la deuxième allocation, une libération non certifiée persistante, six omissions après reuse/growth et un rollback d'initialisation. Le débordement est une faute artificielle de comptabilité, pas un coût géométrique supposé réalisable.

La totalité du lot est prévalidée puis la totalité des sorties est vérifiée avant publication. Aucune erreur ne publie un préfixe. Le travail brut reste inconnu tant que tous les slots ne sont pas valides ; après cela l'adapter agrège temporairement et ne publie `work_known` qu'après succès de toute la somme contrôlée. Une erreur de publication hôte conserve le travail exact déjà vérifié. Si le travail ne peut pas être observé ou représenté complètement, il n'est pas présenté comme zéro exact.

## Réduction de trafic, pas chronométrage

Une nouvelle allocation initialise ses sentinelles une seule fois. Les lots suivants utilisent un epoch u64 strictement croissant, sans wrap ; une sortie omise reste non écrite ou porte un ancien epoch et est refusée. Le futur `cudaMemset` n'est pas un transfert H2D ; son stub utilise `memset`. La gate de réutilisation porte sur 24 requêtes cumulées : **2 496 octets H2D = 24×104**, et **2 552 octets d'initialisation locale = 11×232**, lors de deux réserves et quatre allocations. Le D2H reste confronté à 24×232 octets dans le juge.

Le H2D de sentinelles économisé vaut 232 octets par requête. La copie de publication passe de Result 208 octets à une cible compacte 16 octets, soit 192 octets de charge utile hôte évités par requête. Ce sont des tailles et volumes vérifiés sur le stub, **aucun gain de latence GPU ou de tour complète n'est mesuré**. Le buffer de diagnostic, les demandes hôte et la dernière copie compacte de l'adapter restent présents. La mémoire résidentielle totale n'est pas qualifiée : index/catalogue/semis et staging ne sont pas intégralement compris dans le compteur partiel de capacité du callback.

## Historique intégral, sans promotion rétroactive

| Capture | Résultat exact | Portée |
|---|---|---|
| ROOT o2_r1 | compilation 1 | erreurs strictes de signedness et enum `kOk` inexistant ; aucun test lancé |
| ROOT o2_r2 | 0/0/0/2/2 | 27 650 contrôles, 28 rejets ; avant agrégation atomique et comparaison directe |
| ROOT o2_r3 | 0/0/−6 | le juge interceptait `std::overflow_error`, mais la faute était `full_ball_detail::Failure` ; SIGABRT conservé |
| ROOT o2_r4 | 0/0/0/2/2 | 28 003 contrôles, 34 rejets et un overflow synthétique, 176 identités directes ; avant réduction de trafic |
| ROOT o2_r5 | 0/0/0/2/2 | qualification cumulative 28 044 contrôles |
| ROOT san_root_r1 | 0/0/0/2/2 | même fermeture et mêmes observations que r5 ; ASan/UBSan, LSan actif |
| Trafic isolé o2_r1 | 0/0/0/2/2 | 27 691 contrôles et 35 rejets sur base r2 ; ni atomicité r4 ni 176 comparaisons directes héritées |

Les suites à cinq codes sont version compilateur, compilation, `--selftest`, argument inconnu et argument absent. Le reçu du test isolé conserve explicitement son import du snapshot r2. Les sources historiques restent celles effectivement compilées ; les diffs entre révisions et les deux deltas isolés sont conservés séparément. Les deux échecs ne sont ni effacés ni convertis en gates causales de réussite.

## Relecture portable

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /tmp/mhgp7_batch_neuf
```

Le lecteur contrôle les sept reçus, leurs **30 commandes**, streams et codes exacts, chaque snapshot, les pins des six ELF non distribués, la fermeture finale O2/SAN, l'import isolé, les planchers et les observations chiffrées. Il utilise des conditions explicites, pas `assert`, et ne recalcule aucune géométrie. Les sources sont dédupliquées en objets ; `storage_map.json` permet une extraction create-only. Les captures ROOT épinglent le binaire après compilation ; le publieur reconfronte les ELF présents à ces pins avant de n'en conserver que les hashes. Le témoin trafic possède en plus un pin de binaire après exécution. Le SAN ROOT impose LSan via son recorder source-backed, sans désactivation.

Les commandes exactes de reproduction restent dans chaque reçu. Le recorder courant est `sources/current/record.py` et sa TU est `sources/current/prototype/batch_gate.cpp` ; Boost est nécessaire au juge local, jamais embarqué dans le paquet. Aucune exécution CUDA n'est possible par ce recorder. Une compilation puis exécution réelle du raccord résident devra qualifier sa nouvelle ABI, la résidence, les epochs et les erreurs sur carte, dans une session gardée distincte.

Les données des petits catalogues ne sont pas une preuve de census massif ; les profils pondérés, les nuages non u16, le régime multi-millions et les contrats de toute la tour 50k en une seconde puis 100 ms restent hors de ce reçu. La porte K9/K10 c03 préparée séparément n'est pas réattribuée au terminal avec semis par la présente gate.
