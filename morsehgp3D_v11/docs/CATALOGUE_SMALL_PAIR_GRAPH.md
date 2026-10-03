# Graphe exact de paires dans les petites feuilles

Port neuf, opt-in `CatalogueParams::pair_graph`, désactivé par défaut.
Le modèle Python et les relectures passent ; qualification native G4 et
mesure FULL encore attendues. Aucun gain de temps n'est acquis ici.

## Certificat et parcours

La préparation existante teste déjà chaque paire de sites contre la boîte
fermée Q. Une dominance stricte dans un sens signifie que la bissectrice
de cette paire manque Q. Le graphe relie exactement les paires sans cette
dominance, contact compris. Tout support dont le centre appartient à Q
forme donc une clique de ce graphe. Cette condition nécessaire ne décide
ni la positivité du support, ni le census, ni l'appartenance à la boîte.
Les filtres suivants conservent leurs responsabilités et leurs tests.

Jusqu'à 32 sites, 32 mots de 64 bits portent les voisinages symétriques.
Au préfixe courant, le masque contient seulement les indices restants
compatibles avec chaque indice déjà choisi. Extraire son plus petit bit
puis intersecter avec le voisinage du nouveau site conserve l'ordre
lexicographique historique. Un préfixe impossible disparaît avant son
entrée dans la récursion ; aucun de ses descendants ne pouvait être émis.
Au-delà de 32 sites, le parcours et ses tests de paires restent inchangés.
Il n'y a ni quota de sortie, ni exclusion des triangles obtus, ni table
de triplets. Un tétraèdre positif à préfixe obtus reste une fixture.

Les visites évitées sont précisément les anciens rejets de paires dans
les petites feuilles. Dans un mélange de petites et grandes feuilles :
`prefixes_on = prefixes_off - pair_rejects_off + pair_rejects_on`.
Les tests et rejets de paires diminuent ; tous les autres champs du
ledger catalogue restent identiques. Les sorties, leurs populations et
leur ordre restent identiques. Une seule baisse du résidu ne prouverait
pas un gain : préparation, descendants et assemblage restent payés.

## Mémoire et qualification

Le workspace réserve 256 octets supplémentaires, même si la capacité
maximale permet des feuilles plus grandes. Le plan parallèle admet
256 fois le nombre d'espaces physiques simultanés, avant allocation.
Les mots sont réinitialisés entre feuilles et entre passes, y compris
sous poison. L'identité de la frontière inclut l'option du graphe.

Les portes natives prévues jugent masques aux limites 31/32/33/64/65,
ordre, contacts, q4 à préfixe obtus, repli mélangé, séquentiel et parallèle,
une/deux passes, admission globale, échecs d'allocation et récupération.
Le modèle indépendant vérifie 36 graphes abstraits et 234 configurations
géométriques, 303 contacts, 9 replis, 1677 émissions et 33 corruptions.
Ces comptes Python ne sont pas des résultats natifs.

Trois nouveaux mutants retirent le dernier voisin, omettent une
intersection, ou sous-comptent la mémoire des espaces. Le mutant historique
du préfixe obtus garde son défaut et sa porte ; seule son ancre textuelle
est adaptée à la nouvelle signature de récursion.

La v10 a inspiré le lemme des cliques. Aucun code ou résultat de la v10
n'est transféré implicitement ; u18/u21/u24 et FULL restent à requalifier.
