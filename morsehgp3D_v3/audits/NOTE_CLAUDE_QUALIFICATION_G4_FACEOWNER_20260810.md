# Note de Claude — le kernel face-owner est qualifié sur la G4

Date : 10 août 2026 UTC. Auteur : Claude. Cadre :
`phase=exploration_v3_hors_registre`, `profile=quantized_u16_input_only`,
aucun statut public — une QUALIFICATION de kernel par différentiel natif,
jamais une exactitude 50 k.

## Session G4 (protocole gardé, génération GCE de 5 min 08 s)

`ehgp-blackwell-spot-ai1a` (g4-standard-48, RTX PRO 6000 Blackwell, 95 Go
VRAM, nvcc 12.9, sm_120) : `maxRunDuration=7200 s` recertifié, clé ED25519 de
session à TTL borné par la garde (7200..7860 s), démarrage à deux
coupe-circuits, sources par tar+scp (1,0 Mo), `pip cmake` 4.4.2, configure
`-DMHGP3V_ENABLE_CUDA=ON`, build sans diagnostic, **arrêt certifié TERMINATED
et clé révoquée en fin de session**.

## Le verdict : flux d'arêtes device == CPU, arête par arête

Le kernel (`faceowner_device_kernel.cu`) émet les signatures par dé-rangement
combinatoire lexicographique (la même table de binomiales que le préflight
hôte), trie les clefs 128 bits, choisit l'owner minimal en (rang d'activation,
générateur) par réduction segmentée et rend les branches d'étoile dédupliquées
triées par (lot, owner, membre) — l'objet exact du chemin CPU `collect_edges`.

| n | incidences K=5 | arêtes | device (émission+tri+réduction) | fold CPU complet | verdict |
| ---: | ---: | ---: | ---: | ---: | --- |
| 64 | 3 030 554 | 1 319 076 | **5,95 ms** | 0,52 s | IDENTIQUES ×5 ordres |
| 200 | 17 282 892 | 7 385 988 | **18,73 ms** | 3,18 s | IDENTIQUES ×5 ordres |
| 400 | 44 258 951 | 19 073 174 | **49,25 ms** | 8,67 s | IDENTIQUES ×5 ordres |

Le mutant `--force-drop-edge` (dernière arête device retirée) rend le code 1 :
le comparateur mord. L'admission VRAM (NO-GO au-delà de 70 % de la mémoire
libre) est vérifiée avant chaque lancement ; pic observé ~1,07 Go à n=400
contre 94,4 Go libres.

## Lecture honnête des chiffres

- LES DEUX PÉRIMÈTRES DIFFÈRENT (réception `23379d4`) : le chiffre device
  couvre émission+tri+owner+étoiles et EXCLUT allocations, H2D, D2H et rejeu ;
  le chiffre CPU couvre le fold complet avec la copie des arêtes. Le rapport
  produit sera mur total contre mur total, phases publiées des deux côtés —
  la conclusion recevable aujourd'hui est : 44 M d'incidences sont une charge
  device très favorable, pas « le mur est effacé ».
- CORRECTIONS de la réception : la ligne n=400 publiait des totaux arrondis
  depuis un log tronqué (`44,5 M / 13,9 M`) — les masses exactes recertifiées
  par l'auditeur sur le même binaire CPU sont `44 258 951 / 19 073 174`
  (coord=73). La durée de génération GCE certifiée est 5 min 08 s. L'archive
  source de session a pour SHA-256
  `0717c3948ce6e99e399e6a78b4740cf9034243aea417d078c1d522195a6ced60` ; les
  fichiers de clé locaux ont été supprimés du scratchpad.
- La hiérarchie des coûts pointe vers le GÉNÉRATEUR (`flat_catalogue`,
  52 s à n=400 sur un cœur) et le rejeu hôte comme postes dominants — la
  route à deux étages du contrat 100 ms — mais cette phrase ne deviendra un
  reçu qu'avec les murs totaux des deux chemins, phases publiées.
- Ces tailles restent `partial_refinement` (`smax=11`) : la qualification
  porte sur le KERNEL, pas sur la complétude de la source — le verrou
  50 k exact demeure la source-certificat sparse.

## Prochaines portes que je propose à l'audit

1. Rendre la qualification G4 rejouable en CTest conditionnel (label `cuda`),
   avec planchers de masses et le mutant drop-edge en code attendu.
2. Étendre le kernel au REJEU device-résident (DSU par lots sur device, ou
   rejeu hôte en flux sans rapatrier les arêtes — elles pèsent 13,9 M × 12 o
  à n=400) et mesurer la chaîne complète device.
3. Brancher le fold hybride sur le même différentiel device quand la source
   certifiée existera — le kernel est agnostique (il consomme membres CSR et
   rangs, pas la provenance).

GCP : une session G4 SPOT gardée, VM certifiée TERMINATED, clé de session
révoquée.

## Re-qualification du kernel simplifié (seconde session, commit `a6e3078`)

Le kernel à owner-par-tri (réparation la plus courte de la réception : tri par
(signature, rang d'activation, générateur), owner = tête de groupe —
candidates/reduce_by_key retirés du pic) et validation hostile d'entrée est
RE-QUALIFIÉ dans une seconde session gardée (même protocole, VM certifiée
TERMINATED, clé révoquée ET fichiers locaux supprimés). GPU : RTX PRO 6000
Blackwell Server Edition, driver 580.173.02, CUDA 12.9, sm_120.

Masses PAR ORDRE versionnées à n=400/K=5 (coord=73, graine 20260810) — elles
reproduisent EXACTEMENT la recertification CPU de l'auditeur :

| k | incidences | signatures | arêtes | émission | tri | réduction |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 833 925 | 400 | 833 525 | 0,03 ms | 0,53 ms | 0,84 ms |
| 2 | 3 339 779 | 11 692 | 3 327 949 | 0,08 ms | 1,37 ms | 1,44 ms |
| 3 | 8 340 903 | 106 155 | 5 776 497 | 0,25 ms | 4,38 ms | 3,71 ms |
| 4 | 14 277 993 | 472 755 | 5 332 318 | 0,42 ms | 7,83 ms | 6,75 ms |
| 5 | 17 466 351 | 1 232 705 | 3 802 885 | 0,51 ms | 10,13 ms | 8,13 ms |

Totaux : 44 258 951 incidences, 19 073 174 arêtes — identiques au CPU arête
par arête sur les cinq ordres, device 46,40 ms (périmètre
émission+tri+owner+étoiles, hors H2D/D2H/rejeu), pic modèle ~939 Mo (‑12 %
après la simplification). n=200 : 17,31 ms, identiques ×5. Mutant drop-edge :
code 1. Le finding de provenance de la réception est fermé : chiffres par
ordre versionnés, plus de totaux arrondis.
