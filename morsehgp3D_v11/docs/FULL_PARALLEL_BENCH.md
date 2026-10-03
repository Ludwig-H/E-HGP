# Ablation des cellules régulières en parallèle

Port en cours, aucune qualification native ni mesure acquise. Le contrat
produit est dans [FULL_PARALLEL.md](FULL_PARALLEL.md). Le banc conserve les
bits FULL1=cacheJ2, 2=tri indirect, 4=mémo historique65536 et ajoute
8=lots réguliers. Les bits du banc catalogue ont une signification distincte.
Le catalogue garde ici sa frontière fixe et son assemblage historique.

Le bit8 fixe Q=4096 cellules par tampon et L=48 lanes logiques, indépendantes
du nombre de workers. Avec le bit4, chaque lane possède4096 slots ; la table
historique65536 reste séparée pour les cellules étendues et les verticales.
Ces choix sont des configurations à mesurer, pas des optima théoriques.
Sans bit4, les deux familles de mémo sont désactivées. Les tables et les
tampons sont inclus dans le budget natif8GiB et dans le pic publié.

`bench/full_parallel.py` déclare19 processus frais K1..5 :

- trois LiDAR sans sol entiers × u21/u24 × modes7/15, W48 :12 essais ;
- ng00u21, mode15, W1 puisW8 :2 contrôles du même travail avec L48 ;
- ng00u21, modes3/11, W48 :2 essais sans aucun mémo ;
- uniformes8k/16k/32k u21, mode15, W48 :3 diagnostics de croissance.

Le calendrier donne priorité aux trames LiDAR. Chaque processus est borné
à60s ; le budget de campagne450s peut produire des omissions explicites.
Un échec n'élimine pas son partenaire. Le plan gardé impose la matrice et
le supplément ASan18 avant tout benchmark, dans la garde G4 habituelle3600s.
Lecture, Cloud, création du Pool, sérialisation, décodage, préparation de
grille, segmentation et hiérarchie de points restent hors chrono FULL.

Les comparaisons exigent mêmes empreintes sémantiques entre profils,
mêmes octets enregistrés dans un profil et mêmes comptes structurels entre
options. À option fixe, le travail payé doit aussi être identique entre
W1/W8/W48 et u21/u24. Le mémo privé peut faire varier ce travail entre7 et15 :
le rapport ne doit pas attribuer leur différence au seul nombre de workers.
Les modes3/11 isolent la voie régulière sans cet effet de cache.

Les diagnostics `order.parallel` séparent cellules régulières/étendues,
traces, lots, occupation maximale, appels Pool, somme et maximum des
intervalles des lanes, publication DSU et parcours étendu. Les durées sont
des sous-intervalles des plateaux, jamais ajoutées au temps FULL. La somme
des intervalles des lanes peut dépasser le temps mur ; elle ne mesure pas
directement l'utilisation CPU. Les paramètres et réservations des lanes
sont publiés dans `full.parallel`, tous nuls lorsque la voie est inactive.

La collecte rejoue les contrôles courants même en cas de réutilisation d'un
résumé après rehachage complet. Ces contrôles structurels ne remplacent pas
les petites portes géométriques indépendantes. Un futur succès sur ce banc
ne qualifiera ni le profil float32 sans perte, ni plusieurs séquences LiDAR,
ni le GPU, ni le contrat200ms avant mesure correspondante.

Le [raccord catalogue suivant](FULL_COMBINED_BENCH.md) étend le masque FULL
à0..127 et le schéma de collecte àv7. Le calendrier19essais ci-dessus demeure
le défaut ; le calendrier27essais est une option distincte, sans transfert
des qualifications ou temps entre sources.
