# Fausses pistes fermées (v7 et v8)

22 septembre 2026. Une piste de cette liste ne se rouvre qu'avec un fait
nouveau : une preuve, une fixture ou une mesure épinglée qui contredit la
raison de sa fermeture. Les références complètes sont dans
`morsehgp3D_v8/docs/FAUSSES_PISTES.md`, `morsehgp3D_v7/docs/FAUSSES_PISTES.md`
et les rapports de [audit_v8/](audit_v8/README.md). « Non épinglé » signale une
fermeture fondée sur un journal sans reçu : elle reste une présomption.

Une fermeture n'a pas toujours la même force : une **preuve** ou une
**fixture** (fausse en général), une **mesure** (plus lente sur un régime
donné), un **modèle de coût** conditionnel, ou une **consigne** de priorité.
Seules les deux premières sont définitives. La liste v8 complète
(`morsehgp3D_v8/docs/FAUSSES_PISTES.md`, 580 lignes) contient en outre
environ 70 garde-fous d'exactitude qui ne sont pas recopiés ici : la relire
avant de porter l'atlas, le census q3, la fenêtre q4, le catalogue ou les
parents FULL (liste ciblée dans le [rapport 15](audit_v8/15_entrees_v8_non_lues.md)).

## Objet et mathématiques

| piste | fermée par |
| --- | --- |
| Porter le fold v4 (union des facettes des boules émises) comme tour FULL | registre `false_in_general` : E5 crée une fausse naissance puis une fausse fusion ; minima isolés et K = n absents |
| Minima Gabriel avec leurs seules adjacences induites | deux fixtures à quatre points u16 réguliers (fusion vraie 477/34 manquée ; fusion retardée de 169/9 à 41/2) |
| Publier tous les niveaux Γ | preuve de suffisance des minima et des multifusions |
| Identifier une composante par sa couverture de points, une boule par son support, son arité ou son rayon | deux identités de même couverture ; boules tri-arités du corpus |
| Filtre p + u ≤ smax pour les coquilles non régulières | coquille à sept points donnant une naissance K5 |
| Omettre l'ancre d'une boule localement inerte | contre-exemple à cinq points, résolveur en échec |
| Appliquer α3 = 3 à la voie q4 | tétraèdre entier u16 : z extérieur accepté comme témoin |
| Témoin non strict (≥) | contacts (32,34,28) en q3 et (32,32,32) en q4 |
| Citron sans arête maximale ou sans positivité du support | deux contre-fixtures u16 |
| Alimenter q3/q4 par les arêtes acceptées en q2, ou q4 par les graines acceptées en q3 | fixtures exactes, mutants compilés tués |
| Transmettre aux enfants le **compte** de témoins du parent | support perdu, fixture de cinq points à K = 2 |
| Témoins universels différents par coin pour certifier un rectangle | contre-exemple exact b = (300,200,200) |
| Bornes de puissance aux seuls coins d'une boîte de centres | F1 : coins [0, 0], vraie plage [−25, 0] |
| Restreindre Z aux graines propriétaires ou valides | F9 : deux témoins stricts perdus |
| Orienter N avant de tester les quatre poids q4 | inverse trois poids si det < 0 |
| Comparer les racines q4 par produits croisés de degré 9 | environ 2^2511, hors capacité ; le déterminant réduit de degré 5 suffit |
| Supposer qu'un nuage uniforme u16 est régulier | refus G4 50k : quatre extra-shells à K10 |
| Transposer le plafond de coquille v7 (12 sites) | coquilles de 30 sites exercées ; la coquille n'est pas bornée par K |

## Numérique et entrées

| piste | fermée par |
| --- | --- |
| Réduire simplement le pas u16, élargir `Point3` implicitement, grille 2,5 mm | collision (0,1,0)/(0,0,65536) ; 200000² tronqué en u32, signe q2 inversé |
| Stocker D = (e − a)² en u32 au-delà de 16 bits | 262 143² > 2^32 : troncature silencieuse |
| Former scale·x pour localiser un centre q3 à 18 bits | 2^137 > i128 ; division longue exacte |
| Carte des centres à Q = 2^44 à 18 bits | 96·M²·Q² ≈ 2^130,6 > 2^127 |
| Coupe au milieu géométrique pour l'index float32 | 278 points dyadiques : 277 niveaux contre 9 au rang médian |
| Chemin float32 à 1 728 bits pour le contrat temps | coût par prédicat bien plus élevé que l'entier (ordre de grandeur non épinglé) ; décision utilisateur du 22 septembre |
| Préfixes par priorité de hachage comme expérience de croissance LiDAR | remplacés par le protocole spatial (décision utilisateur du 21 septembre) |
| Scans 100/200 dans le repère du scan 0 pour des coupes capteur | les plans ne passent plus par leur capteur |
| Retrait du sol par seuil z constant | rejeté par le protocole sans sol |
| Versionner des scans bruts ou des nuages dérivés KITTI | licence non commerciale ; dépôt public |

