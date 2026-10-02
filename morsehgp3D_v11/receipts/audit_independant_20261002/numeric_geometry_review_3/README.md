# Geometrie numerique — lecture du commit 6a22a9118

2 octobre 2026. `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.
Capture des objets Git, pas du WIP : [SOURCE_BEFORE.json](SOURCE_BEFORE.json).
Revue statique ; aucun build, test produit, moteur ou GCP execute. La revue
Int/Wide/Level et les recus G4 sont conduits separement par l'auditeur principal.

**Aucun defaut geometrique constate dans la capture.**

- [sphere.cpp](source/morsehgp3D_v11/src/num/sphere.cpp#L15) construit des
  circumspheres q2/q3/q4 ; dependance affine = succes avec optional vide.
  D est normalise positif. q3 utilise le rayon reduit de degre 6 au lieu du
  carre de N3, de degre 10. Cela ne calcule pas encore le MEB d'une partie
  arbitraire ni une cle canonique de boule, limites annoncees dans tests/num/README.
- [geometry.hpp](source/morsehgp3D_v11/src/num/geometry.hpp#L12) ferme le domaine
  des Point et la construction des Sphere. Point::make refuse negatives et
  coordonnees >=2^B. Les centres rationnels peuvent sortir de la boite des sites ;
  les preuves des predicats n'utilisent aucune hypothese de centrage convexe.
- [predicates.cpp](source/morsehgp3D_v11/src/num/predicates.cpp#L6) calcule
  exactement D*(distance^2-rayon^2), donc interieur strict negatif et coquille
  exactement nulle. L'interiorite tetraedrique exige, pour chaque face, le meme
  signe STRICT que le sommet oppose ; un poids nul ou tetraedre plat est refuse.
  Le triangle strictement aigu est distinct de sa simple circumsphere valide.
- Le futur catalogue T0 devra appliquer ces conditions aux supports critiques
  puis recenser I et toute U sur les listes K-certifiees. Un prefixe q3 obtus
  n'autorise pas a rejeter un support q4 : [geometry_test.cpp](source/morsehgp3D_v11/tests/num/geometry_test.cpp#L59)
  grave deja le poids q4 nul et le q4 strict a prefixe obtus. Le quotient local
  demandera aussi le hull ferme ; strictly_inside ne le remplace pas.

**Bornes relues, sommes partielles comprises.** Pour |difference|<M=2^B :
dot<3M^2, cross<2M^2, det<6M^3 ; t=uu*v-vv*u<6M^3 ; N3<24M^5,
D3<24M^4 ; N4<18M^4 et D4<12M^3. La puissance est bornee par
72M^6+144M^6=216M^6. N+D*(anchor-p)<48M^5 donne une orientation de centre
<288M^7. Les deux membres du test de milieu sont <96M^5. Ces bornes portent
sur la somme des magnitudes, donc gardent tous les intermediaires signes.

Le point serre a B24 est [is_midpoint](source/morsehgp3D_v11/src/num/predicates.cpp#L77) :
96M^5<2^127, strictement, donc i128 signe suffit lorsque 5B+7=127.
N3 porte 125 bits de magnitude ; N+D*offset en porte au plus 126.
Le produit uu*vv et 4*g de q3 restent aussi natifs avant la multiplication large.
Les niveaux q3 utilisent <27M^6 et <48M^4 ; q4 <972M^8 et <144M^6.
[budgets.hpp](source/morsehgp3D_v11/src/num/budgets.hpp#L9) couvre ces bornes aux
profils 18/21/24 et refuse B32 actuellement. Le futur u32 requiert de revoir
Vec/dot/cross et les centres natifs, en plus des niveaux ; aucune qualification
32 bits n'est transferee. Les gardes scalaires sont dans [check.py](check.py).

**Oracle existant et preparation distincte.**
[fraction_oracle.py](source/morsehgp3D_v11/tests/num/fraction_oracle.py#L62) calcule
le centre par Gram/Gauss rationnel, l'interiorite par barycentriques 4x4 et les
orientations par expansion de permutations : voies distinctes de Cramer/cross
du produit. Le rayon et la puissance y sont recomputes depuis le centre exact.
Les 504 cas et recus G4 declares sont lus, pas reexecutes ici.

Notre [check.py](check.py), autonome, derive les budgets et quatre attentes par
systemes lineaires de bisecteurs : tetraedre regulier, centre sur une face,
prefixe obtus mais q4 strict, circumsphere non convexe tres mal conditionnee.
24 permutations par support gardent le centre. [G4_INPUT.txt](G4_INPUT.txt) prepare
16 requetes de coquille au protocole num/probe ; [G4_EXPECTED.json](G4_EXPECTED.json)
donne centres/niveaux/barycentriques et side=0, sans lancer la sonde. Ce dernier
cas protege le contrat general de Sphere ; il ne doit pas etre admis comme q4
critique puisque son centre est exterieur au hull.

Sorties normal/-O et empreintes conservees. [SOURCE_AFTER.json](SOURCE_AFTER.json)
referme les objets Git captures. Ni catalogue complet, ni supports canoniques,
ni index/census, ni FULL ou performance ne sont qualifies par ce recu.
