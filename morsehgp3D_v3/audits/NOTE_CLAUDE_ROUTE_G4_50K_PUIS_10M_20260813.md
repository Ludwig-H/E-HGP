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

Deux membres de `S` sont donc distants d'au plus `2R`, et `B` est contenue dans
la boule de centre `a` et de rayon `2R` pour tout `a` de `S`. C'est exactement
ce que le relevé d'arrangement n'a pas : un sommet du `<=k`-niveau peut relier
quatre points arbitrairement éloignés par une sphère de centre extérieur au
tétraèdre.

**RETRACTATION.** Une première version de cette note ajoutait « tout support
propre positif tient dans une boule qui contient au plus onze points ». C'est
**faux**, et le contre-audit
[`AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md`](AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md)
le refuse à juste titre. La condition de Source S est `p+q<=smax` avec
`p=|I_B|` et `q=|S|` : elle ne dit rien de `|U_B|`. Le shell global d'une boule
pertinente peut porter `Theta(n)` labels, et le dépôt contient déjà un support
q2 pertinent de rang fermé douze ainsi qu'une fixture à shell trente. Il en
suit que « le nombre de supports par ancre est petit » ne découle pas de la
pertinence, et que `M=128` ou `256` est un **choix diagnostique**, jamais une
borne. Seule l'inclusion géométrique `B` dans `ball(a,2R)` est utilisée
ci-dessous ; elle ne dépend d'aucun compte.

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

1. une requête k-NN **exacte** lit `M+1` voisins, conserve les `M` premiers
   comme fenêtre `W_M(a)` et garde le premier omis comme **coupure**

$$\delta_{\mathrm{out}}(a)^{2}=\min_{x\notin\lbrace a\rbrace\cup W_{M}(a)}\left\lVert x-a\right\rVert^{2},$$

   avec `+infini` publié explicitement lorsqu'un scan total n'omet rien ;
2. tous les supports propres positifs `S` contenant `a` sont générés
   **à l'intérieur de la fenêtre**, avec leur `I_B`, leur shell fermé `U_B` et
   leur `BallKey` ;
3. le certificat de fenêtre est **exact** :

$$4R^{2}<\delta_{\mathrm{out}}(a)^{2}\Longrightarrow X\cap B\subseteq\lbrace a\rbrace\cup W_{M}(a).$$

   Preuve : pour `a` sur le bord de `B(c,R)` et `y` dans `B`,
   `|y-a| <= |y-c| + |c-a| <= 2R`. L'inclusion couvre donc `S`, `I_B` **et tout
   le shell global `U_B`**. L'inégalité est **stricte** : à l'égalité, le
   premier omis pourrait être sur la sphère, et le cas part au résiduel.

**La coupure est le premier OMIS, pas le `M`-ième inclus.** Avec `d_M(a)`, le
`M`-ième site retenu ne peut lui-même appartenir à aucune boule ainsi
certifiée : la comparaison reste sûre mais perd des supports sans raison. Le
mutant `<=` doit mourir sur la fixture `a=(0,0,0)`, `b=(2,0,0)`, `c=(0,2,0)`,
`M=1`, où `4R^2 = delta_out^2 = 4` et où le support `{a,b}` serait omis ou
gardé selon le seul tie-break.

`R` est rationnel : la comparaison n'est **pas** en général celle de deux
entiers 64 bits. Il faut croiser numérateur et dénominateur en largeur prouvée,
avec repli multiprécision. Un **chemin rapide entier** existe cependant, par
Jung : `4R^2 <= (3/2) diam(S)^2`, donc

$$3\,\mathrm{diam}(S)^{2}<2\,\delta_{\mathrm{out}}(a)^{2}\Longrightarrow4R^{2}<\delta_{\mathrm{out}}(a)^{2},$$

et ce test-là tient dans `i64` sur le profil u16. Il est seulement suffisant :
son échec renvoie au test rationnel exact, jamais au résiduel directement.

**Statut exact de ce certificat.** Il prouve qu'un support local *déjà énuméré*
et certifié est global. Il ne prouve ni la complétude de l'énumérateur local,
ni celle du résiduel. La réciproque — tout support global vérifiant l'inégalité
est retrouvé — n'est vraie que sous des prémisses qui restent à établir :
requête top-`M` et coupure exactes, énumérateur produisant **tous** les q2, q3
et q4 de la fenêtre indépendamment par arité et sans cap silencieux,
indépendance affine, positivité, niveau et identités exacts, census
reconstruisant l'ensemble fermé `I_B union U_B`, et politique explicite du
rayon nul et des positions colocalisées. Le statut visé est donc
`exact_window_certified_subsource`, **pas** `complete_global_source`.

