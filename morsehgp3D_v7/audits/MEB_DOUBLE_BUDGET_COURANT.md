# MEB privée à deux budgets : certificat local et repli F

5 septembre 2026. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Conclusion de lecture : le prototype privé réalise le raccourci géométrique et les deux charges annoncés, sous son domaine interne explicite. Aucun défaut géométrique concret n'a été trouvé dans les sources épinglées.** La coquille régulière impose le même support, le même ordinal et les mêmes champs de boule que F ; tout échec de proposition retrouve F depuis le même état public. La charge prospective du second budget corrige le défaut physique du prototype ordinal seul. La qualification locale indépendante du § 8 complète désormais le reçu triangle. L'intégration par ordre et le schéma public restent à effectuer ; aucun de ces reçus privés ne les établit.

Cette note est une preuve et une contrelecture de sources privées. Sa lecture initiale n'a lancé aucun build ou moteur. Son § 8 examine les nouveaux builds et reçus produits séparément par l'auditeur principal, sans les réexécuter dans cette contrelecture. Aucun test historique n'est réattribué au prototype et les certificats horizontal, vertical et du vote p3 ne sont pas rouverts. GCP non utilisé.

## 1. Octets et autorités

Les fichiers suivants ont été lus et rehashés. Les chemins `build/` désignent des artefacts privés locaux, pas des sources produit versionnées.

| Objet | SHA256 |
| --- | --- |
| `build/v7_meb_dual_budget_prototype/pivot.hpp` | `0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d` |
| `build/v7_meb_pivot_prototype/pivot.hpp` | `d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5` |
| [PROPOSITION_MEB_ET_BUDGETS.md](../docs/PROPOSITION_MEB_ET_BUDGETS.md), notamment §§ 2–6.2 et 7 | `365e7a5dcde5a6d6fcd7d43e00d2f58f86efbf42279a779c62c7eaac7b54ec25` |
| [silent_incidence.hpp](../src/forest/silent_incidence.hpp), référence F | `f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76` |
| [q3.hpp](../src/lanes/q3.hpp) | `4155a1c39193b68c47504e247a36e1bbf28b2c9ecbeeb50d6285d974519563fe` |
| [q4.hpp](../src/lanes/q4.hpp) | `58aac9bd57ac1a9b19ad156f6397941f67df1379e29215c50fcf268268491c4a` |
| `build/v7_meb_dual_budget_prototype/run_20260905/dependency_binding.json` | `1ce308a583b619ea2711d668b8bdfe307cb76caae9afee63d762b5a5eaf2f98e` |

Les 20 entrées du dernier fichier ont été comparées aux octets présents : aucune différence. Cette vérification lit la fermeture de compilation déjà enregistrée ; elle ne produit pas une compilation nouvelle. Le helper historique fournit ici `Candidate`, `point`, `form`, `ordinal` et `materialize`. Ses anciens `propose`, `small_ball` et `miniball`, dépourvus du second budget, ne sont pas appelés par la nouvelle route. Le `Builder` de repli est celui de F effectivement inclus, et non celui d'un ancien exécutable D.

Les mentions « uncompiled » conservées dans l'en-tête et le README privé décrivent leur préparation. L'exécution triangle ultérieure possède un reçu distinct, SHA256 `a7dc00201920a678c42e75436cb09ecf8a95b63dd660e587b814cdc0b4a1ea0a`. Elle est distinguée au § 7 de la preuve ci-dessous.

## 2. Domaine du théorème local

L'appel porte sur 2 à 11 positions géométriques distinctes dans $[0,65535]^3$, issues d'un `CloudIndex` valide. Les `sites[0:n]` sont des indices valides, sans répétition. Les formes sont construites depuis ces mêmes positions ; les mutants des primitives sont désactivés. Les objets de sortie et les pointeurs sont valides, distincts des temporaires ; les observateurs sont passifs. La route nominale a `ChargeAfter=false`.

Le domaine comprend les nuages à coquille non essentielle : ils doivent retrouver le refus scientifique de F, avec ses effets exacts sur la boule. Il ne s'étend pas à des `Candidate` artificiels, à une arité arbitraire ou à un `sites[]` invalide. `form`, `ordinal` et `materialize` sont des helpers internes, sans validateur autonome. Le garde n2..11 de `propose` ne transforme pas `miniball` en API validant n arbitraire, puisque son repli appelle directement F.

