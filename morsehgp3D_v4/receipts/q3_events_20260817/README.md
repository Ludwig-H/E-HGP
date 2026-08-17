# Reçu — instruction q3 : ancres survivantes → événements

Date : 17 août 2026 UTC. CPU 4 cœurs single-thread, non contractuel.

Juge PAR IDENTITÉS contre l'oracle brut (tous les C(n,3) triangles) :

| famille | n | événements | ev/point | shell refusés | manquants | en trop |
|---|---:|---:|---:|---:|---:|---:|
| uniform | 400 | 48 965 | 122,4 | 15 | **0** | **0** |
| eight_clusters | 400 (porte) | — | — | — | **0** | **0** |

Échelle (sans juge) :

| famille | n | ancres vues | porteurs testés | événements | ev/point | t_instruction |
|---|---:|---:|---:|---:|---:|---:|
| uniform | 2000 | 232 006 | 7 307 995 | 313 537 | 156,8 | 35,7 s |
| eight_clusters | 2000 | 479 929 | 115 512 175 | 249 093 | 124,5 | **474,8 s** |

Constat : l'exactitude est acquise (0/0 au juge, refus transactionnel des
coquilles), mais le coût d'instruction est dominé par les ancres
inter-amas à grande lentille — 240 porteurs testés par ancre en moyenne sur
eight_clusters, chacun payant une descente de profondeur. C'est le poste
« census q3 » déclaré non résolu par la v3, reproduit et chiffré sur la v4.
Limite déclarée : le juge partage le prédicat de profondeur du sujet (il
valide énumération, owner, complétude — pas l'arithmétique ; l'oracle à
arithmétique indépendante viendra avec oracle/).

## Addendum — filtre h_a/h_b branché avant l'instruction (consensus des audits)

Mêmes nuages, mêmes événements (exactitude préservée, 0 doublon) :

| famille | n | ancres tuées par h_a/h_b | porteurs testés | t_instruction |
|---|---:|---:|---:|---:|
| eight_clusters | 2000 | 81 390 / 479 929 (17,0 %) | 77,5 M (−32,9 %) | **253,3 s (−46,7 %)** |
| uniform | 2000 | 7 741 / 232 006 (3,3 %) | 6,7 M (−8,3 %) | 26,0 s (−27,3 %) |

Confirmation de la prédiction d'audit : les ancres tuées sont
préférentiellement les LONGUES (17 % des ancres portent 47 % du coût sur la
famille adversariale). Le poste restant est le census partagé par ancre
(plan médiateur, réponse Q9) — prochaine primitive.
