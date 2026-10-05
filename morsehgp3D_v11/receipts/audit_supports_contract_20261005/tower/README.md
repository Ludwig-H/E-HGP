# Relecture S0/S1 et reprise S3 — 5 octobre

Publication examinée : `5adf6a59f3d3b99fdf947e676e8548d4102ab48f`.
La réponse documentaire `9290cf3bf` est capturée séparément. Le WIP S3
reste sur HEAD `f98aeed67d4030dd78e11d5faf7d8556c4d17aaf` ; ses captures
initiale et de suivi sont distinctes, jamais substituées. Aucun code
public, acteur, reçu clos ou registre n'a été modifié.

## Conclusion mathématique

Aucune erreur nouvelle n'est établie dans MATHEMATIQUES §10 ni dans
l'oracle S1 sur ce périmètre borné.

- **P/W.** L'unicité MEB fixe la boule d'un nouveau sommet et de toute
  coface de même niveau qui le contient. Hors fenêtre, les parties strictes
  se rejoignent par échanges incluant les intérieurs ; elles contiennent
  déjà tous les sites. La décomposition bipartie du plateau décrit donc
  les changements de H₀. Cette omission d'événements publiés ne permet
  pas de retirer les liaisons correspondantes du vrai Γ.
- **E5 et D2.** Notre reconstruction Γ indépendante confirme les deux
  lectures de retrait, avec sommets gardés ou retirés. Dans E5 la fusion
  de `83886/3563` conserve trois enfants dans la première lecture, deux
  dans la seconde ; les deux ajoutent une fusion à 24. Pour D2, les dates
  correspondantes sont `1681/25` et `145/2`. Ces graphes restreints restent
  différents de FULL.
- **D/E.** Le journal remonte des naissances, pas les traces elles-mêmes
  au niveau précédent du catalogue. D2 donne bien `41 < 64 < 1681/25`.
  Le S1 direct lit la coupe Γ précédente 64 ; le journal peut lire la
  graine ZW à 41, avec le même nœud. Une requête de la trace AB à 41 est
  correctement refusée. E2 permet `initial=λ` pour une partie générale ;
  l'exigence stricte ne concerne que le journal des traces strictes.
- **H.** L'ensemble de points de chaque composante et coupe est comparé
  par S1 à l'union datée de W_K, puis à la restriction aux fortes. Le
  raisonnement de minimalité parmi les K-parties contenant le site x reste
  dans la composante donnée ; il ne suppose pas que seuls les supports
  ou les naissances contiennent tous les points. K1 garde son exception
  de feuilles sans boule propre.

## Contrôles purs bornés

```sh
python3 -B -S check_published.py
python3 -B -O -S check_published.py
sha256sum -c SHA256SUMS
```

Les sorties sont identiques : **795 gardes du nouveau lecteur**, plus
**652 gardes** du solveur strict Gram/Γ indépendant, **152 MEB**,
sept fixtures à K2 et **40 coupes**. La passagère est contrôlée également
à K1, son ordre documenté. Les 13 mutants publiés sont appliqués en mémoire
aux seules copies : témoins intacts conformes, puis chaque mutant rejeté
avec sa cause nommée. Une empreinte différente ou une exception étrangère
ne suffisent pas.

Les six derniers mutants sont notamment des contrôles de vivacité des
validations W.4/H/C.3/parent/vie/M1 ; tuer une validation inversée ne
qualifie pas un mutant du futur C++. Ce lecteur n'exécute ni la suite S1
complète ni un moteur natif. Il charge seulement `model`, `definition`,
`supports`, sans `__init__`, `constructive`, `judge` ou `dumps`. S1 partage
son MEB et sa forêt avec l'étage A : il ne constitue pas un troisième
constructeur FULL indépendant. Notre voie opposée calcule les partitions
Γ, supports, I/U, traces et cofaces sans cet étage A.

L'échec initial du lecteur est conservé dans `attempts/wrong_passenger_order/` :
il demandait à tort la propriété de passagère K1 à K2. Il ne s'agissait
pas d'un échec de S1.

## Source S3 : changements importants effectivement présents

La capture initiale `attachment.cpp`, SHA `7bcbc62b...`, conservait le
cast non protégé. Dans la capture suivante SHA
`77f84f00506729b253c22216860ec8c0c9bcd579dcac1c9ed3ac8c54dadb01cd` :

- [attachment.cpp](wip_0604/morsehgp3D_v11/src/tower/attachment.cpp) ligne 91
  appelle `published_traces(end-begin)` avant toute conversion ;
- [seed_log.hpp](wip_0604/morsehgp3D_v11/src/tower/seed_log.hpp) ligne 20
  refuse `count>UINT32_MAX` par `tower_capacity`, puis seulement convertit ;
- [attach_test.cpp](wip_0604/morsehgp3D_v11/tests/tower/attach_test.cpp)
  contient les frontières du helper (`UINT32_MAX`, `2^32`, le minorant
  8 361 453 672, `u64_max`), D2/E5 et les propriétaires/enfants attendus ;
- les fixtures dédiées sont construites avec Cat_K, et D2 vérifie le rang
  précédent 41. Les fixtures générales utilisent aussi Kmax4 : ces seules
  exécutions auraient inclus la boule AB au catalogue ; le test dédié
  assure donc la véritable contre-garde D2 ;
- le juge E2 actuel conserve `initial<=λ` et `ancestor_closed` pour ses
  sélecteurs premiers/derniers, tandis que le journal reste strict.

La garde de capacité et les fixtures sont donc **corrigées au source WIP** ;
ne pas les republier comme demandes encore absentes. La qualification
native reste pendante. Le rapport S3 initial contient des placeholders ;
la réponse 929 décrit l'interruption/reprise du conteneur et une règle de
commit révisée. Aucune objection mathématique de cette revue ne justifie
de bloquer la poursuite de S3.
