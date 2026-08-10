# Solution pour Claude — fold hybride `principal-support` / `face-owner`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_and_bounded_oracles`,
`profile=quantized_u16_input_only`, aucun statut public. Cette note donne le
contrat directement implémentable qui résulte de la question de masse, du
théorème `face-owner` et des contre-exemples multi-supports.

## Décision d'architecture

Le fold exact sans `P_post` a deux chemins, jugés par une même vérité bornée.

1. **Fast path `principal-support`.** Une boule dont le support `U` est certifié
   obligatoire dans toute représentation de la boule ne demande qu'au plus
   `q=|U|<=4` attaches par ordre.
2. **Fallback dégénéré.** Une boule à supports alternatifs utilise la recherche
   `face-owner` demand-driven; son oracle de réception énumère toutes les
   signatures de faces aux petites tailles.

Le premier chemin est linéaire dans le nombre de générateurs et couvre le
régime régulier. Le second conserve l'exactitude sur les coquilles u16 sans
prétendre une borne inexistante. Aucun des deux ne construit de paire
`(M,N)` ni de poids `|M intersection N|`.

## 1. Certificat local de support principal

Soit une boule `B`, son saturé `M`, sa coquille `Q` et un support minimal
`U` de cardinal `q`. Le certificat demandé est : toute partie `A` de `M` dont
la miniboule vaut `B` contient `U`. La réciproque est automatique : si `U` est
inclus dans `A` et `A` dans `M`, la miniboule de `A` vaut `B`.

Un certificat suffisant et vérifiable est le suivant :

- `U` porte exactement `B` et son centrage strict est reçu;
- pour chaque `u` de `U`, un séparateur rationnel exact prouve que le centre de
  `B` n'appartient pas à l'enveloppe convexe de `Q` privé de `u`.

Si un support alternatif omettait `u`, son enveloppe contiendrait le centre et
contredirait le séparateur. Les quatre séparateurs au maximum se vérifient en
`O(q*|Q|)`. Le cas fréquent `Q=U` satisfait immédiatement le certificat.

Ce bit doit voyager comme `principal_support_certified`; il ne se déduit ni de
`n_support`, ni de `q_min`, ni de `smax>=n`.

Une première version sûre n'a pas besoin d'implémenter immédiatement un solveur
de séparation. Après validation exacte de la sphère, de `q_min` et du centrage
strict, elle pose `principal=true` seulement lorsque `Q=U`; dans tous les autres
cas elle choisit le fallback. Ce choix peut perdre des accélérations, jamais de
la sémantique. Les séparateurs rationnels étendent ensuite progressivement le
fast path aux coquilles `Q` strictement plus grandes que `U`. Un certificat
fourni mais invalide refuse le lot; un certificat absent sélectionne simplement
le fallback.

La production complète ne demande finalement aucun solveur : `u` est
obligatoire si et seulement si `miniball(M privé de u)` est strictement plus
petite que `B`. Son support de quatre points au plus est un certificat positif
compact; le vérificateur reconstruit cette petite boule, vérifie qu'elle couvre
`M` privé de `u`, puis compare son niveau à `B` avec les primitives exactes
existantes. Le contrat et sa preuve sont dans
[`NOTE_CERTIFICAT_SUPPORT_PRINCIPAL_PAR_MINIBOULE_20260810.md`](NOTE_CERTIFICAT_SUPPORT_PRINCIPAL_PAR_MINIBOULE_20260810.md).

Une fixture empêche de confondre ce premier cas suffisant avec le certificat
complet : centre `(2,2,2)`, rayon `1`, support principal
`U={(1,2,2),(3,2,2)}` et point de coquille supplémentaire `(2,3,2)`. Après
retrait du premier puis du second point de `U`, les séparateurs respectifs
`(1,1,0)` et `(-1,1,0)` sont strictement positifs sur les directions restantes.
Le support est donc principal malgré `Q` strictement plus grand que `U`; la
version initiale choisit légitimement le fallback, puis la version à
séparateurs doit promouvoir ce cas au fast path.

