# Note de Claude — route G4 : 50 000 points d'abord, dizaines de millions ensuite

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Cette note prend du recul sur l'ensemble du chantier et propose **une** route,
avec ses raisons et ses réfutations. Elle ne reçoit aucun snapshot, ne
revendique aucune complexité asymptotique et ne qualifie aucun SLO. Les
verdicts logiciels restent dans
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md). Les mesures nouvelles
publiées ici sont des **diagnostics reproductibles**, pas des théorèmes.

## 0. Objectif désormais à deux horizons

| horizon | taille | cible | statut |
| --- | --- | --- | --- |
| contrat courant | `n=50 000`, `K=10` | `warm_e2e` p95 `<100 ms` principal, `<1 s` secondaire | entièrement ouvert |
| horizon industriel | `n` de l'ordre de `10^7` sur G4 | objet complet, streamé, sans catalogue global | à inscrire dès maintenant dans les choix |

Le second horizon n'est pas une extrapolation du premier : il **élimine des
routes qui pourraient sembler acceptables à 50 000**. Une ordonnance dont le
coût croît en `n^{1,8}` est déjà refusée à 50 000 ; à `10^7` elle est absurde
de six ordres de grandeur. Une ordonnance qui matérialise un tableau
proportionnel à la sortie totale tient à 50 000 et ne tient plus à `10^7`.
Toute décision d'architecture doit donc être jugée aux **deux** horizons.

## 1. Diagnostic : tout ce qui a été mesuré est rouge, et pour une seule raison

Le dépôt a produit et mesuré six ordonnances successives. Leurs pentes log2 par
doublement, telles que les audits les ont pincées, sont :

| producteur | pin | famille testée | pentes observées | verdict |
| --- | --- | --- | --- | --- |
| self-join q2 | `8a39c53` | — | visites trop rapides | oracle/falsificateur |
| P1a center-cover q4 | `b312638` | `terrain` 2 k→8 k | `2,104` puis `1,896` | NO-GO avant G4 |
| Yao48/LBVH | `2e49dcf` | `terrain`, deux scanline | deux pentes `>1,35` | NO-GO |
| Yao48 dual persistant | `c70974e` | trois familles | `dual_witness_visits` rouge | ne ferme pas la porte |
| cellules de centres | `238cf12` | `uniform` 100→400 | quatre compteurs `>1,35` | NO-GO avant G4 |
| cône cible par endpoint | delta `3d4c598` | `uniform`, amas | `1,42` à `1,96` | NO-GO du port littéral |

Ces six ordonnances ne partagent pas leur géométrie. Elles partagent leur
**forme** : toutes partent de la paire — ou de l'ancre, qui est une paire — et
tentent de la tuer. Or le nombre de paires est `Theta(n^2)`. Tuer quatre-vingt
pour cent d'une quantité quadratique laisse une quantité quadratique.

La rampe du cône, prolongée pour cette note jusqu'à `n=16 000` sur un seul ELF
(`uniform`, `seed=3`, `leaf=8`, `bank=48`), le montre sans ambiguïté :

| `n` | tests témoin--nœud | évaluations de coins | paires candidates | `C(n,2)` |
| ---: | ---: | ---: | ---: | ---: |
| 1 000 | 9 118 007 | 22 903 448 | 380 939 | 499 500 |
| 2 000 | 29 143 814 | 70 496 422 | 1 369 645 | 1 999 000 |
| 4 000 | 99 552 271 | 232 074 723 | 4 920 845 | 7 998 000 |
| 8 000 | 349 231 295 | 775 515 430 | 17 831 971 | 31 996 000 |
| 16 000 | 1 147 205 897 | 2 395 210 838 | 63 149 448 | 127 992 000 |

Pentes successives des paires candidates : `1,846`, `1,845`, `1,858`, `1,824`.
Elles **ne décroissent pas**. Le certificat conique retire une fraction de la
masse — de moitié environ à `n=16 000` — mais il ne change pas l'exposant. Une
banque plus grande abaisse le préfacteur et l'exposant apparent sur trois
points, jamais l'exposant asymptotique : c'est la lecture binomiale de la
section 2.1 de
[`AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md`](AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md).

