# Note mathématique : niveau shallow contre rang fermé variable

Date : 9 août 2026 UTC.

Snapshot : commit `5a6cdb1af030a264ce07adddd312be2c458459b4`, `prototype/order_k_bfs.hpp` SHA-256 `c1548b3ce5336a423ceb7f069ba3311749efdca057025bbde1c63333be193457`.

> [!CAUTION]
> **Le théorème de connectivité shallow ne justifie pas la coupe `shell.size() + level <= rank_ceiling` du commit `5a6cdb1`.** Il connecte les sommets dont le nombre d'intérieurs stricts est borné. Dès que la taille de coquille varie, la coupe par rang fermé peut retirer un sommet de niveau zéro qui est le seul pont entre deux sommets de petit rang.

L'[audit complet des dégénérescences](AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md) contient les probes, les quatre fixtures, les défauts de coquille constante, l'audit de l'oracle et le verdict 50 k. La présente note isole uniquement l'implication mathématique pour le reverse search ; elle ne duplique pas cet audit général.

## 1. Deux filtrations différentes

Pour un sommet $v$, notons $S(v)$ sa coquille complète et

$$\ell(v)=\#\lbrace i:L_i(v)<0\rbrace.$$

La preuve polyédrique de [`AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md) porte sur le graphe induit par $\ell(v)\leq k$. Le parent de [`AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md`](AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md) reste lui aussi dans cette filtration, car il suit une arête d'un polyèdre de chambre et ne peut qu'enlever des intérieurs stricts.

Le commit multiplicitaire coupe au contraire selon

$$\rho(v)=\ell(v)+\lvert S(v)\rvert\leq R.$$

Sous arrangement simple, $\lvert S(v)\rvert=4$ et les deux filtrations diffèrent seulement d'une constante. Sous coquille variable, aucune implication de connectivité ne subsiste.

## 2. Contre-exemple exact commité dans l'audit

La fixture u16 à neuf points reproduite dans l'[audit complet des dégénérescences](AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md) satisfait `RelevantGP` à `s_max=2`.

Son graphe de niveau zéro contient :

- huit sommets simples de rang fermé quatre ;
- un sommet vide à coquille cinq, donc de rang fermé cinq ;
- deux composantes après suppression des sommets de rang supérieur à quatre.

Le sommet à coquille cinq est un pont nécessaire vers le sommet simple `{0,1,2,8}`. Le code démarre dans l'autre composante, rend 18 événements au lieu de 21 et omet exactement `{0,8}`, `{1,8}` et `{2,8}`, avec `out_of_domain=0`.

Ce témoin ferme la question logique : une preuve pour $\ell\leq k$ ne peut pas être citée pour élaguer selon $\rho\leq R$.

## 3. Conséquence précise pour un parent de reverse search

Le parent canonique à marge verticale s'étend abstraitement aux sommets non simples du **vrai** graphe de niveau shallow : un polyèdre pointé non simple possède encore une arête améliorante, et le potentiel retire ou conserve l'ensemble des intérieurs stricts.

Mais une arborescence couvrante du graphe $\ell\leq k$ peut traverser un sommet dont $\lvert S(v)\rvert$ est arbitrairement grand. `RelevantGP` ne borne pas les coquilles non utiles. Deux choix seulement restent mathématiquement cohérents :

1. parcourir par niveau strict, puis appliquer le rang fermé uniquement à la publication ;
2. fournir un nouveau théorème de contournement des sommets à grande coquille et une construction exacte de ce contournement.

Le premier choix restaure la complétude mais expose le coût du lien local. Avec $m$ points sur une coquille, les triples bruts sont au nombre de $\binom{m}{3}$ et plusieurs peuvent définir la même droite. Il faut quotienter les triples par leur flat de rang trois canonique ; `seen` après génération ne retire pas ce coût. Le second choix correspond à l'arrangement local des directions sur $S^{2}$ déjà identifié comme dette dans la spécification.

Une perturbation symbolique n'est pas une troisième solution gratuite : elle peut éclater un seul sommet multiple en un nombre combinatoire de sommets simples, puis exige une preuve de quotient vers le centre, la coquille et le lot exacts d'origine.

## 4. Décision

- parent reverse search abstrait sur la filtration par niveau : **théorème conditionnel conservé** ;
- parent restreint au plafond de rang fermé du commit : **réfuté** ;
- harvest `s_max+2` sous événements multiples : **réfuté** ;
- application produit sous `RelevantGP` : **ouverte et NO-GO au snapshot**.

GCP non utilisé.
