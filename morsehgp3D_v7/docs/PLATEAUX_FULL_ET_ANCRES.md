# Plateaux FULL : coquilles locales et ancres de boule

6 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le refus réel à 50k est expliqué géométriquement, pas encore levé dans
FULL.** L'extension ne réclame ni tous les niveaux Gamma ni un catalogue
de leurs facettes. Elle change le certificat, les terminaux du resolver
et la fermeture des lots. Les preuves indépendantes sont les
[contrats de plateau](../audits/receipts_plateaux_full_20260906/README.md)
et le [raccord par ancres](../audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md).

## Ce qu'il faut conserver, et pas davantage

Sous régularité, les minima de cardinal K et leurs multifusions suffisent
à retrouver les couvertures ponctuelles par union des feuilles. Hors
régularité, deux phénomènes imposent un supplément : une naissance peut
couvrir plus de K points ; une composante existante peut gagner un point
sans fusionner. La fixture ABCZ de l'auditeur montre ce second cas à K3.

Le futur certificat doit donc versionner les couvertures initiales, les
parents pré-lot et les gains de couverture datés, y compris lors d'une
continuation à un parent. Celle-ci ne devient pas une nouvelle naissance
ou une multifusion artificielle. Les identités persistent indépendamment
du recouvrement des points. Cela restitue les composantes et leurs
couvertures aux coupes, **pas** les dates de toutes les facettes ni les
poids du manuscrit. Le format régulier `full_minima_merge_forest_v1` n'est
pas réinterprété ; le supplément pondéré garde son propre contrat.

## Réduction locale sans combinaisons d'intérieurs

Pour une boule de rayon positif, I est son intérieur complet, U sa
coquille complète, p=|I|, u=|U| et q_min la taille minimale d'un support
positif du centre. Un sous-ensemble a le même rayon minimal que la boule
exactement lorsque ses points de U contiennent le centre dans leur
enveloppe convexe. Les supports positifs minimaux ont au plus quatre
points en 3D. Ils alimentent une seule table de sous-ensembles de U,
partagée par les ordres et les requêtes d'adjacence locales.

À K≤p, le graphe strict local est connexe et couvre I∪U : branche
analytique, sans énumérer les sous-ensembles de I. À K>p, poser t=K−p :
les sommets réduits sont les t-sous-ensembles stricts de U ; chaque
(t+1)-sous-ensemble strict relie ses faces par une étoile, pas une clique.
Un représentant est I augmenté d'un sommet réduit. Les couvertures
gardent I en facteur commun. Le graphe fermé a un seul bloc pour
1≤K≤p+u, et aucun pour K>p+u.

Le travail de table est local en u, avec propagation en O(u 2^u), hors
validation des données et lecture des résultats. Dans le domaine de
coquille actuellement représenté par le census (u≤12), il y a au plus
4 096 masques, contre huit pour chacun des cas 50k extraits. **Douze n'est
pas une borne mathématique de tous les nuages u16** ; les plus grandes
coquilles restent un domaine à étendre explicitement, jamais à tronquer.
Cette économie ne borne pas le nombre global de boules ni le coût de
leur découverte, et ne réfute pas la [borne quadratique de sortie](CROISSANCE_ET_BORNE_DE_SORTIE.md).

## Le raccord global : une ancre fermée par (K, BallKey)

La fenêtre amont reste p+q_min≤min(Kmax+1,n), sous l'obligation de
complétude S1. Ne pas lui substituer p+u≤smax : la coquille à sept points
de l'auditeur produit une naissance K5 et réfute ce filtre.
L'intervalle utile des ancres d'une boule est
max(1,p+q_min−1)≤K≤min(Kmax,n,p+u). Il retrouve les deux ordres voisins
en régime régulier.

Toutes les facettes de cardinal K de la boule ont une même composante
après fermeture de son niveau. Une ancre indexée par BallKey suffit donc
à les résoudre, même si le label particulier n'est pas Gabriel. Après
une MEB, un hit valide peut précéder le nouveau census d'intrus. En miss,
remplacer un sommet du support choisi par un intrus strict fait décroître
lexicographiquement le rayon puis le nombre de points sélectionnés sur
la coquille. Un terminal faible doit retrouver une ancre déjà fermée.
À rayon égal, un seul échange n'a pas nécessairement diminué le rayon :
le calendrier actuel exigeant une descente stricte doit être adapté.