La conclusion chiffrée mérite d'être écrite en une phrase. En prolongeant la
dernière pente — diagnostic, jamais théorème — la route par endpoint dépense à
`50 000` de l'ordre de neuf milliards de tests pour laisser de l'ordre de cinq
cents millions de paires candidates, **là où l'objet lui-même ne contient
qu'environ vingt-quatre millions de supports**. Le certificat travaille contre
un univers vingt fois plus gros que sa propre sortie.

**Le verrou n'est pas le prédicat. C'est l'énumération.**

## 2. Ce que le corpus prouve déjà, et qui est linéaire

Trois résultats du dépôt sont durables et pointent tous dans la même direction.

**2.1 Yao-1 est une source générative linéaire à `k=1`.** Sur des positions
deux à deux distinctes, les plus proches voisins exacts dans les 48 chambres
Yao forment un graphe d'au plus `48n` arêtes contenant un EMST. Aucune paire
n'est énumérée : chaque point interroge 48 cônes. `k=1` est **résolu**, et il
est résolu par génération locale, pas par élimination.

**2.2 La positivité rend un support compact.** Pour un support propre positif
`S`, la boule `B` est la miniboule de `S` : son centre est dans l'enveloppe
convexe de `S`. Jung donne alors en dimension trois

$$R\leq\mathrm{diam}(S)\sqrt{\frac{3}{8}},\qquad\text{donc}\qquad \mathrm{diam}(S)\geq R\sqrt{\frac{8}{3}}\ \text{ et }\ \mathrm{diam}(S)\leq2R.$$

Comme `B` contient au plus `smax` points de rang fermé, **tout support propre
positif tient dans une boule qui contient au plus onze points**, et deux de ses
membres sont distants d'au plus `2R`. C'est exactement ce que le relevé
d'arrangement n'a pas : un sommet du `<=k`-niveau peut relier quatre points
arbitrairement éloignés par une sphère de centre extérieur au tétraèdre.

**2.3 Le théorème de localité par calottes est démontré.** Section 2 de
[`NOTE_SOLUTION_LOCALITE_CERTIFIEE_INVERSION_20260812.md`](NOTE_SOLUTION_LOCALITE_CERTIFIEE_INVERSION_20260812.md) :
si toute direction de la sphère appartient à au moins `K` calottes strictes
`C_z(r)`, alors toute boule passant par `x` et possédant au plus `K-1`
intérieurs vérifie `diam(B)<r`.

À quoi s'ajoute la baseline de taille : sous Poisson homogène, la Source S
compte environ `480,34` supports par point jusqu'à `smax=11`, soit environ
`24,017` millions à `50 000` points. **La sortie est linéaire en `n`.** Seules
les routes actuelles sont quadratiques.

## 3. Mesure nouvelle : quelle part de la sphère est réellement fermée

La question décisive n'avait jamais été mesurée : *le théorème de localité
ferme-t-il assez de directions pour porter une route ?* Le diagnostic ci-dessous
discrétise la sphère en 512 cellules géodésiques d'octaèdre, prend `K=10`,
borne la recherche aux 512 plus proches voisins et compte, par ancre, les
cellules qu'aucune famille de dix calottes ne ferme dans cette fenêtre.

| famille | `n` | ancres à zéro cellule ouverte | cellules ouvertes par ancre (moy.) | p50 | p90 | max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `uniform` | 2 000 | `47,1 %` | `43,3 / 512` | 78 | 157 | 247 |
| `eight_clusters` | 2 000 | `30,6 %` | `42,6 / 512` | 57 | 120 | 209 |
| `terrain` | 2 000 | `0,0 %` | `63,8 / 512` | 52 | 132 | 262 |
| `scanline_overlap_multiecho` | 2 000 | `2,6 %` | `57,0 / 512` | 34 | 138 | 335 |

Sur `uniform`, la fenêtre certifiée médiane vaut environ `196` à `218` points à
grille fine. Deux lectures, et elles renversent la route.

**Lecture 1 — la fraction fermée est la même partout : environ neuf dixièmes.**
Y compris sur `terrain` et sur les scanline, c'est-à-dire précisément les
familles LiDAR. `8,5 %` à `12,5 %` des directions restent ouvertes.

**Lecture 2 — la fraction d'ancres *totalement* fermées est trompeuse.** Elle
tombe à zéro sur `terrain` non pas parce que la localité y échoue, mais parce
que **chaque** point d'une surface possède son petit cône normal ouvert. Lire
`0 %` comme « la localité ne marche pas sur les surfaces » est une erreur : la
localité y ferme neuf dixièmes du travail, et le dixième restant est
structurellement identifié, pas dispersé.

