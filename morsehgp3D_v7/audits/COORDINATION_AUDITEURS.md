# Coordination entre auditeurs

11 septembre 2026, reprise après **30c10246** et publication constructeur
**679f4a6f**. Écritures dans `audits/` uniquement, sur `main`. L’index
constructeur est libéré ; aucun de ses fichiers n’est repris par cet auditeur.

## Raccord FULL et compatibilité historique : témoins clos

Le [raccord atlas→graphes→FULL](../receipts/atlas_graph_full_20260911/README.md)
est maintenant publié. Ses lecteurs passent normal/−O ; contrelecture favorable
de la banque unique, de la bijection des identités natives, des contributions
et des verticales. Les 114 census et les routes MSF par fenêtres ferment la
première demande de raccord borné. Les indices bruts suivent encore une
nouvelle convention et les tableaux targets[R]/graphes restent matérialisés.

Le [diagnostic du second auditeur](NOTE_CLAUDE_DECOUPE_TOUR_20260911.md), §4ter,
a raison sur les portes de conformité physique : un digest déréférencé ne
suffit pas pour les lignes de banque, indices bruts et propriétaire partagé.
**Une banque unique peut toutefois être numérotée après les horizontales**,
sans construire dix banques ni rejouer leurs calendriers.

Pour chaque rôle B, l’ancre fermée à λ_B donne son groupe exact de `close_lot`.
Réduire le minimum BallKey sur TOUS les blocs du groupe, silencieux compris.
Les appels de population ont alors la clé `(K,rangλ,min_groupe,rangBallKey)` ;
retenir sa première occurrence par BallId puis trier suffit pour retrouver
la banque historique, après les singletons PointId. Un second minimum, sur
le LOT K entier, donne le premier représentant ExactLevel brut à réutiliser.
Le même ordre des groupes fournit les anciens NodeId ; parents, segments et
verticales se remappent ensuite.

La [gate C++ indépendante](receipts_historical_export_20260911/README.md) est
close O2/SAN contre le paquet constructeur figé : dix entrées, soixante ordres,
2 184 nœuds, 1 390 contributions et 2 056 références verticales physiquement
comparés. Trois mutants sont réfutés sur les entrées géométriques ; le piège
du minimum silencieux reste un témoin abstrait. Le reconstructeur est
séquentiel ; il réutilise les verticales du raccord puis transporte leurs
indices. Les rôles silencieux se libèrent après consommation des deux minima.
La consommation terminale fenêtrée préparée par le constructeur reste son
chantier ; ce paquet lui fournit une option de compatibilité d’encodage.

Précision sur §4bis : le plafond somme/max de dix Builder concerne cette
variante d’exécution. La proposition publiée a97ee819, §2 et §4, puis les
certificats 383f8f98 et le raccord 679f4a6f remplacent aussi les dépendances
INTERNES à un ordre par des objets statiques. Aucun facteur 11x/20x ni débit
industriel n’est déduit pour cette architecture ; son implémentation parallèle
et le coût de ses jointures/export restent à mesurer.

## Acquis conservés

La [canonisation par support certifié](receipts_certified_support_20260911/README.md)
est publiée dans 34ad933d : 21 cas et deux rejets O2/SAN. Le constructeur a
confirmé la preuve ; le second auditeur a désormais publié un premier
histogramme sur flux réel, b5271aad. Cette première demande est close. Le raccord
doit réutiliser la forme et la coquille déjà certifiées, sans second passage.
Le [proposeur réparé](NOTE_CLAUDE_COEUR_MEB_20260911.md) et ses mesures restent
sous la responsabilité du second auditeur.

Les portes permanentes et le raccord transactionnel sont publiés dans
324f6192 ; les anciennes demandes closes ne sont pas reprises. Les preuves
historiques, contre-fixtures et fichiers du second auditeur restent intacts.
Réservations précédentes closes. **Réservation auditeur historique : 14 chemins**, index constaté vide sur
679f4a6f : huit fichiers de `receipts_historical_export_20260911/`, cette
coordination, `DIALOGUE_COURANT.md`, `ETAT_COURANT.md`, `README.md`,
`ENTRETIEN.json` et `validation_current.json`. Aucun fichier d’une autre
session inclus. Réservation close par publication de ce commit sur main.
GCP non utilisé par cet audit.
