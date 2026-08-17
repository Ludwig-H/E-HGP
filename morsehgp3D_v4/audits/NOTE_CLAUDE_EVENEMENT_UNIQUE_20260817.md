# Note de Claude — Q3Event matérialisé, un arbitrage de coût à valider

Date : 17 août 2026. Répond au verrou n° 1 commun de vos trois audits
(`6beeb0d`, `489c617`, `bc5b05d`). Mesures complètes :
`receipts/q3_events_20260817/ADDENDUM_EVENEMENT_UNIQUE_20260817.md`.

## Fait, dans votre ordre

1. `Q3Event{support, owner, ball, level, depth, interior}` matérialisé,
   `SupportKey3` en `PointId` u32 triés ; juge brut sur les RECORDS COMPLETS
   (multiensemble), owner du juge départagé sur les vrais `PointId` (plus
   les rangs Morton). 0 manquant / 0 en trop sur les quatre configurations
   appariées (`packet=off|on`, `cover=root|rectangle`, `census=tree|cover`),
   48 965 records à `uniform n=400`.
2. Contrat de capacité : `smax > 11` refusé (code 2), porte gravée. Les
   quatre invariants demandés (`core_ids == h_cœur`, collecteurs h_a/h_b ==
   histogrammes, paquet sans doublon, `|interior| == depth`) sont des codes 3.
3. Oracle étendu : BallKey jugée PROJECTIVEMENT contre `A_o = det²`,
   `B_o = -2·det·num`, `C_o = |num|² - Rnum` (produits croisés, aucun pgcd
   côté oracle) ; listes d'intérieurs (IDs externes) jugées ; fixture owner
   au-dessus du bit 31 gravée (quatre affectations, dont `2^32-1`).
   0 désaccord sur 39 852 triangles.

## L'arbitrage que je vous soumets

Votre § 1 demandait la BallKey « formée avant le census ». Mesure : 77,5 M
candidats pour 249 093 publiés sur `eight_clusters n=2000` — la canonisation
pgcd par CANDIDAT coûte un facteur 3 (12,7 s → 38,6 s ; le pgcd binaire de
Stein fait pire, 62,5 s : ~128 itérations sans division ne battent pas
Euclide qui converge en 2 modulos).

J'ai retenu un **contrat causal en deux temps** : la forme BRUTE (cinq
coefficients, niveau non réduit — fonctions pures du support) vit dans le
candidat avant le census ; la canonisation pgcd/signe est une fonction pure
de cette forme pré-census (`q3_ball_key_reduce(Q3BallForm)` — sa signature
ne reçoit rien du census), appliquée à la publication comme le tri des
InteriorIds. Résultat : 16,6 s, records bit à bit identiques, et il reste
impossible par construction qu'un champ de la clé provienne du census.

Si vous jugez que la lettre du contrat exige la clé canonique dans le
candidat malgré le facteur ~2,3, c'est un déplacement d'une ligne — dites-le.

## Prochaine étape (votre ordre)

Le comparateur `U192` et les macro-lots de niveaux égaux (contre-audit
489c617 § 2), puis la fixture `q4_source_independent_from_q3` (bc1d... §
2.3 de l'audit bc5b05d) avant toute ouverture q4. Je note aussi, pour plus
tard, votre demande d'API `InputPoint` de bout en bout : l'oracle la
respecte déjà, le probe fabrique encore ses `PointId` depuis l'ordre du
nuage — la fixture bit 31 garde le contrat en attendant.
