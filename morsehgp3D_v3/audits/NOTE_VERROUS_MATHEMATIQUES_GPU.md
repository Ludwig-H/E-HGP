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
| `HEAD` | `78583f1950c4c514828c523ba3ad2aa03676bfb0` |
| `prototype/order_k_flats.hpp` | `02ad6f58632de60d47e0b2bbcdf6205d8a3b9d1cab1474dd9d8b566593e9e81a` |
| `prototype/flats_differential.cpp` | `14c690031debf7214ae0fcd40ced0fd1a4169a06b34b0f035ca7103692384fa3` |
| `prototype/order_k_device_core.hpp` | `79382cf2857fb8da4efcecda8b9a164643fb4013c9a56cd6152f102daa155a3d` |
| `prototype/device_wavefront_job.hpp` | `cffe45646eb46ec44f4818ce8c8f0a3e7251084d8fb05c0cb79fbfae243fa31f` |
| `prototype/device_wavefront_kernel.cu` | `bebc6684ccacd763d28d2f336b9cfd17b356914addf37786afbe0c7440901ccc` |
| `prototype/device_wavefront_qualification.cpp` | `3ae284cd1e431ec22ccfe30efa4c3afef8cc91c5b87c92d696f84c2b088cbf89` |
| `CMakeLists.txt` | `6cffa15d014e2f817aa5723565a02bbeff1ea523f92fcae2a2b732400ad2ce64` |

> [!IMPORTANT]
> Cette note aide Claude à construire la voie GPU; elle ne modifie aucun
> prototype. Le snapshot possède maintenant un premier fichier `.cu`, audité
> ci-dessous comme candidat v3. Les exigences d'ingénierie sont formulées
> directement pour la v3; aucune mesure extérieure à ce snapshot ne lui sert de
> preuve.

## 1. Verdict et ordre des verrous

Le live possède désormais un `.cu`, une cible CUDA optionnelle et un lanceur.
Ce premier kernel calcule seulement un masque d'admissibilité
`(flat,direction)` sur des sommets que `navigate_shallow` a déjà entièrement
produits et matérialisés sur CPU. Il n'appelle ni `neighbour_along`, ni
`decide_child`; il ne construit aucun parent, enfant, curseur, sous-arbre ou
lot transactionnel. C'est donc un **microkernel de prédicat**, pas encore une
wavefront de reverse-search.

La porte hôte est positive sur ses sommets admis, mais rouge sur son contrat de
refus. Sa campagne permanente force 27 `kFlatOverflow`, puis saute exactement
ces 27 éléments avant la référence non bornée. Le compteur et le plancher
prouvent que le cap a été atteint; ils ne prouvent ni les 35 flats attendus du
premier contre-exemple, ni un replay, ni la conservation de la sortie.

L'ordre utile est donc :

1. corriger le P0 CPU `i128 -> int` du signe owner;
2. fermer la porte de refus du microkernel, puis un vrai kernel de **verdict parent** en arithmétique 64 bits;
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

Le `WavefrontJob` live n'authentifie aucun de ces invariants : `point_count`
n'est jamais lu par l'évaluateur, `root_size=0` avec pointeur nul rend encore
`kOk`, et tailles, indices, ordre/unicité/disjonction de coquille/intérieur ainsi
que `level==interior_size` sont supposés. Un validateur hôte doit rendre
`invalid_contract` avant toute multiplication de tailles, allocation ou copie.

Conclusion pratique : séparer un kernel parent 64 bits du kernel plus large de
génération du voisin. Employer `i128` partout est exact, mais paie inutilement
son coût sur la décision la plus fréquente. Les deux directions d'une base se
décident en un seul scan : chaque orientation met à jour `allow_minus` et
`allow_plus`, puis le site intérieur ou la somme des quatre racines conclut les
deux bits. Un probe indépendant sur les trois campagnes permanentes a comparé
25 118 sommets admis au format borné, dont les 27 ensuite refusés par le cap de
flats, et leurs 108 177 flats non plafonnés sans écart avec les deux appels
actuels. Le live rescane aujourd'hui la coquille pour chaque direction.

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
premiers membres, puis le premier membre non collinéaire avec eux. Mieux encore,
sur une coquille authentifiée de points distincts, une droite coupe la sphère en
au plus deux points : les trois premiers membres sont donc automatiquement non
collinéaires. Le scan général ne reste utile qu'au validateur hostile. Cela
remplace la boucle cubique de recherche de base par un accès constant après
construction du masque.

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

