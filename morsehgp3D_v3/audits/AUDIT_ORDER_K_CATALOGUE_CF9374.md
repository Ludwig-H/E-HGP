# Audit du catalogue `order_k` — snapshot `cf9374` / `927809`

Date : 9 août 2026 UTC.

Phase revendiquée par le développement : M3. Backend : CPU exact. Profil : grille u16. Mode audité : oracle expérimental. La porte produit M3 n'est pas satisfaite.

> [!CAUTION]
> **P0 catalogue : le nouveau sujet `order_k` est incomplet sur une entrée `RelevantGP`, omet toutes les arités basses pour `n < 4` et laisse passer silencieusement une violation extra-shell utile.** Son intégration dans l'oracle n'est accompagnée d'aucun CTest, et ses reçus l'identifient encore comme `anchored_catalogue`.

## 1. Snapshot et delta

- HEAD : `7fa39b1d8c9d3b566bcd098bb4bdd2dbc107d7af`;
- `prototype/order_k_bfs.hpp` : SHA-256 `cf9374b64fdc6428625a1e8f72ecb6e19e6d66a80d3249361c694ea064c6d256`;
- `oracle/oracle_main.cpp` : SHA-256 `927809a35e0356a29e81dc6ed23ee9363655a4b3e4af2d12974edb8fe3ce6078`;
- `CMakeLists.txt` : SHA-256 `384b940d52b883a98f06657389bc7da8ec5474dcdef4519d7c80e3aa733e0874`.

Le header ajoute `order_k_catalogue` : singletons directs, parcours des sommets jusqu'à `s_max + 2`, récolte des sous-paires et sous-triangles, filtre de bon centrage et census fermé en $O(n)$ par candidat. L'oracle accepte désormais `--subject order_k` et reconstruit ses forêts. CMake reste inchangé : aucune fixture ou campagne `order_k` n'est une porte permanente.

## 2. P0 : le catalogue final reste faux sur la fixture coplanaire `RelevantGP`

Fixture u16 :

```text
0=(4,1,0)
1=(14,19,0)
2=(4,17,0)
3=(17,9,0)
4=(15,8,19)
s_max=4
```

Une énumération indépendante en rationnels `Fraction`, avec résolution de Gram par Gauss, confirme zéro extra-shell parmi tous les supports affinement indépendants et bien centrés d'arités un à quatre. La fixture satisfait donc `RelevantGP` a fortiori.

Résultat exact :

```text
référence exhaustive : 22 sphères
sujet order_k        : 18 sphères

supports manquants :
  {2,3}       arité 2, rang 3
  {0,2,3}     arité 3, rang 3
  {2,3,4}     arité 3, rang 3
  {1,2,3,4}   arité 4, rang 4
```

Le défaut hérité est celui documenté dans [`AUDIT_ORDER_K_BFS_A8111F0.md`](AUDIT_ORDER_K_BFS_A8111F0.md) : les témoins coplanaires au triangle sont ignorés alors que leur contribution au pinceau est constante et peut être intérieure. Le parcours visite les mauvais sommets au niveau 0 et manque les vrais sommets shallow.

Le nouveau census terminal filtre bien le faux support `{0,1,2,4}` : le parcours lui attribue le niveau 0, mais son rang fermé recompté vaut 5. Cette protection empêche une fausse émission; elle ne répare ni la navigation, ni la récolte. Les quatre supports utiles ci-dessus restent absents du catalogue final.

## 3. P0 domaine : le diagnostic extra-shell est inatteignable

`closed_ball_members` renseigne `extra_on_shell`, puis retourne `on_shell == arity`. Dès qu'un point extérieur au support est sur la coquille, `on_shell > arity`; `emit` reçoit donc `false` et retourne immédiatement avant son bloc `if (extra)`. Les compteurs `degenerate_shells` et `out_of_domain` ne peuvent pas enregistrer ce cas.

Fixture minimale en dimension affine trois :

```text
0=(0,0,0)
1=(2,0,0)
2=(1,1,0)
3=(0,3,4)
4=(5,2,3)
s_max=2
```

La miniboule de la paire `{0,1}` a pour centre `(1,0,0)` et rayon carré 1. Le point `2` est sur sa coquille et aucun point n'est strictement intérieur : c'est une violation `RelevantGP` utile. Le probe courant rend pourtant :

