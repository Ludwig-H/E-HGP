# q2 : un certificat autonome avant de fragmenter les requêtes

14 septembre 2026. Neuvième tranche, `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`. Suite du [raccord front–census](P0_FRONT_ET_CENSUS_Q2.md),
pas une tour FULL ni un moteur GPU.

## Idée et preuve locale

Le census traite une ancre a et un groupe B de partenaires. Quand le
compte reste indécis, il divise parfois B avant d'avoir rencontré les
témoins qui permettraient de rejeter beaucoup de ses paires. Le parcours
DFS global impose cet ordre ; la fixture a=1000, B=0..63 en montre le
coût, même avec une seule ancre. Partager plusieurs ancres ne suffit donc
pas à résoudre ce mécanisme.

Après B→B_L/B_R, chaque enfant essaie son frère comme groupe de témoins.
Pour q2, $H(a,b,z)=(z-a)\cdot(b-z)$ est strictement positif exactement
dans l'intérieur de la boule de diamètre ab. Si le frère contient au
moins K sites et si la borne certifiée de H sur toutes les paires
(b,z) de l'enfant et du frère est strictement positive, chaque paire
de l'enfant possède déjà K intérieurs distincts. Tout l'enfant est rejeté.

La borne vient des mêmes constantes préparées exactes que le census ;
pour un enfant singleton, le chemin boule–boîte est conservé. Les sites
du propriétaire sont uniques, les nœuds et leurs populations proviennent
du même index immuable. Aucune hypothèse d'alignement n'est requise pour
la sûreté du test. Les boîtes alignées sur les axes peuvent néanmoins
changer son efficacité après rotation ou réflexion.

Ce certificat est **autonome** : il doit atteindre K à lui seul. Il
n'ajoute jamais sa population au compte hérité et ne change jamais le
curseur Z. Aucun crédit partiel n'est conservé. Même un chevauchement
avec des sites déjà comptés ne peut donc produire un double crédit.
Si le test échoue, le census reprend exactement son compte et son
curseur non consommé ; la collecte des intérieurs et de toute la
coquille reste inchangée. Remplacer Hmin>0 par Hmin≥0 serait incorrect.

L'[auditeur A](../audits/q2_sibling_20260914/README.md) établit une
extension distincte : les sites déjà crédités uniformément sur le parent
B sont hors de B (choisir b=z donnerait H=0). Le frère est donc disjoint
de ces témoins stricts et peut fournir immédiatement les K−c témoins
restants. Cela autorise un **rejet immédiat**, pas le préchargement d'un
crédit conservé dans une continuation. Cette extension n'est pas le mode
Saturating qualifié ici ; ne pas transformer son prototype en résultat
produit hérité. Elle ne supprime pas le verrou du nombre de tâches.

## Implantation et travail réellement payé

`run_wspd_q2_census` accepte une option finale `Q2SiblingMode` :
`Disabled` reste le défaut ; `Saturating` est réservé à `SharedBlocks`.
Un mode invalide ou Pairwise+Saturating est rejeté avant toute émission.
Les autres entrées census et les champs de `Q2CensusWork` sont inchangés.
Le chemin Disabled compile sans le test expérimental.

Un enfant reçoit simplement l'ID de son frère. Pas de recherche de
témoins, de copie de population, de liste d'exclusion ni d'allocation.
Une proposition coûte une comparaison de cardinal et, s'il suffit, un
test de boîte en dimension fixe. Les constantes du groupe B sont déjà
préparées pour le census. Les six compteurs supplémentaires sont stockés
une fois par appel ; ils ne sont pas une nouvelle table par rectangle.

Les identités contrôlées sont :

- tâches = racines + 2 × divisions B ; propositions = 2 × divisions B ;
- propositions = cardinaux insuffisants + tests de bornes du frère ;
- tâches rejetées ≤ tests de bornes ; paires rejetées par frère ≤ rejets census ;
- compteurs du frère tous nuls en Disabled.

`count_node_visits` ne contient pas les nouveaux tests du frère. Les
mesures publient donc séparément ces visites, les propositions et leurs
tests de bornes, les tâches et le temps total. Diminuer un compteur en
déplaçant son travail dans un autre ne constitue pas un gain.

