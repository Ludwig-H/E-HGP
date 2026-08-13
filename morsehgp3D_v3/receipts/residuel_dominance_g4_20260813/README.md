# Reçu G4 du 13 août 2026 — la masse résiduelle de la dominance 432

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

## Provenance

Session `gcp-migration/session_residuel_dominance_g4.sh`, scripts gardés,
instance `ehgp-blackwell-spot-ai1a`, zone `europe-west4-ai1a`,
`g4-standard-48` SPOT, `maxRunDuration=3600 s`, arrêt invité secondaire à
`45 min`. Arrêt certifié `TERMINATED` sur exactement la génération démarrée,
`SPOT`, `maxRunDuration=3600 s`, après la session.

**Cette session ne mesure aucun débit GPU et ne qualifie aucun SLO.** Elle
emploie la machine comme ressource de calcul CPU : le sujet est mono-thread, et
les quarante-huit cœurs servent à lancer les douze runs en parallèle. Le NO-GO
device du contre-audit reste entier.

`coeurs=48`. `CUDA_COMPILE=OK` : la cible CUDA opt-in compile devant `nvcc`
avec `sm_120`, ce que le poste local ne peut pas vérifier. Cela reçoit la
réparation de l'ABI `run_anchor_point` par suppression du paramètre
`density_guard`, et rien d'autre.

Portes reconstruites sur la VM : `98/105`. Les sept rouges sont `mhgp3v_coeur_*`
et proviennent d'une cible manquante dans le script de session — 
`mhgp3v_common_core_probe` n'est pas construit — non d'un défaut du sujet.

## Ce qui est mesuré

Le résiduel `PairId` **non ordonné** et **en valeur absolue** de la lane q4,
après fusion `OU` des deux orientations, sur les quatre familles contractuelles
et la rampe `12 500 / 25 000 / 50 000`. Les pentes log2 sont calculées sur la
VM, sans retranscription.

| famille | `12 500` | `25 000` | `50 000` | pentes | verdict |
| --- | ---: | ---: | ---: | :---: | :---: |
| `uniform` | `34 556 198` | `84 744 304` | `197 456 334` | `1,294 / 1,220` | **VERT** |
| `eight_clusters` | `47 330 683` | `138 139 560` | `382 687 920` | `1,545 / 1,470` | REFUSÉ |
| `terrain` | `12 475 686` | `34 024 195` | `94 469 037` | `1,447 / 1,473` | REFUSÉ |
| `scanline_overlap_multiecho` | `19 365 990` | `58 982 111` | `174 824 688` | `1,607 / 1,568` | REFUSÉ |

Fractions fermées correspondantes :

| famille | `12 500` | `25 000` | `50 000` |
| --- | ---: | ---: | ---: |
| `uniform` | `55,8 %` | `72,9 %` | `84,2 %` |
| `eight_clusters` | `39,4 %` | `55,8 %` | `69,4 %` |
| `terrain` | `84,0 %` | `89,1 %` | `92,4 %` |
| `scanline_overlap_multiecho` | `75,2 %` | `81,1 %` | `86,0 %` |

## L'inversion, qui est le fait le plus instructif

**La famille où le certificat ferme le moins a la meilleure pente.** `terrain`
est déjà à `92,4 %` de fermeture et sa pente vaut `1,473` ; `uniform` part de
`55,8 %`, monte à `84,2 %`, et sa pente tombe à `1,220`.

Couverture et pente vont donc en sens inverse, et la raison est la saturation :
un certificat qui ferme déjà presque tout n'a plus de marge, donc son résiduel
suit `n^2` de près. C'est une propriété structurelle, pas un accident de famille,
et elle interdit de lire une forte couverture comme un bon signe d'échelle.

## Ce que ce reçu NE décide pas

Cette quantité est la masse **sémantique**. Le contre-audit est explicite :
« la masse quadratique peut rester un entier sémantique ; le nombre de records,
visites et octets **physiques** doit, lui, rester sous les pentes
contractuelles ». Un `RectId` résiduel couvre `|A| x |B|` paires avec un seul
enregistrement.

Il est en outre **prouvé** que cette masse est quadratique au pire cas : deux
amas serrés séparés laissent `Theta(|A| |B|)` candidatures, et Chazelle et al.
construisent `n^2` arêtes de Gabriel croisées dans `R^3`. La question posée par
ce reçu n'était donc pas ouverte, et son résultat ne réfute pas la voie.

## Ce que ce reçu décide

Il chiffre le **cahier des charges du front factorisé**. Pour que les records
physiques croissent linéairement là où la masse croît en `n^1,47`, le rectangle
moyen doit couvrir `n^0,47` paires, soit environ `150` paires par record à
`50 000` points — un rectangle de l'ordre de `12 x 12`. Ce n'est pas absurde, et
c'est désormais une contrainte chiffrée plutôt qu'une espérance.

Aucune pente de travail, aucun octet, aucun high-water n'est mesuré ici. La
boucle du sujet reste en `n(n-1)` et n'est pas l'ordonnance. Le contrat
`50 000` reste entièrement ouvert et G4 reste NO-GO pour toute qualification.