Un représentant de chaque composante stricte locale est résolu dans
l'état **avant le lot**. Des chemins extérieurs peuvent déjà joindre
plusieurs de ces composantes ; leur nombre n'est donc pas le nombre de
parents globaux. Les boules de même niveau sont assemblées par leurs
anciennes racines, puis fermées atomiquement. Les ancres ne sont
installées qu'après cette fermeture. Une naissance supérieure retrouve
son image verticale dans l'ancre de la même boule à l'ordre inférieur
fermé. Les singletons à rayon zéro et K=n sont traités explicitement.

**Inertie publique ne signifie pas ancre inutile.** Le contre-exemple à
cinq points de l'auditeur garde la forêt inchangée après omission d'un
bloc, mais son resolver échoue faute d'ancre d'un terminal faible. Une
table d'ancres n'est donc pas limitée aux nœuds publiés.

## Quatre cas réels à 50k

Le [paquet d'extraction local](../receipts/full_extra_shell_50000_20260906/README.md)
conserve les clés, rayons rationnels, coordonnées, identifiants externes
et rangs Morton. Le lecteur indépendant régénère les 50 000 points et
les scanne tous pour **chacune des quatre boules fournies** ; cela vérifie
leurs I/U, pas l'exhaustivité du catalogue. Python normal et optimisé
retrouvent les mêmes données. Chaque coquille possède un unique diamètre
positif, q_min=2, u=3 et des poids 1/2 sur ses deux extrémités.

| Indice de boule dans cette capture | p | Ordres d'ancre dans K1..10 | Quotient local notable |
| ---: | ---: | --- | --- |
| 174406 | 3 | 4..6 | K5 : deux composantes strictes ; K6 : naissance locale |
| 254569 | 0 | 1..3 | K2 : deux composantes strictes ; K3 : naissance locale |
| 996863 | 4 | 5..7 | K6 : deux composantes strictes ; K7 : naissance locale |
| 1251653 | 9 | 10 | Un seul groupe couvrant I∪U à tous les ordres demandés ; ancre K10 à conserver |

« Deux composantes strictes » ne certifie pas une fusion globale. Le
premier rang de chaque intervalle est ici localement inerte, mais peut
porter une ancre nécessaire. Le filtre amont du repli K5 retient les trois
premières boules et écarte la quatrième : c'est une déduction depuis
cette capture K10, **pas une nouvelle extraction K5**. Les anciennes
mesures G4 K10/K5 étaient deux vrais processus, sans ces clés détaillées.

L'extraction a pris 132,027 s externes sur huit threads locaux, avec
compilations légères concurrentes : durée diagnostique jusqu'au refus,
pas benchmark apparié ni temps de tour. Même entrée, mêmes volumes
U=21 685 604 et S=21 468 368 que le refus G4 K10 ; ce rapprochement ne
prouve pas l'identité de chaque boule non exportée par l'ancien binaire.
Le nouveau flag `--extra-shell-diagnostics` est facultatif et conserve
le refus de régularité. Sans lui, aucun champ diagnostique n'est ajouté.

## Ce qui reste à raccorder et à mesurer

Le quotient local ne décide ni parents globaux, ni lots, ni verticale,
ni certificat étendu. La suppression du refus ne sera correcte qu'après
ce raccord et sa qualification sur coupes ouvertes/fermées, continuations,
naissances de plateau, racines répétées et ancres silencieuses.
Le traitement régulier doit conserver son chemin économique ; aucune
table de 4 096 masques n'est à construire systématiquement pour les
millions de boules régulières.

Les contrats 50k/1 s puis 100 ms et plusieurs dizaines de millions G4
restent non atteints. La découverte WSPD, le FULL séquentiel et les
résidences globales restent des chantiers de performance distincts.
GCP non utilisé pour cette extraction et ces qualifications locales.