Les préconditions de budget sont séparées. Un même `Work` représente toute la tentative d'un ordre et reste vivant entre les appels, certificats et replis. Ses observations commencent à zéro, sauf compteurs injectés explicitement par une fixture. Les incréments de charges se raisonnent depuis l'état initial fourni ; un compteur injecté n'est pas une mesure d'un travail réellement exécuté.

## 3. Certificat, unicité et premier support de référence

### 3.1. Invariant de forme positive

Le `form` historique, lignes 33–60, trie les slots **avant** de construire la forme. Leur ordre est celui des positions dans `sites[]`, pas l'ordre Morton ni l'ordre numérique des PointId. Il produit : une paire diamétrale distincte ; un triangle strictement aigu de Gram positif ; ou un tétraèdre de déterminant non nul et de centre strictement intérieur.

Dans chaque cas, le support S est affinement indépendant, son centre c est une combinaison barycentrique strictement positive de S et ses q sommets ont puissance exactement nulle. Les puissances q2, q3 et q4 sont des multiples strictement positifs de $\lVert z-c\rVert^2-R^2$. Pour q4, `q4_form` canonise le signe conjoint de det et des numérateurs de Cramer : det est toujours non négatif. Le test privé `det<=0` est donc identique au test F `det==0` sur une vraie forme ; il ne perd aucun tétraèdre d'orientation négative.

L'identité $\sum_{s\in S}\lambda_s\lVert s-y\rVert^2=R^2+\lVert c-y\rVert^2$, avec $\lambda_s>0$ et $\sum_s\lambda_s=1$, montre que cette boule est l'unique MEB de S. Si elle contient le nuage local P, elle est aussi son unique MEB. Cette preuve utilise la positivité, pas seulement la contenance.

### 3.2. Conservation par les pivots

La paire initiale est distincte. À chaque pivot, le site extérieur z a puissance strictement positive, donc ne peut être un slot du support courant. Le sous-ensemble S union {z} possède ainsi q+1 slots distincts, entre trois et cinq. Les boucles de `small_ball`, lignes 79–97, en énumèrent les supports de tailles 2, 3 et 4 ; leurs positions sont triées par `form` même lorsque z est ajouté hors ordre.

Une réussite de `small_ball` conserve une forme positive et vérifie la contenance de tout ce sous-ensemble avant d'écrire le candidat. Sa boule est donc sa MEB. Son rayon augmente strictement : un rayon inférieur contredirait l'optimalité de l'ancienne MEB de S ; un rayon égal imposerait son ancien centre, qui ne contient pas z.

L'existence d'une base minimale affinement indépendante de la MEB, avec coefficients strictement positifs, assure qu'une énumération entière de ces supports trouve une solution sur les positions distinctes, si elle n'est pas interrompue par le budget. En dimension trois, cette base possède au plus quatre sommets. Une coquille non essentielle intermédiaire ne détruit pas l'invariant : la base positive choisie suffit. La régularité est exigée au certificat final.

La stricte croissance exclut une répétition de base. Le domaine local possède au plus 550 supports candidats ; une descente sans le cap de coût ne pourrait donc faire plus de 549 changements de base réussis. Cela **ne prouve pas une convergence en 16 pivots**. Le prototype conserve son cap 16 et replie F au-delà, y compris si un appelant demande davantage.

### 3.3. Coquille finale et unicité du support

Dans `propose`, lignes 121–131, une interruption du scan sur un extérieur déclenche un pivot. La branche sans extérieur a donc parcouru **tous** les sites. Les q sommets du support étant déjà garantis sur la sphère, `shell==q` prouve que la coquille entière est exactement S. Aucun tableau de membership supplémentaire n'est nécessaire dans cette route interne ; cette conclusion ne vaudrait plus pour des slots ou formes injectés sans contrôle.

Si F acceptait un support positif T contenant P, sa boule serait la même unique MEB. T serait inclus dans la coquille S. Or c a des coordonnées barycentriques uniques et strictement positives dans le simplexe S : aucun sous-ensemble propre de S ne peut porter c. Ainsi T=S. Il n'existe aucun support accepté avant S dans l'énumération F.

