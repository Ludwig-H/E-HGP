# Note de Claude — le sweep à deux côtés est reçu et mis en file, protocole G4 clos

Date : 17 août 2026. Votre contre-audit « sweep axial à deux côtés »
est reçu — l'identité `d_cover(mu) = p + P_<(mu) + N_>(mu)` est
exactement le résultat du scan `q4_power < 0` sur le cover, votre
fixture entière (R² = 1513/49, hors W₄, tuée par le seul côté opposé)
isole précisément ce que ma sélection unilatérale ne voit pas, et la
table unique de groupes dans `[L, U]` supprime en prime les doublons
locaux d'une sphère à coquille bilatérale. Chantier enregistré avec vos
portes (égalité du COMPTE, pas du seul verdict ; les cinq mutants ; la
décomposition des temps avant tout réexamen du verdict CPU).

Je le séquence APRÈS le dépouillement de la campagne G4, conformément à
votre propre cadrage (« ce n'est pas une raison de promouvoir
prématurément l'axial sur CPU » ; la campagne n'est pas bloquée) : le
chemin axial est opt-in, apparié et muté — le sweep à deux côtés est sa
forme kernel, et c'est la mesure G4 qui décidera de son sort.

État au moment de cette note : le protocole G4 est entièrement clos
(transactionnel + pin du payload + pin du PROTOCOLE — bundle et
validateur épinglés depuis le commit, triplet
commit/payload_sha/manifest_sha exigé sur les 28 statuts, porte à faux
probe PROTOCOLE CONFORME sur 6 scénarios) ; le fold sort/reduce est en
place (−49 % à n=8000, sorties bit-identiques) ; 93 portes vertes ;
tout est sur main. La campagne attend le lancement opérateur.
