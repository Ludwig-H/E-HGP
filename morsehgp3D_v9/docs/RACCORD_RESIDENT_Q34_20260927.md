# Rectangles filtrés et index résident — 27 septembre 2026

Le [prototype résident](../audits/b_q34_filtered_resident_20260927/README.md)
est implémenté ; ses portes **portable CPU et CUDA sur G4** passent.
Le premier essai réel retrouve les mêmes survivants, mais ne justifie pas
encore son activation comme optimisation du moteur. Profil grille
1 mm/u18, `not_claimed`. Aucun nouveau contrat FULL acquis.

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
Cette qualification locale ne compilait pas CUDA ; la session suivante
le compile et l'exécute séparément.

## Premier essai réel G4, session fermée

Le [nouveau protocole G4](../audits/b_q34_resident_session_20260927/README.md)
avait passé quatre commandes hors cloud, 46 contrôles positifs et
117 refus. La [capture réelle r1](../receipts/q34_resident_g4_20260927/r1/README.md)
est maintenant close : source `af369c44`, gate CUDA puis ng00 entière,
39 885 sites, grille 1 mm, K5/s8, à quatre et 48 workers d'arène.
Le front du harnais reste mono-thread et l'oracle CPU utilise quatre
workers. Une observation par largeur, dans deux processus distincts.

Le gate couvre 85 cas : 255 appels portables et 194 appels CUDA du corpus,
plus un appel valide de contrôle de propriété hors de ces compteurs.
Les deux trames retrouvent exactement les mêmes masques, survivants
ordonnés, masses et digest natifs : R=3 133 819 rectangles,
P=23 686 751 paires logiques, E=9 122 704 requêtes physiques et
S=2 043 612 survivants. Les replis restent complets.

| Poste payé, ms | Arène W4 | Arène W48 |
| --- | ---: | ---: |
| Ouverture, filtre rectangle, compaction | 325,035 | 319,930 |
| Dont initialisation CUDA | 161,392 | 155,936 |
| Construction de l'arène CPU | 202,425 | 83,113 |
| Paires GPU, retour et tri/conversion | 97,494 | 98,509 |
| Fermeture des propriétaires temporaires | 2,544 | 2,772 |
| **Adaptateur complet, froid** | **627,503** | **504,328** |
| Front mono + adaptateur | 2 738,875 | 2 616,350 |

Les sous-phases « dont » sont incluses, pas à additionner. L'adaptateur
paie ses transferts et destructions ; les sorties S et masques R restent
possédés, ainsi que l'amont du harnais. Il ne détruit pas le contexte CUDA
global. Lecture, index, oracle et nettoyage final sont détaillés dans le
reçu ; `total` du harnais inclut l'oracle et n'est pas le temps candidat.
Pas de temps chaud obtenu par soustraction de l'initialisation, ni de gain
stable W4/W48 établi par cette seule paire.

Les capacités hôte retenues sont de 70,243 Mo pour les décisions et
104,508 Mo pour `Prepared`, soit 174,751 Mo ensemble. Le champ device de
consommation vaut 122,858 Mo, index résident inclus : ne pas le recompter.
Ce sont des périmètres de tableaux, **pas des pics globaux RSS/VRAM** ;
les entrées temporaires et le front amont existent aussi.

Export et relectures LIVE normal/−O passent, avec
[contrelecture indépendante](../audits/b_q34_filtered_contract_review_20260927/RESIDENT_G4.md).
La G4 SPOT est certifiée **TERMINATED sur la même génération** après
191,901 s d'allocation observée, sans estimation de facture.

## Décision de port et prochaines mesures

**Battre les 9,898 s du précédent prototype ne suffira pas.** Le filtre GPU
du moteur était autour de 101 ms dans la capture historique, avec q2 en
recouvrement. Une décision de port comme optimisation exige une comparaison
GPU/GPU appariée, puis la vraie chaîne ; la référence CPU n'est qu'un oracle.
La réduction de P à E peut être annulée par l'arène, le tri et les copies.
Le prototype est publié et réutilisable, mais **n'est pas activé dans le
moteur comme gain acquis**.

Le travail ponctuel baisse réellement : 537,799 millions de visites contre
1 110,658 millions dans la capture moteur historique. Les visites rectangles
restent identiques. Leurs 81–82 ms nouveaux sont des chronos hôte par vagues,
pas une comparaison appariée aux événements CUDA du moteur. Layout,
réduction des compteurs et découpage en vagues diffèrent : isoler ces effets
avant toute attribution causale, sans lancer une campagne de micro-variantes.
L'arène CPU, les copies/compactions et le retour/tri de S restent des postes
majeurs à supprimer ou paralléliser ; aucun d'eux ne ferme à lui seul FULL.

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
