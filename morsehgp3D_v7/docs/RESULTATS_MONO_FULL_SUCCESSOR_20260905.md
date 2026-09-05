# FULL mono : normalisation des successeurs v2

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Périmètre et admission

La [normalisation v2](CONTRAT_NORMALISATION_FULL.md) supprime une lecture
et une écriture redondantes sur chaque chemin non trivial, sans changer
le tableau final ni les forêts après succès. Les plafonds numériques restent inchangés ;
le calendrier de charge change explicitement. Aucune proposition MEB,
réduction q4 ou modification de cache n'est intégrée dans ce delta.

Les vingt portes ciblées passent dans deux builds frais Release et
ASan/UBSan, LeakSanitizer actif. La sonde v3 est compilée sans macro de
test, en C++20/O3/NDEBUG, avec les quarante dépendances MMD utilisateur.
Le binaire `8ff0dd10…` est lié au header `85c27ab9…` ; les dépendances
système ne rendent pas ce build hermétique. L'ancienne sonde implicite
est effectivement rejetée à la compilation, code 1 et motif nommé,
sans produire d'ELF. Les anciens binaires et reçus restent inchangés.

L'admission `c4bbd2cf…` ferme 24 micros n=8 : Kmax=5/10, s=8/10/12,
eager et lazy C=0/1/1M. Elle contient 156 ordres horizontaux, onze rejets
d'arguments attendus et les auto-tests des deux lecteurs, 35 et 27 mutants,
en Python normal et `-O`. Deux modèles positifs et dix mutants contrôlent
aussi les raccords de métadonnées. Les coûts des micros ne sont pas des
mesures de performance.

## Protocole et incident de coordination

Nuages uniformes seed3/u16, un thread CPU6, Kmax=10, lazy first-C=1M.
La série prévue est 8k/s8, 8k/s10, 8k/s12, 16k/s8, 32k/s8. Chaque
moteur garde 600 s, dix secondes de grâce, 26 GiB d'espace d'adressage,
le proxy de payload 8 GiB et 128 millions d'accès aux successeurs par
ordre. Aucun plafond n'est augmenté pour obtenir une sortie complète.

La clôture indépendante de 21:51:59 UTC autorisait les chronométrages.
L'auditeur a ensuite signalé quatre petits rejeux CPU0 entre
21:54:33.912911 et 21:54:34.770106 UTC, après cette clôture : fenêtre
de 0,86 s, moteurs cumulés 0,56 s. Elle recouvre le premier passage
8k/s8. **Son temps de 142,456 s est exclu de la comparaison de performance.**
Ses octets et ses résultats fonctionnels restent conservés ; aucune
soustraction de temps ni correction estimée n'est faite. Les passages
suivants commencent après cet incident. Le rejeu distinct est clos.

## Cinq captures closes, pas cinq succès

Le [paquet publié](../receipts/full_gabriel_successor_mono_20260905/README.md)
conserve 1 162 fichiers, sommes `910d87e0…`, sans ELF. Le reçu `925219da…`
ferme quatre succès et un refus, sans timeout ni changement de sources.
`status=completed` signifie ici capture close ; `all_successful=false`
conserve l'échec à 32k. Le rejeu 8k/s8 est une sixième capture distincte,
reçu `8f3a1cd4…`, pas une réécriture de cette série.

| n | s | Ordres réussis | Temps au terminal, s | FULL seul, s | Pic GNU time, KiB |
| --- | --- | --- | ---: | ---: | ---: |
| 8 000, original | 8 | 1..10 | 142,456 — exclu | 64,152 — exclu | 1 321 072 |
| 8 000, rejeu | 8 | 1..10 | 138,221 | 61,946 | 1 321 272 |
| 8 000 | 10 | 1..10 | 143,301 | 64,117 | 1 321 532 |
| 8 000 | 12 | 1..10 | 145,404 | 64,296 | 1 321 116 |
| 16 000 | 8 | 1..10 | 321,643 | 145,014 | 2 712 144 |
| 32 000 | 8 | 1..8 ; refus K9 | 567,439 | 202,009 | 5 130 396 |

