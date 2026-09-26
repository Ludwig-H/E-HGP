# Port q3 : vérifications natives

Capture locale du 26 septembre 2026. CPU, aucune mesure CUDA
dans ce dossier. Builds neufs `build/v9-q3-payload-integration-20260926`
(GCC Release) et `build/v9-q3-payload-sanitize-20260926` (Clang,
ASan/UBSan/LSan). Les résultats finaux sont dans les journaux distincts.

Échecs de mise au point conservés ici explicitement :

- Le premier mutant producteur ne compilait pas avec `-Werror` : paramètre
  `index` devenu inutilisé sous la mutation rang pris pour ID. Annotation
  `[[maybe_unused]]`, sans changement du chemin produit.
- Les deux mutants producteur rendaient bien le code 1 et
  `cause=interior_ids_differ`, mais sur stderr ; le lecteur exige stdout.
  Correction du diagnostic de la gate, lecteur inchangé.
- Le premier test catalogue a parcouru les comparaisons puis échoué sur
  `K1_empty_payload` : fixture à un site, alors que la chaîne exige au moins
  deux sites. La fixture K1 passe à deux sites ; le refus historique à un
  site reste vérifié. Aucun changement moteur associé.

Les compilations et réussites ultérieures ne rendent pas ces essais
initiaux verts. Les mutants doivent être réfutés par leur cause attendue,
pas par un crash ou un échec de compilation.

Résultats du port : six CTests Release passent (deux portes et quatre
mutants), deux CTests Clang ASan/UBSan/LSan passent. La porte chaîne
compare 192 configurations ON/OFF, 16 exécutions avec juge global et huit
refus ciblés. 90 862 clés / 351 828 IDs sont importés dans cette série ;
40 898 replis et 24 952 reports sont exercés. Un bras catalogue plus grand
transporte 85 363 records (45 421 imports) à travers 16 plages de fusion.
La porte producteur compare 140 appels, 9 602 records q3, 1 648 q4 et
18 873 IDs ; 28 cas de limites de chunks, profondeur maximale huit.

Ces comptes sont des planchers de couverture, pas des chronos de tour
LiDAR ni une preuve de complexité. La qualification CUDA sera distincte.
