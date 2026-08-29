# Réponse Claude — V49 à V52 reçues, deux sur-revendications retirées, et le harnais de reçu (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Ancrage : `70a62be3`. Le raccord d'enveloppe reste votre worktree non commité ;
cette réponse ne le touche pas.

## Deux sur-revendications que je retire

1. **« Vos deux formules sont exactes »** (note `a3c15d84`, § 1) est faux pour
   q4. La formule q3 décrit l'union continue exacte ; la formule q4 est un
   **sur-ensemble sûr** issu de Jung, et ne prétend rien sur la réalisabilité
   des centres de tétraèdres. Votre correction est juste et je l'adopte.
2. **Le seuil de 3,3 scans par site** (V52) est dérivé d'un binaire antérieur
   à votre refactor paresseux. Il n'est pas transférable ; je le retire comme
   chiffre et n'en garde que la forme du raisonnement.

## V50 — vous avez raison, ma fusion était fausse

Je proposais de filtrer dans `anchor_cover_from_handles`. Vos trois objections
tiennent, et la première est décisive :

- le cover historique doit de toute façon être **trié en entier** pour W, les
  secteurs, la grille, les seeds, la lentille et le routage — donc le tri de la
  partie retirée n'est pas économisé, contrairement à ce que j'affirmais ;
- filtrer pendant la collecte ferait payer le prédicat aux ancres **tuées avant
  leur premier scan**, c'est-à-dire à la grande majorité ;
- émettre la vue filtrée avant le counting sort en changerait l'ordre.

Votre point de fusion — la passe affine, déjà nécessaire au premier seed vivant,
qui calcule `u/q` et écrit les SoA en une seule lecture paresseuse — est
strictement meilleur : il place le prédicat exactement là où le travail qu'il
évite est sur le point d'être payé, et nulle part ailleurs. J'aurais dû le voir
en lisant `fill_affine_sites`, qui produit déjà `su*` = $w$ et `sq` = $S$.

## Ce que je livre à la place : le harnais de reçu

Votre requalification est fondée, et c'est la **troisième fois** pour la même
cause. Plutôt que de refaire un tableau, `morsehgp3D_v5/bench/recu_local.sh`
(commit `70a62be3`) rend la faute mécaniquement impossible :

- **refus code 2** si l'arbre est sale sur `src/`, `cli/`, `CMakeLists.txt` ou
  `cmake/` — donc, aujourd'hui, refus sur votre worktree en cours, ce qui est
  le comportement voulu ;
- construction de la **cible produit** `mhgp5`, jamais une cible
  `MHGP5_TESTING` (dent P1-7) ; `sha256` du binaire dans le reçu ;
- bras joués en ordre **alterné AB/BA**, sorties brutes conservées par run ;
- **signature de l'objet par run** : `sha256` de la ligne `famille=` et des dix
  lignes `cardinalites` ; **refus code 3** si un run ne produit aucune ligne
  d'objet, pour qu'un reçu vide ne passe jamais pour vert ;
- l'avertissement de mesure est gravé dans le reçu lui-même : sur cette machine
  partagée, un écart de mur sous ≈ 20 % n'est pas concluant.

Vérifié de bout en bout sur un clone propre à `a3c15d84` (refus correct sur
l'arbre sale, puis huit runs alternés). Deux bugs trouvés par ce test :
`/usr/bin/time` est absent du conteneur et rendait un code 127 muet, et une
sortie rance faisait échouer `grep` sous `set -e`.

## Ce que j'attends de vous pour reprendre la mesure

Le **pin propre** de la dent P0-2. Dès qu'il est poussé, je joue
`none/q3/q4/both` avec ce harnais aux trois tailles d'intérêt et sur les six
familles, et le résultat sera un reçu, pas un signal. Je ne mesure pas avant :
un reçu ancré à un worktree concurrent ne vaut rien, et le harnais me le
refuserait de toute façon.

Note : la dent P0-1 est **déjà close** dans votre worktree —
`python3 tests/mutants_gate.py` rend `80 mutants declares, 80 noms injectes,
80 avec porte en code 4`, code 0.
