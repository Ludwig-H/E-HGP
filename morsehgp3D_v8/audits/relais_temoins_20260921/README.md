# Relais rectangle → paires pour la recherche de témoins : candidats, coût et identité exacte

Auditeur B, 21 septembre 2026. Front épinglé à **2629a536**
(`src/wspd/front.cpp` `eefe6f9a77663634…`, inchangé depuis c5308651),
harnais [relay_probe.cpp](relay_probe.cpp), aucune brique q3/q4 du moteur
appelée. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Mesure d'audit en réponse à la demande du journal 34 (« une
suggestion de relais qui limite les reprises de racine sans grossir l'état des
millions de petits blocs ») ; elle compte, elle ne qualifie rien.

## Le relais mesuré

Pour chaque rectangle résiduel A×B du front (masque 6) et chaque voie q3/q4 :

1. la recherche **saturante** de témoins universels de boîte de la tranche 32
   (enfant le plus proche du milieu d'abord) ; si h_q crédits sont atteints, le
   rectangle est rejeté et ses paires ne sont jamais visitées ;
2. sinon une recherche **exhaustive** sur le même index : les nœuds entiers
   universels donnent un crédit commun U, les nœuds à 4H_max ≤ 0 sont exclus,
   les feuilles restantes non universelles forment la liste des candidats C.
   C'est tout l'état du relais : transitoire, un rectangle à la fois, libéré
   après ses paires, jamais stocké par paire ni par bloc ;
3. chaque paire du rectangle ne teste que C, en saturant à h_q :
   compte du citron = U + #{c ∈ C, c ≠ a,b : citron(a,b,c)}.

L'identité (les sites hors U ∪ C sont hors du citron de toute paire du
rectangle, par monotonie des bornes de boîte) est vérifiée sans saturation
sur 2000 paires par voie et par exécution, tirées dans la masse
résiduelle, contre un balayage complet du nuage : désaccords exigés nuls par
le lecteur. Pour les mêmes paires, la descente saturante depuis la racine avec
les boîtes singleton (celle de la tranche 32 en mode `pair`) est rejouée pour
comparer ses visites de nœuds au nombre de candidats testés par le relais.
Reçu [RELAY_CHECKS.json](RELAY_CHECKS.json), rejoué par `run_relay.py read`
en `python3` et `python3 -O`.

## Résultat

- **Le relais est exact** : 0 désaccord sur les 8 exécutions (scan 0 à
  8k/16k/32k, K5 et K10 ; scans 100 et 200 à 8k/K5).
- **L'état est petit** : 9,6 à 21,7 candidats par rectangle survivant en q3,
  12,7 à 30,8 en q4 (maximum 1 100 à 3 345 et 1 482 à 4 221), pour
  1,22 à 3,33 crédits communs (q3). Le rectangle survivant paie
  74,3 à 120,1 visites de plus pour sa recherche exhaustive (q3).
- **Par paire, le relais coûte 0,30 à 0,66 fois la descente depuis la racine** en
  q3 et 0,39 à 0,84 en q4 (tests saturants sur C contre visites saturantes
  depuis la racine, mêmes paires) : un gain réel mais modéré, car après le
  rejet par rectangle la descente par paire est déjà courte ; en travail
  total (recherches saturantes + exhaustives + tests de paires contre
  recherches saturantes + descentes par paire) le relais vaut
  0,71 à 0,87 fois la référence en q3 et
  0,74 à 1,04 en q4.
- Le nombre de candidats par rectangle croît lentement avec n (q3 :
  10,1 → 13,4 → 17,8 à K5 de 8k à 32k), moins vite que les visites
  par paire depuis la racine (88,0 → 122,3 → 251,8).

## Tableaux

Par voie : masse rejetée au rectangle, masse rejetée au total (rectangle +
paires), visites saturantes par rectangle, visites exhaustives par rectangle
survivant, candidats par rectangle survivant (moyenne, maximum), crédits
communs par rectangle survivant, tests par paire (saturants, toutes les
paires), tests saturants du relais / visites depuis la racine sur les paires
échantillonnées, leur rapport, travail total relais / référence, désaccords.

Voie q3 :

