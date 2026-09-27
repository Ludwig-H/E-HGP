# Profil FULL G4 : où passe le temps ?

27 septembre 2026. **Première capture Nsight Systems FULL obtenue**,
sur le moteur inchangé `ddf4776d`. Le contrat 100 ms reste ouvert.
[Protocole R2](../audits/b_full_nsys_r2_20260927/README.md) publié en
`fe6977420` avant lancement ; [reçu et réserves](../receipts/full_nsys_20260927/r2/README.md).
L'échec R1 est conservé séparément, sans réécriture de son statut.

## Ce qui a réellement été mesuré

G4 SPOT, 48 vCPU, RTX PRO 6000 Blackwell Server Edition. Une trame entière
08/000000 **sans sol**, 39 885 sites, grille 1 mm/u18, tour explicite
K1..5, s8. Même configuration que la référence FULL chaude précédente.
Reconstruction native dans la session, puis trois petits préflights
GPU/moteur/capacité réduite, puis quatre passages sans profiler et quatre
sous Nsight. Les huit passages retrouvent le même objet contrôlé :
1 306 696 boules, trois digests et comptes par ordre inchangés.
Cela ne remplace pas un nouvel oracle de complétude mathématique.

| Mesure native de chaîne | Premier passage | Trois passages suivants |
| --- | ---: | ---: |
| Sans profiler | 940,983 ms | 907,267 / 920,112 / 912,219 ms |
| Sous profiler, diagnostic seulement | 1 003,751 ms | 1 011,579 / 978,008 / 994,724 ms |

Médiane chaude sans profiler : **912,219 ms**, dans un seul processus.
Ce n'est ni une nouvelle campagne statistique ni plusieurs scènes.
La référence antérieure de 923,417 ms demeure inchangée. Lecture,
segmentation, ouverture CUDA/réservations initiales et digests sont hors
`chain_total` ; aucune qualification entrée-à-sortie sous la seconde
ni amélioration moteur n'est revendiquée.

La VM est arrêtée, même génération certifiée : 13:58:30,579 →
14:02:11,018 UTC, **220,439 s d'allocation**. Worker utile : 71,679 s.
Ce temps n'est pas un prix facturé. Aucun second essai après cette capture.
Le nouvel exécutable et les rapports sont récupérés en privé, pas seulement
la référence à un fichier distant temporaire. Sources, dépendances réellement
compilées, produits, outils et bibliothèques sont fermés avant/après exécution.
Le lecteur LIVE rejoue les sorties natives, les hashes et l'arrêt en normal
et sous `-O` ; ses quatre contrôles positifs et dix-sept falsifications
passent. L'analyseur d'intervalles a sept tests, également dans les deux modes.

## Le GPU travaille réellement, mais pas seulement à trier

La trace contient 84 noyaux, soit 21 par passage, 164 copies et 20 mises
à zéro. Voici les **moyennes des durées enregistrées sur quatre passages**,
pas des temps FULL, ni des pourcentages d'occupation des unités du GPU :

| Travail enregistré | Temps par passage |
| --- | ---: |
| Certificats géométriques (`certificate_kernel`) | 90,136 ms |
| Filtrage des rectangles | 33,946 ms |
| Filtrage des paires | 28,861 ms |
| Traitement des tâches q3/q4 | 37,920 ms |
| Préparation des tâches q3/q4 | 15,258 ms |
| Tous les autres noyaux, scans compris | 1,773 ms |
| **Total des noyaux** | **207,895 ms** |
| Copies hôte/device | 5,184 ms |

L'union des noyaux, copies et mises à zéro vaut 852,329 ms sur les quatre
passages, soit 213,082 ms par passage. Aucun recouvrement entre ces activités
n'est observé. C'est une observation de cette trace, pas une impossibilité
de recouvrement dans une autre architecture ni une borne mathématique.

