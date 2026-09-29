# Préenregistrements des campagnes de test du clustering v10

`public_status=not_claimed`, `mode=benchmark_only`. Chaque préenregistrement est commité **avant** la première
génération d'une scène de l'espace `test`. Le JSON fait foi ; ce texte le résume.

## PREREG_V10_KMATCH_A_20260928 : panneau apparié K = min_samples, lot A (K = 1, 2, 3)

**Exécuté le 29 septembre 2026** : la tour bat HDBSCAN à K = 1, 2 et 3 (Δ = +0,053, +0,092, +0,049 ; reçu
`receipts/test_kmatch_A_20260929`).

**Question.** À ordre égal, la tour v10 à l'ordre K, avec sa meilleure tête choisie sur `dev`, fait-elle mieux que
sklearn HDBSCAN à `min_samples` = K, avec sa meilleure tête choisie sur les mêmes scènes par la même règle ? La
comparaison se fait sur les tailles d'intérêt n = 8 000, 16 000 et 32 000. K = `min_samples` suit la directive de
l'utilisateur du 28 septembre 2026 : les deux méthodes mesurent la densité au même K-ième voisin, point compris.

**Méthodes figées (choix sur dev, règle commune, mcs = √n des deux côtés).**

| K | Tour | sklearn HDBSCAN à `min_samples` = K | Écart dev |
| ---: | --- | --- | ---: |
| 1 | EOM, λ = r^(−ẑ), remplissage b(1,5) | EOM, α = 1, b(1,5) | +0,061 |
| 2 | EOM, λ = r^(−ẑ), b(2) | feuilles, α = 2, b(2) | +0,038 |
| 3 | EOM, λ = r^(−ẑ), b(2) | feuilles, α = 2, b(2) | +0,005 |

Références descriptives, sans valeur confirmatoire : `HDBSCAN()` aux défauts, et le protocole de la thèse (mcs = √n,
`min_samples` = 3, α = 1, EOM), tous deux sans remplissage et comparés à la tour à K = 1.

**Choix sur dev.** Reçu `receipts/bench_dev_kmatch_20260928` : 256 scènes, n = 2 000 et 8 000. Grilles
symétriques :
- tour : EOM à z = 1, EOM à z = ẑ, ou feuilles ;
- sklearn : EOM ou feuilles, α = 1 ou 2 ;
- politiques de bruit communes : `none`, `full`, b(1,5), b(2), b(2,5), b(3).

On retient l'argmax de J, la moyenne d'ARI_s pondérée par cellule. Une égalité à 0,002 près se tranche par
l'échelle par défaut, puis la politique la plus simple, puis EOM.

**Plan.** 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {8 000 ; 16 000 ; 32 000} × 5 graines de l'espace `test`,
soit 960 scènes. Le manifeste des spécifications est épinglé par son sha256.

**Décision (`decide.py`).** Pour chaque paire, écart apparié par scène, avec un poids égal par cellule :
- test : retournement de signe stratifié, 100 000 tirages, Holm sur les trois paires ;
- intervalle : IC 95 % par bootstrap de McCarthy–Snowden.

« La tour bat HDBSCAN à K » exige toutes ces conditions :
- p_Holm < 0,05 ;
- Δ ≥ 0,02 et borne basse de l'IC > 0 ;
- Δ > 0 à chaque taille ;
- aucune perte significative d'AMI ;
- refus ≤ 1 %.

Sinon, le verdict est « HDBSCAN bat la tour » ou « pas de différence », avec la non-infériorité à la marge 0,02. Un
refus vaut ARI_s = 0 et n'est jamais omis.

**Prévision écrite d'avance.** Victoire de la tour à K = 1 et K = 2, parité à K = 3.

**Attribution écrite d'avance.** À K = 1, les hiérarchies sont identiques (liaison simple) : un écart y vient de la
tête (échelle ẑ, sélection, remplissage), pas de la géométrie. À K ≥ 2, l'écart mesure le couple hiérarchie + tête.

