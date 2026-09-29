# Reçu : test préenregistré du lot C, tête v10-b, lot B et famille « objet » (29 septembre 2026)

`public_status=not_claimed`, `mode=benchmark_only`. Une seule exécution sur l'espace de graines neuf `test_v10b`, selon
le [préenregistrement](../../bench/synthetic/prereg/PREREG_V10_COVER_C_20260929.json) figé en `bc413ff56` et amendé
en `65ea6b222` : exécution déplacée sur G4 avant toute lecture d'un résultat de test.

## Exécution

- 960 scènes : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {8 000 ; 16 000 ; 32 000} × 5 graines ; 32 méthodes ;
  30 720 lignes ; 0 refus, 0 échec d'ouvrier.
- **VM G4** (`g4-standard-48`, AMD EPYC 9B45), CPU seul, 46 processus, en deux sessions gardées
  (`gcp-migration/v10_session.py`) :
  - **c1** (10:40–11:06 UTC) : 338 scènes, les plus grosses d'abord, en 1 227 s ;
  - **c2** (11:07–11:22 UTC) : les 622 restantes, en 634 s.
- **Arrêt certifié TERMINATED** sur la cible exacte après chaque session (`sessions/*/receipt.json`, adresse du compte
  masquée). Le statut `failed_remote` des deux sessions ne vient que de l'absence de pip sur la VM ; la commande du
  lot a rendu 0 les deux fois.
- **Environnement.** Binaires du commit `c764e121a` liés statiquement, identiques aux binaires épinglés sur 152
  contrôles dev ; CPython 3.12.14 portable avec numpy 2.5.3, scipy 1.18.1 et scikit-learn 1.9.1. Les épingles ont été
  vérifiées par `run_test.check_pins` au début de chaque session.
- **Lanceur et fusion.** `bench/g4/lot_runner.py` appelle `run_test.run_unit`, inchangé.
  `bench/g4/merge_sessions.py` a vérifié qu'aucune scène n'a été calculée deux fois et que chaque scène porte les 32
  méthodes.
- **Décision** par le `decide.py` épinglé : `DECISION.json` et `DECISION.md`, aucun chiffre écrit à la main.

## Concordance avec l'exécution locale interrompue

L'exécution locale commencée à 10:10 UTC a été arrêtée après 59 scènes et mise de côté sans être lue ; elle ne compte
pas dans la décision. Comparaison ligne à ligne (`compare_lotC_local_g4.py`, `concordance_local_g4.txt`) :

- **tour et témoin MR-bord** (arithmétique entière exacte) : 1 180 lignes sur 1 180 identiques, entre binaires
  dynamiques sur le codespace et binaires statiques sur la VM ;
- **sklearn** : 411 lignes sur 708 diffèrent, d'au plus 0,0028 d'ARI_s. Ce sont des écarts flottants entre CPU
  (AVX-512 sur la VM, AVX2 sur le codespace) aux mêmes versions de bibliothèques, qui déplacent des égalités dans
  l'arbre couvrant. HDBSCAN n'est donc pas reproductible bit pour bit d'une machine à l'autre ; l'effet est
  négligeable devant les marges de décision.

## Résultat

**Famille principale : la tour bat HDBSCAN à tous les K**, au sens préenregistré. Chaque condition est remplie :
p_Holm < 0,05, Δ ≥ 0,02, borne basse de l'IC 95 % > 0, Δ > 0 à chaque taille, aucune perte d'AMI, aucun refus.

| K | Tour (v10-b) | sklearn à `min_samples` = K | Δ | IC 95 % | Δ sans remplissage |
| ---: | ---: | ---: | ---: | --- | ---: |
| 1 | 0,7952 | 0,7202 | +0,075 | [+0,067 ; +0,083] | +0,034 |
| 2 | 0,7932 | 0,7202 | +0,073 | [+0,066 ; +0,081] | +0,041 |
| 3 | 0,7966 | 0,7053 | +0,091 | [+0,087 ; +0,096] | +0,049 |
| 5 | 0,8001 | 0,7366 | +0,064 | [+0,060 ; +0,067] | +0,077 |
| 8 | 0,7960 | 0,7560 | +0,040 | [+0,037 ; +0,043] | +0,100 |
| 10 | 0,7957 | 0,7647 | +0,031 | [+0,028 ; +0,034] | +0,105 |

- p_Holm = 6e-5 partout. Les écarts dépassent les prédictions dev (de +0,013 à +0,070).
- Avec remplissage, l'écart croît avec n dès K = 3 : +0,063 à 8 000 points contre +0,120 à 32 000 (K = 3).
- Par famille, avec remplissage :
  - à K ≥ 3, le gain se concentre sur `shells` (+0,27 à +0,51) et `unbalanced` (+0,06 à +0,11). La tour cède un
    peu sur `anisotropic` et `filaments` à K ≥ 8 (−0,03 à −0,05) ;
  - à K ≤ 2, face à sklearn en EOM avec α = 1, elle gagne sur `spherical` (+0,30), `bridge` et `anisotropic`, et
    perd sur `hierarchical` (−0,12).

**Lot B, tête C∩X du 28 septembre** (famille secondaire, Holm dans la famille) :
- K = 5 : pas de différence significative (+0,011) ;
- K = 8 et 10 : HDBSCAN bat cette tête (−0,025 et −0,034).

C'était prédit (écarts dev de −0,007, −0,020 et −0,030) : c'est la tête abandonnée au profit de v10-b.

**Famille « objet »** : la tour contre la hiérarchie d'atteignabilité mutuelle d'HDBSCAN (α = 2), munie de la même
entrée (règle des points-bord) et de la même tête.

| K | Tour | MR₂-bord | Δ | IC 95 % | Verdict |
| ---: | ---: | ---: | ---: | --- | --- |
| 2 | 0,7932 | 0,7937 | −0,001 | [−0,004 ; +0,003] | pas de différence |
| 3 | 0,7966 | 0,7970 | −0,000 | [−0,003 ; +0,003] | pas de différence |
| 5 | 0,8001 | 0,7986 | +0,001 | [−0,001 ; +0,004] | pas de différence |
| 8 | 0,7960 | 0,7958 | +0,000 | [−0,002 ; +0,002] | pas de différence |
| 10 | 0,7957 | 0,7859 | +0,010 | [+0,006 ; +0,014] | avantage significatif à la tour, sous la marge |

## Lecture

- **Objectif atteint sur ce banc :** à `min_samples` = K apparié, la chaîne v10-b bat sklearn HDBSCAN à tous les K,
  de 8 000 à 32 000 points, sur un espace de graines neuf. Ce n'est pas une revendication publique (`not_claimed`).
- **Attribution**, conforme à l'audit du 29 septembre et écrite avant le test :
  - à K = 1, l'écart vient entièrement de la tête ;
  - à K ≥ 2, la famille « objet » montre qu'à même entrée et même tête, la hiérarchie d'HDBSCAN fait presque aussi
    bien que la tour exacte. L'avantage sur HDBSCAN vient de l'entrée des amas discrets et de la tête (z), qu'on peut
    aussi donner à la hiérarchie d'HDBSCAN.
- **Seul avantage propre de l'objet mesuré ici :** +0,010 à K = 10, sous la marge préenregistrée.
- **La voie d'un avantage structurel reste l'axe K de la tour,** que la tête ne lit pas encore : recherche multi-K
  en cours.
