# Analyse des campagnes front + census q2 multiworker

Ce dossier résume **134 captures closes** : 86 dans `../campaigns/` et 48 dans
`../physical_cores/`. Il ne contient aucune nouvelle mesure. Les reçus sources
restent l'autorité ; `SUMMARY.json` épingle les 27 fichiers des neuf triplets
`MANIFEST.json` / `MEASURES.jsonl` / `COMPLETION.json` effectivement lus.

Depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v8/receipts/q2_front_workers_20260914/analysis/analyze.py --root morsehgp3D_v8/receipts/q2_front_workers_20260914
python3 -O -B morsehgp3D_v8/receipts/q2_front_workers_20260914/analysis/analyze.py --root morsehgp3D_v8/receipts/q2_front_workers_20260914
```

La sortie standard est le JSON d'analyse. Code de sortie `0` : lecture stricte
réussie ; code `1` : rejet, avec motif JSON sur la sortie d'erreur. Le lecteur
de campagne existant vérifie commandes, bruts, compteurs, provenance et
fermeture ; l'analyseur vérifie également les identités entre campagnes du
même groupe et les hashes avant/après lecture. À la racine de cette capture,
seuls les deux conteneurs explicites de campagnes sont sélectionnés :
`qualification/` et `thread_sanitizer/` portent d'autres schémas d'artefacts.
Une campagne incomplète à l'intérieur des conteneurs n'est jamais ignorée.

Vérification locale du snapshot : les deux commandes ci-dessus terminent avec
`0` et produisent **exactement les mêmes 135 709 octets que `SUMMARY.json`**.
Une racine temporaire contenant seulement `MANIFEST.json` est rejetée avec
le code `1` en mode normal et `-O` (`incomplete campaign triplet`). La fixture
temporaire a été supprimée ; aucun brut de campagne n'a été modifié.

- SHA-256 de l'analyseur : `3e7423348583875dd359b9c4e541650977ce630068ed4c2c9004e5a7cc7565c7`.
- SHA-256 de `SUMMARY.json` : `7f0ffa2cd751e7f32d07612204b833e4bed7ef16de010f1954c28fbf37a46a27`.

## Lecture des résultats

Les affinités `[0,1,2,3]` (deux cœurs physiques, quatre threads SMT) et
`[0,2,4,6]` (quatre cœurs physiques) restent **deux groupes distincts**.
Aucun temps n'est apparié ni agrégé entre eux. Les cellules temporelles
conservent chaque répétition : médiane, minimum, maximum et `n_samples`.
Les compteurs géométriques doivent être constants, même si la répartition
du travail entre workers et leurs durées varient. `threads=0` désigne l'appel
mono de référence ; `threads=1`, l'API parallèle avec un seul worker.

À 8 000 points, K=10, s=8 et 16 jobs demandés par worker, les médianes
sur quatre cœurs physiques (trois essais par cellule, millisecondes totales)
sont :

| Famille | 1 worker | 2 workers | 4 workers |
| --- | ---: | ---: | ---: |
| Uniforme | 5 387,034 | 2 731,644 | 1 379,475 |
| Terrain | 970,828 | 492,550 | 285,578 |
| Huit amas | 2 993,779 | 1 489,013 | 769,741 |
| Rangées | 230,478 | 122,112 | 119,389 |

La dispersion sur l'hôte partagé est conservée, notamment amas/1 worker
(2 940–4 891 ms) et uniforme/4 workers (1 365–2 309 ms). Les ratios des
médianes ne constituent pas un gain stable qualifié sur une machine dédiée.
Les rangées restent déséquilibrées : à quatre workers, un worker exécutant
un seul job prend environ 115–120 ms, contre 36–45 ms pour les autres.

Sur les deux doublements 8k→16k→32k, s=8 et K=5/10, les maxima observés
sont : candidates ×2,7444 ; produits du front ×2,6820 ; descentes de témoins
×2,8891 ; visites du census ×2,9583 ; raffinements structurels ×2,7362 ;
supports émis ×2,1170. Tous sont atteints par les amas K=10, de 8k à 16k.
Aucun de ces six compteurs n'atteint ×3,8 ou ×4 ; **deux doublements ne
prouvent pas une borne asymptotique sous-quadratique**. Le corpus physique
ne contient que K=10 ; les observations K=5 viennent du corpus SMT.

Le grain 1/16/64 et les charges de chaque worker restent détaillés dans le
JSON. Les temps de partition, temps mur de la chaîne et sommes des durées
des workers/payloads sont des colonnes distinctes : une somme de durées
parallèles ne se soustrait pas du temps mur et n'est pas un temps mur.
Les compteurs de nature différente ne sont pas additionnés.

À 50k points sur quatre cœurs physiques, K=10/s=8/4 workers, le composant
mesuré prend au total 13,998 s (uniforme), 2,037 s (terrain), 10,270 s
(amas) et 0,437 s (rangées), un essai par cas. Il s'agit du **front + census
q2 et de son callback, préparations et destructions incluses**, pas de la
tour FULL, ni d'une mesure GPU/G4. Les contrats une seconde/100 ms et
plusieurs dizaines de millions de points restent non qualifiés ici.
