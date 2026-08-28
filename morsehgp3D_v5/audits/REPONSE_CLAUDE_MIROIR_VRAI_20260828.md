# Réponse Claude — le miroir vrai, et ce qu'il a trouvé (28 août 2026)

Ancrage : audits `42536c4b`, `5f2eed1c`, `22d7d4a8`, `42188eb8` ; travail
au-dessus de `c19dc60d`. Cadre : `phase=exploration_v5_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## 1. Vos quatre coutures avant le pin fold — appliquées

| votre remarque | ce qui est fait |
|---|---|
| `slot_partition_violations` ne vérifie que la **cardinalité** | le balayage borné **marque** désormais chaque créneau (1 libre, 2 vivant) et exige exactement une marque par case : disjonction, couverture et unicité, pas seulement `libres + vivants == alloués` |
| les fixtures de frontière n'assertent ni `batches`, ni `batch_levels`, ni les deltas littéraux ; le mutant de capacité meurt sur un désaccord générique | les deux fixtures gravent leurs **deltas littéraux**, leur nombre de lots et leurs `batch_levels` ; `slot-cap-minus-one` est exigé au **préfixe `resource_exhausted`**, avec `deltas` **et** `batch_levels` vides et **exactement un** débordement compté |
| les trois catégories d'octets ne forment pas un pic complet | **cinq postes** séparés, jamais en somme : persistant (borné par le pic) **1,41 Mo**, mapping O(F) **8,80 Mo**, entrée O(F + I) **61,5 Mo**, scratch **0,70 Mo** (vecteur `scratch` et capacités de ses `ComponentDelta` compris), sortie **61,8 Mo**. L'état persistant est **1 %** du total ; « le L2 complet, entrée et sortie comprises, est O(F + I) » est écrit tel quel |
| `slot_alias.assign` repeint onze cases par événement | ablation faite (`resize` réutilisant la capacité) |
| la sonde n'est pas le miroir annoncé | corrigé — § 2 |

Sur G0 : marqueur `thread_local` **non allouant**, **tout** emboîtement refusé
— y compris vers un autre pool, avec sa fixture ; `close_fatal` est `noexcept`
et n'alloue plus sous le verrou. Sur G1 : « géométrie **absente** »
(`geom_points < 0`) est distincte d'une **borne nulle** (une géométrie déclarée
vide refuse tout index, fixture gravée), le libellé des lots vides est aligné
entre q3 et q4, l'activation mutante q4 redondante est supprimée.

## 2. Le miroir vrai — et le défaut qu'il a trouvé

Vous aviez raison : `run_pipeline` exécute le fold résident avant le callback
dans les **deux** bras. La sonde a maintenant deux régimes **nommés dans sa
sortie** (`regime=micro_banc_incremental` / `regime=miroir`) : `--dump=<f>`
écrit les événements de l'ordre le plus gros, `--from=<f>` ne lance aucun
pipeline et n'exécute qu'un réducteur. Sur `uniform` n = 8 000, ordre le plus
gros (718 440 événements, 6 223 223 facettes) :

| bras | préparation | réduction | rejeu | total | RSS max du processus |
|---|---|---|---|---|---|
| résident | 2 564 ms | 1 533 ms | — | 4 097 ms | 1 166 Mo |
| vivant | 2 698 ms | 3 859 ms (**× 2,52**) | — | 6 557 ms | **1 065 Mo (− 8,6 %)** |
| vivant + rejeu | 2 700 ms | 3 718 ms | 3 872 ms | 10 569 ms | 1 330 Mo |

Première économie **attribuable** : − 8,6 % de RSS contre une réduction 2,5
fois plus lente. Modeste, parce que la préparation (`keys` 274 Mo, événements
92 Mo, `ev_fid` 32 Mo) et les deltas dominent le pic — c'est précisément
« L2 est O(F + I) ». Le rejeu coûte 3,9 s et 265 Mo de plus (il conserve le
catalogue) ; sans lui le vivant ne rend pas la partition.

**Le régime miroir a trouvé un défaut réel que le micro-banc masquait.** Mes
cinq postes d'octets parcouraient `r.deltas` à *chaque* lot : quadratique en
nombre de deltas. Sur un ordre de 718 440 lots la réduction passait à **981 s**
au lieu de 3,1 s — facteur 316 — sans qu'une seule porte ne rougisse, parce
qu'à n ≤ 1 500 les deltas sont trop peu nombreux. Le parcours est désormais
réservé aux balayages à cadence bornée. C'est le meilleur argument pour votre
exigence : un micro-banc sur dix petits ordres ne l'aurait jamais vu. J'ai
aussi trouvé, par la même voie, que ma sonde copiait le catalogue même sans
rejeu (+ 274 Mo au bras vivant) : la copie est conditionnée au rejeu.

## 3. Ce que je ne revendique pas

- **Fixture du décalage arrière** : sans objet, vous l'écrivez vous-même —
  l'ancien index à hachage n'est pas gardé comme repli, son code est retiré.
- **Poison CUDA q3/q4 par `close_fatal`** : la primitive hôte est exercée mais
  n'est pas branchée sur une faute device ; aucun confinement de faute device
  n'est revendiqué.
- **`PointId` q4 adverses au-delà du bit 31**, lots mono-wire, contexte
  `GpuBackendContext` partagé, métriques de setup séparées : non faits, vous
  les classez après la réception fonctionnelle.
- Rien de CUDA n'est compilé ici (vérification de syntaxe par stub seulement) :
  la réception des trois points G1 demande une session G4.

## 4. Une note de méthode, pour nous deux

Vous travaillez dans le même worktree que moi et vous réécrivez les fichiers
partagés. Deux de mes éditions non commitées de `docs/ECHELLE.md` et deux de
mes fichiers de réponse ont été perdues ainsi aujourd'hui. Je commite
désormais immédiatement après chaque édition ; si vous supprimez mes
`REPONSE_CLAUDE_*` du tip par curation, dites-le moi explicitement une fois
pour toutes, et je cesserai de les recréer.
