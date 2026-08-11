# Audit épinglé — source directe `24ad3d37` à `e406e1f`

Dates des snapshots : 9 et 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> [!NOTE]
> Ce fichier est un reçu immuable des snapshots nommés. Il ne décrit ni le
> `HEAD` ni le worktree; leur seul verdict est
> [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## Contre-exemples conservés

Le prototype `direct_source.cpp` cite ce fichier pour quatre défauts qui ont
imposé des invariants permanents :

1. `--judge 0` annonçait une égalité sans exécuter le juge;
2. une map indexée seulement par coquille écrasait des émissions doubles;
   le mutant bidirectionnel produisait 126 émissions au lieu de 56;
3. le payload comparé conservait la taille de `members`, pas leurs
   identifiants ordonnés;
4. absence mathématique et refus de ressource partageaient un statut.

Une source n'est reçue que si mode, domaine, multiplicité et payload complet
sont comparés. Un accord de masses ou une phrase imprimée ne suffit pas.

## Invariants permanents

- Les modes mesure, juge et cover sont exclusifs et explicites.
- Le ledger compare une émission canonique par objet, pas seulement une somme.
- Le payload inclut membres, offsets, sources de forêt et statuts de refus.
- Un chronomètre n'inclut pas silencieusement un juge d'un seul côté.
- Count-only ne qualifie ni complétude, ni fill, ni temps bout en bout.

Cette famille reste un oracle CPU borné, jamais une architecture 50 k.

GCP non utilisé.
