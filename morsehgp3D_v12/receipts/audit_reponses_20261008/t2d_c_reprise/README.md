# T2-d-C : reprise statique après fusion des budgets

Capture **2026-10-08 à 04:45:42 UTC**, copie `repo8da`, base main `8da450ab7`, HEAD local
`656adfa4`. Les dix sources sont stables entre les deux lectures. `device_cuda.cu` et
`transfer_meter.hpp` sont encore non commis. Ce reçu ne qualifie aucune nouvelle compilation,
exécution CUDA ou mesure ; aucune campagne ni ancien témoin n'a été relancé.

La réservation épinglée du flux passe de 16 Mio imposés à `min(16 Mio, plus grand segment)`.
`stage` conserve son plancher de 1 Mio. Pour les mêmes segments de taille maximale M, la nouvelle
demande `max(1 Mio,min(16 Mio,M))` est donc ≤ l'ancienne `max(1 Mio,min(64 Mio,M))`.
Cela corrige la réserve de la [prélecture](../t2d_c_prelecture/README.md) sur le plancher forcé.
Ce n'est pas une borne du pic mémoire total : les sorties anticipées vivent toujours plus tôt.

`kAnticipateOutputs=false` coupe maintenant allocation **et** premier toucher anticipés :
dernier lot, secours sans lot final, puis niveaux avant Emit. Les allocations nécessaires restent
dans la prise finale des sorties. Le bras retire enfin tout le levier annoncé, et peut changer
le pic ou le point de refus sous budget serré.

La fusion conserve `grow → device_budget` avant `cudaMalloc` et `stage → budget` hôte avant
`cudaMallocHost`. Le transit épinglé reste compté côté hôte. Mais **la sonde catalogue appelle
`open(budget)`**, qui aliasse les deux budgets : ses pics restent communs. La sonde FULL sépare
les budgets seulement avec `--budget-appareil` ; sans cette option, elle les partage aussi.
Ne pas renommer ces pics en RSS hôte ou pic VRAM mesuré.

Le pilote `6293bd13` et le juge `e02aed6e` conservent les quatre AST d'admission déjà audités :
`parse`/`probe_run` de `cdde6f84`, `run_complete`/`check_mutant` de `14eb9c8a`.
Les empreintes AST et sources complètes figurent dans la capture. Les trous d'admission de
[t2d_c_admission](../t2d_c_admission/README.md) restent applicables, sans nouveau rejeu des
contre-exemples. Ne pas comparer `check_mutant` à `c8dd` : son garde des codes avait déjà changé
en `14eb` ; le cas code 0 sans empreinte reste insuffisamment protégé.

Deux noms de bras restent trompeurs : `flux_seul` garde la réparation compacte ;
`sans_repli_compact` garde la compaction des marqueurs et rapatrie toutes les clés **par chaîne**
retenue, puis l'ordre/les clés des seuls éléments réparés. Il ne restaure pas les trois tableaux
entiers copiés une fois dans l'ancien chemin. Ces bras ne permettent donc pas d'attribuer seuls
un gain au flux ou à toute la réparation compacte.

Les extraits causaux et pins sont conservés ; les sources complètes restent externes. Relecture
des copies épinglées (Git produit et sources vivantes non modifiés) :

```sh
python3 -B check.py --source COPIE_REPO8DA_V12 --replay SECOURS_REPLAY
python3 -B -O check.py --source COPIE_REPO8DA_V12 --replay SECOURS_REPLAY
```

Le lecteur vérifie hashes, quatre AST et extraits, sans exécuter pilote ni juge. Une source
différente est refusée ; aucun résultat historique n'est transféré à un corps ultérieur.
