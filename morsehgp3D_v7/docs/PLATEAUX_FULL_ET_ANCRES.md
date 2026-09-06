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

Le [complément de l'auditeur](../audits/receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md) autorise une
forme factorisée : une contribution de couverture datée par bloc, liée
à son token après fermeture et à une population immuable. Les contributions
peuvent se recouvrir ; le lecteur fait une union ensembliste. Aucun
ensemble global de points par racine n'est requis à la production.
Plus précisément, soustraire de I∪U l'union des couvertures strictes
locales donne une contribution potentielle suffisante. Avec au moins
un groupe strict, I est déjà couvert : seul un masque de U reste à
enregistrer. Sans groupe strict, la contribution est tout I∪U.

Ce n'est **pas un delta global disjoint** : des chemins extérieurs
peuvent avoir déjà apporté certains de ces points. Une contribution vide
peut être omise, pas l'ancre nécessaire au resolver. Chaque contribution
reste sur son segment temporel ; l'attribuer rétroactivement à une feuille
ou à la racine finale ferait fuiter des points dans les anciennes coupes.
Ces règles simplifient le certificat à construire ; aucun format FULL
étendu n'est déjà livré par ce seul contrat.

## Réduction locale sans combinaisons d'intérieurs

Pour une boule de rayon positif, I est son intérieur complet, U sa
coquille complète, p=|I|, u=|U| et q_min la taille minimale d'un support
positif du centre. Un sous-ensemble non vide de I∪U a le même rayon minimal que la boule
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

L'[audit indépendant du census complet](../audits/receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md#3-ce-que-les-quatre-coquilles-réelles-permettent-de-conclure)
permet de conclure davantage : les trois naissances K6/K3/K7 sont
géométriquement prouvées, de couvertures six/trois/sept points, sans être
déjà produites par FULL. Pour les blocs à deux groupes stricts, leur
union couvre tout I∪U : aucune croissance ponctuelle n'y est possible ;
seules les éventuelles fusions et leurs parents restent à décider.

Le [complément global de l'auditeur](../audits/receipts_plateaux_full_20260906/GLOBAL_PARENTS.md),
publié ensuite dans `a22a65f9`, tranche ces trois ambiguïtés :
174406/K5 a un seul parent, via un chemin strict extérieur par 45617 ;
254569/K2 et 996863/K6 ont chacun deux parents distincts, une facette
étant isolée avant le niveau. Les scans du nuage complet et quelques MEB
rationnelles suffisent à ces certificats particuliers, sans catalogue
Gamma. Le nombre deux est celui des parents du bloc, pas nécessairement
l'arité de la multifusion regroupant d'autres boules du même niveau.
Le premier bloc n'apporte ni topologie ni point, mais conserve son ancre.
Ces preuves fixent les attentes du raccord ; elles ne sont pas encore
des sorties du constructeur C++.

L'extraction a pris 132,027 s externes sur huit threads locaux, avec
compilations légères concurrentes : durée diagnostique jusqu'au refus,
pas benchmark apparié ni temps de tour. Même entrée, mêmes volumes
U=21 685 604 et S=21 468 368 que le refus G4 K10 ; ce rapprochement ne
prouve pas l'identité de chaque boule non exportée par l'ancien binaire.
Le nouveau flag `--extra-shell-diagnostics` est facultatif et conserve
le refus de régularité. Sans lui, aucun champ diagnostique n'est ajouté.

## Composant local implémenté et qualifié

[local_plateau.hpp](../src/forest/local_plateau.hpp) fournit désormais
`mhgp7::local_plateau::ShellTable` : préparation unique de la coquille,
quotient `rank(K)`, représentants, couvertures factorisées et contribution
potentielle `contribution_shell` / `contribution_interior`. Les identifiants
externes sont conservés et les points intérieurs partagés, sans copie
par facette. La liste des masques réduits de chaque composante est locale
à U ; aucun membre global de Gamma n'est matérialisé.

Ce helper utilise seulement les prédicats géométriques élémentaires de
`plateau.hpp`, **pas** son ancienne expansion exhaustive d'événements.
Il vérifie son domaine arithmétique avant calcul : A<2^68, |B_i|<2^87,
|C|<2^105, coordonnées u16 et census local cohérent. La complétude de I/U
relativement au nuage reste une précondition extérieure, pas un fait
établi par ces contrôles. Aucun plafond d'opérations ou d'intérieurs
n'est ajouté ; u≤12 reste le domaine de représentation déclaré.

Le [reçu propre au composant](../receipts/local_plateau_20260906/README.md)
ferme quinze commandes de compilation/tests, dont deux CTests :
18 tables / 96 rangs contre l'oracle rationnel indépendant, et 40 rangs
supplémentaires issus des quatre cas réels. O2 et ASan/UBSan avec détection
des fuites donnent les mêmes résultats ; le mutant supprimant les unions
d'étoile est rejeté sur le nombre de composantes. Les cas de croissance
sans fusion, naissance couvrant plus de K points, chemins extérieurs,
u=12 et p=5000 sont exercés. Les prototypes échoués sont conservés sous
leur propre autorité, sans transfert de leurs succès aux sources promues.

Le [supplément qualifié](../receipts/local_plateau_diameter_20260906/README.md)
répond au renforcement du juge demandé par l'auditeur : 18 ensembles
**complets** de supports minimaux comparés à Gram ; le mutant retirant
un diamètre du carré sans modifier la table booléenne est réfuté.
Pour q_min=2,u≥3,K=p+1, un chemin analytique émet directement la seule
composante stricte, sans DSU ; 17 occurrences testées, u=2 exclu.
La préparation partagée des supports reste inchangée et l'ancre fermée
reste nécessaire. Aucun gain de temps global n'est déduit de ce raccourci.

Le [journal daté v2](CONTRAT_COUVERTURES_DATEES.md) est aussi implémenté
et qualifié séparément : banque I/U partagée entre ordres, naissance de
couverture éventuellement supérieure à K, continuation sans nouveau nœud,
contribution datée et racines historiques aux coupes. Son autorité est
structurelle, sans certification du census ni des parents fournis.

Ce composant n'est encore appelé ni par `full_gabriel.hpp` ni par la
sonde FULL : **aucun gain de temps de tour ni levée du refus 50k n'est
attribué à cette implémentation locale.**

## Ce qui reste à raccorder et à mesurer

Le quotient local ne décide ni parents globaux, ni lots, ni verticale.
Le journal étendu est disponible, sans raccord au constructeur. La
suppression du refus ne sera correcte qu'après
ce raccord et sa qualification sur coupes ouvertes/fermées, continuations,
naissances de plateau, racines répétées et ancres silencieuses.
Le traitement régulier doit conserver son chemin économique ; aucune
table de 4 096 masques n'est à construire systématiquement pour les
millions de boules régulières.

Les contrats 50k/1 s puis 100 ms et plusieurs dizaines de millions G4
restent non atteints. La découverte WSPD, le FULL séquentiel et les
résidences globales restent des chantiers de performance distincts.
GCP non utilisé pour cette extraction et ces qualifications locales.
