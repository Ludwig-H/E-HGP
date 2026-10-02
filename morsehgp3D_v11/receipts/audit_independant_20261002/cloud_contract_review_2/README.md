# Cloud/Morton v11 WIP : lecture de contrat et mémoire

2 octobre 2026. Capture privée avant lecture : 23 fichiers, plus core/core.hpp capturé dans DEPENDENCY_SUPPLEMENT.json. Base et état Git exacts dans SOURCE_BEFORE.json ; cloud/io/tests-cloud sont WIP non committés. Audit **statique**, aucun build ni test produit local, aucun GCP. Les sources IO sont conservées dans la capture initiale mais jugées par l'auditeur principal, pas par ce reçu. Pas de qualification héritée de la v10.

## Positifs ancrés au code

- [cloud.cpp](sources/morsehgp3D_v11/src/cloud/cloud.cpp#L134) garde les tailles avant toute lecture, puis le domaine. n<kNone protège les indices u32, comptes et préfixes du radix et w ; les offsets CSR sont u64. UINT32_MAX reste un PointId externe admis, distinct de la sentinelle des rangs denses (porte source cloud_test.cpp:281–290).
- Les passes stables PointId puis Morton trient bien (clé,ID) ; le contrôle des IDs dupliqués intervient après toutes les passes ID, avant les passes Morton. Skipper une passe constante conserve sa stabilité. Après regroupement, chaque retour unique appartient à une seule ligne CSR, IDs croissants, w égal à la longueur de ligne, poids total n. Pas d'ID dense substitué à l'identité externe.
- [morton.hpp](sources/morsehgp3D_v11/src/cloud/morton.hpp#L62) conserve les bits hauts : bloc bas de 21 bits, bloc haut décalé de63 ; clés u64 à B18/21, u128 à B24. Disjonction des masques et contrôle des vecteurs de base donnent une vraie justification de l'entrelacement dans le domaine. Le helper template autorise32 et sa formule peut porter96bits ; **Cloud/CoordWidth et les profils core restent18/21/24**, aucune qualification u32 ne découle de ce helper.
- Tous les tableaux de tri et de résultat passent par Buffer ; tampon inutilisé et histogrammes sont rendus avant l'allocation du résultat. Aucun vector d'entrée ou de scratch caché dans cette préparation. Le résultat possède ses propres tableaux : une mutation/destruction de l'entrée APRÈS retour ne laisse pas d'alias dans Cloud.

## Deux contrats utiles à fermer au raccord

**Emprunt pendant l'appel.** [cloud.cpp:136](sources/morsehgp3D_v11/src/cloud/cloud.cpp#L136) valide les spans, fill_records:143 conserve seulement clé/ID/index, puis fill_cloud:109–111 relit les coordonnées d'entrée à la fin. Le header ne dit pas explicitement que les quatre tableaux restent inchangés pendant tout l'appel. Choix simple : déclarer cet emprunt synchrone immuable, y compris vis-à-vis des hooks d'allocation réentrants ; ou valider une copie privée et remplir depuis elle. Un futur index doit aussi conserver un propriétaire Cloud stable/const.

Contre-scénario causal **non exécuté** si aucune précondition ne l'interdit : deux coordonnées x=(0,1) sont triées et classées comme deux sites ; le hook d'allocation nothrow du premier tableau résultat modifie x[0]=1. Le résultat garde deux groupes de l'ancienne clé mais remplit deux sites de même position. Modifier x[0] au-delà du domaine contournerait pareillement la validation initiale. Aucun fil concurrent ni data race n'est nécessaire dans ce scénario. Sous la précondition « entrée inchangée pendant l'appel », le scénario est exclu ; ce n'est donc pas une panne native reproduite ni l'ancien défaut de propriétaire déplacé.

**Pic de l'opération entière.** Le [pic publié](sources/morsehgp3D_v11/src/cloud/cloud.hpp#L82) est correct pour les allocations **supplémentaires de cloud** : max(2Rn+H, Rn+4n+24s+8), n retours, s sites ; H=65 536/73 728/81 920 pour B18/21/24, R déclaré16/16/32. Le remplissage peut dominer à B18/21 ; à B24 le tri domine toujours puisque s≤n. Les quatre buffers d'entrée vivants ajoutent16n, ainsi que tout résultat antérieur ou autre tampon vivant. Un refus restitue les allocations de cet appel, pas celles qui existaient déjà dans le budget. Préflight et tests de pic devront prendre ce niveau initial en compte.

| Modèle analytique n=s=50 000 | Pic cloud supplémentaire | Pic avec quatre buffers d'entrée vivants |
| --- | ---: | ---: |
| B18 ou B21 | 2 200 008 octets | 3 000 008 octets |
| B24 | 3 281 920 octets | 4 081 920 octets |

Ces valeurs supposent les tailles R annoncées par la source ; aucune ABI n'est mesurée ici. Ce ne sont ni des RSS, ni des chronos, ni une qualification massive : le régime multi-millions est hors contrat v11 actuel (ARCHITECTURE §7.2). [capacity.py](capacity.py) fait seulement ces calculs scalaires, sans charger le produit ni allouer un nuage ; ses sorties normal/−O sont identiques.

## Portes et précision : lecture, pas résultats acquis

Les tests écrits prévoient juge bit-à-bit indépendant de Morton, attentes manuelles, bornes d'axes, PointId maximal/dupliqué, permutations, renumérotation non monotone, neuf pannes d'allocation et deux phases du pic. Ils ne sont pas exécutés par cet audit. Deux ajouts courts utiles sur G4 : altérer/libérer l'entrée après retour et comparer tout Cloud ; appeler avec un budget déjà occupé et vérifier pic absolu/restitution du delta sur chaque refus.

Cloud contient seulement les coordonnées entières, w, CSR et poids ; il ne transporte ni width déclaré, pas h, origine ni hash de l'entrée. Le raccord IO doit conserver Descriptor+Cloud comme un même objet d'entrée identifiable, puis conserver la correspondance IDs→site avec les payloads externes. Cela relève du raccord, pas d'une erreur d'unité constatée dans une formule cloud. CoordWidth étroit contrôle seulement le domaine, sans réduire tailles de clés ni coûts du profil compilé.

Les hashes avant/après, la dépendance supplémentaire et SHA256SUMS pinent les fichiers réellement lus. Aucun fichier source, document développeur, note active ou ancien reçu n'est modifié.
