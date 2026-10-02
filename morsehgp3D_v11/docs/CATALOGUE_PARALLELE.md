# Catalogue parallèle et Pool de session

Tranche du 2 octobre 2026, CPU u18/u21/u24, hors registre, `not_claimed`.
Implémentation et contrelectures statiques terminées ; **qualification native
G4 attendue**. Le résultat reste CatK, sans forêt FULL ni verticale.

## Même géométrie, propriété explicite

L'API à trois arguments reste la référence séquentielle. La surcharge
`build_catalogue(cloud,params,budget,pool)` emprunte le Pool de la session.
Une préparation DFS gauche/droite coupe au plus à profondeur8, après le
filtre et l'ajustement de la boîte. Elle possède au plus256 listes prêtes ;
une feuille rencontrée avant cette profondeur devient également une tâche.
Chaque liste garde la capacité réellement allouée, pas seulement sa partie utile.
Les listes peuvent se recouvrir : leur somme n'est pas bornée par n.

Le suffixe reprend après cette préparation, sans recompter ni refiltrer sa
racine. Les visites du préambule, ajoutées une fois aux suffixes, reproduisent
les17 compteurs de la référence :15 sommes contrôlées et deux maxima.
Le choix des tâches dépend de la géométrie, pas du nombre de workers.
Un quota atomique unique admet les nœuds avant leur traitement ; un quota
neuf couvre le préambule et les suffixes de la deuxième passe. Une limite
ne devient donc jamais une allocation indépendante par tâche.

La première passe compte les boules et incidences par ordinal. Les sommes
préfixées contrôlées réservent exactement les émissions et populations.
La limite globale de boules est vérifiée avant ces allocations. La deuxième
passe rejoue et compare le préambule, puis remplit des tranches disjointes.
Chaque tâche doit retrouver ses comptes et son ledger. Ses offsets locaux
de population sont ensuite augmentés de son préfixe global. Le tri exact
et l'assemblage CSR restent ceux du séquentiel.

Avec J tâches et W workers, seuls min(J,W) workspaces sont alloués. Si J<W,
le workspace appartient à l'ordinal de tâche : un worker d'indice supérieur
à J peut néanmoins obtenir cette tâche. Sinon il appartient au worker.
Tous les résultats intermédiaires restent privés jusqu'au succès global.

## Admission de mémoire simultanée

Pour n sites, profondeur de coupe d≤8 et D=3B, une borne conservatrice de
préparation est `4*n*(2^d+d+2)` octets. Elle comprend la racine, la pile du
préambule et les listes finales, sans supposer leur disjonction.
Une tâche prête de c sites à profondeur h demande au plus `4*c*(D-h)`
octets supplémentaires de DFS ; une feuille prête n'en demande aucun.
La somme des W plus grandes bornes couvre tous les suffixes simultanés.

Un workspace de capacité c coûte exactement
`sizeof(Point)*c + 8*c*ceil(c/64) + 8*c` octets de buffers. Avant les
workers, le pilote admet leur somme et la borne des suffixes. Avant le
remplissage, les workspaces et listes possédées sont déjà dans `used()` ;
l'admission supplémentaire comprend les sorties et le maximum de la borne
suffixe et de `4*n*(d+2)` pour le rejeu. Rejeu et suffixes sont séquentiels.
Ces temporaires sont libérés avant l'assemblage ; émissions et population
restent vivantes pendant l'allocation de la sortie finale.

Les tableaux de256 propriétaires et compteurs ont une taille constante.
Les seules listes dépendant de l'entrée sont des Buffer. Les bornes
d'admission peuvent dépasser le pic observé ; ce ne sont pas des mesures RSS.

## Pool synchrone

`sched::make_pool` reçoit W explicitement,1..256 appelant compris. Il crée
W−1 threads persistants. Un appel à `parallel_for` emprunte un callback et
son contexte, sans allocation par travail. Une garde membre refuse les
appels concurrents ou réentrants, même vides. Aucun état global mutable ni
TLS ne choisit un rappel séquentiel implicite.

La réclamation par CAS sature à n, y compris pour UINT64_MAX. Toutes les
tranches sont exécutées et leurs refus fusionnés déterministement ; une
exception devient un Outcome et ne déclenche pas un rejeu. Le retour attend
l'acquittement de chaque worker, même sans tranche. La bascule d'époque
booléenne est sûre parce que tous acquittent avant l'appel suivant.
Une création partielle de threads est jointe avant le refus de la factory.
Les piles OS et le vecteur borné par W relèvent du coût de session, distinct
des buffers du catalogue.

Le Pool est un port critique explicite des [sources R2 épinglées](../src/sched/source_pins.json).
La frontière possédée, les deux passes et l'admission simultanée sont neuves.
Ni l'ancien tri parallèle, ni les tableaux non comptés, ni des chronos v10
ne qualifient cette implantation.

## Portes et mesures prévues

Les portes couvrent les travaux courts, la réentrance, les exceptions,
la création partielle, le réemploi et l'absence d'allocation par appel.
Le catalogue est comparé en W1/2/4/8 au séquentiel et à Fraction, notamment
aux contacts, faces obtuses, coquilles étendues et limites globales.
Les builds Release, ASan/UBSan18/24, TSan21 et poison21 se font sur G4.

Le [plan gardé](../bench/plans/parallel_cells_g4.json) déclare36 mesures
u21/u24 : K5 sur les six nuages en W48, trois processus distincts pour les
trois LiDAR W48, puis LiDAR W8 et K10/W48. Chaque processus a15s ; le
collecteur garde les omissions causales et celles de son budget750s.
Il compare géométrie, octets canoniques intraprofil et travail discret.
Le temps API comprend deux passes, tri et sorties en mémoire ; création
du Pool et Cloud sont mesurés séparément. Cette campagne ne mesure pas FULL.

## Diagnostic de la prochaine tranche

La surcharge accepte un cinquième argument optionnel `CatalogueTimings*`.
Absent, il ne déclenche aucune lecture d'horloge interne. Présent, un brouillon
remis au succès seulement sépare préambule, comptage parallèle, rejeu,
remplissage parallèle, tri, scan des niveaux, allocations et assemblage.
Ces intervalles murs sont disjoints sans couvrir tout le temps API : réductions,
contrôles entre phases et destructions restent dans le coût public.
Les sommes et maxima par tâche couvrent `execute_task` seul, hors correction
des offsets et vérifications après retour. Ils ne se soustraient jamais au mur.
Le nombre réel de comparaisons du tri par tas est mesuré séparément.

Le banc parallèle v2 demande ce diagnostic ; son lecteur contrôle les types,
les sommes/murs et la borne des comparaisons. Les anciennes captures v1 restent
épinglées à leurs sources. Portes natives on/off W1/W4 et banc W1/W8 prévues ;
le collecteur factice passe en normal/−O :34 essais,17 divergences,
11 calendriers et deux interruptions (lancement/décodage). Les pilotes
persistent commande, profil et hashes d’entrée avant subprocess.run ; cette
intention ne prouve pas un lancement. Le résultat natif est ensuite conservé
avant décodage. Aucun nouveau chrono acquis.

Le lecteur suivant ajoute `somme_taches <= min(W,J)*mur_phase` pour chaque
passe. Trois essais scalaires contrôlent les deux refus900ms>8×100ms et
la frontière800ms admise ; ils satisfont les anciennes inégalités. Ce contrôle
n’altère pas les captures antérieures, épinglées à leur propre lecteur.
