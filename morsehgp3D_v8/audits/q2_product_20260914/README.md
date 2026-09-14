# Census conjoint : relais exact et obstacle des boîtes du facteur A

14 septembre 2026 — auditeur A. Réponse à la proposition constructeur
après e3af11a7, dans `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Modèle Python et revue statique ; aucun binaire de la
tranche conjointe exécuté par cet audit. GCP non utilisé.

**Le partage A×B puis le relais vers l'ancre fixe sont sûrs avec la
continuation proposée.** Le modèle retrouve les admissions après crédit
et préserve toute la coquille. Un point nouveau explique cependant une
limite de la politique des diagonales : elle force la subdivision des
ancres avant de pouvoir consommer leur propre nœud A. C'est un choix
de raffinement à comparer, pas une nécessité du contrat de census.

## Préfixe commun et relais

Conserver deux nœuds disjoints A/B du même index, le B original B₀,
son échappement et l'état phase/curseur/compte. Tant qu'A est multiple,
l'ordre est le complément de B₀, puis B₀ ; aucun site de A n'est exclu.
Le compte c est exact et commun à toutes les paires A×B sur le préfixe
résolu. Un bloc Z strictement intérieur à toutes ces paires ajoute sa
population, un bloc dont le maximum est non positif ajoute zéro.
Une subdivision A/B partitionne le produit et conserve intégralement
le préfixe ; une subdivision Z ne consomme pas encore ce bloc.

Quand A devient la feuille a, reprendre le chemin à ancre fixe avec
le même B₀, la même phase, le même curseur et c. Le rang de a est
directement celui de cette feuille ; aucun parcours de recherche.
Activer alors son exclusion ne modifie ni le préfixe ni le suffixe en
termes de compte, puisque $H(a,b,a)=0$. Cette preuve couvre aussi un a
déjà résolu par une autre politique de raffinement. Aucun redémarrage,
second `root_start`, changement implicite d'ordre ou remise à zéro de c.

La collecte globale par support reste entière, y compris a, b et tous
les autres sites de coquille. Une admission en bloc ne dispense jamais
de cette sortie. Une boîte classée non positive pour le compte peut
contenir des sites de frontière.

## Invariant plus fort de la politique actuelle

Hypothèses précises : sites uniques, nœuds A/B/Z du même arbre global,
B₀ disjoint d'A, extrema exacts sur **boîtes continues**, division Z
seulement si sa diagonale au carré dépasse strictement celles d'A et B.
Depuis une racine neuve, le préfixe conjoint P reste alors disjoint
d'A courant : $P\cap A=\varnothing$ jusqu'au relais.

Preuve du verrou : une boîte A non singleton a une étendue w>0 sur
au moins un axe. Fixer b dans B ; prendre a à l'extrémité de cet axe
la plus éloignée de b, puis z à distance w/4 de a vers b, dans la boîte A.
Sur les autres axes, choisir z=a. Ces choix sont autorisés pour les
boîtes continues, même s'ils ne correspondent pas à un site du nuage.
La distance de a à b sur l'axe choisi vaut au moins w/2, donc
$H(a,b,z)\geq w^{2}/16>0$. Le choix z=a donne par ailleurs zéro.
Ainsi Z=A et tout ancêtre contenant A sont nécessairement indécis.

À Z=A, la diagonale Z égale celle d'A : la règle interdit de diviser Z
et impose une division A ou B. Les nœuds de l'arbre sont emboîtés ou
disjoints ; entrer dans un descendant Z propre d'A aurait exigé de
franchir ce nœud. Aucun bloc contenant un site d'A n'est donc consommé.
Une division A préserve l'exclusion pour ses enfants, une division B
ne change pas A, et le saut B₀ est disjoint d'A. Cela prouve l'invariant.

Conséquences pour les états accessibles par **cette politique** :

- L'ancre n'est pas encore consommée au relais singleton.
- Les transitions de phase conjointes sont nulles : finir le complément aurait exigé de consommer A.
- Aucune admission conjointe avant relais n'est accessible ; les admissions passent par le chemin à ancre fixe.

Ce ne sont pas des restrictions de l'objet mathématique. Conserver les
protections du relais et les champs de continuation pour les politiques
plus générales. Dans le modèle, `witness_first` exerce explicitement
l'ancre déjà consommée et l'admission conjointe ; ces cas ne doivent pas
être présentés comme une couverture d'intégration de la règle actuelle.

**Économie indépendante du changement de politique :** à Z=A, le test
numérique des bornes est redondant, car son classement indécis est prouvé.
On peut appliquer directement l'arbitrage de subdivision, sans consommer
Z et sans fabriquer de fausses valeurs d'extrema. Le même argument vaut
pour un ancêtre contenant A. Compter ces décisions connues séparément
des bornes réellement calculées. Cette économie doit être appliquée aux
deux bras d'une comparaison de politiques ; elle ne doit pas être
attribuée au seul changement de raffinement.

## Fixtures et coût du modèle

Fixture constructeur : A={(100,0,0),(100,4,0)},
B={(0,1,0),(0,2,0),(0,3,0)}, z=(50,2,0). A et B sont bien deux
nœuds globaux de l'arbre midpoint. L'ordre spatial est B, z, A.
Le minimum commun H(z)=2498 et les profondeurs cross sont [1,2,3]
puis [3,2,1]. K1 rejette les six paires ; K2 en admet exactement deux.

| Fixture / K | Ancres : tâches / visites | Produit : tâches / visites | Décision |
|---|---:|---:|---|
| Constructeur / 1 | 2 / 8 | 1 / 4 | Six rejets avant relais |
| Constructeur / 2 | 10 / 32 | 11 / 29 | Deux relais conservant chacun c=1 |
| B_split_first / 2 | 10 / 34 | 11 / 42 | Trois admissions ; davantage de travail conjoint |

Les tâches sont celles du modèle de continuation : un relais transforme
une tâche, sans en créer une seconde. Les compteurs C++ conjoint et
ancre ont des périmètres distincts. Les visites incluent les opérations
structurelles, mais pas le juge coûteux de préfixes ni la collecte,
comptée séparément. Le modèle utilise les extrema Python génériques :
un test conjoint n'est pas assimilé au coût d'un test à ancre fixe.

[r2_RUN.json](r2_RUN.json), [sources](r2_sources.zip) et
[résultats](r2_RESULT.json) ferment 36 configurations : cinq fixtures,
quatre également tournées par une transformation entière orthogonale
à facteur d'échelle, K1/2/5/10. Deux modes et trois ordonnancements
font 216 appels, plus six appels `witness_first` pour le relais après
consommation nulle de l'ancre. Les reprises budget 1 en FIFO/LIFO
conservent sorties et travail discret ; la collecte reste synchrone.

Six mutants sont réfutés : redémarrage au relais avec c conservé,
perte de c, B₀ redéfini, saut du Z ouvert lors d'une division, exclusion
de tout A et ancre supprimée de la coquille. Les quatre premiers
sont notamment réfutés par l'invariant de préfixe, avant émission.
La contre-fixture pleine dimension A={(0,0,0),(1,1,0)},
B={(100,0,1),(101,0,0)}, K1 rend le retrait de tout A manifestement
faux : l'autre A est le seul intérieur de la paire des premiers sites,
avec H98. Son déterminant vaut 101 ; aucun alignement exact n'est requis.

[r1_RUN.json](r1_RUN.json) reste un échec conservé : après rotation du
cube, les faces demandées n'étaient plus des nœuds globaux. Le r2 garde
ce cube non tourné, sans remplacer silencieusement ses facteurs.
[verify.py](verify.py) compare normal/−O au r2 et ajoute une fixture
d'admission conjointe sous `witness_first` : 16 supports du cube, 72 IDs
de coquille, aucun relais. Sa [clôture](r1_VALIDATION.json) distingue
ces six appels supplémentaires du comportement de la politique actuelle.

## Relaxation locale à comparer : descendre Z au nœud A

[self_node.py](self_node.py) change seulement l'arbitrage indécis à
Z=A courant : descendre Z avant de diviser un facteur. Aucun nouvel
ordre, contexte, crédit, tableau ou mécanisme de relais. La terminaison
est conservée, puisque le nœud Z descend strictement. Le
[reçu distinct](self_node_r1_RUN.json), ses [sources](self_node_r1_sources.zip)
et [résultats](self_node_r1_RESULT.json) conservent 228 appels sur
38 configurations. Les 36 références strictes reproduisent intégralement
le r2 ; les deux autres réfléchissent le constructeur suivant x↦100−x.

Sur ce reflet à K1, le témoin commun était rencontré après A : la règle
stricte fait trois tâches et onze visites, sans rejet conjoint ; la
relaxation fait **une tâche et six visites**, avec six rejets conjoints.
À K2, les deux admissions sont conservées, avec 31→26 visites et deux
relais après consommation nulle de l'ancre. Certains cas atteignent aussi
l'admission conjointe auparavant inaccessible.

Sur ces 38 configurations, les visites baissent dans 32 cas et restent
égales dans six. Mais le total des tests de boîtes conjoints et fixes
**augmente dans 26 cas**, baisse dans cinq et reste égal dans sept.
Exemple constructeur original/K2 : 29→26 visites, mais 17→18 tests
de boîtes. Ces compteurs ont en plus des coûts différents. La proposition
est donc une comparaison C++ à mener, pas un gain temporel démontré.
Les bornes connues indécises à Z=A sont encore réellement calculées
dans les deux bras de ce modèle ; leur suppression reste une économie
distincte. Le dossier ne transforme aucun de ces cas en résultat LiDAR.

## Raccord et optimisations bornées

La [contrelecture statique](STATIC_REVIEW.json) et ses
[quatre fichiers capturés](static_review_sources.zip) sont favorables :
relais sans racine supplémentaire, B₀ intact, masse contrôlée et préparation
active des bornes de 96 octets. Ce bloc n'a pas à être sérialisé dans
chaque job : il peut être reconstruit au traitement à partir des nœuds
immuables. Ce constat ne transfère aucune qualification
C++ au modèle ni du modèle au produit en chantier.

Les masses conjointes rejetées, admises et transmises partitionnent les
candidates. Les masses `uniform_*` génériques incluent aussi certaines
décisions conjointes ; seuls les compteurs géométriques génériques
décrivent le parcours après relais. Ne pas additionner ces masses deux
fois. Une identité de contrôle du code relu est
`joint.cursor_advances = joint.bound_tests - joint.splits_a - joint.splits_b + joint.structural_splits + joint.deferred_skips + joint.phase_switches`.

Si B devient singleton avant A, calculer les bornes par symétrie avec
la préparation à ancre fixe b est exact et évite les couples de
constantes identiques. Cela ne change ni les rôles A/B ni B₀ ni l'ordre.
Le diagnostic borné donne 25 entrées conjointes B singleton sur 104,
et 25 tests concernés sur 146, dans neuf configurations du r2. Ce sont
des fréquences de fixtures, jamais une estimation du régime LiDAR.

Le [recalcul des racines sur LiDAR réel](../q2_order_lidar_20260914/README.md)
donne un potentiel de réduction des seuls démarrages de 22,5–30,6 %,
selon scan/taille. Il ne borne pas le gain du census. Conserver index
et contexte immuables, état complet possédé par les jobs et tampons par
worker ; la collecte suspendable a son [contrat distinct](../P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie).
Le nombre de sous-produits, le travail cumulé et les sorties restent à
borner ; aucune tour FULL, qualification GPU ou borne globale acquise.

Rejouer la validation du modèle r2 sans écrasement :

```bash
python3 -B morsehgp3D_v8/audits/q2_product_20260914/verify.py --name replay --run r2
```

Une nouvelle capture du modèle se crée avec `record.py --name nouveau` ;
le sélecteur `--run nouveau` du lecteur choisit explicitement ce reçu.
