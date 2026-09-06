# Plateaux FULL : quotient local, couverture et terminaison

6 septembre 2026, après le refus 50k publié par 638205bb. phase=exploration_v7_hors_registre, backend=cpu_reference, profile=quantized_u16_input_only, mode=audit_independant_math_and_architecture, public_status=not_claimed. Les parties I/II du manuscrit et la définition par composantes des régions témoins K-NN restent l’autorité visée.

**La garde actuelle ne peut pas être supprimée seule. Une extension exacte et locale est toutefois possible : quotient sur la coquille, raccord aux parents globaux avant le lot, naissances de plateau et deltas de couverture.** Les contre-fixtures sont permanentes. Elles ne sont pas les quatre enregistrements du run 50k, dont les coordonnées restent à extraire. Aucun moteur, compilation ou GCP utilisé par l’auditeur.

La [suite sur les ancres de boule](BALL_ANCHORS.md) raccorde ce quotient à un producteur complet borné : fenêtre amont conservée, une ancre par boule et ordre, résolution à la demande et partage vertical. Les preuves et captures initiales ci-dessous restent inchangées.

## 1. Deux changements réels du certificat hors régularité

Toutes les coordonnées suivantes sont u16, de troisième coordonnée zéro. Les niveaux donnés sont les rayons carrés exacts.

**Continuation qui gagne un point.** Prendre A=(1,8), B=(5,10), C=(9,8), Z=(5,0). À K3, ABC naît au niveau 16. Les trois autres triples et ABCZ naissent au niveau 25, sur la boule de centre (5,5). Ils se raccordent au seul parent ABC, dont la couverture gagne Z. Aucune nouvelle naissance ni multifusion ne permet d’encoder ce gain. Le certificat régulier « couverture = union des labels des feuilles » perd donc Z.

**Naissance qui regroupe plusieurs facettes.** Sur le carré (0,0),(2,0),(2,2),(0,2), les quatre triples et la coface des quatre points arrivent ensemble au niveau 2. À K3, une seule composante naît, couvrant quatre points ; une seule facette de cardinal K ne décrit pas sa couverture. À K2, la même boule fusionne quatre anciens parents, les côtés nés au niveau 1. Elle ne peut être ignorée parce que sa coquille compte quatre points au lieu de l’arité minimale deux.

Ces cas ne réfutent pas les [preuves régulières](../NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md). Ils imposent un contrat supplémentaire : une naissance peut porter une couverture de cardinal supérieur à K ; une continuation, voire une fusion, peut porter un delta de points. Un journal de composantes avec naissances, identifiants des parents et deltas suffit au rejeu des identités et couvertures de points par induction. Les rattachements sans nouveau point et les dates de toutes les facettes demandent une information supplémentaire. Il doit être qualifié comme **FULL non régulier** ; les anciens reçus F et leur champ born ne deviennent pas cette qualification.

## 2. Quel calcul local suffit ?

Soit une miniboule B de centre c, rayon r>0, avec I ses sites strictement intérieurs, U sa coquille complète et S=I∪U. Les positions sont distinctes et le census est complet. Noter p=|I| et u=|U| ; les blocs ci-dessous concernent 1≤K≤|S|. Pour K>|S|, le bloc est vide, pas une naissance. Pour tout sous-ensemble non vide F de S :

$$\rho(F)=r\quad\Longleftrightarrow\quad c\in\mathrm{conv}(F\cap U).$$

Si le centre est une combinaison convexe des points frontière, l’identité de variance impose un rayon au moins r à toute boule contenant F. Sinon, une séparation stricte fournit une direction qui rapproche simultanément le centre de tous ces points frontière. Un déplacement suffisamment petit conserve aussi les points intérieurs stricts ; F tient alors dans une boule de rayon inférieur à r. Si F∩U est vide, il est déjà strictement dans B.

À K≤|S|, toutes les K-facettes de S sont présentes à r et leur graphe de Johnson est connexe : deux voisines ont une union de cardinal K+1 contenue dans B. Le bloc fermé est donc un seul groupe. Son **graphe strict**, avant r, a pour sommets les F de cardinal K tels que c n’appartient pas à conv(F∩U), et pour arêtes les échanges dont l’union satisfait le même test strict.

### Absorber les intérieurs : une réduction supplémentaire

Pour K>p, poser t=K−p. Il suffit de construire le graphe sur les t-sous-ensembles A de U tels que c n’appartient pas à conv(A), en joignant deux voisins lorsque leur union reste stricte. Le représentant complet est I∪A.

**Preuve sur les composantes.** À partir de toute facette stricte F, insérer les intérieurs manquants en retirant des points frontière. Chaque union d’échange conserve la partie frontière stricte précédente. On rejoint I∪A, avec A⊆F∩U. Tous les choix possibles de A sont reliés par des échanges dans F∩U, qui reste strict. Pour une arête stricte F–G, tous ces choix sont de même reliés dans (F∪G)∩U. Réciproquement, une arête réduite devient une arête stricte après ajout de I. Ces applications donnent donc une bijection des composantes.

