# Réponses indépendantes aux cinq verrous du moteur

Rédaction initiale 2026-10-02 08:04:43 UTC ; suivi 2026-10-02 12:28 UTC.
Questions publiées à `93ba16112` ; L02 privé figé dans
[le reçu](../receipts/audit_independant_20261002/math_locks_review/README.md).
Contre-lecture des définitions, pas qualification du moteur v11.
Les [cinq réponses sont adoptées par le développeur](REPONSE_CLAUDE_VERROUS_MOTEUR_20261002.md)
à `986f75799`. Les portes de référence et de projection sont désormais jouées
sur G4 à `a97180667`, publiées à `6a22a9118` ; le moteur natif reste à livrer.

## Q1. Morceaux, raffinement et descente

B cas 4 tient avec **surjection locale→globale**. Les morceaux sont les
composantes du graphe strict induit sur P_b ; deux morceaux peuvent déjà
être reliés hors de cette population. Leur image couvre les composantes
rencontrées, sans bijection. Un représentant par morceau suffit ; dédupliquer
les racines globales avant Kruskal. Exemple : X={(0,0),(2,0),(4,0),(2,3)},
b de centre (2,0), β=4, K2. Deux morceaux locaux, reliés extérieurement par
les triangles de β=13/4<4 via la paire milieu–haut.

Preuve locale : compresser F∈V_< en I∪A par échanges ajoutant des intérieurs.
Une arête stricte F–G a une trace séparable sur U, donc les A,A′ extraits ont
une union séparable. Réciproquement, une union séparable fournit le chemin
Johnson strict. L'inclusion de ce graphe local dans Γ global donne la surjection.

Le corollaire de raffinement est correct pour une **partition exhaustive**
de chaque morceau, un représentant par sous-bloc non vide. « Chaque représentant
appartient à V_< » seul ne suffit pas : il faut la couverture de H4. Un
sous-échantillonnage de représentants ne bénéficie pas de ce corollaire.

D admet toute k-partie de I : elle tient dans une boule concentrique strictement
plus petite. I∪A séparable décroît aussi par le lemme 1 ; Johnson assure le lien
à β(F). Terminaison par l'ensemble fini des k-parties, sans borne pratique
sur le nombre de pas. La borne combinatoire n'est pas un contrat de performance.

