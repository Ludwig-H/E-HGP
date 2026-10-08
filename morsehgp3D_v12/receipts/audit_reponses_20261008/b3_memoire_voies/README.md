# B3 : stockage et circulation des clés selon la voie

8 octobre 2026. Clarification du reçu immuable `b3_supports_math` : la copie supplémentaire des
clés depuis le GPU concerne **la fin d'étage complète**. La fin d'étage par tranches reconstruit
ces clés sur l'hôte. Source B3 `545ed987e0f5a06dbb518fe42ed6c5f100f32770`, base R1 `8a0716e74` ;
13 sources/pilotes épinglés dans `capture.json`. Aucun binaire lancé, construit ou modifié.

Soit **B le nombre de boules du catalogue terminé**, donc le nombre de cases de `table_.val`.
Il ne s'agit ni du nombre de sites, ni du nombre de niveaux. `TableKey` contient deux u64 et
occupe 16 octets dans ce profil ; le tableau hôte `Buffer<TableKey>` porte une clé par case.

| Chemin de finition | Nouvelle charge utile hôte des clés | Circulation supplémentaire propre aux clés |
| --- | ---: | --- |
| Complète, appareil | 16 × B octets | 16 × B octets D2H, depuis les clés déjà triées sur l'appareil |
| Par tranches, appareil | 16 × B octets | **Pas de D2H supplémentaire pour ces clés finales** ; construction sur l'hôte |
| Complète, CPU | 16 × B octets | Copie hôte de 16 × B octets, pas de PCIe |
| Par tranches, CPU si empruntée | 16 × B octets | Construction sur l'hôte, pas de PCIe |

La quantité **16 × B** ne signifie jamais 16 octets par site. Ce sont les
charges utiles logiques du tampon et du mouvement indiqué, pas un supplément identique de pic
mesuré, de RSS ou de mémoire réservée. Les classes du cache, les tampons simultanés et leur
réemploi interviennent dans le pic. Aucun temps de transfert ni nouveau plafond appareil n'en
est déduit. Les tranches conservent leurs transferts existants de boules/populations/niveaux.

## Raccord exact au code

- `morsehgp3D_v12/src/catalogue/catalogue.hpp` : `TableKey`, `table_keys_` ;
  `morsehgp3D_v12/src/catalogue/assemble.cpp` : adoption et égalité des tailles clés/cases.
- `morsehgp3D_v12/src/catalogue/finish_driver.hpp`, `finish_take` :
  `take_segment(... a.table.keys[ct], out.table_keys, in.balls ...)` ajoute un segment de sortie.
  Ces clés existaient déjà pour le tri de la table ; B3 ne crée pas un second tableau GPU de
  clés à cet endroit. Le tampon hôte est réservé puis rempli.
- `morsehgp3D_v12/src/catalogue/finish_slices.hpp`, `slice_take` : quatre segments, boules,
  décalages, population et mots de niveaux ; **aucune clé de table**. `slices_close` appelle
  ensuite `host_table(... out.table_keys ...)`.
- `morsehgp3D_v12/src/catalogue/slices.cpp`, `host_table` : les lignes de table sont triées
  sur l'hôte ; B3 alloue ensuite B clés et joue `TableKeys::body` pour les calculer dans cet ordre.
- `morsehgp3D_v12/src/catalogue/exec_host.hpp` : l'adoption sans copie exige le même type.
  Le tableau trié porte `Key2`, la sortie `TableKey` ; ces types distincts choisissent le repli
  `adopt(A&, Buffer<T>&, ...) = false`. La finition CPU complète copie donc ces 16 × B octets.
- `morsehgp3D_v12/src/core/buffer.hpp:70–73` : `buffer_acquire` réserve la taille physique,
  éventuellement celle d'une classe de cache. La formule de charge utile ne calcule pas le pic.

## Ce que mesure le bras nommé « transfert »

Les substitutions de `morsehgp3D_v12/microbancs/mes_t2d_b3/bras_t2d_b3.json` sont rejouées
**en mémoire Python**, depuis les objets Git R1 : chaque préimage, occurrence et postimage est
vérifiée. Les sept fichiers modifiés du bras `transfert` sont identiques à ceux de `cles` et
`apres`. Seule la modification de `src/catalogue/table.cpp` manque par rapport à `cles` :
la recherche directe reste désactivée, mais la conservation des clés est entièrement présente.

**Oui : sur la voie par tranches, ce bras paie aussi la reconstruction des clés sur l'hôte.**
Il mesure le coût de préparation et de conservation des clés inutilisées par la recherche,
selon la voie réellement empruntée. Ce n'est donc pas une ablation de PCIe pur dans tous les
régimes. Le protocole des cinq trames K5 ne démontre pas à lui seul un coût sur les scènes
massives ni la couverture de cette reconstruction ; son nom ne suffit pas à établir la voie.

## Pourquoi les résultats de capacité R1 ne qualifient pas B3

L'inventaire public des deux répertoires
`morsehgp3D_v12/receipts/g4_mesb1t_20261008/resultats/` et
`morsehgp3D_v12/receipts/g4_mesb2t_20261008/resultats/` contient 23 journaux JSONL et trois rapports,
avec 24 lignes FULL réussies. Ces métadonnées publient les sites, durées, mémoires et refus,
**pas B**. Le lecteur `bench/full_probe.cpp` épinglé confirme cette absence dans la ligne FULL.
La mémoire agrégée ne permet pas de reconstruire B : elle additionne plusieurs tableaux et le
cache. Aucun supplément par scène n'est calculé ici et aucun site n'est assimilé à une boule.

Le record R1 de Paris sans sol, les passages TUWIEN et leurs refus restent des observations de
leurs sources et budgets. B3 conserve un nouveau tampon pendant G et la tour et change soit les
copies, soit le travail de table sur l'hôte. L'export canonique MHGP12DP et FUL1 n'incluent pas
ce tampon : leur égalité atteste l'objet, **pas la même capacité ni le même coût**. Les anciennes
réussites ne qualifient donc pas automatiquement B3 ; réciproquement, rien ici ne prouve un nouveau
refus. Le seul pic GPU ne localise pas un refus de budget.

Pour une future admission massive, conserver B, la voie de finition effectivement empruntée,
les octets D2H et les étapes des mémoires/refus, avec les mêmes budgets, cache et réemplois.
Ce reçu est une fermeture de lecture du code et des métadonnées, pas une mesure de B3.

```sh
python -B check.py DEPOT_GIT
python -B -O check.py DEPOT_GIT
```
