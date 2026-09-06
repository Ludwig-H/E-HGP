# Une ancre par boule pour reconstruire la tour

6 septembre 2026. Complément aux [preuves de plateau](README.md), sur les mêmes régions témoins K-NN. Cadre : phase=exploration_v7_hors_registre, backend=cpu_reference, profile=quantized_u16_input_only, mode=audit_independant_math_and_architecture, public_status=not_claimed.

**Une ancre fermée par couple (K, BallKey) suffit à représenter tous les labels Gabriel faibles de cette boule. Le catalogue amont garde sa fenêtre actuelle ; l’extension porte sur les composantes locales, leurs ancres et le certificat.** En régime régulier, chaque boule concerne seulement les deux ordres voisins déjà utilisés par la v7. Le résultat ne suppose pas que les parents puissent être déduits des seules coordonnées locales.

## 1. La fenêtre amont reste suffisante

Pour une boule B de rayon positif, conserver les notations I, U, p=|I|, u=|U|, S=I∪U et q_min, cardinal minimal d’un support positif sur U. Poser m=p+q_min et H=min(Kmax,n). Les positions sont distinctes, les calculs exacts et le census complet.

Le [théorème de fenêtre existant](../../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) et notre quotient local donnent : si m>Kmax+1, toutes les composantes strictes locales aux ordres demandés sont déjà connexes et couvrent S. Aucun événement de composante ni gain de points ne dépend de cette boule. Lorsque smax=n, m>smax est impossible. Le filtre conservant **m≤smax=min(Kmax+1,n)** reste donc sûr hors régularité.

Les [seuils de génération et du préfiltre](../../src/pipeline/expand.hpp) ne doivent pas être élargis pour ce raccord. Le RLE conserve la plus petite arité émise ; son égalité avec le minimum géométrique reste l’[obligation S1](../../docs/PREUVE_HORIZONTALE_COMPOSITION.md). Le quotient ne remplace pas cette preuve de complétude et ne transforme pas un préfixe sous budget en catalogue complet.

En revanche, **p+u≤smax n’est pas une condition d’admission générale**. Dans la sonde publiée à ceb163f9, cette condition est vérifiée seulement après exclusion des coquilles supplémentaires. L’étendre lors de la suppression du refus éliminerait la naissance K5 de la coquille à sept points déjà publiée. Le compte « une boule = un événement » reste également propre au régime régulier.

## 2. Un intervalle d’ordres, pas tous les labels

Une boule conservée ne demande une ancre que dans l’intervalle

$$\max(1,p+q_{\min}-1)\leq K\leq\min(H,p+u).$$

En dessous, le bloc est inerte et **aucun label Gabriel faible de cardinal K ou K+1** ne peut avoir cette MEB : un tel label contient I et un support positif, donc au moins m points. Au-dessus, S ne contient aucune K-facette. Les boules omises à un ordre peuvent toujours apparaître comme MEB intermédiaires d’une descente ; elles ne peuvent en être les terminaux faibles.

Pour u=q_min, l’intervalle est exactement m−1,m, après restriction à la tour demandée : rôle de connexion inférieur et rôle de naissance supérieur. Une coquille supplémentaire ajoute au plus u−q_min ordres possibles. Le nombre de cellules d’ancre d’une tour explicite est ainsi

$$N_A=\sum_B\max\left(0,\min(H,p_B+u_B)-\max(1,p_B+q_{\min,B}-1)+1\right).$$

C’est une borne de cellules, pas d’octets, de temps ni de stockage minimal obligatoire. Un traitement par ordre peut libérer ses tables de travail après avoir transmis les seules données nécessaires à l’export et à la verticale.

## 3. La clé de boule remplace les labels d’ancre

Après fermeture complète du niveau de B à l’ordre K, tous les K-sous-ensembles de S sont dans une même composante, puisque leur graphe de Johnson est connecté. Définir A[K,B] comme un token de cette composante, normalisable dans l’histoire de l’ordre K.

