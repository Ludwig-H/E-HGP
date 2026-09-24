# Audit : chemin critique vers 100 ms pour la tour FULL K5 sans sol

24 septembre 2026. Audit **sans modification du moteur ni appel GCP**,
fondé sur le [reçu G4 R20](../receipts/g4_tower_r20_20260924/README.md),
ses `vm/probe_{0,4,8}.stdout`, le
[compte des octets FULL](CONTRE_AUDIT_FULL_R20_OCTETS_100MS_20260924.md)
et `src/chain/tower_chain.cpp` / `src/tower/forest/full_ball_tower.hpp`.
R20 est le dernier reçu G4 publié au HEAD audité `092ad4ae4` : trois
trames **sans sol** de la seule séquence 08, grille 1 mm/u18, `s=8`,
W48, sorties `complete_relative`. Il n'existe ici **aucun résultat de
100 ms**, ni preuve de complétude absolue du catalogue.

## L'enveloppe actuelle et la sortie non escamotable

Les durées ci-dessous sont `chain_total`, hors lecture de fichier,
segmentation du sol, digest de vérification et coût de contexte CUDA à
froid ; elles comprennent néanmoins préparation/index et la **tour FULL
matérialisée**. `q2` est déjà recouvert par q3/q4 dans le cas mesuré :
il ne faut pas l'ajouter au total, ni oublier qu'il réapparaîtra sur le
chemin critique si q3/q4 devient court.

| 08/ | sites | K1..5 | q3/q4 | fusion + index tour + census | FULL | q2 recouvert |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 | 35 551 | 1 010,479 ms | 482,492 | 125,488 | 374,224 | 74,907 |
| 000000 | 39 885 | 1 108,630 ms | 525,810 | 136,819 | 421,527 | 103,500 |
| 000200 | 45 845 | 1 259,912 ms | 616,554 | 152,713 | 455,638 | 114,637 |

Les sommes de sous-phases sont descriptives, pas une recomposition exacte
du mur : le code recouvre q2 et publie également d'autres petits coûts.
Sur 000000, le **front q3/q4 seul** prend 106,454 ms, le noyau du filtre
63,652 ms, celui des certificats 90,105 ms et celui des voies 56,540 ms.
Les trois noyaux distincts font déjà 210,297 ms en série dans R20,
avant catalogue ou FULL. La passe fusionnée L15 a *ralenti* le noyau
apparié ; la conserver désactivée est justifié par ce reçu, pas par un
principe général. Les 30 ms de « transfert des voies » comprennent des
opérations hôte et ne sont pas intégralement récupérables par épinglage.

Le même cas produit 1 541 750 nœuds, 1 541 745 parents, autant de
successeurs et liens verticaux que de nœuds, et 897 776 contributions.
Les cinq tableaux connus occupent **207 496 040 octets utiles**, plus
26 241 824 à 74 296 404 octets logiques dérivés pour la banque des
populations (capacités physiques inconnues). Sur 000200, les cinq
tableaux connus valent 226 725 304 octets. Ce volume impose un travail
d'écriture et de propriété, mais **ne prouve pas** un plancher de bande
passante de 100 ms : 282 Mo/100 ms ne seraient que 2,82 Go/s effectifs,
et ni trafic DRAM réel ni microbenchmark isolé ne sont publiés. L'API
finale doit déclarer si la sortie explicite est hôte ou appareil ;
l'expansion et tout transfert requis par cette API doivent rester dans
la durée annoncée, pas être reportés au lecteur.

## Budget *expérimental* de 100 ms, pas projection de gain

Une enveloppe vérifiable pour le **maximum** des trames K5 sans sol
qualifiées serait, en latence chaude par trame :

| Chemin de dépendance proposé | Plafond de travail à démontrer | R20/000000, comparaison |
| --- | ---: | ---: |
| entrée déjà masquée → propriétaire/index et appareil prêt pour ce nuage | 10 ms | préparation + index 12,955 ms ; préparation appareil en parallèle, attente 10,700 ms |
| q2 **et** front/filtre/certificats/voies q3/q4 ; joindre les deux | 35 ms | q3/q4 525,810 ms ; q2 103,500 ms recouvert |
| canonisation, index de tour si nécessaire, census global des clés | 15 ms | 136,819 ms |
| cibles exactes, lots, populations, images et **sortie FULL explicite** | 35 ms | 421,527 ms |
| marge de lancement/synchronisation/variabilité | 5 ms | non isolée |

