# Première campagne mono du producteur FULL horizontal

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Cette campagne observe le [producteur FULL](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md)
dans une sonde distincte de la CLI F. Elle ne mesure pas une tour inter-K
intégrée et ne qualifie aucun contrat 50k. Les catalogues sont partagés entre
les ordres ; chaque certificat est lu puis détruit. Ni archive, ni verticale,
ni profil pondéré ne sont produits.

## Protocole fixé avant les tentatives

Uniforme, 65 536 valeurs par coordonnée, graine 3, un thread réel, CPU6,
Kmax=10. La demande K=n est conservée quand n<Kmax ; les contrôles micro
sur huit points l'exercent séparément. Le binaire O3/NDEBUG est épinglé
`d6126f7778d7d7bb370cc59d356eb927bffa57f4cefeb72f8719a77ef6720204` ;
son [admission micro](../receipts/full_gabriel_probe_20260905/README.md)
ferme six positifs et six rejets de parsing.

Un seul processus de mesure à la fois, limite externe de 600 s puis grâce
de 10 s, limite d'espace virtuel au plus 26 GiB, proxy de payload nommé de
8 GiB. Les plafonds du constructeur sont fixés avant toute tentative,
notamment huit millions d'alias par ordre. Aucun plafond n'est relevé après
observation d'un refus. Ces gardes ne constituent pas un contrat de RSS.

Le temps principal est `elapsed_before_terminal_ms` : génération de
l'entrée, index, génération WSPD/candidats, tri, préfiltre, census, expansion,
construction FULL, lectures sentinelles, destructions et sorties provisoires.
La configuration initiale et l'écriture du terminal sont exclues ; GNU time
mesure séparément le processus entier et son pic RSS. Les horodatages de
collecte des outils ne sont pas utilisés comme chronomètre de processus.

## Résultats observés

| n | s | Temps avant terminal | Pic RSS processus | Statut des ordres horizontaux demandés |
| --- | --- | --- | --- | --- |
| 8 000 | 8 | 150,776 s | 1 837 632 KiB | 1..10 terminés relativement |
| 8 000 | 10 | 150,879 s | 1 831 988 KiB | 1..10 terminés relativement |
| 8 000 | 12 | 151,795 s | 1 833 128 KiB | 1..10 terminés relativement |
| 16 000 | 8 | 275,497 s | 2 893 256 KiB | Refus à K9 : `full_gabriel_alias_budget` |
| 32 000 | 8 | 464,273 s | 5 130 120 KiB | Refus à K7 : `full_gabriel_alias_budget` |

Les cinq tentatives sont closes, sans timeout. Un refus compte comme
tentative mesurée en échec, pas comme
temps de réussite d'un préfixe. Les huit ordres antérieurs du run 16k et
les six du run 32k restent diagnostiques ; aucune forêt n'est publiée.

Les [reçus bruts](../receipts/full_gabriel_mono_20260905/README.md)
conservent toutes les commandes, les lignes JSONL et les rapports GNU time.
Le juge de reçus distingue `audit_status=valid` — cohérence d'une capture —
de `attempt_success`, qui reste faux pour les refus. Ce n'est pas un oracle
géométrique ni une preuve d'égalité des forêts.

## Ce que ces mesures permettent de décider

Sur 8k/s8, la génération prend 61,434 s, le préfiltre 9,711 s, le census
7,298 s et la construction FULL cumulée 68,518 s. Le coût des grands ordres
reste élevé : K10 seul prend 24,962 s de construction, avec 6 209 024 alias,
714 823 appels MEB et 167 966 288 candidats de support examinés. La nouvelle
route n'appelle pas le constructeur de cœur F, mais ne rend pas gratuites
les recherches de portails ni la conservation des incidences déjà connues.

Les refus 16k/32k identifient un verrou précis de ce premier calendrier :
l'installation systématique des facettes incidentes consomme le budget
d'alias. Relever ce budget ne serait pas une optimisation. La prochaine
piste [contre-lue indépendamment](../audits/receipts_full_producer_20260905/lazy_alias_next_step_review.md)
sépare les autorités obligatoires —
minima et ancres des directes, y compris sans fusion — d'un cache facultatif
de facettes résolues. En cas d'absence, une facette à un intrus peut rejoindre
sa directe antérieure sans recopier tous ses alias à la naissance. Ce
changement modifierait explicitement le contrat du cache et du cas J=1 ;
il n'est pas intégré aux sources de cette campagne. Il peut économiser de
la mémoire tout en ajoutant des MEB/census : le temps reste à mesurer.

Une seule observation par configuration sur un hôte partagé ne classe
pas statistiquement les valeurs de s. La sonde ne calcule aucun digest
de forêt ; les comparaisons s=8/10/12 portent donc sur coûts et volumes,
pas sur une identité d'objets qu'elle n'a pas vérifiée. Le défaut s=8 reste
inchangé. Les anciennes mesures F portent un payload réduit différent et
ne sont pas une paire causale avec ce nouveau FULL : aucun facteur de gain
apparié n'est déduit de leur rapprochement.

Pour les trois s, le census conserve 3 113 381 boules, les nombres de
nœuds/minima/parents par ordre et tous les compteurs de portails observés
concordent. Le front, lui, diffère : 3 144 017, 3 129 992 et 3 123 497
candidats bruts respectivement, avec 61,434, 62,927 et 64,299 s de
génération. Cette égalité des volumes aval n'est toujours pas un digest
sémantique des forêts.

Les contrats **50k, toute la tour 1..10 en une seconde**, repli 1..5 puis
100 ms, restent non atteints. Les nuages de dizaines de millions sur G4
ne sont pas qualifiés. Cette campagne locale n'emploie pas GCP.