Tout label faible I∪T de cardinal K est donc représenté par cette ancre ; les facettes de tout label faible de cardinal K+1 le sont aussi. Plusieurs T, voire plusieurs supports positifs alternatifs, ne nécessitent aucun token d’ancre distinct.

Plus fortement, pour **toute facette F dont la MEB est B**, Gabriel ou non, F⊆S. Si A[K,B] est disponible à la coupe, elle résout F. Le lookup peut suivre immédiatement la MEB, **avant un nouveau census des intrus**. Ce hit ne dépend pas du label F ni de son arité de support choisie. Les tokens de deux ordres restent distincts, et deux boules ayant la même ancre ne deviennent pas une même identité géométrique.

Le [LocalBall actuel](../../src/forest/silent_incidence.hpp) contient déjà BallKey et niveau. Une table commune de géométrie peut partager ces champs. Elle ne doit pas confondre son arité locale avec q_min sur la coquille globale : pour les points (10,5),(0,5),(2,1),(2,9), le diamètre des deux premiers donne q_min=2, tandis que le triangle des premier, troisième et quatrième points est un support positif de cardinal trois de la même boule, de centre (5,5) et rayon 5.

**Un bloc sans effet public peut encore porter une ancre nécessaire à ce resolver.** Prendre les cinq points (2,2,2),(2,0,0),(0,2,0),(0,0,2),(0,0,0), de centre commun (1,1,1) et rayon carré 3. Les quatre premiers forment un support positif tétraédrique ; les premier et dernier un diamètre. À K2, le bloc est déjà connexe et couvre les cinq points avant 3 : les triangles du tétraèdre et ceux formés avec l’origine relient toutes les paires strictes. Pourtant la paire diamétrale, née à 3, est faible et sans intrus strict. Si l’on supprime l’ancre sous prétexte d’inertie, ce resolver ne peut pas la retrouver. Utiliser q=4 au lieu de q_min=2 pour une tour Kmax2 commettrait précisément cette omission, alors que la forêt abstraite n’a besoin d’aucun nouvel événement à 3. Un autre protocole pourrait résoudre ce terminal autrement ; ce n’est pas celui qualifié ici.

Une ancre présente mais issue d’un ordre refusé, d’un autre nuage, d’un niveau incohérent ou d’un lot non fermé est une faute d’autorité. Ce cas ne devient pas un miss autorisant le repli. Le niveau se compare exactement ; une racine finale compressée ne remplace pas la normalisation à une coupe ancienne.

## 4. Résoudre un miss sans catalogue global de facettes

Pour une facette F de cardinal K≥2, calculer sa MEB B. Un hit valide dans A[K,B] termine la résolution. En miss, trouver un intrus strict z ; retirer un sommet v d’un support positif choisi et continuer avec F′=(F\{v})∪{z}.

Les deux facettes sont reliées par la coface F∪{z}, au niveau de B. Le rayon de F′ ne croît pas. À égalité, la MEB reste B par unicité et le nombre de points sélectionnés sur sa coquille diminue de un. Le couple « rayon, cardinal de coquille sélectionné » décroît strictement sur un ensemble fini. La descente termine donc, sans exiger que le sommet du support choisi soit essentiel à toute la coquille.

En l’absence d’intrus strict, F est faible : I⊆F et sa coquille sélectionnée contient un support positif. Donc m≤K≤p+u ; sa boule est conservée et l’intervalle contient K. Si la demande initiale était strictement antérieure au lot consommateur, le niveau terminal l’est également. L’ancre a donc déjà été installée. Son absence est alors un défaut de catalogue ou de calendrier, pas une nouvelle naissance.

Les singletons à K1/rayon zéro sont initialisés séparément. Cette descente ne réclame que des MEB de cardinal K ; elle peut garder les raccourcis du moteur actuel si leur autorité est présente. Le coût des requêtes spatiales, des lookups et de la trajectoire reste à mesurer ; aucune accélération universelle n’est déduite de la borne de cardinal.