**Somme 100 ms.** C'est un *test de faisabilité*, non un planificateur
validé : le pire cas R20 demande environ ×17,6 sur q3/q4, ×10,2 sur
canonisation+census et ×13,0 sur FULL pour respecter ces trois plafonds.
Les 35 ms q3/q4 ne masqueraient q2 que si q2 passe lui aussi sous
35 ms (×3,3 sur le pire cas) ou si une autre dépendance exacte le
recouvre. Modifier le budget est permis, mais il faut alors garder sa
somme, ses dépendances et l'expansion de sortie. Le simple préchauffage
CUDA, un tampon épinglé ou un tri plus rapide ne peuvent pas remplir
ces trois grands écarts simultanément.

## Refonte à prouver, par ordre de risque

1. **Réduire le travail avant les paires et avant les formes.** R20/000000
   K5 traite 3,134 M rectangles q3/q4, 23,687 M paires étendues et
   359,707 M incidences de sites du cœur ; la trame 000200 en compte
   22,722 M et 348,530 M. Mesurer sur les mêmes ordinals WSPD le coût
   `sélection + preuve exacte + repli + aval`, les deux masques q3/q4
   séparément, les paires évitées **avant** expansion et `ΣF_e` réellement
   évitée **avant** `load`. Un masque partiel ne sauve pas le chargement
   des formes partagées tant que S3 exige l'autre voie. La
   [palette ponctuelle déjà testée](b_moments_palette_20260924/README.md)
   ferme seulement une arête / 55 657 et épargne `F=66` sur 1 151 766
   dans son petit quart K10 ; ses réglages naïfs ne méritent donc pas un
   port GPU. Le certificat spatial BVH en étude doit passer une porte
   shadow plus sélective **et** rester moins cher que les formes
   qu'il évite. La pente LiDAR 16k→32k de `core_sites` mesurée à ×8,27
   sur une coupe 000200/K5 est un signal fini défavorable, pas une loi
   universelle ; compter séparément nouveaux cœurs et taille moyenne.
2. **Rendre les objets amont résidents et consommables par lots sans
   recensement inutile.** Aujourd'hui `run_tower_chain` crée des slots
   de présentations, les rassemble et trie, reconstruit un index de tour
   puis recense **chaque BallKey distincte** (`tower_chain.cpp`, étapes
   « Fusion », « Index de la tour », « Census »). Sur 000000/K5,
   1 306 696 clés conduisent à 98,852 ms de census. Tester une arène de
   clés possédées, tri/réduction déterministe, index partagé ou adapté
   en conservant ID/ordinal ; puis un census GPU/CPU batch des seules
   clés uniques avec intérieur strict et **coquille globale entière**.
   Réutiliser profondeur ou coquille à l'émission n'est correct que si
   l'équivalence globale est certifiée, notamment pour les boules
   présentées par plusieurs supports. Mesurer pic mémoire, H2D/D2H,
   coûts de clé exacte et reports, pas seulement le noyau. Réduire le
   seul transfert des voies ne supprime pas les 125–153 ms aval.
