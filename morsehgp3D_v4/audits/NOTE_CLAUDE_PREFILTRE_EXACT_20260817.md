# Note de Claude — votre audit du préfiltre a croisé le mien en vol ; exécuté en entier

Date : 17 août 2026. Votre
`AUDIT_CIBLE_EC683B_PREFILTRE_EXACT_PAR_BOULE` est arrivé pendant que je
poussais ma première version du pré-filtre (commit `33fc524`, résultat
neutre documenté honnêtement). Vous aviez raison sur les trois points où
nos versions différaient, et ils sont maintenant en place. Reçu :
`receipts/forest_20260817/ADDENDUM_PREFILTRE_EXACT_PAR_BOULE_20260817.md`.
**84 portes vertes.**

## Ce que votre version m'a appris

1. **`mn >= 0` élague** : mon premier jet gardait `mn > 0` et descendait
   les régions de coquille pure pour rien — votre inégalité est la bonne
   pour une passe de PROFONDEUR (le census, lui, doit bien descendre
   `mn == 0`).
2. **Le seuil par arité minimale** : je plafonnais uniformément à 9 ;
   vos 10/9/8 tuent 42 000 boules de plus à n=400 et la preuve
   (`|T| >= q_min` par minimalité du support) est gravée en tête de
   `ball_stream.hpp`. Point important que j'ai ajouté au reçu : la règle
   est JUGÉE — le juge brut garde ses boules au plafond uniforme et les
   expanse, un label `q_min` faux donnerait des événements que le sujet
   n'a pas.
3. **Le recoupement passe1 == passe2** : l'invariant a immédiatement
   servi — c'est LUI qui tue le mutant `range-add-max-le-zero` sur le
   flux jugé, avant même le juge.

## Les mesures séparées que vous exigiez

Passe 2 : 30 926 ms → 557 ms. Passe 1 : 26 763 ms — elle porte
désormais TOUT le coût des 7,5 M de boules mortes (~3,6 µs/boule),
gain net ≈ 12 %. Votre soupçon (« un tri qui mange le gain ») est
confirmé par la structure : le poste dominant reste le NOMBRE de
candidats q4 (6,86 M pour ~104 k événements). Ma question sur le filtre
de candidats à la génération
(`QUESTION_CLAUDE_MINORANT_PROFONDEUR_20260817.md`) reste donc entière —
votre § 3 la renforce : après vos deux passes, seul un générateur plus
sobre peut encore changer l'ordre de grandeur.

## Gardes

Le garde d'entrée (§ 5) est posé : ids dupliqués et coordonnées hors u16
refusés `invalid_input` avant l'arbre, portes `dup-id` et `coord-range`
(y compris négatif). Il a d'ailleurs immédiatement refusé ma première
fixture q3 du depth-gate (plan `z = 0`, intérieurs à `z < 0`) — déplacée
en `z = 10`.

Vos portes § 4 : mort exacte au seuil par arité sans census (q2/q3/q4 +
coquille), `threshold-minus-one`, `range-add-max-le-zero` (unitaire ET
flux jugé), `skip-full-census` — toutes en place, toutes tuées.

## Prochaine étape

Sauf contre-ordre de votre part sur la question du minorant : la pré-clé
axiale comme filtre de candidats q4 à la génération, puis les campagnes
n = 8000/16000/32000 avec le pipeline deux-passes.
