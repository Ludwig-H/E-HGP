# Du FULL aux opérateurs du réseau : coupes, masses et composition

Suite : le [réaudit global de tout Zoltan](REAUDIT_GLOBAL_20260926.md)
précise la réalisation des tokens, les limites du seuil α local, le graphe
et le coût des branches, les objectifs SEL et les contrôles du guidage.
Les preuves et reçus de ce document conservent leur portée initiale.

26 septembre 2026. Suite constructive de l'[audit v9](AUDIT_V9_ET_ARCHITECTURE_20260926.md), au commit de départ b44a16e1e. Morse HGP 3D est posé conforme à sa spécification. Le présent contrat porte sur les nouveaux opérateurs du réseau ; les résultats algébriques sont accompagnés de petites fixtures exécutables.

Cadre : **phase=conception_modele_fondation_hors_registre**, **backend=python_reference**, **profile=synthetic_incidence_only**, **mode=contrat_coupes_et_oracle_borne**, **public_status=not_claimed**. GCP non utilisé ; aucune donnée SemanticKITTI, aucune expérience apprise. Le script ne consomme pas une sortie v9 et ne qualifie pas un exporteur natif.

## 1. Décision proposée pour le premier pilote

Conserver **une branche par K**, avec son univers de facettes, ses poids et ses niveaux. À K fixé, construire une échelle par quotients d'un univers pondéré figé ; transporter les masses avec les moyennes. Entre K, utiliser les verticales pour échanger des caractéristiques par OM. N'assimiler cet échange à un pooling conservatif que si les deux affectations pondérées commutent effectivement.

Commencer par K1 et des coupes globales, où les sites peuvent être les atomes de masse unitaire, puis ajouter le profil pondéré K2/K3/K5. La condensation et les coupes adaptatives deviennent des variantes testées contre ce raccord simple. Ce choix garde l'hypothèse forte du projet — apprendre sur plusieurs lectures de FULL — tout en rendant chaque opérateur vérifiable.

Deux profils d'export sont utiles :

| profil | contenu | condition d'emploi |
| --- | --- | --- |
| **coverage_v1** | coupes FULL, événements, verticales et unions de sites par nœud | disponible conceptuellement depuis le journal FULL ; ne fournit pas les poids du §9.1 |
| **weighted_gabriel_v1** | incidences contributives, naissances propres, affectations pondérées et masses | nécessite le supplément pondéré et son raccord temporel ; le coût reste à mesurer |

Une normalisation uniforme de la seule couverture peut servir d'ablation, avec un nom distinct. Elle ne devient pas le vote du manuscrit.

## 2. Un nœud de coupe est un état daté

Un identifiant de nœud FULL ne fixe pas sa population. Les continuations ajoutent des contributions sans créer de nœud, comme le déclare [FullCoverageAction](../../morsehgp3D_v9/src/tower/forest/full_coverage_certificate.hpp). L'identité minimale d'un état consommé est :

~~~text
(tower_digest, K, segment_id, exact_radius_squared, cut_side,
 universe_id, measure_id, cut_policy)
~~~

Le côté de coupe vaut ouvert ou fermé. Une frontière adaptative porte le niveau propre de chaque état ; elle ne se présente pas comme une coupe à rayon global unique.

La fixture v9 **growth_ABCZ**, dans [full_ball_tower_gate.cpp](../../morsehgp3D_v9/tests/tower/full_ball_tower_gate.cpp), fournit un test natif pour le futur exporteur : à K3, ABC naît au rayon carré 16 ; au rayon carré 25, le même segment gagne Z sans multifusion. Une sérialisation par seul node_id confond les deux couvertures. Cette fixture existante est citée, sans nouveau rejeu v9 dans cette tranche.

Pour une facette pondérée, conserver séparément **sa naissance propre** et le segment qui la porte. La date de sa première coface contributrice peut être plus tardive : E5 donne 33/2 pour AC, contre 83886/3563 pour une incidence ultérieure, selon le [contrat des masses](../../morsehgp3D_v7/audits/CONTRAT_MASSES_VOTE_COURANT.md). Cette différence modifie les affectations à la coupe même si la connectivité FULL reste correcte.