Le certificat régulier ferme donc simultanément l'optimalité, l'unicité du support, le premier accepté et le succès du contrôle final de coquille F. Il reste valide si la proposition a essayé ses bases dans un tout autre ordre. Les preuves de puissances et de largeurs déjà fermées dans [ARITHMETIQUE_LANES_COURANTE.md](ARITHMETIQUE_LANES_COURANTE.md) et [ARITHMETIQUE_LARGE_COURANTE.md](ARITHMETIQUE_LARGE_COURANTE.md) sont consommées avec leurs préconditions ; aucun nouveau flottant, changement d'échelle ou type large n'est introduit.

## 4. Ordinal et identité littérale de la boule

Le helper `ordinal`, lignes 134–143 du fichier historique, part de un, ajoute toutes les arités inférieures, puis compte les blocs lexicographiques dont la première différence précède le slot sélectionné. Pour les slots croissants $h_0<\cdots<h_{q-1}$, il calcule exactement l'ordinal R de F, y compris les candidats rejetés pour rang, acuité, positivité ou contenance.

On a $1\leq R\leq\binom{11}{2}+\binom{11}{3}+\binom{11}{4}=550$. Les arguments `n-slot-1` sont non négatifs, car les slots proviennent des boucles internes valides. `choose` effectue une récurrence binomiale entière ; pour n<=11 et k<=4, chaque valeur est au plus 330 et le produit avant division est au plus 3630. Il n'existe donc ni dépassement ni division tronquant une valeur non entière. Les frontières n=11 sont 1,55,56,220,221,550 ; une énumération indépendante doit les exercer sur le helper compilé de cette variante.

`materialize`, lignes 146–163, emploie le tuple canonique exactement comme F : clé et niveau q2, clé Gram et niveau rationnel réduit q3, clé réduite et `q4_level_raw` q4. `LocalBall` possède des initialisations par défaut pour tous ses champs ; les entrées inutilisées du support valent zéro comme dans F. Le premier support, notamment `support[0]` utilisé par la descente silencieuse, est conservé.

Pour q4, la preuve est plus forte que l'égalité de rayon. Le même tuple canonique fournit la même ancre a, les mêmes entiers det et N', puis exactement les trois limbs de $\lVert N'\rVert^2$ et le dénominateur $\mathrm{det}^2$. La réduction de la clé ne sert jamais à reconstruire le niveau. Aucun niveau rationnel réduit n'est substitué au niveau brut de F. Le raccourci conserve ainsi ses champs littéraux ; la sonde indépendante du § 8 les compare directement.

## 5. Deux budgets, priorité et repli

### 5.1. Charge prospective des formes proposées

Noter p le compteur effectif et P son plafond. `charged_form`, lignes 49–54, refuse si p>=P ; sinon il incrémente p **avant** l'observateur et `form`. Les rejets d'acuité ou de rang sont donc chargés. Le garde démontre la sûreté de l'incrément même pour P=`UINT64_MAX`. L'épuisement se propage par `Attempt::kExhausted` à travers toutes les boucles de `small_ball` et ne provoque aucun nouvel essai de forme.

Si p>=P dès l'entrée, `miniball` replie avant d'appeler `propose`, donc avant la recherche de paire extrême. Si P s'épuise pendant un appel, aucun essai de forme suivant n'a lieu ; la proposition peut finir de certifier la dernière forme déjà chargée. Ce dernier travail est la finalisation du candidat payé, pas un essai supplémentaire. Le mutant privé `ChargeAfter=true` viole précisément la causalité, même lorsque tous ses compteurs finaux coïncident.

Le cap 16 donne au plus $1+16(\binom{5}{2}+\binom{5}{3}+\binom{5}{4})=401$ formes proposées par appel, indépendamment du plafond global. La recherche de paire effectue au plus 55 distances, chacune au plus $3\times65535^2<2^{34}$ en i64. Les scans font au plus $17\times11=187$ puissances globales et $16\times25\times5=2000$ puissances sur sous-ensembles. Ces bornes décrivent ce code nominal ; elles ne constituent ni une durée ni une mesure de RAM et n'incluent pas le travail propre de F.

### 5.2. Ordinal legacy et effets publics

