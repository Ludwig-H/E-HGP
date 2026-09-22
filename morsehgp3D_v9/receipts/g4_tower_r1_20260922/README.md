# Reçu : première session G4 de la tour FULL v9

22 septembre 2026, 23:26–23:44 UTC. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : une session SPOT gardée sur la
cible fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48, AMD EPYC 9B45, 48 fils),
génération `2026-09-22T16:27:08.673-07:00`, arrêt ciblé certifié
(`host/guarded_stop.redacted.stdout`, `targeted_shutdown_certified: true`) et
état `TERMINATED` relu après la session. GPU non utilisé.

Protocole : `gcp-migration/tower_{session,worker,snapshot}_v9.py`, paquet
construit depuis le commit **`e28296bb`** (`PACKAGE.json`, snapshot
`e766319e…`, manifeste `afae8980…`), compilé sur la VM par GCC 11.4 en Release
strict (`-Werror`). Plan : huit cas sur les trois trames sans sol à 1 mm.

## Résultats

| cas | trame | K | fils | tour statique | total (s) | q3/q4 (s) | tour (s) | CPU·s | RSS (Go) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 000000 | 5 | 48 | 0 | 18,81 | 11,12 | 6,74 | 545 | 1,13 |
| 1 | 000100 | 5 | 48 | 0 | 15,05 | 9,11 | 5,30 | 439 | 0,96 |
| 2 | 000200 | 5 | 48 | 0 | 29,25 | 21,33 | 6,89 | 960 | 1,21 |
| 3 | 000000 | 10 | 48 | 0 | 111,68 | 32,72 | 75,90 | 1 663 | 4,88 |
| 4 | 000100 | 10 | 48 | 0 | 82,31 | 24,40 | 55,67 | 1 238 | 3,90 |
| 5 | 000200 | 10 | 48 | 0 | 125,44 | 58,03 | 64,28 | 2 788 | 4,91 |
| 6 | 000000 | 5 | 24 | 0 | 21,92 | 13,88 | 7,03 | 344 | 1,14 |
| 7 | 000000 | 10 | 48 | 48 | 70,00 | 32,76 | 34,14 | 1 681 | 4,76 |

Les huit cas sont `complete_relative`. Les condensés de tour sont identiques
entre fils (cas 0 et 6, cas 3 et 7) et identiques aux lignes locales du
[reçu local](../first_tower_20260922/README.md) sur les mêmes trames
(`67450c64611075b1` à K5, `ac108f7f71096c3f` à K10 pour 000000) : la sortie ne
dépend ni de la machine, ni du nombre de fils, ni de la voie de résolution.

## Lecture

- Le générateur q3/q4 tient sur 48 fils : 11,1 s contre 128,5 s sur huit fils
  locaux pour 000000 à K5 (×11,6).
- À K10, la tour en un fil domine (56 à 76 s) ; la voie statique à 48 fils la
  ramène à 34 s sur 000000 (même condensé).
- Écart au contrat d'une seconde : ×15 à ×29 à K5, ×70 à ×125 à K10. Ce reçu
  mesure le premier moteur ; il ne qualifie aucun contrat, et une seule
  répétition par cas.
- Ce paquet précède le noyau MEB « première paire maximale » (commit suivant),
  qui réduit la tour K10 de 130 à 109 s en local.

## Contenu

- `vm/` : sorties du worker sur la VM (sondes JSON, `/usr/bin/time -v`,
  compilateur, lscpu, preuves de garde, manifestes de sources avant/après,
  reçu du worker).
- `host/` : enregistrements des commandes du contrôleur (argv, codes, empreintes
  de sortie), reçu hôte, marques de garde, journaux de démarrage et d'arrêt
  **expurgés** (adresse du compte masquée). Les sorties de `oslogin_add`, la
  clé SSH éphémère et l'archive du paquet ne sont pas versionnées (données
  personnelles ; paquet reconstructible depuis `e28296bb`).
- `SUMMARY.json`, `SHA256SUMS`.
