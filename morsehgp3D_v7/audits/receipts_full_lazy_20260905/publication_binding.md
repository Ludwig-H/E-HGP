# Raccord du paquet public FULL lazy aux preuves closes

Observation du 5 septembre 2026 à 17:48:59 UTC. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [paquet public](../../receipts/full_gabriel_lazy_20260905/README.md) est désormais présent et correspond aux octets de la campagne privée déjà contre-vérifiée. Ce raccord complète la [revue initiale](constructor_review.md), conservée intacte ; il ne constitue pas une nouvelle exécution ni une qualification générale du produit. Les pins détaillés et les limites figurent dans le [reçu de raccord](publication_binding.json).

## Intégrité et attribution

Le paquet contient 198 fichiers : son sceau externe couvre les 197 autres fichiers et son manifeste inventorie 196 entrées. Les chemins, inventaires, tailles et SHA-256 concordent, sans doublon JSON ni chemin dangereux. Les 190 fichiers de `capture/` sont identiques aux 190 fichiers privés clos, concordent tous avec les pins de la revue initiale et, pour 127 d’entre eux, avec les copies portables déjà conservées dans l’audit. Le sceau externe vaut `e0d99febecd20f9991e400e378f2add86d5dec764a76f0b7cb0d84298bdd632c`.

Le reçu publié, son contrôleur et les références du publisher et du checker concordent avec leur provenance. Le publisher et le checker n’ont pas été exécutés par cette contrelecture. Les métadonnées de publication décrivent une copie ; leur présence ne prouve pas à elle seule un commit Git ou une publication distante.

## Portée conservée

Les commandes, binaires, durées et sorties restent ceux des 14 CTests ciblés Release et des 14 CTests ciblés ASan/UBSan examinés précédemment. Aucun résultat de suite complète n’est ajouté. L’inventaire de 582 sources est inchangé dans la copie publique ; les 60 fichiers v7 de cet inventaire concordent encore avec les fichiers courants relus. Le CMake est inchangé depuis la campagne : `53f122566fa18281a39d2d83615af18799ccae0ccbb9e62395f2c67ae824cd0d`. Les autres documents produit n’étant pas tous épinglés dans cet inventaire, aucun delta documentaire historique supplémentaire n’est déduit ici.

Le JUnit et le résumé publié conservent la troncature explicite à 1 024 octets de la sortie du test d’allocation lazy, dans chaque configuration. Les sorties complètes LastTest et CTest, elles aussi identiques aux captures closes, portent les 434 injections refusées, sans exception échappée. Cette limite du résumé ne devient ni une anomalie produit ni une prétendue égalité de toutes les sorties JUnit avec les bruts.

L’échec de développement 12/14 reste une preuve négative séparée. Le paquet d’admission `full_gabriel_lazy_probe_20260905` relève d’une autre contrelecture. Aucun moteur, build, CTest, benchmark ou outil Git n’a été lancé ; aucun reçu historique ni manifeste courant n’a été modifié. GCP non utilisé.
