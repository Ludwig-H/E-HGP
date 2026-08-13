# Reçu — la rampe qui déclenche le critère de mort de l'étape 1

Date : 13 août 2026 UTC. Session G4 employée comme ressource **CPU** : aucun
kernel, aucun débit GPU, aucun SLO revendiqué, `48` cœurs.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`.

## Provenance

| élément | valeur |
| :--- | :--- |
| `git_head` | `3c11bc8f99dd5f43eeaa973d61157ac2ae58e74e`, worktree **propre** |
| `tar_sha256` | `a2af7f2312732e5a5b87a13eaec8e92d8bab3fbed005de0c65fc17c3a3e40ea6` |
| ELF mesuré | `630e486285036575da4d4859b9bad38640bd950457e3cf3e496732f820151bc3` |
| script | `gcp-migration/session_fenetre_raffinement_g4.sh` |
| cible | `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, `g4-standard-48` SPOT |
| arrêt | **certifié `TERMINATED`** sur la génération démarrée |
| graine | défaut du probe, une seule ; une répétition par taille |

## Complétude

Les **quarante** runs rendent le code zéro, les dix fichiers publient leurs
quatre codes, `COMPLETUDE=OK`. Les portes ont été rejouées indépendamment sur la
VM : `27/27` pour `wspd_wavefront`, `10/10` pour `ball_event`.

C'est la première session de ce dépôt dont aucune famille n'est masquée. La
précédente en perdait trois sur quatre : `set -e` tuait le sous-shell avant
`echo code=$?`, `wait || true` effaçait le statut, et `$?` après un pipeline
rendait celui de `sed`. Les trois défauts sont corrigés ici.

## Le résultat

| famille | `r` | masse q4 ouverte | pentes `sum E_4` | `max E_4` |
| :--- | ---: | :--- | :--- | ---: |
| `uniform` | `0` | `12,94 → 2,11 %` | `1,236 / 1,012 / 1,137` | `1 531` |
| `uniform` | `4` | `5,44 → 0,80 %` | `1,099 / 1,075 / 1,058` | `449` |
| `terrain` | `0` | `7,71 → 2,83 %` | `1,402 / 1,535 / 1,617` | `10 130` |
| `terrain` | `4` | `2,37 → 0,67 %` | `1,296 / 1,344 / 1,537` | `5 076` |
| `eight_clusters` | `0` | `83,73 → 68,21 %` | `1,908 / 1,900 / 1,896` | `41 484` |
| `eight_clusters` | `4` | `51,15 → 42,07 %` | `1,898 / 1,909 / 1,911` | `31 151` |

Sur `eight_clusters`, le raffinement change la constante et pas l'exposant : le
résiduel passe de `852,6` à `525,9` millions d'arêtes candidates, facteur
`1,62`, pendant que le front passe de `20,3` à `31,9` millions de records,
facteur `1,57`. **Il achète la masse au prix qu'il coûte.**

Les deux familles `scanline` ont des résiduels de `0,5 %` à `12 %` et des pentes
non monotones ; elles ne sont reçues ni vertes ni rouges.

## Limites explicites

Une graine, une répétition, aucun p95, aucun temps qualifiable. Les pentes
portent sur `sum E_4` — le nombre d'arêtes candidates du certificateur central
sous hypothèse d'arête maximale — pas sur un travail, pas sur une sortie, pas
sur `M`. La profondeur de raffinement `4` n'est pas un optimum. Le verdict est
dans [`../../audits/NOTE_CLAUDE_CRITERE_DE_MORT_ETAPE_1_20260813.md`](../../audits/NOTE_CLAUDE_CRITERE_DE_MORT_ETAPE_1_20260813.md).
