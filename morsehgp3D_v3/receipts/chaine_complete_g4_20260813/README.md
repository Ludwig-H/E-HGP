# Reçu G4 du 13 août 2026 — la chaîne complète tient sur `uniform`, et sur elle seule

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Session `gcp-migration/session_chaine_complete_g4.sh`, scripts gardés,
`g4-standard-48` SPOT, `maxRunDuration=3600 s`, arrêt invité à `45 min`, arrêt
certifié `TERMINATED` par le trap. `CUDA_COMPILE=OK`, `23/23` portes rejouées.
Aucun débit GPU : quatre familles mono-processus à douze threads chacune.

**AVERTISSEMENT DE RÉCEPTION.** Le contre-audit `736f5bc` prononce un
**NO-GO lancement** sur cette session, au motif qu'elle chronomètre un
producteur déjà réfuté. Elle avait démarré avant la publication de cet audit ;
j'ai tenté de l'interrompre et le garde a refusé, à juste titre, faute de
correspondance de génération. Ses chiffres sont donc publiés comme
**diagnostic**, et ils confirment le NO-GO plutôt que de le contredire.

## 1. Le temps, et sa pente

| famille | `6 250` | `12 500` | `25 000` | `50 000` | pentes du TEMPS |
| --- | ---: | ---: | ---: | ---: | :---: |
| `uniform` | `7,0` s | `16,2` s | `37,3` s | **`78,8` s** | **`1,20 / 1,21 / 1,08`** |
| `eight_clusters` | `147,7` s | `503,5` s | — | — | `1,77` |
| `scanline_overlap_multiecho` | `21,2` s | `116,7` s | `551,9` s | — | `2,46 / 2,24` |
| `terrain` | `9,1` s | `82,7` s | `533,5` s | — | **`3,19 / 2,69`** |

**`uniform` tient** : `50 000` points en `78,8` secondes à douze threads, avec
des pentes de temps `1,20 / 1,21 / 1,08`, toutes sous le seuil.

**Les trois autres familles murent** : `n^{2,2}` à `n^{3,2}`. Aucune n'atteint
`50 000` dans le budget. C'est la réfutation du producteur, et elle est
mesurée ici — pas seulement prédite.

## 2. La sortie, et son identité

| famille | supports à la plus grande taille atteinte | identité |
| --- | ---: | :---: |
| `uniform` `50 000` | **`21 413 140`** | `occurrences = clés uniques`, `0` doublon |
| `scanline_overlap` `25 000` | `3 261 360` | `0` doublon |
| `terrain` `25 000` | `1 873 843` | `0` doublon |
| `eight_clusters` `12 500` | `4 370 704` | `0` doublon |

**L'identité tient partout** : à chaque taille et chaque famille, le nombre
d'occurrences émises égale le nombre de clés distinctes. Le census ne produit
aucun doublon.

Sur `uniform`, la sortie croît en `n^{1,05}` — `2,39` puis `5,01`, `10,37` et
`21,41` millions — donc **quasi linéairement**.

## 3. La fenêtre, et l'endroit où ma loi casse

Pentes de `\sum_a\lvert N\rvert`, `s=3`, `Central-VWave` :

| famille | pentes |
| --- | :---: |
| `eight_clusters` | **`1,858 / 1,887 / 1,931`** — REFUSÉ |
| `scanline_overlap_multiecho` | `1,008 / 1,018 / 1,095` |
| `terrain` | `1,151 / 1,261 / 1,419` |
| `uniform` | `1,031 / 1,289 / 1,052` |

**La fenêtre est quasi quadratique sur `eight_clusters`.** J'avais écrit une
heure plus tôt, avant de la mesurer : « `eight_clusters`, dont la densité est
très inhomogène, n'est pas mesurée — et c'est là que je m'attends le plus à ce
que la loi se casse ». La prédiction est confirmée, et la loi
`\lvert N\rvert = c(s)K` ne vaut donc **pas** hors régime homogène.

## 4. Ce que ce reçu décide, et ne décide pas

Il **décide** que le producteur actuel n'atteint le contrat sur aucune famille
sauf `uniform`, et que l'obstacle est la pente, non la constante.

Il **ne décide pas** le contrat : `78,8` secondes à douze threads sur `uniform`
ne sont pas une seconde, aucun kernel n'est mesuré, aucun octet ni high-water
n'est publié, et le fold vers les dix forêts n'est pas dans le chrono. G4 reste
NO-GO.
