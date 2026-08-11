# Note de Claude — profil d'échelle de P1a et verdict de falsification

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le port CPU u16 de `P15-HOCUDA-P1a` **fonctionne et falsifie sa propre
route**. Le certificat géométrique est exact — quinze mutants meurent, le juge
déterminantal indépendant ne trouve aucun désaccord, le ledger transactionnel
ferme — mais ses compteurs de travail croissent trop vite. C'est précisément
la fonction d'un falsificateur de masse : il refuse tôt, avant toute session
G4 native.

Ce verdict porte sur l'ordonnance mesurée, pas sur le théorème de
center-cover, ni sur une future implémentation tuilée ou device. Le statut
logiciel appartient au verdict live.

## Mesure

Famille `terrain`, graine `20260811`, `leaf_size=8`, `microtile=64`, un seul
binaire, mono-thread. L'exposant est
`log2(travail_2n / travail_n) / log2(2)` entre tailles consécutives.

| compteur | 2 000 | 4 000 | 8 000 | exposants |
| --- | ---: | ---: | ---: | --- |
| blocs tentés | 24 473 | 66 934 | 167 147 | 1,45 puis 1,32 |
| blocs prunés | 4 000 | 12 173 | 33 082 | 1,61 puis 1,44 |
| splits | 20 473 | 54 761 | 134 065 | 1,42 puis 1,29 |
| microtuiles | 17 024 | 43 749 | 103 265 | 1,36 puis 1,24 |
| visites témoin--patch | 11 342 326 | 48 755 505 | 181 460 408 | 2,10 puis 1,90 |
| évaluations de coins | 1 499 943 648 | 5 880 386 017 | 20 267 313 188 | 1,97 puis 1,79 |
| tests ponctuels | 371 871 550 | 1 454 747 634 | 5 017 937 282 | 1,97 puis 1,79 |
| part terminale | 32,0 % | 20,2 % | 12,2 % | — |

## Lecture

Trois faits séparés, à ne pas confondre :

1. **La condition no-go « majorité de la masse terminale » n'est PAS
   déclenchée.** La part terminale décroît de 32,0 % à 12,2 % : le
   center-cover prune bien la majorité de la masse, et de plus en plus. Le
   certificat fait ce qu'il promet.
2. **Les compteurs de travail sont rouges.** Visites patch--nœud, évaluations
   de coins et tests ponctuels ont deux exposants successifs très supérieurs à
   `1,35` — approximativement quadratiques à cubiques. Vingt milliards
   d'évaluations de coins à 8 000 points interdisent toute extrapolation à
   50 000.
3. **Le coût est concentré dans les coins.** Chaque bloc tenté évalue jusqu'à
   `64 patchs x 8 coins x 2 côtés` avant même de chercher un témoin, soit
   1 024 évaluations de `clip` par bloc dans le pire cas. Le rapport
   `coins / blocs tentés` vaut environ 61 000 à 2 000 points et 121 000 à
   8 000 : ce n'est pas la partition qui explose, c'est le travail par bloc.

## Conséquence pour la session G4 native

La note de solution prévoit une session G4 sanctionnée qui construit le cubin
`sm_120`, ferme la parité fake/native puis exécute les deux profils 50 k. Ces
mesures la rendent prématurée : porter sur CUDA une ordonnance dont le travail
croît en `n^1,8` à `n^2,1` ne ferait que déplacer le mur. Aucune session G4
n'est demandée pour P1a en l'état.

## Directions, sans engagement

- réduire le travail par bloc avant les patchs : une borne unique de bloc qui
  élimine les 64 patchs d'un coup lorsque le domaine `T0` est déjà couvert,
  ou un ordre de patchs qui s'arrête au premier survivant sans témoin ;
- réutiliser la traversée duale `Q--W` de la lane q2, dont l'héritage exact
  des deux verdicts supprime le rescan : le même schéma s'applique aux
  témoins de patch, où seul le majorant change ;
- mesurer d'abord `coins / blocs tentés` à budget constant, puisque c'est ce
  rapport, et non la partition, qui décide.

Ces directions ne sont ni reçues ni promises. Le probe reste utile tel quel :
il est le falsificateur qui a produit ce refus.

GCP non utilisé pour cette note.
