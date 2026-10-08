# D6 M : profils et largeur locale, 8 octobre 2026

**126 journaux, 630 passes dont 504 chaudes admis**, source `957e9784fb19ccd2d6348e779ed8b7affadc1f3d`.
Les 63 couples catalogue/G sont exactement les trois trames × sept combinaisons profil/facteur × trois tours,
dans l'ordre tournant du pilote épinglé. Chaque processus joue cinq passes, dont la première est exclue du temps
chaud. [Provenance, commandes et arrêt](../session_m_provenance/README.md) : aucun appel moteur/GCP par l'audit.

Les valeurs suivantes sont des médianes des trois médianes chaudes par processus, en ms. **C et G sont mesurés
séparément sur CPU**, K5/W48/feuille24 ; ce tableau n'est ni FULL ni une campagne GPU.

| profil, facteur | C ng00 | C ng01 | C ng02 | G ng00 | G ng01 | G ng02 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| u21 ×1 | 320,051 | 273,853 | 318,941 | 47,871 | 38,448 | 43,925 |
| u24 ×1 | 323,597 | 275,716 | 327,963 | 48,523 | 38,406 | 48,666 |
| u32 ×1 | 324,915 | 275,681 | 326,243 | 49,638 | 40,159 | 49,745 |
| u32 ×2048 | 955,304 | 802,574 | 945,998 | 52,971 | 43,020 | 48,102 |

Les rapports sont appariés **par tour** avant médiane, et ne sont donc pas les rapports des colonnes ci-dessus.
Pour C, u24/u21 donne +1,108 / +0,791 / +2,137 % ; u32/u21 +1,424 / +0,778 / +2,456 %.
Pour G, u24/u21 donne +1,361 / +0,468 / +10,795 % ; u32/u21 +3,845 / +4,452 / +13,629 %.
À ×2048, C vaut 2,987 / 2,931 / 2,966 fois sa référence u21 ×1. Tous les facteurs ×8, les intervalles observés
entre tours et les nombres exacts sont conservés dans [results.json](results.json).

**D6 reste ouvert.** Le critère produit « moins de 3 % de u21 » ne se prouve pas avec ces deux étages isolés,
ni par la somme de leurs médianes. La préparation et le FULL des profils élargis restent à mesurer au régime retenu.
Les prises ×1 utilisent les mêmes coordonnées entières : elles comparent le coût des profils, sans précision
géométrique nouvelle. Les facteurs ×8/×2048 sont des homothéties de ces entiers déjà quantifiés ; multiplier les
mots n'ajoute pas de détails au nuage d'origine.

## Largeurs et identité

Les classes physiques de feuilles sont stables dans les cinq passes et les trois tours de chaque cellule.
À ×1, u21 et u32 ont respectivement 123 580 / 99 765 / 115 657 feuilles étroites et 1 / 3 / 0 exactes.
À u32 ×2048 : zéro étroite, 123 497 / 99 671 / 115 572 exactes, dont 119 408 / 95 720 / 113 373 moyennes
et 4 089 / 3 951 / 2 199 larges. Le compteur `region_line_fallbacks` reste nul. Ces observations expliquent
un changement de classe de calcul ; elles ne prouvent ni l'attribution causale de tout le surcoût ni une borne
générale en fonction du nombre de sites.

L'admission de ×8 en u21 implique, par la sélection `max_coord × facteur < 2^bits` du pilote, que les coordonnées
initiales sont inférieures à 2^18. Le facteur 2048 les laisse donc **sous 2^29**. Cette déduction utilise le pilote
épinglé et la cohorte, sans lire les coordonnées. « Remplir les 32 bits » n'est pas établi par ce banc ; le précédent
[erratum D6](../d6_session_k/README.md) reste applicable. Aucun test nouveau du domaine u32 entier.

Les empreintes de résolution sont égales entre profils à ×1 et les comptes de l'objet restent invariants sous
homothétie. Les corps de catalogue ×1 sont **déclarés** identiques dans le rapport ; les exports ayant été effacés
par le pilote, l'auditeur ne les rehache pas. Le digest G est émis à la dernière passe seulement. Ce reçu ne transforme
pas ces contrôles de concordance en nouvel oracle géométrique ou en certificat FUL1.

## Contrôle reproductible

`capture.json` épingle trois sources Git, le rapport et l'inventaire complet des journaux, reliés à l'archive
de résultats M. `review.py` refuse les doublons JSON, constantes non finies, codes non entiers/non nuls, cohortes
ou résumés altérés. Il réutilise le lecteur D6 qualifié, confronte indépendamment médianes/rapports, vérifie les
comptes de ledger C, les sommes d'horloges G et l'identité avant/après de tous les fichiers lus. Les codes internes
restent ceux déclarés par le pilote, distincts du code externe archivé par le contrôleur.

```sh
python3 -B review.py --repo DEPOT --folder DOSSIER_D6_M
python3 -B -O review.py --repo DEPOT --folder DOSSIER_D6_M
```

Les deux sorties égalent `results.json`. Aucune copie de source complète, de JSONL ou de payload de scène dans
ce reçu. Aucun banc natif ou build relancé. Les sources, journaux et métadonnées externes sont nécessaires au rejeu.

[Erratum proposé au reçu développeur72](../d6_m_erratum/README.md) : les plages sont des min/max de trois ratios,
pas des intervalles de confiance ; ×2048 ne remplit pas 32 bits. Le patch documentaire est vérifié en copie Git.