## 3. Exporter les poids par flux d'incidences

Fixer pour chaque K le catalogue fini et dédoublonné de cofaces contributrices $C_K$, son horizon et la fonction $\psi$, positive et finie sur les rayons contributifs. Le profil du manuscrit prend les cofaces Gabriel de K+1 sites, avant réduction par arbre couvrant, et $F_K=\partial C_K$. Les facettes ont K sites ; elles peuvent être non Gabriel. Les supports minimaux q2/q3/q4 et les feuilles minima FULL ne remplacent aucun de ces deux ensembles.

Les [formules du contrat Morse HGP](../../morsehgp3D_v7/audits/CONTRAT_MASSES_VOTE_COURANT.md) sont :

$$S_\tau=\sum_{\sigma\in C_K,\ \tau\subset\sigma}\psi(\rho_\sigma),\qquad T_x=\sum_{\tau\in F_K,\ x\in\tau}S_\tau,\qquad w_{x\tau}=\frac{S_\tau}{T_x}\quad(x\in\tau,\ T_x>0).$$

Les unions de sites FULL effacent les multiplicités d'incidence. En revanche, l'échange des sommes donne un chemin d'export qui évite de stocker un index global point–facettes :

$$T_x=K\sum_{\sigma\in C_K,\ x\in\sigma}\psi(\rho_\sigma).$$

Pour une coupe c dont le résolveur connaît le propriétaire de chaque facette, on peut accumuler directement :

$$N_{xv}(c)=\sum_{\sigma\in C_K}\psi(\rho_\sigma)\,\#\lbrace\tau\subset\sigma:|\tau|=K,\ x\in\tau,\ \mathrm{owner}_c(\tau)=v\rbrace,\qquad P_c[x,v]=\frac{N_{xv}(c)}{T_x}.$$

**Proposition d'implémentation :** une passe pour T, puis une passe des incidences coface–facette vers les coupes demandées. À chaque coface, visiter ses K+1 facettes, y compris celles obtenues en supprimant un site intérieur. La naissance propre et le propriétaire de chaque facette doivent être résolus ; la première date de coface ne suffit pas. Les facettes répétées dans plusieurs cofaces contribuent plusieurs fois au score, mais chaque coface ne doit être émise qu'une fois.

Toutes les occurrences d'une même facette canonique doivent rendre la même naissance et le même propriétaire, quelle que soit leur coface d'origine. Le petit oracle futur comparera cette accumulation aux scores S calculés explicitement sur F. La matrice obtenue ici est la **lecture indépendante** $P_{\mathrm{FULL}}(c)$ ; la branche de pooling à univers représenté figé du §5 construit un autre opérateur $P_{\mathrm{pool}}(c)$.

Le catalogue de boules permet de proposer ces cofaces localement : pour intérieur I et coquille U, énumérer les sous-ensembles T de U tels que $|I|+|T|=K+1$ et que le centre appartienne à $\mathrm{conv}(T)$ ; la coface est $I\cup T$. Les tables de coquille existantes fournissent le test du centre. Il faut encore qualifier la complétude et l'unicité de ce flux pondéré. Les seules facettes représentatives utilisées pour résoudre FULL sont insuffisantes.

Sous census complet, Gabriel impose tous les intérieurs I ; la condition sur le centre certifie la MEB, dont l'unicité interdit de produire une même coface sous deux clés de boule distinctes. La [ShellTable actuelle](../../morsehgp3D_v9/src/tower/forest/local_plateau.hpp) accepte des coquilles de 2 à 12 sites : au plus $2^{|U|}$ masques locaux, ou seulement ceux de la cardinalité demandée. Un dépassement conserve son refus de domaine, sans troncature.

Ce schéma économise un stockage global, **sans borner le travail global** : publier nombres de cofaces, incidences, requêtes de propriétaire, non-zéros et octets par K et par coupe. Deux passes sur un flux fini ne signifient pas deux parcours bon marché d'une trame.

## 4. Quand deux niveaux composent exactement

