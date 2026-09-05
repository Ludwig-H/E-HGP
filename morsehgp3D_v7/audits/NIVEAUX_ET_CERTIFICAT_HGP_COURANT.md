# Niveaux utiles et certificat des hiérarchies HGP

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Réponse indépendante à la question prioritaire de l’utilisateur et du constructeur. Les parties I et II du manuscrit restent lues ; les pages PDF 84–85, 113–117 et 122–126 sont relues pour cette décision. Aucune mutation du produit.

**Pour HGP complet du manuscrit, isolés inclus, il suffit sous les hypothèses déclarées de conserver les minima Gabriel de cardinal K avec leurs points et niveaux, puis les vraies multifusions aux niveaux Gabriel de cardinal K+1 avec leurs parents. Les couvertures sont les unions des feuilles descendantes : aucun delta ponctuel de continuation FULL n’est nécessaire.** Les rattachements silencieux restent indispensables à la construction des bons parents, sans devoir devenir des événements du certificat final. Cette conclusion est plus forte que celle sur la seule restriction réduite initialement examinée. Elle ne qualifie pas encore une implantation industrielle ou son coût.

La distinction décisive est entre **ce qu’il faut savoir pour construire** et **ce qu’il faut garder pour rejouer**. Une incidence silencieuse peut être indispensable à la première opération et absente de la seconde. Demander Gamma exhaustif comme sortie ou comme architecture par défaut serait une exigence excessive.

## 1. HGP complet et sa restriction réduite

Les définitions 21–22 du manuscrit, PDF 84, définissent les K-polyèdres par les composantes de Gamma **isolés inclus**. La figure 6.5, PDF 85, montre explicitement une arête CD isolée comme K-polyèdre à K2. Cet objet FULL est la cible de la question présente. Il conserve les composantes avec leurs identités, leur généalogie et leurs couvertures en points ; il ne confond pas les composantes par simple recouvrement.

Le payload actuellement qualifié de la v7 conserve la restriction aux composantes non triviales pour K supérieur à un, et les points normatifs à K1. Cette restriction réduite possède sa propre preuve. Elle ne doit pas être appelée FULL ni servir à exclure les racines isolées du modèle mathématique.

La partition exhaustive des facettes, leur première incidence et le carrier géométrique marqué sont encore d’autres objets. Reconstruire les composantes et leurs points ne signifie pas reconstruire chacune des facettes qu’elles contiennent. Le §9.1 permet de distinguer la partition de facettes et le recouvrement de points. Le qualificatif « HGP exact » doit préciser lequel de ces objets est livré.

La preuve régulière suppose des supports essentiels uniques, des intrus strictement intérieurs, l’absence d’extra-shell pertinente et le traitement atomique des niveaux égaux. Le [certificat horizontal](CERTIFICAT_HORIZONTAL_COURANT.md) dispose d’une extension au domaine CPU par inertie des blocs saturés hors fenêtre de rang. Cette extension demeure une autorité distincte : le nouveau producteur de portails devra certifier les contrôles de fenêtre, de contact et de chaîne qu’il consomme. La présente conclusion ne supprime aucun refus géométrique actuellement requis.

## 2. FULL : naissances, multifusions et rien d’autre à publier

### 2.1 Les naissances sont exactement les facettes Gabriel

Soit F une facette de cardinal K, de niveau $b=\beta(F)$. Si F est Gabriel et régulière, sa boule fermée ne contient aucun point extérieur à F. Une extension de F au même niveau aurait la même MEB par unicité, ce qui est impossible. Sa première incidence est donc strictement postérieure à b, ou infinie : F est une vraie naissance isolée.

Si F n’est pas Gabriel, un intrus strict z donne la coface $Q=F\cup\lbrace z\rbrace$ dès le niveau b. Cette coface possède au moins deux facettes strictes, déjà présentes dans FULL avant b. F rejoint donc un groupe ayant des parents antérieurs, sans nouvelle naissance. **Avec un seul intrus, Q peut être Gabriel et fusionner plusieurs parents** : il serait faux d’attribuer systématiquement un apex unique à toute facette non-Gabriel. Le lemme d’apex s’applique aux cofaces non-Gabriel, pas à cette première coface directe.

