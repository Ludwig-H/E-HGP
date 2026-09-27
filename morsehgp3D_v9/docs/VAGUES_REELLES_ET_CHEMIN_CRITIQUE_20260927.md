# Vagues réelles et chemin critique — 27 septembre 2026

## Ce qui est acquis

Le consommateur q3/q4 peut parcourir le résidu par vagues sans construire
un tableau de toutes les paires candidates. Les plans par facteurs éliminent
des paires et des voies avec des témoins certifiés ; les produits non
éliminés restent exhaustifs. Les survivants sont remis dans l'ordre natif
avant les étapes suivantes. Il ne s'agit pas d'une approximation géométrique.

Le [prototype CPU réel](../audits/b_q34_waves_real_20260927/README.md)
compare tout le vecteur natif, pas uniquement un condensé. Qualification
Release/sanitizers close ; trame entière sans sol, uniforme8k/16k/32k et six
coupes spatiales mesurées. Une observation ordonnée par cas, hôte partagé,
pas une campagne statistique de latence. Les résultats défavorables restent
dans les reçus et dans la décision de port.

| Cas CPU mono | Natif S2 | Prototype S2, préparation comprise |
|---|---:|---:|
| LiDAR 08/000000 sans sol, 39 885 sites | 47,691 s | 31,048 s |
| Uniforme 8 000 | 4,245 s | 4,555 s |
| Uniforme 16 000 | 9,407 s | 10,158 s |
| Uniforme 32 000 | 20,041 s | 21,530 s |

Ces S2 n'incluent pas l'index/front commun aux deux bras, ni S3/S4, catalogue
et FULL. Le coût candidat depuis lecture jusqu'à S2 vaut 34,949 s sur
LiDAR00. Ce n'est ni une mesure G4 ni un nouveau chrono de tour.

Sur cette trame, P=23 686 751 paires logiques deviennent E=9 122 704
requêtes ponctuelles, et S=2 043 612 survivantes exactement natives.
Les visites ponctuelles diminuent de 1 110 657 775 à 537 798 656.
La préparation coûte toutefois 14,133 s mono : elle inclut le filtre
de tous les rectangles, pas seulement l'arène des facteurs.

## Passage à l'échelle : conclusion précise

Sur uniforme8k/16k/32k, les principaux travaux et temps restent sous le
quadruplement à chaque doublement. Le prototype est néanmoins 7–8 % plus
lent que le natif, faute de paires à éliminer dans ce régime.

Sur les six relations LiDAR quart→moitié et moitié→entier, avec les vrais
effectifs et le masque entier figé avant les coupes, les exposants observés
sont 1,004–1,820 pour E, 1,032–1,223 pour S, 0,886–1,766 pour les visites
ponctuelles et 0,859–1,606 pour le temps candidat. **Exception à conserver :**
un compteur de tests de coins de préparation atteint 2,230. Ce diagnostic
spatial asymétrique n'est pas une preuve asymptotique et ne couvre ni les
amas déjà défavorables ni la croissance aval de toute la tour.

## Les deux obstacles révélés par le raccord

1. Le bornage par Q supprime les tableaux temporaires de taille P/E, mais
   pas les rectangles du front ni les sorties S. Sur LiDAR00, `Prepared`
   conserve 187,227 Mo, dont seulement 29,328 Mo d'arène ; le front d'entrée
   reste aussi présent. Pic RSS du processus : 371,208 Mo. Sur uniforme32k,
   `Prepared` atteint 350,184 Mo malgré une arène de 0,136 Mo.
2. Le prototype conserve une description de chaque rectangle fermé et
   refait son filtre en CPU. Le paramètre quatre workers de la version CUDA
   ne parallélise que l'arène, pas cette boucle. Il faut réutiliser le filtre
   rectangle GPU déjà existant, puis compacter les seuls rectangles utiles
   en conservant leur ordinal global. Le gain GPU ne peut pas financer par
   hypothèse une préparation CPU laissée séquentielle.

Le [port CUDA isolé](../audits/b_q34_cuda_waves_20260927/README.md) et son
[protocole G4](../audits/b_q34_cuda_session_20260927/README.md) sont publiés
en `33c1d28d7`. Leur première qualification est portable : 31 commandes,
85 cas, 340 consommations par build, deux mutants réfutés. La capture
device distincte ci-dessous qualifie et mesure le consommateur, pas FULL
ni l'activation de ce raccord CPU/GPU dans le moteur.

## Première exécution G4 close : exacte, mais raccord non rentable

Une session SPOT sur RTX PRO6000 Blackwell a compilé le `.cu`, exécuté la
gate CUDA puis traité la trame entière ng00. Aucun désaccord avec le filtre
natif : mêmes P/E/S, ordinals et masques ; 537 798 656 visites ponctuelles.
La gate porte sur85cas, avec CUDA pour Q7/Q257 et Q1 lorsque E≤4.
Son compteur `runs=340` dénombre les boucles portables, **pas340appels GPU**.