```text
out_of_domain=0
degenerate_shells=0
support {0,1} omis silencieusement
vertices_visited=3
```

L'appel direct confirme `closed_ball_members=false`, `extra_on_shell=true` et `on_shell=3`. Une campagne oracle indépendante sur le worktree reproduit le même défaut de fermeture :

```text
--subject order_k --clouds 20 --seed 4242
--min-points 5 --max-points 8 --max-order 4 --coord-max 50

trial 11 : sujet=dans, référence=hors
attempted=20 decided=19 rejected_domain=0
exit_code=1
```

## 4. P0 : arités deux et trois absentes pour les tailles de base

Le commentaire prétend récolter les arités un à quatre, mais `order_k_vertices` retourne immédiatement vide pour `n < 4`. Aucun chemin séparé n'énumère alors les paires ou triangles.

```text
n=2, points [(0,0,0),(2,0,0)], s_max=2
attendu : 2 singletons + 1 paire
obtenu : 2 singletons, 0 paire

n=3, triangle aigu [(0,0,0),(4,0,0),(1,3,0)], s_max=3
attendu : 3 singletons + 3 paires + 1 triangle
obtenu : 3 singletons, 0 paire, 0 triangle
```

Le CLI de l'oracle rejette actuellement ces tailles comme campagne absurde. L'intégration ne peut donc même pas tester ses deux cas de base publics sans une fixture ou un harness distinct.

## 5. Traçabilité et coût encore ouverts

- Le diagnostic de valeur invalide continue d'annoncer seulement `v2`, `anchored` et `edge_shallow`, alors que `order_k` est accepté.
- `order_k_total` est additionné mais aucun compteur `order_k` n'est affiché ou sérialisé; les branches de navigation et de récolte sont invisibles dans le reçu.
- Le champ sujet du reçu classe `order_k` dans le dernier bras du ternaire, donc sous le libellé `mhgp3v anchored_catalogue`.
- Le reçu contient à la place le bloc `edge_shallow` nul, sans schéma `order_k`.
- Aucun CTest ne verrouille le signe `InSphere`, la fixture coplanaire, l'extra-shell, les tailles deux et trois ou un accord différentiel.
- Le coût historique $8(n-4)V$ demeure. La récolte ajoute un census complet en $O(n)$ pour chaque singleton ou sous-support candidat; elle aggrave donc le facteur entrée fois sortie au lieu de fournir l'index de voisinage sous-linéaire requis.

La dérive de reçu est reproduite dynamiquement par un nuage générique vert de cinq points : le processus annonce `sujet=order_k`, mais le JSON écrit simultanément `subject="mhgp3v anchored_catalogue"`, `status="qualified"` et un unique bloc `edge_shallow` dont tous les compteurs valent zéro. Une campagne expérimentale `order_k` ne doit pas pouvoir recevoir ce statut ni emprunter l'identité d'un autre sujet.

La justification « toute arête de rang `r` atteint un tétraèdre en deux pas de rang `r+2` » n'est pas l'énoncé exact. [`AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md) démontre depuis que le plafond uniforme `s_max + 2` est conditionnellement suffisant dans le vrai squelette d'un arrangement simple, essentiel et de dimension affine trois. Ce théorème ne couvre ni les tailles deux et trois, ni les offsets coplanaires, ni les multiplicités autorisées par `RelevantGP`; il ne répare donc aucun des écarts dynamiques ci-dessus.

## 6. Verdict et portes

Le delta rend le prototype comparable par l'oracle sur certains nuages génériques, mais il ne ferme ni le catalogue, ni le domaine, ni la traçabilité. Son statut reste **oracle expérimental NO-GO produit**.

Avant nouvelle promotion :

1. diagnostiquer l'extra-shell avant tout retour fondé sur `on_shell == arity`;
2. rendre les catalogues `n=1`, `n=2` et `n=3` exacts;
3. corriger les témoins coplanaires constants et figer les quatre supports manquants;
4. ajouter des CTests indépendants et des reçus `order_k` complets;
5. remplacer les rescans en $O(n)$ par une primitive dont temps et mémoire respectent la cible 50 k.

GCP non utilisé.
