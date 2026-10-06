# Cache des variantes et portée des reçus GPU — 6 octobre 2026

Relecture au commit publié `5861c223f31b5d7d6f621522d0ca84d0064c9904`, dans le cadre `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Aucun build, calcul natif, accès cloud ou nouvelle campagne par l’auditeur ; aucune donnée LiDAR, sortie binaire ou journal brut copié ici.

## Constat reproductible et correction proposée

Le juge `bench/gpu_ab.py` réemploie une extraction lorsque `src_<variante>/morsehgp3D_v11` existe (ligne 160) et un binaire lorsque `b_cuda_<variante>/mhgp11_full_bench` existe (ligne 97). Il publie pourtant le SHA de l’archive courante (lignes 164–165). Une archive remplacée sous le même nom peut ainsi être attribuée à l’ancienne source et à l’ancien binaire.

[Le rejeu autonome](cache/replay.py) appelle le véritable `main()` du juge épinglé `05db6f5b78d43a2657e519bc02841738d4dd02c1`, avec toutes les frontières de processus remplacées par des doubles stdlib. Il produit deux verdicts `conforme` : après remplacement de l’archive C, le SHA déclaré change, la source extraite reste OLD et le SHA du binaire reste identique. Les 24 appels sont simulés ; ce témoin ne prouve aucun résultat géométrique natif. [Le résumé](cache/summary.json) conserve les empreintes. Les deux sources du juge sont identiques aux fichiers publiés en `5861`, comparaison vérifiée par le second rejeu.

[Le patch minimal proposé](cache/fix_proposal.patch), non appliqué au développeur, ajoute le SHA courant aux deux espaces de cache : extraction et build. Il corrige ce remplacement d’archive sous même nom ; il ne certifie pas la voie `--src`/`new`, ni une modification manuelle des répertoires de cache. Pour les futurs plans seulement, un sanitizer AC qui vise actuellement `b_cuda_AC/mhgp11_full_bench` devra viser `b_cuda_AC_<SHA de AC_src.tar.gz>/mhgp11_full_bench`. Les archives, plans et reçus historiques restent immuables.

## Trois chaînes fermées vérifiées

[Le résumé de lecture](receipts/summary.json) et [son rejeu](receipts/replay.py) vérifient les 36 pièces publiées et leurs `SHA256SUMS`, leur identité avec les sessions, les archives de résultats et manifestes, les plans/paquets, les générations et arrêts ciblés certifiés. Pour chaque paquet, 637 fichiers utiles sont comparés exactement au Git du commit indiqué, sans les docs ni reçus historiques.

| Session | Source | État conservé | Arrêt certifié UTC | Pièces publiées |
|---|---|---|---|---:|
| `claudereservoir2` | `59509bbc8` | `failed_remote`, worker 1, DONE contrôleur 3 | 13:55:00.691 | 9 |
| `claudereservoir3` | `79fa5e9f7` | `completed`, worker 0, DONE 0 | 14:20:11.813 | 7 |
| `claudewfgpu1` | `05db6f5b7` | `completed`, worker 0, DONE 0 | 16:48:28.914 | 20 |

Les trois commandes de reservoir2 ont terminé avec code 0 ; l’échec de session conserve l’éviction explicite d’un dump sanitizer de 272 195 386 octets lors de l’export. Il n’est pas transformé en session réussie. Aucun membre tronqué ou ignoré n’est accepté par le rejeu. Les résultats GPU conformes ont tous les processus et passes attendus :

| Session | K / feuilles | Appels FULL | Hashes conservés |
|---|---|---:|---:|
| reservoir2 | 5 / 16 ; 10 / 24 | 153 ; 99 | 54 ; 36 |
| reservoir3 | 5 / 16 ; 10 / 24 ; 5 / 24 | 153 ; 99 ; 66 | 54 ; 36 ; 24 |
| wfgpu1 | 5 / 16 ; 10 / 24 ; 5 / 24 | 195 ; 120 ; 195 | 60 ; 45 ; 60 |

Les sorties froides et dernières chaudes concordent avec les six empreintes canoniques K5/K10 des trois trames. Les passes chaudes intermédiaires ont un statut de succès conservé ; aucun dump intermédiaire ni contrôle de registre intermédiaire n’est déduit de ce statut. Pour wfgpu1, les cinq variantes sont GPU : les références CPU proviennent de reservoir3 et sont reprises explicitement par leurs empreintes.

## C intégrée et AC instrumentée

Les quatre archives `sync/A/C/AC` locales ont les mêmes tailles et SHA que les données uploadées déclarées et que les trois rapports wfgpu1. Le premier rapport contient cinq constructions réussies, sans réemploi initial ; les suivants conservent les mêmes empreintes d’archives et des cinq binaires. **Aucune fausse attribution n’est établie dans ces sessions réelles.**

Les 165 fichiers de `src/` de la variante C sont identiques octet pour octet au produit intégré en `5861`. Les nouvelles portes et les mutants ajoutés à l’intégration ne sont pas des commandes de cette session. Les différentiels sur les trames qualifient cette variante GPU u21 dans leur périmètre ; aucun PASS CTest, mutant, u18 ou u24 n’est transféré.

Reservoir2 et wfgpu1 conservent chacun 12 appels ordinaires conformes, avec registres présents et égaux, puis cinq couples instrumentés : memcheck A/B, racecheck A/B et synccheck A. Les codes sont nuls et les marqueurs de sortie des outils sont propres. Pour wfgpu1, le binaire instrumenté est **AC**, alors que la variante intégrée est **C**. Aucun résultat sanitizer AC n’est transféré à C. Le défaut indépendant du juge sanitizer concernant le registre absent reste une question de source ; les registres effectivement conservés dans ces reçus sont présents. Les registres et `exit.status` des passes instrumentées ne sont pas conservés par ce rapport.

Cette lecture ne recalcule aucune statistique de temps et ne déclare pas le contrat de 100 ms acquis.

## Rejeu

Depuis ce dossier :

```sh
sha256sum -c SHA256SUMS
python3 -B cache/replay.py
python3 -B -O cache/replay.py
python3 -B receipts/replay.py
python3 -B -O receipts/replay.py
```

Les deux modes ont produit des résumés identiques lors de la fermeture. Le rejeu du cache est autonome avec les deux snapshots Python. Le rejeu des reçus dépend du Git local contenant les commits indiqués, des trois sessions fermées sous `/workspaces/.ehgp-sessions`, et des quatre archives de variantes dans `build/v11-persist/wf_gpu/data_g4` ; options de chemins disponibles dans `--help`. Cette capsule ne contient pas ces archives volumineuses et n’est donc pas un reçu natif autonome.