| Poste de cette observation G4 | Temps |
|---|---:|
| Front CPU commun | 2 097,076 ms |
| Préparation CPU, dont filtre rectangle sériel | 9 898,348 ms |
| Copie privée des structures | 49,733 ms |
| Runner CUDA complet, jusqu'à la sortie native | 267,480 ms |
| Dont allocation/transfert montant | 8,195 ms |
| Dont35vagues, filtre/scan/scatter et lecture des comptes | 34,967 ms |
| Dont allocation/rapatriement des survivants | 22,568 ms |
| Dont tri/conversion à l'ordre natif | 39,171 ms |
| Dont libérations internes | 1,732 ms |

Les sous-phases du runner ne couvrent que106,633ms de ses267,480ms :
160,847ms restent non attribuées. Les appels d'initialisation CUDA sont
avant le premier sous-chronomètre, mais leur contribution exacte n'est
pas mesurée. Ne pas retirer ce reste comme un gain présumé à chaud.
La gate et la trame utilisent deux processus, donc deux contextes distincts.

La somme préparation+copie+runner vaut10 215,561ms, contre10 080,260ms pour
le filtre natif CPU W4 exécuté ensuite. Elle n'inclut même pas la destruction
tardive des structures, mélangée aux autres destructions dans8,303ms.
Le total expérimental22,437s inclut le front, la référence et le juge :
ce n'est pas un chrono moteur. Il n'y a donc **aucun gain net qualifié**.
Les35ms de vagues sont encourageantes pour le composant, pas pour FULL.

188,403Mo sont transférés en entrée,49,049Mo en retour ; buffers device
203,084Mo. Snapshot privé et `Prepared` coexistent sur l'hôte. Leur copie
et les rectangles fermés sont des coûts à supprimer dans le prochain port.

Session close, sources/dépendances/binaire stables ; budget utile34,882s.
Génération07:54:44,162UTC arrêtée07:57:52,578UTC, même cible relue
`TERMINATED`, allocation188,416s. Aucun autre lancement G4 dans ce lot.
Le [reçu device](../receipts/q34_cuda_g4_20260927/r1/README.md) garde
la portée S2 et une seule observation K5/s8 sur une seule trame ; aucun
nouveau résultat FULL, K10, sol conservé ou autre séquence n'en découle.

## Où chercher le facteur dix

La [relecture du chemin critique G4](../audits/b_critical_path_20260927/README.md)
corrige un risque d'interprétation des précédents prototypes FULL : les K
sont **déjà encodés simultanément**. Sur deux premiers passages historiques,
l'encodage occupe une fenêtre de 37–48 ms, pas la somme 85–113 ms des K.
La construction de tour hors cette fenêtre prend encore 252–256 ms.

La phase q34 occupe 487–491 ms, dont seulement environ101 ms pour le filtre
S2 mesuré ; le reste q34 vaut 386–390 ms. q2 et son census anticipé se
recouvrent avec q34 : leurs chronos ne s'additionnent pas au mur. Ces
différences sont des périmètres chronométrés, pas des bornes algorithmiques
ni des gains futurs calculables par soustraction.

Ordre de développement retenu :

1. Conserver le consommateur maintenant testé sur G4, remplacer le
   filtrage/préparation CPU et les descriptions de rectangles fermés,
   puis comparer le coût **net** du nouveau raccord.
2. Réduire et partager les travaux q3/q4 après S2 : Pool ne change pas S,
   donc il ne résout pas à lui seul S3/S4, le census ni le catalogue.
3. Paralléliser la construction des événements/drafts FULL et les liens
   explicites, pas uniquement l'encodage final déjà partagé entre K.
   Conserver continuations, contacts égaux, multifusions et verticales.

Point de raccord concret : `Q34FilterBatch.rectangle_masks` reste actuellement
de longueur R, pour les comptes et le contrôle des survivants dans
`wspd_q34.cpp`. Compacter les descripteurs ne signifie pas supprimer ce
contrôle : on peut garder le tableau compact de masques d'origine, tout en
ne transportant les métadonnées lourdes que pour les rectangles survivants.
Les bases ordinales doivent encore compter les produits fermés antérieurs.
Enfin `witness.pairs.queries` vaut actuellement P dans le moteur : le futur
port doit distinguer P logique et E réellement testé, sans annoncer que P
requêtes ont été exécutées ni casser les identités de couverture.

L'objectif100ms reste toute la tour explicite K1..5 sur trame sans sol G4,
pas un filtre ni un format compact à décoder plus tard. Les références
K10, sol conservé, s10/s12 et autres séquences gardent leurs statuts séparés.
Le moteur de production n'est pas modifié dans cette tranche d'essais.
