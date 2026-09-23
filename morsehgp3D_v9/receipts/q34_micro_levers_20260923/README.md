# Reçu : profil q34 et cinq micro-leviers (aucun retenu)

23 septembre 2026. **GCP non utilisé.** Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

## Conditions communes

Toutes les mesures portent sur la même entrée, avec les mêmes réglages.

- **Entrée** : trame 08/000000 sans sol, entière, 39 885 sites, sha256
  `0baa4de1…` (`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le`).
- **Réglages** : K5, s = 8, W8, `--no-tower` (sauf les sorties `full_*`), tous les leviers de la sonde
  v16 ON.
- **Hôte** : local partagé à 8 cœurs. Le CPU q34 (`q34_occupancy.cpu_sum_s`)
  est le juge, le mur n'est qu'indicatif. Chaque comparaison est appariée,
  deux essais par variante en alternance.
- **Base** : `ed11c6c3` rebasé sur les audits jusqu'à `9be92674`, sans
  changement de code.

Les résumés `catalogue` de toutes les sorties de `out/` sont identiques
champ pour champ : nombre de clés, présentations par arité, histogrammes,
Euler. Ce sont des comptes, pas les clés une à une. Seules les sorties
`full_*` (tour comprise) portent le condensé FULL, identique entre la base
et la variante « pas +1 et marge ».

## Profil

Échantillonneur `SIGPROF` sur le temps CPU du processus, à 1 kHz, en
préchargement (`profile/sampler.c`). Symbolisation par `addr2line -i`
(`profile/lines.py`) sur un build Release avec `-g`. Le profil publié,
`profile/k5_after_counter_fastpath.txt`, est celui de la variante « pas +1 »
(les parts des autres postes n'en dépendent qu'à 4 % près) ; le binaire
symbolisé n'est pas versionné.

Fonctions réelles, part des échantillons :

| poste | part |
| --- | ---: |
| DFS témoin des paires (`filter_impl<true,true>`) | 15,8 % |
| certificat de voie morte (`cell`, `load`) | 12,3 % |
| DFS témoin des rectangles (`filter_impl<true,false>` + `xi_bounds`) | 12,0 % |
| q4 local (balayage, formes, fragments, racines, atlas) | environ 17 % |
| `census_q3_ball` | 6,0 % |
| `Q34EdgeCover::build` (cœurs et couvertures) | 5,4 % |
| ordre des enfants du DFS (`midpoint_distance16`) | 5,2 % |
| filtre du front WSPD (`Front::filter`) | 5,0 % |

Dans les deux DFS, le coût tombe sur les lignes qui lisent la boîte du
nœud : bornes h (ligne 122) et ξ (ligne 143). Aucun poste ne dépasse
16 %. Le filtrage (front, rectangles, paires) fait environ 40 % du CPU,
le cœur et le certificat 18 %, la génération q3/q4 environ 25 %.

## Compteurs : pas +1 sans contrôle, mesuré puis non retenu

Variante (`counter_step_headroom.patch`) :
- `counter_add(u64&)` (pas de 1) devient `++value`, `noexcept`. Les
  incréments arbitraires gardent le refus exact de dépassement.
- Trois entrées publiques exigent au moins 2^63 de marge sur chaque mot du
  ledger de l'appelant (`require_ledger_headroom`) :
  `filter_q34_witnesses`, `q34_cached_witness_rejections` et
  `census_q3_ball`.

Le profil de la base attribuait 8,9 % des échantillons aux lignes inlinées
de `counter_add`, dispersées sur des centaines de sites.

Sans tour, pas +1 seul :

| variante | essai | q2 (ms) | q34 (ms) | CPU q34 (s) |
| --- | --- | ---: | ---: | ---: |
| base | 1 | 1 324 | 21 260 | 130,50 |
| base | 2 | 1 291 | 21 870 | 130,68 |
| pas +1 | 1 | 996 | 18 436 | 124,74 |
| pas +1 | 2 | 1 110 | 19 054 | 125,00 |

Tour comprise, pas +1 et marge (binaire sha256 `9b567878…`) :