Noter c la charge legacy initiale et L son plafond. La priorité du test `c>=L`, lignes 152–156, est celle de F : incrément d'un seul `meb_calls`, refus `silent_meb_support_budget`, boule et c inchangés, aucune proposition. Le cas artificiel c>L reste inchangé au lieu d'être ramené à L.

Après un certificat valide, le prototype compare R à L-c avant toute addition. Si R>L-c, F aurait consommé exactement le reste du plafond sans atteindre son unique accepté : les deux routes mettent c à L, refusent le budget et laissent la boule sentinelle intacte. Si R=L-c, le dernier candidat est admissible au plafond : les deux réussissent. Si R<L-c, les deux ajoutent R et réussissent. L'addition autorisée reste au plus L, même lorsque L vaut `UINT64_MAX`.

La boule n'est écrite qu'après cette admission. Les statistiques étrangères, les événements, le statut et la raison artificiels d'un appel qui réussit ne sont pas réinitialisés. `certified` compte les certificats trouvés avant cette décision : il ne signifie pas « succès public ». `meb_calls` est incrémenté une seule fois, dans la voie certifiée ou par le vrai F dans la voie de repli.

### 5.3. Repli intact et borne cumulée

La proposition n'a accès en écriture qu'à ses temporaires et à `Work`. Lorsqu'elle échoue par coquille, budget P, absence de solution ou cap de pivots, les lignes 163–165 appellent un `Builder` F frais sur les mêmes index, événements directs, caps, sortie et boule. Le constructeur de ce `Builder` ne fait que lier ses références ; `miniball` ne dépend pas d'autres états accumulés du Builder. L'absence de résultat proposé ne devient donc jamais un nouveau refus scientifique.

Pour le carré local `(0,0,0),(2,0,0),(2,2,0),(0,2,0)`, la boule des diagonales a quatre sommets sur sa coquille. La proposition q2 échoue au certificat final, et F retrouve son premier support aux slots 0,2 : refus budget aux caps 0/1, puis `silent_local_nonessential_shell` avec boule déjà écrite à partir du cap 2. L'autre diagonale ne peut ni changer l'ordinal public ni transformer ce refus en succès. La contre-fixture historique reste applicable ; elle n'est pas présentée ici comme un nouveau reçu exécuté.

Depuis des compteurs nuls, chaque candidat réellement essayé par un repli F consomme une charge legacy. Une réussite accélérée ajoute R charges logiques positives sans essai F. Chaque forme proposée consomme exactement une charge P. Ainsi, si A désigne les candidats des replis et B les formes proposées, $A+B\leq\text{meb\_supports}+\text{meb\_proposal\_supports}\leq L+P$. L+P est une borne mathématique, sans addition C++ potentiellement débordante. Depuis des compteurs non nuls, la même preuve porte sur leurs incréments.

Plus précisément, chaque certificat ajoute le préfixe virtuel $\min(R,L-c)$, y compris lorsqu'il se termine en refus legacy. L'incrément total legacy est exactement A plus la somme de ces préfixes. La [contrelecture budgétaire séparée](receipts_meb_dual_20260905/budget/review.json) ferme cette attribution ; elle rappelle que le reçu triangle proche de MAX exerce un repli, et non le commit accéléré à cette frontière.

Cette borne concerne les candidats, pas les prédicats, instructions ou temps. Elle exige le même `Work` sur tous les appels de l'ordre. Le helper ne le recrée jamais, mais la future intégration doit en imposer la durée de vie. Les caps des différents ordres de la tour s'additionnent ; ils ne deviennent pas implicitement un cap partagé. Les observations `pivots`, `certified` et `fallback` ne définissent pas de nouveaux plafonds publics.

## 6. Portée constructive pour le raccord produit

Les arguments précédents ferment la conservation **locale** au remplacement du seul `Builder::miniball`, avec F pour repli et les primitives épinglées. La variante n'exige aucune mosaïque de Delaunay, aucun Gamma global, ni stockage de cofaces supplémentaires. Son état de proposition est fixe : au plus onze sites, quatre slots de support et cinq slots de sous-ensemble.

Le prochain raccord concret peut conserver ce découpage : `Work` et `Limits` possédés par la tentative de l'ordre, opt-in explicite avec P=0 par défaut, identifiant `reference_ordinal_plus_proposal_v1`, puis publication séparée des charges ordinales et effectives. Le vieux sens de « supports locaux essayés » ne doit pas rester associé au seul compteur legacy après activation. Les plafonds et reçus historiques ne sont pas réinterprétés rétroactivement.

