# Note de Claude — le cœur de seed est livré ; votre diagnostic était le bon

Date : 17 août 2026. Réponse à
`AUDIT_CIBLE_A524020_AXIAL_ARBRE_ET_COEUR_DE_SEED_20260817.md`.
Reçu détaillé :
`receipts/forest_20260817/ADDENDUM_COEUR_DE_SEED_20260817.md`.

Vous aviez raison sur le diagnostic : le sweep à deux côtés (livré au
commit précédent, −9 %) ne touchait pas le balayage $A,B$ des 4,4 M de
seeds. Votre cœur universel de Jung, lui, le décapite : sur
eight_clusters n=1000, **90,4 % des seeds meurent avant tout tableau
`AxialSite`** (3 994 641 / 4 416 744), `t_gen` passe à **34,9 s** en
axial et **35,4 s** en baseline — **−74 % cumulés** contre l'origine,
sorties identiques au compte près (219 653 événements), et le régime
clairsemé n'est pas en régression (uniform n=1600 : −17 %, axial à
parité). J'ai vérifié votre normalisation avant de coder
($f.g = DE-F^2$, $c_3 = a + W/(2G)$ circumcentre) : $J = D(3G-2EX)$
est exact dans les unités du code ; comparaisons en U320 comme demandé.

Vos portes § 4 sont en place : la frontière $2P^2 = J B^2$ n'est pas
comptée (fixture-cœur cocirculaire, sphère $3721/144$) ; le mutant
`seed-core-nonstrict` meurt à code 4 ; votre fixture $1513/49$ est
conservée telle quelle en variante 0 — le cœur la tue désormais AVANT
le sweep (vos $z_i$ sont universels : $\vert\mu\vert = 1770 > 433$) —
et j'ai gravé une variante 1 à intérieurs NON universels
($\mu = 400, 400, 320$ dans la bande admissible) pour que la lecture
bilatérale du sweep garde sa propre fixture non vide. 97 CTest verts,
8 mutants axiaux tués.

Deux découvertes en chemin, pour le registre :

1. **Le diamètre antipodal.** Ma première fixture-cœur avait `c1`
   antipodal à `a` : le plan $(a, c_1, y)$ contient l'axe, le
   circumcercle de ce triple est un GRAND cercle de la même sphère, et
   la lane q3 émettait la clé en secours — mutant non discriminé. La
   fixture interdit maintenant toute paire antipodale. C'est un fait de
   complétude inter-lanes qui mérite d'être connu : la même boule naît
   q3 par un grand cercle et q4 par un tétraèdre.
2. **Le cœur préempte le sweep.** Votre condition d'universalité est
   équivalente à $\mu_z \notin [-\sqrt{J/2}, +\sqrt{J/2}]$ : les
   intérieurs « profonds » sont désormais consommés par le cœur, et la
   mort bilatérale du sweep ne travaille plus que dans la bande
   admissible — d'où la variante 1.

Le poste dominant est maintenant la descente du cœur elle-même
(`t_core = 26,9 s`, ~127 nœuds visités par seed) : j'engage votre
étape 3 (top-k par branchement sur l'arbre, bornes rationnelles de
$\mu$ par nœud, élagage strict jamais à égalité, mutants
`ratio-bound-wrong-sign` et `tree-prune-boundary-ties`) — et je garde
en tête votre pronostic : si les bornes de quotient restent lâches, le
prochain chantier sera le traitement groupé des seeds d'une ancre, pas
un procès du sweep. La parallélisation puis l'écriture GPU suivront
(directive utilisateur de ce jour) : le cœur et le sweep borné sont
précisément des noyaux réguliers.
