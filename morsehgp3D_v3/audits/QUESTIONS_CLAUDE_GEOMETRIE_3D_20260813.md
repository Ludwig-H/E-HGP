# Questions de Claude — ce que la dimension trois offre et qu'on n'exploite pas

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Ces questions font suite à
[`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md).
Les trois primitives qui y sont proposées sont acceptées et vont être mises en
concurrence par la gate `counter-only`. Les questions ci-dessous portent sur ce
que la dimension trois — et le profil u16, qui est une **grille finie munie du
groupe octaédrique** — semblent offrir de plus, et que le chantier n'exploite
pas encore. Chacune est posée pour changer une décision, pas pour explorer.

Le sujet de fenêtre certifiée est en place et son identité tient dans les deux
sens contre le juge rationnel exhaustif :
`faux_positifs = identités_fausses = sur_certifiés = manques = 0` sur les quatre
familles à `n=24`, fenêtre 8. Le mur qu'il révèle est net : à `eight_clusters`,
`1 277` supports globaux ne sont **jamais proposés** contre `156` certifiables.

## Q1 — Une borne de paquet par chambre, plutôt que par ancre

Le dépôt a réfuté toute borne de degré Gabriel : deux constructions u16 à treize
voisins tuent le cap 12, et `smax` ne borne pas `|U_B|`. Ces contre-exemples
bornent-ils aussi le **degré par chambre** ?

Dans une chambre Yao de diamètre angulaire strictement inférieur à 60 degrés,
deux sites `x` et `y` de la chambre vérifient `|xy| < max(|ax|,|ay|)`. C'est
l'argument qui donne Yao-1. Pour q2 avec au plus `p` intérieurs, la question
est : le nombre de partenaires q2 de `a` **dans une même chambre** est-il borné
par une fonction de `p` seul, indépendante de `n` ?

Mon contre-exemple naturel — `m` sites à distance exactement 1 de `a` dans une
chambre étroite — échoue : deux sites à distance égale de `a` ne sont jamais
intérieurs à la boule diamétrale l'un de l'autre, donc ils sont tous partenaires
q2. Mais ces `m` sites sont alors **cosphériques**, et le dépôt traite déjà les
plateaux cosphériques par quotient ou refus. La question précise devient :

> existe-t-il une famille u16, **sans plateau cosphérique**, où une seule
> chambre porte `omega(1)` partenaires q2 de rang fermé au plus onze ?

Si la réponse est non, `48 * c(p)` majore les partenaires q2 et la source
devient linéaire par construction, pas par mesure. Si elle est oui, la
construction est la contre-fixture qu'il faut graver.

## Q2 — Le facteur trois du cutoff de hauteur est-il serré ?

La section 2.4 du déblocage donne : si `d` et `s` sont dans le même sous-cône et
`tau(d) >= 3 tau(s)`, alors `|s|^2/|d|^2 <= 1/3 < 9/25`, donc témoin q4. Le
facteur trois vient de `|v|^2 <= 3 tau(v)^2`, qui est l'inégalité de la chambre
canonique `x >= y >= z >= 0` — saturée seulement à la diagonale `x=y=z`.

Or les `432` sous-cônes sont bien plus étroits que la chambre : chacun est
engendré par trois rayons `v_ij=(3,i,j)` avec `0 <= j <= i <= 3`. Pour un
sous-cône donné, le rapport `|v|^2 / tau(v)^2` est majoré par le maximum sur ses
trois rayons, pas par 3. Le sous-cône `(3,0,0),(3,1,0),(3,1,1)` donne par
exemple `11/9` et non `3`.

> Peut-on remplacer le facteur universel 3 par un facteur `c_j` **tabulé par
> sous-cône**, avec `c_j = sqrt(max_j(|v|^2/tau(v)^2) / (9/25))` arrondi
> supérieurement en rationnel exact ?

Le gain est direct : le seuil `tau(d) >= c_j tau(s)` ferme plus de cibles pour
la même banque, donc réduit `R_dir`. Les `432` constantes sont finies, entières
après mise au même dénominateur, et vérifiables une fois pour toutes.

## Q3 — Filtre flottant certifié pour le prédicat, avant le repli exact

Les comparaisons utiles occupent 68 à 72 bits, et l'audit demande un lowering
deux limbes pour le device. En dimension trois et sur u16, les entrées sont
bornées : `|H| <= 1,29e10`, `E2 X2 <= 1,66e20`. Un filtre à la Shewchuk avec
borne d'erreur **prouvée** trancherait donc la quasi-totalité des cas en
flottant simple ou double, et ne tomberait sur les deux limbes que dans une
bande d'ambiguïté explicitement comptée.

> Une telle bande, avec sa borne d'erreur prouvée et son compteur
> `exact_fallback_calls`, est-elle recevable dans le chemin produit, ou le
> registre interdit-il tout flottant même sous certificat d'erreur ?

La spécification dit « FP certifié » côté produit `morsehgp3d/`, ce qui suggère
que oui, mais je ne veux pas l'inférer. La réponse change le dimensionnement
device d'un facteur important : les deux limbes ne seraient plus le débit
nominal mais l'exception.

## Q4 — Le groupe octaédrique comme facteur, pas comme boucle

Le profil u16 est une grille munie du groupe octaédrique complet, d'ordre 48, et
les 432 sous-cônes sont exactement `48 * 9`. La réduction canonique
`x >= y >= z >= 0` est donc une **action de groupe**, pas 48 cas indépendants.

> Un unique tri par la clé canonique réduite permet-il de partager la
> précomputation entre les 48 chambres, en ne conservant que neuf structures de
> dominance au lieu de 432 ?

Concrètement : la transformée `Phi_j(x) = (ell_j1, ell_j2, ell_j3, tau_j)` d'une
chambre `g * chambre_canonique` est `Phi_canonique(g^{-1} x)`. Si oui, le coût
mémoire des listes dirigées est divisé par 48 et le radix porte sur un seul jeu
de formes. Si non, quelle obstruction — l'attribution half-open des frontières,
ou l'ordre canonique des ex æquo ?

## Q5 — Le cœur de Jung est un problème de boules, donc 4D

Le certificat de cœur commun de la section 5 du déblocage teste : `huit ou neuf
PointId dans la boule ouverte de centre `m_0` et de rayon `R_AB`. C'est une
requête de boule, et le relèvement `phi(p) = (p, |p|^2)` transforme toute requête
de boule en requête de demi-espace en dimension quatre.

> Le relèvement 4D est interdit comme **arrangement** ; est-il interdit comme
> simple transformée de requête, c'est-à-dire un LBVH sur `phi(X)` interrogé par
> des demi-espaces, sans jamais matérialiser aucune cellule, face ou incidence ?

L'invariant d'architecture interdit la mosaïque et le catalogue de cofaces ; il
ne me semble pas interdire une structure de recherche. Si c'est permis, le cœur
commun, le census `I_B/U_B` et la requête top-`(12-q)` deviennent trois
instances d'une seule primitive, et le device n'a qu'un noyau à optimiser.

## Q6 — Ce que la finitude de la grille donne, et que l'asymptotique n'a pas

Les réfutations de sparsité — `Omega(n^2)` arêtes de Gabriel en dimension trois,
`Theta(m^4)` supports q4 cosphériques, `Omega(m^2)` facettes dans une composante
— sont établies dans `R^3` réel, avec des conditions strictes ouvertes.

Le profil contractuel est une grille `65536^3` **finie**. Pour une boule de
rayon `R` contenant au plus onze points de la grille, la densité locale borne
`R` par un `R_max` dépendant de la densité, et le nombre de directions
distinctes entre points de grille à distance bornée est fini.

> Existe-t-il une borne **finie et calculable** du nombre de supports propres
> positifs par point sur la grille u16, en fonction de `smax` et de la seule
> hypothèse « positions deux à deux distinctes » ? Ou bien une famille u16
> explicite atteint-elle déjà `Theta(n)` supports pour un seul point ?

Cette question décide si la route peut publier `resource_exhausted` comme
événement de mesure nulle, ou si elle doit le traiter comme un régime nominal.

## Q7 — Deux amas séparés : le cas que je veux traiter en premier

Le déblocage note que deux amas serrés séparés peuvent laisser
`Theta(|A| |B|)` candidatures maximales résiduelles, et le cœur commun est
précisément la primitive qui doit les fermer en bloc.

En dimension trois, deux amas de diamètres `S_A`, `S_B` séparés de `d` ont
toutes leurs paires inter-amas de longueur dans `[d - S, d + S]`. Toutes leurs
boules de milieu q4 sont donc **presque confondues**.

> Le certificat de cœur commun, appliqué au seul couple de blocs `(A,B)`, ferme-t-il
> l'intégralité de `A x B` en une seule requête dès que `d > 3S`, sans jamais
> énumérer une paire ? Et si oui, quel est le bon critère de **choix des blocs** :
> la WSPD, ou directement les nœuds du LBVH ?

C'est la question dont dépend l'ordre de mes implémentations : si le cœur commun
ferme le mur des amas en une requête par bloc, il passe avant la dominance 432.

## Ce que je fais en attendant

J'implémente la gate `counter-only` dans l'ordre : dominance 432 avec cutoff
ponctuel d'abord, parce que ses deux théorèmes sont déjà écrits et sans racine ;
groupes coniques ensuite ; WSPD/cœur commun en troisième, sauf si la réponse à
Q7 le fait passer devant.

Je ne lance **aucune session G4** pour l'instant, et je veux dire pourquoi
explicitement : une G4 mesurerait aujourd'hui une ordonnance déjà refusée. La
condition que je me fixe pour en lancer une est celle de la section 7 du
déblocage — deux pentes vertes du front et du résiduel, un cap d'octets absolu,
le lowering reçu des comparaisons 68 à 72 bits, un résiduel authentifié. La
seule exception que j'envisage avant cela est un usage de **compilation** : la
cible CUDA opt-in ne compile pas aujourd'hui, `run_anchor_point` exigeant
`theta_audit` et `density_guard` que `anchor_source_kernel.cu` ne transporte
pas, et `nvcc` est absent de la machine locale. Je répare d'abord l'ABI en
source ; si une vérification de compilation reste nécessaire, elle sera annoncée
et passera par les scripts gardés de `gcp-migration/`.
