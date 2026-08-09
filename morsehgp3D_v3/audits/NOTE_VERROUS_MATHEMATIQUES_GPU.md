# Verrous mathématiques GPU de MorseHGP3D v3

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_and_gpu_candidate_under_audit`,
`profile=quantized_u16_input_only`,
`mode=exact_gpu_wavefront_without_higher_order_mosaic`,
`public_status=not_claimed`.

Les constructions mathématiques de cette note sont indépendantes du live. Les
constats d'implémentation sont épinglés au snapshot committé suivant; les deltas
produit non committés sont réaudités séparément lorsqu'ils sont stables :

| objet | empreinte |
| --- | --- |
| snapshot de code et de claims audité | `180975e4a967475067961d4f215ab2f2a4f9760a` |
| `prototype/order_k_flats.hpp` | `b3ba750d938e3c4fa52453730011e2f8ed06e477b40ae971562c15aed07b65f5` |
| `prototype/flats_differential.cpp` | `6271f26ab8782fed0e46dd1200fa030d68d3036257c57f9f73320ca4f2ec1cb4` |
| `prototype/order_k_device_core.hpp` | `79382cf2857fb8da4efcecda8b9a164643fb4013c9a56cd6152f102daa155a3d` |
| `prototype/device_wavefront_job.hpp` | `cffe45646eb46ec44f4818ce8c8f0a3e7251084d8fb05c0cb79fbfae243fa31f` |
| `prototype/device_wavefront_kernel.cu` | `bebc6684ccacd763d28d2f336b9cfd17b356914addf37786afbe0c7440901ccc` |
| `prototype/device_wavefront_qualification.cpp` | `3ae284cd1e431ec22ccfe30efa4c3afef8cc91c5b87c92d696f84c2b088cbf89` |
| `CMakeLists.txt` | `e8ddd3c21eafa361d5c37cd8a585905db2a4bf8404639b8eefc72bd9df803c9f` |
| `prototype/scale_profile.cpp` | `e6c31f544d8275b3f89affde11b52e11972dd7e76cf9b556112c96a43d96aacb` |
| `prototype/admissible_pair_probe.cpp` à `180975e` | `fa3e464c422839f0485a032016831d3727fb42cbf1a9bd5be7a9427da3fe55fd` |

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

Pour les supports u16 d'arité au plus quatre, la borne conservatrice auditée
reste inférieure à $2^{108.2}<2^{109}$. Le census exact tient donc lui aussi
dans un `i128` signé.

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
`0x940800000009`. Le delta replay post-`8481b67` compare maintenant ce préfixe,
compte les refus d'admission, sépare les planchers et tue le mutant qui refuse
tout. La campagne à 27 refus crédite aussi 27 rejeux et conserve exactement les
masses scalaires de flats et de couples. C'est une fermeture positive de la
vacuité antérieure.

Ce n'est pas encore le replay exigé ici. `reference_vertex` précalcule l'oracle
avant admission; sur refus, aucun item du suffixe n'est conservé ou émis, seuls
deux comptes sont additionnés. Les flats 32--34 de la fixture peuvent être
permutés ou substitués à masse constante sans rougir. `pending>0` est autorisé,
un statut inconnu est traité comme `kOk`, et le statut intérieur « hors contrat »
est rejoué au lieu d'être fatal.

Le masque ne certifie pas non plus l'ordre des flats lorsqu'il ne porte aucun
bit. Retirer le septième point ci-dessus donne 20 flats et un masque nul; une
permutation arbitraire de ces 20 flats conserve exactement `(count,mask)`. Le
nouveau FNV64 ordonne base, taille et bits, mais omet les identifiants de
fermeture et reste collisionnable. La porte du parent doit comparer les items
structurels `(closure,base,slot,verdict)` ou, mieux, réduire la plus petite clef
admissible et comparer cette clef exacte.

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
sommets doit faire rougir la porte. Le delta post-`8481b67` tue désormais ce
mutant par des planchers séparés. Il ne ferme pas encore la seconde moitié :
aucun payload de replay ou de tâche n'est publié.

### 10.2 Fixtures minimales

| fixture | attente |
| --- | --- |
| déterminants owner 1290/1291 et alternés 1023/1024/1025 | même signe CPU/device, UBSan vert, mutant de troncature tué |
| coquille 33 sur sphère entière | rollback device puis replay du sous-arbre, aucune perte ni duplication |
| sept points cosphériques ci-dessus | 35 flats CPU, `flat_overflow` puis replay 35/35; mutant all-refused tué |
| centre `(100,100,100)`, coquille des six axes de rayon 50, trente intérieurs | admission, high-water intérieur 30, aucun faux overflow |
| même domaine avec 31 ou 32 intérieurs | `invalid_contract`, car la coupe produit borne l'intérieur à 30 |
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

Le débit seul ne qualifie rien. Le commit `78583f1` rapporte bien une compilation
`nvcc` et quatre exécutions G4 `sm_120` sans écart entre les `VertexVerdict`
bornés hôte/device. C'est un crédit positif pour le transport du microkernel.
La porte de refus reste pourtant vacuable : les 27 refus ne sont jamais comparés
à la vérité non bornée ni rejoués. Aucun stdout brut, commande complète, version
patch du toolkit, hash binaire, PTX/cubin, rapport `ptxas`, digest d'entrée ou
répétition n'est versionné. Les temps sont kernel-only et excluent production
CPU du batch, allocations et transferts.

La première campagne représente 1 031 640 appels directionnels en `0,224 ms`,
soit environ 4,61 milliards d'appels par seconde, et non un milliard. Le débit
575 M sommets/s ne se transporte ni aux coquilles dégénérées, ni à un terrain
50 k non borné, ni aux étages absents. Une nouvelle session qualifiante vient
après replay exact, planchers acceptés/rejoués, enveloppe CUDA fermée et reçu
versionné.

### 10.4 Profil d'échelle et source critique directe

Le profileur `f851374` corrige positivement la densité décroissante du profil
cube antérieur. Il reste un diagnostic CPU sur une densité codée en dur et une
nappe synthétique; il ignore les nuages non `kOk`, déduplique en $O(n^2)$ hors
chrono, exclut la construction d'index du temps navigation et ne mesure aucun
aval. Ni une graine, ni un ratio `n<=400` ne borne 50 k.

Le delta `70ead99` prolonge la table jusqu'à `n=800`. La diminution de trois
incréments ne prouve pas que la suite converge et n'identifie pas une asymptote.
La table mélange en outre `repeats=2` à `n=100` et `repeats=1` à `n=200`,
donc ses incréments ne partagent pas le même estimateur.
Aux tailles 400/800, le catalogue de la nappe s'écarte du cube de 9,7 % puis
14,3 %; la densité seule n'explique donc pas les sorties observées.
Les projections `1 430/390` par point, puis `7,1e7/1,9e7` à 50 k, `0,124 s` et
un facteur `15--40`, restent conditionnelles à un modèle géométrique non ajusté,
au débit de la campagne G4 la plus favorable et au facteur de pipeline non reçu.

Si Claude choisit la voie directe, le verrou n'est pas « filtrer 6,5 fois » mais
produire de façon complète et output-sensitive les supports critiques
`U`, `|U|<=4`, sans les obtenir depuis un propriétaire du terrain. Chaque
élagage doit fournir un certificat exact `rank > s_max`; chaque émission doit
porter miniboule, census terminal complet, support canonique, propriétaire et
clef de déduplication. La note de source directe ferme le passage d'une sphère
déjà certifiée vers ses cofaces, pas encore la production du stream initial.

Pour la voie reverse, la baseline `next` exacte à deux passes est une vérité de
qualification. En position générale elle coûte environ `16*n*V` visites de points;
elle doit donc être remplacée par un index terminal certifié ou des requêtes
groupées prouvées avant 50 k. Les deux voies restent ouvertes; aucune mesure
actuelle ne prouve que l'une est mathématiquement nécessaire.

### 10.5 Premier jalon direct falsifiable : les supports d'arité deux

Le P0 du probe de `40ad152` est fermé à `180975e` (`fa3e464c...`) : le minimum
fermé est le complément exact du maximum ouvert, la projection étroite reste
dans `i128`, six fixtures et le mutant 2/5 sont permanents. Un oracle indépendant
en multiprécision donne zéro écart sur 74 613 multisets planaires et 9 593 paires
3D. Ce résultat qualifie le sweep borné, pas encore ce jalon industriel : la
« vérité » partage `flat_catalogue`, porte seulement sur le catalogue fermé et
les tables finies ne prouvent ni $O(n\log n)$ ni une masse à 50 k.

Les rangs k-NN sont maintenant de compétition, leurs ex æquo sont groupés, le
maximum vrai est indépendant de l'admission et leur coût est chronométré à part.
Le CTest n'asserte toutefois aucun rang ni histogramme, et le chrono du filtre
inclut par défaut le mutant quadratique. Ces mesures restent un diagnostic; elles
ne définissent aucune frontière complète sans certificat de la masse écartée.

Aucune borne petite sur ce rang n'est implicite. Pour `K=16000`, la fixture u16
`p=(16001,0,0)`, `u=(48003,0,0)` et les points
`p-(i,0,0),u+(i,0,0)`, `1<=i<=K`, a une boule diamétrale ouverte vide :
`A(p,u)=2`. Les deux extrémités ont pourtant chacune `K` voisins strictement
plus proches, donc le rang croisé minimal vaut 16 001. Une frontière k-NN
bornée n'est jamais complète sans un certificat distinct couvrant sa masse
écartée.

Le lemme peut néanmoins être renforcé proprement pour la source **ouverte**.
Pour une paire `(p,u)`, soit $D_{pu}^{\circ}$ la boule diamétrale ouverte et
définir $A(p,u)=2+\min_H\lvert(X\setminus\{p,u\})\cap D_{pu}^{\circ}\cap H\rvert$,
où $H$ parcourt les demi-espaces fermés dont le plan contient la droite
`(p,u)`. Si `p` et `u` appartiennent à une sphère `B(c,R)`, poser
$m=(p+u)/2$ et $v=c-m$. Alors $v\perp(u-p)$ et
$R^2=\rho^2+\lVert v\rVert^2$. En choisissant le demi-espace du côté de `v`,
tout $y\in D_{pu}^{\circ}\cap H$ satisfait
$\lVert y-c\rVert^2<\rho^2+\lVert v\rVert^2=R^2$; si `v=0`, tout demi-espace
convient. Donc, pour toute paire d'un support utile d'arité
$q\in\{2,3,4\}$, $A(p,u)\leq2+\lvert I\rvert\leq q+\lvert I\rvert\leq s_{\max}$.
L'ouverture de $D_{pu}^{\circ}$ est essentielle : un extra-shell sur sa
frontière ne devient pas un faux témoin intérieur.

Le sweep entier exact groupe les rayons primitifs et calcule le maximum de
masse dans un demi-plan **ouvert**; si `m` est la masse projetée non nulle et
`always` celle de la droite, le résultat est
`minimum_closed=always+m-maximum_open`. Les rayons confondus, antipodes,
points sur la droite et frontière diamétrale ont des fixtures séparées. Une
sonde exhaustive rationnelle sur 200 nuages de dix points a vérifié 59 154
inégalités $A(p,u)\leq q+\lvert I\rvert$ sans écart; la preuve ci-dessus reste
l'autorité.

La représentation device peut rester étroite. Pour une paire de points distincts,
poser $d=u-p\neq0$, $e=2z-p-u$ et $r=d\times e$. Si $q$ est la projection
orthogonale de $e$ sur $d^\perp$, alors $r=d\times q$; l'identité
$d\times(d\times q)=-\lVert d\rVert^2q$ montre que $r=0$ si et seulement si
$q=0$. Choisir une fois par paire un axe canonique $k$ tel que $d_k\neq0$, fixer
l'ordre des deux autres coordonnées et conserver ces deux composantes de $r$ :
la restriction de $\pi_k:x\mapsto(x_i)_{i\neq k}$ à $d^\perp$ est injective,
donc cet isomorphisme préserve rayons, antipodes et demi-plans centraux, à une
orientation globale fixée près.

Sous u16, $r=2(u-p)\times(z-p)$ donne
$\lvert r_i\rvert\leq2\times65535^2<2^{33}$; un déterminant angulaire de deux
rayons vérifie donc une borne stricte $<2^{67}$. `i64` suffit aux composantes et
`i128` à leurs déterminants, à condition de convertir les opérandes vers `i64`
**avant** chaque produit du cross, puis vers `i128` **avant** les produits du
déterminant. Cette construction est une recommandation auditée, pas le live :
le probe committé construit encore une base entière plus large en `i128`.

Ce lemme n'impose aucune longueur maximale à une paire : l'autre demi-boule
peut être arbitrairement peuplée et un grand vide peut produire une paire longue.
Il ne fournit donc pas, seul, un rayon de voisinage ni une borne de degré. Après
un `center-cover` certifié, il peut filtrer le graphe de rayon; tout vrai support
de taille trois ou quatre y induit alors un triangle ou un `K4`. L'énumération
par intersections est exacte mais reste conditionnelle à ses propres caps et
replays.

Pour une paire `(u,v)`, la fonction exacte
$\Phi_{u,v}(x)=(x-u)\mathbin{\cdot}(x-v)$ est strictement négative exactement à
l'intérieur de la boule de diamètre `[u,v]`. Une tâche spatiale portant des
paires peut donc être rejetée si elle fournit `s_max-1` témoins distincts, hors
support, dont l'inégalité stricte est certifiée pour **toutes** les paires de la
tâche. L'égalité reste sur la coquille et ne compte jamais comme témoin strict.
Sous u16, ce prédicat et ses bornes de boîte doivent être redémontrés en largeur
entière avant device.

Le ledger autoritaire partitionne toute la masse des paires :
$M_{\mathrm{candidate}}+M_{\mathrm{pruned}}+M_{\mathrm{unresolved}}=\binom{n}{2}$.
Les splits sont disjoints, les saturations produisent une frontière reprenable,
et aucune publication n'est permise tant que `M_unresolved != 0`. Chaque paire
survivante passe ensuite par un census global terminal, la coquille complète,
la canonicalisation et la déduplication exactes.

Ce jalon teste immédiatement si un flux direct sparse est plausible sans
construire cellule, coface ou incidence. Il ne prouve rien pour les arités trois
et quatre, dont les frontières restent indépendantes. Il doit aussi compter les
témoins **strictement** intérieurs : filtrer seulement par rang fermé censure les
cofaces Gabriel ouvertes à grand extra-shell, précisément hors de l'univers
mesuré par `flat_catalogue(...,s_max)`.

### 10.6 Source directe arités deux à quatre sous `center-cover + degree`

Voici une porte exacte qui retire réellement le propriétaire shallow de l'entrée.
Soit `X` un nuage u16 à coordonnées distinctes. Pour
`q` dans `{2,3,4}`, poser :

$$t_q=s_{\max}-q+1.$$

Le profil produit `s_max=11` donne donc `t_q=12-q`. Choisir un entier `Q_q`
et une subdivision canonique de la boîte de `X` en AABB à bornes entières.
Avant toute multiplication, valider
`1<=Q_q<=Q_root=sum(span_i^2)+1` puis calculer `4*Q_q` en `u64` checked.
Cette restriction ne retire aucune totalité : le cover racine avec `Q_root`
est toujours disponible lorsque `n>=t_q`. Chaque feuille `C` authentifie
`t_q` PointId distincts
`W_C` tels que, pour tout `w` de `W_C`, le maximum aux huit coins vérifie :

$$\sum_{i=1}^{3}\max\left\lbrace (w_i-C_i^-)^2,(w_i-C_i^+)^2\right\rbrace<Q_q.$$

Le certificat porte sur la fermeture de la feuille; l'ownership des centres est
half-open et déterministe. Un centre de miniboule propre appartient à
`conv(U)`, donc à la boîte couverte.

**Lemme de rayon.** Toute circumboule propre de support `q` qui satisfait
`q+|I|<=s_max` a `beta<Q_q`. Sinon, les `t_q` témoins de sa feuille sont tous
strictement intérieurs et donnent `q+|I|>=s_max+1`, contradiction. Par conséquent,
pour `p` dans `U` et tout membre fermé `x` de la boule :

$$\lVert x-p\rVert^2\leq4\,\mathrm{beta}(U)<4Q_q.$$

La liste exacte `N_q(p)={x!=p:dist2(x,p)<4Q_q}` contient donc support,
intérieur et coquille complets. Énumérer `U` une seule fois par
`p=min PointId(U)` et `U\{p}` dans `N_q^+(p)` est complet pour toute sortie de la
fenêtre, sans arrangement ni mosaïque d'ordre supérieur.

**Ordre algorithmique non circulaire.** Après le test d'indépendance et de
barycentriques strictement positives, localiser exactement le centre rationnel
dans sa feuille et classifier ses `t_q` témoins par `sphere_side` en `i128` :

- tous strictement intérieurs donnent
  `AboveInteriorWindow{witnesses}`;
- sinon un témoin non intérieur satisfait `beta<=dist2<Q_q`; seulement alors le
  scan de `N_q(p)` est un census global complet.

Ainsi un grand candidat localement visible ne peut pas être accepté par omission.
Comparer directement `beta<Q_q` pour l'arité quatre peut dépasser `i128` :
une fixture u16 extrême produit déjà des carrés de numérateur sur 132 bits.
La banque garde le hot path point--sphère sous 128 bits. Les distances u16 du
cover tiennent sous $2^{34}$ et `4Q_q` sous $2^{36}$ avec le fallback racine;
il faut élargir avant soustraction et carré. L'inégalité du cover est
strictement `<Q_q`; `<=` est faux au bord.

La capability minimale scelle digest/epoch du nuage, `q`, `s_max`, `Q_q`, boîte,
topologie et digest du cover, témoins et maxima par feuille, puis digest CSR des
voisinages, degré complet maximal, profondeur du locator et compteurs de
construction. Le reçu publie `locator_steps_total/max`; sans locator à profondeur
contractuelle, le coût porte explicitement le terme `C_q*L_q(P_q)`.
`CandidateDecision` vaut seulement `NotProper`,
`AboveInteriorWindow{witnesses}` ou
`CompleteSphere{SphereKey,U,I,S,census_digest}`. `SourceOutcome` porte
séparément `Complete`, `UnsupportedInput/Degeneracy`,
`OutsidePerformanceEnvelope{resume_token}`, `IncompleteResume`,
`InvalidContract` ou `ArithmeticFailure`. Les deux états de reprise ne sont ni
des verdicts scientifiques ni une absence. Aucun cap ne tronque une liste, un
census ou un segment publié.

Avec `d_q(p)=|N_q(p)|` et `d_q^+(p)` le degré vers les PointId supérieurs, publier
en `u128` :

$$C_q=\sum_p\binom{d_q^+(p)}{q-1},\qquad T_q=\sum_p d_q(p)\binom{d_q^+(p)}{q-1},\qquad H_q\leq T_q+t_qC_q.$$

`C_q` compte les candidats, `T_q` borne les classifications locales et le terme
`t_q C_q` les prétests de banque. Tests de distance et nœuds de construction du
cover, octets, high-waters, locator, groupement/tri et queue maximale par ancre
restent séparés. Sous une capability reçue `P_q=O(n)`, degré complet capé et
locator de profondeur constante scellée, le travail géométrique vaut
`O(P_q+n*D_q^q+sortie)`; sinon ajouter `C_q*L_q(P_q)` et les coûts de
construction, tri, sortie et replay. Sans cette porte, une feuille racine avec
`Q_q=sum(span_i^2)+1` donne le repli exhaustif exact; elle ne donne aucun SLO.
Si `n<t_q`, l'énumération directe est déjà bornée et remplace la banque.
`RelevantGP` n'implique ni ce cap de degré ni une sortie linéaire, même pour les
paires. À 50 k, `C_q/T_q/H_q` tiennent dans `u128` pour `q<=4`, mais le nombre
d'expansions Gabriel ouvertes peut dépasser 128 bits; les compteurs de sortie
hors porte régulière sont donc multiprécision ou saturés avec statut typé, jamais
tronqués.

La construction des voisinages possède elle aussi une borne simple. Prendre `a`
égal à la plus grande puissance de deux telle que `a*a<=Q_q`, puis trier les
points par cellule de grille de pas `a`. Deux points d'une même cellule ont une
distance carrée strictement inférieure à `3*a*a<4*Q_q`; une cellule contient donc
au plus `D_q+1` points sous la capability. Comme `Q_q<4*a*a`, tout voisin est
dans l'un des `9^3=729` offsets de cellules. Les offsets dont la distance AABB
minimale est au moins `4*Q_q` sont rejetés exactement. Le plafond de construction
est ainsi `729*n*(D_q+1)` tests de distance avant déduplication, avec radix sur
des clefs fixes; ce compteur reste distinct de `H_q`.

Le census d'une `CompleteSphere` agrège les occurrences par `SphereKey`. Depuis
une sphère `base=a,num=n,den=d>0`, former
`(A,Bx,By,Bz,C)=(d,-2*(d*a+n),d*|a|^2+2*n.a)`, diviser les cinq coefficients
par leur gcd positif et imposer `A>0`. Cette clef primitive `i128` est commune
aux lanes deux, trois et quatre; les supports et provenances restent séparés.
Pour le catalogue fermé, le census impose `|I|+|S|<=s_max`. Pour Gabriel ouvert, il conserve tout
extra-shell et développe les cofaces contenant au moins un support. La source
certifie elle-même `RelevantGP` : après census terminal et vérification
`q+|I|<=s_max`, un support admissible avec `S\U` non vide est exactement un témoin
de violation. Les trois lanes `q=2,3,4` sont obligatoires; les singletons sont
traités séparément et les coordonnées dupliquées restent hors de cette capability.
`flat_catalogue(...,s_max)` ne mesure ni ces
témoins au-delà du rang fermé ni leur expansion ouverte.

Deux contrôles indépendants renforcent la preuve sans la remplacer : 4 105
supports propres aléatoires, dont 4 085 dans la fenêtre, n'ont produit aucune
violation du cover/rayon; dix petits nuages ont donné le même verdict
`RelevantGP` entre la définition exhaustive et le critère par supports. Le
lemme de paire ouverte de la section précédente a en outre passé 59 154
inégalités sur 200 nuages bornés.

La norme active doit toutefois être raffinée avant de déclarer cette voie
conforme. Elle exige actuellement la coquille complète de tout support rencontré
dès que `|I|<=s_max-2`. Pour `q=3/4`, le terminal
`AboveInteriorWindow{t_q witnesses}` peut conclure plus tôt tout en prouvant que
`q+|I|>s_max`. Il est mathématiquement suffisant pour l'antécédent produit,
mais interdit littéralement par le contrat uniforme. Claude doit versionner ce
terminal certifié ou conserver un census global; une optimisation ne peut pas
modifier silencieusement la norme.

Les plafonds illustrent enfin le verrou restant. À 50 k et degré uniforme
`D=8/12/16/24`, la borne `H=sum_q H_q` vaut respectivement environ
`75,8 M / 302,5 M / 842,8 M / 3,735 G` appels point--sphère, avant construction
des voisins, tests `Proper`, locator, tri et sortie. Ce ne sont ni des mesures ni
des caps recommandés : chaque débit exact doit être reçu sur le kernel concerné.

La résidence cible se limite à `X`, trois covers et leurs banques, trois CSR de
voisinage, des offsets combinadiques par ancre, un chunk de candidats et des runs
de `SphereKey`. `C_q` n'est jamais matérialisé : count/scan puis curseurs
combinadiques le streament. Aucun sommet ou flat d'arrangement, cellule/coface
order-k, `Gamma` ou mosaïque n'est construit. Le high-water autoritaire porte sur
`n + sum(P_q) + sum(L_q) + chunk + run`, puis sépare la sortie persistante.

Fixtures prioritaires pour `s_max=11` :

```text
centre c=(10,10,10)
q2 U={(0,10,10),(20,10,10)}, beta=100, I={c, six voisins axiaux unitaires,
    (12,10,10),(8,10,10)}
