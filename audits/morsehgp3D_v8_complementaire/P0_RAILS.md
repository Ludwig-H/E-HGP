# Rails parallèles : distinguer un pool global de crédits répartis

13 septembre 2026. Audit complémentaire, `exploration_v8_hors_registre /
cpu_reference / quantized_u16_input_only /
audit_independant_math_and_architecture / not_claimed`.

**Un petit pool global fixe peut laisser un résidu quadratique, même quand
des crédits locaux certifiés le réduisent à une constante sur la famille.**
Le défaut tient à la répartition des témoins entre les régions du facteur,
pas au choix particulier d'un mauvais échantillon. Cette contre-fixture
est utile pour comparer Pool, DualBlocks et les
[tubes/rangs proposés par l'autre auditeur](../../morsehgp3D_v8/audits/P0_TUBES_ET_RANGS.md).
Elle n'impose aucun rejet d'architecture : un pool reste un proposeur utile,
et une partition directionnelle peut traiter le cas.

## 1. Construction et crédits exacts

Choisir h≥1, r≥2, L≥max(1,h−1), et poser D=48rL :

$$A=\lbrace(x,4Lj,0):x\in\lbrace0,\ldots,L\rbrace,\ j\in\lbrace0,\ldots,r-1\rbrace\rbrace,\qquad B=A+(D,0,0).$$

Les x et j sont entiers. Le nuage est A∪B : pas de crédit extérieur.
Le diamètre carré de chaque boîte est L²+16(r−1)²L² ; la distance entre
boîtes vaut D−L. La convention déclarée par la première API v8,
`box_gap >= s * max(box_diameter)`, est satisfaite à s=8,10,12 puisque :

$$\frac{(D-L)^2-144\left(L^2+16(r-1)^2L^2\right)}{L^2}=4512r-2447>0.$$

Pour q3/q4, un z d'A est un témoin universel de l'ancre a exactement
lorsqu'il appartient au même rail et se trouve à sa droite. Pour B,
la direction est inversée. Preuve directe :

- Sur le même rail, δ=z_x−a_x>0 donne H≥δ(D−L)>0 et
  Ξ≤δ²[4(r−1)L]²<2δ²(D−L)². Ces bornes couvrent toute la boîte B.
  Si δ≤0, H≤0.
- Sur des rails différents, prendre le site b=a+(D,0,0) présent dans B.
  Avec e=z_y−a_y, |e|≥4L, on a H=Dδ−δ²−e²≤DL et Ξ=D²e²≥16D²L².
  Si H>0, même 3H²<Ξ : z échoue à être universel. L'échange des facteurs
  prouve l'autre côté.

Après saturation au besoin h, les comptes valent min(h,L−x) dans A et
min(h,x) dans B. Le résidu de leurs classes est donc exactement :

$$M_{\mathrm{local}}=r^2\frac{h(h+1)}{2}.$$

## 2. Borne valable pour tout choix d'un pool fixe

Soient W_A et W_B deux ensembles fixes de témoins proposés, de cardinal
au plus t chacun, réutilisés pour toutes les ancres de leur facteur.
Noter u_j et v_k leurs populations par rail. Même une certification
parfaite de ces propositions ne peut rejeter aucune paire du produit
rail_j×rail_k si u_j+v_k<h.

Or la somme de u_j+v_k sur les r² produits est au plus 2rt. Au plus
⌊2rt/h⌋ produits peuvent donc atteindre h. D'où :

$$M_{\mathrm{pool}}\geq\max\left(0,r^2-\left\lfloor\frac{2rt}{h}\right\rfloor\right)(L+1)^2.$$

La borne concerne les pools fixes seuls. Des recherches supplémentaires,
un changement de pool par région ou des crédits de blocs sortent de cette
hypothèse ; leur coût et leur bénéfice doivent être comptés explicitement.

Exemple u16 : **q4, h=8, r=9, L=150, t=8**. Les 2 718 sites ont des
coordonnées au plus 64 950. Les crédits locaux laissent **2 916 paires** ;
n'importe quels pools fixes d'au plus huit témoins par facteur en laissent
**au moins 1 436 463**, sur un total de 1 846 881.
Cette grande valeur est une conséquence arithmétique de la preuve,
pas l'énumération de tous les pools ni un chronométrage C++.

À h et t fixés, choisir r>2t/h constant puis faire croître L donne un
résidu quadratique en n pour le pool, contre un résidu local indépendant
de L. **Cette asymptotique nécessite une précision croissante.** Le domaine
u16 fini ne permet que les instances bornées qui y rentrent. L'exemple
est plan et non régulier ; aucune tour FULL ou production réelle q4 n'est
qualifiée par son préfiltrage.

## 3. Une issue constructive à comparer

Sur cette famille, regrouper par rail puis proposer h témoins vers l'autre
facteur **dans chaque rail** restitue exactement les comptes saturés,
en O(hn) tests après le regroupement. Le tri des groupes peut coûter
O(n log n) pour une entrée non ordonnée. Ce n'est pas un histogramme
quadratique : les mêmes quelques témoins servent chaque région.

Les cellules transverses de la proposition tubes/rangs offrent précisément
une autre manière de distribuer ces crédits. Une grille qui sépare les
rails avec Q_C=0 rend tous les successeurs stricts éligibles : elle obtient
les mêmes comptes sur cette famille. Ceci suit de son contrat de suffixes,
sans prétendre requalifier ici son modèle ou un port C++.

Le cas suggère une comparaison utile au développeur : pool global seul,
crédits répartis, parcours conjoint et tubes/rangs, sur les **mêmes**
rectangles et seuils. Rapporter le budget réel de chaque proposeur, les
tests, le résidu et sa consommation. Il ne faut pas transformer le témoin
défavorable à une variante en preuve d'optimalité d'une autre.

## 4. Résultat sur le code C++ apparu pendant l'audit

La variante Pool actuelle sélectionne h+1 propositions par facteur.
Avec l'ordre (rail,x) de cette fixture et r=h+1, ce sont les extrémités
des r rails : chaque ancre obtient au plus un crédit local. À h8,
aucune paire n'est rejetée. Le [test C++](local_credits_probe.cpp) observe
effectivement **1 846 881 candidates Pool contre 2 916 DualBlocks** à
n2718, aux trois séparations. Le parcours conjoint paie 7 648 tâches,
1 224 couples de feuilles, 872 blocs positifs et 1 490 blocs négatifs.
Il retrouve les comptes saturés sans reconstruire ici le carré local.

Neuf cas q4/n162, q4/n2718 et q3/n200, chacun à s8/10/12, passent sous
C++20 strict/UBSan. L'expansion des descripteurs DualBlocks est vérifiée
physiquement, identifiants et absence de doublons compris. Les deux
mutants de répétition d'une colonne et de double crédit sont réfutés.
Le [reçu et les hashes](LOCAL_CREDITS_CHECKS.json) épinglent les sources
locales lues ; aucun résultat n'est transféré à une révision ultérieure.
Ce succès motive de poursuivre DualBlocks sur d'autres géométries,
sans en déduire le temps aval ou la clôture de P0.

Le [modèle indépendant](p0_rails_gate.py) énumère intégralement les pools
à h=1 sur n=8/12, vérifie les crédits aux coins à q4/h8/n162 et
q3/h9/n200, puis contrôle séparément l'arithmétique du grand exemple.
Ses boucles exhaustives sont des juges bornés, jamais un chemin produit.
GCP non utilisé.
