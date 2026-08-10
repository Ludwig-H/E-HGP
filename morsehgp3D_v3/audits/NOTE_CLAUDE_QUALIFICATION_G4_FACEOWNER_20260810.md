# Note de Claude — le kernel face-owner est qualifié sur la G4

Date : 10 août 2026 UTC. Auteur : Claude. Cadre :
`phase=exploration_v3_hors_registre`, `profile=quantized_u16_input_only`,
aucun statut public — une QUALIFICATION de kernel par différentiel natif,
jamais une exactitude 50 k.

## Session G4 (protocole gardé, ~15 minutes de VM)

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
| 400 | 44 ,5 M | 13,9 M | **49,25 ms** | 8,67 s | IDENTIQUES ×5 ordres |

Le mutant `--force-drop-edge` (dernière arête device retirée) rend le code 1 :
le comparateur mord. L'admission VRAM (NO-GO au-delà de 70 % de la mémoire
libre) est vérifiée avant chaque lancement ; pic observé ~1,07 Go à n=400
contre 94,4 Go libres.

## Lecture honnête des chiffres

- Le chiffre device couvre émission+tri+owner+étoiles — la partie MASSIVE du
  join. Le rejeu DSU par lots, le marquage, les records et le catalogue
  restent sur l'hôte et ne sont PAS dans les millisecondes device.
- À n=400 le join device vaut ~49 ms là où le fold CPU complet vaut 8,7 s sur
  le même hôte : le mur du join est effacé par le device À CES TAILLES ; le
  poste dominant devient le GÉNÉRATEUR (`flat_catalogue`, 52 s à n=400 sur un
  cœur) et le rejeu hôte — exactement la hiérarchie que la route à deux
  étages du contrat 100 ms prévoit (construction offline, requête sur
  certificat).
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
