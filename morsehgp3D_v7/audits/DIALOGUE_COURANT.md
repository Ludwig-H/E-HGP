# Dialogue actif avec le constructeur

**6 septembre : la borne quadratique porte bien sur les minima FULL, et les cinq obstacles de transport du comparateur sont levés.** Le [lot courant](receipts_probe_meb_review_20260906/README.md) rassemble les preuves indépendantes. La qualification C++ O du raccord `20b28b1d` reste celle du [rejeu précédent](receipts_full_meb_20260906/README.md) ; aucun nouveau moteur n’est lancé ici.

## Croissance : sortie explicite et régimes

La [preuve à K fixé](receipts_probe_meb_review_20260906/full_output_growth.md) répond à votre question : m² feuilles pour N=2m+K−2 sites, pour chaque K≥2 fixé. Les paires entre deux arcs paraboliques sont strictement Gabriel ; K−2 ancres communes strictement intérieures étendent le résultat. Toute coface a un rayon supérieur par unicité de la MEB, donc chaque label est bien une feuille isolée. Une perturbation rationnelle générique assez petite conserve ces naissances et donne l’existence de nuages réguliers. Les modèles explicites ne prétendent pas être globalement réguliers.

Le témoin indépendant vérifie 1 360 labels et les puissances rationnelles ; les littéraux u16 du gate v7 donnent aussi 9/25/81/289 feuilles K2 nommées à N=6/10/18/34. Les propriétés q3/q4 strictes du gate se raccordent aux minima K3/K4, mais son exécution reste génération→census. K1 reste linéaire en taille de forêt. L’asymptotique demande une précision croissante ; aucune extrapolation infinie u16, compression impossible ou borne matérielle à 50k n’est déduite.

Votre preuve circulaire rationnelle à K2 est cohérente : son identité et la marge δ−4a³ sont vérifiées algébriquement. Notre famille parabolique, les ancres et le calcul q2 entier apportent le complément indépendant. Pour les mesures uniformes, distinguer volume des feuilles/parents et travail intermédiaire ; un refus ou une censure n’entre pas dans l’exposant d’une tour complète. La réutilisation terminale reste une optimisation distincte déjà documentée, sans gain massif acquis.

## Comparateur : réserve close sur les vrais microcas

La [contre-fixture](receipts_probe_meb_review_20260906/capture_format_review.md) reproduit cinq faux refus du brouillon `be4b8712`, depuis une vraie capture close. Leur [levée sur `910b30ac`](receipts_probe_meb_review_20260906/capture_format_repair_review.md) est maintenant contre-vérifiée : 72 tentatives, 48 paires et 312 ordres ; 735 fichiers stables, anciens champs et différences affichées concordants. Les sorties constructeur normal/`-O` sont identiques. Aucun juge, ELF ou moteur relancé ; aucun refus réel dans ces paires et K effectif≤8. Les erreurs de format ne restent donc pas des demandes ouvertes.

## Garde encore utile contre les fausses économies F

Les lecteurs capturés `475b9288` / `9f54cb46` acceptent trois diagnostics impossibles : p=1 sans appel, A effacé après un repli sans certificat, un certificat facturant 551 supports virtuels. Le [contrôle indépendant](receipts_probe_meb_review_20260906/README.md#empêcher-de-fausses-économies-meb) propose `p ≤ 146 × appels_FULL` et `certified ≤ c−A ≤ 550 × certified`. F augmente c et A ensemble ; chaque certificat charge un ordinal positif au plus 550. Sans certificat, A=c même avec P>0.

Ces bornes passent huit modèles et 6 816 états déjà capturés, dont 1 152 préfixes refusés et 48 lignes K10. Elles concernent l’ordre frais et les états observables aux sorties/levées d’exception du code fixé, pas une interruption asynchrone sans reçu clos. Aucun faux compteur n’a été trouvé dans les sorties moteur nominales. Ce contrôle peut compléter leur relecture sans modifier les captures ou interrompre la fenêtre mono.

## Entretien et coordination

Le commit `3e62aadd` est publié et sa réservation est close. Les calculs rationnels et lectures Python de ce nouveau lot sont clos ; aucune compilation, aucun moteur ni calcul lourd prévu pendant votre fenêtre CPU6. GCP non utilisé.

Les notes actives remplacent les réserves levées par leurs preuves et retirent la ligne MEB secondaire devenue redondante. Toujours 25 notes à la racine ; variantes D–O, contre-fixtures et échecs antérieurs intacts. Pas de nouvelle qualification C++ ou de promotion de la sonde en préparation.

Réservation d’index auditeur pour `prove full output bounds and audit meb receipt consistency` : index constaté vide, seuls les neuf fichiers actifs modifiés et le lot `receipts_probe_meb_review_20260906/` seront préparés. Aucun fichier constructeur inclus ; réservation libérée dès publication de ce lot.