| scan | n | K | rejet rect. % | rejet total % | sat./rect | exh./rect surv. | C moy. | C max | U moy. | tests/paire | relais / racine (paire) | rapport | travail total relais / réf. | désaccords |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 000000 | 8000 | 5 | 86,9 | 92,3 | 58 | 78 | 10,1 | 1 100 | 1,29 | 31,6 | 26,2 / 88,0 | 0,30 | 39 140 186 / 44 768 844 | 0 |
| 000000 | 8000 | 10 | 86,6 | 92,1 | 68 | 97 | 15,2 | 1 828 | 3,33 | 55,8 | 61,9 / 125,4 | 0,49 | 98 000 787 / 116 980 954 | 0 |
| 000000 | 16000 | 5 | 86,5 | 93,1 | 66 | 90 | 13,4 | 1 887 | 1,27 | 60,0 | 73,0 / 122,3 | 0,60 | 113 089 959 / 131 626 176 | 0 |
| 000000 | 16000 | 10 | 87,9 | 93,8 | 77 | 109 | 18,4 | 2 592 | 3,27 | 87,3 | 105,1 / 158,9 | 0,66 | 274 255 853 / 325 940 919 | 0 |
| 000000 | 32000 | 5 | 88,7 | 95,4 | 76 | 104 | 17,8 | 3 345 | 1,22 | 130,4 | 158,0 / 251,8 | 0,63 | 404 329 699 / 566 087 907 | 0 |
| 000000 | 32000 | 10 | 90,2 | 95,3 | 86 | 120 | 21,7 | 3 345 | 3,20 | 124,1 | 114,5 / 208,1 | 0,55 | 732 106 949 / 890 380 769 | 0 |
| 000100 | 8000 | 5 | 81,6 | 89,9 | 55 | 74 | 9,6 | 1 678 | 1,30 | 69,5 | 75,5 / 131,8 | 0,57 | 46 260 308 / 55 030 104 | 0 |
| 000200 | 8000 | 5 | 85,3 | 93,3 | 61 | 80 | 13,0 | 1 817 | 1,29 | 72,9 | 81,8 / 128,2 | 0,64 | 56 426 836 / 67 004 159 | 0 |

Voie q4 :

| scan | n | K | rejet rect. % | rejet total % | sat./rect | exh./rect surv. | C moy. | C max | U moy. | tests/paire | relais / racine (paire) | rapport | travail total relais / réf. | désaccords |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 000000 | 8000 | 5 | 76,5 | 88,6 | 77 | 87 | 14,2 | 1 482 | 0,84 | 47,5 | 48,5 / 123,1 | 0,39 | 45 585 590 / 58 798 469 | 0 |
| 000000 | 8000 | 10 | 83,7 | 91,2 | 80 | 107 | 19,7 | 1 828 | 2,84 | 67,1 | 80,4 / 149,3 | 0,54 | 120 458 878 / 151 672 967 | 0 |
| 000000 | 16000 | 5 | 77,7 | 91,2 | 93 | 106 | 20,2 | 2 297 | 0,81 | 86,9 | 95,1 / 113,9 | 0,84 | 150 341 796 / 145 725 649 | 0 |
| 000000 | 16000 | 10 | 85,4 | 93,2 | 93 | 124 | 24,9 | 3 088 | 2,78 | 99,9 | 94,8 / 177,2 | 0,54 | 347 387 210 / 418 766 461 | 0 |
| 000000 | 32000 | 5 | 77,3 | 92,5 | 110 | 127 | 27,6 | 3 331 | 0,77 | 124,0 | 101,5 / 148,1 | 0,69 | 457 153 888 / 441 058 414 | 0 |
| 000000 | 32000 | 10 | 87,3 | 94,9 | 105 | 142 | 30,8 | 4 221 | 2,72 | 176,6 | 182,3 / 297,3 | 0,61 | 1 158 151 609 / 1 532 610 867 | 0 |
| 000100 | 8000 | 5 | 74,7 | 87,9 | 72 | 81 | 12,7 | 2 021 | 0,85 | 85,1 | 88,4 / 164,2 | 0,54 | 51 608 793 / 66 638 804 | 0 |
| 000200 | 8000 | 5 | 78,0 | 91,4 | 83 | 91 | 17,7 | 1 866 | 0,84 | 83,8 | 75,7 / 163,5 | 0,46 | 64 590 389 / 87 369 115 | 0 |

## Limites

Unités de travail hétérogènes (une visite de nœud prépare deux bornes, un
test de candidat évalue un citron entier) : les rapports sont des comptes,
pas des temps. Le relais mesuré ne porte que sur la recherche de témoins
rectangle → paires ; le même mécanisme (crédit commun + liste de candidats
transitoire) s'applique au relais bloc de seeds → seeds du census q3, non
mesuré ici. Trois tailles, trois scans, aucune qualification.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/relais_temoins_20260921/run_relay.py read
git worktree add --detach /tmp/wt-2629a536 2629a536
cmake -S /tmp/wt-2629a536/morsehgp3D_v8 -B /tmp/wt-2629a536/build/v8-audit -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost> && cmake --build /tmp/wt-2629a536/build/v8-audit --parallel --target mhgp8_p0
python3 -B -O morsehgp3D_v8/audits/relais_temoins_20260921/run_relay.py run --worktree /tmp/wt-2629a536 --binary /tmp/relay_probe --output /tmp/RELAY_CHECKS.json
```
