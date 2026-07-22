# Incidences fermées des selles directes — Phase 10.4

## Portée du jalon

Phase 10.4, backend reference_cpu, profil hgp_reduced, mode certified, sémantique partial_refinement.

Ce jalon complète l'alphabet local d'une selle directe sans construire de forêt d'ordre supérieur. Il réutilise les bras stricts de 10.2, ajoute seulement les facettes obtenues par suppression d'un point intérieur et fournit séparément un petit noyau combinatoire sur des racines externes déjà certifiées. Il ne localise aucune facette, ne génère pas les gateways non directs, ne conclut jamais qu'une facette absente est isolée et ne publie aucune action de hiérarchie.

## 1. Partition stricte et égale des facettes

Soit un événement terminal direct de coface fermée $S=I\cup U$, où $U$ est le support positif minimal de sa miniball, $I$ l'ensemble de ses points strictement intérieurs et $a=\beta(S)=\beta(U)$. Pour une selle d'ordre $k$, on a $\lvert S\rvert=k+1\leq11$ et $2\leq\lvert U\rvert\leq4$.

Les facettes de $S$ se répartissent en deux familles disjointes et exhaustives :

- pour $u\in U$, la facette $F_u=S\setminus\lbrace u\rbrace$ est un bras strict et 10.2 prouve $\beta(F_u)<a$;
- pour $x\in I$, la facette $G_x=S\setminus\lbrace x\rbrace$ naît exactement au niveau $a$.

La seconde assertion est une conséquence immédiate de la monotonie de la miniball. Comme $U\subseteq G_x\subseteq S$, on obtient :

$$a=\beta(U)\leq\beta(G_x)\leq\beta(S)=a,$$

donc $\beta(G_x)=a$. Aucune miniball supplémentaire n'est nécessaire dans le chemin produit. Les identifiants retirés appartiennent respectivement aux ensembles disjoints $U$ et $I$; les deux familles contiennent ensemble $\lvert U\rvert+\lvert I\rvert=\lvert S\rvert=k+1$ facettes, donc aucune suppression locale n'est perdue.

## 2. Couverture en points et nécessité des clés de facettes

Les bras stricts couvrent déjà tous les points de $S$. En effet, pour chaque $y\in S$, il existe $u\in U$ avec $u\neq y$ puisque $\lvert U\rvert\geq2$, et alors $y\in F_u$. Ainsi :

$$\bigcup_{u\in U}F_u=S.$$

Une suppression intérieure de niveau égal n'ajoute donc aucun PointId à la couverture de cet événement isolé. Elle ajoute néanmoins une clé de facette indispensable : deux cofaces simultanées peuvent partager cette clé, et une coface silencieuse antérieure peut l'avoir attachée à une racine qui sera réutilisée plus tard. Une égalité du delta de points n'autorise jamais à supprimer l'incidence de facette.

## 3. Représentation factorisée et bornes

Notons $J$ le nombre de selles directes, $A=\sum_e\lvert U_e\rvert$ le nombre de bras stricts et $P=\sum_e\lvert I_e\rvert$ le nombre de suppressions intérieures. Pour $K\leq10$ :

$$A\leq4J,\qquad P\leq9J,\qquad A+P=\sum_e(k_e+1)\leq11J.$$

Le journal 10.4 ne recopie pas les $A$ graines de 10.2. Il conserve une famille par selle et une graine source_family + removed_interior_point_id par suppression intérieure. Son ajout vérifie donc :

$$J+P\leq10J\leq10E.$$

Comme 10.1 et 10.2 sont bornés ensemble par $3n+10E$, la coexistence des trois journaux vérifie :

$$\text{stockage}_{10.1+10.2+10.4}\leq3n+20E.$$

Chaque facette est reconstruite à la demande dans un tableau fixe de dix PointId, triée et vérifiée contre les digests des autorités. Le préflight calcule $J$, $A$ et $P$ depuis la façade terminale avant le rejeu de 10.2 et avant toute réservation. Le vérificateur parcourt familles et graines en flux sans construire une seconde sortie $O(J+P)$.

Le journal ne matérialise ni ensemble de facettes, ni miniball, ni cellule, ni coface globale, ni mosaïque de Delaunay d'ordre supérieur, ni coupe Gamma, ni table de composantes. Son coût intermédiaire est proportionnel aux événements directs déjà retenus.

## 4. Fermeture combinatoire sur racines gelées

Le noyau autonome direct_frozen_root_quotient traite un cas volontairement plus étroit. Étant donnés un ensemble fini de racines externes et des hyperarêtes CSR non vides dont toutes les références de racines ont déjà été certifiées par l'appelant dans le snapshot strict, la DSU locale calcule exactement la plus petite relation d'équivalence engendrée par ces hyperarêtes.