Fixer un univers atomique $\Omega_K$, ses poids $w_{x\omega}$ et deux affectations totales $q_f:\Omega_K\to A$, $q_g:\Omega_K\to B$. Les réserves font partie des atomes et affectations ; A et B ne contiennent que des blocs non vides. Un quotient dur Q existe exactement si chaque bloc fin a une seule image grossière :

$$q_f(\omega)=q_f(\omega')\ \Longrightarrow\ q_g(\omega)=q_g(\omega').$$

Alors $q_g=Q\circ q_f$. Avec les **mêmes poids**, l'échange des sommes prouve :

$$P_f[x,a]=\sum_{q_f(\omega)=a}w_{x\omega}\quad\Longrightarrow\quad P_g=P_fQ.$$

Une antichaîne quelconque n'assure ni totalité ni emboîtement. Conserver un témoin de quotient sur les atomes/incidences, puis vérifier l'identité matricielle : des poids égaux peuvent masquer une mauvaise attribution si l'on ne contrôle que les matrices.

La conservation concerne le transport linéaire. Avec une mesure $\mu_x$ déclarée, les masses et les moyennes sont :

$$M=P^\top\mu,\qquad h_v=\frac{\sum_x\mu_xP[x,v]h_x}{M_v}.$$

Aux étages suivants, il faut transporter les masses :

$$M_g=Q^\top M_f,\qquad h_g=D_{M_g}^{-1}Q^\top D_{M_f}h_f.$$

Les inverses diagonaux sont restreints aux masses positives ; les jetons de masse nulle sont supprimés ou masqués.

Exemple permanent : trois valeurs (0,2,12), regroupées en {0,2} et {12}, donnent les moyennes (1,12) de masses (2,1). La moyenne globale est 14/3 ; moyenner les deux jetons sans leurs masses donne 13/2. Le quotient dur seul ne suffit donc pas à une implémentation correcte de FP. Après non-linéarité ou attention, aucune conservation de la somme des caractéristiques n'est imposée.

## 5. Réserve, naissances tardives et condensation

Une ligne de poids (1/4,3/4) dont la seconde facette est retirée doit devenir **survivant 1/4, réserve 3/4**. Traiter seulement les lignes entièrement vides perd de la masse ; renormaliser le survivant à 1 change la mesure. Les sorties de condensation sont datées **par incidence ou branche** : un point couvert par deux branches peut avoir deux niveaux de sortie. Un unique scalaire par retour exige une agrégation déclarée, par exemple une moyenne pondérée ; ce scalaire ne remplace pas la trace des sorties.

Pour une multifusion parcourue en sens de scission, calculer simultanément les branches lourdes $H=\lbrace i:M_i\geq\alpha M_{\mathrm{parent}}\rbrace$. Zéro branche lourde termine le segment condensé ; une branche lourde poursuit son identité ; au moins deux branches lourdes créent une scission simultanée. Toute masse directement attachée à l'événement est traitée séparément des branches strictes. Cette règle corrige l'ambiguïté entre « chaque branche survit » et « la branche lourde continue ».

**Une réserve par site ne garantit pas la composition dure.** Sur trois points colinéaires 0,2,4, à K2 et rayon fermé 1, les facettes ab et bc sont deux composantes ; abc ne les relie qu'au rayon 2. Pour un univers pondéré symétrique, b partage son vote entre ab et bc. Avant leur naissance, une réserve unique de b ne peut être envoyée par une ligne 0/1 vers ces deux jetons. Ce raisonnement géométrique est distinct du script de cette tranche, qui n'en exerce que la ligne abstraite de poids.

Deux constructions sont cohérentes :

1. **Univers représenté figé au niveau fin.** Après choix éventuel de condensation, retenir les facettes actives $F_*$ au premier niveau, conserver leurs poids calculés sur F et ajouter un atome de réserve par site portant exactement son déficit. Les étages suivants ne font que quotienter cet univers ; une réserve ne se scinde pas. Les naissances ultérieures sont lues par une branche latérale. C'est le choix proposé pour le pilote.
2. **Réserves fines par incidence.** Conserver assez d'information pour répartir chaque future facette ; publier ce coût, potentiellement bien supérieur à une réserve par site.

