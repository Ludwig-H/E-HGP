# Verrous mathématiques GPU de MorseHGP3D v3

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_and_gpu_candidate_under_audit`,
`profile=quantized_u16_input_only`,
`mode=exact_gpu_wavefront_without_higher_order_mosaic`,
`public_status=not_claimed`.

Les constructions mathématiques de cette note sont indépendantes du live. Les
constats d'implémentation sont épinglés au snapshot suivant :

| objet | empreinte |
| --- | --- |
| `HEAD` | `fbfb2c0425a5b5a3c062b5eac92019075126c21d` |
| `prototype/order_k_flats.hpp` | `02ad6f58632de60d47e0b2bbcdf6205d8a3b9d1cab1474dd9d8b566593e9e81a` |
| `prototype/flats_differential.cpp` | `14c690031debf7214ae0fcd40ced0fd1a4169a06b34b0f035ca7103692384fa3` |
| `prototype/order_k_device_core.hpp` | `79382cf2857fb8da4efcecda8b9a164643fb4013c9a56cd6152f102daa155a3d` |
| `CMakeLists.txt` | `fdc00942cc8aed26f46c40ad3a95ef7be040d968ff819fd1ffb9368f171946c4` |

> [!IMPORTANT]
> Cette note aide Claude à construire la voie GPU; elle n'implémente aucun
> kernel. Les anciennes implémentations CUDA ont été consultées, avec
> l'autorisation de l'utilisateur, uniquement pour récupérer des contrats
> d'ingénierie. Ni leur géométrie, ni leurs mesures, ni leur statut ne prouvent
> quoi que ce soit sur la v3.

## 1. Verdict et ordre des verrous

Le live possède un **cœur hôte à forme device**, pas encore une voie GPU. Il n'y
a aucun fichier `.cu`, aucune cible CUDA, aucun lancement de kernel et aucun
passage `nvcc` dans la v3. Surtout, `neighbour_along` reste exécuté sur CPU : le
différentiel construit chaque voisin dynamique avec des vecteurs, puis transmet
seulement le voisin déjà connu à `device::decide_child`.

L'ordre utile est donc :

1. corriger le P0 CPU `i128 -> int` du signe owner;
2. fermer un vrai kernel de **verdict parent** en arithmétique 64 bits;
3. produire `next(v,d)` sur device avec certificat terminal ou replay;
4. partitionner la reverse-search en tâches transactionnelles disjointes;
5. intégrer owner puis le census exact cappé;
6. produire des runs triés par une clef de rayon exacte;
7. seulement ensuite déplacer le fold d'un lot complet et mesurer 50 k sur G4.

Le GPU ne doit jamais devenir une mosaïque de Delaunay d'ordre supérieur sous
un autre nom. Il reçoit des sommets, flats, candidats de support et tâches de
reverse-search; il ne matérialise ni cellules globales, ni étoiles, ni toutes les
cofaces.

## 2. Trois étages arithmétiques exacts

Posons $M=65535$. Toutes les bornes suivantes supposent un nuage authentifié
u16. Le type de l'API doit porter cette authentification; un pointeur `P3*` et
des tailles publiques arbitraires ne suffisent pas à invoquer les preuves.

### 2.1 Verdict parent : 64 bits suffisent

Un déterminant `orient3d` est une somme de six produits de trois différences :

$$\lvert\det\rvert\leq6M^3<2^{51}.$$

Au niveau zéro, `pair_admissible` additionne exactement les quatre orientations
de `root_base`, donc :

$$\lvert\mathrm{total}\rvert<24M^3<2^{53}.$$

Un `int64_t` signé est exact avec plus de dix bits de marge. Il faut toutefois
imposer `root_size==4`, coordonnées u16 authentifiées, indices valides et
`direction` dans `{-1,+1}` **avant** toute négation. Le live conserve encore
`i128` sur ce hot path et `backward_pair_admissible` évalue `-forward` avant la
validation; `INT_MIN` y reste un comportement indéfini.

Conclusion pratique : séparer un kernel parent 64 bits du kernel plus large de
génération du voisin. Employer `i128` partout est exact, mais paie inutilement
son coût sur la décision la plus fréquente.

### 2.2 Voisin, owner et census : 128 bits

Les comparaisons exactes du pinceau sont plus larges. Sous u16, une borne sûre
du prédicat in-sphere est inférieure à $2^{87}$ et celle du prédicat coplanaire
à $2^{103}$; 88 et 104 bits signés suffisent respectivement. `i128` est donc la
bonne largeur fixe pour `next(v,d)`.

Pour une sphère `base,n,d` avec $d>0$ et centre
$c=\mathrm{base}+n/d$, ne pas développer de carré rationnel. Avec
$w=p-\mathrm{base}$, le signe point--sphère est celui de :

$$H_B(p)=d\left\Vert w\right\Vert^2-2n\mathbin{\cdot}w.$$

Pour les supports u16 d'arité au plus quatre, une borne conservatrice donne
$\lvert H_B(p)\rvert<216M^6<2^{104}$. Le census exact tient donc lui aussi en
`i128`.

Une clef primitive de sphère évite les produits croisés larges. Former :

$$K(B)=\left(d,-2(d\,\mathrm{base}+n),d\left\Vert\mathrm{base}\right\Vert^2+2n\mathbin{\cdot}\mathrm{base}\right).$$

Diviser ses cinq coefficients par leur gcd positif et imposer le premier
coefficient positif. Deux supports décrivent la même sphère si et seulement si
leurs cinq coefficients primitifs coïncident. Chaque coefficient tient en
`i128`; le census est évalué par la même forme affine sur
$\varphi(p)=(p,\left\Vert p\right\Vert^2)$.

L'objectif owner tient également en `i128` sous la cible produit 50 k. Les
sommes globales de coordonnées sont calculées une fois, scellées par digest et
lues sans mutation par les kernels.

### 2.3 Ordre exact des rayons : 384 bits ou merge CPU

La comparaison de niveaux ne tient pas en `i128`. Pour les triangles u16,
$d\leq24M^4$ et $\lvert n_j\rvert\leq24M^5$; comparer
$\beta=N/d^2$ entre deux sphères demande des produits croisés sous $2^{308}$.
Six mots de 64 bits, soit 384 bits signés, suffisent.

Fixture permanente :

```text
A=(0,0,0) B=(65535,1,0) C=(65534,1,0)
D1=(0,0,1) D2=(0,0,2)
beta1=18445055288272617483/4
beta2=9222527644136308743/2
beta2-beta1=3/4
```

Les deux niveaux ont pourtant la même projection binary64
`0x1.fff4001dffdc0p+61`. Aucun tri, lot ou fold ne peut donc employer le `double`
public comme clef d'autorité.

Deux voies exactes sont acceptables : radix-sort GPU sur six limbs, ou runs GPU
avec merge CPU multiprécision. La seconde est la première étape la plus simple.

### 2.4 Contrat compilateur

CUDA moderne supporte `__int128` sur Linux lorsque le compilateur hôte le
supporte. Si la branche 12.9 est retenue, exiger au moins **12.9 Update 1** :
NVIDIA y a corrigé une génération de code erronée touchant les mises à jour
`__int128`. Voir la
[documentation des types CUDA](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cpp-language-extensions.html)
et les
[release notes 12.9.1](https://docs.nvidia.com/cuda/archive/12.9.1/cuda-toolkit-release-notes/index.html).

Le reçu doit épingler toolkit, driver, architecture, PTX/cubin et digest du
binaire. Les frontières arithmétiques sont rejouées sur CPU et device; compiler
n'est pas un certificat de calcul exact.

## 3. Paralléliser la décision locale sans changer son théorème

### 3.1 Fermeture sous forme de masque

Après admission, $m=\lvert S(v)\rvert\leq32$. Toute fermeture est donc un masque
`uint32_t` sur la coquille triée. L'ordre lexicographique des fermetures se lit
en itérant les bits actifs, puisque leurs positions et les identifiants de
points ont le même ordre.

Pour une fermeture de rang trois, la base canonique est exactement : les deux
premiers membres, puis le premier membre non collinéaire avec eux. Si aucun
membre ne convient, toute la fermeture est collinéaire. Cela remplace la boucle
cubique de recherche de base par un scan $O(m)$.

Ces deux identités ont été vérifiées indépendamment sur 20 000 fermetures
aléatoires : masque/vecteur et base rapide/triple boucle concordent sans écart.
Elles réduisent `BoundedFlat` d'environ 144 octets à une poignée de mots et
suppriment le tableau local `previous[32]`.

### 3.2 Réduction parallèle exacte de `decide_child`

Soit $K_R$ le couple de retour dans l'ordre total canonique
`(fermeture,base,slot)`. Le verdict séquentiel est équivalent à :

- `Reject` s'il existe un couple admissible $K<K_R$;
- `Accept` si $K_R$ est admissible et aucun $K<K_R$ ne l'est;
- `Broken` sinon, ou si l'intégrité de l'ordre/retour est violée.

Les couples $K>K_R$ n'influencent jamais le verdict. Un bloc peut donc tester
les couples en parallèle puis réduire deux booléens idempotents
`earlier_admissible` et `return_admissible`, séparément des fautes d'intégrité.
Une comparaison exhaustive sur 131 068 états booléens bornés a reproduit le
scan séquentiel sans écart.

Ce résultat est plus adapté au GPU que copier littéralement le DFS local dans
chaque thread. Les atomiques ne doivent jamais choisir le premier verdict; la
réduction booléenne puis la clef canonique rendent le résultat indépendant du
scheduling.

## 4. Reverse-search : théorème de partition en sous-arbres

La fonction de parent exacte $\pi$ définit un arbre enraciné. Soit $A$ une
antichaîne de sommets : aucun élément de $A$ n'est ancêtre d'un autre. Alors les
sous-arbres descendants $D(a)$ sont deux à deux disjoints. En effet, si un sommet
appartenait à $D(a)\cap D(b)$, son unique chaîne d'ancêtres contiendrait $a$ et
$b$; l'un serait donc ancêtre de l'autre.

Après un préfixe $P$ qui produit une frontière antichaîne $A$, on obtient la
partition exacte :

$$V=P\mathbin{\dot\cup}\coprod_{a\in A}D(a).$$

C'est la source du parallélisme GPU : produire une frontière canonique bornée,
puis confier chaque sous-arbre à une tâche indépendante. Aucune table `seen` ni
déduplication atomique globale n'est nécessaire.

Une tâche peut néanmoins dépasser profondeur, coquille, sortie ou budget. Le
refus doit alors porter sur **tout son sous-arbre**. Compter puis ignorer le
sommet refusé supprime ses descendants et viole la partition.

### 4.1 Le parcours cible n'a pas besoin d'une pile de profondeur

La reverse-search classique se parcourt avec un état constant `(v,cursor)`.
Tester le prochain voisin et descendre si son parent est `v`. Lorsque
l'adjacence de `v` est épuisée, recalculer `p=pi(v)`, retrouver dans l'ordre
canonique de `Adj(p)` l'arête inverse vers `v`, puis reprendre au slot suivant.

Ici l'arête porte la même fermeture aux deux extrémités; sa base canonique par
identifiants est donc identique et la direction est opposée. Trois recherches
de position dans la coquille triée retrouvent son curseur. Sous la précondition
de symétrie exacte du lot, chaque liste d'adjacence est consommée une fois :
travail $\sum_v\deg(v)$ plus un calcul `pi/next` par backtrack, et mémoire d'un
nombre constant de sommets/cursors.

Un work item GPU devient
`(subtree_root,v,cursor,snapshot_epoch,output_segment)`. Une profondeur élevée
n'est plus un overflow normal; une queue ou un segment de sortie borné peut en
revanche provoquer rollback et repartition. La porte doit comparer ce parcours
sans pile au parcours de référence, notamment sur des backtracks répétés et une
mutation de la clef inverse.

La mémoire constante est **par work item**, pas pour toute la vague. Une
frontière parallèle peut avoir une largeur $\Theta(\lvert V\rvert)$ et deux
buffers BFS peuvent donc être proportionnels à la sortie. Publier capacité,
pages, travail différé et high-water en octets de `current/next`; lorsque la
frontière dépasse, fermer une page stable ou rollback/repartitionner. Le mot
« stateless » supprime `seen`, pas cette masse de parallélisme.

## 5. Transaction et replay : unité minimale correcte

Chaque tâche GPU possède un segment non committé. Elle ne publie rien avant sa
fin. Le ledger minimal est :

$$N_{\mathrm{begin}}=N_{\mathrm{commit}}+N_{\mathrm{rollback}}.$$

Un rollback invalide tout le segment de sortie de la tâche. Le CPU rejoue la
tâche depuis une source authentifiée, ou la repartitionne en une nouvelle
antichaîne. Cela évite les doublons et les préfixes partiellement publiés.

Reçu minimal :

```text
TaskReceipt {
  cloud_epoch, cloud_digest, task_id, seed_id,
  parent_key, edge_cursor, reason,
  staged_records, staged_children, arithmetic_tier,
  begin_id, status, output_digest
}
```

`task_id` référence une table hôte immuable de seeds; il n'est pas nécessaire
de transférer une coquille de taille $\Theta(n)$. Pour un refus rencontré depuis
un parent borné, `(task_id,parent_key,edge_cursor)` permet au CPU de recomputer
le voisin, son census et tout son sous-arbre.

Statuts à distinguer :

| statut | sens |
| --- | --- |
| `complete` | tâche entière décidée et segment committable |
| `shell_capacity` | géométrie valide mais coquille au-delà du fast path; replay |
| `arithmetic_capacity` | largeur fixe insuffisante; drain exact CPU |
| `cursor_invalid` | clef inverse, symétrie ou reprise incohérente; fail-closed |
| `output_capacity` | segment insuffisant; rollback/page stable |
| `invalid_contract` | u16, ordre, indices, tailles ou identité non authentifiés; fail-closed |

Sous `s_max<=32`, un sommet validé satisfait $\lvert B(v)\rvert\leq30$.
`interior>30`, `shell<3` et une fermeture dépassant une coquille déjà admise ne
sont donc pas des fallbacks produit : ce sont des violations de contrat. Seul
`shell>32` est ici un refus géométrique normal.

Le live mesure désormais les admissions au bon endroit et porte
`kMaxInterior=32`, mais le différentiel fait encore `continue` sur un refus. Il
ne compare donc pas l'union « device committé + replay CPU » au parcours de
référence.

## 6. Produire réellement le voisin sur GPU

Le verrou principal n'est pas `decide_child`, mais `next(v,d)`. Un candidat de
voisin n'est autoritaire que si deux faits sont certifiés : aucun point n'a un
paramètre strictement plus petit, et le lot contient **tous** les ex æquo au
minimum. Un lot peut avoir $\Theta(n)$ membres.

Première voie exacte, sans promesse de débit : un bloc par
`(sommet,fermeture,direction)`, scan tuilé de tout $X$, réduction exacte du
minimum rationnel, puis second passage pour le lot complet. Cette baseline est
$O(n)$ par couple mais constitue un vrai kernel différentiel et ne matérialise
aucune mosaïque.

Le live contient déjà un travail supprimable avant portage : lorsque la boîte
indexée atteint une demi-largeur 65 535 autour d'une ancre u16, elle couvre
toute la grille déclarée et `touched` contient déjà tous les points. Si aucun
événement n'a été trouvé, le second `for z in X: absorb(z)` est un no-op exact.
Les cinq portes courantes comptent 424 150 de ces `exhaustive_scans`. Certifier
« boîte = grille entière » permet de conclure le rayon non borné sans ce second
balayage; une mutation qui omet des points de la première couverture doit faire
rougir la porte.

Voie indexée : le reçu transporte une antichaîne de couverture de l'index, les
feuilles examinées, le minimum exact et le lot. Chaque prune est recertifiée par
une borne entière; tout statut inconnu descend ou rejoue. Une proposition LBVH
incomplète peut accélérer la recherche, jamais conclure « minimum terminal ».

Les kernels de blocs ordinaires n'ont pas de synchronisation globale implicite;
une frontière, un lot complet et un commit global se ferment naturellement à
une frontière de kernel. Les Cooperative Groups existent, mais ajoutent des
contraintes de lancement et ne remplacent pas le reçu transactionnel. Voir le
[guide NVIDIA sur la synchronisation des groupes](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/cooperative-groups.html).

## 7. Owner et census : parallélisme exact

### 7.1 Owner

Paralléliser **entre supports**. Pour $m\leq32$, un warp calcule les contraintes
signées; une lane applique dans l'ordre canonique l'automate
`FULL/HALF/LINE/RAY/WEDGE/ZERO`. L'arité trois réduit deux booléens `allow_plus`
et `allow_minus`; l'arité quatre n'a aucun rayon.

Ne pas employer `atomicCAS(first_claimant)`. Le théorème owner impose que chaque
sommet décide indépendamment le même prédicat exact. En qualification, conserver
tous les claims puis exiger exactement un propriétaire et la même identité que
l'oracle CPU. Un first-wins masquerait le mutant de signe déjà observé et
rendrait la sortie dépendante du scheduling.

L'intersection géométrique des cônes est associative, mais aucun opérateur
canonique de fusion des six représentations n'est encore prouvé. Ne pas annoncer
un tree-reduce de l'automate avant cette preuve; le scan d'une lane est borné à
32 contraintes.

### 7.2 Census asymétrique

Pour prouver que le rang fermé dépasse `s_max`, il suffit de produire
`s_max+1` identifiants distincts avec $H_B(p)\leq0$. C'est un certificat positif
borné; un kernel même non terminal peut donc être autorité de **rejet**.

En revanche, conclure que le rang est au plus `s_max` et rendre les ensembles
complets intérieur/coquille exige un report terminal certifié ou un replay CPU.
API recommandée :

```text
RankOverflow { witnesses[33], digest }
NeedsTerminalCensus { sphere_key, task_id }
CompleteCensus { interior, shell, coverage_receipt }
```

« Peu de points trouvés » ne signifie jamais « census complet ».

## 8. Runs, lots exacts et fold

Les threads peuvent réserver des emplacements par atomique, mais cet ordre de
réservation n'est pas un ordre public. Chaque record transporte une clef totale
exacte; le run est ensuite trié et fusionné. Les occurrences parallèles gardent
leur identité source et ne sont pas écrasées dans un `set`.

Le fold GPU ne peut commencer qu'après fermeture d'un lot de niveau exact :
figer le snapshot strict, valider chaque record brut, projeter ses endpoints,
calculer les composantes, puis classifier les composantes disjointes en
parallèle. Les représentants DSU internes ne sont pas sérialisables; choisir le
minimum de la clef typée canonique et allouer les nouveaux IDs par scan stable.
Le lot entier commit ou rollback.

Fixtures obligatoires : une chaîne `R1--N--R2` coupée entre deux runs; plusieurs
composantes indépendantes; faute injectée dans la dernière après staging de la
première; permutations des blocs. Toutes rendent le même transcript ou zéro
mutation sur faute.

## 9. Patterns récupérables des implémentations CUDA précédentes

Les éléments suivants sont réutilisables comme ingénierie v3 :

- vue d'index immuable authentifiée par epoch et digest;
- moteur exact `host/device` unique et faux launcher hôte distinct;
- statuts où `unknown`, `overflow` et `zero` sont trois résultats différents;
- échelle de largeurs fixes avec drain rationnel CPU;
- double buffer de frontière, compteurs de masse et rollback de vague complète;
- staging des sorties, commit atomique, reçus de transaction et digest final;
- contrôle hôte de tous les compteurs device avant toute allocation ou copie.

Le pattern de vague transactionnelle est visible, par exemple, dans
[`phase15_exact_pair_block_transactional_frontier_resident_cuda.cu`](../../morsehgp3d/src/cuda/phase15_exact_pair_block_transactional_frontier_resident_cuda.cu).
Il faut reprendre le **ledger**, pas sa géométrie de paires ni ses mesures.

## 10. Porte permanente GPU v3

### 10.1 Non-vacuité

Exiger séparément : kernels lancés, tâches commencées/committées/rollbackées,
sommets admis, décisions parent, voisins produits, refus coquille, drains
arithmétiques, runs et replays CPU. Une suppression complète du bloc GPU doit
faire rougir la porte.

### 10.2 Fixtures minimales

| fixture | attente |
| --- | --- |
| déterminants owner 1290/1291 et alternés 1023/1024/1025 | même signe CPU/device, UBSan vert, mutant de troncature tué |
| coquille 33 sur sphère entière | rollback device puis replay du sous-arbre, aucune perte ni duplication |
| centre `(100,100,100)`, coquille des six axes de rayon 50, trente intérieurs | admission, high-water intérieur 30, aucun faux overflow |
| fermeture masque, 20 000 cas générés | même base, même ordre et même multiplicité que le vecteur CPU |
| réduction `decide_child` | même verdict sur scan séquentiel, permutations et mutations d'ordre |
| deux niveaux séparés de `3/4` mais même `double` | deux lots exacts distincts |
| owner signed cone | un claim exact et identité attendue, mutant first-wins/non signé tué |
| census dense | `RankOverflow` avec témoins exacts, jamais faux `CompleteCensus` |
| capacité de sortie tardive | rollback total puis replay, aucun préfixe publié |

Une coquille entière de taille 33 se construit autour de
`(32768,32768,32768)`, rayon 25 : six points axiaux, les 24 permutations signées
de `(24,7,0)` et trois points distincts de l'orbite `(20,15,0)`.

### 10.3 Reçu 50 k / G4

Le reçu final comporte au minimum : commit, diff source, compilateur/toolkit,
driver, modèle/architecture GPU, digest du binaire, digest du nuage, paramètres
50 k/K, temps par étage, octets et high-waters par conteneur, compteurs exacts
d'admission/refus/replay/commit, mutations, concordance byte-à-byte sous
répétitions et arrêt GCP certifié.

Le débit seul ne qualifie rien. La première campagne G4 utile vient après une
cible `.cu` réelle et une porte locale non vacuable; avant cela, démarrer une VM
ne mesurerait aucun chemin GPU v3.

GCP non utilisé pour cette note.