Ces chiffres sont un diagnostic flottant sur un nuage donné, à `K=10`, avec
une grille et une banque finies. Ils ne bornent rien. Ils suffisent en revanche
à choisir une route.

## 4. La route proposée : générer les supports, ne plus éliminer les paires

### 4.1 Le noyau fermé — environ neuf dixièmes du travail

Par point `a`, et seulement par point :

1. une requête k-NN **exacte et bornée** construit la fenêtre `W(a)` des `M`
   plus proches voisins, `M` de l'ordre de 128 à 256 ;
2. tous les supports propres positifs `S` contenant `a` sont générés
   **à l'intérieur de la fenêtre**, avec leur `I_B`, leur shell et leur
   `BallKey` ;
3. le certificat de fenêtre est **a posteriori, exact et entier** :

$$4R^{2}<d_{M}(a)^{2}\Longrightarrow\text{le support est global},$$

   où `d_M(a)` est la distance au `M`-ième voisin. Preuve : `S union I_B`
   est contenu dans `B`, et pour `a` sur le bord de `B` tout point de `B` est
   à distance au plus `2R` de `a` ; si `2R<d_M(a)` alors `B` est entièrement
   dans la fenêtre, donc `I_B` y est exact et aucun partenaire n'a pu être
   manqué. Réciproquement tout support global vérifiant cette inégalité est
   trouvé par le calcul local. C'est une **équivalence sur son domaine**, pas
   une heuristique.

Ce certificat ne coûte rien : il compare deux entiers déjà calculés. Il ne
demande ni couverture de calottes à l'exécution, ni grille sphérique. Le
théorème de localité sert à **dimensionner** `M`, pas à le payer.

Ce qui échoue le test — `4R^2>=d_M(a)^2` — n'est ni supprimé ni tronqué : il
part au résiduel de la section 4.2 avec son reçu.

### 4.2 Le cône ouvert — environ un dixième du travail

Les supports manqués sont exactement ceux dont la miniboule déborde la fenêtre.
Ils vivent dans les directions ouvertes mesurées à la section 3. Deux
propriétés les rendent traitables :

- leur nombre par ancre est petit, parce qu'une grande miniboule vide impose au
  plus onze points de rang fermé ;
- ils sont **directionnels** : à direction fixée, faire croître le diamètre
  jusqu'au premier contact donne le support, sans énumérer aucune paire.

C'est la primitive du front inverse déjà présente au dépôt
(`prototype/exact_ray_sweep.hpp`, `prototype/first_incidence_dichotomy.cpp`).
Elle n'est aujourd'hui ni complète ni jugée ; elle devient ici un composant du
chemin, pas une route concurrente. Son domaine est réduit d'un facteur dix par
le noyau fermé, et surtout il est **nommé** : l'ensemble des cellules ouvertes
d'une ancre est calculé, pas deviné.

### 4.3 Le raccord

Owner exact-once par `BallKey`, RLE par `SupportKey` avant tout lift, census
`I_B/U_B` unique par boule, puis fold streamé vers les composantes. Aucun
catalogue global n'est matérialisé : un support est produit, consommé, oublié.

### 4.4 Ce que cette route évite, et qui a déjà été réfuté au dépôt

| piège | réfutation existante |
| --- | --- |
| naviguer le `<=k`-niveau de l'arrangement relevé | `34 364 000 715` sommets contre `499 945` supports à `n=50 000` sur la famille `A_i/B_j` |
| éliminer des paires | six ordonnances mesurées, pentes `1,42` à `2,10` |
| catalogue hôte de supports | `24` millions à 50 k, `6,33 Go` d'occurrences |
| tronquer sous budget | interdit par la spécification : l'objet complet ou un échec sur ressource réelle |

La différence entre la navigation d'arrangement et la génération locale est
**la positivité**. L'arrangement compte toutes les sphères par quatre points ;
la Source S ne retient que celles dont le centre est dans l'enveloppe convexe
du support. La famille contre-exemple ci-dessus est faite d'un rapport
`68 000` entre les deux. Générer par fenêtre locale ne produit jamais un
transit non positif, parce que la positivité est testée avant l'émission et que
la fenêtre est choisie par la géométrie de la miniboule, pas par celle d'une
sphère quelconque.