**L'owner ne filtre pas avant certification.** Avec `a=(0,0,0)`, `b=(10,0,0)`,
`c=(0,1,0)` et `M=2`, le support `{a,b}` échoue le certificat vu de `a` et le
passe vu de `b`. Un chemin où seul l'owner minimal propose perd ce record.
Chaque endpoint certifiant émet donc une occurrence ; le RLE attribue l'owner
canonique **après**, et agrège tous les `SupportKey` d'une même `BallKey`.

### 4.2 Le résiduel — une couverture complète, pas une file de rebuts

**Correction majeure.** La première version de cette note écrivait « les
supports manqués sont exactement ceux dont la miniboule déborde la fenêtre, et
ils vivent dans les directions ouvertes ». Cette phrase colle trois objets sans
théorème de raccord : une boule globale encore inconnue, l'échec d'un
certificat sur un candidat **déjà formé**, et une cellule directionnelle
flottante issue d'un diagnostic échantillonné. Elle est retirée.

Mettre en file les candidats locaux qui échouent le certificat ne couvre pas
les supports globaux dont **aucun tuple n'a jamais été formé** dans une
fenêtre. Le résiduel doit donc partitionner un **domaine de recherche**, avant
les tuples : une tâche porte `(ancre, cellule directionnelle, intervalle de
rayon, epoch)`, ou bien un bloc collectif `A times B times C`. Ses cellules
doivent couvrir la sphère **exactement**, ses frontières être half-open, et
chaque split conserver la masse. L'identité à faire tenir sur petit `n`, contre
l'oracle exhaustif, porte sur les identités et non sur des comptes :

$$\mathcal{S}_{\mathrm{globale}}=\mathcal{S}_{\mathrm{fenetre\ certifiee}}\mathbin{\dot\cup}\mathcal{S}_{\mathrm{residuelle}}.$$

Le diagnostic à 512 directions de la section 3 sert à **ordonner** ce résiduel
et à justifier la priorité du chemin rapide. Il ne le certifie pas.

Les deux fichiers que je citais comme primitive directionnelle ne ferment pas
ce trou : `exact_ray_sweep.hpp` part d'une paire déjà choisie et mesure la
profondeur de ses sphères, `first_incidence_dichotomy.cpp` part d'une facette
du cœur déjà connue et emploie un univers oracle. Aucun ne paramètre
exhaustivement les directions d'une ancre pour générer son premier support.
« Faire croître jusqu'au premier contact » reste à prouver : couverture des
directions, owner, ordre des contacts, tangences et ex æquo.

Une alternative recommandée par l'audit remplacerait avantageusement l'étape 3 :
un oracle composante--domaine rendant le **minimum sortant exact** de chaque
composante sur le domaine résiduel, avec toutes les égalités requises et un reçu
de couverture des directions non sélectionnées. C'est une nouvelle
implémentation du résiduel, pas sa suppression.

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

La positivité explique la différence **sémantique** entre les deux objets :
l'arrangement compte toutes les sphères par quatre points, tandis que Source S
ne retient que les centres appartenant à l'enveloppe convexe du support. Sur la
famille ci-dessus, le quotient exact vaut environ `68 735,56`, mais il compare
des sommets q4 à toutes les sorties q2--q4 et ne constitue pas un taux de rejet
homogène. Ce constat ne donne aucune économie de travail automatique. Tester la
positivité avant l'émission évite de stocker un transit non positif ; si ses
quatre points ont déjà été formés et son centre calculé, son coût de proposition
est payé. La génération locale doit donc publier et faire passer la pente de
`q2/q3/q4_products_considered`, pas seulement celle des émissions.

## 5. Ce qu'il faudrait recevoir avant de viser `10^7`

