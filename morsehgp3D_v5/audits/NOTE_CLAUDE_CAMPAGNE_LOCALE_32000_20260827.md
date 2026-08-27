# Note — campagne locale de conformité v4 ≡ v5 aux trois tailles d'intérêt

- **Date :** 27 août 2026
- **Pin du moteur :** `210571fb` (binaire `mhgp5_conformity_v4` de `build/v5`, sha256 dans le manifeste)
- **Reçu :** `../receipts/conformite_v4/campagne_v5_210571fb_20260827.txt` (manifeste : pin, sha256 du binaire et du reçu v4, toolchain, machine ; par cas : code, temps, RSS max, digests, cardinalités) et le dossier de sorties brutes du même nom.
- Machine : 8 vCPU, 31 Go, **partagée** pendant toute la campagne avec la suite ASan/UBSan et des builds — les temps ne sont pas des mesures de coût.
- GCP non utilisé pour cette campagne (la session G4 est distincte et a son propre reçu).

## Résultat

**12/12 configurations conformes** (`digest_balls` ET `digest_all` égaux aux digests calculés par la v4) :

| famille | n=8000 | n=16000 | n=32000 |
|---|---|---|---|
| uniform | égal (98 s) | égal (201 s) | égal (523 s) |
| terrain | égal (20 s) | égal (42 s) | égal (177 s) |
| eight_clusters | égal (178 s) | égal (581 s) | égal (1 672 s) |
| scanline_single_pass | égal (28 s) | égal (51 s) | égal (140 s) |

Pic de résidence (RSS max cumulé du processus de conformité, `getrusage`) : **2,6 Go à 8000, 4,7 Go à 16000, 8,3 Go à 32000** — la v4 mesurait 5,7 / 11,0 / 21,0 Go sur les mêmes entrées (dix forêts résidentes). Le streaming par ordre K (`src/pipeline/run.hpp`) est donc effectif ; le pic à 32000 est porté par les boules censusées (amont résident) et le fold de K = 10.

## Ce que cela prouve, et ce que cela ne prouve pas

- Prouvé : la v5 produit **le même objet que la v4** (candidats post-RLE, dix forêts horizontales : facettes, partitions finales, deltas) sur ces douze entrées, avec une résidence réduite.
- Non prouvé : l'exactitude HGP (elle relève des oracles bornés, tous verts, et des preuves de `../docs/MATHEMATIQUES.md`), la tour (applications verticales non livrées), tout SLO de temps (machine partagée ; la campagne G4 mesure à 48 fils sur machine dédiée).
- ASan/UBSan : suite `gate` 103/103 sans rapport, au pin `096ae788` (antérieur au streaming par K) ; à rejouer sur `HEAD` sur machine libre.