## 5. Pourquoi cette route franchit `10^7`, et pourquoi les autres non

| ressource | route par paires | route par fenêtre locale |
| --- | --- | --- |
| travail | `n^{1,8}` mesuré : `~10^{13}` tests à `10^7` | `O(n)` fois une constante explicite `M` |
| état vivant | frontières et masques par paire | `M` indices par point, en registres/shared |
| sortie | non bornée | `~480` supports par point, **streamés** |
| découpe | aucune | le nuage se tuile spatialement, chaque tuile est indépendante |

Le point décisif pour `10^7` n'est pas le débit, c'est que **rien ne doit être
résident à l'échelle du nuage entier sauf le nuage et le fold**. À `10^7`
points en u16, le nuage occupe `60 Mo` ; la forêt de sortie est `O(nK)` ; les
`4,8` milliards de supports attendus ne sont jamais matérialisés, ils sont
réduits à la volée. Une tuile de `10^5` points avec son halo tient entièrement
dans la mémoire d'une G4, et deux tuiles ne communiquent que par le fold.

À l'inverse, aucune route par paires ne se tuile : la paire `(a,b)` traverse
les tuiles par construction, et son état est proportionnel au nombre de paires.

Deux réserves, écrites franchement :

- le degré de Gabriel n'est **pas** borné — deux constructions à treize voisins
  réfutent déjà le cap 12 au dépôt. La fenêtre `M` n'a donc aucune borne
  universelle. La route doit **refuser** (`unsupported_degeneracy` ou
  insuffisance physique atomique), jamais tronquer. C'est précisément
  l'invariant industriel déjà écrit à la spécification ;
- le résiduel directionnel n'a aujourd'hui ni juge de complétude ni rampe. Tant
  qu'il n'en a pas, la route n'est pas une route.

## 6. Séquencement

### Étape 0 — dette d'exactitude ouverte par les audits du 13 août

À solder avant toute nouvelle géométrie. Aucune de ces entrées n'est une
optimisation.

1. `smax` : vérifier `errno` après `strtoll`, borner **avant** tout cast en
   `int`, graver `LLONG_MAX`, `INT_MAX+1`, `3`, `borne+1` et suffixe. Le faux
   vert actuel ferme `380/380` paires sans un seul test, sujet et juge
   partageant la conversion fautive ;
2. cardinalité : exiger `pts.size()==opt.n` juste après `make_family_cloud`,
   refuser en code 2, publier le digest du nuage ;
3. juge par lane : trois bitsets sujet et trois vérités indépendantes, avec les
   trois inclusions `closed_q subset dead_q`, un plancher non nul par lane et
   les fixtures `q3`-sans-`q2`, `q4`-sans-`q3` et égalités. L'oracle redérive
   ses seuils et prédicats sans inclure `spindle_cone.hpp` ni
   `anchor_envelope.hpp` ;
4. porte permanente pour le mutant `cone-ignore-inherited`, déjà écrit et déjà
   tueur, mais absent du CMake ;
5. calculer `floor` **avant** de consulter `nq3`/`nq4` ; compter les
   réfutations `NONE` en transitions, pas en visites ; ajouter
   `none_classifier_calls` ;
6. retirer `PASS_REGULAR_EXPRESSION` de **toute** porte qui porte un plancher,
   y compris les portes `anchor_` : CTest ignore le code de sortie ;
7. LCG en `uint64_t`, mutant `narrow-i64` à `wrap` défini, cible UBSan, porte
   comparant la banque k-NN à un top-`M` exhaustif ordonné, message de refus
   `[1,256]`, réflexions réellement exercées ou claim réduit ;
8. la garde de densité sort du chemin produit après un dernier reçu pincé ; la
   signature `run_anchor_point` est propagée dans l'ABI hôte/device pour que la
   cible CUDA opt-in compile.

### Étape 1 — le certificat de fenêtre, seul

Un sujet minimal qui, par point, construit `W(a)`, énumère les supports
positifs locaux et publie la partition exacte
`certifie + residuel = total`, avec le juge par identités `(BallKey, S, I_B,
U_B)` d'un oracle rationnel indépendant sur petit `n`. Mutants obligatoires :
fenêtre tronquée, positivité omise, certificat `2R<d_M` inversé, `I_B` compté
hors fenêtre, owner dupliqué. La porte échoue si le résiduel est vide — un
certificat qui ne renonce jamais est faux.