La réduction préserve aussi leur couverture : chaque point frontière x de F peut être inclus dans un choix A, puisque t≥1. Une composante réduite couvre exactement I plus l’union de ses A. Pour K≤p, le graphe strict est directement connexe et couvre S : rejoindre les K-sous-ensembles d’intérieurs et compléter chaque point frontière par K−1 intérieurs.

Sous le plafond de census **u≤12 actuellement déclaré**, on peut préparer une table de 2^u≤4096 masques de coquille. Identifier les supports positifs minimaux de cardinal au plus quatre, puis propager le prédicat « contient c » à leurs sur-ensembles, en O(u·2^u) opérations booléennes après les prédicats géométriques. La table sert à tous les K, aux adjacences et aux retraits essentiels. Un rang compte au plus binom(12,6)=924 sommets réduits. Chaque masque strict de taille t+1 connecte ses faces par une étoile de t unions, suffisante pour H0 ; aucune clique n’est nécessaire. Les intérieurs ne produisent pas le facteur combinatoire binom(p+u,K).

Ces bornes sont locales, sous plafond de coquille. Elles ne bornent ni le nombre de boules, ni les parcours pour retrouver leurs racines globales, ni un profil acceptant des coquilles arbitrairement grandes. Aucun gain de temps C++ n’est déduit. Le modèle exhaustif fourni ci-dessous utilise aussi les facettes complètes comme juge borné ; il ne propose pas ce stockage global au produit.

## 3. Classer l’effet et assembler les boules

Chaque composante stricte locale fournit un représentant à résoudre vers une racine globale **antérieure au lot**. Les composantes locales seules ne donnent pas le nombre de parents distincts.

Contre-fixture : boule de centre (2,0), rayon carré 4, coquille A=(0,0), B=(4,0), C=(2,2), aucun intérieur. À K2, elle fusionne AC et BC. Ajouter Z=(2,3), strictement hors de cette boule, ne change ni I ni U ; mais ACZ et BCZ, de niveau 13/4, relient déjà les deux facettes par CZ. Le même bloc local a maintenant un seul parent global. Une décision de fusion fondée seulement sur les coordonnées de I∪U serait fausse.

Assembler ensuite tous les blocs du même rayon par leurs racines globales distinctes. Une facette née exactement à r ne peut appartenir à deux boules différentes de rayon r : par unicité de sa MEB, ces deux boules seraient identiques. Les blocs distincts se raccordent donc uniquement par les anciennes racines. Ne pas publier des fusions intermédiaires produites par un traitement séquentiel du plateau.

Pour chaque groupe fermé : zéro parent donne une naissance, un parent une continuation, plusieurs parents une multifusion. Sa couverture est l’union des couvertures parentales et des S des blocs du groupe. Calculer explicitement la différence avec les couvertures parentales ; **un parent ne signifie pas nécessairement aucun effet public**. Les portails sans changement peuvent rester des ancres internes. Une même couverture de points ne fusionne jamais deux identités de composantes.

### Deux décisions locales moins coûteuses

Soit q_min la plus petite taille d’un sous-ensemble de U contenant c dans son enveloppe convexe. Si K≤p+q_min−2, le graphe strict est connexe et couvre S. Pour K>p, cela suit de la réduction : t≤q_min−2 et chaque union de voisins possède au plus q_min−1 points de coquille, donc reste stricte. Pour K≤p, utiliser le cas précédent. Le bloc n’introduit alors ni fusion ni gain de couverture ; ses rattachements peuvent toujours être nécessaires à un resolver. C’est un certificat suffisant d’inertie, pas un classement complet.

Noter h le plus grand cardinal d’un sous-ensemble de U dont l’enveloppe convexe ne contient pas c. Par séparation stricte, h est aussi le nombre maximal de points de coquille dans un hémisphère ouvert. Le plus grand sous-ensemble strict de S a cardinal p+h. Pour K≤|S|, le bloc n’a donc aucun parent local exactement lorsque **K>p+h** : il crée alors une seule composante, de couverture S.

Enfin, **|S|>Kmax+1 n’autorise aucune omission**. À Kmax=5, les sept points (10,5),(0,5),(5,10),(5,0),(8,9),(2,1),(9,8) sont sur le cercle de centre (5,5), rayon 5. Trois paires sont antipodales. Tout choix de cinq points contient une telle paire : avant 25, aucune facette K5 ; à 25, les 21 facettes et leurs cofaces forment une naissance couvrant sept points, bien que 7>6. Le test de fenêtre doit tenir compte des rangs utiles, pas remplacer q_min par u.

## 4. Portails : une progression qui accepte les supports alternatifs