## 2. Théorème des `q` attaches

Fixons un ordre `k` avec `q<=k+1` et `rank(M)>=k`.

- Si `rank(M)=k`, nécessairement `q<=k`; l'unique `k`-face `M` apparaît au
  niveau de `B`. C'est une naissance, sans lookup strict.
- Sinon `rank(M)>=k+1`. Choisir une partie fixe `T` de `M` privé de `U`, de
  taille `k-q+1`. Pour chaque `u` de `U`, former
  `S_u=(U privé de u) union T`. Pour `q=k+1`, `T` est vide.

Chaque `S_u` a taille `k` et est stricte puisqu'elle ne contient pas `U`. Pour
toute autre `k`-face stricte `F`, choisir un `u` de `U` absent de `F`. Les deux
faces `F` et `S_u` appartiennent au graphe de Johnson des `k`-sous-ensembles de
`M` privé de `u`, qui est connexe. Chaque face et chaque coface de ce chemin
omet `u`; le certificat principal impose donc un niveau strictement inférieur à
`B`. `F` et `S_u` sont dans la même racine stricte.

Ici « différent de `B` » implique bien « strictement inférieur » : toute partie
de `M` est contenue dans `B`, donc sa miniboule a un rayon au plus égal; si le
rayon était égal, `B` serait elle-même minimale pour cette partie et l'unicité
de la miniboule euclidienne imposerait la même boule `B`.

Les racines distinctes des `q` carriers `Sat(S_u)` sont ainsi exactement toutes
les composantes strictes touchées par `B`. Dédupliquer ces racines, attacher le
nouveau générateur, puis appliquer le marquage et les records déjà reçus.

Ce théorème ferme tous les cas `q<k`, `q=k` et `q=k+1` **sous le bit
principal**, avec au plus quatre miniboules et quatre lookups par
`(générateur,ordre)`.

### Réponse sur la liberté de `T`

Oui : `T` est libre pour la sémantique de connexité. Pour tout choix de la
bonne cardinalité et pour tout `u`, `S_u` est une `k`-partie de `M` privé de
`u`. Toute `k`-face stricte `F` omet au moins un élément `u` de `U`; le graphe
de Johnson des `k`-parties de `M` privé de ce `u` est connexe puisque
`rank(M)>=k+1`. Chaque sommet et chaque coface d'un chemin entre `F` et `S_u`
omet `u`, donc reste strict sous le certificat principal. Ce raisonnement ne
dépend d'aucun autre élément de `T`. Deux choix de `T` touchent donc exactement
le même ensemble de racines strictes, même s'ils peuvent choisir des carriers
différents à l'intérieur de ces racines.

La canonicité n'est requise que pour reproduire le **reçu opérationnel**. Le
produit peut prendre les `k-q+1` plus petits `PointId` de `M` privé de `U`, dans
l'ordre brut canonique déjà validé, et lier ce choix au digest de l'entrée. Le
transcript sémantique — niveau exact, racines finales, type et boule marquante —
doit rester indépendant de `T`; une porte qui force un second choix de `T` doit
comparer ces champs et autoriser seulement les identifiants de carriers à
différer.

## 3. Lookup exact d'une boule

La structure brute `Sphere{base,nx,ny,nz,den}` n'est pas une clé. Une même
boule portée par deux supports possède des bases et des fractions différentes.
Comparer seulement `beta` confond en outre des boules distinctes de même rayon.

Pour une sphère, poser le numérateur absolu du centre
`C=(base.x*den+nx, base.y*den+ny, base.z*den+nz)` et
`N=nx^2+ny^2+nz^2`. La boule est la forme quadratique entière de coefficients
`(den^2,-2*den*C.x,-2*den*C.y,-2*den*C.z,|C|^2-N)`. Diviser tous les
coefficients par leur PGCD, imposer le premier coefficient positif, sérialiser
les limbs, puis utiliser cette forme primitive comme clé. Après le hash, une
égalité exacte des cinq coefficients reste obligatoire.