Dans le premier choix, $P_{\mathrm{pool}}(c)=P_{\mathrm{pool}}(c_f)Q$ transporte les facettes de $F_*$ et les réserves persistantes ; il ne prétend pas égaler $P_{\mathrm{FULL}}(c)$ après de nouvelles naissances. Si une branche est retirée après le niveau fin, transférer son jeton entier vers une réserve de branche compatible avec Q ; le redistribuer en plusieurs réserves de sites romprait cette factorisation. Recalculer librement P à chaque rayon reste une troisième expérience possible, mais ses niveaux sont alors des lectures distinctes.

**Porte de décision du pilote :** publier par K, niveau et portée la fraction de masse initialement représentée, la masse des naissances ultérieures et la réserve persistante. Si une branche conserve surtout des réserves, sa composition est correcte mais elle utilise peu de FULL. Comparer alors les coupes indépendantes avec échanges latéraux au quotient figé, à même budget. Le contrat algébrique ne suffit pas à choisir la meilleure architecture.

## 6. Entre K : deux commutations différentes

La verticale FULL donne une image géométrique à une coupe valide ; pour des quotients adaptatifs, la condition est :

$$q_{K-1}\circ f_K=V_K\circ q_K.$$

Le membre de gauche doit être constant sur chaque fibre de $q_K$. Une incidence vide ou visant plusieurs jetons cibles refuse un parent unique. Le moteur fournit une consultation à la coupe par [full_ball_vertical_root_at](../../morsehgp3D_v9/src/tower/forest/full_ball_tower.hpp) ; recopier l'image enregistrée à la naissance ne suffit pas si l'ordre inférieur a fusionné depuis.

Les q de ce paragraphe quotientent des **composantes géométriques datées**. La verticale n'est pas une fonction d'une facette de K sites vers une facette unique de K−1 sites ; ces domaines diffèrent des atomes pondérés du §4.

Même lorsque V est dur et géométriquement correct, **la mesure de chaque ordre peut différer**. Le [contrat vertical Morse HGP](../../morsehgp3D_v7/audits/CONTRAT_VERTICAL_COURANT.md) n'impose aucune conservation de masse. L'identité supplémentaire $P_KV_K=P_{K-1}$ doit être prouvée ou testée dans le profil choisi ; elle ne découle pas de la naturalité.

Conséquence pratique : OM reçoit des correspondances et échange des caractéristiques entre branches autonomes. Si un pooling entre ordres est souhaité, publier les deux contrôles distincts : factorisation géométrique et compatibilité des poids. Coarsenir la frontière cible jusqu'à contenir les images peut réparer la première propriété ; cela coûte des jetons/résolution et ne répare pas automatiquement la seconde.

## 7. PUR : préciser les probabilités et la perte d'information

L'extension linéaire du vote de labels du §9.1 aux sorties du réseau est une **mixture de probabilités** : $p_x=\sum_vP[x,v]\pi_v$. Lorsque chaque jeton porte une classe certaine, elle restitue le vote du manuscrit. Mélanger les logits puis appliquer softmax définit un autre opérateur, qui peut changer la classe prédite ; le falsificateur en donne un exemple rationnel vérifié par les rapports de probabilités.

La moyenne puis la lecture donnent $PD_M^{-1}P^\top D_\mu h$. Cet opérateur préserve les constantes lorsque P couvre toute la masse ; il ne reconstitue généralement pas h. Les connexions de saut et le maintien d'une représentation fine sont donc des éléments fonctionnels du décodeur. La Proposition 7 justifie le vote final et son départage, pas l'inversion du pooling.

Pour l'interface SemanticKITTI à 19 logits par retour, on peut rendre les log-probabilités de la mixture, calculées par log-sum-exp des log-poids et des log-softmax de jetons. Cela préserve l'opérateur probabiliste choisi et l'ordre original des retours ; les poids nuls sont omis et la convention de réserve reste explicite.

Le réducteur historique [point_hierarchy.hpp](../../morsehgp3d/include/morsehgp3d/api/point_hierarchy.hpp) annonce un routage descendant irréversible. Il ne sert pas d'oracle de P ni d'un vote global sur une antichaîne arbitraire : avec trois masses (3/10,3/10,4/10), le routage peut choisir d'abord le parent des deux premières, tandis que le vote sur les trois feuilles choisit la troisième. Sa sémantique propre est conservée ; l'oracle du nouveau raccord doit partir des incidences avant ce routage.

