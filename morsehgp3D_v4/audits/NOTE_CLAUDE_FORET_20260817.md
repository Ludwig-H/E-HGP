# Note de Claude — le noyau de forêt est debout, jugé, macro-lots compris

Date : 17 août 2026. Votre ordre publié étant épuisé (sélection axiale
reçue, avec sa mesure honnête — voir `NOTE_CLAUDE_SELECTION_AXIALE`), j'ai
ouvert le dernier grand morceau : la forêt. Reçu complet :
`receipts/forest_20260817/README.md` ; dossier : § 5 réécrit.

## Trois choses que vous voudrez peut-être contester

1. **L'invariant des rayons de naissance remplace la piste v3.** « Les
   facettes nées au niveau même n'apportent aucune fusion » est en fait un
   théorème trivial : une facette `σ∖{z}` (z intérieur) naît AU niveau
   `ρ(σ)` ; si elle était facette d'un simplexe STRICTEMENT antérieur, son
   rayon de naissance serait < `ρ(σ)` — contradiction. J'unionne donc les
   `K+1` facettes (conforme aux cliques de la Déf. 29) et je MESURE
   `attach_violations = 0` (un bras non actif né dans un lot antérieur) —
   c'est un détecteur de flux incohérent, pas une hypothèse.
2. **Les macro-lots sont massifs** : 365 nœuds à >= 3 composantes
   absorbées sur 487 événements (petits nuages entiers). Votre contrat
   (`same_exact_level`, jamais la représentation, jamais de chronologie
   binaire) est gravé et ses DEUX mutants meurent — dont `repr-ties` sur
   une fixture qui met le tétraèdre `R² = 14900` (U192 non réduit) et une
   arête `14900/1` (canonique) dans le même lot K=3.
3. **Le juge valide plus que la forêt** : énumération de tous les
   sous-ensembles, miniboule par recherche de support propre (OBig),
   cliques complètes du manuscrit — il re-démontre la bijection
   événement-boule et la complétude JOINTE des trois lanes sur petits n
   (régime oracle T2, `n <= 14`). Le refus transactionnel est vérifié
   cohérent des deux côtés (point sur la sphère hors support ⟹ σ écarté).

## Prochaines étapes proposées, dans l'ordre

(a) **Raccord du flux réel** : refactoriser les pipelines des trois probes
en bibliothèque pour que la forêt consomme les événements WSPD (les
probes restent les portes ; la bibliothèque devient le chemin) ;
(b) **rendu § 9.1** : `F_K^render`, poids `S_τ`, `T_x`, `m_τ`, avec la
question Q4 (facettes actives seulement — ma proposition) à trancher ;
(c) l'équivalence **Théorème 2** comme second chemin de vérité (composantes
de `L_K(r)` + couverture) sur petits n ;
(d) puis l'échelle : n = 8000/16000/32000 sur les trois lanes + forêt,
avec les compteurs qui décideront des prochaines optimisations.

Je pars sur (a) sauf avis contraire. 64 portes CTest vertes, tout est
poussé sur main.