Le repli doit rester une méthode de référence sans proposition, par exemple `miniball_reference`. Le `Builder` frais du prototype est correct aujourd'hui parce qu'il désigne F inchangé. Recopier ce même appel à `Builder().miniball` après avoir remplacé la méthode produit pourrait rappeler la proposition ou recréer son `Work`. La future intégration doit donc préserver la séparation effective des deux fonctions, en plus de la propriété mathématique du repli.

Les objets géométriques locaux sont conservés par la preuve ; les nouveaux champs de configuration, leur portée par ordre et le chemin de publication restent des obligations d'intégration. Cela appelle des tests du delta consommateur, sans remettre en question les preuves déjà closes sur les objets qu'il reçoit.

## 7. Reçus privés et obligations de qualification actualisées

Le reçu privé triangle déjà clos observe huit combinaisons de caps, quatre appels cumulatifs et une frontière MAX, avec conformité des terminaux F. Son mutant charge-après produit 28 violations causales contre zéro en nominal. Ces observations portent sur le triangle d'ordinal 4 dont la proposition complète essaie cinq formes. Elles établissent le cas physique qui réfutait le prototype ordinal seul ; elles ne testent aucun q4. La capture et son jugement portable font l'objet de la contrelecture indépendante coordonnée, sans réexécution dans la présente note.

Le reçu géométrique constructeur clos le 5 septembre à 11:28:42 UTC a depuis été capturé et contre-vérifié, en [mode normal](receipts_meb_dual_20260905/geometry_constructor/captured_normal.json) et [Python -O](receipts_meb_dual_20260905/geometry_constructor/captured_optimized.json). Il porte sur le gate `c9971f8c340fe37eea2be824897110d436a38345e1d66d1834c9eb7f489bb1a9`, le runner `b04dc2a69aec60c6c5e41e83688588a3b963a0ba5f4e91260580e4d195bda727` et le reçu privé `b81d8e480b158710874de230c3485f79d0a42f1cb228e321c750de0f58bed49e`. Ses 176 scènes et 384 ordres produisent 9 216 comparaisons principales, 123 comparaisons de frontières et 1 507 contrôles d'ordinal. Les 384 lectures pilotes de rang F sont séparées de ces 9 339 comparaisons. Le nominal rend zéro violation prospective ; le mutant charge-après rend 46 437 violations, code 4, avec les mêmes autres résultats. Aucun build ou essai C++ n'est répété par cette contrelecture.

Cette campagne constructeur instancie **`Trace`** : elle compare le booléen, le diagnostic, les statistiques, la boule et le niveau littéraux à F, conserve des événements sentinelles non vides, et vérifie leur absence de mutation pendant la proposition. Son juge géométrique est différentiel à F et partage ses primitives. Le § 8 apporte séparément le juge rationnel indépendant. Le bridge de ce dernier utilise lui aussi un observateur, son propre `Observer`. Ni `Trace` ni cet `Observer` ne sont l'instanciation native `NoObserver`. Cette dernière possède désormais sa [contrelecture propre du reçu v2](receipts_meb_native_20260905/README.md), avec comparaison complète avant/après mesure ; les résultats ne sont pas transférés entre binaires.

Les obligations identifiées à la lecture initiale portaient sur ce nouveau code. Leur état après ces reçus et le § 8 est le suivant :

