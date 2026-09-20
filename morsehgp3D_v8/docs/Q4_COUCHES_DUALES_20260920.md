# q4 : retirer ensemble les faces et témoins impossibles

20 septembre2026, tranche29 aprèsc051bdb0. Cadre inchangé :
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `public_status=not_claimed`.

## Idée et différence avec28

Les cellules28 partageaient les témoins, mais relisaient encore beaucoup
de sites pour chaque face. Cette tranche prépare une sélection commune
à **tous les centres** d'une arête. Un site écarté est prouvé extérieur
à toutes les boules susceptibles d'être acceptées. Il n'est donc plus
ni un témoin à compter, ni une face à balayer. On retire du travail
avant la génération des événements, sans modifier les résultats attendus.

On n'augmente ni la profondeur de carte ni un budget de pool. L'ancienne
voie reste inchangée et sert de référence réelle, pas de temps extrapolé.
Les objets nouveaux sont un ensemble immuable de sites retenus et des
buffers privés de balayage ; aucun nuage ou index n'est reconstruit.

## Certificat exact

Dans le plan de centres de28, un site a une forme affine
$L_z(t)=c_z+x_z\xi+y_z\eta$, dont le signe négatif exprime l'intérieur
strict de la boule. Pour q4, poser $T=K_{\max}-2$ ; seuls les centres
de profondeur strictement inférieure àT sont utiles à ce producteur.
Toutes ces formes portent les sites du cover certifié de l'arête.

Garder tous les sites avecc=0. Pourc≠0, définir le point rationnel
$p_z=(x_z/c_z,y_z/c_z)$ et séparer les deux signes dec. Dans chaque
groupe, extraire successivement T frontières d'enveloppes convexes,
avec **tous** les points sur les arêtes et tous les IDs de points
duaux coïncidents. Les frontières retirées du problème géométrique sont
**gardées** comme témoins. L'intérieur restant après T couches est,
lui, écarté du census. Si l'enveloppe n'est pas de dimension2, tout
son restant est conservé ; pas d'inférence de stricte inégalité.

Soit z finalement écarté. Il est strictement intérieur à chaque enveloppe
antérieure de son groupe. Pourt≠0, une fonction linéaire non constante
sur le plan prend un minimum strictement inférieur et un maximum
strictement supérieur sur chaque frontière. Si $L_z(t)\leq0$ :

- pourc_z>0, $1+p_z\cdot t\leq0$ ; chaque couche contient un point
  avec une valeur strictement plus petite, donc un site strictement intérieur ;
- pourc_z<0, $1+p_z\cdot t\geq0$ ; chaque couche contient un point
  avec une valeur strictement plus grande, encore un site strictement intérieur.

Les couches sont disjointes : cela donne T IDs retenus distincts. Àt=0,
un sitec>0 est extérieur ; si un sitec<0 a été écarté, les T couches
de son groupe donnent directement T intérieurs retenus.

Ainsi, pour tout centre, **si la profondeur sur les retenus est<T,
tous les écartés sont strictement extérieurs**. C'est l'implication
requise pour calculer uniquement sur l'ensemble réduit, pas seulement
une conclusion conditionnée à la profondeur globale inconnue.
Profondeur exacte et coquille complète du cover sont alors conservées.
Une seed écartée, pour laquelleL_z=0 sur sa droite, ne peut donc porter
un centre admis. Aucune liste d'intérieurs écartés n'est à rajouter,
aucun créditT ne doit initialiser le census.

Ce certificat est distinct du transfert cover→nuage : comme24, seuls
la positivité du tétraèdre et son arête propriétaire certifient que la
boule fermée entière est contenue dans le cover. Les groupes profonds
réduits n'ont pas nécessairement leur profondeur globale exacte.
q3 a un autre seuil ; ce port n'autorise pas à y réutiliser T sans preuve.

## Arithmétique et coût de la sélection

Les fractions ne sont pas divisées en flottant : normaliser le signe
du triple(x,y,c) pour un dénominateurc>0, sans confondre les deux groupes
d'origine. Les comparaisons lexicographiques croisent deux coefficients.
L'orientation est le déterminant homogène des trois triples, pas un
produit de différences de fractions. Sous u16,M=65535, les bornes28
|c|≤15M² et |x|,|y|≤8M² donnent une orientation≤5760M⁶<2¹⁰⁹ ; i128 suffit.
Tous les produits sont promus avant multiplication.