**Lot B.** K = 5, 8 et 10 seront préenregistrés séparément par la même règle, après l'accélération de la tour, dont
le coût actuel est super-linéaire. Les deux lots seront publiés quel que soit leur résultat.

**Écarts à EVAL_v2** : listés dans le JSON (`deviations_from_EVAL_v2`).

## Exécution

```bash
python3 run_test.py --prereg prereg/PREREG_V10_KMATCH_A_20260928.json --build <build épinglé> --out <dossier> --jobs 5
python3 decide.py --prereg prereg/PREREG_V10_KMATCH_A_20260928.json --run <dossier>
```

`run_test.py` refuse avec le code 2 si le binaire, un script, une version de bibliothèque ou le manifeste diffère de
son épingle. Une réexécution sur les mêmes graines n'est permise que pour un échec détecté par ces contrôles
(EVAL_v2 D11) ; tout autre changement impose un nouvel espace de graines.

## Historique

Un premier projet, jamais commité ni exécuté, opposait la tour à K = 1 à un sklearn réglé librement
(`min_samples` = 12). Il a été retiré avant tout gel, sur la directive K = `min_samples`.

## PREREG_V10_COVER_C_20260929 : tête v10-b (première couverture), lot C ; lot B ; famille « objet »

**Question.** À K = `min_samples` (1, 2, 3, 5, 8, 10), la tour avec la tête v10-b fait-elle mieux que sklearn HDBSCAN,
chaque méthode avec sa meilleure tête choisie sur dev par la même règle ? La tête v10-b fait entrer les points par
première couverture (amas discrets du théorème 2) et sélectionne par EOM avec λ = r^(−z).

**Méthodes figées.** Le choix se fait sur les 128 scènes dev de 8 000 points : l'optimum de z croît avec n (audit du 29
septembre), et les tailles de test commencent à 8 000.

| K | Tour | sklearn à `min_samples` = K | Écart dev |
| ---: | --- | --- | ---: |
| 1 | cover, EOM z = 6, b(2) | EOM α = 1, b(1,5) | +0,070 |
| 2 | cover, EOM z = 4, b(2) | EOM α = 1, b(1,5) | +0,067 |
| 3 | cover, EOM z = 5, b(2) | feuilles α = 2, b(2) | +0,060 |
| 5 | cover, EOM z = 6, b(2) | feuilles α = 2, b(2,5) | +0,046 |
| 8 | cover, EOM z = 6, b(1,5) | feuilles α = 2, b(2,5) | +0,022 |
| 10 | cover, EOM z = 6, b(1,5) | feuilles α = 2, b(2,5) | +0,013 |

**Familles secondaires**, avec la même règle de décision et un Holm dans chaque famille ; elles ne changent pas la
décision principale :
- **Lot B**, promis par le préenregistrement du lot A : tête C∩X du 28 septembre à K = 5, 8 et 10, contre sklearn.
  Écarts dev : −0,007, −0,020 et −0,030.
- **Objet** : la tour contre la hiérarchie d'atteignabilité mutuelle d'HDBSCAN (α = 2), munie de la même entrée
  (règle des points-bord) et de la même tête. On mesure ainsi ce que l'objet exact apporte au-delà de l'entrée et de
  la tête. Écarts dev de +0,001 à +0,008 : on attend la parité.

**Paires descriptives sans remplissage** (audit IMP-11). Écarts dev de +0,035 à +0,112.

**Plan.** 960 scènes de l'espace neuf `test_v10b` : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {8 000 ; 16 000 ;
32 000} × 5 graines. Le manifeste des spécifications est épinglé par son sha256, ainsi que les deux binaires
(`c764e121a`) et les scripts du banc.

**Bases dev** : reçus `bench_dev_z_samehead_20260929` et `bench_dev_objet_20260929`.