Le **terminal peut dépendre du choix** : sur X={0,2,4}, K2, les extrêmes β=4
peuvent descendre vers {0,2} ou {2,4}, deux naissances β=1. Seule leur classe
est unique aux coupes a≥β(F). Le corollaire D est bien formulé ; limiter
« fonction pure » à une politique déterministe fixée. Le mémo cellule (b,k)
peut fournir un terminal représentant sa classe à a≥λ_b ; il ne peut identifier
ses morceaux distincts avant λ_b. À coupe ouverte, exiger a>λ_b ; le plateau
fermé ne peut être anticipé. [Contrelecture concordante](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

## Q2. Signe d'un polynôme avec annulations

Employer u=2^-52 pour les quatre arrondis. Pour +,−,× sur entrées et coefficients
entiers exacts, développer **l'expression évaluée avant annulations**. Poser
M=Σ|termes développés|, majoré sur le domaine certifié. Chaque terme reçoit
un produit de facteurs d'arrondi ; l'exposition E maximale donne
|v_approché−v|≤γ M, γ=(1−u)^(-E)−1, sous résultats finis, sans sous-flux.

Récurrences suffisantes : E(feuille exacte)=0 ; E(a±b)=max(Ea,Eb)+1 ;
E(ab)=Ea+Eb+1 ; E(fma(a,b,c))=max(Ea+Eb,Ec)+1. Réutilisation comprise :
t=fl(ab), fl(t·t) expose trois arrondis, pas une profondeur de deux.
Borner E sur tous les ordres autorisés ; le cas décontracté majore la FMA.
M peut se construire par M(a±b)=Ma+Mb, M(ab)=Ma·Mb, sans simplifier les
annulations. Coefficients/conversions non exacts doivent être inclus dans E.

Le seuil doit lui-même être une **majoration certifiée**. Si Eu≤1/2,
γ≤Eu/(1−Eu)≤2Eu. Avec M≤2^q et E≤2^e, τ=2^(q+e−51) convient, exactement
représentable si son domaine exponentiel est protégé. Accepter seulement
|v_approché|>τ ; sinon exact, notamment à l'égalité. Aucun gamma/M arrondi
vers le bas ni auto-test ne remplace ces preuves.

« Degré≤3, coordonnées quinze bits » ne suffit pas en général : coefficients,
nombre de termes et toutes sommes/intermédiaires comptent. x=32767,
A=512x³ : (A+1)−A−1 vaut 0 exactement et −1 en binary64 nearest.
L'orientation standard à six monômes ±1 est en revanche exacte si toutes ses
différences sont réellement de quinze bits : 6·2^45<2^48. Certifier chaque
opérande, y compris supports/témoins extérieurs à la feuille ; une largeur
mesurée de feuille ne leur transmet pas sa borne.

Portes indispensables : zéro et signes ±1 près d'un grand permanent,
réutilisations/carrés, permutations/parenthésages, quatre arrondis, FMA on/off,
limites exactes du domaine local et repli déclenché. Ces portes portent sur
les expressions réelles ; notre petit témoin ne qualifie aucun prédicat C++.

## Q3. Euler accompagne la preuve de complétude ; il ne la remplace pas

Faire d'Euler une porte d'échelle indépendante est utile. **Ne pas appeler
« catalogue certifié » son seul succès**, ni transférer les 9,6 % détectés de
L02 à une garantie universelle. χ=β0−β1+β2 ne détermine ni la partition ni
même β0 ; les contributions omises peuvent se compenser au même plateau.

Témoin u18 explicite : X={(0,5),(8,9),(8,1),(35,5),(45,5)}, K1. À β=25,
le triangle de centre (5,5) remplit un trou (+1 Euler) et la paire de centre
(40,5) fusionne deux composantes (−1). Omettre les deux sphères conserve toute
la courbe d'Euler, tout en retardant cette fusion : trois composantes au lieu
de deux. Les 19 coupes exactes et poids sont vérifiés ; aucun catalogue produit
amputé dans ce reçu. Le catalogue élargi pour Euler diffère bien du catalogue
minimal de π0 : la sphère q3 n'est pas nécessaire à la forêt K1.

Port produit peu coûteux : refuser une MEB rencontrée absente **dans la fenêtre
où sa présence est requise**, plutôt que poursuivre silencieusement ; cela ne
certifie pas une sphère jamais rencontrée. Ordre 1 contre EMST, census ciblé,
neutralité des descentes et déterminisme complètent les diagnostics sans les
transformer en preuve de complétude générale. Celle-ci reste le théorème de
couverture du générateur et de ses élagages. Euler peut être un mode de diagnostic
nommé/payé séparément ; aucun besoin d'en faire le chemin rapide par défaut.

## Q4. Ensemble admissible d'abord ; singleton intrinsèque impossible parfois

Publier l'ensemble cover, à la coupe fermée, et séparer sa convention de sortie
est le bon contrat. Il n'existe pas de départage singleton **équivariant sous
toutes les isométries** sur toute géométrie : X={0,2,4}, K2, β=1 ; la réflexion
x→4−x fixe le point médian et échange ses deux composantes admissibles.
Une fonction équivariante ne peut en choisir une seule.

Un ordre lexicographique de centres/supports exacts est une convention déterministe
dans un repère donné, indépendante de l'ordre d'entrée ; elle ne garantit pas
l'invariance par échange d'axes. Les descripteurs invariants peuvent aussi être
égaux par symétrie. Garder alors l'ensemble. Attacher au premier ancêtre commun
est une projection conservatrice distincte, dont la date est celle de cet ancêtre,
pas l'entrée cover d'origine ; elle n'acquiert pas la cible des deux triangles.

## Q5. Ne pas imposer la maturité dans la première livraison

Livrer d'abord FULL exact, core, cover ensembliste et relation boule→nœud, avec
rangs exacts et vrais plateaux. Ces baselines rendent les pertes mesurables.
La maturité reste recherche tant que présence, projection et compatibilité ne
sont pas établies ; elle ne répare pas automatiquement une projection instable.

Garder les triangles en porte permanente : FULL porte ABC, CD, DEF ; la cible
points reste ABC|DEF avant fusion globale. Les deux objets ne se confondent pas.
Une masse géométrique recouvrante ne garantit pas mcs membres exclusifs après
projection, comme [le témoin trois points](../../morsehgp3D_v10/receipts/audit_independant_20261002/maturity_review/README.md)
l'établit. La première livraison doit annoncer sa règle d'attache et mesurer
ces trois étapes, sans qualifier son modèle statistique par la seule sélection.

## Robustesse : FULL stable en rayon, premier cover et LCA à distinguer

Apparier tous les retours avec déplacement maximal ε, dans le même repère,
conserve les inclusions L_k^X(r²)⊆L_k^Y((r+ε)²) et réciproques. Les cartes
sur les composantes commutent avec les verticales. Le cover **dynamique à tous
les rayons** et les dates de première couverture ont aussi cette garantie.
Cela ne couvre ni le catalogue ni les affectations figées au premier instant.
[Preuve et trois fixtures u18](../receipts/audit_independant_20261002/boundary_stability_review_2/README.md).

X={0,2,4}, K2 : le point médian entre dans deux composantes à r=1 et sa
projection LCA est datée r=2. Dans Y={0,2,4+δ}, δ>0 arbitrairement petit,
il entre seulement dans la composante gauche à r=1 ; la projection est datée 1.
Les dates FULL bougent d'au plus δ/2, mais l'attache projetée change de 1.
La baseline LCA acceptée est équivariante et laminaire à K fixé ; elle n'acquiert pas
pour autant une stabilité géométrique. À tester avant masses, mcs et sélection.

Les [comparaisons MR/cover de l'autre auditeur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md)
séparent aussi les hiérarchies sur {0,2,5}. Conserver MR comme témoin concurrent,
sans transformer un accord moyen des meilleurs blocs en identité des objets.

## Plusieurs ordres : les groupes core peuvent se croiser

P4 de `MATHEMATIQUES.md` prouve la laminarité **à K fixé**. Même les groupes
statiques de descendants, une fois toutes les attaches terminées, ne sont
pas nécessairement compatibles entre ordres. Témoin exact u18 collinéaire :
X={0,10,11,26,27,45,46}.

| Ordre | Groupe core du nœud | Naissance β | Parent β |
| --- | --- | ---: | ---: |
| K1 | S1={0,10,11} | 25 | 225/4 |
| K2 | S2={10,11,26,27} | 64 | 361/4 |

À K2, 0 n'entre core qu'à β=100, après le parent de S2 : il s'attache plus
haut et ne devient jamais descendant de ce nœud. S1∩S2={10,11},
S1\S2={0}, S2\S1={26,27}. Une seule hiérarchie laminaire ne peut donc
conserver les deux groupes. Les verticales à coupe fixée restent cohérentes ;
ces groupes ont des dates différentes. [Modèle Gamma indépendant, dates,
parents et contrôles normal/−O](../receipts/audit_independant_20261002/cross_order_contract_review_3/README.md).

Cela ne bloque pas la première livraison par K. Pour une hiérarchie commune,
déclarer le critère de choix et publier les groupes présents, incompatibles
et perdus lors de la projection. Faire du témoin une future porte G4, sans
imposer une politique ad hoc ni prétendre à une supériorité statistique.
