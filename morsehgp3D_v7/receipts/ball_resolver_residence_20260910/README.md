# Cache exact du resolver FULL et réduction de résidence

10 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé par ce paquet ; aucun ELF. Les chiffres sont des diagnostics
locaux, jamais le contrat 50k ni une qualification GPU.

## Delta actif et preuve bornée

Le cache facultatif réutilise une facette triée de K indices géométriques,
après comparaison de toute la clé. Un hash seul n'est jamais une identité.
Une collision évince une entrée ; aucun sondage ou parcours de collisions ne
peut devenir quadratique. La capacité est la prochaine puissance de deux au
moins égale à 16n, sans plafond fixe ; une allocation impossible désactive
seulement le cache. Le token historique d'un hit est toujours normalisé vers
la racine pré-lot par `root(token, prior_count)`.

Les réponses antérieures restent valides parce que les niveaux d'un ordre
croissent et que ses composantes ne font que continuer ou fusionner. La table
est vidée entre les ordres. Après fermeture complète d'un lot, le constructeur
sème aussi l'unique facette `I ∪ U` quand sa population fermée a cardinal K.
Le support positif contenu dans cette population certifie sa MEB ; toutes les
consultations ultérieures sont strictement après sa naissance. Aucun catalogue
Gamma ni ensemble de sous-facettes n'est construit. Le semis est permanent,
sans macro `PRIVATE`, dans les octets actifs qualifiés.

Les deux deltas de [résidence qualifiée](../full_tower_residence_20260910/README.md)
libèrent les états de construction morts avant la copie de banque et réservent
les tailles exactes des tableaux du journal. Le cache est libéré à la même
frontière ; `resolver_cache_released_slots` comptabilise la libération réelle.
Les compteurs `resolver_cache_slots` et `resolver_cache_bytes` conservent la
capacité allouée à des fins de mesure : ils ne décrivent pas des octets encore
vivants après la libération. Le schéma `full_dated_coverage_forest_v2`, la banque
immuable, les populations, les identifiants, les dates et les images verticales
restent inchangés.

Sources actives :

- `full_ball_tower.hpp` : `910f45baea1750b11d2b34f40c893c9d1a34f950705cdb127ffa226de60f7b2e` ;
- `full_coverage_certificate.hpp` : `7608e70ec0bf7df7ed726ae2388a39e800ab2db35043b4ba42c619ceef13bac0` ;
- test cache final : `898533cacf20b5d3444cd63250bba8f7727f709c3e5ee6b97ad7a68d71e42a64`.

Les deux headers de base étaient `0b72b4e9…` et `e8e65b21…`, correspondant au
moteur annoncé `d188e3de`. Les snapshots et SHA complets sont conservés dans
`combined/`; l'attribution cache seul/avec semis est conservée dans `comparison/`.

## Qualification et réfutations

Les quatre portes passent en O2 et ASan/UBSan (`detect_leaks=1`, arrêt sur erreur),
avec selftest 0 et argument inconnu 2, sorties O2/SAN identiques :

| Porte | Contrôles bornés |
| --- | --- |
| Tour indépendante Gram/Gamma | 170 320 contrôles, 28 nuages, 112 ordres, 2 508 coupes, 45 948 contrôles verticaux, huit refus |
| Travail | Peignes 256/512/1024, lots unitaires/groupés, validation directe et MEB supplémentaires |
| Journal structurel | 823 contrôles, 30 refus, 30 coupes de replay, 40 coupes Gamma, 20 refus d'allocation injectés |
| Cache physique | Collisions/évictions, allocation facultative refusée, reset/libération ; 28 nuages, 556 hits, 352 semis, 2 752 slots libérés |

Six mutants sur les headers normalisés sont tués : identité de clé ignorée,
token non normalisé, semis omis, libération omise, croissance groupée omise,
ancre inerte groupée omise. La croissance groupée échoue sur
`growth_ABCZ_doubled_lot/gamma.coverage_multiset_including_multiplicity` ;
l'ancre inerte groupée sur `full_ball_vertical_birth_anchor`. Le cache ne masque
donc pas ces deux réfutateurs de lots groupés.

Deux résultats négatifs sont conservés explicitement :

