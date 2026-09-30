# Projection persistante : balayage chronologique sans antichaîne

Paquet autonome de preuve algorithmique **abstraite**, 30 septembre 2026.
Ce n'est ni un oracle géométrique Gamma, ni un nouveau générateur/export natif,
ni une qualification statistique, FULL, croissance globale ou G4.

## Lemme et equivalence

Fixer un arbre enraciné de fusions, dont les naissances sont croissantes vers
la racine. Pour un point x, chaque témoin w porte une date c(w) et un nœud
v(w) vivant à cette date : b(v)<=c(w)<d(v). La coupe est fermée ; toutes les
fusions d'un même niveau sont déjà réalisées. W doit certifier exactement
les composantes couvrantes : Cov_x(r)={anc_r(v(w)):c(w)<=r}.
Les témoins d'ancêtres redondants et les incidences dupliquées sont permis.

Pour W_r non vide, J(r)=LCA{v(w):c(w)<=r}. Si b(J)>r, remonter chaque
nœud jusqu'à sa composante vivante à r ne change pas leur LCA : celui-ci
est strictement au-dessus de la coupe. Si b(J)<=r, tous les nœuds sont
déjà dans la même composante à r. Le premier instant s>=r où toutes les
composantes couvertes en r ont fusionné est donc

    M_x(r)=max(r,b(J(r))).

Une antichaîne minimale produit exactement les mêmes ensembles Cov_x(r) :
un témoin d'ancêtre u est né à c>=b(u), tandis qu'un témoin descendant v
vit à c(v)<d(v)<=b(u). Lorsque u est activé, v l'est déjà et leurs
composantes à c sont identiques. Retirer ces ancêtres ne change donc M.
Le LCA brut peut cependant monter jusqu'à un ancêtre déjà vivant : son
nouveau niveau est <=r, et le max(r,...) absorbe précisément cette montée.
Ne pas réutiliser cette conclusion pour les bandes non saturées.

Pour kappa>=1, alpha=min c(w), définir

    t=max(alpha, sup_{r>=alpha}[M_x(r)-kappa(r-alpha)]).

Entre deux dates témoins, J reste fixe. Les deux termes
r-kappa(r-alpha) et b(J)-kappa(r-alpha) sont non croissants ; le premier
est toujours <=alpha. Le supremum se prend donc aux dates témoins :

    t=max(alpha, max_c[b(J(c))-kappa(c-alpha)]).

Un pliage de TOUS les témoins par dates croissantes calcule ainsi le même
P_kappa que l'antichaîne, sans tri Euler ni code-barres par point.
Le propriétaire reste anc_t(J_alpha), avec J_alpha plié après **tout**
le plateau alpha. J_final est interdit : il peut être encore non né à t.

## Hypotheses pour un futur raccord geometrique

K est fixe ; population fermée>=K et p+q_min<=K ; incidences complètes I union U,
y compris entrées internes et contacts dégénérés ; ball_node vivant à sa
propre date fermée. Un catalogue limité aux seules populations K ne suffit
pas. L'exhaustivité géométrique de ces témoins est une hypothèse externe
au paquet, pas une conséquence des arbres synthétiques.
Les plateaux exacts sont atomiques. Les arbres du test ont des naissances
parentales strictes, comme une représentation contractée des plateaux.
Toute représentation avec vie nulle doit d'abord résoudre les nœuds à la
coupe fermée canonique. Un arbre incomplet, une incidence absente ou une
date non ordonnée ne permet aucun transfert de cette preuve.
Toutes les formules sont en **rayon**. Un catalogue natif trié par beta=r^2
fournit le même ordre, mais t est généralement une somme algébrique de
racines, non un niveau rationnel déjà présent dans le catalogue.

## Cutoff conditionnel et cout

Si M_x(r)<=r+rho/2 pour tout r>=alpha, alors, pour kappa>1,

    c>=alpha+rho/[2(kappa-1)] => M_x(c)-kappa(c-alpha)<=alpha.

Garder c<=ce seuil suffit donc exactement. Dans la géométrie HGP,
rho=d_K(x) et rho<=2alpha donnent aussi le seuil conservateur
c<=kappa/(kappa-1)*alpha. Le test vérifie explicitement la borne de
résolution pour son rho SYNTHETIQUE ; il ne qualifie pas un calcul k-NN.
Le seuil fin demande une comparaison exacte de racines ; son majorant
grossier au carré est rationnel pour kappa rationnel.

Le balayage fait O(D*cout_LCA), puis une requête d'ancêtre par point,
avec O(n) états hors index LCA et stockage/transport des incidences.
Le tri global des niveaux est supposé déjà payé par FULL/catalogue :
si le flux arrive en ordre de générateur non trié, il faut payer le tri
ou un rangement stable adéquat. « Sans tri par point » ne signifie pas
« sans aucun tri » ni O(D) sans hypothèse sur l'index LCA. Sur GPU, les
préfixes/date-cohortes doivent être cohérents ; les écritures concurrentes
non ordonnées et l'usage prématuré d'un plateau partiel sont exclus.
D lui-même et le coût de construction de l'univers fort restent à mesurer.

## Capture et reproduction

256 cas / huit formes d'arbre, 1 024 profils, 5 460 incidences ; unaires,
entrées internes, fusions N-aires, ex aequo et doublons. Deux algorithmes
LCA distincts ; l'oracle de résolution énumère les futures coupes et
compare les composantes, sans utiliser max(r,b(J)).
4096 réponses, 3072 cutoffs, 5915 résolutions, 16230 comparaisons de
couverture, 26248 partitions et 6144 hauteurs de paires passent.
Le mutant ordre inversé non validé produit (alpha,t,owner)=(18,50,6)
au lieu de (10,10,0) ; le lecteur gardé le refuse. J_final donne un
propriétaire non né. Les sorties normal/−O sont byte-identiques.

Les pins code/protocole ont été enregistrés avant les deux premières
invocations. normal.output.txt/optimized.output.txt sont les sorties
combinées fournies par exec_command, chacune une ligne JSON, code 0.
Aucun chronomètre de performance n'est revendiqué.

    python3 -B verify.py
    python3 -B -O verify.py

Le lecteur portable vérifie les hashes avant toute exécution de check.py,
rejoue normal et −O et exige les sorties exactes, puis reverifie les hashes.
Aucune dépendance privée, compilation, géométrie native ou GCP.
