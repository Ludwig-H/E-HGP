# Boules critiques et géométrie d'une composante — Q5

Question figée `fd85f3bb582616324e9d4034b84ca96d742a439a`. Preuves indépendantes en `Fraction`, quatre petits
témoins 3D plans. Aucun lancement du moteur, compilation, CTest ou GCP. Les critères de la fenêtre sont ceux de
`docs/MATHEMATIQUES.md:82,462`; le modèle énumère complètement les supports positifs de ces nuages de deux ou trois
sites (diamètres, puis triangle strictement aigu). Il ne qualifie aucune implantation.

Rejeu portable, sans modifier le JSON figé :

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```

Sortie des deux replays : `critical_ball_geometry_verdict conforme cases4 Fraction native0 cloud0`.

## Ce qui est relié, ce qui ne l'est pas

Noter `C_v(r)` la composante de `Omega_k(r)` correspondant au nœud vivant v. Pour les boules de fenêtre b de son
sous-arbre, datées `r_b <= r`, le centre `z_b` appartient à `C_v(r)`. Donc, avec des boules fermées :

`union_b B(z_b,r_b) ⊆ union_b B(z_b,r) ⊆ C_v(r) ⊕ B(0,r)`.

Cela utilise la date et la fenêtre : une boule quelconque du catalogue, ou une boule future du nœud, n'a pas ce
certificat. À k=1, une feuille sans boule publiée n'en fournit pas non plus. L'inclusion ne donne ni égalité ni
équivalence d'homotopie avec l'ensemble de référence.

Le lemme 6.1 de [BCY, version des auteurs, §6.1.4 p.141](https://geometrica.saclay.inria.fr/team/Fred.Chazal/papers/CGLcourseNotes/main.pdf)
identifie le type d'homotopie de l'union **des boules fournies** avec leur complexe alpha pondéré au paramètre zéro.
Ici les poids `w_b=r_b²` donnent `min_b(||y-z_b||²-w_b) <= 0`, donc l'union critique fixe. Au paramètre alpha s,
les rayons deviennent `sqrt(r_b²+s)`. Ni s=r² ni le remplacement des poids par r² ne reconstruit en général
`Omega_k(r) ⊕ B_r`. Le complexe alpha de ces points pondérés est un modèle de cette union, pas une complétion du
catalogue HGP. Les barycentres des k-parties et leurs poids **moins la variance** constituent une autre construction.

## Quatre témoins exacts

1. **Croissance après l'unique naissance** : X={0,2} sur l'axe, k=2. La seule boule éligible du nœud est centrée en1,
   de rayon1. À r=2, 2 appartient à `Omega_2(2)` et 4 appartient à son offset par B2. Mais 4 est hors de B(1,1) et
   même de B(1,2). Les deux unions proposées ne sont donc pas l'objet demandé.
2. **Même boule de naissance, géométrie différente** : k=n=3, `X_A={(0,1,0),(1,1,0),(2,1,0)}` et
   `X_B={(0,1,0),(1,2,0),(2,1,0)}` ont la même unique boule de fenêtre W3 : centre(1,1,0), rayon1, support diamétral.
   À r=2, q=(1,-1/2,0) est dans Omega_A, et y=(1,-5/2,0) est dans son offset. Mais l'offset de Omega_B est contenu
   dans B((1,2,0),4), tandis que la distance carrée de y à ce site vaut81/4>16. L'identité concerne les boules du
   **nœud d'ordre3**, pas toute la tour FULL aux ordres1..3, ni leurs populations p/m.
3. **Les offsets fusionnent des nœuds** : X={0,2,4}, k=2,r=1. Omega est constitué des deux points1 et3, donc deux
   nœuds vivants. Les offsets B(1,1) et B(3,1) se touchent en2 : leur union est connexe. Conserver les labels de
   nœud avant la dilatation ; ses composantes globales ne redonnent pas les nœuds de T_k.
4. **H0 ne reconstruit pas le trou de Omega** : A=(0,0,0),B=(2,0,0),C=(1,2,0), k=1,r²=5/4. Les trois boules
   originales ont toutes des intersections deux à deux, mais aucune intersection triple, car le rayon carré de
   la MEB est25/16. Le nerf est un cycle, donc H1 de Omega a rang1. Les trois boules diamétrales critiques du nœud
   contiennent toutes (1,3/4,0), donc leur union est étoilée et contractile ; toute sélection de deux aussi.
   Ce témoin compare à **Omega**, pas à son offset : la dilatation peut elle-même combler le trou.

## Construction fidèle disponible en théorie

À une coupe donnée, `Omega_k(r)=union_{|Q|=k} W_Q(r)` avec `W_Q(r)=intersection_{x in Q} B(x,r)`. Cette identité
est déjà le contrat FULL (`docs/MATHEMATIQUES.md:137–157`). Pour un modèle plus petit, utiliser
`C_Q(r)=Vor_k(Q) ∩ W_Q(r)`. Les régions non vides sont convexes et couvrent exactement Omega. Leur nerf est
homotope à Omega, avec raccord de filtration en r. C'est la construction du [multicover, §2.3](https://link.springer.com/article/10.1007/s00454-022-00476-8),
qui distingue explicitement le nerf simplicial du dual polyédrique de la mosaïque.

Pour l'objet dilaté, l'identité exacte est `Omega ⊕ B_r = union_Q (C_Q(r) ⊕ B_r)` : ces pièces restent convexes.
Les rattacher d'abord aux nœuds FULL via leur k-partie, puis garder cette provenance malgré les intersections après
dilatation. Le contrat de `descent.hpp:95–99` donne la graine de la partie à partir de sa date initiale ;
`ancestor_index.hpp:34–56` permet de remonter à la coupe. Il s'agit d'un raccord à implanter et qualifier.

La frontière exacte de Omega porte des morceaux de sphères originales de rayon r et leurs strates d'intersection,
pas des faces de `conv(S*)`. Un maillage polygonal est une approximation à déclarer. Pour une première géométrie
bornée : construire les régions restreintes sur petits nuages, comparer composantes au FULL, puis extraire cette
frontière avec une erreur géométrique explicite. Au rayon critique, conserver aussi les composantes ponctuelles ou
filaires ; une extraction du seul volume ouvert les perdrait. Ne pas déduire de l'homotopie un certificat de
proximité de la frontière.

Alternative certifiée pour l'offset : un ensemble fini E_v contenu dans C_v(r), formant un epsilon-net, donne
`E_v ⊕ B_r ⊆ C_v(r) ⊕ B_r ⊆ E_v ⊕ B_{r+epsilon}`. Les centres critiques ne sont pas automatiquement un tel net.
Cette condition permet une approximation contrôlée avec des boules à rayon commun ; son coût reste à mesurer.
