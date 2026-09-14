# q3/q4 : objets, preuves et stratégie proposée

14 septembre 2026. Proposition d'architecture, non implémentée ni mesurée. Cadre : phase=exploration_v8_hors_registre, backend=cpu_reference, profile=quantized_u16_input_only, mode=implementation_v8_p0, public_status=not_claimed. Aucun résultat FULL, GPU ou contrat G4 ici.

## 1. Décision proposée

Conserver des **familles de boules** le plus longtemps possible, plutôt qu'une liste de tétraèdres à tester indépendamment :

1. Le front produit des rectangles avec des masques q2/q3/q4 indépendants.
2. Pour q3/q4, une arête propriétaire et un bloc de troisièmes sommets décrivent implicitement des triangles de départ, appelés seeds.
3. Les formes exactes des seeds q3 peuvent être groupées pour partager un parcours de témoins. Le bénéfice de ce regroupement reste à mesurer.
4. Pour q4, chaque seed définit une famille à un paramètre : les témoins produisent des événements d'entrée/sortie. Leur tri exact puis des sommes segmentées donnent la profondeur de toutes les complétions.
5. Les boules retenues sont identifiées exactement, puis leurs intérieurs et leurs coquilles complètes sont collectés. Le catalogue obtenu n'est pas encore la reconstruction FULL.

Deux interdictions structurantes : **ne pas alimenter q3/q4 seulement avec les arêtes acceptées par q2 ; ne pas alimenter q4 seulement avec les seeds acceptés par le census q3.** Les contre-exemples du §3 sont entiers, petits et indépendants des choix de parallélisation.

Cette proposition reprend des identités mathématiques auditées, pas l'implémentation ni la qualification v7. Sources d'entrée : [WSPD q2/q3/q4](../audits/WSPD_Q2_Q3_Q4.md), [fondements et objet](../audits/FONDEMENTS_ET_OBJET.md), [algorithme expliqué](ALGORITHME_EXPLIQUE.md), et [lecture du manuscrit et contrats v7](../../morsehgp3D_v7/docs/LECTURE_ET_CONTRATS.md).

## 2. Quel objet faut-il conserver ?

La cible est la tour HGP fondée sur les ensembles de centres, pas une suite de graphes k-NN ordinaires. Les composantes vivent naturellement sur les facettes ; leur projection sur les points peut se recouvrir. Le catalogue exhaustif des niveaux Gamma n'est pas nécessaire, mais les incidences silencieuses qui déterminent les parents ne peuvent pas être supprimées sans certificat de remplacement.

Pour une boule, noter p le nombre de sites strictement intérieurs, U sa coquille complète et q_min la cardinalité minimale d'un support positif qui détermine cette boule. La fenêtre utile à la tour satisfait $p+q_{\min}\leq K_{\max}+1$. Les seuils des trois voies sont $h_q=K_{\max}+2-q$, donc $h_2=K_{\max}$, $h_3=K_{\max}-1$ et $h_4=K_{\max}-2$. La voie q3 est utile dès Kmax=2, q4 dès Kmax=3.

Une présentation positive de cardinal q peut avoir q supérieur au q_min global de la boule : d'autres sites de sa coquille peuvent fournir un support plus petit. Un rejet à h_q élimine donc cette voie, pas nécessairement la boule. La voie minimale doit la conserver. La fusion par clé exacte de boule garde le plus petit q représenté ; elle ne fusionne pas simplement des rayons égaux.

Un représentant d'arité minimale, avec tous les intérieurs et toute la coquille, peut suffire au **catalogue de boules** selon la preuve géométrique [S1, §6](../../morsehgp3D_v7/audits/S1_COURANT.md). Cela n'autorise pas à changer silencieusement un contrat de sortie qui demande toutes les incidences de supports, comme le flux q2 actuel. Déclarer séparément les deux interfaces et leurs oracles.

La coquille n'est pas bornée par K. La suite FULL doit encore résoudre parents, multifusions, plateaux et ancrages verticaux ; voir [verrous mathématiques, §§1–3](../audits/VERROUS_MATHEMATIQUES_20260914.md) et [tour par boules v7](../../morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md).