## Voie q2 et crédits locaux

| piste | fermée par |
| --- | --- |
| Histogrammes locaux O(\|A\|²+\|B\|²) de la v7 | 2,04 → 8,39 → 31,08 s sur deux amas |
| Filtre axial, addition seule des colonnes, intersection | rotation : 256 M paires conservées ; addition 237 ms contre 57 ms |
| Tubes comme chemin général de crédits | 0 crédit sur nuage irrégulier (7 818 cellules pour 8 060 sites) |
| DualBlocks, `CreditBatch` par rectangle isolé | hors chemin WSPD, remplacés par le Pool sur nœuds globaux |
| Fabrique de rectangle recopiant le nuage | coût ×R sur les préparations |
| Census conjoint A/B équilibré ; conjoint A seul | rangées 8k 0,242 → 1,888 s ; gain non concluant |
| Test de lentille avant recherche | 0,4 à 1 % des recherches évitées |
| Blocs Z certifiés le long de la descente | **différée, non fermée** : uniforme 32k 57,6 → 78,6 s, mais la comparaison front + census demandée par l'auditeur B n'a jamais été faite |
| Réutiliser le pivot du parent sans redescendre ; descente plafonnée à K | ×0,94 à ×6,00 ; 38 à 91 bornes par rectangle |
| Élargir la fenêtre de témoins **des voies q3/q4** au lieu d'une descente saturante | fenêtre 2K : 36 à 56 % de la masse q3 contre 82 à 91 % pour un proposeur parfait (plafond théorique, pas une mesure de moteur) ; la fenêtre 2K reste portée pour q2 seul |
| Lots de singletons entrelacés | ×1,005 à ×1,225 sur 54 comparaisons, aucune plus rapide |
| Redistribution `Donate`, détachement par ancre, équipe coopérative, plages d'ancres | pas de gain général ; 17 comparaisons rangées sur 18 plus lentes (coopératif) ; 8 favorables sur 72 (détachement) |
| Nouvelles micro-variantes q2 isolées | **consigne** des tranches 19 à 34 (constantes, pas exposants) ; restent différées : reprise exacte de la descente (×0,94–0,98, à réévaluer à 18 bits), produits de masse 1 |

## Voies q3/q4

