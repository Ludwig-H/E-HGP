# Reçu : affectation sous le col, montée de densité contre remplissage borné (dev, 29 septembre 2026)

`public_status=not_claimed`, `mode=benchmark_only`, graines **dev** seulement. Ce n'est pas un test préenregistré :
rien ici ne décide contre HDBSCAN.

## Question

L'audit multi-K (`audits/tete_multik_20260929`) a montré que la tête v10-b retient 91 % des composantes gaussiennes
séparables, et que l'écart restant au plafond de Bayes vient de l'affectation de la masse sous le col. Le remplissage
borné b(ρ) du lot C donne à un point de bruit l'amas du point classé le plus proche. Une affectation par bassins
d'attraction de la densité, la partition de Morse de l'estimateur K-NN, fait-elle mieux ?

## Exécution

- Script `bench/synthetic/alloc_dev.py` au commit `c2049eb1c`, lecteur `bench/synthetic/analyse_alloc.py`.
- 384 scènes dev : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {8 000 ; 16 000 ; 32 000} × 2 répliques.
- Têtes du lot C, inchangées, aux six K : tour (entrée `cover`, EOM, z du lot C) et sklearn (configuration du lot C).
- 13 remplissages, appliqués à égalité aux deux têtes :
  - `none`, `full`, `b1.5`, `b2`, `b2.5` ;
  - montée de densité `asc` (k = max(K, 5)) et `asc20` (k = 20), avec ou sans le rejet de b(ρ).
- La montée est un QuickShift sur l'estimateur K-NN. Chaque point pointe vers le plus dense de ses k voisins (lui
  compris). Un point de bruit suit les pointeurs jusqu'au premier point classé, dont il prend l'amas, ou jusqu'à un
  maximum non classé : il reste alors du bruit.
- **VM G4** (`g4-standard-48`, SPOT), CPU seul, 46 processus, session gardée `gcp-migration/v10_session.py`, plan
  [`plan_alloc.json`](plan_alloc.json) :
  - 524 s de mur, 22 902 s de CPU utilisateur ; 384 scènes, 0 refus, 0 échec ;
  - binaires construits sur la VM depuis le commit ; Python portable 3.12.14 envoyé comme donnée
    (`bench/g4/pyenv_run.py`) ;
  - **arrêt certifié TERMINATED** sur la cible exacte (génération `05:53:52.913-07:00`, arrêt
    `06:06:01.653-07:00`), clé OS Login retirée ;
  - statut `failed_remote` seulement parce que la VM n'a pas pip ; la commande a rendu 0 ;
  - `session/receipt.json` et `session/preflight.json` : adresse du compte masquée, sha256 des originaux dans
    `session/ORIGINAUX.sha256`.

## Résultats (ARI_s moyen, 384 scènes)

**Tour : `asc20_b2` est le meilleur remplissage à chaque K.** Écart apparié au remplissage du lot C, avec l'IC 95 %
bootstrap sur les scènes, puis les gains et les pertes à 0,005 près :

| K | Remplissage du lot C | Δ `asc20_b2` | IC 95 % | Gains / pertes |
| ---: | --- | ---: | --- | --- |
| 1 | b2 | +0,0034 | [+0,0011 ; +0,0056] | 102 / 42 |
| 2 | b2 | +0,0060 | [+0,0039 ; +0,0082] | 106 / 27 |
| 3 | b2 | +0,0069 | [+0,0048 ; +0,0091] | 116 / 32 |
| 5 | b2 | +0,0067 | [+0,0045 ; +0,0091] | 127 / 28 |
| 8 | b1.5 | +0,0092 | [+0,0073 ; +0,0113] | 162 / 43 |
| 10 | b1.5 | +0,0093 | [+0,0074 ; +0,0113] | 160 / 41 |

- Le rejet borné et le lissage sont nécessaires :
  - la montée seule (`asc`, `asc20`) perd de 0,015 à 0,037 contre le remplissage du lot C ;
  - la montée bornée non lissée (`asc_b*`) perd jusqu'à K = 8.
- **Par famille** (tour, `asc20_b2` contre le remplissage du lot C) :
  - gains : `unbalanced` (+0,022 à +0,037), `heteroscedastic` (+0,002 à +0,017), `bridge` et `spherical` (+0,004 à
    +0,011) ;
  - pertes : `anisotropic` à K ≤ 5 (−0,005 à −0,009).