## 3. Contre-fixtures : les voies ne sont pas des filtres successifs

Convention dans cette note : la puissance vaut $\Pi_B(z)=|z-c_B|^2-R_B^2$ ; elle est négative à l'intérieur. Les certificats d'arête utilisent au contraire $H_{ab}(z)=(z-a)\cdot(b-z)$, positif dans la boule diamétrale.

### 3.1 q2 rejette le propriétaire, q3 et q4 doivent survivre

Prendre a=(900,1000,1000), b=(1100,1000,1000), puis $z_j=(1000+j,910,1000)$ pour $j=0,\ldots,K-1$. Pour 2≤K≤10, $H_{ab}(z_j)=1900-j^2>0$ : la boule diamétrale possède K intérieurs, donc le census q2 rejette ab.

Pour q3, ajouter c=(1000,1120,1000). Sa circumboule a pour centre $(1000,3055/3,1000)$ et rayon carré $93025/9$. Les poids barycentriques du centre sont $(61/144,61/144,11/72)$, tous strictement positifs. L'arête ab est l'unique plus longue : son carré vaut 40000, contre 24400 pour les deux autres. Or $\Pi_{abc}(z_j)=1400+j^2>0$. Le triangle est donc un support minimal positif de profondeur zéro ; sa coquille est exactement {a,b,c}. Il doit survivre en q3.

Pour q4, remplacer c par c+=(1000,1120,1040) et d=(1000,1120,960). La circumboule a pour centre (1000,1025,1000), rayon carré 10625 et poids $(19/48,19/48,5/48,5/48)$. Le déterminant vaut −1920000, donc le tétraèdre n'est pas plat. Ab reste l'unique plus longue arête : 40000 contre 26000 pour les quatre arêtes latérales et 6400 pour c+d. Pour 3≤K≤10, $\Pi_{abc_+d}(z_j)=2600+j^2>0$. Ce support q4 a lui aussi profondeur zéro et coquille exactement égale à ses quatre sommets.

Les masques indépendants du front préservent correctement cet exemple : $\Xi_{ab}(z_j)=|(b-a)\times(z_j-a)|^2=324000000$, alors que $3H_{ab}(z_j)^2\leq10830000$ et $2H_{ab}(z_j)^2\leq7220000$. Ces sites satisfont W2, mais pas W3 ni W4.

Cette fixture réfute directement un accès par les seuls propriétaires acceptés en q2, ainsi qu'une énumération par cliques du graphe q2 accepté. Elle ne prouve pas l'impossibilité de toute autre construction qui réutiliserait certaines arêtes survivantes avec une nouvelle preuve d'accès et de propriété.

### 3.2 q3 rejette le seed canonique d'un q4 admissible

Reprendre le tétraèdre précédent et ajouter $w_j=(1000+j,1020,1105)$ pour $j=0,\ldots,K-2$. La boule de la face abc+ a pour centre $(1000,2045/2,2015/2)$ et rayon carré $21125/2$. On obtient $\Pi_{abc_+}(w_j)=-1050+j^2<0$ et $\Pi_{abc_+d}(w_j)=425+j^2>0$.

Le site d est extérieur à la boule de face, avec puissance 1200 ; les z_j éventuels le sont aussi, avec puissance $2150+j^2$. Le census q3 de cette face atteint donc K−1 et la rejette, tandis que le tétraèdre garde profondeur zéro. Attribuer à c+ un ID original plus petit qu'à d en fait le seed aigu canonique sur ab. L'autre face ne fournit pas une réparation automatique : cela changerait la règle d'émission unique.

Ces identités sont maintenant des [fixtures permanentes indépendantes](../tests/q3_q4_owner_independence_gate.py), vérifiées avec des fractions exactes pour Kmax=5 et 10 : quatre cas de propriétaire rejeté en q2 et deux cas de seed rejeté en q3. Elles ne rapportent pas un passage du futur produit q3/q4. Le test emploie rayon²−distance² dans les quatre premiers cas, et distance²−rayon² dans les deux derniers ; les conventions sont explicites dans ses champs.

