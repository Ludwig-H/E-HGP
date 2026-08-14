# Reçu — première exécution CUDA de la ligne v3, noyau `Q4SeedAxisTopR4`

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cuda_g4`,
`profile=quantized_u16_input_only`,
`mode=debit_de_kernel_et_parite`,
`public_status=not_claimed`.

Zone `europe-west4-ai1a`, `ehgp-blackwell-spot-ai1a`, `g4-standard-48` SPOT,
RTX PRO 6000 Blackwell. Arrêt certifié `TERMINATED`.

> **Périmètre.** Ce reçu mesure le **débit d'un kernel** et la **parité** de ses
> verdicts. Il ne mesure ni la chaîne complète, ni un `warm_e2e`, ni un SLO, ni
> le contrat. Les listes de sites sont **construites sur l'hôte** et
> transférées ; le transfert n'est pas inclus dans le temps du kernel et n'est
> pas soustrait non plus. Un débit de kernel n'est pas un débit de bout en bout.

## 1. La parité, d'abord

Le kernel n'implémente aucune géométrie : il appelle `evaluate_seed`, la
fonction que l'hôte compile. Chaque champ de chaque verdict est comparé —
verdict, `p`, `k`, profondeur minorée, les deux listes de racines.

**Zéro écart sur 14 787 889 verdicts**, six configurations. La parité est donc
exacte, et c'est la condition sans laquelle le débit ne voudrait rien dire.

## 2. Le débit

| famille | `smax` | `n` | seeds | sites | hôte (1 cœur) | device | accélération |
|---|---:|---:|---:|---:|---:|---:|---:|
| `uniform` | 6 | 1 500 | 628 990 | 59,9 M | 2 160 ms | **12,0 ms** | `180×` |
| `uniform` | 6 | 3 000 | 1 384 420 | 135,9 M | 4 819 ms | **26,9 ms** | `179×` |
| `uniform` | 6 | 6 000 | 2 956 531 | 293,6 M | 10 399 ms | **55,3 ms** | `188×` |
| `uniform` | 11 | 6 000 | 3 000 000 (cap) | 589,0 M | 21 533 ms | **86,4 ms** | `249×` |
| `eight_clusters` | 6 | 1 500 | 917 948 | 600,0 M (cap) | 12 177 ms | **42,9 ms** | `284×` |

Débit device stable : `53,4 Mseeds/s` et `5 307 Msites/s` sur `uniform` à
`smax=6`, jusqu'à `6 817 Msites/s` à `smax=11` et `13 980 Msites/s` sur les
amas, où les listes sont plus longues et la latence mieux masquée.

L'accélération contre **un** cœur vaut `180` à `284`. Contre les quarante-huit
cœurs de la machine, elle vaut donc environ `4` à `6` — chiffre qu'il faut citer
en même temps, sans quoi la comparaison flatte le GPU.

## 3. Ce que cela dit du contrat, et rien de plus

À `n=6000`, `smax=6`, la source demande `2 956 531` `Q4Seed3` et `293,6 M`
sites. Les seeds croissent à peu près linéairement en `n` ; à `50 000` points
cela donne environ `24,6 M` seeds et `2,45 G` sites.

Au débit mesuré, **l'étage de sélection seul coûterait environ `460 ms`** —
`24,6/53,4` en seeds, `2450/5307` en sites, les deux estimations coïncidant.

| cible | verdict sur ce seul étage |
|---|---|
| `p95 < 1 s`, `K=5` | **plausible** — il reste `540 ms` pour tout le reste |
| `p95 < 100 ms`, `K=5` | **hors d'atteinte sans la descente** — `4,6×` de trop |
| `K=10` | environ `750 ms` pour ce seul étage : `100 ms` est exclu |

Le levier restant est nommé et il est algorithmique, pas matériel : le kernel
lit aujourd'hui **tous** les sites du `Q4Seed3` — `99` en moyenne à `smax=6`,
`196` à `smax=11`. La recherche best-first sur l'octree, où `A_z` est convexe
séparable et `B_z` linéaire, ramènerait ce nombre à l'ordre de `k log n`. Un
facteur `5` à `10` sur le terme dominant, et alors `100 ms` redevient discutable.

## 4. Ce qui n'est pas mesuré, et doit l'être avant tout claim

1. le **transfert** : `2,45 G` sites font `9,8 Go` à `50 000` points. Le lot ne
   doit pas être construit sur l'hôte dans une chaîne réelle ; la collecte des
   sites doit vivre sur le device ;
2. les lanes `q2` et `q3`, la phase d'ancre, et tout l'aval ;
3. `terrain` et les familles `scanline`, absentes de ce reçu ;
4. `p95` sur trente nuages frais, le seul chiffre que le contrat accepte.

## 5. Défauts de recette relevés sur cette session

La session a compilé, mesuré, puis est morte en `rc=127` à l'étape de reçu ; le
brut a été rapatrié par une session de récupération dédiée, le disque de
démarrage persistant à l'arrêt. Et l'échappement de `${PIPESTATUS[0]}` a produit
un littéral `code=\0` au lieu du code numérique : le verdict automatique ne peut
donc pas lire les codes de cette exécution. Les deux sont à corriger avant la
prochaine.
