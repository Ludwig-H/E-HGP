# État de l'audit complémentaire v8

13 septembre 2026, deuxième passe après `7cea0eaf`. Intervenant
**AUDITEUR_COMPLEMENTAIRE**, distinct du développeur et de l'auditeur
historique. Cadre : `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Demandes actives

**P0 reste ouverte.** Les nouvelles contre-fixtures séparent deux problèmes :
trouver tôt les témoins utiles et réduire les paires que le certificat
local ne peut jamais éliminer.

| Priorité | Constat vérifié et suite utile |
| --- | --- |
| P1, reçus | Le runner `d2f2514b…` accepte huit résultats de configuration contradictoire sur neuf dans un rejeu artificiel, puis annonce `completed` ; [preuve](CAMPAIGN_RECEIPT_CHECKS.json). Le développeur annonce son durcissement ; la contre-qualification de cette correction reste à faire. |
| P2, diagnostic | Une sortie JSON tronquée n'est pas consignée et ne produit aucun reçu de fermeture sur le runner épinglé. La finalisation `invalid` est incluse dans le durcissement annoncé. |
| P0, résidu | Sur les [rangées transverses](P0_RESIDU_TRANSVERSE.md), même des crédits locaux parfaits laissent 65 536 paires à n512 ; seulement 2 786 passent le contrôle q2. Comparer des certificats de sous-rectangles ou de profondeur par blocs. |
| P0, ordre des témoins | La [variante temporaire ordonnée](P0_ORDRE_TEMOINS.md) réduit les tâches de 188 910 à 82 sur une tige diagonale à 36 candidates. Construction d'arbre toujours payée ; une hausse locale de tests sur grille est conservée. |

Ces constats ont été transmis dans le
[journal partagé](../COORDINATION_MORSEHGP3D_V8.md). Le rejeu contradictoire
est un test artificiel de validation du reçu, pas un benchmark ni une
accusation de résultat réel falsifié. Le census q2 de l'audit est un juge
borné, pas le moteur proposé pour le résidu.

Le [défaut de copie/affectation du propriétaire](../../morsehgp3D_v8/audits/DIALOGUE_COURANT.md)
est suivi par l'autre auditeur. Les avis géométriques ci-dessous supposent
les propriétaires issus de la factory sans mutation ultérieure ; ils ne
qualifient pas cette garantie d'immuabilité. Ses preuves ne sont pas copiées
ni réattribuées ici.

La suppression des quatre opérations de copie/affectation est maintenant
visible dans le header `f6c89476…`. Le [rejeu de nos rails](LOCAL_CREDITS_POST_OWNER_CHECKS.json)
sur ces nouveaux octets passe avec les deux mutants ; il conserve le reçu
initial séparé et ne se substitue pas au juge de propriété de l'autre auditeur.

## Résultats bornés disponibles

- [Campagnes initiales capturées](CAMPAIGN_INITIAL_CHECKS.json) : les
  729 tuples commande/résultat concordent dans notre contrôle indépendant ;
  le lecteur constructeur est passé en normal/`-O` sur quatre campagnes et
  513 configurations avant leur archivage par le développeur dans
  `first_pass_pre_owner_fix`. Aucun résultat n'est transféré à ses corrections
  en cours. Le défaut du runner n'est pas une contradiction détectée dans
  les tuples de ces mesures initiales.
- [Ordre DualBlocks](DUAL_ORDER_CHECKS.json) : 15 cas par ordre, trois
  stratégies, comptes littéraux égaux ; 4 664 064 couples ancre–témoin
  examinés par le juge multiprécision par ordre, avant les trois plans.
  Cinq cas de la variante sont exécutés sous UBSan. Aucun patch produit.
- [Tubes C++](TUBES_CHECKS.json) : 3 051 plans, 65 772 crédits confrontés
  aux coins par un oracle indépendant, 360 933 paires/expansions ;
  mutations Δ≥0 et séparation 100→25 compilées puis réfutées sous UBSan.
  Les directions négatives, la frontière D=10R, les replis et les produits
  dépassant i64 sont exercés. Propriétaires factory non mutés uniquement.
- [Rangées transverses](P0_RESIDU_TRANSVERSE.md) : 24 essais C++20 strict/UBSan,
  comptes, expansion et profondeurs, frontière mutée réfutée ;
  [reçu](TRANSVERSE_RESIDUAL_CHECKS.json).
- [Rails](P0_RAILS.md), première passe : à n2718, Pool laisse 1 846 881
  candidates, DualBlocks en laisse 2 916 avec les comptes exacts attendus.
  Neuf cas et deux mutants compilés ; [reçu initial](LOCAL_CREDITS_CHECKS.json).
  L'autre auditeur a vérifié son modèle tubes sur cette même fixture.
- [Quantificateurs](P0_QUANTIFICATEURS.md) : preuve et fixture du piège
  « aucun témoin commun ». Le prédicat v8 audité l'évite correctement.
  [Prédicats initiaux](PREDICATES_CHECKS.json) : 900 000 comparaisons
  ponctuelles, 7 290 000 requêtes universelles, deux mutants compilés réfutés.
  [Modèles exacts](MODELS_CHECKS.json) : modes normal/`-O`.

Les reçus épinglent leurs sources par SHA256. Les sources v8 étaient
locales et en construction pendant ces passes : aucun résultat n'est
transféré à une révision ultérieure. Aucun CTest du développeur, temps de
tour, producteur q3/q4 complet, qualification FULL ou GPU n'est annoncé
par ces contrôles. La preuve continue des bornes et leur falsification
sur échantillons sont deux autorités distinctes.

## Rejeu et entretien

```bash
python3 -B audits/morsehgp3D_v8_complementaire/tubes_checks.py
python3 -B audits/morsehgp3D_v8_complementaire/dual_order_checks.py --selftest
python3 -B audits/morsehgp3D_v8_complementaire/transverse_residual_checks.py --selftest
python3 -B audits/morsehgp3D_v8_complementaire/local_credits_checks.py --selftest
python3 -B audits/morsehgp3D_v8_complementaire/campaign_receipt_probe.py --selftest
```

Les contrôles C++ compilent dans des copies temporaires neuves. Celui des
tubes exige ses cinq sources épinglées ; les contrôles transverse et local
capturent un nouvel instantané. Le runner local inclut désormais le header
des tubes ; les reçus antérieurs restent inchangés. Le contrôle de campagne
refuse un runner ou un binaire différent de ses pins historiques, avec
code 2 : une correction demande une nouvelle qualification.

Le lecteur `predicates_checks.py` garde ses pins initiaux. Le commentaire
de largeur de `types.hpp` ayant changé avec les tubes, son refus du nouveau
header est attendu ; il ne signifie pas une régression géométrique.
Ne pas réécrire ce reçu clos pour faire disparaître la différence.

Les anciens exposés détaillés ont été sortis de cette entrée courante vers
leurs notes nommées, sans déplacer les scripts ni les reçus épinglés. Les
rapports constructeur et ceux de l'autre auditeur restent sous leur autorité.
La porte documentaire générale exclut ces audits indépendants : appeler
aussi explicitement `tools/check_docs.py:validate` sur leurs Markdown.
GCP non utilisé.
