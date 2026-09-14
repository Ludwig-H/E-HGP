# Fondements : quel objet faut-il vraiment reconstruire ?

13 septembre 2026. Audit constructeur, appuyé sur le manuscrit et les
contre-preuves indépendantes v7. Aucune nouvelle qualification du moteur.
Le [périmètre de lecture](PERIMETRE_ET_PREUVES.md) distingue lecture
antérieure intégrale et passages réexaminés aujourd'hui.

## 1. La cible n'est pas un graphe k plus proches voisins usuel

Pour un rayon r, considérer les positions de centres dont la boule fermée de
rayon r contient au moins K points du nuage. Cet ensemble est noté L_K(r).
Lorsque r augmente, ses composantes naissent et fusionnent. HGP conserve
cette histoire. Cela n'est pas la filtration d'un graphe où chaque point
choisirait ses K voisins les plus proches.

Pour un ensemble F de K points, les centres couvrant F forment une
intersection de boules, convexe quand non vide. Elle apparaît au rayon de
la plus petite boule contenant F, notée MEB(F). Ces régions convexes
recouvrent L_K(r). Leurs intersections donnent une description combinatoire
exacte de ses composantes. C'est l'objet des définitions 20–22 et du
théorème 2 du [manuscrit, PDF 83–87](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf).

Le graphe Γ_K a pour sommets les ensembles F de cardinal K déjà apparus,
**y compris les sommets isolés**. Il suffit pour la connexité de conserver
les connexions élémentaires portées par des unions de cardinal K+1 ;
cela ne rend pas leurs adjacences identiques à celles du graphe complet.
Les feuilles sont des identités de composantes de facettes, pas une
partition des points. Deux composantes qui partagent un point ne sont
pas, pour cette seule raison, une même composante.

## 2. Ce que l'on peut supprimer — et ce qui doit rester

Sous les hypothèses régulières déclarées en v7, la réponse est simple :

- Garder les minima Gabriel de cardinal K comme feuilles, avec leur date.
- Garder les vraies multifusions, avec leurs dates et **tous leurs parents**.
- Calculer ces parents en tenant compte des connexions silencieuses ;
  ne pas conserver ces connexions comme nœuds publics inutiles.
- Pour toute la tour, garder aussi les cartes vers l'ordre inférieur.

K+1 est la cardinalité des cofaces qui portent les fusions, pas leur
nombre de parents. Un plateau peut réunir plus de deux parents.

Une facette Gabriel stricte F a une MEB ne contenant aucun site étranger,
coquille comprise. Toute extension aurait une boule de rayon strictement
supérieur : sinon l'unicité de MEB(F) imposerait que le site ajouté soit
dans cette même boule. F est donc isolée à sa naissance. Une facette
non-Gabriel possède au contraire un intrus qui permet un rattachement
au même niveau à des facettes antérieures. Sous régularité, les cofaces
non-Gabriel ne créent pas de vraie fusion nouvelle après cette résolution.

Les [preuves FULL et de confluence](../../morsehgp3D_v7/audits/receipts_gabriel_20260905/README.md)
justifient cette réduction. Avec L feuilles, I vraies multifusions et R
racines, on a $I\leq L-R$ et $L+I-R$ liens. La forêt topologique est donc
linéaire en L, **pas nécessairement en n**. Sa couverture régulière se
déduit de l'union des feuilles. Aucun catalogue exhaustif Γ n'est requis
comme sortie ni comme moteur.

## 3. Pourquoi « sommets Gabriel et graphe induit » ne suffit pas

L'intuition de remplacer les sommets de Γ par les seuls minima Gabriel
est pertinente **si les connexions portent les bons seuils des chemins
supprimés**. Elle est fausse si l'on garde seulement leur graphe induit.

Le [contre-exemple régulier à quatre points](../../morsehgp3D_v7/audits/receipts_gabriel_vertices_20260906/README.md)
retarde alors une fusion de 169/9 à 41/2. Il réfute le graphe induit sur
les seuls minima, pas à lui seul la proposition 6 littérale. La
[contre-fixture E5](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md)
réfute en revanche la proposition 6 littérale, comme égalité des collections
d'ensembles de points, et le théorème 5 qui en dépend : voir leur statut
`false_in_general` au [registre des preuves](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md).
Dans E5, AC n'est pas rattachée par le flot brut avant l'arrivée de ABC,
qui crée une composante artificielle. Ces énoncés du manuscrit ne sont
donc **pas** des autorités valables pour supprimer ces incidences.
Le caractère Gabriel nécessaire des événements critiques réguliers
(théorème 4) ne prouve pas, à lui seul, la suffisance d'un graphe amputé.

