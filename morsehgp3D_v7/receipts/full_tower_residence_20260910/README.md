# Réduction de résidence FULL — qualification bornée

10 septembre 2026. Capture privée publiée sans changement de code actif ni opération Git/GCP. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Deux petits deltas conservent exactement l’API et les sorties physiques : libération des structures mortes avant copie de la banque immuable, puis réservation exacte des arènes du journal. Le [rapport complet](capture/README.md) contient la preuve de durée de vie, les limites, le plan de streaming transactionnel et le [delta minimal](capture/minimal_delta.patch).

Sur n=200/400/800, s=8, toute la tour K1..10, un fil CPU : mêmes digests physiques pour les quatre variantes. À n=800, sortie conservée **75,10→57,76 MB** (−23,1 %), pic d’octets demandés **108,45→105,19 MB** (−3,0 %). Ce compteur exclut l’entrée, les métadonnées d’allocateur et la fragmentation : **ce n’est pas le RSS**. Aucun gain de temps ni extrapolation32k/50k n’est revendiqué. Le pic restant apparaît déjà pendant la construction K10 ; l’aplatissement des brouillons reste nécessaire.

Qualification des deux deltas ensemble : portes O2 et ASan/UBSan strictes, 28 nuages, 170 320 contrôles Gamma/histoire, 45 948 contrôles verticaux ; journal structurel 823 contrôles, 30 rejets et les 20 allocations encore observées toutes refusées tour à tour. Les anciennes 34 allocations testées deviennent20 grâce aux réservations ; le plancher de non-vacuité original passe sans modification. Codes0/2 vérifiés. L’état combiné avec le nouveau cache du resolver est qualifié dans un reçu distinct : ne pas transférer les chronométrages de cette capture au code ultérieur.

Le [manifeste](capture/manifest.json) est épinglé à `84b439e0309c6beda2c0e7414477be403b5a364e384ab5b3cd717fba40196960`. Il comprend360 fichiers de preuve et sources, sans ELF ; le vérificateur est portable et en lecture seule. Les scripts de capture conservent leurs chemins historiques `build/` : pour reproduire, les placer dans un nouveau répertoire de travail à la même profondeur, jamais écrire dans ce reçu.

```bash
python3 -B morsehgp3D_v7/receipts/full_tower_residence_20260910/verify.py
python3 -B -O morsehgp3D_v7/receipts/full_tower_residence_20260910/verify.py
```

Contrats50k/1s, 100ms et plusieurs dizaines de millions de points non qualifiés par ce reçu. GCP non utilisé.