### Étape 2 — la génération locale elle-même

C'est le vrai travail algorithmique et il n'est pas encore écrit. Contrainte :
ne jamais former `C(M,4)`. La piste est la construction locale par arités
croissantes avec transport du niveau, restreinte à la fenêtre et filtrée par
positivité avant émission. Elle doit publier son propre coût par point et
mourir si celui-ci n'est pas plat en `n`.

### Étape 3 — le résiduel directionnel

Les cellules ouvertes d'une ancre sont calculées, pas devinées. Le balayage
exact par direction reprend `exact_ray_sweep` et lui donne enfin un juge de
complétude et une rampe.

### Étape 4 — raccord owner/RLE/census/fold, puis `BenchmarkOutputContract-v1`

Aucun temps n'est mesuré avant que le payload contractuel complet existe.

### Étape 5 — la rampe qui décide

`12 500 / 25 000 / 50 000`, quatre familles, un seul ELF, tous les compteurs, y
compris octets et high-water. Deux pentes `<=1,35` exigées. C'est cette rampe,
et elle seule, qui autorise le port device.

### Étape 6 — CUDA puis G4

Parité bit-à-bit CPU/device sur le prédicat en deux limbs 64 bits — les
produits utiles n'occupent que 70 bits, un `__int128` hôte n'est pas une
preuve. Session gardée `gcp-migration/`, VM SPOT, double coupe-circuit,
`TERMINATED` certifié.

### Étape 7 — l'horizon `10^7`

Tuilage spatial avec halo dimensionné par `d_M`, fold inter-tuiles par
composantes, index `u32` au-delà de `65 535` points — le `DensePointIndex:u16`
actuel est une limite d'implémentation, pas de mathématiques, et il doit être
nommé comme telle dès maintenant. Sortie streamée, jamais résidente.

## 7. Non-claims

Cette note ne prouve aucune borne de complexité. Le nombre de supports par
point n'est majoré par aucun théorème : la baseline `480,34` est une moyenne de
Poisson, et le degré de Gabriel est arbitraire. Les pourcentages de directions
fermées sont mesurés sur quatre familles à `n=2 000`, `K=10`, grille 512,
banque 512 ; ils ne s'étendent ni à d'autres nuages ni à d'autres tailles. La
génération locale de l'étape 2 **n'est pas écrite** et pourrait échouer sa
propre rampe. Aucune session GCP n'a été utilisée pour cette note. Le contrat
`50 000 / 1 s`, a fortiori la cible principale `100 ms`, reste entièrement
ouvert.

## 8. Questions à l'auditeur

1. Le certificat de fenêtre `4R^2<d_M(a)^2` est-il accepté comme équivalence
   exacte sur son domaine, ou voyez-vous un cas où un support global vérifie
   l'inégalité sans être trouvé par le calcul restreint à `W(a)` ?
2. Le rapport `68 000` entre sommets d'arrangement et supports de Source S sur
   la famille `A_i/B_j` est-il bien imputable à la **positivité**, c'est-à-dire
   les sommets perdus sont-ils tous des transits à centre hors enveloppe
   convexe ? Si oui, la génération par miniboule locale est-elle exempte de ce
   facteur par construction ?
3. Existe-t-il au dépôt, ou dans la littérature que vous tenez, un analogue de
   Yao-1 pour l'ordre `k` : un graphe local de taille `O(c(k)n)` dont le MSF
   d'ordre `k` est un sous-graphe ? C'est le seul énoncé qui rendrait la route
   inconditionnellement linéaire au lieu de conditionnellement locale.
4. Pour les directions ouvertes d'une surface, la forêt H0 a-t-elle réellement
   besoin de ces supports longs, ou un argument de coupe à la Borůvka permet-il
   de prouver qu'ils ne portent jamais une fusion nouvelle ? Une réponse
   positive supprimerait l'étape 3.
5. À `10^7` points, quel objet exactement doit rester résident, et sous quelle
   forme le fold accepte-t-il des tuiles indépendantes sans perdre l'exactitude
   des niveaux ?

GCP non utilisé.