q3 U={(15,10,10),(7,14,10),(7,6,10)}, beta=25,
    bary=(3/8,5/16,5/16), I={c, six voisins axiaux unitaires,(12,10,10)}
q4 U={(9,9,9),(9,11,11),(11,9,11),(11,11,9)}, beta=3,
    bary=(1/4,1/4,1/4,1/4), I={c, six voisins axiaux unitaires}
```

Ces bases portent respectivement neuf, huit et sept intérieurs, donc
`q+|I|=11`. Ajouter `(10,12,10)` aux deux premières ou `(11,11,10)` à la
troisième donne la frontière 12 et doit rendre `AboveInteriorWindow`. Ajouter
respectivement `(10,20,10)`, `(10,10,15)` ou `(9,9,11)` sans l'intérieur
supplémentaire donne un extra-shell et une violation `RelevantGP`; ajouter les
deux doit rester `AboveInteriorWindow`.

Compléter par le cube à six supports minimaux, un grand candidat rejeté par la
banque avant census local, une cellule dont seul le coin le plus lointain atteint
`Q_q`, un témoin dupliqué, un centre sur split-plane, un voisinage au cap puis
replay, et permutation du stockage à PointId stables. Les mutants `<` vers `<=`,
coin maximal vers centre, `t_q` diminué de un, scan avant contraposée, rayon
`Q_q` au lieu de `4Q_q`, census sur `N_q^+` seulement et publication avant
watermark doivent tous rougir. Une variante portant simultanément extra-shell
et `t_q` intérieurs doit rendre `AboveInteriorWindow`, pas une violation GP : le
scan continue après le premier extra jusqu'au seuil intérieur ou au watermark.

GCP utilisé uniquement en lecture seule pour l'audit d'état final; aucune VM
créée, démarrée, arrêtée ou modifiée par l'auditeur.
