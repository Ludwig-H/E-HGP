# Contrelecture MES-M3/M4 — corrections du 7 octobre 2026

**Pin audité : `07ee13ef6bebc0b6b85da90207f3f067ffff1755`, correction `320db4a12`.**
Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Une unité C++ M4 normale compilée en Release local, neuf petits cas natifs ; 22 scénarios Python du développeur
(partie B seulement) et trois contre-épreuves du présent audit. Les résultats sont identiques en Python normal et
`-O`, contrôles explicites sans `assert`. Aucun GCP/GPU, LiDAR, nouveau mutant natif, sanitizer ou build global.

| Constat | Conclusion proposée au pin audité |
| --- | --- |
| CST-0213 | Clore le défaut de réplication dans le pilote : cinq processus neufs par campagne, campagnes ajoutées et journaux conservés. Les performances des sessions réelles relèvent du reçu G4, pas de ce témoin. |
| CST-0214 | Clore le succès vacuement vert par absence de FLOWER dans le microbanc M4 : refus natif et pilote, et couverture de toutes les naissances contrôlée. |
| CST-0018 | Correction substantielle des sorties muettes et preuves normales incomplètes ; garder ouvert pour le résidu du validateur du mutant M4 ci-dessous. |
| CST-0021 | Porte positive dans le pilote M3/M4 : empreintes du binaire avant/après, comparaison au binaire enregistré et refus après remplacement ou modification en cours d'exécution. Aucun transfert aux autres outils ou au produit. |

**Preuves positives.** `pilote.py:741` crée un sous-dossier neuf par prise, appelle `lancer` dans la boucle
`processus`, compare tous les vidages réécrits aux références puis conserve les journaux. Notre instrumentation de
scripts simulés observe **dix PID distincts**, deux campagnes de cinq prises et dix journaux distincts ; les
journaux de la première campagne restent identiques. Réutiliser son identifiant est refusé. Les 22 scénarios de
`tests/test_pilote.py:293` couvrent notamment sorties muettes, graines différentes, ordre manquant, temps non fini,
vidage modifié, quatre prises insuffisantes, dernière campagne invalide et provenance discordante. Ce sont des
tests du protocole et de ses juges ; leurs durées inventées ne sont jamais des mesures de résolution.

Le carré exact est celui de l'audit antérieur, repris dans `tests/carre.py` : sites Morton
`(0,0,0), (2,0,0), (0,2,0), (2,2,0)`, quatre boules diamétrales des côtés au niveau rationnel 1, cercle au niveau 2.
Il comporte quatre naissances à K2, une à K3 et une à K4. Le binaire réel reconstruit les forêts attendues et juge
**six naissances T6** ; K4 couvre aussi le cas où l'image basse est une naissance. Une image basse fausse rend 1.
FLOWER absente à K2–K4 ou au seul K3 rend 3 avec raison explicite. Les cinq autres corruptions ciblées sont refusées :
ordre d'en-tête, genre de forêt, clé de naissance hors catalogue, boule de cellule hors catalogue, offsets décroissants.

Le contrôle obligatoire de FLOWER précède le calcul (`mes_m4.cpp:653`) ; toutes les naissances sont parcourues
(`:791`), et le nombre effectivement jugé doit égaler leur nombre (`:814`). Le pilote contrôle aussi cette égalité
et l'identité des forêts, avec une ligne par ordre (`pilote.py:497`). Si l'ordre précédent est déjà non conforme,
le natif ne peut plus donner code 0, même quand T6 ne s'exécute pas. La contraction T4 et le passage au quotient
utilisés par T6 ne changent pas dans cette correction. Cette porte ne revendique pas une nouvelle preuve de toutes
les verticales des nœuds de fusion ni une qualification FULL du produit.

La protection `CELLS.ball < balls` est désormais exécutée avant les tableaux indexés par cette clé
(`mes_m4.cpp:686`, avant `:825`) ; le témoin natif hors catalogue est refusé explicitement. Les domaines des
naissances et la monotonie des offsets sont contrôlés avant calcul (`:682`). La garde d'opérandes 31 bits déjà
qualifiée reste présente (`:675`) ; elle n'est pas requalifiée par une nouvelle campagne ici.

`lancer` hache avant et après chaque invocation (`pilote.py:318`), compare au binaire de la construction et rend la
provenance obligatoire (`:335`). Outre la substitution préalable de la porte développeur, un script simulé modifiant
son propre fichier pendant l'appel est ici refusé avec code 3, `construit=true`, `inchange=false`. Cela établit le
rattachement des appels testés ; l'égalité d'une empreinte n'est pas à elle seule une preuve géométrique.

**Résidu confirmé CST-0018 — preuve du mutant non rattachée au cas, sévérité moyenne (qualification).**
`valider_mutant_m4`, `pilote.py:569`, ne reçoit ni la trame attendue, ni K, ni l'inventaire. Il ne demande qu'une
entrée avec le drapeau mutant, une ligne d'identité fausse et une fin concordante sans refus. Le témoin de
`check.py:134` injecte un faux exécutable de mutant dont les trois lignes sont :

```json
{"phase":"entree","K":1,"trame":"autre_trame","mutant_sans_contraction":true}
{"phase":"ordre","k":1,"identite":{"identiques":false}}
{"phase":"fin","code":1}
```

Le M4 normal exécuté à côté est le vrai binaire compilé, sur le carré K4. Les empreintes des deux exécutables sont
bien enregistrées et stables. Malgré la mauvaise trame, le mauvais K, les ordres 2–4 manquants et l'absence de
compteurs, **l'étape `m4` rend code 0**, déclare le normal conforme et le mutant `tue=true`,
`sortie_complete=true` (`pilote.py:923`). Ce n'est ni une erreur géométrique du moteur ni une allégation que le
mutant réel des sessions G4 serait incomplet : c'est une preuve reproductible que ce juge accepte un tel journal.
Correction attendue : rattacher sa validation à la trame/K/inventaire attendus et préciser/contrôler les éléments
nécessaires à une réponse géométrique complète, puis refuser ce témoin. Aucun nouvel identifiant requis.

**Rejeu et clôture.** [sources.json](sources.json) épingle 16 sources/contrats vérifiés avant et après chaque rejeu ;
[build.json](build.json) contient la commande Release exacte et l'empreinte du binaire. Recréer ce binaire hors du
dépôt avec cette commande, puis exécuter :

```bash
python3 -S check.py /tmp/ehgp-v12-audit-cd-fixes-math-bin-20261007/mhgp12_mes_m4 > normal.json
python3 -S -O check.py /tmp/ehgp-v12-audit-cd-fixes-math-bin-20261007/mhgp12_mes_m4 > optimized.json
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

Les fichiers [normal.json](normal.json) et [optimized.json](optimized.json) donnent chaque verdict, notamment le
résidu attendu ; le script échoue si ce comportement change, pour imposer une relecture de la conclusion.
[attempts.json](attempts.json) conserve deux erreurs initiales du seul script auditeur, corrigées avant les captures
finales (filtre incluant le vidage sans chronométrage, puis libellé de refus trop strict). Aucune source produit
modifiée. M3 natif, statistique G4 et sessions C/D ne sont pas rejoués ici.