## 5. Construction et verticale sans circularité

Pour chaque K, parcourir les boules de l’intervalle par niveau. Préparer les composantes strictes locales par la table de coquille publiée, puis résoudre un représentant par composante dans l’état **avant le lot**. Toutes ces facettes sont strictes ; les résolutions précédentes terminent donc sur des ancres plus anciennes. Assembler les boules du niveau par leurs racines pré-lot communes, puis fermer atomiquement naissances, fusions et gains de couverture. Installer les A[K,B] seulement après cette fermeture.

L’induction sur les niveaux donne les bonnes composantes et leurs couvertures. Les blocs omis sous l’intervalle n’ajoutent ni fusion ni point ; les requêtes relatives à leurs nouvelles facettes sont résolues à la demande. Les boules distinctes du même niveau ne partagent pas de facette nouvellement née, par unicité de sa MEB : aucune union du lot ne dépend d’une ancre encore en cours d’installation.

Une naissance de plateau à l’ordre K≥2 correspond à une boule B sans parent antérieur. Son image verticale est **A[K−1,B] après fermeture du niveau inférieur**. L’intervalle inférieur contient ce rang : une naissance n’a aucune K-facette stricte, donc K≥m. La même valeur inférieure sert ainsi l’ancre horizontale et la verticale, sans choisir une coface unique. Les fusions transportent les images, et les gains de points n’imposent pas une nouvelle image : la naturalité découle des inclusions des régions témoins. K=n garde la naissance terminale et son image inférieure, sans ordre n+1.

L’algorithme de construction n’a besoin ni des membres complets de chaque composante, ni d’un catalogue des labels faibles I∪T, ni des adjacences Gamma globales. Il conserve des racines, des ancres, les composantes de coquille transitoires et les données du certificat. Les couvertures peuvent être exportées par deltas ; les affectations de toutes les facettes et les poids du manuscrit gardent leur supplément distinct.

## Vérification bornée

Le [modèle d’ancres](ball_anchor_model.py) sépare le producteur et le juge Gamma : le premier ne consulte jamais le second pour ses parents et ne conserve pas les membres globaux des composantes. La géométrie est fournie par le tableau rationnel exhaustif précalculé de l’oracle ; les compteurs MEB désignent ici des lectures de ce tableau, pas des exécutions du helper C++. Les niveaux de l’oracle sans bloc programmé donnent seulement des snapshots inchangés pour comparer les coupes.

**39 ordres, 502 coupes ouvertes/fermées et 1 703 facettes** sont comparés exactement. Les six tours précédentes retrouvent leurs digests ; le complément couvre shell7 avec Kmax5 et les deux mélanges d’arités. Le juge vérifie 91 ancres verticales de naissance, 334 images verticales et 304 carrés de naturalité, sans modifier les snapshots du producteur.

La production utilise 253 ancres, dont 44 naissances ponctuelles de rayon zéro, omet 49 blocs localement inertes et effectue deux descentes ainsi que deux hits non Gabriel. Le rejet global d’une boule hors fenêtre est aussi exercé. L’échange à rayon égal est vérifié séparément ; il ne s’est pas produit dans ces deux descentes de production. Le mutant de l’ancre inerte conserve la forêt mais provoque le refus attendu du resolver. Les versions internes de croissance ne sont pas des naissances ou multifusions supplémentaires.

Python [normal](ball_anchor_normal.json) et [optimisé](ball_anchor_optimized.json) terminent avec code 0, mêmes octets. Les [sources et contrôles](ball_anchor_review.json) bornent la portée. Il s’agit d’une proposition exécutable indépendante, pas du raccord C++, d’un certificat exporté puis rejoué ou de l’extraction des enregistrements 50k. Aucun moteur, compilation ou GCP utilisé par l’auditeur.