Un seul tri rationnel, regroupement des coordonnées identiques, puis
chaînes monotones et compactage des survivants **sans retri** à chaque
couche. CoûtO(m log(1+m)+Tm), mémoireO(m), pourm sites du cover.
L'ordre de sortie est celui des IDs originaux ; tous les IDs d'un groupe
dual conservé sont restitués. Déplacements, copies de hull, duplicats
et capacités simultanées sont payés ; aucune borne RSS n'est déduite.

## Raccord, parallélisation et limites

`Q4ShallowSet` possède sa géométrie et ses IDs ; elle peut être partagée
en lecture entre appels. `run_q4_shallow_edge_candidates` construit cette
sélection une fois, puis balaye uniquement les seeds aiguës propriétaires
retenues contre les témoins retenus. Les buffers sont privés et réutilisés
entre seeds. Le compte repart de zéro, sorties puis observation puis
entrées à chaque groupe, sans saturation. Canonisation et coquilles
gardent le contrat23/24 ; seules les vues synchrones sont émises.

Cette première intégration n'empile ni pool26 ni atlas28. Le coût aval
estO(Sr log(1+r)) pourS seeds etr témoins retenus, hors travail ajouté
par le consommateur et éventuelle matérialisation répétée des coquilles.
**r peut encore égalerm** :
sites duaux tous convexes, grands groupes coïncidents ou nombreuxc=0.
Le prétraitement est sous-quadratique à Kmax fixé ; il ne qualifie donc
pas tout le moteur. Si K croît avec m, le termeTm doit rester explicite.
Le scan communO(m) par arête doit lui aussi être payé au raccord WSPD.
Aucun ordonnanceur q4, catalogue, intérieurs, FULL ou GPU n'est ajouté ici.

Pour situer le chantier suivant, le résultat de
[Har-Peled et Sharir, §2.1](https://www.math.tau.ac.il/~michas/k_depth.pdf)
borne les sommets de profondeur≤k parn(k+1), en position générale.
Cela motive une énumération des seuls événements peu profonds ; cela
ne donne ni le coût du présent balayage, ni une qualification de ses
dégénérescences/coquilles, ni une borne sommée sur toutes les arêtes.
Notre certificat de couches ci-dessus est démontré séparément et ne
suppose ni position générale ni perturbation des données.

## Qualification et décision

Qualification close :90 CTests Release, huit gates/dix sondes Clang
ASan/UBSan (pas90),3686 contrôles de la nouvelle gate, trois mutants
compilés causalement tués,20 différentiels de l'ancienne voie contre28,
52 mesures et lectures normal/−O concordantes. Les184 sources et builds
`build/v8_q4_shallow_20260920` / `build/v8_q4_shallow_sanitize_20260920`
sont désormais épinglés. Voir les [reçus](../receipts/q4_shallow_20260920/README.md).

Dense permutéK10 : sites retenus408/708/1256 à8k/16k/32k,
W165648/499848/1575024 (×3,018/×3,151), tris×3,251/×3,475.
Les sorties restent égales à28 réellement exécuté. Le run32k passe
de1105,682 à562,050ms, mais un essai sur hôte partagé ne qualifie pas
un gain stable. À8k ce chemin trie davantage que28 ; à32k il rejette
encore803471 groupes profonds après construction des événements.

Contre-régimes publiés : adversaireK10 ne retire aucun site et reste
quadratique, run256 régressant×10,27 ; cap32k/K10 est réduit à1149
témoins mais deux seeds suffisent et les blocs28 sont beaucoup moins
coûteux. Ne pas remplacer28 automatiquement. Prochaine question :
extraire seulement les événements peu profonds ou composer les couches
avec les partitions spatiales, en payant construction, conflits et sorties.
La mémoire propre monte aussi :1861533octets contre267672 pour28
à32k/K10 permuté, capacités couplées et non RSS.

s8/10/12 est sans objet dans cette primitive à arête fournie et reste
à comparer au raccord global. Aucun contrat50k/G4 ou massif acquis ;
GCP non utilisé.
