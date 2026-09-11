# Calendrier filtré privé : référence structurelle, pas géométrique

Source autonome C++20 dans `filtered_calendar.hpp`, sans moteur actif ni nouvelle
arithmétique produit. `Date` et `Less` sont fournis par l'appelant et doivent
définir un ordre exact de dates valides. Des rangs issus d'un atlas exact sont
une entrée possible, mais cet atlas n'est pas construit ou certifié ici.

Le domaine est le **graphe réduit des naissances** : chaque arête est strictement
postérieure aux deux naissances qu'elle relie. Une arête égale ou antérieure est
refusée, pas interprétée comme un hub brut. Les identités natives distinctes,+même de date égale, restent distinctes jusqu'à leur connexion. Les hubs non
natifs sont des marques séparées, avec identité, représentant natif et admission.
Leur représentant ancien ne les rend pas disponibles avant cette admission.

## API et conservation

`Graph<Date>` porte `Birth{id,date}`, `Edge{id,a,b,date}` et
`Mark{id,representative,admission}`. Les identités sont u64 ; la seule valeur
réservée est `UINT64_MAX` pour l'absence de segment, pas un plafond de calcul.
Les boucles et arêtes parallèles d'identités distinctes sont admises ; Kruskal
les écarte ou conserve une occurrence à date minimale selon son ordre stable.

`spanning_certificate` produit une forêt minimale dont les arêtes conservent
l'identité, les extrémités et la date exactes de l'entrée. `verify_certificate`
vérifie séparément la provenance, l'acyclicité et, pour chaque arête originale,
la connexion de ses extrémités dans la forêt à sa propre date. Ce dernier
critère implique l'égalité H0 aux coupes ouvertes comme fermées. Le vérificateur
n'impose pas le choix précis de Kruskal parmi les arbres ex aequo.

`reconstruct` traite toutes les arêtes de même date ensemble. Il capture les
racines pré-lot, effectue les unions, puis crée une multifusion par groupe,
avec les anciens parents distincts. Aucun parent artificiel de durée nulle
n'est émis. `History` conserve les nœuds, l'arène de parents, les successeurs,
les racines finales, les références des naissances et celles des marques.
Chaque parent est d'indice et de date strictement antérieurs à sa fusion.
L'ancre d'une marque est calculée après fermeture entière de sa date.

La forêt minimale seule perd les arêtes non sélectionnées et leurs incidences
brutes : elle ne restitue pas R, les représentants d'origine, les couvertures
ou les verticales. La table de marques conserve seulement le rattachement
historique explicite fourni en entrée. Aucun payload ni référence verticale
n'est inventé, ni réaffecté implicitement à une racine finale.

## Coût et limites

Avec V naissances, E arêtes et M marques, construction et vérification coûtent
O((V+E+M) log(V+E+M)) comparaisons/opérations indicées et O(V+E+M) mémoire,
hors coût du comparateur exact. La reconstruction depuis la forêt coûte
O((V+M) log(V+M)) : seuls les endpoints touchés sont parcourus à chaque plateau,
et les marques sont traitées par balayage hors ligne, sans M parcours d'ancêtres.
Les consultations unitaires de référence restent O(hauteur), pas une API de
consultation massive optimisée. Le prototype est séquentiel ; il ne revendique
aucun speedup parallèle, GPU, ni sous-quadraticité en nombre de points 3D.
E et les sorties du producteur géométrique peuvent eux-mêmes être quadratiques.

Les copies de travail privilégient une référence lisible et vérifiable, pas la
résidence finale industrielle. Les fonctions construisent des valeurs locales
et lèvent une exception sur refus ; elles ne publient pas un préfixe de résultat.
`reconstruct` exige un certificat provenant de `spanning_certificate` ou déjà
vérifié contre son graphe source. Il ne peut deviner la provenance géométrique.

## Gate bornée

`gate.cpp` compare la forêt et l'histoire à des BFS par seuil indépendantes,
sur toutes les dates de naissance/arête/admission, ouvertes et fermées. Entre
ces dates le graphe est constant : ce corpus contrôle donc toutes ses coupes,
pas seulement l'état final. Les dates de test sont des rationnels u64 exacts :
le comparateur du helper fait un produit croisé u128, celui de l'oracle utilise
les fractions continues euclidiennes. Aucun flottant n'intervient.

Le corpus couvre graphe vide, isolés et racines déconnectées, triangle à trois
arêtes égales (une fusion ternaire), composantes sans relation malgré une date
commune, chaînes, marques non natives, doublons/boucles, identités au-delà de
2^32 et fractions aux extrêmes u64. Les corruptions portent sur dates, identités,
marques, cycles, arête manquante et forêt tardive. Ce sont des graphes abstraits,
pas une nouvelle qualification HGP, census, contribution ou verticale.

Fondement lu : proposition `GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md` et
preuve/modèle indépendants `audits/receipts_filtered_graph_20260911`. Leurs
267 cas et leurs résultats restent distincts de cette nouvelle gate C++.
GCP non utilisé ; aucun changement du moteur actif.