Le bon objet réduit est un graphe daté sur les naissances : ses connexions
sont calculées à partir des rattachements réels. Le graphe peut ensuite
être réduit à une forêt minimale qui conserve les composantes à toutes
les coupes, puis à l'histoire de ses multifusions. Un MST sur les points
ne fournit cette réponse qu'à K1, pas aux ordres supérieurs.

## 4. Où se cache le travail géométrique ?

Sous régularité, pour savoir de quelle naissance descend une facette non minimale, on
peut remplacer un point essentiel par un intrus strict : la MEB diminue
et le chemin reste admissible au niveau source. Itérer donne un terminal
minimal. Il faut ensuite le ramener à sa composante **à la bonne date**.
Sa racine finale ne suffit pas : plusieurs parents distincts peuvent
fusionner plus tard.

La [descente de cardinal K](../../morsehgp3D_v7/audits/receipts_gabriel_vertices_20260906/README.md)
explique cette possibilité ; la v7 ajoute un traitement exact des
coquilles et des ancres. Les résolutions de facettes distinctes peuvent
être préparées indépendamment de l'histoire des fusions. L'histoire
consomme ensuite leurs résultats. Il n'y a donc pas une dépendance
mathématique obligeant tous les calculs MEB à suivre un calendrier
global séquentiel.

