# Rectangles filtrés et index résident — 27 septembre 2026

Le [prototype résident](../audits/b_q34_filtered_resident_20260927/README.md)
est implémenté et sa qualification **portable CPU** est close. Il prépare
le prochain essai GPU, sans changer le moteur ou ses défauts. Profil grille
1 mm/u18, `not_claimed`. Aucun gain G4 ou contrat FULL nouveau à cette porte.

## Le changement concret

Le précédent essai CUDA gardait une préparation géométrique CPU de tous
les rectangles : elle prenait 9,898 s sur la trame sans sol ng00, malgré
35 ms pour les vagues GPU de filtrage/compaction. Ce sous-chrono n'incluait
pas le retour des survivantes ni leur tri/conversion. Le raccord complet
n'était pas un gain.

La nouvelle session filtre d'abord les rectangles, conserve leurs décisions
dans un objet privé, puis prépare l'arène pour les seuls rectangles encore
ouverts. Elle garde le même index device entre les deux filtrages. Pas de
recalcul du filtre rectangle en CPU après sa décision GPU, ni de tableau
temporaire contenant toutes les paires P ou E.

Les tableaux de travail des vagues sont bornés par Qr/Q, **pas la recherche**.
Les produits éliminés restent représentés dans les masques et les offsets
d'origine. Les survivantes retrouvent exactement l'ordre natif avant la
sortie. L'arène temporaire est libérée après construction de ses tableaux
de transport ; leurs capacités, transferts et destructions restent payés.

L'interface ne permet pas à l'appelant de déclarer ses propres masques
« certifiés ». Une préparation issue d'une autre session, même sur un nuage
identique, est refusée. Voir le
[contrat et sa preuve de composition](../audits/b_q34_filtered_contract_review_20260927/README.md).

## Tests effectivement terminés

[Capture locale r1](../receipts/q34_filtered_resident_20260927/r1/capture.json) :
50 commandes, builds neufs Release et Clang ASan/UBSan/LSan.

- 85 cas × trois configurations Qr/Q/workers par build ; mêmes masques,
  survivantes ordonnées et masses que la référence native.
- 75 990 requêtes physiques et 29 901 survivantes cumulées par build ;
  mêmes compteurs physiques entre les tailles de vagues et workers.
- K1/2/5/10, s8/10/12, quatre familles synthétiques, permutations,
  masques partiels, plans et replis complets, sorties vides.
- 276 refus ciblés ; trois mutants réfutés causalement : fermeture
  erronée, mauvais offset brut et suppression du contrôle d'origine.
- Quatre exécutions avec panne de pile simulée dans le backend portable,
  sans publication de succès. Ce ne sont ni des pannes CUDA réellement
  injectées ni une qualification des échecs d'allocation.
- Interface de compilation C++17 contrôlée, petite entrée artificielle
  de 12 points testée ; lecteurs normal et `-O` identiques.

Un compteur de couverture du gate sous-débordait pour certains masques
mono-voie : il a été corrigé avant gel, en comptant les réductions réellement
énumérées. Cette correction ne change pas les prédicats du prototype.
La capture fraîche r1 a terminé sans échec. Pas de nouvelle gate TSan.
L'unité CUDA est hachée mais **pas encore compilée par cette qualification**.

## Ce qui reste à mesurer

Le [nouveau protocole G4](../audits/b_q34_resident_session_20260927/README.md)
est également qualifié hors cloud : quatre commandes, 46 contrôles positifs,
117 refus, annulation/jointure testées, lecteurs normal/−O. Il prévoit une
porte CUDA puis ng00 complète K5/s8 à quatre et 48 workers d'arène. Le front
du harnais reste mono-thread, la référence CPU de correction utilise quatre
workers. Les gardes et l'arrêt ciblé restent ceux du protocole épinglé.

Le chrono `adapter` inclura initialisation CUDA, filtrages, préparation,
transferts, tri/conversion et destruction des propriétaires temporaires.
La sortie S+R reste possédée. Front/index amont, oracle et nettoyage final
sont publiés séparément ; `total` du harnais n'est pas le temps candidat.

**Battre les 9,898 s du précédent prototype ne suffira pas.** Le filtre GPU
du moteur était autour de 101 ms dans la capture historique, avec q2 en
recouvrement. Une décision de port comme optimisation exige une comparaison
GPU/GPU appariée, puis la vraie chaîne ; la référence CPU n'est qu'un oracle.
La réduction de P à E peut être annulée par l'arène, le tri et les copies.

Le raccord ajoute des scans linéaires en R et ne réintroduit pas le carré
des facteurs. Cela ne borne pas globalement R, les visites, F, E ou S.
Les précédents tests 8k/16k/32k et coupes LiDAR restent leurs propres
captures : aucune nouvelle mesure de croissance de ce raccord n'est encore
publiée. Aucun transfert automatique de qualification ou de vitesse.

## Suite au-delà de S2

Le [ledger q34](../audits/b_q34_outer_ledger_20260927/README.md) confirme
386–390 ms hors S2 dans deux premiers passages historiques ; 83–84 ms
restent non attribuées. S3, S4, census et construction FULL ne disparaissent
pas quand S2 devient plus rapide.

Deux prochains lots sont définis, sans gain encore acquis :

1. [Conserver et trier S sur GPU](../audits/b_q34_filtered_contract_review_20260927/NEXT_SURVIVORS.md),
   sans recréer une allocation de taille E avec des chunks partiellement
   remplis. Comparer au format de lignes déjà ordonnées.
2. [Exporter les vrais blocs FULL](../audits/b_full_construction_parallel_20260927/NEXT_MANIFEST.md)
   et rejouer exactement leurs drafts/ancres, avant le constructeur
   événementiel parallèle. Les blocs muets et contributions datées restent
   nécessaires ; les seuls drafts finaux ne suffisent pas.

Objectif inchangé : tour explicite K5 sans sol en 100 ms, encore non acquis.