### 3.3 Ce que le premier microkernel prouve réellement

Le source du snapshot prévoit un même corps `host/device`; son exécution hôte
compare le masque terme à terme pour les sommets dont le nombre de flats ne
dépasse pas 32. Les quatre CTests hôte passent; la campagne nominale publie
19 019 sommets, 76 076 flats, 49 785 couples admissibles et zéro désaccord. Ce
crédit est strictement CPU : le transport CUDA n'est pas encore reçu.

Le cap 32 n'est pas cohérent avec la capacité de coquille 32. Sans quatre points
coplanaires, le nombre de flats d'une coquille de taille $m$ vaut
$\binom{m}{3}$ : il atteint déjà 35 pour $m=7$ et peut atteindre 4 960 pour
$m=32$. La fixture entière suivante est cosphérique, sans quadruplet coplanaire :

```text
centre=(100,100,100), rayon=25
(75,100,100) (76,93,100) (76,100,93) (76,100,107)
(80,85,100) (80,88,91) (80,91,112)
interieur optionnel=(100,100,100)
```

Le chemin CPU non borné énumère 35 flats. Le microkernel rend
`kFlatOverflow`, `flat_count=32`; avec l'intérieur, son masque partiel vaut
`0x940800000009`. La qualification exécute `continue` avant l'oracle pour ce
statut, puis `summarise` compte un refus mais zéro flat. La campagne permanente
à 27 refus reste donc verte sans comparer ni rejouer aucun des 27 préfixes.
Un lot entièrement refusé satisfait même la seule garde `total_vertices>0`.

Le masque ne certifie pas non plus l'ordre des flats lorsqu'il ne porte aucun
bit. Retirer le septième point ci-dessus donne 20 flats et un masque nul; une
permutation arbitraire de ces 20 flats conserve exactement `(count,mask)`. La
porte du parent doit comparer les items structurels `(closure,base,slot,verdict)`
ou, mieux, réduire la plus petite clef admissible et comparer cette clef exacte.

Enfin, admissibilité retour ne signifie pas filiation. La fixture permanente
du différentiel possède un retour admissible mais un couple antérieur
admissible, donc `decide_child=Reject`. Dans la fixture du futur voisin décrite
au paragraphe 6, le même sommet `w` est atteint depuis son vrai parent `v` et
depuis un autre sommet `u`; le retour est admissible dans les deux cas, mais
seule l'arête issue de `v` est acceptée. Un kernel limité aux bits locaux
dupliquerait `w`.

Pour fermer ce jalon, remplacer le masque fixe par une réduction paginée de la
plus petite clef admissible, ou implémenter un fallback entier reçu. Chaque
refus doit satisfaire `refused = replayed + pending + fatal`, et l'union des
résultats committés et rejoués doit égaler la séquence CPU complète avec sa
multiplicité.

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

La fixture entière des sept points du paragraphe 3.3 donne une partition
géométrique permanente : le catalogue borné au niveau 3 contient 18 sommets;
les six enfants canoniques de la racine portent des sous-arbres de tailles
`1,1,2,6,1,6`, disjoints et de somme 17. L'ordre de `navigate_shallow` n'est pas
topologique pour cet arbre : un descendant apparaît avant son parent canonique.
L'indice du batch live ne peut donc servir ni de `task_id`, ni de preuve d'ordre;
la clef de tâche doit être structurelle et authentifiée.

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
antichaîne. Cela évite les doublons et les préfixes partiellement publiés. Deux
modèles de donation sont licites : soit les racines actives forment une
antichaîne de sous-arbres complets, soit une continuation d'ancêtre retire
atomiquement de son domaine le sous-arbre donné. Dans le second modèle, les
racines ne forment plus une antichaîne, mais les **domaines de travail** restent
disjoints.

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

