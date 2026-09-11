# Clés de boules GPU : prototype privé qualifié localement

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé par ce paquet ; **aucun device n'a encore été exécuté ici**.

Trois annotations privées `MHGP7_HD` rendent accessibles les fonctions
existantes `ugcd64`, `ugcd128` et `ball_key_reduce`. Pas de duplication du
diviseur ni changement du moteur actif. q2 donne déjà une clé primitive ;
q3/q4 forment leurs coefficients puis utilisent ce même réducteur.
Le [contrat et la preuve](NOTE_MATH_ET_DEVICE.md) distinguent canonisation
algébrique, circumboule, support positif, MEB et terminal de descente.

## Résultats fermés

O2 et ASan/UBSan ROOT avec `detect_leaks=1` : **83 215 contrôles** contre
cpp_int et un système de Gram rationnel indépendant. Domaines non vides :
4 103 formes i128, 2 171 PGCD u128, 4 217 divisions i128/i128, 1 025 supports
de chacune des arités 2/3/4, 1 536 permutations, 20 refus et deux dégénérescences
géométriques séparées. Les tests couvrent MIN128, signes, diviseurs au-delà
de 64 bits, coefficients hauts et clé commune entre les trois lanes.

Cinq mutations physiques compilent puis sortent au code 1 sur leurs juges
causaux : haut du PGCD tronqué, c omis du PGCD, diviseur large tronqué,
signe du reste perdu et a=0 admis. Un refus de compilation n'est pas compté
comme mutant réfuté. Une première exécution SAN sous-agent a échoué sur
LSan/ptrace ; sa capture est conservée. La réussite vient d'une recompilation
ROOT du juge avec export, sans désactiver LeakSanitizer, pas d'une réécriture
du statut de l'échec initial.

Le juge exporte ses attentes depuis cpp_int/Gram, jamais depuis les sorties
natives. Le fichier comporte **13 573 cas** et est épinglé à
`4bf82467f04ab30a78c9704603578b4ae8838f44cbc2620ef59fdd6bcb06b679`.
La gate autonome **sans Boost** passe O2, SAN ROOT et le mode `--host` du
binaire NVCC : **325 752 comparaisons de mots**, 22 refus attendus inclus.
Écriture omise et mot corrompu sont réfutés au code 1 par une divergence
`backend.expected_word` ; mauvais SHA et arguments sont refusés séparément.

Les vrais kernels compilent et se lient sous NVCC 12.9 strict, SM120.
Une première compilation de la nouvelle gate a échoué car sa passe hôte
voyait un `__CUDA_ARCH__` absent ; échec/source préservés, puis branche hôte
explicite corrigée. La gate device utilise un transport de mots u64, sans
copie d'objets i128, et un kernel de vérification de taille/alignement/offset,
endianness et architecture. Ses allocations ont un nettoyage vérifié avant
PASS. **Compiler/lier ou exécuter `--host` ne vaut pas une exécution CUDA.**

La gate générique compilée utilise 82 registres et 104 octets de stack,
sans spills ; ce n'est ni un débit ni l'occupation du futur kernel fusionné.
La normalisation n'effectue pas encore lookup, admission K, recherche d'intrus,
descente ni assemblage FULL. Aucun contrat 50k/1s, 100ms ou massif n'est acquis.

## Lecture et reproduction

```bash
python3 -B morsehgp3D_v7/receipts/gpu_ball_key_device_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/gpu_ball_key_device_20260911/verify.py
```

`manifest.json` décrit les chemins logiques, leurs objets SHA-256, et les
hashes des ELF exclus. Tous les snapshots avant/après et toutes les anciennes
sources mutantes/échouées restent disponibles. Aucun ELF ni vendor inclus.
Une parenthèse manquante dans le premier script d'emballage a été corrigée ;
source fautive et reproduction de son erreur de syntaxe sont conservées,
sans modifier une capture scientifique ni compter cet échec comme mutant.
Le lecteur vérifie les objets puis réemploie le même lecteur Python épinglé
qui a été exécuté localement, avec une vue en lecture seule des objets ;
il ne lance ni compilateur, ni C++, ni kernel, ni GCP.

Les sources autonomes sont `device_gate.cu`, `wire.cuh`, `ball_key_device.cuh`
et `source/morsehgp3D_v7/src/`. Les commandes exactes sont dans les reçus
`device_nvcc_r2` et `host_san_root_r1` ; l'adaptateur strict NVCC est conservé.
Matérialiser ces fichiers avec leurs includes relatifs et le fichier
`export_r1/export_vectors.stdout`, puis recompiler constitue une nouvelle
capture à épingler. Le fichier `README.md` logique décrit les options de gate,
notamment `--selftest PATH --expected-sha=HASH`, `--host` et les deux injections.
L'exécution G4 future appartient à un autre reçu fermé ; elle n'est pas
anticipée par le présent paquet local.
