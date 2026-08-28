# Question Claude — la WSPD ne se généralise pas aux triplets : théorème et contre-famille (28 août 2026)

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

L'utilisateur a proposé de « généraliser le WSPD aux triplets » pour obtenir
directement les ancres q3. La règle du dépôt est qu'une piste ne se ferme pas
sur un benchmark : il faut un théorème. En voici un, élémentaire, que j'ai
vérifié pas à pas et que je soumets à réception.

## Théorème (borne inférieure quadratique pour toute décomposition ternaire séparée)

**Définition.** Un triplet de facteurs $(A, B, C)$ est *$s$-séparé* si toute
distance entre deux facteurs distincts vaut au moins
$s \cdot \max(\mathrm{diam} A, \mathrm{diam} B, \mathrm{diam} C)$.
Une *décomposition ternaire $s$-séparée* de $P$ est une famille de tels
triplets telle que tout triplet de points distincts de $P$ soit couvert
exactement une fois.

**Énoncé.** Pour tout $s > 4$, il existe des nuages $P$ de taille $n$ dont
toute décomposition ternaire $s$-séparée a **$\Omega(n^{2})$ triplets**.

**Preuve.** Soit $\delta > 0$ avec $\delta/s < 1$. Prendre
$P = P_0 \cup P_1$ où $P_0$ compte $n/2$ points dans une boule de diamètre
$\delta$, et $P_1$ compte $n/2$ points deux à deux distants d'au moins 1 et à
distance au moins 1 de $P_0$.

Soit $r \in P_1$ et $p, q \in P_0$ distincts, et soit $(A, B, C)$ le triplet
qui couvre $\lbrace p, q, r \rbrace$, avec $p \in A$, $q \in B$, $r \in C$.
La séparation donne
$s \cdot \max \mathrm{diam} \le \mathrm{dist}(A, B) \le \lvert pq \rvert \le \delta$,
donc $\max \mathrm{diam} \le \delta/s < 1$. Les trois facteurs ont donc
un diamètre strictement inférieur à 1. Comme les points de $P_1$ sont deux à
deux distants d'au moins 1 **et** à distance au moins 1 de $P_0$, aucun
facteur ne peut contenir deux points de $P_1$, ni un point de $P_1$ et un
point de $P_0$ : donc $C = \lbrace r \rbrace$ et $A, B \subseteq P_0$.

À $r$ fixé, la famille des couples $(A, B)$ ainsi employés couvre **toutes**
les paires de $P_0$ et chacun de ces couples est $s$-séparé : c'est une
$s$-WSPD de $P_0$. Pour $s > 4$, les arêtes représentatives d'une $s$-WSPD
forment un $t$-spanner connexe de $P_0$ (Callahan–Kosaraju, $t = (s+4)/(s-4)$),
donc leur nombre est au moins $\lvert P_0 \rvert - 1$. Chaque $r$ donne des
triplets distincts, puisque $C = \lbrace r \rbrace$ l'identifie. Le total est
donc au moins $(n/2)(n/2 - 1) = \Omega(n^{2})$. $\square$

**Cause structurelle.** Une *paire* séparée tolère deux échelles ; un
*triplet* impose une échelle commune, fixée par la plus petite distance, et
une extension fixée par la plus grande — de rapport non borné. C'est
exactement le régime du triangle aigu $89°$–$89°$–$2°$ : deux sommets à
distance $\sin 2°$, le troisième à distance 1. Or **c'est précisément le
régime que la lane q3 doit produire.**

## Corollaire (la séparation ne borne pas le circumrayon)

Trois points mutuellement éloignés peuvent être quasi colinéaires : l'aire
tend vers 0 et $R = abc/(4\,\mathcal{A})$ explose, le circumcentre étant
arbitrairement délocalisé. La séparation contrôle les **distances**, jamais la
**forme**. Or ce dont le census et le filtre de profondeur ont besoin est le
**centre**, pas les distances. Aucune notion de séparation ternaire ne
localise donc le centre.

**Ce qui localise le centre est déjà dans la v5** : l'ancrage sur l'arête
**maximale** joint à l'acuité (§ 10.1 : $\lvert c - m \rvert^{2} \le D^{2}/12$
en q3, $\le D^{2}/8$ en q4 par Jung). Autrement dit, le théorème ci-dessus ne
condamne pas la v5 — **il justifie sa conception actuelle** : le troisième
sommet n'est pas un facteur de décomposition, il est compté comme témoin
contre l'arbre.

## Ce que je demande

- **V43** — recevez-vous ce théorème ? Il est élémentaire et je l'ai vérifié
  ligne à ligne ; il n'utilise que la borne de spanner de Callahan–Kosaraju,
  qui est déjà l'autorité de la WSPD dans ce dépôt.
- **V44** — si oui, l'entrée de `PISTES_FERMEES.md` doit-elle être
  « décomposition **ternaire séparée** (WSPD d'ordre 3) comme source des
  ancres q3 » — fermée **par théorème**, avec la contre-famille
  $P_0 \cup P_1$ gravée comme fixture — plutôt qu'une formulation plus large
  qui interdirait aussi les décompositions ternaires **non séparées** (dont
  le troisième facteur serait une cellule de l'espace des centres, et non un
  nuage de points) ?
- **V45** — la contre-famille demande une réalisation entière sous le profil
  u16. Je propose : $P_0$ = $m$ points d'une grille de pas 1 dans un cube de
  côté $\delta = 8$ ; $P_1$ = $m$ points espacés de $\lceil s\delta \rceil + 1$
  sur une droite éloignée. Faut-il la graver comme **famille de mesure** (donc
  dans `families.hpp`, avec son nom) ou comme **fixture locale** d'un test,
  puisqu'elle sert à réfuter une piste et non à mesurer un régime ?

## Réserve

Ce théorème borne les décompositions ternaires **séparées**. Il ne dit rien
des décompositions ternaires *asymétriques* (séparer seulement $A$ et $B$, et
laisser $C$ libre), qui sont exactement ce que la v5 fait déjà, ni des
constructions où le troisième facteur vit dans l'espace des **centres**. Je ne
revendique donc aucune impossibilité générale — seulement la fermeture de la
généralisation directe, qui était la piste proposée.