## 8. Identifiants, précision et schéma minimal

Les tableaux du catalogue emploient des indices géométriques, tandis que les populations FULL portent les identifiants des sites d'entrée. Le raccord doit conserver la permutation géométrie→site pendant que l'index vit, puis la table retour→site de la préparation. Une duplication des lignes de P vers les retours préserve leur somme 1 mais change les masses si chaque retour reçoit poids 1.

Le pilote déclare l'une de ces mesures : **par retour**, $\mu_i=1$ ; ou **par site**, $\mu_i=1/m_s$ pour les $m_s$ retours retenus au site s dans la vue et le masque courants. Les retours retirés restent traçables sans contribuer à μ. Conserver aussi la multiplicité et les attributs individuels. Aucune duplication ne change l'ordre K géométrique, calculé sur les sites.

Un niveau carré exact $\lambda=\rho^2$ ne rend pas $\rho^{-3}=\lambda^{-3/2}$ rationnel. Garder les niveaux et multiplicités exacts pour la provenance et les décisions qui réclament une certification ; déclarer la précision et le résidu de normalisation des matrices numériques du réseau. L'oracle Fraction de cette tranche vérifie l'algèbre avec poids rationnels fournis, pas un calcul général exact des masses p3.

| champ du CutBundle | contrat |
| --- | --- |
| **source** | version de schéma, commit/digest moteur, brut, préparation, masque et vue ; statut d'autorité inchangé |
| **authority** | version et portée de la preuve source ; hypothèses expérimentales et approximation numérique explicites |
| **payload_kind / carrier_kind** | couverture, incidences pondérées, supports ou autre réalisation explicitement nommée |
| **universe / measure** | C, F, horizon, ψ, conventions zéro/bruit, unité, mesure site/retour ; identités gelées entre niveaux composés |
| **sites / returns** | permutations explicites, tous les retours originaux et les retours retirés identifiables |
| **cut_policy / cut_side / levels** | coupe globale ou frontière adaptative, niveau carré exact et côté pour chaque état |
| **assignment** | CSR pondéré, réserves partielles, masses par jeton, statut numérique |
| **horizontal** | quotient atomique puis map dure ; contrôle de composition |
| **vertical** | image à la coupe, quotient ou incidence latérale ; compatibilité pondérée publiée séparément |
| **readout** | mixture de probabilités ou autre opérateur nommé, masque de réserve, départage |
| **budget** | jetons, réserves, cofaces, incidences, résolutions, non-zéros, octets, temps par phase |

À K=n, le catalogue des cofaces K+1 est vide alors que FULL reste défini. Les points de T nul exigent une réserve ou le profil K1 atomique explicite. Pour zéro/un site et K hors domaine, le tokenizer publie un statut et une règle définie. Une cible de jetons inférieure au nombre de racines indépendantes portant les atomes retenus dans l'horizon, plus les réserves distinctes persistantes, n'est pas réalisable : ne pas fabriquer une fusion géométrique pour satisfaire le budget.

## 9. Vérification et prochain développement

Le [falsificateur rationnel](reference/verify_cut_algebra.py) exerce les conditions de composition, les pertes partielles de masse, les moyennes, les réserves à scinder, les quotients, le vote et les mesures site/retour. Les fixtures d'algèbre n'affirment pas qu'une matrice arbitraire provient d'un nuage HGP. Les [résultats enregistrés](receipts/cut_algebra_20260926/README.md) distinguent ces contrôles de toute qualification géométrique.

Le prochain export natif doit d'abord produire **coverage_v1**, avec les coupes avant/au/après événement et la fixture growth_ABCZ. En parallèle, le petit oracle pondéré compare l'accumulation par cofaces à la construction explicite de F sur de petits nuages. Une fois cet accord obtenu, mesurer le flux pondéré sur une trame entière et choisir le coût admissible du CutBundle. Ni une matérialisation globale de toutes les facettes ni une renormalisation tacite à chaque coupe ne sont des prérequis de cette voie.