- La première version de l'injecteur du nouveau test cache ne surchargeait pas
  `new(nothrow)` ; ASan a signalé son mélange `operator new`/`free` lors de
  `stable_sort`. Les moteurs n'ont pas changé. Les surcharges appariées ont été
  ajoutées au test, puis sa compilation et ses exécutions O2/SAN rejouées dans
  des captures neuves. Les trois autres portes SAN étaient déjà vertes.
- Dans l'exploration initiale, omettre le reset inter-K ne change pas les sorties :
  les clés complètes triées de points distincts non négatifs, avec padding nul,
  encodent déjà implicitement K pour K ≥ 2. Ce reset borne la résidence par ordre ;
  il n'est pas prétendu condition géométrique indispensable de ce layout.

Les mutants cache ont été compilés en O2 sur les moteurs définitifs, avant la
correction `nothrow` du seul injecteur ; leurs assertions et leurs données sont
inchangées. Les deux mutants groupés ne consomment pas cet injecteur. Les portes
nominales finales utilisent bien le test corrigé.

## Attribution initiale : cache seul puis semis

Trois répétitions appariées à ordre alterné, uniform/graine3/s8/K1..10/mono-thread.
Toutes les entrées et tous les payloads sont identiques à n fixé. Temps de tour
sur une machine partagée avec des compilations concurrentes :

| n | bras | appels MEB | supports exacts | tour médiane, s |
| ---: | --- | ---: | ---: | ---: |
| 400 | baseline | 392 135 | 31 719 733 | 2,7205 |
| 400 | cache seul | 251 687 | 21 519 754 | 2,1252 |
| 400 | cache + semis | 180 260 | 15 708 447 | 1,6217 |
| 1 000 | baseline | 1 174 515 | 97 376 638 | 8,3710 |
| 1 000 | cache seul | 783 299 | 68 700 123 | 6,4347 |
| 1 000 | cache + semis | 583 337 | 52 168 577 | 5,2963 |

Le cache + semis supprime 54,03 % puis 50,33 % des appels MEB. L'ensemble des
compteurs et les temps totaux sont dans les sorties JSON brutes. Le triplet
supplémentaire intitulé `run_quiet_diagnostics` ne certifie pas l'isolation :
un compilateur apparaît au relevé final, même si aucun n'était observé avant.

## Combinaison active : n1000 apparié

Même entrée uniforme, s8, K1..10, un fil. Un passage par bras, pendant les grands
runs locaux de ROOT et les dernières compilations : chronométrage diagnostique
partagé, non contractuel. Digest exact commun :
`1040c1be04a90dbb11982e9f78b6034e9695b6e69f3746bc47d291a03d1924cc`.

| Grandeur | Baseline | Combinaison active |
| --- | ---: | ---: |
| MEB réelles | 1 174 515 | 583 337 |
| Supports exacts évalués | 97 376 638 | 52 168 577 |
| Capacité des tableaux de sortie, verticale comprise | 74 448 896 octets | 55 758 656 octets |
| Pic RSS processus | 229 956 Kio | 233 156 Kio |
| Tour | 14,7510 s | 9,7710 s |
| Total du probe | 26,7277 s | 21,0335 s |

La capacité de sortie baisse de 25,10 %, mais le pic RSS ne baisse pas : ne pas
transformer ce gain de capacité en claim de pic mémoire. Le cache de ce run
alloue 786 432 octets, fait 580 186 hits, puis libère ses 16 384 slots avant
la copie de banque. Son dimensionnement prend 24 Mio à 32k, 48 Mio à 50k et
24 Gio à 30 millions : la capacité devra être requalifiée pour les très grands
nuages. Aucun résultat sous-quadratique global ne découle du seul cache ; ses
opérations supplémentaires sont O(K) par requête/insertion, avec K ≤ 10, et
ses resets sont O(n) par ordre.

## Lecture reproductible sans compilation

Depuis la racine :

```bash
python3 -B morsehgp3D_v7/receipts/ball_resolver_residence_20260910/verify.py
python3 -B -O morsehgp3D_v7/receipts/ball_resolver_residence_20260910/verify.py
```

Le lecteur contrôle SHA des fichiers, fermeture des sources, codes attendus,
les deux exceptions historiques déclarées, égalité O2/SAN, digests appariés et
réductions physiques. Il ne compile rien, n'exécute aucun binaire ni ne contacte
GCP. Les commandes de compilation originales, options, horodatages, sorties
brutes, dépendances et mutations physiques sont conservés ; les chemins absolus
de ces captures désignent leur environnement original, pas une autorité actuelle.