Chaque slot d'adjacence reçoit exactement une classe terminale : `no_neighbor`,
`outside_cut`, `reject_backward`, `reject_parent`, `descended`, `delegated`,
`fallback` ou `fatal`. Au point de donation, classification du slot, transfert
d'ownership et convention d'émission sont atomiques : soit le donneur émet la
racine et la tâche porte `emit_root=false`, soit seul le receveur l'émet. Une
sortie partielle publiée avant rollback dupliquerait le même préfixe au replay;
elle reste donc task-local jusqu'au commit, ou porte un identifiant de tentative
dont seules les tentatives committées sont compactées.

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
sont donc pas des fallbacks produit : ce sont des violations de contrat.
`shell>32` est un refus géométrique normal. Dans le design live, `flat_count>32`
est un second refus normal, mais il apparaît dès une coquille générique de sept
points; la voie cible doit plutôt paginer les flats ou rejouer le sommet entier.

Le live mesure désormais les admissions au bon endroit et porte
`kMaxInterior=32`, mais le différentiel et le nouveau qualificateur font encore
`continue` sur les refus. Les refus d'admission disparaissent même avant le
compteur du microkernel. Aucun des deux ne compare donc l'union « device
committé + replay CPU » au parcours de référence.

## 6. Produire réellement le voisin sur GPU

Le verrou principal n'est pas `decide_child`, mais `next(v,d)`. Un candidat de
voisin n'est autoritaire que si deux faits sont certifiés : aucun point n'a un
paramètre strictement plus petit, et le lot contient **tous** les ex æquo au
minimum. Un lot peut avoir $\Theta(n)$ membres.

Première voie exacte, sans promesse de débit : un bloc par
`(sommet,fermeture,direction)`, scan tuilé de tout $X$, réduction exacte du
minimum rationnel, puis second passage pour le lot complet. Cette baseline est
$O(n)$ par couple mais constitue un vrai kernel différentiel et ne matérialise
aucune mosaïque. La première passe réduit `Pencil.compare_t` en `i128`; la
seconde rescane les identifiants croissants et compacte **tous** les ex æquo
`compare_t==0` par prefix-sum. Le représentant du minimum ne remplace jamais le
lot. La largeur 384 bits reste nécessaire au tri global des niveaux, pas à cette
comparaison locale sur un même rayon.

Fixture entière minimale pour cette porte :

```text
0=(0,0,0) 1=(4,0,0) 2=(0,4,0) 3=(0,2,2)
4=(0,0,4) 5=(0,0,2) 6=(4,4,2)
v: shell={0,1,2,3}, interior={}, flat={0,1,2}, direction=+1
```

Le long des centres `(2,2,t)`, le point 4, rencontré avant 5 et 6 dans l'ordre
des identifiants, donne l'événement plus lointain `t=2`; les points 5 et 6
donnent ensemble le vrai minimum `t=1`. Le voisin exact est
`shell={0,1,2,5,6}`, `interior={3}`, `level=1`. Le lot compte deux ex æquo. La
direction opposée est non bornée. Cette fixture tue `first-valid-wins`, la perte
d'un ex æquo, le mauvais sens et l'oubli de transférer l'apex 3 vers l'intérieur.
Elle donne aussi un enfant positif : le retour est admissible et
`decide_child=Accept` depuis `v`; depuis un autre voisin incident au même `w`, le
retour reste admissible mais `decide_child=Reject` à cause d'un couple antérieur.

Le live contient déjà un travail supprimable avant portage : lorsque la boîte
indexée atteint une demi-largeur 65 535 autour d'une ancre u16, elle couvre
toute la grille déclarée et `touched` contient déjà tous les points. Si aucun
événement n'a été trouvé, le second `for z in X: absorb(z)` est un no-op exact.
Une campagne locale des cinq portes a compté 424 150 de ces
`exhaustive_scans`; ce nombre reste un diagnostic local, pas un reçu de débit.
Certifier « boîte = grille entière » permet de conclure le rayon non borné sans
ce second balayage; une mutation qui omet des points de la première couverture
doit faire rougir la porte.

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

## 9. Exigences d'ingénierie propres à la v3

La voie v3 conserve les exigences suivantes :

- vue d'index immuable authentifiée par epoch et digest;
- moteur exact `host/device` unique et faux launcher hôte distinct;
- statuts où `unknown`, `overflow` et `zero` sont trois résultats différents;
- échelle de largeurs fixes avec drain rationnel CPU;
- double buffer de frontière, compteurs de masse et rollback de vague complète;
- staging des sorties, commit atomique, reçus de transaction et digest final;
- contrôle hôte de tous les compteurs device avant toute allocation ou copie;
- enveloppe CUDA fail-closed : compilateur NVIDIA et version reçus, architecture
  exactement `120-real`, aucune option inconnue transmise implicitement;