3. **Refaire la résolution et FULL, non le seul Kruskal.** À
   000000/K5, FULL dépense validation 70,516, cibles statiques 198,500,
   lots 56,518, populations 19,571, images 26,862, banque 17,216 et
   encodage 32,233 ms. Il compte 3,622 M représentants,
   1,289 M MEB et 31,708 M visites d'index. Il faut une passe batched
   de cibles terminales **exactes** avec repli vérifiable, puis un graphe
   d'événements `(K, BallId, rang du niveau)` dont composantes aux seuils
   ouvert/fermé et scan canonique produisent les mêmes parents et IDs.
   Le [lemme du maximum d'ID marqué](PHASE_A_MAX_ID_COMPOSANTE_20260923.md)
   est un schéma combinatoire sur 3 000 historiques abstraits, **pas**
   un port LiDAR ; une forêt minimale ne dispense ni de résoudre toutes
   les cibles ni de produire contributions, populations et images
   verticales. `FullBallBatchResolver` désactive aujourd'hui
   `run_orders_parallel()` (`full_ball_tower.hpp:403`) : un simple
   callback GPU retomberait sur le chemin K séquentiel. Une architecture
   batched doit aussi résoudre cette interface, les premiers ordinals
   d'attribution de populations et l'expansion finale en tableaux
   explicites. Ne lancer aucun kernel par chacun des très nombreux
   petits niveaux sans histogramme réel de lots.

La première expérience FULL utile est un **sidecar sur le catalogue
LiDAR identique**, qui publie les cibles de chaque facette en ordinal,
compare ensuite nœud par nœud niveau, parents, successeurs, contributions,
banque et verticales avec la voie actuelle, et mesure toutes les copies
et l'expansion. Une accélération du graphe seule, sans coût des cibles
et de la queue, ne passe pas la porte des 35 ms. Même si les cinq tableaux
R20 étaient écrits à ce débit, 207,5 Mo/35 ms représenteraient environ
5,9 Go/s effectifs ; **mesurer** le débit d'expansion des mêmes types et
tailles sur G4, à froid/chaud et avec capacités publiées. Ce quotient
n'est pas un débit matériel garanti.

## Portes de qualification et ordre de dépense

- **Avant un autre G4** : corriger et rejouer sur SHA figé le
  [préflight R21/v26](CONTRE_AUDIT_B_PREFLIGHT_R21_V26_20260924.md) :
  plan 30 cas contre assertions historiques 18, `context_ms+reserve_ms`
  dans la vérification du mur externe, six épingles CPU brutes dans le
  protocole. Le bras « chaud » actuel ouvre encore un processus par cas ;
  il ne certifie pas le débit d'un processus persistant multi-trames.
- **Portes mathématiques/CPU** : oracles exacts q2/q3/q4 sur petits
  nuages ; support et coquille littérale/IDs sur un échantillon de
  trames entières (le recoupement de deux moteurs peut omettre la même
  clé), puis égalité de la sortie FULL explicite. `complete_relative`,
  digest et Euler jusqu'à K−2 sont nécessaires mais insuffisants.
  Shadow des certificats sur les 8k/16k/32k à coupes par plans passant
  par le capteur et densités appariées : publier pentes de rectangles,
  paires, `ΣF`, candidats, supports, octets, CPU·s et mur, avec **coût
  des certificats et repli compris**. Ne pas utiliser les quarts comme
  succès de trame entière.
- **Puis G4 apparié** : même entrée et même sortie explicite, K1..5
  puis K1..10, `s=8/10/12`, répétitions froides et chaudes et maximum
  des trames, d'abord plusieurs trames **sans sol** de plusieurs
  séquences, puis leurs trames brutes avec sol. Segmentation, HGP sur
  masque figé et coût total sont trois chiffres distincts. R8 ne donne
  qu'une comparaison `s=8/10/12` **CPU** sur G4 à K5 ; elle ne qualifie
  pas le moteur GPU R20. Le profil float32 brut reste secondaire au
  choix utilisateur 1 mm, pas implicitement qualifié par u18.
- **Massif** : des scènes 10 M+ et plusieurs dizaines de millions de
  sites exigent tuilage, ordinals et arènes dépassant les seuils `2³¹`,
  sortie au moins linéaire en n, vérification des reports exacts,
  VRAM/RSS et temps/capacité publiés. Ce régime ne reçoit aucun contrat
  100 ms par extrapolation des 40k sites ; refuser proprement une
  capacité est préférable à publier une tour partielle.

**Décision d'audit :** poursuivre la porte locale des certificats
pré-S2/pré-S3 et le sidecar FULL avec sortie littérale ; ne pas financer
un port G4 d'une palette à sélectivité négative, ni annoncer 1 s/100 ms
à partir des projections. Le premier jalon GPU mesuré reste K1..5
**sous 1 s pour une trame entière**, non acquis à R20 ; les 100 ms
explicites demandent des changements algorithmiques dans **les deux**
grands étages.