Un sommet d’un support positif choisi n’est pas forcément essentiel à l’ensemble entier. Sur un carré, supprimer l’extrémité d’un diamètre laisse l’autre diamètre : la MEB ne décroît pas. L’ensemble des sommets essentiels est l’intersection des supports minimaux possibles.

Une chaîne peut néanmoins garder l’opération simple « retirer un sommet du support choisi et ajouter un intrus strict ». Pour une coface Q de cardinal K+1, Q′ reste dans sa boule, donc le rayon ne croît pas. Si le rayon reste égal, unicité implique la même boule, et **le nombre de sommets sélectionnés sur cette coquille diminue exactement de un**. Le rayon, puis ce cardinal de coquille, donnent une progression lexicographique stricte dans un ensemble fini. À rayon positif fixé, au plus K−1 échanges sont possibles, puisque deux points frontière au moins sont nécessaires.

Q et Q′ partagent une K-facette et leurs MEB ne dépassent pas le rayon initial : la composante est conservée. Pour un appel strictement avant un lot supérieur, toute cette chaîne reste strictement avant ce lot. Le terminal sans intrus strict est un label Gabriel faible I∪T ; il faut son ancre **après fermeture de son propre plateau**.

Le modèle conserve F={(0,2),(2,4),(4,2),(2,0)} et les intrus (2,2),(2,1). Après extension de F par le premier intrus, un échange conserve le rayon carré 4 et diminue la coquille sélectionnée de quatre à trois ; il atteint ici le terminal. La condition de décroissance stricte actuelle le refuse. Ce lemme ne permet pas de changer seulement >= en > dans le moteur : ses gardes de coquilles et son autorité de terminal doivent aussi être étendues.

## 5. Raccords C++ et certificat à qualifier

Le [producteur actuel](../../src/forest/full_gabriel.hpp) impose q≤4 et tous les retraits actifs, résout ces retraits comme strictement antérieurs et suppose qu’aucune facette au niveau ne relie deux directes. [silent_incidence.hpp](../../src/forest/silent_incidence.hpp) refuse aussi les coquilles locales supplémentaires et les points étrangers sur le bord. Les lots existants ne lèvent donc pas ces hypothèses. Le compte régulier « une BallData = un événement » de la sonde devra également être versionné.

[plateau.hpp](../../src/forest/plateau.hpp) fournit déjà le bon test de retrait par enveloppe convexe fermée et les labels Gabriel faibles I∪T. Son expansion bornée n’est pas une qualification de FULL non régulier. Le quotient de coquille décrit ici évite de matérialiser toutes les facettes globales et partage ses prédicats entre ordres ; il reste à raccorder aux racines, aux refus et aux compteurs.

Pour la vraie tour, une naissance de plateau supérieure s’ancre dans la composante inférieure **fermée au même niveau**, éventuellement sans coface unique distinguée. La naturalité vient toujours des inclusions des régions témoins. K=n reste une naissance de X à sa MEB, sans ordre supérieur ; le rayon zéro et les doublons gardent leur contrat distinct. Les poids sur toutes les facettes et leurs dates d’affectation ne sont pas remplacés par ces seules couvertures.

Avant toute nouvelle mesure utile, extraire des quatre/trois enregistrements 50k : BallKey, niveau, arité minimale, I/U complets avec identifiants et coordonnées, puis les rangs concernés. Une petite ré-exécution du sous-problème suffit pour juger la géométrie locale ; la qualification des **parents globaux** doit conserver leur contexte extérieur. Aucune des fixtures ci-dessus n’est présentée comme cette extraction réelle.

## Vérifications indépendantes

[plateau_model.py](plateau_model.py) utilise le solveur Gram rationnel historique de l’auditeur, jamais un helper produit. Sur six nuages de trois à sept points : **28 ordres, 352 coupes ouvertes/fermées**, avec comparaison des partitions exactes de facettes, pas seulement des couvertures ; 134 quotients de coquille, 30 blocs localement inertes, 60 diagnostics de naissance, 214 images verticales et 192 carrés de naturalité. Le carré vérifie aussi la fusion atomique de quatre boules distinctes au même niveau. Le modèle conserve les membres complets pour cette comparaison ; ses events donnent des diagnostics avec nombres de parents, pas un certificat exporté puis rejoué.

Python normal et -O rendent chacun 0 et des [octets](normal.json) [identiques](optimized.json). Les cas fixés réfutent l’omission des coquilles supplémentaires, la naissance représentée par une seule K-facette, l’absence de deltas, le classement par seuls parents locaux, le rejet fondé sur |S|>Kmax+1, l’assimilation support choisi/sommets essentiels et la descente strictement décroissante. Les [sources lues](source_review.json) bornent la portée ; aucune qualification C++ D–O modifiée, aucune mesure de gain ni tour industrielle revendiquée.