Le surcoût est O(1) par enfant effectivement créé ; cette propriété
locale ne borne pas le nombre d'enfants. Le front, le nombre d'ancres
et la masse des candidates sont identiques entre les deux variantes.
Cette option ne garantit aucune complexité globale sous-quadratique.

## Qualification et décision

La [qualification propre à cette tranche](../receipts/q2_sibling_20260914/README.md)
compare les supports, les clés et toutes les populations
d'intérieur/coquille à un oracle indépendant sur petits nuages :
1 449 appels et 64 725 supports, 44 CTests Release et Clang ASan/UBSan.
Les 32 mesures comparent s8/10/12 à 8k et la croissance à s8 jusqu'à 32k.
La fixture alignée est un détecteur de fragmentation, pas un modèle
représentatif des amas ou du LiDAR. Les témoins de la huitième tranche
restent épinglés ; aucune qualification n'est héritée par cette option.

Décision : garder Disabled par défaut et l'option expérimentale. Les
rangées gagnent nettement (1,494→0,241 s à 8k ; 0,969 s à 32k avec
le frère), mais les amas gardent ×4,29 puis ×4,22 sur visites+tests
du frère. Le gain local ne résout donc pas P0. Les propositions et les
tests sont payés, même quand aucun rejet nouveau n'est possible.

Pour la parallélisation future, la proposition utilise uniquement l'état
local et l'index partagé en lecture seule. Les compteurs et buffers
restent propres à l'appel. La récursion actuelle n'est toutefois ni une
file suspendable, ni un lancement multi-CPU/GPU : ces étapes restent à
implémenter après choix des objets et mesure du travail total.

## Prochaine expérience : choisir un ordre utile sans liste de frontières

Le frère traite certains rejets locaux, pas le blocage du parcours Z.
Une proposition plus compacte qu'une liste persistante de témoins est
maintenant contre-vérifiée, **non implémentée dans cette tranche** :
visiter d'abord le complément du B original de la requête, puis ce B,
en retirant l'ancre a du seul comptage d'intérieur. L'ancre reste bien
présente dans la collecte de coquille, puisque H(a,b,a)=0.

L'ordre doit être fixé pour toute la descendance de la requête, pas
recalculé selon chaque enfant B. Son contexte immuable garde le nœud
B original, son échappement et le rang spatial de a (déjà connu dans
la boucle d'ancres, sans table inverse). La continuation garde phase,
curseur et compte. Pendant la phase complément :

1. Forcer la descente des ancêtres propres du B original avant tout test
   de borne : on ne doit pas consommer un bloc qui inclut le groupe différé.
2. Au nœud B original, sauter vers son échappement ; à la fin de l'index,
   entrer dans la phase B original et terminer à son échappement.
3. Forcer également la descente des nœuds contenant le rang de a, puis
   sauter sa feuille, uniquement pour le comptage des intérieurs.
4. À chaque division des requêtes B, transmettre le même contexte et
   la même continuation. Le compte reste celui du préfixe résolu de
   cet ordre spécifique, sans recomptage ni préparation quadratique.

Le simple report de B ne suffit pas. Contre-fixture 3D : B={0,1,2,3}³,
a=(1000,1000,1000), douze témoins W={997,998,999}×{998,999}², K10,
front Pure, s12. Tout W est strictement intérieur pour toutes les
paires a×B. Pourtant le bloc {a}∪W est petit et indécis à cause de a ;
la politique de diagonales actuelle peut donc diviser B avant de voir W,
même si B a été reporté. L'exclusion structurelle de a expose ces témoins.
Un prototype doit rejeter la requête de cette ancre sans division B,
tout en retrouvant l'oracle complet sur l'ensemble du nuage.

Coûts à distinguer : descentes structurelles, tests géométriques, phases,
tâches B, visites totales et temps. L'exclusion d'une feuille et le report
d'un sous-arbre sont compacts ; les autres blocs extérieurs mixtes peuvent
toutefois rester coûteux. Ni cette preuve de conservation ni cette fixture
ne donnent encore une borne de croissance. L'arène persistante, plus
générale mais plus coûteuse en état et allocations, n'est pas retenue
avant cette expérience minimale.
