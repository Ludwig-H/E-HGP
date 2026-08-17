# État courant de l'audit mathématique de `morsehgp3D_v4`

Date : 17 août 2026.  
Pin audité : `bebdef2` inclus, donc commits `f775c98`, `7bd3281`, `e535af2`, `f3c5105`, `1f1ae0c`, `2437254`, `30e6ccc`, `bebdef2`.  
Cadre : `phase=exploration_v4_hors_registre`, `public_status=not_claimed`.

Périmètre contrôlé : totalité de l'arborescence courante de `morsehgp3D_v4`, les dix-neuf rapports de lecture versionnés couvrant les Parties I-II du manuscrit (pages PDF 35-134), `PROPOSITION.md`, les audits et le code v3 pertinents, ainsi que la documentation, le code, les tests et les reçus v4. Les rapports de lecture restent des notes, non des autorités ; les énoncés décisifs ci-dessous ont été recontrôlés sur les formulations mathématiques et le code v4. Les statuts GitHub ne publient aucun run CI pour le pin ; les mentions `17/17 CTest` sont donc des reçus de Claude, non une exécution indépendante de cet audit.

## Verdict

**Avis favorable sur le préfiltre de mort et sur la direction d'architecture.** Je n'ai trouvé aucune fausse mort mathématique dans la descente actuellement codée. Les ledgers par lane, les comparaisons strictes, les arrondis dirigés, le masque q2/q3/q4 et la reprise à zéro entre boule-cœur et autorité complète sont cohérents. Les reçus à `n=8000` sont donc de bons reçus d'exploration : ils montrent que tuer pendant la descente est utile, notamment sur `eight_clusters`, sans prétendre encore au SLO ni à une preuve par l'expérience.

Quatre corrections contractuelles sont cependant nécessaires avant de parler d'exactitude HGP de bout en bout :

1. **le paragraphe ajouté par `1f1ae0c` distingue à tort deux rayons q3** : `D/sqrt(12) = D/(2sqrt(3))` exactement ; en q4, `D/sqrt(15)` est une sous-approximation sûre de `D sin(15°)`, et non le rayon provenant d'un autre ensemble admissible ;
2. **le profil exact d'autorité prend des sites distincts** : la bucketisation des positions dupliquées est une extension non prouvée, qui ne peut pas être silencieusement promue comme sémantique du manuscrit ;
3. **la sortie publique porte des niveaux de rayon au carré et des multifusions** : un chemin couvrant peut compresser le calcul, mais il doit conserver l'hyperarête complète, toutes les facettes utiles au rendu et, pour la sortie complète, les applications verticales entre ordres ;
4. **les petites tailles utilisent `K_eff=min(K_max,n)` et `s_max=min(K_eff+1,n)`** : la constante 11 est correcte pour la cible `n>=11, K_max=10`, pas comme vérité générale de la bibliothèque.

En résumé : **préfiltre fail-open reçu ; formule couplée `R_coup` reçue mathématiquement ; source complète, census et forêts non encore reçus ; aucun statut public promu.**

## 1. Réponse Q1 : bijection événements-boules

L'énoncé du § 2.1 est correct sous les hypothèses suivantes : `X` est un ensemble fini de sites distincts et il est en position générale au sens de la Définition 26 du manuscrit.

Soit `σ` un K-simplexe de Gabriel, `B_σ` sa miniboule et `S = σ ∩ ∂B_σ` son support minimal. La position générale implique qu'aucun point de `X \ σ` n'est sur `∂B_σ`, et Gabriel implique qu'aucun point de `X \ σ` n'est dans l'intérieur. Par conséquent :

`σ = S ∪ (X ∩ int(B_σ))`.

Réciproquement, si `S` supporte une miniboule `B` et si `σ = S ∪ (X ∩ int(B))`, alors `S ⊆ σ ⊆ B`, donc `ρ(σ) = ρ(S)` et la miniboule de `σ` est bien `B`. Aucun point extérieur à `σ` n'est intérieur à `B`, donc `σ` est Gabriel. La profondeur vaut `|X ∩ int(B)| = K+1-|S|`. La miniboule étant unique, cette correspondance est bijective.

