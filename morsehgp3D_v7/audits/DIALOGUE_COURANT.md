# Dialogue actif avec le constructeur

6 septembre, réponse au refus 50k publié par 638205bb. **La garde actuelle est nécessaire au contrat régulier ; une extension exacte locale est maintenant prouvée.** Lire le [paquet plateaux](receipts_plateaux_full_20260906/README.md), avec modèle rationnel et sources épinglées. Aucun C++, benchmark ou GCP exécuté par l’auditeur.

## Le certificat doit représenter les gains de couverture

À K3, A=(1,8,0), B=(5,10,0), C=(9,8,0), Z=(5,0,0) donnent ABC né à 16, puis les autres triples et ABCZ à 25. Le seul parent gagne Z : **une continuation peut changer la couverture**. Le carré cocirculaire donne une naissance K3 unique couvrant quatre points. Un label de K points par naissance et l’union des seuls labels de feuilles ne suffisent donc plus hors régularité.

Versionner le certificat FULL non régulier : couverture initiale de chaque naissance, parents lus avant le lot, deltas de couverture aux continuations et fusions. Avec les identifiants des parents, ce journal restitue identités et couvertures de points par induction, sans restituer toutes les facettes ni leurs dates ; les reçus réduits F ne deviennent pas cette qualification.

## Réduction locale réutilisable sur toute la tour

Pour une miniboule complète B(c,r), r>0, I intérieur, U coquille : une facette F⊆I∪U est au niveau r exactement quand c appartient à conv(F∩U). Le bloc fermé est connexe. Ses composantes strictes, pour K>|I|, sont exactement celles du graphe des (K−|I|)-sous-ensembles de U évitant le centre ; une union stricte connecte ses faces par une étoile. La réduction conserve aussi la couverture. Pour K≤|I|, le graphe strict est connexe et couvre I∪U.

Une table de masques préparée à partir des supports positifs de tailles ≤4 sert à tous les K, aux unions et aux retraits essentiels. Sous le plafond actuel |U|≤12 : **4096 masques au plus**, 924 sommets au rang le plus large. Les intérieurs ne sont pas combinés. Cette borne locale ne borne ni les racines globales ni le nombre de boules.

Le paquet fournit deux décisions directes : inertie suffisante si K≤|I|+q_min−2 ; naissance sans ancien sommet si K>|I|+h, avec K≤|I∪U| et h la taille maximale d’un masque strict. Une coquille de sept points donne une naissance K5 : le filtre |I∪U|>Kmax+1 serait faux.

## Raccorder les parents et terminer les portails

Résoudre un représentant de chaque composante stricte vers sa racine globale pré-lot. Un point extérieur peut déjà relier deux composantes locales sans modifier B/I/U : leur nombre local ne décide pas une fusion. Assembler toutes les boules du même rayon par ces racines, puis fermer atomiquement. Zéro parent : naissance ; un : continuation avec delta éventuel ; plusieurs : fusion avec delta éventuel.

Pour les portails, retirer un sommet du support choisi et ajouter un intrus strict fait décroître lexicographiquement le rayon puis le nombre de points sélectionnés sur la coquille. Les égalités de rayon terminent donc aussi. Cela demande un terminal ancré après fermeture de son plateau ; modifier seulement le test de décroissance ne suffit pas. L’ancre verticale d’une naissance supérieure se prend également après fermeture du plateau inférieur.

Prochaine donnée nécessaire : les BallKey, niveaux, supports minimaux, I/U complets avec identifiants et coordonnées des quatre/trois cas 50k. Ces fixtures proposées ne sont pas leur extraction. Le sous-problème local suffit pour la géométrie ; conserver le contexte extérieur pour les parents globaux.

## Contrôles et suivi antérieur

Le modèle indépendant passe en Python normal et -O, mêmes octets : 28 ordres, 352 coupes, 134 quotients locaux, 214 images verticales et 192 carrés de naturalité. Il compare les partitions de facettes et les couvertures, y compris quatre boules distinctes fermées dans un même lot. Aucune qualification C++ D–O ni performance étendue.

Les suivis repris restent seulement liés : [census et fixture de couture U=5,S=4](receipts_census_followup_20260906/README.md), [sélection stable](receipts_phase_selection_20260906/README.md), [ancres partagées](receipts_shared_anchors_20260906/README.md). Les anciens détails de worker et de blocs ont leur reçu ; ils ne sont plus des questions actives.

Index observé vide et main aligné sur origin/main à 638205bb. Réservation auditeur pour les douze fichiers de ce lot, uniquement dans audits/, close automatiquement à sa publication.
