# MEB à double budget — qualification locale du 5 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Suite distincte : le [filtre privé du 6 septembre](RESULTATS_MEB_FILTREE_20260906.md)
est maintenant qualifié sur ses propres sources et captures. Le présent
reçu conserve le calendrier historique et ses comptes, sans réattribution.

## Résultat et portée

Le prototype privé à double budget conserve les terminaux de F dans
**9 339 comparaisons locales** : booléen, statut, raison, treize statistiques
legacy, événements sentinelles, clé, support entier et niveau exact brut.
La porte instrumentée ne constate aucune violation de charge prospective.
Son mutant charge-après produit exactement 46 437 violations, sans changer
les autres résultats ; il est rejeté avec le code 4 attendu.

Ce résultat n'est pas une intégration dans le pipeline. Les fichiers produit
F restent inchangés ; aucune option MEB supplémentaire n'est livrée par ce
reçu. L'accord à F utilise des prédicats communs : il ne remplace pas un juge
arithmétique indépendant, ni la qualification de la composition dans la tour.
La [contrelecture mathématique indépendante](../audits/MEB_DOUBLE_BUDGET_COURANT.md)
établit séparément la conservation locale sous les préconditions du helper.
L'auditeur a aussi qualifié ce prototype avec son [oracle rationnel propre](../audits/receipts_meb_dual_20260905/README.md) :
3 430 MEB par build O2 et O1/UBSan, 1 507 ordinaux et trois corruptions
géométriques détectées (coquille, ordinal, écriture brute q4). Les relectures
Python normal et optimisé consomment les mêmes sorties, pas quatre campagnes
moteur. Cette autorité distincte ne qualifie pas encore le futur port Builder.

## Deux charges, sans réinterpréter les anciens plafonds

La [note de proposition](PROPOSITION_MEB_ET_BUDGETS.md) conserve la réfutation
du prototype ordinal seul. Un ordinal préserve la décision de budget de F,
mais ne borne pas à lui seul les essais supplémentaires d'un proposeur.
Le nouveau prototype distingue donc :

- L : plafond legacy ; la charge conserve l'ordinal de référence.
- P : plafond des formes proposées, chargées avant leur construction,
  y compris celles rejetées pour rang ou acuité.
- A : candidats réellement essayés dans le repli F, mesurés par leur
  incrément legacy, sans inclure les appels de référence du juge.

Depuis des compteurs nuls, $A+P_{\mathrm{consomme}}\leq L+P$. Les deux
termes sont gardés séparément en C++ : la somme mathématique peut dépasser
u64. Cette borne compte des candidats, pas des instructions ou des durées.
Les recherches de paire et les tests de puissance ont aussi un coût.

P épuisé provoque un repli F, pas un nouveau refus scientifique. Un certificat
rapide peut ensuite être refusé par L ; `certified` ne signifie donc pas
« succès public ». L'état P persiste entre les appels d'une même tentative.
Le futur raccord par ordre doit requalifier cette durée de vie et versionner
la comptabilité `reference_ordinal_plus_proposal_v1`, avec P=0 par défaut.

## Corpus et observations

Le corpus comprend huit scènes explicites, 160 scènes LCG déterministes et
huit compléments ciblés, soit 176 scènes de 2 à 11 points u16 distincts.
L'ordre initial, le renversement et les permutations exhaustives des trois
scènes extrêmes q2/q3/q4 donnent 384 ordres. Un appel F séparé par ordre
détermine R, sans fournir son support au proposeur.

La matrice principale croise huit P (0, 1, 4, 5, 15, 16, 25, 401) et trois
L (R−1, R, R+1), soit 9 216 comparaisons. Les 123 frontières supplémentaires
portent sur les compteurs non nuls, MAX, P épuisé, les replis forcés, les
séquences cumulatives et le cap de pivots. Une énumération indépendante
contrôle les 1 507 ordinaux valides pour n2..11 et q2..4.

| Observation sur les 9 339 comparaisons | Nombre |
| --- | ---: |
| Succès locaux identiques à F | 6 047 |
| Refus de coquille non essentielle | 160 |
| Refus legacy L | 3 132 |
| Certificats trouvés avant décision L | 5 081 |
| Appels réels du repli F | 3 824 |
| Formes proposées chargées | 46 431 |
| Candidats essayés dans les replis A | 341 083 |
| Incréments legacy, dont ordinaux virtuels | 551 216 |

Il y a 3 616 succès rapides (q2 : 1 610 ; q3 : 1 520 ; q4 : 486),
1 465 certificats suivis d'un refus L et 434 gardes legacy avant toute
proposition. Les scènes extrêmes nommées exigent explicitement 8, 16 et 52
succès rapides respectivement ; les seules sommes globales ne suffisent pas.
Les q4 nommés exigent plusieurs pivots et un limb supérieur non nul.

Six contrôles directs de formes sont distincts de cette matrice : quatre
rejets géométriques et deux cas q2 admis. Ils ne sont pas ajoutés aux
46 431 formes agrégées des comparaisons. Le mutant rend ainsi exactement
46 431 + 6 violations causales. Tous ses autres compteurs concordent avec
le nominal, pas seulement ses terminaux géométriques.

## Capture

Les [deux reçus publics](../receipts/meb_dual_geometry_20260905/README.md)
conservent 154 copies exactes et 67 snapshots de sources dans 161 fichiers,
sans ELF ; les 160 entrées de sommes ont été vérifiées. Leur fermeture
`SHA256SUMS` est `2abbc2130bb3023122293afaa451dcb0bc3f4d367db5b1f179800fb4403d1605`.
L'export ne lance aucun moteur. La restauration des sources et les commandes
de reconstruction sont documentées, sans prétention de capture hermétique
ou de relocalisation des runners historiques.

Le reçu de budget antérieur `a7dc0020` porte sur un triangle d'ordinal 4,
ses caps, une séquence cumulative et une frontière MAX. Il ne qualifie pas
q4. Le reçu géométrique `b81d8e48` est une exécution nouvelle, terminée le
5 septembre à 11:28:42 UTC, CPU0, en 3,96 s compilation et fermeture comprises.
Cette durée est celle de la qualification, pas un benchmark du helper.

Ses six codes sont 0/0/0/4/2/2 : version du compilateur, compilation stricte
C++20/O2, nominal, mutant, argument inconnu, argument supplémentaire.
Les 66 pins de sources, 21 dépendances compilées et 38 artefacts ont été
rehashés à la clôture. Le CLI F est protégé par hash, mais n'a pas été lancé
par cette campagne. Aucun sanitizer ni `MHGP7_TESTING` n'est activé.

Cette porte utilise **Trace seulement**. L'instanciation `NoObserver`, son
coût et le raccord effectif dans le Builder restent des qualifications
distinctes. Les futurs mutants de coquille, d'ordinal, de canonicalisation
et du niveau q4 doivent traverser la vraie route proposée ; le mutant de
charge prospective ne les remplace pas.

## Contrats inchangés

Aucune structure globale de cellules, cofaces ou incidences n'est ajoutée
par ce helper : ses sous-problèmes restent limités à onze sites, quatre
sommets de support et cinq sites par pivot. Cela ne réduit pas à lui seul
les occurrences de facettes ni la résidence globale du pipeline F.

Les [résultats mono F](RESULTATS_MONO_F_20260905.md) restent les mesures de
tour disponibles : accord s8/10/12 à 8k sans gain robuste du delta F,
succès à 16k et refus de ressources à 32k. Aucun résultat local ci-dessus
ne valide 50k en 1 s, 100 ms ou plusieurs dizaines de millions de points.
GCP non utilisé pour cette qualification.
