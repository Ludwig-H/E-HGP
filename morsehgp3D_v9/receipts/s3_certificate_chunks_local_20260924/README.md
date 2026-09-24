# Reçu local : parcours et chargement par blocs du certificat (S3)

24 septembre 2026. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP non utilisé**, aucun GPU : l'hôte exécute
la copie portable (`HostGroup`) du noyau des certificats.

- **Commit** : `0c834dff` (`out/commit.txt`). Le script refuse des sources
  différentes de HEAD.
- **Machine** : 8 cœurs partagés (hôte chargé par d'autres travaux).
- **Provenance** : première exécution par l'agent au commit local `32379637`
  (non publié), rejouée par l'intégrateur au commit publié `0c834dff` : mêmes
  compteurs, seuls les murs changent.
- **Entrée** : trame sans sol 08/000000 de la v8, 39 885 sites. Les
  empreintes de l'entrée et des deux binaires sont dans
  `out/inputs.sha256`.
- **Commande** :
  `bash morsehgp3D_v9/receipts/s3_certificate_chunks_local_20260924/run.sh build/v9-cert morsehgp3D_v9/receipts/s3_certificate_chunks_local_20260924/out`

Conception et portes : [PROVENANCE](../../docs/PROVENANCE.md), section
« Parcours et chargement par blocs du certificat ».

## Chaque survivante, arête par arête

Mode fichier de `mhgp9_gpu_certificate_port_gate` (4 fils). Pour chaque
survivante du filtre : parcours par blocs contre `build_cover` sur la boule
du cœur et sur celle du cover (réponse, travail, plages), chargement par
fenêtres contre `load_forms` (prouveur, trois tableaux de formes),
`certify_edge` des deux chemins (statut, masque, travail), puis le prouveur
produit (masque, travail champ par champ).

| K | survivantes | fermées par le cœur | visites (avant) | blocs de décision | pas de rejeu | passes de chargement (avant → après) | mur (4 fils) |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 2 043 612 | 1 143 235 | 540 624 193 | 93 621 341 | 410 957 802 | 58 955 540 → 20 830 439 | 145 s |
| 10 | 4 507 278 | 2 572 879 | 1 394 025 439 | 228 671 376 | 1 059 220 070 | 161 704 731 → 59 128 851 | 265 s |

Sorties de bloc à K5 : 264 584 553 fils gauches et 182 418 299
échappements dans le bloc, 4 255 549 fils gauches au-delà de la dernière
voie, 89 361 741 échappements au-delà du bloc, 4 051 fins d'index. Formes
comparées : 3 323 158 682 à K5, 8 715 006 677 à K10.

Toutes identiques, aucune mise en attente. Les compteurs d'avant (visites,
une passe par plage) et d'après (blocs, pas de rejeu, fenêtres de 32 sites)
sont ceux de l'appel lui-même.

## Chaîne (lots CPU, S2 + S3 + S4a + S4b, W4)

| K | tour | catalogue | présentations | survivantes |
| ---: | --- | --- | --- | ---: |
| 5 | `67450c64611075b1` | `5ad1fe09354411ba` | `a2aa4b20ca392dfe` | 2 043 612 |
| 10 | `ac108f7f71096c3f` | `a6e959d227f3dafa` | `43ff64fb1c3846d9` | 4 507 278 |

La chaîne CPU n'exécute pas `certificate.hpp` (sa référence est le
prouveur produit) : ces condensés montrent que rien d'autre n'a bougé. Ils
égalent les épingles de l'auditeur C et le condensé des présentations des
reçus précédents.

## Ce que seul G4 peut montrer

La durée du noyau, l'égalité octet pour octet appareil/jumeau (mode
`--device` de la même porte) et le poids du trafic L2 des blocs. La
projection de la section PROVENANCE (52 à 71 ms à K5, 132 à 181 ms à K10)
n'est pas une mesure.

## Fichiers

`out/` : `commit.txt`, `nproc.txt`, `inputs.sha256`, `port_k{5,10}.txt` et
`.time`, `k{5,10}_chain.json`, `SUMMARY.json`, `SHA256SUMS`.
