# Note de Claude — état de session et condition d'entrée en G4

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note déclare ce qui a été fait, ce qui a été **réfuté**, et pourquoi
aucune session G4 n'a été ouverte. Elle ne prononce aucune admission ; le
verdict live reste seul autorité.

## 1. Ce qui a été construit

| lane | fichier | portes | état déclaré |
| --- | --- | ---: | --- |
| disposition | `prototype/morton_lbvh.hpp` | — | en-tête partagé par toutes les lanes |
| `k=1` | `prototype/emst_boruvka{.hpp,_probe.cpp}` | 15 | oracle borné nommé, pas une architecture |
| q2 | `prototype/yao48_source.hpp`, `pair_yao48_source.cpp` | 37 | count-only, juge borné, aucune admission |
| q4 P1a | `prototype/center_cover_mass_probe.cpp` | 34 | falsificateur de masse — **il a réfuté sa propre route** |
| degré Gabriel | `prototype/gabriel_degree_gate.cpp` | 5 | régression de politique |
| diagnostic | `prototype/warm_e2e_h0_diagnostic.cpp` | 3 | `DiagnosticHorizontalReceipt-v1`, `partial_h0_wall` |

## 2. Ce que la session a réfuté

- **P1a q4 est NO-GO sur l'ordonnance mesurée.** États témoin, évaluations de
  coins et tests ponctuels croissent en `n^{1,8}` à `n^{2,1}`. La condition
  « majorité de la masse terminale » n'est en revanche pas déclenchée : la
  part terminale décroît de 32,0 % à 12,2 %. Détail et reçus dans
  [`NOTE_CLAUDE_P1A_NOGO_20260811.md`](NOTE_CLAUDE_P1A_NOGO_20260811.md).
- **L'ordonnance q2 par chambres est NO-GO**, comme l'audit du reçu `2e49dcf`
  l'avait établi.
- **Deux politiques de travail ont été mesurées puis refusées** : l'ordre
  best-first par majorant (173 millions de visites contre 122, trois fois le
  temps) et le plafond de frontière (visites divisées par 1,7 mais survivantes
  multipliées par huit).

## 3. Ce qui a changé de régime

La traversée duale `Q--W`
([`NOTE_CLAUDE_TRAVERSEE_DUALE_Q2_20260811.md`](NOTE_CLAUDE_TRAVERSEE_DUALE_Q2_20260811.md))
fait passer la gate `1,35` à tous les compteurs de sortie et de
classification, sur les trois familles structurées. À `12 500` terrain, même
binaire et census identique (`253 129`), les survivantes tombent de
`4 543 219` à `674 986`.

Un compteur reste **rouge** : les états de la frontière ambiguë,
`1,50` puis `1,92` sur `terrain`. C'est du travail de recherche, pas de la
sortie ; la gate n'est donc pas entièrement verte.

## 4. Pourquoi aucune G4 n'a été ouverte

[`AUDIT_REPONSES_G4_Q2_YAO1_20260811.md`](AUDIT_REPONSES_G4_Q2_YAO1_20260811.md)
fixe la règle : réserver la prochaine session à une architecture dont la gate
de travail locale est verte, ou au protocole P1a natif complet. Les deux
conditions manquent : le compteur de recherche q2 reste rouge et P1a est
réfuté avant CUDA.

Le chiffrage renforce ce refus plutôt qu'il ne l'atténue. Le run `50 000`
terrain en mode dual prend `116,994 s` de phase locale mono-thread. Même en
supposant, sans le démontrer, un partage parfait sur 48 vCPU, la seule lane q2
resterait de l'ordre de deux à trois secondes, sans `k=1`, sans q3/q4, sans
census matérialisé, sans resolver, sans fold ni payload. Une session G4
mesurerait donc, à grands frais, une seconde qu'elle ne peut pas atteindre.

Le protocole reste prêt : quotas vérifiés disponibles, scripts gardés, double
coupe-circuit, arrêt ciblé certifié. Il attend une architecture qui le mérite.

## 5. Suite proposée, sans engagement

1. faire baisser les états de frontière q2 — c'est le seul verrou restant de
   cette lane, et il est isolé ;
2. reprendre P1a par le rapport `coins / blocs tentés`, qui est la cause
   mesurée, avant toute autre optimisation ;
3. n'ouvrir la G4 qu'ensuite, ou pour la session native P1a lorsqu'elle aura
   un sens.

GCP non utilisé pour cette session.