Le temps inclut la génération, les lectures, digests, sorties provisoires
et libérations avant le terminal. Le pic est celui du processus entier,
pas une taille d'arène ni un échantillon pris après destruction. Une
observation par réglage sur cet hôte partagé ne qualifie ni p95, ni
meilleur s, ni accélération robuste. Aucun contrat intégré n'est acquis.

Le rejeu s8 commence à 22:17:41.921080 et termine à 22:20:00.210394 UTC,
après la clôture de tous les autres moteurs. Aucun nouveau chevauchement
de l'auditeur n'est signalé ; aucune isolation matérielle de cet hôte
partagé n'est certifiée. Le lecteur compare les configurations, sources,
dix ordres et terminal v3-v3 sur tous les champs hors temps/RSS déclarés,
**sans transformer le compteur de successeurs**. Ces comparaisons passent ;
leur portée est distincte du différentiel historique de calendriers.

À 32k, le nouveau motif est `full_gabriel_meb_call_budget` : exactement
4 000 000 appels MEB, 553 128 490 supports essayés et 125 373 952 accès
aux successeurs. L'ancienne limite de successeurs n'est plus le premier
refus, mais un autre plafond arrête toujours K9. Ni K9 ni K10 ne sont
livrés ; le digest global reste vide. Le temps au refus n'est pas celui
d'une exécution complète et ne forme pas un ratio de gain avec l'ancien
refus, qui interrompait une autre quantité de travail.

Les trois réglages 8k donnent le digest `e6e3fa51…` pour l'entrée
`b7374475…`. À K10, le compteur de successeurs vaut 33 607 807,
contre 38 240 799 historiquement, pour 2 316 496 ancres normalisées :
12,12 % d'accès en moins. À 16k/K10, il vaut 75 223 906, contre
85 034 894 historiquement, pour 4 905 494 ancres. Ce sont des comptes
de travail, pas des gains de temps. La génération prend encore 55,114 s
dans le rejeu 8k/s8 et 127,125 s à 16k ; FULL prend respectivement
61,946 et 145,014 s. Aucun gain général n'est établi par cette campagne.

## Comparaisons fonctionnelles closes

Les chronométrages sont NEW-only. Le bras fonctionnel historique 8k est
le singleton `21b77d29…` ; les bras 16k/32k sont lazy `13c6cc72…`.
Les anciennes captures ne forment pas
une nouvelle campagne de temps appariés. Le reçu de comparaison `5ee62561…`
ferme 29 cas, normalement et sous `-O` : 24 micros/156 ordres, trois
8k/30 ordres, 16k/10 ordres, puis le seul préfixe 32k/8 ordres. Les
204 ordres comparés conservent digests, champs hors mesures et les
24 compteurs sérialisés autres que `successor_steps`. L'identité
`S_v2=S_v1−2A` passe sur chacun de ces ordres réussis. Les huit champs géométriques
du résultat C++ non sérialisés ne peuvent pas être comparés depuis ces
bruts. L'identité de travail ne s'applique qu'aux ordres réussis des
deux côtés, jamais au dernier ordre refusé. Les deux 32k refusent à K9,
à des étapes différentes : seule leur partie réussie K1..8 est comparée.
Les deux refus globaux sont préservés ; la comparaison valide de leur
préfixe ne qualifie pas le dernier ordre ni une tour entière.

## Contrats encore ouverts

Ces forêts horizontales sont lues puis détruites par ordre, pas retenues
ensemble ni archivées. Ni verticale FULL, ni supplément pondéré, ni
export de tour intégré ne sont qualifiés. Aucun contrat 50k/tour entière
sous 1 s, puis 100 ms, ni plusieurs dizaines de millions sur G4 n'est
acquis. La diminution d'un compteur ne suffit pas à les annoncer.
GCP non utilisé.