Mais « possible indépendamment » ne signifie pas « bon marché » :
une descente peut répéter beaucoup de propositions de supports, de
tests de confinement et de recherches d'intrus. C'est précisément ce
que les [compteurs 50k](CONTRATS_ET_MESURES.md#2-les-quatre-vrais-runs-50k-full)
mesurent. L'objet terminal est petit ; sa recherche actuelle ne l'est pas.

## 5. Les entrées u16 ne sont pas toutes régulières

Avec A=(0,0,0), B=(2,0,0), C=(1,1,0), AB et ABC ont la même MEB.
La vacuité ouverte de AB ne garantit plus une naissance isolée : C est
sur la coquille. Perturber les données en silence changerait le problème.

La [construction non régulière v7](../../morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md)
regroupe les objets d'une même boule et traite les niveaux égaux en lots
atomiques. Une naissance peut alors couvrir plus de K points ; une
continuation peut acquérir de nouveaux points sans créer un nœud
topologique. Des contributions datées encodent cette information, sans
imposer des différences d'ensembles disjointes coûteuses.

La v8 doit conserver séparément : identités, histoire topologique,
contributions de couverture et ancres. La borne sur les seuls nœuds
topologiques ne borne pas le volume des contributions.

En dimension trois, une MEB de rayon positif admet un support positif
minimal d'au plus quatre sites. Cela ne borne **pas** la taille de sa
coquille : un nombre arbitraire de points peut être cosphérique. Les
singletons se traitent séparément. La limite active de coquille 12 est
une restriction d'implémentation, pas un théorème géométrique.

## 6. Partage des boules et paramètres

Noter p le nombre de points strictement intérieurs, q_min la taille du
plus petit support positif et Kmax le dernier ordre demandé. La fenêtre
utile est $p+q_{\min}\leq K_{\max}+1$, même avec coquille supplémentaire.
Une boule et son census doivent être partagés entre tous ses rôles ;
on ne reconstruit pas dix géométries indépendantes.

Pour une voie active de support q≤Kmax+1, il suffit d'établir au moins
$h_q=K_{\max}+2-q$ témoins strictement intérieurs pour l'éliminer :
la fenêtre ne peut plus être satisfaite par ce support. Donc à Kmax=10,
les seuils q2/q3/q4 sont 10/9/8, et à Kmax=5 ils sont 5/4/3.
Ces tests portent sur les voies et leur complétude ; un simple compteur
mal crédité n'est pas un certificat. La preuve détaillée et les pièges
de h, h_a, h_b figurent dans [l'audit WSPD](WSPD_Q2_Q3_Q4.md).

s WSPD contrôle la séparation des blocs, q le support, K l'ordre : aucun
de ces trois paramètres ne se substitue aux deux autres.

## 7. Dix forêts ne sont pas encore toute la tour

Comme $L_K(r)\subseteq L_{K-1}(r)$, chaque composante d'ordre K possède
une image à l'ordre inférieur. Pour une naissance K≥2, prendre l'ancre
inférieure de la même boule **après fermeture de son plateau**, puis
la normaliser à la coupe demandée. Les histoires déjà achevées permettent
des consultations indépendantes en parallèle ; elles n'autorisent pas
de confondre les identités des deux ordres.

Les [ancres partagées](../../morsehgp3D_v7/audits/receipts_shared_anchors_20260906/README.md)
permettent de réemployer la valeur géométrique commune. Elles ne permettent
pas de retrouver les parents supérieurs à partir de leur seule image
inférieure : cette image peut déjà être la même avant leur fusion.

Cas particuliers à garder explicites : K1 est le single-linkage, les
points naissent à zéro ; si K=n est demandé, X est une seule feuille
née à MEB(X), sans coface ni fusion. Les poids du manuscrit sont encore
un supplément : l'algorithme 1 les construit à partir de toutes les
facettes contributrices avant sa réduction, pas des seuls minima FULL.
Une couverture correcte ne constitue pas une implémentation de ces poids.

## 8. Le sous-quadratique universel explicite est une fausse cible

La [preuve indépendante de sortie](../../morsehgp3D_v7/audits/receipts_probe_meb_review_20260906/full_output_growth.md)
établit, pour chaque K fixé ≥2, des nuages réguliers exacts à précision
croissante de N=2m+K−2 points possédant au moins m² feuilles FULL.
Ce n'est pas une borne sur des cellules Delaunay inutiles : elle porte
sur la sortie demandée.

Idée de la construction : deux petits arcs fournissent m² paires croisées
dont les boules diamétrales excluent strictement les autres points des
arcs. Placer K−2 ancres dans leur intérieur commun donne m² ensembles
minimaux de K points. Les inégalités strictes survivent à une petite
perturbation générique, qui rend le nuage régulier.

Le [recalcul rationnel du 13 septembre](../receipts/audit_v7_20260913/MATH_CHECKS.json)
vérifie en modes normal et `-O` 1 360 labels, 34 720 puissances étrangères
et 3 740 puissances d'ancres ; les témoins u16 vérifient 9/25/81/289
paires croisées aux tailles 6/10/18/34. Ce contrôle fini ne prouve pas à
lui seul le théorème infini, ne régularise pas ces entrées et ne fait
tourner aucun moteur FULL.

Conséquences pour les contrats :

- Viser un coût supplémentaire faible **en plus du volume de sortie**.
- Mesurer séparément travail intermédiaire et taille du résultat dans
  chaque régime 8k/16k/32k ; un comportement uniforme ne couvre pas les arcs.
- Ne pas annoncer une asymptotique infinie dans l'univers u16 fini.
- Ne pas déduire de la borne une impossibilité matérielle de 1 s à 50k.
- Une représentation implicite reste envisageable, mais doit préciser
  les requêtes conservées, leur coût et le coût d'expansion complète.
  Ce serait un nouveau contrat, pas une substitution silencieuse à FULL.

## 9. Ce qui reste à prouver pour une v8 industrielle

Les preuves mathématiques décrivent l'objet ; elles ne certifient pas
toutes les omissions possibles du générateur C++. Un validateur de
bonnes boules ne prouve pas la complétude du catalogue. Un lecteur
structurel ne prouve pas la géométrie des parents. Un digest identique
n'est pas un oracle indépendant de grande taille.

Les obligations prioritaires sont la couverture de la construction WSPD
réellement choisie et son coût, les crédits de témoins sans doublon,
l'exactitude et la complétude des voies q2/q3/q4, les terminales avant
normalisation, les plateaux/verticales et la provenance immuable des
certificats. Les propositions v8 restent des propositions jusqu'à leurs
preuves et tests propres. Aucun statut `exact` n'est hérité de la v7.