- validation des tailles avant multiplication/allocation, puis contrôle de
  chaque retour CUDA, y compris événements et copies;
- zéro-initialisation des structures copiées afin que queues et padding ne
  transportent ni octets indéterminés ni faux digest.

Le CMake filtre maintenant correctement `-Wall -Wextra -Werror` sur le seul
C++; son rebuild hôte est vert. L'enveloppe reste ouverte : elle accepte
une architecture surchargée, initialise CUDA avant de fixer éventuellement
`120-real`, n'impose ni compilateur NVIDIA, ni toolkit corrigé pour `__int128`,
ni politique d'avertissements CUDA. Une session G4 a été lancée avant fermeture
de cette enveloppe et de la porte de replay; elle fournit un diagnostic de
microkernel, pas un précédent autorisant à ignorer ces invariants.

## 10. Porte permanente GPU v3

### 10.1 Non-vacuité

Exiger séparément : kernels lancés, nuages traités, sommets admis, flats et
couples décidés, tâches commencées/committées/rollbackées, décisions parent,
voisins produits, refus coquille/flats, drains arithmétiques, runs et replays
CPU. Une suppression complète du bloc GPU ou un mutant qui refuse tous les
sommets doit faire rougir la porte. Sur le snapshot, ce mutant passe encore les
trois campagnes : le plancher de refus sans plancher d'acceptation ni replay
rend la vacuité plus facile, pas plus difficile.

### 10.2 Fixtures minimales

| fixture | attente |
| --- | --- |
| déterminants owner 1290/1291 et alternés 1023/1024/1025 | même signe CPU/device, UBSan vert, mutant de troncature tué |
| coquille 33 sur sphère entière | rollback device puis replay du sous-arbre, aucune perte ni duplication |
| sept points cosphériques ci-dessus | 35 flats CPU, `flat_overflow` puis replay 35/35; mutant all-refused tué |
| centre `(100,100,100)`, coquille des six axes de rayon 50, trente intérieurs | admission, high-water intérieur 30, aucun faux overflow |
| fermeture masque, 20 000 cas générés | même base, même ordre et même multiplicité que le vecteur CPU |
| réduction `decide_child` | même verdict sur scan séquentiel, permutations et mutations d'ordre |
| voisin 0--6 du paragraphe 6 | minimum `t=1`, lot `{5,6}`, voisin et parent exacts; faux premier candidat tué |
| deux niveaux séparés de `3/4` mais même `double` | deux lots exacts distincts |
| owner signed cone | un claim exact et identité attendue, mutant first-wins/non signé tué |
| census dense | `RankOverflow` avec témoins exacts, jamais faux `CompleteCensus` |
| capacité de sortie tardive | rollback total puis replay, aucun préfixe publié |
| jobs hostiles | `root_size!=4`, coordonnées hors u16, indices/tailles invalides, shell non triée ou niveau incohérent donnent `invalid_contract` avant allocation |

Une coquille entière de taille 33 se construit autour de
`(32768,32768,32768)`, rayon 25 : six points axiaux, les 24 permutations signées
de `(24,7,0)` et trois points distincts de l'orbite `(20,15,0)`.

### 10.3 Reçu 50 k / G4

Le reçu final comporte au minimum : commit, diff source, compilateur/toolkit,
driver, modèle/architecture GPU, digest du binaire, digest du nuage, paramètres
50 k/K, temps par étage, octets et high-waters par conteneur, compteurs exacts
d'admission/refus/replay/commit, mutations, concordance byte-à-byte sous
répétitions et arrêt GCP certifié.

Le débit seul ne qualifie rien. La cible `.cu` existe maintenant, mais sa porte
de refus reste vacuable et aucun `nvcc`, `ptxas` ou GPU ne l'a encore exécutée.
La première campagne G4 utile vient après replay exact, planchers acceptés et
enveloppe CUDA fermée; avant cela elle ne mesurerait qu'un préfixe censuré du
microkernel.

GCP non utilisé pour cette note.
