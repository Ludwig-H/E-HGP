# Sommets peu profonds : conserver les dimensions zéro et un

Audit indépendant après31b0243a, en réponse à la question constructeur29.
Les preuves ci-dessous sont élémentaires et indépendantes ; aucun résultat de
complexité en position générale n'est transféré aux données dégénérées.
Aucun port produit ni gain de performance n'est revendiqué.

## Une boule positive isolée, sans région ouverte peu profonde

Prendre a=(30,30,30), b=(36,36,30), x=(30,36,24), y=(36,30,24), puis
u=(30,36,30), v=(36,30,30), r=(30,30,24), s=(32,32,32), dans cet ordre d'IDs.
Le support abxy est un tétraèdre régulier : toutes ses arêtes ont longueur
carrée72, centre(33,33,27), rayon carré27 et poids barycentriques1/4.
L'arête ab est propriétaire par départage des IDs. Les huit sites distincts
u16 sont sur la même coquille ; aucun n'est strictement intérieur.

La base entière du plan des centres est A=(−6,6,0), B=(0,0,6).
Le centre de cette boule est (ξ,η)=(0,−1). Les quatre témoins supplémentaires ont les formes suivantes.

$$L_u=-144\xi,\quad L_v=144\xi,\quad L_r=144(1+\eta),\quad L_s=-48(1+\eta).$$

Pour K3, T=1. Les inégalités L_u,L_v≥0 imposent ξ=0 ; L_r,L_s≥0 imposent
η=−1. La profondeur strictement inférieure àT se réduit donc à ce point.
Les deux formes de seed sont 144(1−ξ+η) et144(1+ξ+η), non parallèles.
Le sommet accepté est réel, positif et propriétaire, mais aucune région
ouverte peu profonde ne l'entoure. Explorer seulement de telles régions,
ou jeter les intersections de dimension0/1, n'est donc pas complet.

Une opposition de signes n'annule pas deux populations. Pour une droite F=0
avec poids w₊ et w₋, la contribution est w₊[F<0]+w₋[F>0]. Sur la droite,
elle est zéro et tous les IDs sont sur la coquille. Remplacer ces deux poids
par leur différence détruit exactement le creux de profondeur sur la droite.
Regrouper une orientation répétée exige également son poids entier, pas un
seul témoin. Plusieurs droites différentes peuvent ensuite concourir au même
centre ; leur groupe de coquille doit être reconstruit dans son ensemble.

## Un énumérateur exact par régions fermées, avec limite explicite

Voici une référence constructive pour contrôler une future méthode rapide.
Elle ne construit pas l'arrangement complet, mais n'a pas de borne de travail
utile établie pour les grands nuages au seuil général.

Travailler dans un rectangle fermé C contenant tous les centres recherchés.
Séparer les formes constantes : négatives dans un compte fixe, nulles dans
la coquille commune. Regrouper les formes non constantes proportionnelles
de même signe par triplet primitif orienté, avec poids égal au nombre d'IDs.
Garder les orientations opposées dans deux groupes distincts. Après retrait
du compte fixe, soit k le budget d'intérieurs, donc k=T−1−compte_fixe.

Un état R est un ensemble de groupes retirés, de coût w(R)≤k. Sa région est
l'intersection fermée suivante, conservée dans toutes ses dimensions.

$$P_R=C\cap\bigcap_{g\notin R}\{t:F_g(t)\geq0\}.$$

Si P_R n'est pas vide, ses points ont au plus w(R) intérieurs parmi les groupes.
Émettre ses sommets portant au moins deux droites de sites non parallèles ;
les sommets dus uniquement au bord artificiel de C ne sont pas des événements
q4. Évaluer profondeur et coquille sur tous les groupes, retirés compris.
Puis créer les enfants retirant un groupe restant qui touche P_R, si son poids
respecte le budget. Une contrainte touche P_R lorsque sa valeur est nulle en
au moins un sommet de ce polygone, segment ou point fermé.

Si P_R est vide, brancher sur les groupes d'un certificat exact d'infaisabilité.
Tous les groupes restants forment toujours un tel certificat ; un certificat
plus petit serait une optimisation à construire et à payer. Ne jamais retirer
les contraintes de C. Dédupliquer les états par leur ensemble R canonique.

**Complétude.** Fixer un sommet cible t∈C de profondeur≤k. On peut choisir un
chemin ne retirant que les groupes strictement négatifs en t. Si P_R est vide,
t viole au moins un groupe de tout certificat d'infaisabilité. Si P_R est non
vide mais ne contient pas t, prendre p∈P_R : le premier point de sortie sur
le segment p→t appartient à P_R et annule une contrainte violée par t.
C étant convexe et contenant p,t, cette contrainte est un groupe amovible.
Elle touche donc P_R et fait partie des branches proposées. Chaque pas retire
un véritable intérieur de t ; le chemin respecte le budget et finit.

Dès que t∈P_R, les deux formes non parallèles qui définissent t n'ont pas été
retirées, puisqu'elles valent zéro en t. Si t était une combinaison convexe
non triviale de deux points de P_R, chacune de ces deux formes, non négative
sur P_R, s'annulerait aux deux points ; leur intersection unique imposerait
qu'ils valent t. Donc t est un sommet de P_R, même si P_R est de dimension0/1.
Cette preuve conserve les racines multiples et ne perturbe aucune égalité.

**Limite.** Pour g groupes de poids au moins1, le nombre d'états peut atteindre
le nombre de sous-ensembles de taille au plus k : Σ_{j=0}^{min(k,g)} binom(g,j).
La famille abstraite F_i=η−2iξ+i² l'exerce : chaque contrainte restante touche
toujours la région au point(i,i²), où F_j=(i−j)²≥0, quel que soit R.
Choisir C contenant tous ces points ; toutes les branches de retraits restent
donc proposées. Le gate vérifie les37 états pour g=8,k=2. Cette explosion
concerne l'algorithme de référence, pas une borne inférieure de tout algorithme.
Ce n'est pas un moteur recommandé sans étude supplémentaire pour T=8 et
des milliers de groupes. L'intersection des demi-plans, ses intersections
intermédiaires, les états, les coquilles et leur déduplication ont tous un coût.
Un budget d'exploration peut déclencher le repli exact ; il ne peut tronquer
les états restants. Pour k=0, une seule intersection fermée suffit : ce cas
donne déjà une voie exacte intéressante pour les feuilles de budget nul.

La sortie géométrique doit ensuite passer positivité, propriété et canonisation
du producteur q4. Un catalogue de centres peut partager profondeur et coquille,
mais ne dispense pas de préserver les présentations/supports exigés en aval.
Une borne sur les sommets seuls ne borne pas ce payload ni la somme par arête.

[degeneracy_gate.py](degeneracy_gate.py) calcule le centre et les poids par
élimination rationnelle, énumère tous les supports abij de la fixture réelle,
contrôle les puissances et confronte l'énumérateur fermé à un oracle de toutes
les paires de droites. Les cas aléatoires sont des formes affines abstraites,
pas des nuages u16 qualifiés. Aucun code produit n'est importé. GCP non utilisé.