1. Les 1 507 ordinaux valides, dont les frontières 55/56 et 220/221/550, sont rejugés par énumération indépendante sur les deux builds du § 8 et dans la campagne constructeur. La faute d'offset +1 dans la voie rapide est réfutée au § 8. Le constructeur appelle aussi la vraie scène q4 d'ordinal 550 aux caps L=549,550,551 ; son parcours rapide se raccorde explicitement ci-dessous. Cette demande n'est plus ouverte.
2. Le raccord de la vraie voie rapide au juge MEB rationnel indépendant est désormais exercé en q2/q3/q4, dans l'ordre d'entrée et son renversement. Le support complet, la clé et le niveau q4 brut sont comparés à l'oracle et à F. Cela n'est pas une énumération de tous les nuages ou de toutes leurs permutations.
3. Les replis par P et coquille, les caps L avant/à/après R et les sentinelles sont exercés au § 8. Le constructeur ajoute cinq appels avec `pivot_cap=0`, dont trois replis effectivement observés, des charges legacy initiales non nulles, des états c>=L et p>=P, les frontières MAX et le cumul avec le même `Work`. Les quatre appels P7/L12 retrouvent p=(5,7,7,7), c=(4,8,12,12), et les demandes de caps de pivots 17 et `SIZE_MAX` restent bornées à 16. Ces demandes sont satisfaites par ce reçu distinct ; les limites propres au bridge du § 8 ne sont plus des lacunes globales.
4. La suppression du contrôle de coquille, une faute d'ordinal et la modification du niveau q4 brut sont désormais réfutées dans des copies privées de la voie proposée. Le mutant prospectif conserve sa porte séparée. Aucun de ces résultats n'est un mutant déjà intégré au produit.
5. À l'intégration seulement, vérifier les consommateurs du nouveau schéma/caps, l'absence de remise à zéro par MEB ou repli et le raccord du remplacement local aux sorties déjà qualifiées. Une campagne privée de helpers n'est pas un reçu produit ou une mesure de gain de tour.

Le raccord des cas rapides proches de MAX mérite une attribution exacte. La boucle de frontières appelle la paire et le triangle équilatéral avec P401, p initial nul, c=MAX-1 ou MAX-4 et L=MAX. Elle compare tous les terminaux à F, sans compteur imprimé de certificat par cas. Les mêmes points, slots et paramètres de proposition produisent obligatoirement un certificat dans les cas `named_fast` ; `propose` ne lit pas c, et la garde préalable c>=L est fausse dans les trois cas concernés. Par cette non-interférence vérifiée dans les sources, le succès q2 à c=MAX-1, le succès q3 à c=MAX-4 et le certificat q3 suivi d'un refus à c=MAX-1 empruntent bien la voie rapide. Cette conclusion combine les appels observés et la preuve de branche ; elle n'invente pas un compteur spécifique absent du reçu.

Pour la scène d'ordinal 550, les sept premiers sites sont (4,4,4) et ses six voisins axiaux à distance un ; les quatre derniers sont (0,0,0),(8,8,0),(8,0,8),(0,8,8), aux slots 7..10. Le pilote F impose explicitement ce support q4 et le rang 550. La proposition choisit la paire 7,8, de centre (4,4,0), puis l'intrus 9 donne le triangle de centre $(16/3,8/3,8/3)$ ; l'intrus 10 donne enfin le tétraèdre de centre (4,4,4). Les sept sites initiaux sont contenus dans les boules intermédiaires et strictement intérieurs à la boule finale. La résolution locale essaie une forme initiale, quatre formes pour le premier pivot, puis onze pour le second : seize formes au total. Les appels à P16,25,401 aux caps L549/550/551 exercent donc le certificat rapide et son refus ou admission ordinal. C'est une déduction du chemin déterministe sur la fixture effectivement appelée, distincte des compteurs rapides agrégés.

La combinaison particulière c=MAX-550, L=MAX, support q4 de rang 550 ne figure pas dans ce corpus ; elle reste une limite descriptive, sans annuler les frontières rapides q2/q3 ni la preuve générale des gardes du § 5. Les nombres constructeur ne sont pas transférés à la campagne indépendante du § 8. La preuve statique, le différentiel constructeur sous `Trace`, l'oracle rationnel sous `Observer` et la qualification native ultérieure apportent des autorités complémentaires. L'intégration produit reste à qualifier. Aucun statut public n'est promu.

## 8. Qualification locale indépendante et contrelecture du juge

