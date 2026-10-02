# Index natif : capacité et propriété — revue 10 close

Source figée Git `e8520481d1745627e156723ad995ac5175a8163f`, indépendante des changements ultérieurs. Lecture des 77 dépendances capturées, contrats antérieurs déclarés épinglés séparément. Aucune compilation, exécution native, allocation massive ou utilisation GCP dans cette capsule. Les portes unitaires, fault et G4 sont **relues, pas exécutées** ici. Aucun défaut causal de capacité ou de propriété trouvé sur ce périmètre.

La factory possède le Cloud après succès seulement ; ses refus conservent le propriétaire appelant et libèrent les nouveaux buffers. Index et Census sont non copiables, leurs déplacements transportent les buffers ; les budgets doivent survivre aux objets. Un Census conserve ses propres SiteIdx, même après destruction de l’index ; leur interprétation exige le même Cloud. Aucun pointeur au Cloud appelant ni scratch/TLS persistant dans le parcours. Références : [index.hpp](sources/morsehgp3D_v11/src/index/index.hpp), [build.cpp](sources/morsehgp3D_v11/src/index/build.cpp), [census.cpp](sources/morsehgp3D_v11/src/index/census.cpp).

Pour n sites, feuille ℓ∈[1,256], soit w la première puissance de 2 avec floor(n/w)≤ℓ et e=n mod w si floor(n/w)=ℓ, sinon 0. La réservation exacte vaut 2(w+e)−1 nœuds et la profondeur floor(log2(floor((n−1)/ℓ)))+2, équivalente à bit_length((n−1)//ℓ)+1. Pour n<kNone, nœuds≤2n−1<2³³, profondeur≤33 ; la garde sizeof(Node)≤128 donne une réservation <2⁴⁰ octets. Les plages SiteIdx restent u32, leurs fins ≤n ; escape et comptes de nœuds sont u64. Le modèle indépendant vérifie 65 536 couples petits, 643 couples virtuels de frontière et 5 400 scénarios de signes/census, sans construire de grand arbre (165 469 contrôles au total).

Les deux passes d’un census stable produisent soit k témoins stricts distincts sans coquille, soit I/U complets. Les deux buffers exacts retiennent 4(|I|+|U|)≤4n octets ; le résultat n’est publié qu’après remplissage réussi. Le modèle abstrait vérifie ces propriétés et le refus avec un résultat précédent encore vivant. Il utilise des certificats de signes supposés corrects et ne constitue pas une nouvelle preuve des bornes numériques.

La construction ajoute S×nodes au Cloud déjà préparé, où S=sizeof(Node). Après cette phase, T réponses complètes simultanées ont le sous-total analytique Cloud unitaire 28n+8 + S×nodes + 4Tn. Le banc exige S=40 et le probe publie sizeof(Node) ; cette capsule ne mesure pas cet ABI. Sous **cette condition** et avec ℓ=8,T=8 :

| Sites | Nœuds | Profondeur | Nœuds, octets si S=40 | Sous-total Go | Sous-total Gio |
|---:|---:|---:|---:|---:|---:|
| 30 000 000 | 8 388 607 | 23 | 335 544 280 | 2,135544288 | 1,988880605 |
| 50 000 000 | 16 777 215 | 24 | 671 088 600 | 3,671088608 | 3,418967694 |

Les 4Tn sont des capacités sûres ; aucun grand nuage atteignant cette borne n’a été exécuté. Sont exclus : entrée conservée, préparation antérieure, catalogue/FULL, autres scratch, métadonnées d’allocateur et RSS. Aucune extrapolation de performances. Le banc lit quatre buffers d’entrée, les libère après Cloud, puis ne garde qu’une réponse par boucle ; cela ne qualifie pas huit réponses conservées.

Deux décisions utiles au futur raccord : (1) budgéter ensemble Cloud, index et toutes les sorties vivantes, puis ajouter les états du catalogue/FULL ; ajouter une porte G4 avec un Census complet retenu, un second refus juste au-delà du budget et un petit saturé encore admis. (2) L’API actuelle fait count/admit/fill dans un appel ; l’admission exacte de toutes les sorties d’un lot exige soit une rétention bornée/sérialisée, soit la borne 4Tn, soit un futur ticket de comptage possédé liant même index, géométrie, seuil, comptes et ledger. Ce ticket est une option de raccord, aucun défaut de l’API actuelle. Le test concurrent existant partage l’index mais possède un budget privé par thread ; il ne prouve pas l’admission globale d’un lot sous un budget commun.

Reproduction sans produit : `python3 -B derive.py` et `python3 -B -O derive.py`, puis lecteurs `python3 -B judge.py` et `python3 -B -O judge.py`. Sorties et stderr conservés. Les snapshots restent l’autorité même si LIVE évolue ; SOURCE_AFTER.json consigne toute dérive. Fermeture : SHA256SUMS couvre tous les fichiers sauf lui-même à la racine.
