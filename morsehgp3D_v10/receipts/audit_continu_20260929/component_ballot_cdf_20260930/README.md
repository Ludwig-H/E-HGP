# CDF de votes : la couverture d’une composante ne suffit pas

Deux contre-exemples exacts Fraction K2/K5, petits Γ complets colinéaires. Ni export natif FULL, ni EOM, ni performance, ni GCP. Le paquet ne propose jamais l’énumération globale des K-parties.

## Résultat à bande fixe η=1/8

K2 : sites0,19/10,39/20,2. α_0=19/20, bande supérieure171/160>1. Au rayon39/40, les cofaces012 et123 sont actives, partagent12 et donnent une composante de cinq facettes couvrant les quatre sites. Le vote03 n’est pas né. Masse de0=2, et non C(3,1)=3. Au rayon1, la même couverture et la même composante portent trois votes de0.

K5 : sites0 et19/10,48/25,97/50,49/25,99/50,2. α_0=49/50, bande supérieure441/400>1. Au rayon99/100, les cofaces{0,1,2,3,4,5} et{1,2,3,4,5,6} partagent la facette{1,2,3,4,5}. Leur unique composante a11facettes et couvre7sites ; les votes contenant0 et6 ne sont pas nés. Masse de0=5, et non C(6,4)=15. Au rayon1, les21facettes existent, la couverture reste7sites et la masse devient15.

Chaque nouvelle facette rejoint immédiatement la composante existante au second plateau : aucune fusion de deux composantes anciennes. Les traces montrent les Γ complets et admissions, pas un export natif de nœuds FULL. Ceci illustre pourquoi une CDF peut évoluer sans nouvelle multifusion positive. Une bonne couverture et sa taille ne donnent pas le compte des K-parties.

Le K2 est minimal pour cette réfutation lorsque Cov est l’union des sommets des facettes Γ : trois sites ne permettent qu’une facette isolée ou la coface entière, qui admet toutes ses paires. Le principe général sans énumération lourde : sur0..K+1, rayonK/2, deux (K+1)-cofaces consécutives fusionnent, mais toute K-partie contenant les deux extrêmes reste absente. Les deux captures choisissent des points proches en haut pour garder TOUS les votes de0 dans sa bande1/8.

## CDF correcte

Pour un point x, seuil de bande fixé b_x et rayon r, mettre t=min(r,b_x). La masse uniforme de référence est le nombre de K-parties F contenant x, de rayon MEB≤t, dont le propriétaire Γ au rayon r est C. C’est un nombre de PARTIES, pas le cardinal des sites couverts, ni le nombre de boules canoniques fortes, ni une masse EOM ou une mesure volumique. Des points retournés multiples d’un capteur réclament aussi une politique explicite ; ces fixtures ont des IDs/sites distincts.

Le nuage original et FULL peuvent encore suffire avec de nouvelles requêtes géométriques : le contre-exemple ne démontre AUCUNE impossibilité de reconstruction. Il réfute uniquement le raccourci C(|Cov(C)|−1,K−1) et l’usage des seules tailles/niveaux des nœuds.

## Identité de comptage possible, pas architecture industrielle

Pour les centres c, d_t(c) compte les boules FERMÉES B(x_i,t) contenant c. Si d_t(c)≥K, les K-parties des sites couverts appartiennent à une seule composante Γ au rayon r≥t : chaque (K+1)-partie de cette boule est une coface admise, et le graphe des K-parties par échange est connexe. Appeler λ_r,t(c) cette composante.

La masse cherchée est l’intégrale d’Euler à supports compacts de

1_{c∈B(x,t)} · 1_{λ_r,t(c)=C} · C(d_t(c)−1,K−1).

Preuve : chaque K-partie F est représentée par son ensemble de centres ∩_{i∈F} B(x_i,t). Cet ensemble est convexe compact, éventuellement singleton, donc sa caractéristique d’Euler vaut1 s’il est non vide,0 sinon. Sur cet ensemble, le propriétaire est celui de F. Additionner les indicatrices, puis utiliser l’additivité finie de χ, compte chaque F exactement UNE fois. Ne pas remplacer cette intégrale par un volume : les contacts/singletons comptent.

Le lecteur contrôle cette identité en1D sur les quatre snapshots : somme des valeurs aux endpoints MOINS somme des valeurs sur les arêtes ouvertes. Un intervalle fermé non vide compte1, un intervalle ouvert compte−1, un point compte1. Le calcul du propriétaire est contrôlé par les facettes Γ archivées ; aucun nouveau cas ni moteur n’est exercé.

Une autre identité est l’union des K-cliques des ensembles maximaux de sites couverts par une boule de rayon t, groupées par leur propriétaire Γ_r. L’inclusion-exclusion sur leurs intersections évite le double comptage algébriquement, mais peut coûter exponentiellement dans le nombre de cliques. Leurs cardinalités seules ne s’additionnent pas.

Ni l’intégrale d’Euler, ni les cliques maximales n’offrent ici de méthode industrielle : coût de stratification des centres, contacts, propriété Γ et incidences à établir. Un arrangement global coûteux déplacerait le problème. Ce sont des identités susceptibles de guider des requêtes locales/blocs et de servir d’oracles, pas des ports ni une preuve sous-quadratique.

## Capture

Deux appels check.py normal/−O, à17:49:00.970–17:49:01.102UTC, source hachée avant/après, code0, stderr vide et sorties identiques11123octets. Aucun préflight en échec. Le lecteur vérifie inventaire/hashes avant reçu, puis rejoue seulement cette petite preuve et les quatre comptages Euler1D.

Usage : python3 -B verify.py puis python3 -B -O verify.py. Reçus relocalisables, aucune dépendance LIVE ni accès GCP.