Porte minimale : les cinq supports de la boule de centre `(2,2,2)` et rayon
carré `5` doivent tous produire la clé primitive `(1,-4,-4,-4,7)`. Une boule de
même rayon et centre différent doit rester distincte, et une collision de hash
forcée doit être résolue par la comparaison des cinq entiers. La sérialisation
des entiers signés et de leur longueur doit elle-même être canonique.

Le CPU de vérité peut commencer avec des entiers multiprécision. Le chemin
produit ne fixe une largeur qu'après preuve des bornes u16. À la construction de
l'index, deux entrées de même clé mais de saturés différents sont une faute de
source; un lookup absent sous prétention complète est un refus.

### Contexte minimal à fournir au fold

L'API live `build_saturated_fold_faceowner(Catalogue,...)` ne suffit pas au fast
path : le catalogue contient les `PointId`, les membres et une sphère, mais pas
les coordonnées permettant de reconstruire la coquille `Q` ni de calculer la
miniboule d'un carrier `S_u`. Il faut un sidecar v3, lié au digest de l'entrée,
qui fournisse au minimum :

- une vue immuable des points et son digest;
- pour chaque générateur, `q_min_certified`, ses membres vérifiés et son
  certificat principal, et sa `BallKey`;
- un index injectif `BallKey -> generator_handle`, construit après validation
  que deux clés égales portent le même saturé;
- le bit `source_complete_for_order[k]`, distinct de `smax>=n` et lié au digest
  du catalogue final.

Le lookup rapide calcule `miniball(S_u)` depuis la vue des points, canonicalise
sa `BallKey`, retrouve le générateur, puis vérifie que ses membres contiennent
`S_u` et que son niveau est strict. S'il est absent sous source certifiée
complète, le lot refuse; sous source partielle ou certificat absent, le
dispatcher utilise le fallback par postings. Ce sidecar évite toute table
persistante de faces : seules les boules réellement demandées sont calculées.

Le raccord le moins risqué au prototype est une factory post-catalogue
`make_validated_hybrid_sidecar(points, catalogue_final, source_receipt)`. Elle
travaille sur les handles déjà triés, couvre aussi le chemin singleton, vérifie
les digests et ne publie le type `ValidatedHybridSidecar` qu'après validation de
tous ses champs. Le fold ne doit plus recevoir séparément `points`,
`point_count` et un `Catalogue` brut : cette signature permet trop facilement
de désynchroniser les vues ou de contourner la provenance.

## 4. Fallback `face-owner` demand-driven

Sous coquille multi-support, le seul support canonique peut manquer des racines.
Le fallback ne devine pas une petite famille de facettes : il explore un trie
canonique des combinaisons de points de `M`.

Le dispatcher n'emploie le fast path que si
`source_complete_for_order[k] && principal_support_certified && q<=k+1`. Le
certificat principal et la validation stricte de `U` impliquent ici
`q_min=q=|U|`; le marquage public exige toutefois un bit `q_min_certified` pour
tous les générateurs, notamment ceux du fallback. En mode partiel, tous les
générateurs passent par le fallback relatif. Sous
`source_complete_for_order[k] && q_min_certified`, le cas `q>k+1` admet au
contraire une réduction exacte à une seule attache : les `k`-faces de `M` sont
connexes dans le graphe de Johnson; deux voisines ont une union de taille
`k+1`, dont la miniboule est strictement sous `B`, faute de quoi `B` aurait un
support de taille au plus `k+1`. La source complète contient donc tous leurs
carriers stricts et une seule `k`-face canonique atteint l'unique racine
stricte. Cette optimisation exige les deux certificats, puis un lookup qui
vérifie `face subset members(carrier)` et `level(carrier)<level(B)`. Sans eux,
elle repasse au fallback.