| ressource | route par paires | route par fenêtre locale |
| --- | --- | --- |
| travail | `n^{1,8}` mesuré : `~10^{13}` tests à `10^7` | inconnu ; l'énumération naïve vaut `M+C(M,2)+C(M,3)` propositions par ancre |
| état vivant | frontières et masques par paire si matérialisés | fenêtres des seules ancres actives, index global et résiduel borné ou spillable |
| sortie | non bornée | espérance Poisson `~480` par point, sans borne au pire cas ; stream obligatoire |
| découpe | les relations peuvent être tuilées, sans devenir sparse pour autant | sous-nuages non indépendants ; RLE, niveaux, lots et fold exigent une ordonnance globale |

Le point décisif pour `10^7` n'est pas le seul débit : les supports ne peuvent
pas tous rester résidents. Les `4,8` milliards attendus sous Poisson doivent
être streamés ou réduits à la volée, tout en conservant une ordonnance globale
exacte. Cette espérance n'est ni une borne de sortie, ni une preuve que la
réduction est possible avec un état borné.

**Correction : le nuage ne coûte pas 60 Mo.** Écrire « le nuage occupe 60 Mo,
donc le nuage et le fold tiennent » était une erreur de comptabilité. Les
planchers réels, à `n=10^7` :

| objet | taille si matérialisé |
| --- | ---: |
| coordonnées seules, `3*u16` | `60 Mo` |
| un index dense `u32` par point | `40 Mo` |
| un `PointId:u64` durable par point | `80 Mo` |
| une coupure carrée `u64` par point | `80 Mo` |
| listes explicites `M=128`, indices u32 | `5,12 Go` |
| listes explicites `M=256`, indices u32 | `10,24 Go` |
| snapshot de traversée reçu, `192n-80` octets | `1 919 999 920` octets, environ `1,79 Gio` |

La ligne enregistrée emploie un `PointId:u64`; un `DensePointIndex:u32` est une
identité locale distincte et exige une bijection durable. Les listes `W_M(a)`
ne doivent donc **jamais** être toutes résidentes : elles se calculent par tuile
et vivent dans les mémoires rapides pour les seules ancres actives. Le layout
de traversée reçu inclut déjà coordonnées, `PointId` en ordre Morton et
`2n-1` nœuds de 80 octets. Le fold porte des handles de facettes et de carriers
— un DSU des seuls `PointId` est **incorrect** dès les ordres supérieurs.

**Correction : les tuiles ne sont pas indépendantes.** `delta_out` est le
résultat d'une requête **globale**, pas un halo connu d'avance ; une tuile ne
peut déclarer son halo fermé que si l'index global prouve qu'aucune page omise
ne contient un site plus proche. Et même alors, un support résiduel peut
traverser plusieurs tuiles sans rayon local borné. Réduire chaque tuile à sa
composante finale puis fusionner ces composantes perd les niveaux de connexion
et les lots simultanés. Le protocole minimal exact est : halo en lecture seule
et owner canonique indépendant du scheduling ; RLE global par `SupportKey` puis
`BallKey`, y compris entre tuiles ; front global spillable pour les résiduels
transfrontières ; flux de chaque tuile trié par `(ordre, niveau exact, clé
canonique)` ; fusion multiway globale, gel de toutes les racines du niveau,
construction de toutes les incidences, puis **commit unique du lot égal** ; DSU
global ou contractions locales accompagnées d'un certificat de coupe. Aucune
constante de résumé d'interface n'est prouvée : au pire, ce résumé est aussi
gros que le flux d'événements.

Le mot « tuile » ne doit pas emprunter une preuve par homonymie : les chunks
déjà reçus dans l'architecture sont des suites de lots exacts complets, pas des
sous-nuages spatiaux. La couture spatiale avec halo est une **nouvelle
obligation de preuve**.

Une relation de paires peut être partitionnée en tuiles `A times B`; ce
découpage ne la rend toutefois ni sparse, ni indépendante. Une paire
transfrontière reste à couvrir et un état explicite proportionnel au nombre de
paires reste interdit.

Trois réserves, écrites franchement :

- le degré de Gabriel n'est **pas** borné — deux constructions à treize voisins
  réfutent déjà le cap 12 au dépôt — et `smax` ne borne pas `|U_B|`. La fenêtre
  `M` n'a donc aucune borne universelle ;
- un échec de fenêtre sur une entrée régulière est un **résiduel normal**. Ce
  n'est pas `unsupported_degeneracy`, qui n'appartient qu'à une vraie violation
  du domaine mathématique, ni `resource_exhausted`, qui est atomique et
  physique. Confondre les trois masquerait une incomplétude en dégénérescence ;