| variante | essai | condensé FULL | q34 (ms) | CPU q34 (s) | chaîne (ms) |
| --- | --- | --- | ---: | ---: | ---: |
| base | 1 | `67450c64611075b1` | 17 710 | 131,09 | 21 559 |
| pas +1 et marge | 1 | `67450c64611075b1` | 16 681 | 125,68 | 20 529 |
| base | 2 | `67450c64611075b1` | 19 006 | 130,46 | 23 222 |
| pas +1 et marge | 2 | `67450c64611075b1` | 17 157 | 125,67 | 21 044 |

Gain de 4,0 à 4,4 % du CPU q34, condensé FULL identique, et 141/141 CTests
une fois la marge ajoutée : sans elle, les deux portes de dépassement et
trois mutants qui en dépendent échouaient, comme B l'avait annoncé. B a
ensuite relevé trois défauts de cette variante.
- D'autres entrées publiques (`point_witness`, `universal_witness`,
  `classify_witness_block`, `Q34DeadLaneProver::load`/`prove`) passent au pas
  non contrôlé **sans** marge.
- La marge refuse aussi les champs fusionnés par maximum (piles de pointe),
  légaux au maximum.
- La lecture par `bit_cast` suppose un agrégat sans remplissage.

**Non retenu** : 4 % ne justifient pas de changer le contrat de
dépassement de toutes les API publiques hérité de la v8.

La version sûre (compteurs du seul DFS témoin en locaux partis de zéro,
puis ajout contrôlé au ledger de l'appelant, `dfs_local_counters.patch`
sur la base contrôlée) ne gagne que 0,9 % : 130,49/130,72 →
129,37/129,42 s (`nt_*`). Le gain venait du retrait des contrôles partout,
pas du seul DFS. Non retenu non plus.

Le même pas non contrôlé sur les compteurs de la tour (`full_detail::add`)
était resté sans effet mesurable.

## Leviers fermés

| essai | idée | mesure | verdict |
| --- | --- | --- | --- |
| `rect_refine.patch` | un rectangle qui échoue au filtre est raffiné en rectangles enfants tant que sa masse dépasse un seuil | seuil 64 : paires développées 23,7 → 7,4 M, mais visites du filtre de rectangle 230 → 453 M et rejets du cache 16,5 → 3,0 M ; CPU 124,9 → 140,0 s (seuil 16 : 154,4 s ; seuil 4 : 177,8 s) | fermé : le cache par `a` rejetait déjà ces paires pour presque rien |
| `dfs_local_counters.patch` sur le pas +1 | compteurs du DFS en variables locales, ordre des enfants inliné sans liste d'initialisation | CPU 124,74/125,00 → 125,08/125,05 s | fermé : sans effet |
| `b_cache.patch` | second cache de nœuds témoins par `b` du rectangle | recherches de paire 7,16 → 8,20 M (le cache par `b` évite la recherche qui aurait établi le cache de la ligne `a`) ; CPU 124,3/124,7 → 127,0/126,7 s | fermé : +2 % |

## Lecture

Le filtrage coûte environ 40 % du CPU q3/q4 et il est proportionnel au
nombre de visites de nœuds (666 M à K5). Le raffinement et le cache par
`b` ne font que déplacer ces visites. Les micro-réglages de comptabilité
valent au plus 4 %. Au moins trois gains plus lourds
restent candidats :
- une disposition des nœuds plus compacte (latence mémoire) ;
- le déport GPU des filtres ;
- la réfutation des ancres longues avant expansion.

## Contenu

- `out/` : sorties de la sonde. `base_*`, `full_base_*` et `nt_base_*` :
  binaire `-g` au commit de base. `cnt1_*` : pas +1 seul. `full_hr_*` :
  pas +1 et marge, tour comprise. `nt_dl_*` : compteurs locaux du DFS sur la
  base contrôlée. `wl_*`, `bc0_*`/`bc1_*` et `refine_*` : autres essais.
- Les quatre patchs.
- `profile/` : échantillonneur, script de symbolisation, profil et sortie
  de sonde associés.
- `SHA256SUMS`.
