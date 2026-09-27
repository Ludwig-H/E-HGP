# Où les prototypes peuvent vraiment gagner du temps

27 septembre 2026, audit sur `70168cc3b`. Relecture des deux premiers
passages GPU « core ON » de la capture close du matin ; **aucun nouveau
benchmark ni appel GCP**. Le lecteur intégral de cette capture est rejoué
avant le calcul ci-dessous, avec son snapshot et ses preuves de fermeture.
Les sources qui placent les chronomètres sont identiques au snapshot G4.

## Deux confusions à éviter au prochain port

La baisse locale 164→121 ms du prototype FULL parallèle est une somme
de médianes par ordre. Le moteur G4 encode **déjà les différents ordres
simultanément** : `Builder::finish` lance `parallel_items(kmax, ...)`.
Paralléliser l'intérieur de chaque ordre est une nouvelle possibilité,
mais sa baisse de somme ne se soustrait pas au mur de la chaîne.
Les destructions des drafts sont en outre incluses dans les chronos natifs
`encode_by_k`, contrairement à la simple construction du résultat dans la
sonde isolée. Une mesure appariée de chaîne reste indispensable.

Le Pool q34 garde le même S. Il peut alléger le filtre S2, mais les
certificats sur S, les voies q3/q4, le catalogue et les lots FULL restent
à calculer. Ce n'est pas une raison d'abandonner le port : c'est une raison
de ne pas lui attribuer le facteur neuf nécessaire aux 100 ms.

## Fenêtres réellement mesurées sur G4

Une trame sans sol 08/000000, K1..5/s8, 39 885 sites. Ces colonnes sont
deux processus, premier passage uniquement ; les sous-chronos ne sont pas
publiés pour les passages chauds. Millisecondes :

| Fenêtre | Processus 0 | Processus 3 |
|---|---:|---:|
| Chaîne | 946,725 | 939,321 |
| Génération q34, filtre inclus | 487,264 | 490,861 |
| Filtre S2 complet inclus dans q34 | 101,017 | 100,707 |
| q34 hors cette fenêtre de filtre | 386,247 | 390,154 |
| Census tardif du catalogue | 102,965 | 103,195 |
| Construction tour, encodage inclus | 303,985 | 288,771 |
| Fenêtre d'encodage de tous les K | 48,343 | 37,237 |
| Somme des encodages par K (recouverte) | 112,728 | 85,376 |
| Tour hors fenêtre d'encodage | 255,642 | 251,534 |

Les deux différences « hors fenêtre » sont des soustractions d'intervalles
emboîtés identifiés dans le code, **pas** une mesure d'une implémentation
où la fenêtre aurait été supprimée. Modifier une phase peut changer les
recouvrements et la contention. Ces nombres ne sont donc ni une prévision
de gain, ni une borne inférieure de tous les algorithmes possibles.

Le temps q2 (105,837 / 103,445 ms) et son census anticipé
(70,953 / 71,204 ms) sont recouverts par q34. Leurs attentes après q34
valent zéro. Ne pas ajouter ces temps aux phases extérieures successives.
La somme de préparation/index/q34/attentes/merge/index-tour/census/tour
laisse 11,774 / 12,812 ms non affectés ; cette différence inclut les
intervalles hors chronomètres et n'est pas attribuée sans mesure à un
destructeur ou à un traitement géométrique particulier.

Chaque passage produit S=2 043 612 arêtes filtrées, 1 306 696 boules et
1 541 750 nœuds explicites. Réduire P=23 686 751 par le Pool ne réduit
aucune de ces trois sorties dans le contrat du prototype.

## Conséquence pratique

Conserver trois chantiers distincts : mémoire/consommation S2 par vagues,
génération et certification q3/q4 partagée, puis construction des lots
FULL et écriture des sorties. L'encodeur parallèle qualifié est utile,
mais ne remplace ni la génération des drafts ni les liens verticaux.
Après les gates CUDA, mesurer le port sur la chaîne avant activation ;
ne pas présenter son temps isolé comme le nouveau temps FULL.

Reproduction en lecture seule, normale et `-O` :

```sh
python3 -B morsehgp3D_v9/audits/b_critical_path_20260927/analyze.py /workspaces/E-HGP/build/v9-g4-core-snapshot-20260927/snapshot.tar.gz
python3 -B -O morsehgp3D_v9/audits/b_critical_path_20260927/analyze.py /workspaces/E-HGP/build/v9-g4-core-snapshot-20260927/snapshot.tar.gz
```

Ce script vérifie les pins du placement des chronomètres et rejoue
`b_gpu_next_20260927/readback.py` ; il ne remplace pas le protocole G4
et ne réécrit aucun reçu. Il n'utilise pas les nouveaux chronos locaux
pour extrapoler la vitesse du CPU ou GPU G4.
