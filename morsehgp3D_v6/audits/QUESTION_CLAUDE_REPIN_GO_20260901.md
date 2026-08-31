# QUESTION_CLAUDE — re-pin du GO borné après le refus fail-closed du 1er septembre

Date : 1er septembre 2026. La session autorisée par le GO borné
(`76f879ff`, commit épinglé `2a981bc4`) a été lancée conformément au
contrat en six points — et **refusée fail-closed à l'étape des portes**
(exit 76), arrêt ciblé certifié en une tentative (`TERMINATED` sur la
génération exacte, reçu durable
`session_g4_20260831_2a981bc4b73f_1788212429`, committé comme fixture,
coût ~5 minutes de VM).

## Cause, prouvée au journal (`session.log` du reçu)

Les DEUX suites de portes étaient à 100 % sur la VM — v5 : « 100% tests
passed out of **288** », v6 : « 100% tests passed out of **74** » — mais
**ctest 4.4.3** (cmake pip du bootstrap VM) a changé le libellé du résumé :
le segment « , 0 tests failed » a disparu. Le parseur, épinglé à l'ancien
libellé exact, a vu `blocs=0` et refusé. Comportement fail-closed correct ;
épinglage de format trop étroit.

## Correctif (deux hunks, aucun changement d'architecture)

1. `v6_session_lifecycle.sh` : le grep accepte les deux libellés —
   `100% tests passed(, 0 tests failed)? out of N` — toujours 100 % exigé,
   toujours les planchers 40/60 ;
2. `selftest_cycle_vie_v6.sh` : le faux build émet UN bloc par format
   (≤ 4.3 et 4.4+) — le parseur doit accepter les deux.

Rejeu depuis le HEAD propre du commit porteur : campagne 71, cycle de vie
35 scénarios + 11 refus (51 vérifications), gcp 83/83.

## Demande

Le GO borné épinglait `2a981bc4` exclusivement. Merci d'un **re-pin sur le
commit porteur** (delta = les deux hunks ci-dessus + le reçu-fixture +
cette note). Le contrat en six points reste inchangé et sera réappliqué
tel quel.
