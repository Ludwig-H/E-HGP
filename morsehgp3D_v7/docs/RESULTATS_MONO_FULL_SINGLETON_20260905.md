# FULL mono : lots à une seule directe

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Objet constant et admission

Le [delta singleton](CONTRAT_LOT_UNITAIRE_FULL.md) remplace uniquement
la DSU locale des lots à une seule directe par quatre racines triées et
dédupliquées. Les demandes géométriques, les 33 compteurs logiques, le
cache et tous les plafonds restent inchangés. Il ne modifie ni le
générateur q4 ni la normalisation des successeurs.

Les [17 tests ciblés par build](../receipts/full_gabriel_singleton_20260905/README.md)
ont terminé dans des constructions fraîches Release et ASan/UBSan,
LeakSanitizer actif, avant les mesures. Les 434 allocations lazy du témoin
deviennent 209 sur les mêmes six cellules de test ; ce n'est pas une
mesure d'allocations de la sonde lourde.

Même [sonde v2 et sérialiseur](CONTRAT_DIGEST_FULL.md), O3/NDEBUG,
40 dépendances MMD utilisateur. Le témoin compilé `1d5a38ce…` reste lié
au header `13c6cc72…` ; le nouveau binaire `57c598bf…` au header
`21b77d29…`. Les dépendances compilées de l'ancien binaire ne sont jamais
remplacées par les sources vivantes du nouveau.

L'admission fraîche rejoue 24 micros par binaire à n=8 : s=8/10/12,
Kmax=5/10, eager puis lazy C=0/1/1M. Les 24 paires comparent 156 ordres ;
Kmax=10 s'arrête à K=n=8. Tous les champs rapportés hors liste explicite
des timers et RSS sont égaux, types JSON inclus. Les refus parser, le
juge v2 et le supplément first-C sont exécutés ; les 14 mutants de
champs et trois mutants de provenance du comparateur sont réfutés,
en Python normal et `-O`. Ces micros ne sont pas des mesures de latence.

## Protocole mono

Nuage uniforme/seed3/u16, n=8 000, K1..10 horizontaux, lazy C=1M,
un thread CPU6. Ordre fixé : ancien/nouveau à s8, nouveau/ancien à s10,
ancien/nouveau à s12. Chaque moteur a 600 s, puis 10 s de grâce ; limite
d'espace d'adressage 26 GiB, proxy de payload 8 GiB. Aucune augmentation
des caps. Les moteurs de l'auditeur sont clos à 20:19:12 UTC avant le
premier passage ; les lectures documentaires restent distinctes.

Le temps jusqu'au terminal inclut calcul, lecteur, digests, sorties
provisoires et libérations dans cet intervalle. Les temps d'étapes et
le pic GNU time sont conservés séparément. Une paire par s sur cet hôte
partagé est une observation, pas un p95 ni une qualification statistique
du gain. Le défaut s n'est pas choisi sur ces seules paires.

## Six passages clos : pas d'accélération robuste établie

Le [paquet publié](../receipts/full_gabriel_singleton_mono_20260905/README.md)
conserve le build, l'admission et les six captures lourdes, toutes
terminées sans refus ni timeout. Le reçu de campagne `eabece59…` clôt
trois paires et 30 ordres comparés, sources et binaires stables.
Les digests par ordre et tous les champs hors mesures sont identiques
dans chaque paire ; le digest global `e6e3fa51…` et l'entrée `b7374475…`
coïncident aussi entre s. Les juges v2 et first-C passent normalement
et sous `-O`. Aucun de ces contrôles ne certifie la complétude géométrique.

| s WSPD | Ancien, s | Nouveau, s | Variation | Pic ancien, MiB | Pic nouveau, MiB |
| --- | ---: | ---: | ---: | ---: | ---: |
| 8 | 145,540 | 144,337 | −0,83 % | 1 292,758 | 1 289,758 |
| 10 | 141,857 | 145,201 | +2,36 % | 1 291,422 | 1 289,867 |
| 12 | 145,436 | 145,544 | +0,07 % | 1 291,734 | 1 291,332 |

Temps jusqu'au terminal ; pics GNU time convertis de KiB en MiB, pas
échantillons après destruction du Builder. Les variations sont de signes
opposés. Le temps du seul constructeur FULL passe respectivement de
66,802 à 65,381 s, de 64,842 à 64,986 s et de 65,406 à 64,021 s.
Cela n'établit ni gain global robuste, ni meilleur s ; les faibles
écarts de pic ne constituent pas une nouvelle qualification de résidence.
Les allocations sont réduites sur les fixtures, sans être comptées par
ces passages lourds. Les résultats défavorables ne sont pas écartés.

À 8k/K10, les 1 202 962 appels MEB, 250 854 612 supports candidats et
96 517 944 nœuds de requêtes restent identiques. La génération prend
encore plusieurs dizaines de secondes et n'est pas modifiée. Ces coûts
motivent les deltas suivants ; ils ne deviennent pas des gains prédits.

## Portée et suite

Les [paliers 16k/32k précédents](RESULTATS_MONO_FULL_LAZY_20260905.md)
restent attachés à `13c6cc72…`. Ce delta garde les dépenses de successeurs :
il ne lève donc pas, par lui-même, le refus 32k/K9 à 128 millions.
Une normalisation allégée aura un calendrier de charge versionné et
ses propres essais, sans soustraire des dépenses déjà effectuées.

Les forêts horizontales sont lues puis détruites par ordre, pas archivées
ensemble. Ni verticale inter-K, ni supplément pondéré, ni tour FULL
intégrée ne sont qualifiés. Les contrats 50k/tour entière sous 1 s,
puis 100 ms, et plusieurs dizaines de millions sur G4 restent ouverts.
GCP non utilisé.
