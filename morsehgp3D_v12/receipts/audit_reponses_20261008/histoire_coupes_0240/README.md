# CST-0240 : certificat de toutes les coupes par contrôles locaux

Proposition mathématique, **sans patch ni exécution du moteur**. Elle complète les
[gardes minimales](../../audit_foret_validation_20261008/README.md) du validateur TMVR repo5 : le lemme concerne
l'égalité `component_at(leaf,r) = coupe(parent,leaf,r)`, pas la provenance géométrique de la forêt. Les quatre
sources relues sont épinglées dans [capture.json](capture.json), base `c903774b1`, patch `6f0643ac…`.

**Le critère proposé est suffisant**, sous les préconditions ci-dessous. Il évite de comparer toutes les requêtes
de toutes les naissances à tous les rangs. Sa validation coûte **O(E + B log(B+1))**, avec B naissances, E=B−1
événements et profondeur d'attache au plus floor(log₂ B).

## Critère précis

La forme parent/enfants est déjà validée : arbre enraciné, naissances feuilles, chaque parent de rang strictement
supérieur à son enfant. On protège toutes les tailles, sentinelles et références avant de consulter l'historique.
Les E événements figurent **exactement une fois** dans la CSR des survivants ; chaque événement désigne une fusion
et porte son rang. Les domaines de `event_cell` et des champs non utilisés par la requête restent à contrôler
séparément, comme dans les gardes minimales.

La forêt d'attaches est acyclique et de profondeur bornée. Pour toute attache `x→a` au rang τ, exiger
`rank(x)≤τ` et `rank(a)≤τ`. Ces deux conditions protègent le domaine de la requête de raccord, y compris lorsque
les naissances ont des dates différentes.

Première passe sur les lignes CSR : partir du nœud naissance x. Chaque événement doit désigner soit le **même
nœud courant**, soit son **parent immédiat** ; dans ce second cas avancer. Une égalité de rang seule ne suffit
pas à reconnaître une répétition : deux multifusions disjointes peuvent avoir le même rang. Si x s'attache au
rang τ, tous ses événements doivent avoir un rang ≤τ. Cette passe valide toutes les lignes avant toute requête.
Elle implique leur monotonie et l'absence de trou dans la chaîne d'ancêtres parcourue.

Seconde passe, avec z le dernier nœud de la ligne, ou x si elle est vide :

```text
si x n'a pas d'attache : exiger z == racine
sinon, a = attach_parent[x], τ = attach_rank[x] :
    si rank(z) == τ : cible = z
    sinon : exiger parent(z) présent et rank(parent(z)) == τ ; cible = parent(z)
    exiger component_at(a, τ) == cible
```

z se lit en O(1) à la dernière case CSR : aucun tableau de terminaux supplémentaire n'est nécessaire. Toute
erreur de la requête de raccord fait refuser la validation ; les gardes ont déjà assuré son domaine et sa sûreté.

## Preuve par induction

Noter C(x,r) la coupe de l'arbre parent. Une ligne sans attache parcourt, sans saut, tous les ancêtres de x jusqu'à
la racine. Sa dernière entrée de rang ≤r est donc exactement C(x,r), avec x comme valeur avant son premier
événement. Les répétitions d'un même nœud au même plateau ne modifient pas ce résultat.

Supposer maintenant toutes les requêtes du survivant a exactes ; c'est une induction sur la hauteur restante dans
la forêt d'attaches, dont l'acyclicité est déjà établie. Pour x attaché à a au rang τ : avant τ, la liste de x couvre
tous ses ancêtres possibles, car son dernier nœud est soit de rang τ, soit l'enfant immédiat d'un nœud de rang τ.
La requête de x est donc exacte à chaque r<τ. Au rang τ, le contrôle de raccord impose
`C(a,τ)=C(x,τ)`. Deux feuilles dans le même nœud restent ensemble à toute coupe ultérieure de cet arbre. Pour
r≥τ, l'algorithme de requête suit d'abord l'attache x→a, puis rend la requête de a au même rang ; elle vaut donc
C(x,r). Cela couvre tous les rangs du domaine, sans les énumérer. Aucune monotonie supplémentaire des rangs le
long des attaches n'est nécessaire à cette preuve de requêtes ; la chronologie du noyau est une autre propriété.

Le scan des événements coûte O(E). Chaque naissance attachée provoque une seule requête : O(log B) sauts grâce à
la profondeur bornée, puis O(log(E+1)) pour la dichotomie. Le contrôle d'unicité peut employer E bits ou octets,
réutilisés ensuite pour le tampon de profondeur de B octets puisque E=B−1 ; admission et durée de vie sont à
expliciter lors de l'intégration. Aucun test exhaustif de requêtes n'entre dans le validateur proposé.

## Vérification indépendante bornée

[check.py](check.py) énumère **27 238 historiques sur douze arbres fixés de une à trois naissances** : étoiles,
arbres binaires, permutations des deux premières branches fusionnées, dates de naissance égales ou différentes.
Pour ces arbres et les domaines de rang bornés du script, toutes les affectations admissibles d'événements aux
lignes et toutes les forêts d'attaches de profondeur autorisée sont explorées. Le certificat accepte **90**
historiques ; leurs **1 250 requêtes** coïncident avec une remontée parent indépendante. Aucun contre-exemple.
Ce n'est pas une énumération de tous les arbres ou de tous les rangs possibles : la preuve générale est l'induction.

Trois cas supplémentaires à quatre naissances couvrent trois événements contractés dans une seule multifusion,
puis deux multifusions disjointes de même rang et une attache de profondeur deux, et enfin la limite ci-dessous.
Dix altérations sont refusées :
historique vide, nœud de naissance, survivant erroné, événement dupliqué, mauvais nœud au même rang, date d'événement
fausse, cycle, attache tardive et deux attaches avant naissance. Ces deux derniers cas utilisent des naissances de
rangs 0 et 1, fusion de rang 2 : une attache au rang 0 est refusée avant d'appeler une requête sur la naissance
de rang 1. Les cas sains de ces dates sont également couverts par l'énumération.

La limite de portée est elle-même illustrée : arbre `{1,2}` fusionné au rang1, puis `{0,{1,2},3}` au rang2 ;
attaches `0→1` au rang2, `1→2` au rang1, `3→2` au rang2. La ligne de 2 contient les événements des deux fusions,
avec le dernier répété. Ce certificat est accepté et toutes les requêtes sont correctes, malgré l'attache de 0
vers un survivant 1 déjà perdu à une date antérieure. Il ne prétend donc pas reconstituer une exécution légale
du noyau union-find. Les rangs d'attache monotones et les tailles seraient des contrôles distincts si cette
provenance faisait partie de la portée souhaitée.

```sh
python check.py --prototype "$TMVR_REPO5/morsehgp3D_v12"
python -O check.py --prototype "$TMVR_REPO5/morsehgp3D_v12"
```

Sources vérifiées avant/après, résultats identiques normal/`-O`, quelques centièmes de seconde observées localement
pour ce seul modèle. Aucune conclusion sur les chronos du moteur. Le certificat n'établit pas que les événements
proviennent des bonnes cellules, ni la règle d'union par taille, ni les compteurs physiques. Il certifie toutes les
réponses de l'historique relativement à un arbre parent validé. CST-0240 reste ouvert jusqu'à l'intégration et à sa
porte native.