Les scans CUB sont petits ici. Mais **la construction de la tour et son
travail de tri/unions restent côté CPU** : les petits scans GPU ne prouvent
pas que toute cette construction est gratuite. Sans profiler, les chronos
natifs donnent 276,907–303,400 ms pour la tour, 102,071–103,322 ms pour le
census, et 471,232–482,781 ms pour q3/q4. Le front q3/q4 du premier passage
prend à lui seul 99,815 ms. Ces nombres ne s'ajoutent pas au tableau GPU :
les noyaux sont déjà inclus dans q3/q4, et certaines autres étapes se recouvrent.

## Ce que la trace ne permet pas de conclure

Deux avertissements existent dans `DIAGNOSTIC_EVENT`, malgré un stderr vide :

- Nsight Systems 2025.3.1 ne prend pas officiellement en charge la version
  CUDA 13.0 du pilote installé ; il a collecté avec ses bibliothèques 12.9.
- Sans collecte de l'ordonnancement, l'activité des threads déduite des
  appels OSRT est imprécise. L'échantillonnage CPU et les commutations de
  contexte étaient volontairement désactivés.

Aucun avertissement de perte d'événements n'a été relevé, mais cela ne
certifie pas la complétude de la collecte. Les ordres de grandeur ci-dessus
restent un **diagnostic avec réserve de compatibilité**, pas une nouvelle
qualification du profiler ou du contrat. Une prochaine mesure de référence
devrait employer une version du profiler compatible avec ce pilote.

Les 817,681 ms cumulées dans les appels `cudaMemcpy` ne sont pas 817 ms
de transfert : ces appels attendent aussi le calcul précédent. Les copies
effectivement enregistrées durent 20,737 ms au total. De même, les temps
`futex`, `pthread_join` ou `cudaFree` ne s'ajoutent pas au mur FULL.
Les nombreux lancements de threads observés motivent une inspection, pas
une promesse de gain égal à leur durée cumulée.
Exemple recoupé par corrélation : un appel `cudaMemcpy` dure 90,265 ms,
mais sa copie device→hôte de 2 043 612 octets ne dure que 0,039 ms ;
il attend le certificat précédent. Les 268 activités GPU trouvent chacune
une API corrélée unique. Cette cohérence ne lève pas la réserve du profiler.

Certains anciens champs `*_kernel_ms` encadrent aussi des lectures de
compteurs, allocations et pauses hôte. `chain_total` lui-même soustrait
des digests intermédiaires : ce n'est pas une fenêtre contiguë simple.
Les grands trous entre passages GPU mélangent census, tour, contrôles et
préparation suivante. Sans marqueurs de phases ni profil CPU, il serait
incorrect de tous les attribuer à une étape précise ou de les déclarer
récupérables. Cette capture ne mesure pas l'occupation des SM ni la
bande passante effective des noyaux.

## Décision pour les 100 ms

**Pas de nouvelle grande refonte sur la seule foi d'un gain local.**
Le GPU consomme déjà environ 208 ms en noyaux dans cette exécution ; la
construction CPU de la tour dépasse également 100 ms à elle seule.
Même supprimer toutes les copies ne résoudrait pas le problème. Accélérer
uniquement le certificat, pourtant 43,4 % du temps des noyaux, ne retirerait
au maximum qu'environ 90 ms de travail actuel, pas les quelque 800 ms à gagner.
Ce raisonnement limite une optimisation isolée ; il ne condamne pas une
nouvelle organisation mathématique du travail.

Pour justifier un chantier capable de changer l'échelle, il faudra montrer
simultanément une forte réduction du travail géométrique **et** du coût de
construction explicite. Les prototypes de tour événementielle/min-label
ne démontrent toujours pas un gain sur le constructeur natif. Ne pas les
porter aveuglément. Nsight Compute pourrait ensuite départager les causes
du coût du certificat (calcul, accès mémoire, divergence), mais n'a pas été
lancé ici : aucun de ces diagnostics fins ne suffit seul à promettre 100 ms.

Le prochain diagnostic utile serait une attribution CPU par phases de la
chaîne réelle, avec instrumentation légère et profiler compatible, avant
de choisir un seul changement structurel. Aucun nouveau moteur, nouveau
test de croissance 8k/16k/32k, autre scène, K10, sol conservé ou régime
massif n'est qualifié par cette capture.
