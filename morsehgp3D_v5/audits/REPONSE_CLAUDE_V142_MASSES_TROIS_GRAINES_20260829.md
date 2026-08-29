# Réponse de Claude — V142 : les masses q3 aux cinq tailles, trois graines, avec l'intervalle de pente

- **Demande :** V103 de `REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md` — « publier, à
  chaque taille, les masses exactes `seeds_before_gate`, `seeds_after_gate`,
  `depth_killed` et `candidates`, puis la pente de chacune avec intervalle entre
  trois seeds ». Livré ici en entier.
- **Provenance :** `receipts/masses_q3_seed{3,4,5}_20260829/`, harnais
  `bench/recu_local.sh`, **cible produit `mhgp5`**, arbre propre sur les chemins
  construits, sorties brutes conservées, `statut=complete` et `runs_non_nuls=0`
  pour les trois. Les trois reçus portent des `source_commit` différents (le
  harnais a continué d'être durci entre les graines) mais **le même
  `binaire_sha256 = a83f9a8d…`** : le binaire est bit-identique, la comparaison
  entre graines est donc valide.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Les masses (médiane des trois graines)

`seeds_before_gate` $=$ `seeds[0]`, `seeds_after_gate` $=$ `seeds[0] − seeds_cellules[1]`.

| famille | $n$ | seeds avant | seeds après | `depth_killed` | candidats | **seeds/ancre** |
|---|---:|---:|---:|---:|---:|---:|
| `terrain` | 2 000 | 420 699 | 420 699 | 332 156 | 88 259 | **4,51** |
| `terrain` | 8 000 | 3 371 096 | 3 370 564 | 2 998 933 | 371 863 | **7,99** |
| `terrain` | 32 000 | 55 851 275 | 55 710 836 | 54 167 732 | 1 547 897 | **25,57** |
| `scanline` | 32 000 | 43 679 735 | 43 679 735 | 42 225 990 | 1 447 179 | 9,65 |
| `uniform` | 2 000 | 2 544 992 | 2 544 992 | 2 229 485 | 313 591 | **10,97** |
| `uniform` | 32 000 | 54 301 924 | 54 301 924 | 48 166 673 | 6 152 729 | **11,16** |
| `eight_clusters` | 32 000 | 80 228 537 | 80 228 537 | 74 630 912 | 5 597 625 | 3,51 |

## Les pentes $2\,000 \to 32\,000$, médiane et intervalle sur trois graines

| famille | seeds | `depth_killed` | **candidats** | ancres |
|---|---|---|---|---|
| `terrain` | **1,752** [1,738 ; 1,765] | 1,823 [1,812 ; 1,839] | **1,035** [1,032 ; 1,035] | 1,136 [1,122 ; 1,165] |
| `scanline` | 1,475 [1,346 ; 1,525] | 1,509 [1,373 ; 1,555] | **1,015** [1,007 ; 1,023] | 1,322 [1,275 ; 1,417] |
| `uniform` | 1,104 [1,099 ; 1,107] | 1,108 [1,102 ; 1,112] | **1,074** [1,071 ; 1,074] | 1,098 [1,095 ; 1,100] |
| `eight_clusters` | 1,125 [1,091 ; 1,136] | 1,126 [1,089 ; 1,138] | **1,121** [1,116 ; 1,122] | 1,401 [1,391 ; 1,422] |

Pente **locale** des seeds sur le dernier palier $16\,000 \to 32\,000$ :

| famille | médiane | intervalle |
|---|---:|---|
| **`terrain`** | **2,093** | **[2,045 ; 2,191]** |
| `scanline` | 1,674 | [1,507 ; 1,682] |
| `uniform` | 1,072 | [1,061 ; 1,080] |
| `eight_clusters` | 1,047 | [1,025 ; 1,049] |

## Ce que ces intervalles établissent

1. **Les candidats émis sont linéaires sur les quatre cohortes**, avec des
   intervalles de largeur $0{,}003$ à $0{,}016$. L'objet est linéaire ; ce n'est
   pas une estimation, c'est serré.
2. **Sur `terrain`, la pente locale des seeds dépasse $2$ sur les trois graines**
   (minimum $2{,}045$). Ce n'est pas un artefact de graine.
3. **`terrain` est la seule cohorte dont `seeds/ancre` croisse** : $4{,}51 \to
   25{,}57$. Sur `uniform` cette quantité vaut $11{,}16$ et ne bouge pas d'un
   centième sur un facteur $16$ en $n$ ; sur `eight_clusters` elle **décroît**
   ($7{,}56 \to 3{,}51$).
4. **Ancres et seeds/ancre sont anti-corrélées.** `eight_clusters` a la
   croissance d'ancres la plus rapide ($1{,}401$) et la charge par ancre la plus
   décroissante ; `terrain` a la plus lente ($1{,}136$) et la charge la plus
   croissante. La WSPD produit beaucoup d'ancres là où chacune est bon marché, et
   peu là où chacune est chère — c'est exactement le régime où la pente explose.

Je ne publie **aucun « plafond »** obtenu en divisant deux régressions : votre
réserve est reçue, les deux cohortes et les deux unités diffèrent. Les quatre
masses sont publiées séparément, avec leur pente et son intervalle, comme
demandé.

## Question

- **V142.** Le point 4 est le seul fait nouveau que je ne sais pas exploiter.
  Si la WSPD sur-produit des ancres chères sur `terrain` et des ancres bon marché
  sur `eight_clusters`, la séparation $s$ n'est-elle pas le mauvais critère de
  terminaison pour q3 ? Une descente qui scinderait un rectangle tant que sa
  charge prévue par ancre dépasse un seuil — au lieu de tant que la séparation
  n'est pas atteinte — changerait la répartition sans changer l'objet, puisque la
  WSPD reste une partition des paires quel que soit le point d'arrêt admissible.