## 4. Accès canonique : arête, lentille et cover sont trois choses différentes

Le propriétaire d'un support positif est son arête de longueur maximale, départagée par les IDs originaux des extrémités. Ne jamais employer les rangs de permutation spatiale comme IDs.

Pour une arête ab de longueur D, les sommets de complétion appartiennent à la lentille $|x-a|^2\leq D^2$ et $|x-b|^2\leq D^2$. Les tests exacts d'acuité, de propriété et de rang restent nécessaires. En q4, au moins l'une des deux faces incidentes à une arête maximale d'un tétraèdre positif est aiguë. Le plus petit ID original parmi ces seeds admissibles fixe une présentation canonique. La preuve figure dans [S1, §1](../../morsehgp3D_v7/audits/S1_COURANT.md).

Les témoins de profondeur, eux, ne sont pas limités à cette lentille. Pour m=(a+b)/2, la borne de variance donne $R^2\leq (q-1)D^2/(2q)$. Toute la boule fermée est couverte par $4|z-m|^2\leq3D^2$ en q3 et par $4|z-m|^2\leq4D^2$ en q4 ; voir [S1, §2](../../morsehgp3D_v7/audits/S1_COURANT.md). La seconde est une surcouverture rationnelle commode de la borne plus fine $(2+\sqrt{3})D^2$.

Le cover doit être une collection de nœuds disjoints de l'index global, ou un parcours équivalent, contenant tous les sites pouvant être intérieurs ou sur la coquille. Un site rejeté comme complétion doit encore produire son événement ou contribuer au census s'il appartient au cover. Les tests de cover sont fermés : une égalité ne peut pas éliminer une coquille.

L'ancien [edge_cover.hpp](../../morsehgp3D_v7/src/lanes/edge_cover.hpp) illustre cette séparation ; sa représentation et ses coûts restent des objets d'audit, pas une base à copier implicitement.

## 5. q3 : arête × bloc de complétions, puis lot de formes × témoins

### 5.1 Générer sans matérialiser toutes les combinaisons

Une tâche de complétion porte l'arête ab, un nœud X de l'index et les voies encore possibles. Des bornes nécessaires de lentille, d'acuité et de propriété peuvent éliminer X entier. Sinon le bloc est subdivisé ; un singleton passe les prédicats exacts. Tant qu'un filtre porte encore sur un produit d'arêtes A×B, conserver ce produit au lieu de développer toutes ses paires.

Les minorants W3/W4 et les crédits locaux sont des filtres de familles. Un Pool q2 ne devient pas un Pool q3 en changeant seulement K : ses certificats doivent satisfaire la géométrie W3, puis W4. Les preuves d'addition exigent des identités de témoins disjointes. L'échec d'un certificat n'est jamais un motif de troncature.

### 5.2 Partager le census de plusieurs vraies formes