Le triangle obtus A=(0,0,0), B=(4,0,0), C=(1,1,0) rend cette différence explicite. À K2, AC et BC naissent comme minima Gabriel aux niveaux 1/2 et 5/2. AB, non-Gabriel avec l’unique intrus C, apparaît au niveau 4 en même temps que ABC, qui fusionne les deux anciens parents. AB n’est pas une troisième feuille.

La [preuve FULL indépendante](receipts_gabriel_20260905/full_proof_review.md) borne aussi la régularité : pour A=(0,0,0), B=(2,0,0), C=(1,1,0), AB est Gabriel au sens de l’intérieur strict vide, mais C est sur le shell et $\beta(AB)=\beta(ABC)=1$. Émettre AB comme une naissance isolée persistante serait faux. Le quotient de naissance/connexion simultanée hors du domaine régulier reste un contrat distinct.

### 2.2 Toute continuation FULL conserve les points

Pour une coface régulière Q, retirer chacun des sommets essentiels donne au moins deux facettes strictes dont l’union couvre Q. Elles appartiennent à des composantes FULL antérieures, même lorsqu’elles sont encore isolées. La couverture après le lot est donc exactement l’union des couvertures de ses parents. Aucune coface ne crée une nouvelle composante ex nihilo dans FULL, et aucun point supplémentaire n’apparaît hors de cette union.

Avec un parent, c’est une continuation sans changement de couverture ; avec plusieurs parents, c’est une vraie multifusion. Le §3 traite les plateaux silencieux entiers et prouve que les vraies fusions sont portées par des valeurs Gabriel de cardinal K+1. Une naissance Gabriel de cardinal K ne peut être incidente à une coface du même niveau : elle ne devient jamais un parent fabriqué dans ce lot. Les parents d’une fusion sont toujours lus dans la coupe stricte.

### 2.3 Certificat final FULL

| Enregistrement | Données à conserver |
| --- | --- |
| Manifeste | Identité d’entrée, PointId, métrique/unité, ordres disponibles, horizon certifié, version, convention de coupe et autorité terminale de complétude |
| Feuille | Identité distincte, K PointId du minimum Gabriel et son niveau exact |
| Multifusion | Identité distincte, niveau exact et parents distincts actifs avant le lot |

Activer les feuilles à leur niveau, puis contracter les groupes de parents atomiquement. La couverture de toute composante est l’union des points de ses feuilles descendantes. L’induction précédente reconstruit donc les deux côtés des coupes, les recouvrements et la généalogie. Les cofaces silencieuses, les couvertures copiées à chaque nœud et les deltas de continuations ne sont pas des données nécessaires à ce rejeu. Un cache de ces unions peut accélérer la lecture, sans ajouter de contenu mathématique.

À K1, les feuilles ponctuelles naissent à zéro : la coupe ouverte zéro est vide, la coupe fermée zéro contient les points. À **K=n**, X est une unique feuille Gabriel née à $\beta(X)$, sans aucune coface ni fusion ; elle ne doit pas disparaître parce que le générateur ne produit plus d’ordre de connexion supérieur. La restriction réduite est vide à cet ordre pour n supérieur à un. Un ordre vide reste distinct d’un ordre non demandé ou interrompu.

Si L est le nombre de minima et R le nombre de composantes finales, les multifusions vérifient $\sum_v(d_v-1)=L-R$, donc $I\leq L-R$ et le nombre de liens vaut $L+I-R$. Le stockage topologique est linéaire en L, avec O(KL) identifiants pour les labels des feuilles. **L n’est pas n aux ordres supérieurs** ; aucune borne générale de n−1 fusions par ordre n’est acquise.

Pour une tour demandée jusqu’à K maximal, il suffit donc des points et des catalogues Gabriel de cardinalités 2 à $\min(n,K_{\max}+1)$. Une directe de cardinal m fournit, avec la même géométrie déjà certifiée, une feuille à l’ordre m et une connexion candidate à l’ordre m−1. Cela borne les rangs nécessaires ; cela ne dispense ni de découvrir les bons parents ni de qualifier un nouveau fold FULL. Le produit actuellement réduit n’est pas promu par ce raisonnement.

## 3. Pourquoi un niveau sans Gabriel est invisible pour cet objet