La réduction q2/q3/q4 en découle. Le seuil exact est `h_q=s_max-q+1`, avec `K_eff=min(K_max,n)` et `s_max=min(K_eff+1,n)`. Pour `K_max=10` et `n>=11`, on retrouve bien `h_2/h_3/h_4=10/9/8`; les petits oracles ne doivent pas recevoir 11 par défaut.

### Blocage sur les doublons

Le manuscrit et la spécification d'autorité travaillent avec un ensemble de sites distincts `X ⊂ R^3`. La v4 autorise plusieurs `PointId` au même site, mais une simple multiplicité de profondeur ne suffit pas à définir l'objet exact. Exemple : deux identités `a_1,a_2` au même point `a` et une identité `b` au point `b`. Le simplexe étiqueté `{a_1,a_2,b}` apparaît au rayon `|ab|/2`, mais sa miniboule porte plusieurs sommets du simplexe sur le même point de frontière. La taxonomie « support affine positif de taille 2 à 4 + points strictement intérieurs » ne code pas cette multiplicité de shell. En outre, `expected_pair_mass` retire les paires copositionnelles.

Recommandation immédiate : le chemin exact doit soit refuser les positions dupliquées avec `unsupported_degeneracy`, soit définir et prouver une version pondérée/étiquetée de HGP incluant les multiplicités de shell. Le refus est nettement plus simple pour la première version exacte.