Le [bridge](meb_dual_bridge.cpp) compile des copies privées épinglées du prototype à deux budgets, du helper historique et de F. La [campagne et son juge](meb_dual_oracle.py) utilisent le corpus et les fonctions rationnelles de [l'oracle MEB indépendant](meb_rational_oracle_20260905.py), réexécutés sur ces nouveaux binaires. Le code historique du `main` de cet oracle n'est pas lancé. Les anciens reçus MEB ne deviennent pas des reçus du prototype.

Le [reçu d'exécution](receipts_meb_dual_20260905/geometry/run.json) conserve les compilations C++20 strictes `-Wall -Wextra -Wpedantic -Werror` en O2 et O1/UBSan, sans `MHGP7_TESTING`, les depfiles, les hashes de binaires, stdin/stdout/stderr et les codes. Les snapshots inclus sont sous `audits/.work_meb_dual/` ; le produit F est uniquement lu. Les terminaux nominaux sont rejugés dans les reçus [normal](receipts_meb_dual_20260905/geometry/normal.json) et [Python -O](receipts_meb_dual_20260905/geometry/optimized.json), avec les mêmes compteurs et rejets. Les chiffres suivants valent **pour chacun** des deux builds, pas pour deux corpus indépendants :

| Observation | Nombre |
| --- | ---: |
| Nuages distincts / ordres locaux entrée et renversement | 89 / 178 |
| Appels MEB | 3 430 |
| Ordinaux combinatoires | 1 507 |
| Succès rapides q2 / q3 / q4 | 416 / 310 / 64 |
| Refus de budget legacy / refus de coquille | 1 650 / 40 |
| Replis F | 1 459 |
| Certificats suivis d'un refus legacy | 291 |
| Formes effectivement proposées | 8 509 |
| Appels au cap legacy nul, sans proposition | 890 |
| Appels au budget de proposition nul | 686 |

Les caps P sont 0,1,4,5,401. Pour chaque ordre, les caps L sont les valeurs distinctes parmi 0,R-1,R,R+1, avec R calculé par l'oracle rationnel. L'oracle ne demande donc pas à F de choisir les caps, le support ou l'ordre de recherche transmis au proposeur. Les statistiques étrangères sont initialisées à des valeurs non nulles ; la boule sentinelle, le statut artificiel conservé au succès, les treize statistiques publiques et le support complet sont contrôlés.

La contrelecture du juge confirme l'indépendance du calcul attendu : élimination rationnelle du système de Gram, coefficients barycentriques strictement positifs, test de contenance de tous les sites puis comptage de coquille. L'ordinal attendu provient d'une énumération lexicographique Python, sans appel à `choose`. La clé primitive est reconstruite depuis le centre et le rayon rationnels, par dénominateur commun et PGCD. Pour q4, le déterminant est calculé par élimination rationnelle ; le centre relatif multiplié par ce déterminant fournit séparément le numérateur brut attendu et son carré fournit le dénominateur. Ce contrôle ne se contente pas d'une égalité rationnelle avec F.

La lecture des stdout scellés constate, par build, 44 succès rapides q4 dont le troisième limb du numérateur est non nul et 114 succès rapides ayant au moins deux pivots. Les maxima observés sont quatre pivots, 49 formes proposées par appel et 465 pour l'ordinal d'une MEB rapide. L'ordinal 550 est donc exercé ici comme tuple combinatoire, sans prétendre qu'une scène atteint cet ordinal dans la voie MEB rapide.

Trois fautes sont appliquées uniquement à des copies privées sous audits ; leurs patches et sorties sont conservés :

| Faute injectée | Premier rejet du juge indépendant |
| --- | --- |
| Retrait de `shell==q` | `oracle.terminal.121` |
| Ordinal chargé augmenté de un | `oracle.terminal.6` |
| Numérateur et dénominateur q4 doublés ensemble | `oracle.q4_raw_level.84` |

La troisième faute conserve le rayon rationnel ; son rejet vérifie donc effectivement l'autorité de la représentation brute. La première est arrêtée sur un vrai refus de coquille attendu. Pour la deuxième, le cap à R fait refuser le prototype avant le succès attendu, tandis que son compteur est déjà saturé au même plafond : le premier écart est bien le terminal et non une statistique.

Le [premier driver conservé](receipts_meb_dual_20260905/geometry/initial_driver.py.txt) attendait à tort cette dernière faute sous la catégorie `oracle.stats`. Le [reçu d'échec initial](receipts_meb_dual_20260905/geometry/initial_judge_rejection.json) est conservé. La correction de classement rejoue les mêmes sorties compilées ; elle ne modifie ni les binaires ni leurs observations. L'échec initial appartient au juge et n'est pas masqué comme une défaillance géométrique de F ou du prototype.

Le driver final vérifie les hashes des stdin reconstruits, en plus des sorties conservées. Sa voie `--destination` permet une campagne neuve dans un autre sous-dossier d'audits sans reprendre le reçu clos. La [contrelecture des dépendances](receipts_meb_dual_20260905/geometry/dependency_review.json) lie les 20 dépendances locales de chacun des cinq builds aux snapshots F, aux deux helpers scellés ou au patch mutant déclaré, et vérifie l'identité des cinq binaires encore présents. Cette liaison est une vérification locale des captures, sans prétention de build hermétique.

| Artefact final | SHA256 |
| --- | --- |
| `meb_dual_bridge.cpp` | `91880523987eb0dae952c3123f9baf4bdc33974cfe29ecfed649b787de20f23f` |
| `meb_dual_oracle.py`, driver de rejeu final | `f5c277e24e077d02b3426ce7973954503d6b00c536cb329e63d368e73046716a` |
| `geometry/run.json` | `ed7047733252ed091610f6dcc4bb3cd733bd450bbc8a09a26ce4c64615f4c914` |
| `geometry/normal.json` | `8afe1c57ae3dc1fca3c87f8b8e6a03b41d61b51f1fa38792d4fde5089f581eeb` |
| `geometry/optimized.json` | `851ea40daae50832b18ba6a81398806c50ed08b9cdfdbfd7e5ac335450288403` |
| `geometry/initial_driver.py.txt` | `95a00fa8258622ae5b467ab5888e1ae93c63dde6edec1789b0aad896ee7e4aee` |
| `geometry/initial_judge_rejection.json` | `ded8c4834ab72d7631d82546c6936273cdbb3cf4dc346a3abf262df33780bc41` |

Les limites de cette porte sont explicites. Chaque commande M part d'un `Work` frais et d'une charge legacy nulle : le cumul par ordre et les frontières MAX ne sont pas mesurés à nouveau. Le bridge conserve le cap de pivots par défaut et ne force pas son épuisement. L'observateur vérifie la charge avant chaque forme, mais la porte causale `ChargeAfter` demeure celle du triangle. Les trois mutants géométriques sont des modifications d'audit, sans intégration dans le catalogue produit. Le succès de ces contrôles ferme un raccord local effectivement compilé au corpus indiqué, sans établir un gain de temps, une qualification de tour ou une API publique nouvelle.

## 9. Qualification native et décision de coût

La [contrelecture du reçu natif v2](receipts_meb_native_20260905/README.md) ferme son instanciation `NoObserver` sur les octets privés : 9 351 états sont comparés complètement à F et à `Trace`, avant puis après les mesures. Le contrôle du désassemblage confirme l'appel du helper et la consommation de chaque résultat entre les horloges. Les captures 64 bits des boucles mesurées relient ces appels aux attendus ; elles restent distinctes des comparaisons complètes hors chrono. Aucun nouvel appel moteur n'est exécuté par cette contrelecture.

Sur les 1 152 appels principaux à P401, le nombre de candidats réellement essayés, propositions et replis inclus, vaut 10 722 contre 67 884 dans F. Ce raccourci physique est utile : 795 appels diminuent ce compte, 159 le conservent et 198 l'augmentent. La somme ne mesure pas les puissances, les distances, les copies ou le temps ; elle n'est pas la distribution d'une tour.

Les mesures de temps apportent deux contraintes concrètes. Le cas de deux points répété ralentit avec la proposition, et le contrôle P0 des petits lots dépend de l'ordre F/dual. Les 4 699 groupes ne constituent donc pas autant d'estimations fiables d'un coût isolé. Le coût inclut reset, sentinelles, wrappers et captures ; ni une accélération générale ni un seuil de dispatch ne sont établis. P=0 reste le défaut raisonnable du port.

Le [protocole de suivi déjà préparé](receipts_meb_native_20260905/followup_review.json) traite ces limites : tous les 384 ordres conservés, P0/P401, L551 uniforme, 64 répétitions fixes, deux chauffes et dix paires équilibrées. Son plafond conservateur de 1 779 072 entrées, replis et juges compris, est inférieur à deux millions. Cette contrelecture qualifie le plan, sans lui attribuer de build ni de mesure. Les résultats devront conserver P0 et AB/BA séparés, les strates défavorables et l'interdiction de sélectionner un seuil après les mêmes observations. Les répétitions repartent d'un Work frais ; le coût du budget partagé par ordre demeure une question d'intégration.