Écrire $\beta(Q)=\rho(Q)^2$. Pour une coface non-Gabriel Q de niveau a, soit U son support essentiel et z un intrus strict. Pour chaque $u\in U$, la coface $(Q\setminus\lbrace u\rbrace)\cup\lbrace z\rbrace$ a un niveau strictement inférieur à a. Ces remplacements relient toutes les facettes strictes de Q dans une même composante antérieure. Comme U contient au moins deux sommets, leur union couvre déjà tous les points de Q. Cette composante est non triviale : une coface silencieuse seule n’apporte ni naissance, ni fusion, ni point nouveau.

Il faut encore fermer le cas **simultané**. Deux cofaces silencieuses de niveau a qui partagent une facette stricte ont déjà le même parent. Si la facette partagée est de niveau a, l’unicité de la MEB identifie leurs boules et leurs supports. Retirer un sommet essentiel et ajouter leurs deux sommets intérieurs distincts fournit une coface de niveau inférieur reliant leurs bras stricts. Elles ont donc encore le même parent. Par connexité, tout groupe de cofaces silencieuses du plateau est attaché à un seul ancien parent et n’ajoute aucun point.

C’est le [lemme d’apex avec confluence](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#52-théorème-conditionnel-de-rétraction-sur-le-cœur-direct), renforçant le théorème 4 du manuscrit. L’affirmation isolée « chaque coface n’est pas séparante » ne suffit pas à traiter les égalités ; la confluence est l’étape nécessaire. La [contrelecture de cette preuve](receipts_gabriel_20260905/level_proof_review.md) distingue aussi le domaine régulier et les blocs hors fenêtre.

**Conséquence.** Entre deux valeurs Gabriel consécutives du même ordre, les inclusions des composantes réduites sont des bijections qui préservent leurs couvertures. Toutes les naissances, multifusions et croissances en points ont lieu à des valeurs Gabriel. Les coupes ouvertes et fermées restent distinctes aux valeurs conservées. Cela ne signifie ni que toutes les cofaces Gabriel provoquent un changement, ni que toutes les valeurs Gamma cessent d’être utiles aux calculs internes.

## 4. E5 confirme exactement cette distinction

La [contre-fixture et sa portée](receipts_gabriel_20260905/counterfixture_scope.md) donnent, pour E5 à K2, ce **journal réduit** de composantes avec couverture. FULL conserve aussi les minima antérieurs : les naissances et croissances ci-dessous sont des projections de ses multifusions, pas ses événements natifs.

| Rayon carré | Information à publier |
| --- | --- |
| 162/25 | Naissance d’une composante couvrant CDE |
| 189/17 | Même composante ; ajout du point A |
| 33/2 | Aucun changement de cet objet |
| 83886/3563 | Même composante ; ajout du point B |
| 24 | Aucun changement de cet objet |

Au niveau 33/2, Gamma rattache la facette AC à l’ancienne composante, qui couvre déjà ACDE. Quand ABC arrive, il doit retrouver cette composante par AC. Le fold Gabriel brut l’ignore, invente une seconde naissance, puis une fusion à 24. Il possède donc même le mauvais arbre non gradué ; déplacer ses poids ne le réparerait pas.

En revanche, **il n’existe aucune fusion Gamma à récupérer au niveau 33/2**, ni même à 24 dans cet exemple. Le niveau 33/2 est contractible dans le journal ci-dessus, à condition que l’attache de AC soit résolue avant ABC. Les quatre points ACDE suffisent déjà à montrer une activation de facette sans événement visible ; le cinquième point d’E5 rend cette omission dangereuse lors d’une réutilisation future. L’exemple de la courbe des moments qui manque une facette hors du cœur réfute, lui aussi, une exhaustivité facettée ; il ne réfute pas la graduation du quotient réduit.

## 5. Portails : la construction réduite qualifiée comme modèle

Une règle plus précise que « garder des attaches » est disponible. Traiter les cofaces Gabriel complètes par niveau exact atomique, en mémorisant les facettes qu’elles ont déjà consommées et les identifiants historiques de leurs composantes.

Pour une facette régulière F encore inconnue à sa première consommation par une directe Q de niveau a, compter les intrus stricts $J_F=(X\cap B_F^{\circ})\setminus F$.

- Si $\lvert J_F\rvert\leq1$, une première incidence minimisante est directe. Elle ne peut être strictement antérieure à a sans avoir déjà enregistré F. L’inconnue appartient donc au traitement du lot, sans parent antérieur à inventer.
- Si $\lvert J_F\rvert\geq2$, sa première incidence est silencieuse et $\lambda(F)=\beta(F)<a$. L’inégalité est stricte : une égalité identifierait $B_F=B_Q$, tandis que Q ne pourrait absorber qu’un des deux intrus, contredisant son caractère Gabriel.

Dans le second cas, partir d’une coface $F\cup\lbrace z\rbrace$, puis remplacer un sommet essentiel par un intrus strict jusqu’à une coface directe antérieure ou un cache entièrement certifié. Les niveaux diminuent strictement. Le certificat de descente donne une ancre dans l’ancienne composante de F ; retrouver le successeur courant de cette ancre dans l’état **strictement antérieur au lot**.

Le quotient atomique du lot contracte les facettes partagées par ses directes et les parents ainsi retrouvés. Il publie les naissances, multifusions et deltas de points utiles. Les cofaces intermédiaires de descente et le niveau silencieux ne doivent pas être émis pour ce seul rejeu. Une facette enregistrée au cours du lot ne devient pas pour autant un parent pré-lot ; la présence d’une clé dans un dictionnaire n’est pas un certificat temporel.

Cette construction demande un catalogue direct complet, une table sans omission des consommations passées, une descente certifiée et des identités normalisées. Les requêtes exactes de MEB et d’intrusion restent nécessaires là où elles prouvent ces rattachements. Le gain de stockage ou de tri est une piste concrète ; aucune borne de temps linéaire, aucun gain mesuré et aucune couverture produit des mutants ne sont déduits de ce changement de représentation.

Le modèle FULL ajoute les racines Gabriel dès leur naissance, avant leur première incidence. Une facette inconnue à un seul intrus rejoint le lot courant ; une facette sans intrus aurait déjà dû être enregistrée comme minimum. Le lot gèle tous ses anciens parents, conserve les alias des facettes consommées même sans événement publié et n’émet que ses multifusions. Ses nouvelles feuilles sont disjointes des connexions du même niveau. La sonde FULL ci-dessous vérifie cette adaptation séparément.

## 6. La restriction réduite se déduit du certificat FULL

Sous la même régularité, une composante FULL isolée ne peut devenir non triviale sans une vraie fusion. Lors de sa première coface incidente, une seconde facette stricte distincte existe avant le lot ; si elle appartenait déjà à cette même composante, la première ne serait plus isolée. Pour K supérieur à un, une composante est donc non triviale exactement lorsqu’elle descend d’un nœud interne, c’est-à-dire d’au moins deux minima. Aucun bit ni date supplémentaire de « non-trivialité » n’est nécessaire.

À chaque multifusion FULL, compter ses parents déjà internes : zéro donne une naissance réduite, un une continuation réduite, au moins deux une multifusion réduite. Les parents-feuilles supprimés expliquent les éventuels points nouveaux dans cette projection. Ainsi les deltas ci-dessous sont **dérivables du certificat FULL**, et non un second journal à conserver obligatoirement. La [contrelecture du certificat FULL](receipts_gabriel_20260905/full_certificate_review.md) ferme cette projection et le cas K=n.

Le résultat peut conserver une identité d’entrée, les PointId, les ordres disponibles, l’horizon certifié, la convention de coupe et le succès terminal de complétude. Un ordre réduit vide doit être distingué d’un ordre absent ou interrompu. Par ordre, le journal suivant suffit :

| Enregistrement | Données nécessaires au rejeu |
| --- | --- |
| Racines K1 | Un identifiant distinct par point, éventuellement dérivé du manifeste |
| Naissance | Identifiant neuf, niveau exact, ensemble des points couverts |
| Multifusion | Identifiant neuf, niveau exact, parents distincts actifs avant le lot, points nouveaux par rapport à leur union |
| Continuation avec croissance | Identifiant de la composante, niveau exact, seuls points nouveaux |

Une continuation à un parent sans point nouveau n’a aucun enregistrement à produire pour cet objet. Une continuation avec croissance peut garder l’identité abstraite de sa composante ; les anciennes clés dépendant de facettes exhaustives ne sont pas une obligation de cet encodage.

**Preuve de suffisance.** Initialiser les racines normatives, puis lire chaque lot depuis l’état strict antérieur. Chaque parent est consommé au plus une fois dans ce lot. Pour une naissance, créer sa couverture ; pour une fusion, unir les couvertures de ses parents et le delta ; pour une continuation, ajouter le delta à sa composante. Cette induction reconstruit les composantes, leur généalogie et leurs couvertures à toute coupe. La constance démontrée entre niveaux conservés fournit toutes les coupes intermédiaires. Un journal syntaxiquement correct ne certifie toutefois pas sa fidélité au nuage : la complétude de son producteur reste une prémisse externe contrôlée.

C’est un **certificat suffisant pour cet objet**, pas un théorème d’encodage optimal en nombre de bits. Les identités et parents portent l’information que le recouvrement des points ne détermine pas. La [contrelecture du certificat](receipts_gabriel_20260905/minimal_certificate_review.md) précise ces obligations de rejeu et les suppléments suivants.

## 7. Supplément inter-K : une ancre par vraie naissance

Pour une naissance FULL de label F à l’ordre K supérieur à un, F est elle-même une directe de l’ordre K−1. Le groupe de cette directe, après fermeture de son plateau inférieur, fournit directement l’ancre cible. Aucun scan géométrique supplémentaire de toutes les suppressions n’est nécessaire à cette règle ; il faut conserver l’association au groupe même si la directe inférieure était une continuation sans événement publié.

Pour chaque naissance à l’ordre supérieur, conserver cette référence certifiée à sa composante cible inférieure à la même coupe fermée. Propager cette ancre dans les continuations et la normaliser au fil des fusions inférieures. Lors d’une multifusion supérieure, les images de ses parents doivent coïncider **après fermeture du plateau inférieur simultané**. Les comparer trop tôt créerait un faux refus.

La naturalité du [contrat vertical](CONTRAT_VERTICAL_COURANT.md) donne ensuite toutes les cartes et compositions. L’union des graduations utiles des ordres suffit à synchroniser le rejeu ; un compteur de lots propre à K n’est pas une horloge commune. Un identifiant cible certifié peut remplacer son témoin géométrique dans la sortie. Trouver et vérifier l’ancre pendant la construction reste distinct de sa conservation.

## 8. Supplément pondéré : préciser la mesure, sans réintroduire Gamma

L’Algorithme 1, PDF 126, fixe les cofaces contributrices Gabriel et **toutes leurs facettes** ; les scores sont agrégés avant l’arbre couvrant. Une facette d’une coface Gabriel n’est pas nécessairement elle-même Gabriel. Les descentes auxiliaires de rattachement ne sont pas de nouvelles contributions à ces scores. Imposer ici un univers Čech exhaustif au manuscrit serait injustifié.

Les minima Gabriel du certificat FULL ne sont pas automatiquement les feuilles pondérées de l’Algorithme 1. Dans le triangle obtus précédent, les minima K2 sont AC et BC ; son univers pondéré contient aussi AB, facette de la directe ABC. Avec le poids uniforme autorisé, les trois scores valent un, chaque point a T=2 et chaque arête a masse un. Omettre AB perd donc une masse un ; recalculer les normalisations sur les seuls minima changerait le profil.

Pour un univers de feuilles fixé, le §9.1 donne $w_{x\tau}=S_{\tau}/T_x$ et $m_{\tau}=\sum_{x\in\tau}w_{x\tau}$. La seule couverture en points d’une composante ne détermine pas les feuilles qu’elle contient ni sa masse. AC est une facette pondérée issue du catalogue de cofaces Gabriel d’E5 et porte une masse positive, sans être un minimum FULL. **Si** la politique l’affecte à sa première incidence Gamma, son transfert à 33/2 doit être conservé pour retrouver la masse entre 33/2 et 83886/3563. Le reporter à ABC change cette masse, alors que le journal topologique reste correct.

Cette politique temporelle n’est pas explicitement fixée par l’Algorithme 1 ou le §9.1 : il ne faut pas présenter sa nécessité conditionnelle comme une obligation déjà démontrée du manuscrit. Le constructeur doit préciser le traitement des feuilles isolées, la date d’affectation à une composante et la convention de condensation avant de revendiquer la même sélection pondérée.

Même après ce choix, Gamma complet n’est pas requis par défaut. Pour les seules masses, un journal des transferts scalaires utiles peut suffire au contrat fixé. Pour le vote, il faut en outre pouvoir retrouver les contributions par point des familles de feuilles susceptibles de recevoir des étiquettes différentes, ou leurs incidences équivalentes. Un total scalaire de composante ne permet pas de reconstruire ces votes. Les [contrats de mesure](CONTRAT_MASSES_VOTE_COURANT.md) et l’[autorité numérique p3](AUTORITE_VOTE_P3_COURANTE.md) restent applicables à ce supplément déclaré.

## 9. Vérifications exécutées et limites

Les [reçus reproductibles](receipts_gabriel_20260905/README.md) distinguent trois expériences, chacune en Python normal et optimisé, avec résultats identiques hors indicateur d’optimisation.

| Expérience | Résultat borné |
| --- | --- |
| Construction réduite par portails | 8 cas, 36 ordres, 1 752 coupes contre Gamma exhaustif, 1 372 carrés horizontaux ; 100 événements utiles |
| Construction FULL par minima et portails | 10 cas, 50 ordres K1..n, 2 265 coupes, 2 225 carrés horizontaux ; 178 minima et 107 multifusions, aucun delta ponctuel |
| Projection depuis les seuls journaux scellés | Les 100 événements réduits des 36 ordres communs sont retrouvés avec leurs parents et deltas ; 1 288 coupes appariées, 10 projections terminales K=n vides |

FULL vérifie 123 groupes de cofaces ayant des parents stricts dont l’union couvre déjà le groupe, 100 contrôles de frontière zéro et dix feuilles terminales K=n. Les triangles obtus exercent le cas à un intrus et les minima simultanés. L’extra-shell est conservé comme contre-fixture hors domaine. Six mutants de modèles d’audit sont rejetés, dont un par la garde de naissance incidente au même lot ; ils ne sont pas des mutants du produit.

Gamma exhaustif, borné ici à sept points, fournit les catalogues de fixtures et le juge de coupes. Les constructeurs reçoivent seulement les coordonnées, les PointId et les catalogues Gabriel ; ils n’interrogent pas la partition Gamma. La primitive MEB rationnelle est commune aux deux voies, tandis que les constructions d’incidence, les quotients et les lecteurs sont distincts. Cette expérience juge la réduction topologique, pas une deuxième implémentation indépendante de la géométrie.

Les deux descentes exercées ont un seul pas strict, sur E5 et son réindexage. Les normalisations historiques sont aussi exercées sur les deux triangles séparés, mais aucune chaîne longue n’est qualifiée. Les tables de construction restent des témoins d’audit, sans borne industrielle de résidence. Les cartes verticales FULL et la masse sont prouvées ou contractualisées ci-dessus, sans nouvelle exécution produit. Les corpus FULL et réduit diffèrent : leurs nombres totaux d’événements ne donnent pas un ratio de compression.

Le [premier composant C++ FULL](CERTIFICAT_FULL_CPP_COURANT.md) possède depuis `f4c0734c` une qualification indépendante du stockage et du rejeu sur les lots fournis. Ses 4 608 coupes et 616 couvertures par build ne sont pas réattribuées aux sondes Python ci-dessus. Cette étape ferme le lecteur structurel, sans encore fournir les minima ou les parents depuis la géométrie.

## 10. Décision pour la suite

La question « faut-il garder tous les niveaux Gamma pour les K hiérarchies HGP du manuscrit ? » est résolue négativement dans le domaine annoncé. Le certificat FULL est constitué des minima Gabriel et des vraies multifusions, munis de leurs niveaux et de leurs parents. Il permet aussi de dériver la restriction réduite actuellement qualifiée. Les rattachements sont à certifier pendant la construction ; la tour, les masses et le vote possèdent leurs suppléments explicites. Les saturés Gabriel de tous les ordres réencodent certes les niveaux Gamma, mais leur rang peut atteindre n ; cette identité n’est pas une architecture parcimonieuse pour une fenêtre K limitée.

La prochaine qualification utile porte sur le constructeur par portails et son journal consommé : fidélité des ancres pré-lot, égalités atomiques, normalisation des identités, domaines géométriques et budgets. Elle ne demande pas de reconstruire Gamma pour le produit ni de refaire les preuves locales MEB déjà closes. F reste inchangé, le statut public reste `not_claimed`. GCP non utilisé.
