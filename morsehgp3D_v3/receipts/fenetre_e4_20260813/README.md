# Reçu — fenêtre d'arêtes `E_4` par `EdgeWindowRangeAdd-v0`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. **GCP non utilisé.** Aucun kernel, aucun `BallKey`,
aucun census, aucun fold, aucun payload : ce reçu compte des arêtes candidates,
il n'en construit aucune.

Ce dossier est un **diagnostic daté**, jamais une porte v3 ni un benchmark reçu.
Le contre-audit `b96751c` relevait à juste titre qu'il n'avait ni README, ni
manifeste, ni commande, ni graine ; c'est ce que ce fichier répare.

## Provenance

| élément | valeur |
| :--- | :--- |
| sujet | `prototype/wspd_wavefront_probe.cpp` |
| binaire | `build/v3/mhgp3v_wspd_wavefront_probe` |
| construction | `cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release` |
| graine | `12345` (défaut du probe, jamais passée en option) |
| coordonnées | `--coord` non passé, donc `65535` |
| répétitions | **une seule par taille**, sans p95 |
| machine | codespace 2 vCPU ; les temps ne sont pas qualifiables |

Les `SHA-256` du sujet, du binaire et de chaque sortie sont dans
[`manifeste.txt`](manifeste.txt), produit par
[`refaire.sh`](refaire.sh), qui contient aussi les commandes exactes.

## Fichiers

| fichier | commande | portée |
| :--- | :--- | :--- |
| `uniform_s8.txt` | `--family=uniform --points=3000,6000,12000,24000 --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger --max-slope=9` | rampe volumique ; `sum E_4` linéaire, `max E_4` borné |
| `terrain_s8.txt` | idem, `--family=terrain` | nappe ; pentes `sum E_4` croissantes |
| `terrain_s16.txt` | `--points=3000,6000,12000 --sep-euclid=16/1` | la même nappe à `s=16` ; améliore sans réparer |
| `eight_clusters_s8.txt` | idem `uniform_s8` sans `--max-slope=9` | amas ; `sum E_4` quadratique. **Finit en code `3`** : l'ancienne porte `degre_residuel` refuse à `1,458` puis `1,502`, avant l'impression des pentes `sum E_4`. Les quatre tailles ont bien terminé et leurs quatre lignes `fenetre q4` sont présentes ; les pentes `sum E_4` de ce fichier sont donc **calculées à la main** dans la note, non imprimées par le binaire |
| `coeur_par_paire_amas.txt` | `--inflation=4000 --inflation-lane=2` sur `eight_clusters` à trois tailles | tendance du cœur **par paire**, boule inscrite `209` uniquement ; ce fichier ne porte **aucun** compteur anisotrope |

## Limites explicites

- une seule graine, une seule répétition, aucun p95 : ces pentes ne sont pas des
  pentes reçues au sens du contrat ;
- `scanline_overlap_multiecho` et `scanline_single_pass` n'ont pas été mesurées ;
- `eight_clusters_s8.txt` sort en code `3` ; son verdict est un refus de pente
  de l'**ancienne** porte, pas de la porte `sum E_4` ;
- le libellé `E_4` désigne ici le résiduel du certificateur central sous
  hypothèse d'arête maximale, **pas** le reporter projectif `PWC0-A` ;
- aucun oracle n'établit que l'arête maximale canonique de chaque vrai q4 tombe
  dans la fenêtre centrale : l'oracle du probe juge le ledger, pas la géométrie.

Le sujet est jugé dans
[`../../audits/AUDIT_ETAT_COURANT.md`](../../audits/AUDIT_ETAT_COURANT.md) ; ce
reçu ne reçoit rien par lui-même.