- le résiduel n'a aujourd'hui ni domaine complet, ni juge, ni rampe. Tant qu'il
  n'en a pas, la route n'est pas une route.

## 6. Séquencement

### Étape 0 — dette d'exactitude ouverte par les audits du 13 août

Le commit `519ddfb` ferme les entrées 1 à 5 et une partie de 7 ci-dessous ; le
rejeu indépendant est rapporté dans `AUDIT_ETAT_COURANT.md`. Les autres restent
à solder avant de promouvoir une nouvelle géométrie. Aucune de ces entrées
n'est une optimisation.

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
fenêtre tronquée, positivité omise, certificat `4R^2<delta_out^2` inversé, `I_B` compté
hors fenêtre, owner dupliqué. La porte échoue si le résiduel est vide — un
certificat qui ne renonce jamais est faux.

### Étape 2 — la génération locale elle-même

C'est le vrai travail algorithmique et il n'est pas encore écrit. Contrainte :
ne jamais former `C(M,4)`. La piste est la construction locale par arités
croissantes avec transport du niveau, restreinte à la fenêtre et filtrée par
positivité avant émission. Elle doit publier son propre coût et échouer si deux
pentes successives d'un compteur dominant dépassent `1,35`, sans confondre ce
seuil avec une preuve linéaire.

### Étape 3 — le résiduel directionnel

Les cellules ouvertes d'une ancre sont calculées, pas devinées. Le résiduel met
en concurrence un front directionnel exact, le lift collectif
`A_endpoint times B_partner times C_witness` et les certificats collectifs de
[`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md).
`exact_ray_sweep` reste un juge à paire déjà fournie, pas le générateur complet.

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

Index ou annuaire global certifiant les requêtes k-NN, fenêtres par tuiles de
travail, résiduel trans-tuile, runs triés et merge multiway global des niveaux
et lots. Le fold porte des handles de facettes/carriers et aucune contraction
locale n'est commise sans certificat de coupe. Un index dense `u32` au-delà de
`65 535` points reste distinct du `PointId:u64` durable. Sortie streamée, mais
lot égal et verticales fermés globalement avant tout checkpoint scientifique.

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

## 8. Réponses reçues de l'auditeur

[`AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md`](AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md)
répond aux cinq questions ci-dessous. Les corrections qu'il impose sont
intégrées aux sections 2.2, 4.1, 4.2 et 5 ; les questions restent affichées
telles qu'elles ont été posées, avec leur réponse.

- **Q1 — oui, mais.** Le certificat est suffisant, et équivalence seulement
  **conditionnelle**, dans un sous-domaine complètement énuméré. Il faut la
  coupure au premier omis. Statut : `exact_window_certified_subsource`.
- **Q2 — oui pour la sémantique, non pour l'algorithme.** Sur la famille
  `A_i/B_j`, tous les sommets q4 comptés sont bien non positifs, et le rapport
  exact est `68 735,56…`, non `68 000`. Mais tester la positivité **après**
  avoir formé les quatre points et calculé le centre paie tout de même le
  facteur. L'étape 2 doit prouver qu'elle ne forme pas ces transits, avec
  `q4_products_considered`, `lifts`, `positivity_tests`, `positive_candidates`
  et `emitted_supports`.
- **Q3 — aucun analogue reçu.** Aucun analogue de Yao applicable à l'ordre `k`
  n'a été trouvé pour l'objet Morse 3D, ni au dépôt ni dans la recherche ciblée.
  Chazelle et al. montrent même
  qu'un Gabriel ordinaire peut avoir `Omega(n^2)` arêtes en dimension trois, et
  un corollaire d'audit ferme la variante littérale sur les facettes avec
  `Omega(m^2)` arêtes dans une même composante. La route reste
  **conditionnellement locale**, jamais inconditionnellement linéaire.
- **Q4 — non.** Un support long peut être la première liaison entre deux amas
  ou deux feuilles de surface. Une cellule ouverte qui rencontre encore le cône
  tangent positif n'est pas un certificat d'absence de fusion. La fixture E5 le
  rend concret : une incidence
  silencieuse ne fusionne pas immédiatement, mais installe une facette qui
  porte une fusion ultérieure. L'étape 3 reste.
- **Q5 — voir la section 5 corrigée.**

## 9. Questions posées

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
