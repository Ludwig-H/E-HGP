# Collecte quadratique des témoins, même avec index idéal

Paquet autonome privé, aucun moteur/export natif/GCP. Les méthodes réelles
pair_level/third_sites/resolve de frontier_core.py sont copiées sans changer
leur corps (700–720 et733–765, enveloppe de classe privée), puis compilées
AST. Source intégrale86ba984f… pinée avant/après. Les anciens paquets clos
restent intacts. La factory, le catalogue et la forêt natifs ne sont PAS appelés.

## Famille exacte et coût analytique

A=128000 ; sites collinéaires entiersu18 a=0 et b_j=A+j, j=0..m−1,
2<=m<=32000. Les m votes de a sont dans la bande K2 eta1/4, puisque
d_2(a)=A et b_j<=160000=(5/4)A. Chaque site dense a seulement ses
voisins à distance1 : au totalD=m+2(m−1)=3m−2 votes dirigés, linéaire.

La boule diamétrale de(a,b_j) contient exactementj autres sites b_i,
i<j, strictement intérieurs. third_sites les matérialise TOUS dans une
liste, puis resolve les reparcourt et calcule leur distance à l’ANCRE
cur[0] pour choisir min. L’ancre restea, donc le choix est toujoursb0.
Mémo(a,b0) évite une nouvelle descente, mais ne dispense pas de scanner
les j témoins de chaque clé initiale DISTINCTE(a,b_j).

Ainsi pour toutes ces paires : j sommés donnent m(m−1)/2 prédicats de
census, et autant de distances d’argmin. Au pic, une liste contientm−1
éléments. L’exemple est indépendant de tout faux candidat d’une grille.
Pourm8000/16000/32000 on publie des comptes ANALYTIQUES, jamais des
exécutions quadratiques ni des timings.

## Ce qui reste linéaire avec la mémoïsation keep0

Un témoin z distinct des extrémités dans la boule FERMÉE satisfait
|a−z|²<|a−b|² : l’inégalité |a−z|²<=(z−a)·(b−a)<=|a−z||a−b|
n’est égale jusqu’au bout qu’en z=b. La descente termine et, à ancrea
fixe, chaque sous-paire d’une entrée de bande reste dans SA bande.

Chaque état frais visité est mémorisé avant le retour ; strict décroissance
interdit de répéter une clé dans un appel, et keep0 lit les clés existantes.
En mono/synchrone sans éviction, le nombre TOTAL d’états frais est donc
au plus le nombre de paires de bande non orientées rencontrables, <=D.
Cette preuve ne borne pas le coût de leurs census, ni keep1 sansmémo,
ni une exécution multi-worker à caches privés. Dans cette famille complète,
2m−1 états etcensus frais suffisent pourD=3m−2 requêtes dirigées.

## Oracle d’index et propriétaires : périmètre explicite

Le mock PerfectInteriorGrid ne fait PAS une requête de boîte réelle :
prepare(a,b) utilise les coordonnées collinéaires triées pour fournir
directement un générateur des intérieurs exacts (sans extrémités), dans
un ordre croissant ou décroissant. Il vérifie que la boîte passée par la
méthode réelle les contient. C’est une hypothèse d’index PLUS FORTE que
SiteGrid, destinée à isoler le travail aval paid-list/min, pas à qualifier
un index développé. Les compteurs sont des opérations de cette simulation.

Une forêt toy géométrique sert uniquement aux contrôles : chaque paire
dense adjacente naît àr1/2 ; elles sont toutes réunies àr1 ; (a,b0)
naît àrA/2, puis rejoint le groupe dense àr(A+1)/2. Pourm2 la branche
dense naît directement à1/2. Les propriétaires sont contrôlés vivants à
LEUR PROPRE niveau, et recoupés avec l’union exacte des intervalles L2.
birth_of_empty_pair est une table analytique, pas un lookup natif.

## Streaming et premiers témoins

Un argmin streaming exact rend le même témoin (distance àcur[0], ID
en cas d’égalité) avecO(1) stockage, mais il visite encore les j sites :
pas de gain asymptotique en temps acquis. Le contrôle keep1 choisit
le voisin du dernierb_j, PAS le site proche du milieu. Le mutant
« distance au milieu » est tué par le chemin différent, même si un autre
chemin peut encore atteindre le même propriétaire.

Un stopfirst dans un ordre arbitraire n’est PAS un argmin équivalent.
L’ordre inverse en donne un contrôle causal concret. Ce paquet ne
qualifie ni la correction générale d’une autre descente stopfirst ni
son temps. Le mutant cache ignoré est tué par le coût des replays,
pas par une affirmation de propriétaire faux.

Cela n’interdit pas un changement de témoin justifié séparément : dans
une tour FULL complète, un intérieur arbitraire peut garder le même
propriétaire au niveau de la paire, grâce aux réunions par cofaces dans
la boule. Ce théorème topologique ne promet pas la même feuille de
naissance, le même chemin brut ni les mêmes compteurs. Aucun port de
cette variante n’est fait ou qualifié ici ; le carré établi concerne la
collecte/argmin ACTUELS. Le contrôle streaming utilise un range paresseux
et garde un seul record candidat ; la référence min est rejouée à part.

## Captures

Petitsm exhaustifs2/3/5/8/16, deux ordres de requêtes et deux ordres
de candidats. Normal/−O identiques, pinssources début/fin. Reader exige
SHAmanifeste externe avantJSON/replay, inventaire fermé7 fichiers et
hashes aprèsreplay. Aucun assert ni dépendance horsstdlib.