Pour un seed abx, poser d=b−a, u=x−a, D=d·d, E=u·u, F=d·u, $G=DE-F^2>0$ et $W=E(D-F)d+D(E-F)u$. La puissance entière de sa boule est $P_x(z)=G|z-a|^2-W\cdot(z-a)$. La formule est auditée dans [q3.hpp](../../morsehgp3D_v7/src/lanes/q3.hpp) et [l'arithmétique des voies](../../morsehgp3D_v7/audits/ARITHMETIQUE_LANES_COURANTE.md).

Proposition minimale à comparer au parcours individuel : regrouper des **formes déjà valides**, d'abord à même arête ab, et préparer une enveloppe de leurs coefficients G/W. Un calcul par intervalles dirigé vers l'extérieur peut alors borner P pour un lot de seeds et une boîte Z. Ce n'est pas une nouvelle formule d'extrema exacts ; une enveloppe trop large provoque seulement une subdivision.

L'enveloppe se prépare une fois par lot, pas par rencontre avec Z. Ne pas invoquer la convexité du certificat W3 en a/b pour étendre sans preuve le test aux x variables : les coefficients du seed dépendent non linéairement de x. La validité et les bornes de largeur arithmétique doivent être démontrées sur les formes effectivement admises.

Invariant du partage : même ordre global des témoins, même préfixe consommé et même compte exact sur ce préfixe pour toutes les formes du lot. Si max P<0, ajouter |Z| puis avancer ; si min P≥0, avancer sans crédit ; sinon subdiviser le lot ou Z et transmettre compte et curseur ensemble. Un compte atteignant h3 rejette le lot.

Aucun crédit W3 d'amont ne précharge ce compte. Aucun ensemble entier de troisièmes sommets X n'est exclu : un site qui est sommet d'un seed peut être strictement intérieur à la boule d'un autre seed. La collecte ultérieure repart avec son propre contrat ; pour elle, min P=0 ne permet pas de supprimer Z, car il peut contenir la coquille.

Ce partage peut échouer exactement comme le partage q2 : coefficients trop dispersés, blocs de témoins mêlant intérieur et extérieur, coût de préparation supérieur à l'économie. Garder un relais individuel sans reconstruire l'index et mesurer le taux de décisions collectives.

## 6. q4 : événements de demi-droites et balayage segmenté

### 6.1 Une seed, une famille affine

Pour une face aiguë abx, soit n=(b−a)×(x−a), G=|n|², c0 son centre et R0 son rayon. Les centres équidistants de a,b,x sont $c_\mu=c_0+\mu n/(2G)$, et $R_\mu^2=R_0^2+\mu^2/(4G)$. Pour un site z, poser $P(z)=G(|z-c_0|^2-R_0^2)$ et $B_z=n\cdot(z-a)$. L'intérieur strict est exactement $P(z)-\mu B_z<0$.

La condition nécessaire sur le rayon des q4 positifs borne une corde fermée : $2\mu^2\leq J$, où $J=D^2(3G-2|x-a|^2|x-b|^2)>0$ et D²=|b−a|². Une racine rationnelle r=P/B appartient à la corde si $2P^2\leq JB^2$. Ces identités viennent de [S1, §3](../../morsehgp3D_v7/audits/S1_COURANT.md) et de la [preuve de corde](../../morsehgp3D_v7/audits/PREUVE_CHORD_SECTOR_COURANTE.md).

| Cas du témoin | Contribution sur la famille |
| --- | --- |
| B>0 | Entrée : intérieur pour μ>P/B |
| B<0 | Sortie : intérieur pour μ<P/B |
| B=0, P<0 | Intérieur constant |
| B=0, P=0 | Coquille constante, jamais intérieur |
| B=0, P>0 | Extérieur constant |

Une racine strictement hors corde n'est pas nécessairement jetable : le signe est constant sur la corde, donc le site peut être intérieur partout. Le classer avec un point exact de la corde, par exemple μ=0. Une racine sur une borne reste dans le segment fermé.

### 6.2 Ce qui est massivement parallélisable

Les couples seed × bloc de témoins calculent indépendamment P, B, la classe constante et les événements. Ils alimentent des segments identifiés par seed, sans recopier les coordonnées du nuage.

Après tri exact des racines de chaque seed, regrouper les racines égales. Si e_j et x_j comptent les entrées et sorties du groupe j, et c0 les intérieurs constants, sa profondeur stricte vaut $p_j=c_0+\sum_{i<j}e_i+\sum_{i>j}x_i$. Deux sommes préfixes/suffixes segmentées calculent donc toutes les profondeurs ; on retire les sorties du groupe avant lecture et on ajoute ses entrées après. Un groupe coupé entre deux tuiles nécessite une fusion de frontière : la coupure mémoire ne crée pas un nouvel instant géométrique.

Le tri n'est pas nécessairement le poste dominant, mais « presque gratuit » n'est pas une preuve de coût : il faut mesurer la production des événements, leur largeur, les comparaisons, les déplacements et les volumes par seed. Les racines exactes ne sont pas des clés double ni des mots de 64 bits interchangeables. Les bornes de produits croisés larges sont à requalifier depuis les preuves v7, pas à hériter.

**Piste arithmétique précisée avec B le 14 septembre.** Le produit naïf P1·B2−P2·B1 peut dépasser i128, mais contient un facteur Gram commun qui peut être éliminé **avant** le calcul. Pour d=b−a, u=x−a, v_i=z_i−a, poser Δ le déterminant 4×4 dont les lignes sont `(d,|d|²)`, `(u,|u|²)`, `(v1,|v1|²)`, `(v2,|v2|²)`. Alors `G·Δ = P2·B1 − P1·B2`. Si B1 et B2 sont non nuls, `sign(μ1−μ2) = −sign(Δ)·sign(B1)·sign(B2)` ; Δ=0 est l'égalité exacte. Oublier le signe des dénominateurs inverserait certains groupes.

Ce déterminant peut être calculé directement en i128 : différences de coordonnées <2^16, relèvements <3·2^32, donc la somme des valeurs absolues de ses 24 termes est <72·2^80<2^87. La promotion doit précéder les produits. Les six mineurs 2×2 des deux premières lignes peuvent se préparer une fois par seed ; le développement de Laplace donne ensuite six produits entre mineurs. Cela suggère un comparateur plus compact que des produits rationnels larges. L'identité et la borne sont acquises mathématiquement ici, **pas encore implémentées ni qualifiées** dans la v8 ; les tests de corde, la positivité, les clés de boules et les niveaux ne sont pas couverts par cette seule simplification. Voir la [réponse indépendante B](../audits/DIALOGUE_AUDITEUR_B.md).

Pour vérifier l'identité, soustraire à la dernière colonne W/G fois les trois colonnes spatiales : elle devient `(0,0,P1/G,P2/G)`. Le développement suivant cette colonne donne la formule ci-dessus. Inverser l'orientation de la seed inverse aussi le paramètre : garder ensemble orientation, signes B et classification entrée/sortie.

L'ID départage l'ordre matériel d'événements de même racine ; il ne doit jamais casser leur égalité géométrique. Le balayage ne peut pas saturer aveuglément un compte à K puis lui soustraire les sorties : une profondeur élevée peut ensuite redescendre sous le seuil.

### 6.3 Validation et collecte ne disparaissent pas après le tri

Un groupe de profondeur p≥h4 peut éviter sa cascade de complétions. Il ne permet pas d'arrêter le seed entier. La [contre-fixture S1, §7](../../morsehgp3D_v7/audits/S1_COURANT.md) a une première racine de profondeur 1 puis une suivante de profondeur 0, admissible à Kmax=3. Son site Z est hors lentille de complétion, mais son événement est indispensable à cette baisse de profondeur.

Les sites de la racine candidate doivent encore passer propriété, seed canonique, rang et positivité stricte du centre du tétraèdre. Une racine n'est pas une preuve de support positif. L'ancien [generate.hpp](../../morsehgp3D_v7/src/pipeline/generate.hpp) et [q4.hpp](../../morsehgp3D_v7/src/lanes/q4.hpp) rendent ces postes visibles ; ils restent des objets d'audit.

Toutes les présentations d'une racine d'un même seed décrivent la même boule. Pour une sortie catalogue, une première présentation valide suffit localement ; une première présentation invalide ne suffit pas à rejeter le groupe. Pour un contrat de toutes les incidences, les autres présentations doivent au contraire rester accessibles.

Deux chemins de payload sont à comparer explicitement :

- Exploiter les listes d'événements : intérieurs constants, entrées strictement antérieures et sorties strictement postérieures ; coquille constante et tous les IDs du groupe courant.
- Dédupliquer d'abord les clés de boules, puis effectuer un census global exact de chaque boule retenue.

Le premier réutilise le travail déjà payé mais conserve des listes ; le second peut éviter des collectes répétées entre seeds, mais paie un nouveau parcours. Aucun des deux n'est gratuit. La seule profondeur p_j n'est pas le payload I/U nécessaire à la suite.

## 7. Objets partageables et durée de vie

| Objet | État durable minimal et responsabilité |
| --- | --- |
| Produit du front | Nœuds A/B, masque, propriétaire de l'index global |
| Bloc de complétion | Arête, nœud X, contraintes nécessaires, IDs originaux |
| Lot q3 | Références de formes validées, enveloppe préparée, seuil, ordre Z |
| Continuation de census | Lot ou forme, compte, curseur non consommé, contexte possédé |
| Segment q4 | Seed exact, corde, événements et comptes constants |
| Boule canonique | Clé exacte, q_min observé, autorité de collecte I/U |

Le front est une bonne source de travail distribué, mais pas un grain suffisant : une seule arête avec beaucoup de seeds, ou une seule seed avec beaucoup d'événements, peut devenir le dernier worker actif. Il faut pouvoir reprendre aussi ces étapes sans refaire leurs ancêtres, leurs covers ni leurs préparations.

L'index global et les préparations parentales doivent rester possédés pendant toutes les continuations. Un pointeur vers le contexte sur la pile d'un autre worker n'est pas une tâche transférable. Les buffers mutables de production et de collecte restent privés ou ont une propriété exclusive explicite. Les exceptions rejoignent les workers avant libération ; les sorties déjà émises ne deviennent pas atomiques par magie.

L'expérience actuelle [redistribution du front q2](P0_REDISTRIBUTION_FRONT_Q2.md) et les [échanges A](../audits/DIALOGUE_COURANT.md) / [B](../audits/DIALOGUE_AUDITEUR_B.md) montrent pourquoi la masse de paires n'est pas un indicateur suffisant du coût d'un job. Le dispatcher de front ne divise pas un census déjà en cours. Les temps de présence Donate incluent l'attente de fermeture globale : un rapport max/moyenne proche de 1 ne prouve pas un travail équilibré.

Une taille de tuile est une limite de résidence mémoire, pas un quota de recherche. Pour un segment q4 plus gros que la tuile, produire des runs triés et les fusionner exactement, avec groupes de racines préservés. Compter les doubles lectures éventuelles d'un schéma « compter puis remplir », les fusions et les transferts. File pleine : continuer localement ; éviter une arène où tous les producteurs attendraient de la mémoire que seuls ces mêmes workers peuvent libérer.

Enfin, un groupe de racines q4 d'une seed n'est pas le plateau global de niveaux de la tour. Des boules de seeds et de voies différentes peuvent avoir le même rayon. La reconstruction doit traiter leur niveau exact ensemble avant de publier les parents ; la clé de boule et la clé de niveau sont deux objets différents. Voir [level.hpp](../../morsehgp3D_v7/src/lanes/level.hpp) et [tour par boules](../../morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md).

## 8. Ce qui peut encore recréer le carré

Noter T les produits visités au front, F la somme des tailles des facteurs réellement lus, S3/S4 les nombres de seeds, m_t le nombre de sites parcourus dans le cover du seed t, e_t ses événements, et L le nombre d'IDs d'intérieurs/coquilles effectivement écrits.

Un q3 individuel conserve un coût potentiel de l'ordre de S3·n. Le balayage q4 retire la répétition « une complétion × tout le cover », mais conserve au moins la production des événements et le tri : $O(\sum_t m_t+\sum_t e_t\log(1+e_t))$, auxquels s'ajoutent génération des seeds, cascades, déduplication et payload. Cette expression est une comptabilité de postes, pas une borne sous-quadratique en n.

Les risques concrets à mesurer sont :

- Développer un résidu d'arêtes déjà quadratique avant le premier vrai test de complétion.
- Refaire le cover ou lire les facteurs pour chaque arête/seed ; un petit nombre de rectangles ne borne ni F ni leur masse.
- Préparer les enveloppes de lots de seeds à chaque bloc Z.
- Construire des groupes de témoins en essayant toutes les paires locales : retour dissimulé à |A|²+|B|².
- Réémettre et recollecter une même boule à chaque présentation.
- Réserver simultanément tous les événements de toutes les seeds, puis confondre mémoire temporaire et mémoire de sortie.
- Paralléliser davantage sans diminuer ces volumes.

Les certificats collectifs de [P0 sous-rectangles et groupes, §§1–6](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md) sont une piste complémentaire importante : une moyenne pondérée de puissances strictement négative prouve au moins un intérieur dans le groupe. Elle ne prouve pas que tous ses membres sont intérieurs. Les moments doivent inclure la dispersion, pas seulement le barycentre. Des groupes disjoints peuvent additionner leurs crédits ; des groupes qui se recouvrent, ou des poids différents selon les coins testés, ne le peuvent pas sans preuve nouvelle. Préparer un petit nombre de groupes hiérarchiques avec coût de sélection déclaré est une expérience ; chercher tous les groupes possibles ne l'est pas.

Aucune borne classique de taille de WSPD ne doit être transférée sans preuve à l'arbre spatial effectif ; même une borne sur le nombre de rectangles ne bornerait pas leur masse ni l'aval.

Enfin, la sortie explicite FULL peut elle-même être quadratique dans certaines familles à précision croissante ; voir [croissance et borne de sortie v7](../../morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md). Ce fait distingue coût nécessaire et gaspillage ; il ne démontre pas l'impossibilité du contrat fini à 50k en u16. Les doublements locaux ne constituent pas non plus une preuve asymptotique.

## 9. Prochaine tranche : expériences bornées et portes de réfutation

Ordre proposé, sans ouvrir simultanément tous les moteurs :

1. Formes q3/q4 et ownership exacts, jugés par un oracle rationnel indépendant sur de petits nuages ; interface distincte pour présentations et catalogue.
2. Accès arête × bloc de complétions, comparé à l'exhaustif sans filtrage préalable par les supports q2 acceptés.
3. Census q3 individuel, puis lots de formes × Z en variante : même résultat exact, travail et préparation séparés.
4. Une seed q4 scalaire, puis production par blocs et balayage segmenté : mêmes racines, groupes, profondeurs, boules et I/U.
5. Déduplication inter-seeds/inter-voies et collecte unique de boules, avant de raccorder les parents et plateaux FULL.

Les fixtures minimales incluent les deux contre-exemples du §3 ; triangle droit ou collinéaire ; tétraèdre plat ou centre sur une face ; tétraèdre régulier entier avec six longueurs égales ; même boule présentée en q2 et q3 ; coquille bien plus grande que K ; groupe de racines mêlant entrées et sorties ; racines sur les bornes de corde ; témoin hors lentille ; profondeur qui redescend ; permutation des IDs, axes et ordre des tâches.

Ajouter les formes proches de la dégénérescence de [l'audit arithmétique](../../morsehgp3D_v7/audits/ARITHMETIQUE_LANES_COURANTE.md), notamment Gram=1 avec coordonnées u16, et des cas où tous les témoins ponctuels échouent mais un groupe collectif réussit. Les deux rangées sont coplanaires : elles ne suffisent pas à rendre une campagne q4 positivement exercée. Leur adjoindre des amas 3D, des nappes non coplanaires et de petites familles adverses.

Après ces portes, mesurer n=8 000/16 000/32 000, Kmax=5/10, s=8/10/12, sur familles appariées et entrée LiDAR locale explicitement épinglée. Garder exactement la convention de séparation v8. Publier séparément T, F, résidu par voie, seeds, visites Z, tests de bornes, décisions collectives, événements, comparaisons de tri, groupes, cascades, boules distinctes, incidences, L et octets de pointe. Conserver les doublements égaux ou supérieurs à quatre, ainsi que les échecs et les régressions.

Les temps incluent préparation, répartition, copies, collecte, validation et destruction ; distinguer temps mur et sommes workers. Comparer des affinités identiques, répéter les bras retenus et ne pas réduire l'équilibrage aux temps de présence. Les objets devront ensuite être portés et requalifiés sur GPU, sans transférer une qualification CPU à G4.

Le contrat final reste toute la tour K=1..10 à 50k sous une seconde sur G4, avec repli K=1..5, puis cible 100 ms, et un régime distinct à plusieurs dizaines de millions de points. Aucun temps de front, de tri, de census ou de catalogue pris isolément ne ferme ce contrat.