La preuve est standard : chaque union ajoute une paire imposée par une hyperarête, donc la partition produite est plus fine que toute relation d'équivalence admissible; réciproquement, toute paire reliée dans la DSU appartient à la clôture transitive des unions imposées, donc aucun raccord supplémentaire n'est créé. La partition est indépendante de l'ordre des unions. Le tri final par plus petit identifiant de racine fournit une forme canonique. Un groupe d'une seule racine reste explicitement présent : c'est une fermeture combinatoire d'arité un, pas un no-op.

Pour $E_q$ hyperarêtes, $H_q$ références, $R_q$ racines distinctes et $G_q$ groupes, la sortie plate contient $E_q+G_q+R_q\leq2E_q+H_q$ entrées et le scratch conservateur vaut au plus $3H_q+E_q$. Le noyau ne vérifie ni l'appartenance des racines au snapshot externe ni la complétude du lot géométrique.

Ce noyau n'est pas encore le quotient HGP générique. Il ne représente ni les facettes isolées latentes, ni les facettes de niveau égal, ni un bras unresolved; le miroir à quatre points avec $q_R=0$ est donc hors de sa portée. Ses dispositions one_root_class et multiple_root_class décrivent seulement le cardinal d'une classe de racines externes. Elles n'autorisent aucune RootBirth, AtomicUnionBatch, mutation de forêt ou classification publique.

## 5. Fixture frontière $K=10$

La fixture collinéaire minimale utilise les onze points $x_i=(i-5,0,0)$ pour $0\leq i\leq10$. La selle diamétrale d'ordre dix a $U=\lbrace0,10\rbrace$, $I=\lbrace1,\ldots,9\rbrace$ et $a=25$. Supprimer un endpoint donne un bras de niveau $81/4<25$; supprimer l'un des neuf points intérieurs conserve les deux endpoints et donne exactement le niveau 25.

Le test exerce donc la frontière maximale 2 bras stricts + 9 facettes égales, vérifie les onze suppressions sans matérialiser leurs ensembles dans le journal et rejoue toute la chaîne Phase 9, 10.1, 10.2 et 10.4 en moins d'une seconde sur la machine de développement.

## 6. Limite des gateways directs

Les suppressions intérieures émises ici appartiennent seulement aux événements directs présents dans le flux de Phase 9. Elles ne génèrent pas toutes les cofaces silencieuses non directes. Dans la fixture permanente à cinq points, les cofaces ACD et ACE au niveau $33/2$ sont non-Gabriel mais attachent silencieusement la facette AC; la selle directe future ABC la réutilise. Ces cofaces ne peuvent pas être recréées en parcourant uniquement les événements directs.

Par conséquent :

- direct_events_only == hgp_reduced est faux en général pour $k\geq2$;
- la génération complète des gateways reste une proof_obligation;
- M.1 reste une proof_obligation pour full_pi0;
- une recherche positive peut certifier une cible trouvée, mais une absence reste unresolved, jamais isolated;
- aucune naissance, continuation ou multifusion d'ordre supérieur ne doit être committée tant qu'un endpoint du lot ou la complétude des gateways manque.

## 7. Statuts des énoncés

| énoncé | statut |
|---|---|
| partition support/intérieur de toutes les suppressions | proved_here |
| égalité $\beta(S\setminus\lbrace x\rbrace)=\beta(S)$ pour $x\in I$ | proved_here |
| couverture locale de $S$ par les seuls bras stricts | proved_here |
| fermeture d'équivalence du noyau sur racines externes complètes | proved_here |
| journal factorisé 10.4 et rejeu streaming | validated_host_software |
| localisation positive par témoin strict rejouable | conditional_theorem |
| génération complète des gateways silencieux | proof_obligation |
| M.1 et exactitude full_pi0 | proof_obligation |
| exactitude du seul flux direct pour $k\geq2$ | false_in_general |

## 8. Prochain incrément

Le locator sparse positif 10.5a réalise maintenant la jointure `facet_key -> component_handle` relativement à des jetons d'autorité affirmés par l'appelant. La clé complète de dix identifiants au plus est comparée après toute recherche par empreinte; le handle suit les unions sans réécrire les clés, les requêtes restent pré-appel et un conflit exact post-unions est une contradiction permanente. Le noyau ne rejoue toutefois pas l'autorité externe et une absence reste `unresolved`. L'incrément suivant doit fournir une sonde `const` budgétée puis un saut top-k LBVH borné, reconstruire la miniball cible et n'accepter que $\beta(G)<\beta(F)$; il ne doit toujours créer aucun singleton sans certificat d'isolation ou autorité totale.
