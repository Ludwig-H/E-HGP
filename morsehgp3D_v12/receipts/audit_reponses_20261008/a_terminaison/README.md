# A : cycle de non-terminaison après épuisement du travail

8 octobre 2026, Codex ; source **86d7e39d834cae86768d49a23d83bf53454ec2a3**, `src/tower/pipeline_run.cpp:431–451`. Un entrelacement **séquentiellement cohérent** autorise deux fils à se réannoncer indéfiniment après la fin de tous les travaux. Les deux fils exécutent leurs instructions ; aucun appel métier n’est infini. C’est un défaut de garantie de terminaison, **sans incident natif observé**. Il ne diagnostique pas le timeout MES-M0, dont la chaîne n’appelle pas cette région A.

## Cycle exact

Chaque scan ne trouve plus de travail ; epoch est constant. A attend, B est annoncé et va se retirer. B décrémente in_flight de 1 à 0. A observe ce zéro dans la boucle d’attente et recommence : il annonce 1 avant le test de sortie de B. B relit donc 1, son dry-scan échoue, et il attend. A échoue à son tour, se retire à zéro ; B observe zéro et se réannonce avant le test de sortie de A. La situation est inversée, puis identique après la seconde moitié.

`results.json` donne un préfixe atteignable et la période exacte de **16 transitions**. Aucun passage dans `relax` : idle ne croît pas, ni aucune autre variable du cycle. La répétition est équitable **pour les fils** : chacun accomplit huit transitions par période. Il ne s’agit pas de supposer qu’un fil ne sera jamais planifié. Les scans sont négatifs même sans contention sur des tâches ; les ordonner plus fortement en mémoire ne supprime donc pas ce cycle SC.

Le point perdu est le zéro du retrait : `fetch_sub` le produit, mais sa valeur retournée est ignorée ; la lecture suivante peut déjà voir l’annonce du pair. L’ancienne phrase « plus rien ne peut devenir réclamable » était aussi trop forte : une sortie individuelle ne certifie pas une quiescence globale.

## Proposition minimale, non appliquée

Sur le seul chemin où le scan de réclamation a échoué :

```cpp
const bool last = p.in_flight.fetch_sub(1, std::memory_order_acq_rel) == 1;
const u64 seen = p.epoch.load(std::memory_order_acquire);
if (last && !find_job(p, false, job)) return {};
```

Le dry-scan est conservé. Le zéro linéarisé au retrait ne peut plus être effacé par l’annonce d’un autre fil. Une nouvelle prise pendant ce scan peut rendre le scan vide ; son propriétaire conserve alors la responsabilité de finir et publier ce travail avant son retrait. `proposition.patch` change ces deux lignes et l’explication de tête seulement. Application et inversion isolées vérifiées, aucun produit modifié.

La sûreté de publication s’appuie sur un argument distinct du modèle : les RMW acq_rel du même compteur lisent leur prédécesseur dans l’ordre de modification et transmettent les publications antérieures au retrait. Un acquire suivant son propre retrait ne peut lire un zéro plus ancien que celui-ci. Une nouvelle annonce entre zéro et scan interdit de conclure à une quiescence globale, mais ne permet pas d’abandonner le travail de cet autre participant. Contre-lecture statique indépendante par latest_perf_e : cycle et correction confirmés ; aucune exécution de sa part.

Sous jobs finis, dépendances valides et progrès des fils, une fois les dernières publications achevées, epoch est stable. Les fils retirés sans être derniers attendent tant que le compte est positif. Un dernier retrait atteint donc zéro. Avec la correction, son auteur peut sortir malgré une nouvelle annonce concurrente ; le nombre de participants non sortis diminue. Les quelques callbacks Pool qui démarrent plus tard restent des participants finis supplémentaires, pas une source de nouveaux travaux métier. Aucun délai maximal ni équité de l’OS n’est prouvé.

## Modèle et limites

Deux fils, six graphes abstraits, 0–3 tâches : coupe déjà vide, tâche unique, chaîne, fourche, jonction, refus sans publication d’epoch. L’annonce, le scan élément par élément, la prise, la publication, epoch, le retrait, les deux dry-scans et les deux lectures d’attente sont séparés. Chaque graphe fini est exploré exhaustivement ; une SCC non terminale est dite équitable lorsque chacun de ses fils non sortis peut y avancer indéfiniment. Les publications et tâches terminées sont monotones.

**Livré : une SCC équitable non terminale dans chacun des six cas. Proposition : aucune.** Aucune fin globale avec tâche abandonnée dans les deux variantes. Le refus laisse ses descendants bloqués légitimement ; cela ne vaut pas réussite métier. Le modèle n’exécute ni StepDeps réel, ni arithmétique, ni callbacks natifs, ni CAS faible, ni modèle mémoire C++ faible. Il abstrait l’horloge/backoff ; la trace causale publiée évite complètement ce backoff. Le contrôle du [graphe et des durées de vie](../t2d_a_dependances/README.md) reste une preuve séparée.

```sh
python check.py /chemin/depot
python -O check.py /chemin/depot
```

Les sorties sont égales à `results.json` ; cinq blobs Git sont épinglés. Aucun natif, TSan, GPU/GCP ou mesure. Au développeur : intégrer puis graver une porte contrôlant cet entrelacement de retrait/réannonce ; une campagne chronométrée réussie ne remplace pas ce témoin de terminaison.