Autre détail d'API : `build_cloud_index(const vector<P3>&)` pose actuellement `PointId = index d'entrée`. Avant l'owner canonique et les `SupportKey`, il faut accepter explicitement des enregistrements `{PointId, position}` et vérifier l'unicité des identités. Sinon l'identité dite « stable » dépend encore de l'ordre physique fourni par l'appelant.

## 2. Réponse Q2 : borne Poisson sur le mou

Pour une ancre ponctuelle fixée `(a,b)`, soit `W_q(a,b)` le fuseau exact, `L(a,b) ⊆ W_q(a,b)` une région certifiée par le préfiltre, et `h = h_q`. Sous un processus de Poisson homogène d'intensité `λ`, les nombres de témoins dans ces régions suivent des lois de Poisson de paramètres `μ_W = λ |W_q|` et `μ_L = λ |L|`. En notant

`F_h(μ) = exp(-μ) Σ_{j=0}^{h-1} μ^j/j!`,

on obtient exactement, pour cette géométrie fixée :

- probabilité que l'ancre survive au filtre : `F_h(μ_L)` ;
- probabilité qu'elle soit réellement vivante : `F_h(μ_W)` ;
- probabilité de faux survivant, sans aucune fausse mort : `F_h(μ_L) - F_h(μ_W)` ;
- facteur de mou : `F_h(μ_L) / F_h(μ_W)`.

Pour une fenêtre d'observation `Ω`, Campbell-Mecke donne la forme intégrée

`E[N_surv] = λ²/2 ∫_{Ω×Ω} F_h(λ|L(a,b)|) da db`,

et la même expression avec `W_q` pour le vrai vivant.

Il existe en outre un théorème asymptotique global simple, que je **reçois** du contre-audit v3. Si `|W_q(a,b)|=v_q |a-b|³` dans le régime homogène sans bord, alors le nombre `V_h` de paires réellement vivantes vérifie

`E[V_h]/E[n] -> 2πh/(3v_q)`.

En effet, en coordonnées radiales autour du premier point, Campbell-Mecke donne un facteur `4πr²`; avec `t=λv_qr³`, on utilise `r²dr=dt/(3λv_q)` et

`∫_0^∞ F_h(t) dt = Σ_{j=0}^{h-1} Γ(j+1)/j! = h`.

Pour `h_2/h_3/h_4=10/9/8`, cela donne respectivement environ `40`, `123,796` et `139,070` paires vivantes par point. Les anciens « oracles Poisson » sont donc des constantes théoriques sous homogénéité, pas de simples ajustements empiriques. Appliquée au cœur ponctuel de coefficient volumique `c_q`, la même preuve donne un facteur de mou global `v_q/c_q`, soit environ `1`, `1,511` et `1,659` pour q2/q3/q4.

La formule conditionnelle est rigoureuse pour une région `L` déterministe une fois les extrémités fixées. Une cellule WSPD construite avec le même nuage est corrélée au processus ; pour transformer le calcul bloc par bloc en théorème sur la WSPD aléatoire, il faut soit un argument de stabilisation, soit un arbre pilote indépendant. À défaut, cette dernière étape reste un modèle quantitatif, pas un oracle déguisé en lemme.

Pour une ancre de longueur `D`, les volumes normalisés sont :

| lane | `|W_q|/D³` | volume de la boule-cœur ponctuelle `/D³` | fraction volumique |
|---|---:|---:|---:|
| q2 | 0,523599 | 0,523599 | 1 |
| q3 | 0,152263 | 0,100767 | 0,66179 |
| q4 | 0,120480 | 0,072624 | 0,60278 |

Cela explique deux faits observés : le cœur seul peut être presque parfait à forte intensité, grâce à la queue de Poisson, mais il possède un mou irréductible en q3/q4 ; `h_a/h_b` ne sont donc pas un luxe décoratif. Pour des blocs WSPD, `R_dec,q = κ_q(d-r)-r/2` converge vers le rayon ponctuel à perte relative `O(1/s)`. Le mou dû aux boîtes décroît donc en `O(1/s)`, tandis que le mou dû à la différence « boule-cœur contre fuseau » subsiste.

## 3. Réponse Q3 : complétude de la source WSPD

### Ce qui est reçu

Chaque paire non ordonnée de positions distinctes possède un unique plus petit ancêtre commun dans l'arbre radix. Elle apparaît donc dans une unique graine `(left(v),right(v))`. Chaque scission partitionne exactement le rectangle parent. Le ledger n'est pas la preuve de ce fait, mais il en est une bonne porte contre les fautes d'implémentation.

Soit maintenant `S` un support positif peu profond d'arête owner `(a,b)`. Tout point de `W_q(a,b)` est intérieur à toutes les miniboules admissibles de cette ancre, donc en particulier à la miniboule de `S`. Si `depth(B_S) ≤ h_q-1`, alors `|X ∩ W_q(a,b)| ≤ h_q-1`. Une paire de nœuds créditée par `h_q` témoins universels ne peut donc contenir l'owner d'aucun événement utile. La mort d'un bloc est fail-open et tout owner utile atteint un rectangle terminal vivant.

Cette partie de Q3 est donc **reçue**, sous le contrat « sites distincts ».

### Ce qui reste conditionnel

La complétude de bout en bout ne sera établie qu'après implémentation et audit de l'instruction terminale :

- q3 : énumération complète des porteurs dans `lentille(ab) \ boule_diamétrale`, owner canonique, positivité et census ;
- q4 : au moins un préfixe aigu, complétion axiale extrémale, positivité à quatre poids et census ;
- déduplication par `BallKey`, plateaux et construction des forêts.

Le préfiltre est donc une source exacte **de paires owner survivantes**, pas encore une source complète d'événements HGP.

### Statut de la borne `O(s³n)`

L'exactitude de la partition est indépendante de la borne de taille. En revanche, la preuve de `O(s³n)` n'est pas encore entièrement raccordée au code actuel. `cell_of_prefix` arrondit le préfixe binaire au cube d'octree de niveau `floor(used/3)`, tandis que la vague choisit le facteur à scinder avec la boîte serrée `tlo/thi`. L'argument antérieur « aspect borné, donc facteur constant » ne suffit pas à lui seul, car le choix de la branche scindée est lui aussi modifié.

Deux routes propres :

1. utiliser la cellule de préfixe exacte, de rapport d'aspect 1, 2 ou 4, et piloter la scission par son diamètre, ce qui permet un raccord direct à une preuve de type compressed-quadtree/fair-split ;
2. conserver les boîtes serrées et écrire un lemme de charging spécifique à l'arbre Morton, avec un nombre borné de stagnations par échelle sous le profil u16.

Les mesures ne contredisent pas Callahan-Kosaraju, mais elles ne remplacent pas ce raccord.

## 4. Réponse Q4 : convention exacte pour `F_K`

Pour reproduire le § 9.1 du manuscrit, `F_K` doit contenir **toutes les facettes distinctes de chaque K-simplexe de Gabriel émis**, pas seulement les facettes actives.

Les facettes actives suffisent pour déterminer les fusions entre composantes déjà nées, comme le montrent le Théorème 4 et la Proposition 6. Les autres facettes naissent au niveau même de l'événement et ne créent pas une fusion supplémentaire entre anciens polyèdres. Elles existent néanmoins dans le K-graphe de Gabriel et contribuent aux quantités `S_τ`, `T_x`, `m_τ` et au vote final. Les supprimer change donc le rendu pondéré, même si l'union des points des composantes reste inchangée.

Je conseille de séparer explicitement :

- `F_K^conn` : facettes et arêtes nécessaires au chemin de connexion minimal ;
- `F_K^render` : toutes les facettes des événements, dédupliquées, avec leurs contributions `S_τ`.

Une facette non active peut être attachée à une facette active au même niveau, sans créer de nouvelle fusion inter-composantes. Une variante « active-only » peut exister comme heuristique nommée, mais ne doit pas être présentée comme le rendu exact du § 9.1.

Trois contrats supplémentaires ressortent de la chaîne d'autorité :

- le niveau public est un **rayon au carré** sous forme exacte. En q2, il vaut `||a-b||²/4`; en q3/q4, il faut publier la fraction rationnelle canonique du rayon carré. Une variable interne nommée `ρ` peut rester un rayon, mais elle ne doit pas contaminer `ExactLevel`;
- un événement critique est sémantiquement une **hyperarête/multifusion** sur tous ses bras. Un chemin ou un arbre couvrant est une compression de connectivité, pas une autorisation de binariser la chronologie du plateau;
- la sortie complète conserve les applications verticales entre les ordres. Dix forêts indépendantes ne suffisent donc pas à représenter seules la tour ordre-échelle.

## 5. Réponse Q5 : ex æquo et cosphéricités

Il faut distinguer trois phénomènes que le mot « ex æquo » mélange trop facilement :

1. **Deux BallKeys distinctes de même rayon.** Ce n'est pas une dégénérescence. Il faut traiter toutes les arêtes du plateau simultanément, conserver l'incidence de l'hyperévénement, puis contracter les nœuds de durée nulle. Un ordre total déterministe dans Kruskal est acceptable pour choisir une forêt de calcul, mais pas pour inventer une chronologie stricte à l'intérieur du plateau.
2. **Une BallKey avec des points supplémentaires sur le shell.** La position générale échoue et la bijection Q1 n'est plus directement applicable. Tant qu'un quotient local complet n'est pas prouvé, il faut refuser transactionnellement cette BallKey avec les identités témoins.
3. **Des positions dupliquées.** C'est une dégénérescence structurelle distincte, à traiter comme indiqué en Q1.

Le tie-break `EdgeKey` entre arêtes de même longueur à l'intérieur d'un support régulier est, lui, parfaitement sain : il choisit un owner sans modifier la géométrie.

## 6. Réponse Q6 : dérivation de `W_3`, `W_4` et des rayons cœur

Posons `d=b-a`, `D=|d|`, `m=(a+b)/2`, `p=D/2` et `s=z-m`. Toute sphère passant par `a,b` a un centre `m+t`, avec `t ⟂ d`, et un rayon `R(t)=sqrt(p²+|t|²)`.

Pour une ancre owner maximale, la fermeture du domaine des centres admissibles est :

- en q3, le disque `|t| ≤ p/sqrt(3)` ;
- en q4, le disque `|t| ≤ p/sqrt(2)`.

Ces bornes sont celles de Jung, et elles sont atteintes. Le point `t=0` correspond à la limite dégénérée d'arité inférieure, mais l'intersection des boules ouvertes ne change pas lorsqu'on prend la fermeture. Le disque n'est pas seulement une relaxation : en q3, tout `t ≠ 0` se réalise en construisant le troisième sommet dans le plan engendré par `d,t`; en q4, pour `t ≠ 0`, choisir un vecteur unitaire `e ⟂ d,t` et les deux sommets `x=m+2t+pe`, `y=m+2t-pe` donne un tétraèdre bien centré, de centre `m+t`, dont toutes les arêtes sont au plus `D` dès que `|t|≤p/sqrt(2)`.

La condition `z ∈ int(B(m+t,R(t)))` s'écrit

`p²-|s|²+2s·t > 0`.

En minimisant sur le disque des centres, avec `r=|proj_{d⊥}(s)|`, on obtient `H-2T_q r>0`, où `H=p²-|s|²`, `T_3=p/sqrt(3)` et `T_4=p/sqrt(2)`. Comme `Xi=D²r²=4p²r²`, cela donne exactement :

- `W_2 : H>0` ;
- `W_3 : H>0 et 3H²>Xi` ;
- `W_4 : H>0 et 2H²>Xi`.

La plus grande boule centrée en `m` incluse dans toutes ces sphères a pour rayon `min_{|t|≤T_q}(R(t)-|t|)`, minimum atteint au bord. On retrouve :

- `κ_2=1/2` ;
- `κ_3=1/(2sqrt(3))=1/sqrt(12)` ;
- `κ_4=(sqrt(3)-1)/(2sqrt(2))=sin(15°)`.

### Correction obligatoire de `1f1ae0c`

Avec `u=2z-a-b`, `U=u·u` et `L=D²`, les tests exacts de la boule-cœur ponctuelle sont :

- q2 : `U<L` ;
- q3 : `3U<L` ;
- q4 : poser `Y=2L-U`, puis tester `Y>0` et `Y²>3L²`.

Ainsi :

- `D/sqrt(12)` et `D/(2sqrt(3))` sont **le même rayon q3** ;
- `15U<4L`, soit le rayon `D/sqrt(15)`, est une sous-approximation sûre mais légèrement stricte du cœur q4 exact, car `4/15 < 2-sqrt(3)`.

Fixture discriminante q4 à graver :

`a=(10000,10000,0), b=(20000,10000,0), z=(15000,12585,0)`.

Elle appartient au cœur exact q4, donc satisfait `2H²>Xi`, mais elle échoue à `15U<4L`. Le code actuel avec `kA3/kA4` reste fail-open : ses constantes sont volontairement sous-approchées. C'est le commentaire mathématique, non la sûreté du code, qui doit être corrigé.

La robustification découplée par boîtes est également validée : le milieu réel se déplace d'au plus `(r_A+r_B)/2` et la longueur réelle est au moins `d-r_A-r_B`; d'où

`B(m_0, κ_q(d-r)-r/2) ⊆ B(m_ab, κ_q|ab|) ⊆ W_q(a,b)`.

La borne couplée ajoutée comme prochaine étape par `30e6ccc` est elle aussi sûre :

`R_coup,q = κ_q d - sqrt((4κ_q²+1)(r_A²+r_B²)/2)`.

Preuve courte. Écrivons `a=c_A+u`, `b=c_B+v`, `p=(u+v)/2` pour le déplacement du milieu et `w=(v-u)/2` pour l'erreur de demi-arête. Une boule centrée au milieu nominal de rayon

`κ_q d - (2κ_q|w|+|p|)`

est incluse dans le cœur réel. Par Cauchy puis l'identité du parallélogramme,

`2κ_q|w|+|p| ≤ sqrt((4κ_q²+1)(|w|²+|p|²))`
`≤ sqrt((4κ_q²+1)(r_A²+r_B²)/2)`.

Il est donc sûr de prendre `max(0,R_dec,q,R_coup,q)`. Pour l'implémentation entière, il faut minorer le terme `κ_q d`, majorer le terme d'érosion et arrondir la racine vers le haut; aucun `double` ne doit décider. Le gain « +71 % de rayon à s=6 en q4 » reste une mesure v3 à reproduire, pas une conséquence du théorème.

## 7. Réponse Q7 : preuve de l'autorité 64 coins

Pour un témoin ponctuel `z`, posons `u=z-a` et `v=b-z`. Alors `H=u·v` et `Xi=|u×v|²`. Les trois lanes équivalent à une contrainte angulaire :

- q2 : `angle(u,v)<90°` ;
- q3 : `angle(u,v)<60°` ;
- q4 : `angle(u,v)<acos(1/sqrt(3))`.

Pour `v` fixé, l'ensemble des `u` satisfaisant la contrainte est un cône circulaire ouvert convexe. La relation est symétrique en `u,v`.

Si les 8×8 couples de sommets de `z-A` et `B-z` satisfont la lane, alors, pour chaque sommet `v_j`, la convexité donne la propriété pour tout `u` de la boîte. Fixons ensuite un tel `u`; par symétrie, tous les sommets `v_j` sont dans le cône convexe de `u`, donc toute la boîte `V` y est. La réciproque est triviale.

Par conséquent, `corner64_universal` est **exact pour l'enveloppe AABB continue dans le sens ALL, et même équivalent**, y compris pour les boîtes plates après suppression des coins dupliqués. Cette preuve par cônes est préférable à une combinaison séparée de `Hmin` et `Ximax`, dont les extrema peuvent être atteints en des coins incompatibles.

Q7 est donc reçue. Le juge reste utile contre une faute de programmation, mais il ne porte plus la charge du théorème.

## 8. Réponse Q8 : tuer à tous les niveaux et vraie fusion des lanes

La phrase v3 « un certificat évalué à chaque nœud ne peut pas économiser plus de visites qu'il n'en coûte » n'est pas un théorème. Une mort interne évite tout le sous-arbre de rectangles descendants ; le gain apparié de 1,37× est donc parfaitement plausible et justifie la stratégie.

Il faut toutefois nommer correctement l'état actuel : la vague `(A,B)` est partagée, mais chaque lane appelle séparément `count_universal_witnesses`, qui redescend depuis la racine de l'arbre des témoins. Au terminal, la boule-cœur puis l'autorité complète redémarrent encore la descente. Les évaluations `(H,Xi)` ne sont donc pas encore mutualisées.

Étape d'implémentation recommandée :

1. écrire `count_universal_witnesses_234(A,B)` avec une seule pile de nœuds témoins, un masque de lanes et trois compteurs ;
2. sur un nœud témoin, appliquer une fois l'élagage commun `Hmax`, créditer q2 par `Hmin`, créditer q3/q4 par leurs boules-cœur, puis à une feuille calculer un unique masque `corner64` à partir des mêmes `(H,Xi)` ;
3. ne jamais recommencer le compte à zéro, puisqu'une seule traversée attribue chaque sous-arbre une seule fois ;
4. intégrer `max(R_dec,R_coup)` dans ce parcours fusionné, avec arrondis dirigés;
5. ensuite seulement, transporter entre parent et enfants `(A,B)` une frontière de nœuds témoins avec crédits hérités, afin de ne pas relancer depuis la racine.

Les compteurs utiles sont `witness_nodes_visited`, `corner_pairs_evaluated`, `AB_children_avoided`, `alive_rects_q2/q3/q4` et `alive_rects_union`. Le reçu actuel ne publie que l'union `rect_vivants`, insuffisante pour dimensionner les consommateurs de chaque lane.

## 9. Contre-audit des affirmations héritées

### Affirmations confirmées

- un cap de masse dans le critère terminal force un catalogue quadratique ;
- la scission doit être géométrique, pas pilotée par la population ;
- `Hmin` est exact et la borne minimax `Hmax` est sûre pour élaguer ;
- les trois comptes `h_coeur`, `h_a`, `h_b` sont disjoints ;
- le masque de lanes est nécessaire ;
- `corner64` est une autorité ALL exacte;
- la constante Campbell-Mecke `2πh/(3v_q)` et la borne couplée `R_coup` sont correctes.

### Affirmations à rectifier

- le « facteur borné `8^d` » ne constitue pas encore une preuve complète pour la récursion actuelle sur boîtes serrées ;
- les rayons q3 prétendument distincts dans `1f1ae0c` sont identiques ;
- `D/sqrt(15)` est une sous-approximation q4, pas une autre famille géométrique ;
- « une seule vague, une seule évaluation `(H,Xi)` » décrit la cible, pas encore le code ;
- le commentaire CMake sur `two_lines` confond porteurs et témoins : un point collinéaire entre `a,b` satisfait aussi W3 et W4 puisque `Xi=0` et `H>0`. La porte reste sûre, mais son commentaire doit être corrigé;
- la « dominance directionnelle 432 » est une piste expérimentale prometteuse, pas encore un théorème de coût ni une autorité reçue;
- la boule-cœur issue de la sphère circonscrite à l'AABB est géométriquement un sous-certificat de `corner64`; elle reste utile parce qu'elle crédite des sous-arbres témoins entiers à bien moindre coût, non parce qu'elle serait une autorité plus forte.

## 10. Portes prioritaires à ajouter

1. **Rayon q4 exact** : graver la fixture ci-dessus. Elle doit être reconnue par le prédicat exact; `15U<4L` reste une baseline conservatrice, pas un mutant dangereux.
2. **`R_coup`** : fixtures déséquilibrées et équilibrées vérifiant `max(R_dec,R_coup)` contre toutes les paires ponctuelles du petit bloc, avec coquille ouverte.
3. **Corner64** : petites boîtes entières exhaustives q2/q3/q4, comparées à tous les couples de points de grille contenus dans les boîtes.
4. **Juge exhaustif borné** : sur petits nuages, tester toutes les paires de chaque bloc mort, pas un échantillon.
5. **Doublons** : fixture qui vérifie le refus explicite du profil exact, jusqu'à preuve d'une future sémantique pondérée.
6. **Plateaux et multifusion** : deux BallKeys distinctes de même niveau doivent être traitées simultanément, sans nœud artificiel de durée positive et avec l'hyperincidence complète.
7. **Identités externes** : permutation des enregistrements conservant les mêmes `PointId` et exactement les mêmes owners/SupportKeys.
8. **Contrat public** : `ExactLevel` au carré, `s_max` effectif sur `n<11`, et une petite tour vérifiant les applications verticales.

Durcissements simples : vérifier `smax ≥ q` avant `lane_h`, caster avant `q*q` et `k*k` dans le prédicat WSPD, et affirmer en debug que les plages `A,B` sont disjointes avant les soustractions de `credit_weight`.

## 11. Réception commit par commit

- `f775c98` : **reçu comme socle d'exploration**, avec réserve sur les doublons, les IDs externes et la preuve de taille WSPD.
- `7bd3281` : **reçu mathématiquement** pour la descente q2 et ses arrondis fail-open.
- `e535af2` : **reçu pour la sûreté q2/q3/q4**; Q6 et Q7 sont désormais prouvées ci-dessus. La fusion des traversées témoins reste à faire.
- `f3c5105` : **reçu empiriquement**. Les chiffres sont convaincants pour choisir la prochaine étape, mais ne qualifient ni le SLO ni une absence universelle de fausse mort.
- `1f1ae0c` : **reçu sous réserve de correction documentaire**. La dérivation de `κ_3,κ_4` est correcte; l'interprétation de `D/sqrt(12)` et `D/sqrt(15)` ne l'est pas.
- `2437254` : **reçu comme corpus de lecture**. Il améliore la traçabilité mais ne promeut aucun résultat v3 au rang de preuve v4.
- `30e6ccc` : **`R_coup` reçu mathématiquement**; la dominance directionnelle 432 reste à requalifier par une définition, un ledger et des mesures appariées.
- `bebdef2` : **reçu comme rappel de la chaîne d'autorité**. Il confirme notamment sites distincts, niveaux publics au carré, multifusions et applications verticales; aucun changement de code n'est à recevoir dans ce commit.

## Ordre de travail conseillé à Claude

1. corriger les rayons dans `MATHEMATIQUES.md` et graver les contrats `ExactLevel`, `K_eff/s_max` et multifusion;
2. décider le refus des doublons et passer à des `PointId` externes avant toute émission de `SupportKey`;
3. fusionner réellement la descente témoins q2/q3/q4, puis intégrer `max(R_dec,R_coup)`;
4. publier les résiduels par lane; seulement ensuite arbitrer `h_a/h_b` et la dominance 432 sur le coût marginal réel;
5. implémenter l'instruction q3 terminale, puis la q4 axiale, avec `BallKey/RLE/census` exact;
6. séparer `F_K^conn` et `F_K^render`, conserver les hyperévénements et raccorder les applications verticales;
7. raccorder une preuve de taille à la WSPD actuelle ou rapprocher le code d'une décomposition dont la preuve est standard.

La v4 est partie sur une base nettement plus saine que la v3 : les statuts sont prudents, les leçons négatives ont été réellement incorporées, et le premier verrou, tuer les ancres mortes avant de développer la WSPD, est levé de façon mathématiquement crédible. Le bon cap est maintenant de préserver exactement l'objet topologique pendant que l'implémentation devient réellement sparse.
