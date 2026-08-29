# Réponse de Claude — V141 : ce qui survit à la relecture adversariale, et deux de mes énoncés à corriger

- **Ancrage :** pin `bd35b88e`. Les cinq pistes ouvertes sur la route center-cover
  ont été soumises à une **réfutation adversariale indépendante** (une par piste,
  consigne : réfuter, pas confirmer, et conclure « réfuté » au moindre doute non
  levé). Voici le tri, avec ce que j'en corrige.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Ce qui survit

**Le lemme du rayon hors axe — non réfuté, et c'est le seul.** Attaqué sur cinq
angles, il tient :

- oracle **indépendant** écrit de zéro (rationnels exacts, circumcentre par
  système linéaire, aucun code de la sonde réutilisé) : $146\,030$ triangles
  strictement aigus d'arête maximale $ab$, boîtes ponctuelles **et** dilatées,
  $0$ violation ; l'oracle n'est pas vide, `rho2-1` lui en fait signaler $2\,932$ ;
- l'hypothèse du lemme est exactement ce que la lane impose
  (`src/lanes/q3.hpp` : $l_{ax}\leq D^2$ et $l_{bx}\leq D^2$ donnent $ab$ arête
  maximale, $\lVert 2x-a-b\rVert^2 > D^2$ donne l'acuité stricte en $x$) ;
- arrondi vérifié sur les trois résidus : $\lfloor(M_{\mathrm{off}}+2)/3\rfloor \geq M_{\mathrm{off}}/3$,
  donc $\rho_2 \geq \sqrt{M_{\mathrm{off}}/3}$, aucun off-by-one ;
- $w_i = \max(\lvert A_{hi}-B_{lo}\rvert, \lvert B_{hi}-A_{lo}\rvert)$ vaut
  **exactement** $\max\lvert d_i\rvert$, sans surmajoration ;
- l'inclusion serrée $\subseteq$ courante : $400\,000$ tirages entiers plus un
  balayage ciblé du point d'égalité $r=\sqrt{3}/2$, $0$ contre-exemple.

C'est le résultat le mieux étayé de la journée, et il est déjà dans la sonde
committée avec sa porte de mutant.

## Ce qui tombe

- **Attaquer le sur-masque : réfuté.** Raffiner le seul masque jusqu'à $K=32$
  ($512$ fois plus de tuiles, mêmes tests) ne tue que $2$ seeds sur $2\,437$. La
  sur-couverture n'est donc un artefact ni de la taille ni de la forme des
  tuiles. Cela conforte au passage le verdict **par seed**, qui contourne le
  masque au lieu de l'attaquer.
- **La porte `core` comme preuve de rentabilité : réfutée** — c'est la
  réfutation qui m'a fait refaire tout mon bilan en tests de sites. La porte
  reste une réduction de coût valide ($2{,}5$ fois), elle ne rend pas la route
  rentable.
- **Le pavage oblique : réfuté sur la force, pas sur l'existence.** Sûreté,
  arithmétique et composition tiennent ; mais les chiffres publiés venaient d'un
  repère orthogonal par produit vectoriel, dont la sonde de largeur déclare
  elle-même $160$–$173$ bits, donc infaisable en `i128`. Le repère **recommandé**
  (base du réseau plan, réduction de Gauss, $\det = \lVert e_0\rVert^2$) n'avait
  jamais été passé à la sonde de crédit ; le réfuteur l'a implémenté et **il
  tient**, à un facteur moindre : réduction des survivants
  $1{,}64\times \to \mathbf{1{,}46\times}$ sur `terrain` à $K=2$, et
  $1{,}94 \to 1{,}83$ sur `uniform`. La piste reste donc ouverte, à $1{,}46$ et
  non $1{,}66$.

## Deux corrections à mes propres énoncés

**1. Ma mesure du § V134 était un artefact.** J'écrivais que vos deux critères
$\min$ et $\max$ pour `NONE` « donnent exactement le même nombre de `NONE`
($88\,596$), donc $\mathrm{signe}(m_A)=\mathrm{signe}(m_B)$ partout ». C'est faux :
dans la sonde mesurée, la sortie anticipée de la branche $\min$ ne laissait
survivre que les cas où **les deux** minima courants restent positifs — elle
calculait donc exactement le critère $\max$. Les deux options étaient le **même
prédicat**, et la mesure était vide. Avec un $\min$ correctement implémenté :
$88\,486$ `NONE` et $22{,}6\,\%$ de visites en moins.

Ce qui reste vrai, et qui est mathématique : le majorant du prédicat
$L_A(z)>0 \lor L_B(z)>0$ sur $W$ est $\max(U_A,U_B)$. Votre $\min$ élague donc
plus que la preuve ne l'autorise ; il reste **sûr** (il ne peut que sous-compter
$g_{AB}$) mais il n'est pas justifié par cette preuve-là.

**2. Le coût de la version nœud était surestimé.** Avec le $\min$ correctement
implémenté, le rapport nœud/point tombe de $3{,}41$ à $2{,}52$ sur `terrain`
$K=4$. Elle reste plus chère que la version point ; ma conclusion tient, son
amplitude non.

## L'état, dans la bonne unité

| étape | rapport gain/coût, `terrain` |
|---|---:|
| ce matin ($K=8$, boîte lâche, verdict bloc) | 0,0055 |
| $+$ boîte serrée, porte `core`, verdict par seed, $K=2$, tri radial | **0,295** |
| $+$ pavage oblique, au facteur **réfuté-corrigé** $1{,}46$ | $\sim 0{,}43$ (projection) |

Il resterait un facteur $2{,}3$. Rien n'entre dans le chemin produit.

## Question

- **V141.** Le seul résultat qui ait survécu à une réfutation sérieuse est un
  **lemme géométrique**, pas un mécanisme. Il resserre le lieu des centres d'un
  facteur $3$ en volume et n'a jamais été appliqué là où il vaudrait le plus
  cher : dans `anchor_universal_kill`, c'est-à-dire au fuseau $W_3$ lui-même, qui
  est le seul point de la courbe où l'amortissement est maximal et le seul
  mécanisme q3 dont le taux de mort **croît** avec $n$ ($19{,}7 \to 32{,}0\,\%$).
  Est-ce là qu'il faut le porter plutôt que dans un pavage ?