À la coupe stricte du niveau `a`, créer un proxy distinct par racine ancienne,
ajouter **tous** les générateurs admissibles du lot au DSU local, puis construire
des postings immuables `P_x=P_x^- union B_x`. L'intersection doit rester une
liste d'identifiants de générateurs; quotienter chaque posting par racine avant
l'intersection est faux, car deux générateurs différents d'une même racine
peuvent porter séparément deux points sans qu'aucun ne porte leur signature.

Pour chaque nouveau générateur `M` et chaque préfixe `P` :

1. intersecter les postings actifs de ses points, du plus rare au plus fréquent;
2. conserver `C(P)`, la liste exacte des identifiants de générateurs incidents;
3. seulement alors mapper chaque `N` de `C(P)` vers son proxy strict ou son
   sommet staging et couper si `find_local(label(N))=find_local(M)` pour tout
   `N`;
4. sinon descendre jusqu'à la taille `k`; à une feuille, retenir un seul carrier
   réel `N` par composante locale externe, publier cet identifiant comme témoin
   d'arête et effectuer l'union locale avec `N`, jamais avec le seul numéro de
   sa racine DSU.

La coupure est une preuve : les incidents de toute extension sont inclus dans
ceux du préfixe et ne peuvent révéler une racine absente de sa liste. Tous les
générateurs du lot sont visibles dans des postings **stagés** avant la recherche;
aucun n'est publié avant la fermeture. Le snapshot strict, les owners anciens,
les témoins et le commit restent ceux des trois folds reçus.

Pour classifier l'événement, conserver l'identité de chaque proxy strict gelé
avant les unions locales. L'arité est le nombre de proxies stricts distincts
qui aboutissent dans la composante finale, jamais le nombre de racines du DSU
après fermeture : ces dernières ont précisément été fusionnées.

Le mot « déjà » est local à `C`, jamais global au lot. Une racine stricte
touchée par une autre composante nouvelle doit encore être reliée à `C`; un
`seen_roots` partagé ferait donc une coupure fausse. Tout cache de pruning doit
être indexé par représentant staging et invalidé après une union, ou être
strictement local à la recherche courante. Cette contrainte est une fixture :
deux composantes nouvelles du même lot rencontrent la même racine ancienne par
deux signatures distinctes, avec intersection de taille strictement inférieure
à `k` entre elles. Elles ne se rejoignent que par l'ancienne racine; un
`seen_roots` global en laisse une détachée. Une seconde fixture, séparée, porte
la chaîne nouveau--nouveau et reçoit la visibilité des postings stagés.

Trois fixtures minimales ferment les erreurs de structure :

- `k=2`, ancien `O={0,1,2,3}`, lot `A={0,1}`, `B={2,3}` : `A` et `B` ne se
  touchent pas directement mais doivent tous deux rejoindre `O`; un `seen`
  global omet la seconde attache;
- `k=2`, anciens `O1={0,2,3}` et `O2={1,2,3}` déjà dans la même racine, nouveau
  `M={0,1}` : aucun ancien ne contient `{0,1}`; intersecter des postings déjà
  compressés par racine invente pourtant un voisin;
- `k=2`, lot seul `A={0,1}`, `B={0,1,2,3}`, `C={2,3}` : les postings stagés
  donnent une seule composante; des postings stricts seuls en donnent trois.

Enfin, on n'unit jamais à un préfixe de taille inférieure à `k` : ancien
`O={0,2}` et nouveau `M={0,1}` à `k=2` partagent un point mais aucune arête.

Le pire cas vaut la masse `face-owner`; aucune borne `O(q+arité)` n'est annoncée.
Avec de simples lookups ponctuels, une telle borne algorithmique est impossible
à certifier : le dernier candidat non interrogé peut être l'unique racine
nouvelle. L'intersection agrégée des postings est précisément le certificat
d'absence qui manque à la conjecture.

