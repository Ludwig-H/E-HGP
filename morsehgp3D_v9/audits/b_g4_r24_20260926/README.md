# G4 R24-B : compléments ciblés à la séance R23-C

Plan d'audit seulement, protocole v28 commité inchangé. C a déjà exécuté
25 cas sur G4 ; ne pas les refaire comme s'ils manquaient. La séance est
maintenant close : [reçu G4 R24-B](../../receipts/g4_tower_r24b_20260926/README.md)
et [lecture critique](../AUDIT_B_R24B_G4_20260926.md). Le script
`analyze_receipt.py` reconstruit `SUMMARY.json` depuis les sorties brutes.
Elle complète deux contrôles laissés ouverts :

1. Séparer `tower_overlap_static` et `tower_pipelined_tail` avec trois bras
   sur les trames entières 08/000000 sans sol (`00`, 39 885 sites) et brute
   (`b00`, 123 389 sites), K5 et K10, s8 : (recouvrement, queue) = (1,1),
   (1,0), (0,1). Deux répétitions entrelacées par triplet, dans l'ordre
   A–B–C puis C–B–A pour contenir une dérive monotone. Un jumeau moteur
   par trame/K contrôle l'identité de la tour et du catalogue.
2. Tester directement la voie GPU complète à s10 et s12, K5, sur les deux
   mêmes trames, avec jumeau moteur correspondant. La séance C n'avait
   exercé le GPU qu'à s8.

Les 36 cas sont des trames entières, grille entière 1 mm, `workers=48`,
`static_threads=48`. Les autres leviers restent identiques à R23-C ; L15
reste désactivé. Le premier cas active tous les leviers activés dans le plan,
comme l'exige le préflight. Aucun résultat de ce plan ne peut qualifier le
profil float32, d'autres séquences, le contrat 100 ms, ni un gain de budget
de fils non testé.

Exécution autorisée uniquement par le contrôleur `tower_session_v9.py`,
sur la cible G4 SPOT fixe, depuis un paquet reconstruit d'un commit. Le
contrôleur impose la double garde et l'arrêt ciblé. Budget utile plafonné à
1 500 s, 600 s par cas ; publier aussi refus, dépassements et cas sautés.
