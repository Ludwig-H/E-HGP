# Majorité exacte par deux sélections pondérées

Audit du 30 septembre 2026. Le calcul conserve exactement la majorité
stricte à dénominateur figé du candidat de bande ; ce n'est pas une autre
règle statistique. Preuve et prototype Python isolés, pas un moteur livré.
Aucun fichier produit modifié, aucun générateur ni GPU/GCP exécuté.

## La simplification

Pour un point x, les atomes choisis sont (c_i,v_i,w_i), avec poids positifs,
total W figé, dates exactes et propriétaires vivants. L'arbre est préparé
une fois, avec naissance b(v), intervalles Euler et requêtes LCA/ancêtre.
Avant admission, la masse d'un atome reste en réserve ; après, elle suit
uniquement les ancêtres de son propriétaire. Toutes les fusions et
admissions d'un même niveau sont simultanées. La majorité est strictement
supérieure à W/2, jamais supérieure ou égale.

1. Choisir m, nœud d'un atome médian pondéré dans l'ordre `tin(v_i)`.
   Il suffit de choisir la première clé dont la masse cumulée dépasse W/2.
2. Pour chaque atome, calculer h_i=max(c_i,b(LCA(v_i,m))).
3. La première date de majorité t est la première clé h dont le cumul
   pondéré, cohorte complète incluse, dépasse W/2.
4. Le propriétaire est `anc_t(m)`, avec la coupe fermée. Ne pas renvoyer
   m ou un LCA déjà mort à t, notamment dans un plateau de durée nulle.

**Preuve.** Tout sous-arbre qui contient plus de la moitié de la masse
totale contient m : son intervalle Euler laisse moins de W/2 à l'extérieur,
donc contient le quantile médian. Une composante active majoritaire a
a fortiori cette masse dans son sous-arbre futur ; elle est donc ancêtre
de m. À r≥b(m), l'atome i appartient à `anc_r(m)` exactement lorsque
c_i≤r et b(LCA(v_i,m))≤r, soit h_i≤r. Tous les h_i sont≥b(m).
Le premier cumul strict est donc exactement la première majorité dans
toute la forêt à racine commune. Les ancêtres témoins ne sont **pas**
redondants pour cette règle : retirer leur poids changerait W et les dates.

## Coût et parallélisation

Après préparation globale, coût proposé
O(D·coût_LCA+D+n·coût_ancêtre), D=incidences retenues réellement lues.
Deux sélections pondérées à partitions équilibrées suffisent ; pas de tri
complet par point ni de visite de tous ses ancêtres/enfants. Le prototype
utilise une sélection BFPRT pour ses pivots. Ne pas attribuer une borne
worst-case linéaire à n'importe quel `nth_element` ou quickselect.

Pour l'uniforme, les sélections sont les médianes hautes d'indice d//2,
même lorsque d est pair. Un port GPU peut faire sélection segmentée des
tin, D requêtes LCA indépendantes, puis sélection segmentée des h et n
requêtes d'ancêtre. Radix sur rangs entiers exacts possible sans trier
entièrement les listes ; transposition CSR, mémoire O(D), index global,
passes et allocations restent payés. Les rangs d'admission et de naissance
doivent appartenir au **même** ordre exact. Un `max` entre deux tables de
rangs indépendantes est incorrect ; les doubles coalescés ne suffisent pas.

Avec des poids 1/β, les sommes rationnelles peuvent avoir de grands
dénominateurs. La borne ci-dessus compte les opérations arithmétiques,
pas leur coût bit ; aucun accumulateur natif de taille fixe n'est qualifié.
L'uniforme est donc le premier profil pertinent à porter. Rien ne borne
ici D sous-quadratiquement sur LiDAR ou le coût de la génération FULL.

## Contrôles clos et limites

392 arbres/listes abstraits, dont 128 arbres aléatoires avec trois profils
de poids et huit cas ciblés. 7 742 atomes, 1 176 variantes d'ordre,
23 226 appels LCA logiques ; dates, propriétaires et masse gagnante
concordent avec le sweep exact par coupes. Départs différés, nœuds internes,
propriétaires répétés, cohortes et parents à même date sont inclus.

Quatre mutations donnent une mauvaise valeur : médiane basse, pivot
arbitraire, activation omise, suppression des ancêtres pondérés. Huit
entrées invalides sont refusées. Les trois sondes 8k/16k/32k portent sur
la **sélection de listes d'atomes**, pas sur des nuages, le générateur ni
la chaîne complète. Les visites enregistrées omettent les comparaisons
internes des groupes de cinq, les allocations et le coût bit : ne pas les
présenter comme un compteur complet de pipeline ou un benchmark LiDAR.

Une première exploration à 390 cas, puis un appel optimisé à 392 cas,
précèdent la capture close de deux appels normal/−O avec sources hachées
avant/après. Aucun échec mathématique rencontré. Le LCA et l'ancêtre du
test utilisent des marches de parents sur ces petits arbres : le prototype
ne qualifie pas encore l'index natif de la borne proposée. Pas de géométrie
3D, preuve d'avantage EOM/ARI, mesure de temps native ou contrat 100ms.

Lecture portable : `python3 -B verify.py` et `python3 -B -O verify.py`.
Inventaire et hashes vérifiés avant tout rejeu ; les lecteurs ne font
qu'exécuter le petit oracle Python et rejuger les captures. Les scripts
de capture ne sont pas nécessaires pour relire l'archive.