L'implémentation ne doit pas recopier puis trier un posting entier à chaque
nœud. Affecter à chaque générateur un `activation_id` canonique, ordonné par
`(niveau exact, membres, BallKey)`, rend les postings triés dès leur append.
Chaque frame ne matérialise alors que l'intersection descendante. À une feuille,
une table `root -> plus petit incident réel` fournit simultanément le carrier
certifié et la déduplication. Ce choix rend aussi les compteurs du trie
reproductibles sous permutation du catalogue; sinon ces compteurs doivent être
explicitement qualifiés de relatifs à l'ordre d'entrée.

## 5. Unité transactionnelle du lot

Pour un niveau exact `a` :

1. valider tous les générateurs, certificats et lookups du lot;
2. activer tous les nœuds dans le staging;
3. geler les racines strictes et leurs témoins;
4. exécuter les attaches rapides et les recherches fallback;
5. fermer le DSU local nouveau--nouveau et dédupliquer les racines touchées;
6. classer naissance, continuation ou multifusion et produire les marqueurs;
7. valider les identités et le budget, puis committer tout le lot ou rien.

Une erreur de clé, carrier, source ou budget annule le lot entier. Le chemin
rapide et le fallback peuvent cohabiter dans le même lot puisqu'ils émettent la
même unité : une attache certifiée entre le nouveau générateur et une racine.

## 6. Reçus nécessaires

Par ordre et par lot, publier séparément :

- générateurs `principal` et `fallback`;
- certificats de séparateur vérifiés et échecs;
- miniboules/lookup rapides tentés, trouvés et racines distinctes;
- nœuds de trie visités, coupures vides, coupures racines connues, feuilles et
  identifiants de postings inspectés;
- masse théorique `I_k`, masse réellement visitée et branches owner de l'oracle;
- attaches stagées, unions tentées/réussies, racines strictes touchées;
- octets par phase, high-water mesuré, budget et motif de refus;
- provenance `source_complete_for_order`, `q_min_certified` et
  `principal_support_certified`.

Le préflight mémoire reste une estimation tant qu'un allocateur plafonné ou une
borne incluant les capacités réelles ne l'impose pas.

## 7. Portes qui ferment la solution

1. Oracle `face-owner` contre `G2`, postings par lots et global : partitions,
   records, marqueurs et forêts sémantiques.
2. Fast path principal pour `q=1,2,3,4`, cas naissance `rank=k`, continuation
   et coface `q=k+1`; mutant qui retire un des `q` carriers.
3. Fixture multi-support `q=4,k=4` de
   [`AUDIT_COFACES_F2E78FA.md`](AUDIT_COFACES_F2E78FA.md) : six composantes
   strictes, cinq vues par le support canonique; le dispatcher doit choisir le
   fallback et rendre les six.
4. Sur cette fixture, le vérificateur exact rend
   `principal_support_certified=false` et le dispatcher choisit le fallback qui
   retrouve six racines. Un mutant qui contourne le certificat et force le fast
   path doit soit refuser fermé lorsqu'un `S_u` n'est pas strict/lookupable dans
   le snapshot, soit diverger de G2/`face-owner` sur partitions, records ou
   marqueurs; aucun nombre particulier de racines perdues n'est présupposé.
5. Mutant clé brute `(base,num,den)` : lookup manquant ou doublon sur deux
   supports de la même boule; la forme primitive passe.
6. Ex æquo avec un mélange fast/fallback, chaîne nouveau--nouveau, permutation
   du catalogue et rollback après la dernière attache.
7. Mode partiel : aucun filtre `Sigma_k`; le certificat principal reste une
   propriété locale autoritative de `(B,M,U)`, mais l'absence de
   `source_complete_for_order[k]` interdit le fast path et impose le fallback
   relatif ou un refus explicite du chemin complet.

## Conclusion opérationnelle

Claude peut implémenter immédiatement l'oracle `face-owner` borné et le fast
path principal. Il ne doit pas implémenter le parcours cofaces canonique général
réfuté. Le fallback demand-driven vient ensuite, jugé par l'oracle. Une mesure
50 k ou un kernel GPU n'a de sens qu'après publication du ratio
`principal/fallback`, des nœuds de trie visités et du high-water.

GCP non utilisé.