- **sklearn :**
  - à K ≤ 2 (EOM, α = 1), `asc20_b2` gagne +0,006 ;
  - à K ≥ 3 (feuilles, α = 2), toute montée perd (−0,014 à −0,10). La sélection en feuilles laisse environ 40 % des
    points en bruit, et leurs montées finissent sur des maxima non classés.

  Le levier est donc propre à la tête EOM de la tour, et non une amélioration de tout clustering.
- **Tour − sklearn**, chacun avec le remplissage du lot C :

  | K | 1 | 2 | 3 | 5 | 8 | 10 |
  | --- | ---: | ---: | ---: | ---: | ---: | ---: |
  | dev | +0,082 | +0,079 | +0,084 | +0,061 | +0,039 | +0,029 |
  | test du lot C | +0,075 | +0,073 | +0,091 | +0,064 | +0,040 | +0,031 |

  Avec le même remplissage b2.5 pour les deux : +0,083, +0,080, +0,076, +0,057, +0,038, +0,027.

## Décomposition de l'écart au plafond de Bayes (192 scènes gaussiennes)

Familles `spherical`, `anisotropic`, `heteroscedastic`, `unbalanced`. Scripts [`ceiling_alloc.py`](ceiling_alloc.py)
et [`morse_ceiling.py`](morse_ceiling.py), sorties `*.txt` et `*.csv`.

- **Bayes** : partition MAP du mélange (paramètres estimés sur la vérité, bruit uniforme sur la boîte élargie).
- **Morse** : bassins d'attraction de la *vraie* densité du mélange, par QuickShift sur la densité exacte à k = 20 (et
  k = 40 en contrôle), avec le même rejet du bruit que Bayes. C'est la partition que viserait une méthode par niveaux
  de densité qui connaîtrait la densité.

| K = 10 | Bayes | Morse (k = 20) | Tour, lot C | Tour, `asc20_b2` | sklearn, lot C |
| --- | ---: | ---: | ---: | ---: | ---: |
| toutes | 0,895 | 0,847 | 0,816 | 0,830 | 0,801 |
| `spherical` | 0,948 | 0,948 | 0,920 | 0,930 | 0,928 |
| `anisotropic` | 0,950 | 0,932 | 0,854 | 0,859 | 0,892 |
| `heteroscedastic` | 0,796 | 0,712 | 0,705 | 0,707 | 0,679 |
| `unbalanced` | 0,885 | 0,797 | 0,787 | 0,824 | 0,706 |

À K = 3 et 5, les moyennes sur toutes les familles sont semblables : Morse 0,847, tour 0,815–0,817, tour `asc20_b2`
0,828–0,829.

**Lecture :**

- **Prix du modèle : Bayes − Morse = 0,048.** Aucune méthode par niveaux de densité ne le récupère.
  - Entre gaussiennes inégales, les frontières de Bayes ne passent pas par les cols.
  - Au niveau extrême, certaines composantes n'ont pas de mode propre dans la vraie densité.
  - Sur `spherical`, le prix est nul.
- **Prix de l'estimation, de la sélection et de l'affectation : Morse − tour = 0,031.**
  - L'affectation par bassins en récupère 0,013, soit 44 %.
  - Le reste, 0,017, se concentre sur `anisotropic` (0,859 contre 0,932). La sélection en feuilles de sklearn y fait
    mieux (0,892) : c'est le prochain levier de la tête, pas l'affectation.
- **Morse n'est pas une borne de l'ARI_s.** La tour la dépasse sur `unbalanced` et `heteroscedastic` au niveau
  extrême : laisser en bruit un point ambigu coûte moins que le verser dans un bassin fusionné.

## Conséquences

- `asc20_b2` devient le remplissage candidat de la tour pour le prochain préenregistrement. Son gain (+0,003 à
  +0,009) reste sous la marge de décision (0,02) : pas de test pour lui seul.
- Le k = 20 du lissage n'a pas été réglé : c'est un choix unique, fait avant la campagne.
- Réponse à l'objection des mélanges séparables : la tour identifie les modes (91 %, 99 % sous un col à 30 % du
  pic). La cible d'une méthode de densité est la partition de Morse, à 0,847, et non Bayes, à 0,895. Après
  affectation par bassins, la tour en est à 0,017, dont l'essentiel sur `anisotropic`.