| piste | fermée par |
| --- | --- |
| Census q3 scalaire sur le cover | 361 G tests ponctuels à 8k, 99,1 % extérieurs, ×10,8 de 4k à 8k |
| Window30 en remplacement de Local28 | médiane 1,153 plus lente ; K10 32k 241 s contre 153 s ; réserve du reçu : plus rapide à 32k/K5 sous charge (40,5 s contre 45,0 s) |
| Couches duales en remplacement de Local28 | adversaire K10 : ×10,27 |
| Couches duales comme filtre des graines q3 à K−1 | **différée, non fermée** : modèle de coût conditionnel (1,5 à 3,4 fois le census évité), à réexaminer si graines par arête dépasse le seuil ; régime sans sol 1 mm jamais mesuré |
| Parcours `Joined` graines × cellules | 57 M initialisations de cache à 32k, pas de gain stable sur LiveOnly |
| Pool universel, minimum collectif de graine, carte des centres comme cache de rejet | résidu dense, régressions jusqu'à ×1,56 |
| Covers comme stockage partagé | contre-régime : 65 024 lectures pour 14 sorties |
| Cache des formes dans la frontière de l'atlas ; classification conjointe des quatre cellules filles | neutre ; +18 % (quart de 7 067 sites, **non épinglé**) |
| Récursion relançant l'index par sous-rectangle ; reprise par listes d'IDs | 903 → 1 095 ms ; 450 → 511 ms (archive d'audit, **non épinglé**) |
| Rejeter q4 seul pour supprimer l'atlas | réserve de méthode, pas une réfutation : l'atlas sert aussi aux graines q3 |
| Crédit par nœuds d'index du certificat de voie morte, décisions exactes identiques | mêmes cellules et voies prouvées, mais CPU +27 % (K5) et +32 % (K10), +7 % sur le cœur seul : les seuils exacts forcent la résolution jusqu'aux sites ([reçu](../receipts/dead_node_credit_negative_20260923/README.md)) |
| Formes du cœur calculées à la première consultation par `prove` (variante paresseuse seule, CPU, sans sol) | les arêtes fermées ne lisent que 54 % des formes chargées, mais les formes pèsent environ 3 % de la phase (ticks TSC écoulés, indicatif) : gain projeté de l'ordre de 1 %, non borné ; brut, GPU et rejet avant cœur non jugés ([reçu](../receipts/q34_survivor_phases_20260923/README.md), [addendum](../receipts/q34_survivor_phases_20260923/ADDENDUM_20260923.md)) |
| Index des selles seul dans la phase 0 de la tour (lemme A du D5) | 1,01 M MEB évités à K10 16k, mais 10,2 M entrées à construire et trier : phase 0 inchangée ou plus lente ([reçu](../receipts/saddle_index_negative_20260923/README.md)) |
| Certificat de voie morte sur les 16 plus proches voisins globaux des extrémités (q3/q4) | exact, 62 % des requêtes de paire évitées à K5, mais 764 M tests de formes : CPU q34 −2 % à K5, +3,7 % à K10 ([reçu](../receipts/near_sites_negative_20260923/README.md)) |
| Raffiner en rectangles enfants un rectangle résiduel qui échoue au filtre témoin | paires développées 23,7 → 7,4 M, mais visites du filtre de rectangle ×2 et rejets bon marché du cache perdus : CPU q3/q4 +12 à +42 % ([reçu](../receipts/q34_micro_levers_20260923/README.md)) |
| Second cache de nœuds témoins par extrémité `b` | évite la recherche qui aurait établi le cache de la ligne `a` : recherches de paire +14 %, CPU +2 % ([reçu](../receipts/q34_micro_levers_20260923/README.md)) |
| Pas +1 des compteurs de travail sans contrôle de dépassement | CPU q3/q4 −4 %, mais change le contrat de dépassement des API publiques à ledger fourni (B) ; la version sûre, compteurs locaux du seul DFS témoin, ne gagne que 0,9 % ([reçu](../receipts/q34_micro_levers_20260923/README.md)) |
| Assimiler une sonde d'une arête ou un quart spatial à une mesure d'échelle | diagnostic « 8k/16k/32k » constant par construction (7 tests pour tout n) |
| Exposant par relation parent/enfant comme verdict de croissance | un compteur exactement linéaire reçoit 0,78 à 1,27 |

## Parallélisme, GPU, tour

| piste | fermée par |
| --- | --- |
| Jobs Coarse (sous-arbres entiers du front) comme seule unité | G4 W48 : 1,9 à 11,1 CPU occupés sur 48 |
| Ajouter des fils sans réduire le travail | les compteurs géométriques ne dépendent pas des workers |
| GPU par petits lots synchrones avec reconstruction hôte | v7 : 0,189 s de kernels pour 4,54 s de phase ; v6 : 154 ms pour 7 717 ms |
| Espérer 1 s par le port préfiltre/census seul | tour v7 418,9 s dont FULL 389,7 s |
| Copier dix fois le constructeur ; alias eager de toutes les facettes | allocations et calendriers multipliés ; plafond à 16k/K9 |
| Welzl à base non bornée ; comptabilité de supports incomplète | ×2,2 en candidats ; rapport annoncé 14,75 ramené à 3,25 |
| Déclarer un parallélisme ×20 sous découplage des ordres | plafond réel 11,4× ; le découplage échoue trois contrôles physiques |
| Juger un terminal par la seule composante finale | deux mutants survivants |

## Processus

| piste | fermée par |
| --- | --- |
| Enchaîner des campagnes par fichier sentinelle | sentinelle périmée : deux campagnes concurrentes, trois lignes écrasées |
| Lancer `ctest`, même `-N`, dans un build épinglé | quatre builds épinglés altérés le 21 septembre |
| Déposer une sonde de reçu dans un scratchpad | sonde de la phase 1 perdue |
| Preuves d'audit dans une archive jointe à une conversation | chiffres non rejouables depuis le dépôt |
| `git add` sans vérifier l'index partagé | commit d'audit `4c3cdb0c` emportant 300 fichiers du constructeur |
| Mutations par réécriture de texte liées à un arbre de build canonique | cinq tests désactivés hors de cet arbre, trois sans condition |
| Autorité de porte liée à un chemin de build épinglé | porte spatiale inexécutable là où `ctest` est permis |
| Chaîne de validateurs à inventaires cumulés et comptes figés | captures vertes refusées par leur propre lecteur |
