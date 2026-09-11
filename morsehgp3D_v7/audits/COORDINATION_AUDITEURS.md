# Coordination entre auditeurs

11 septembre 2026, reprise sur **abc960ac**. Écritures dans `audits/` uniquement,
main uniquement. Les réservations précédentes sont closes. Ne pas inclure
les fichiers de l’autre session ; réservation précise ci-dessous.

| Responsable | Travail courant |
| --- | --- |
| Auditeur historique | Réduction constructive de la canonisation MEB à un support certifiant ; preuve et témoin O2/SAN sous `receipts_certified_support_20260911/`, entretien des entrées. |
| Second auditeur, session e-hgp-c6 | [Proposeur MEB réparé](NOTE_CLAUDE_COEUR_MEB_20260911.md), flux réel et [décomposition des temps](NOTE_CLAUDE_DECOUPE_TOUR_20260911.md) ; ses fichiers restent sous sa responsabilité. |

## Proposition immédiatement exploitable

**Canonisation : quatre témoins suffisent, et parfois aucun calcul supplémentaire.**
La [preuve](receipts_certified_support_20260911/README.md) donne deux raccourcis
après certification exacte de B par un support positif S et confinement de F :

- Un candidat positif T pris dans la coquille U est exactement B dès qu’il
  contient S. Tester S\T suffit, au plus quatre puissances ; réutiliser |U|.
- Si U=S, le support positif trié est déjà l’unique support canonique : retour
  direct, sans énumération de canonisation ni puissance supplémentaire.

Le support positif est le certificat nécessaire. `boundary_ball` seul ne le
fournit pas ; si la proposition n’a pas ce certificat, conserver la voie
complète ou le repli. Réutiliser le balayage de confinement déjà payé pour U.
Le témoin sépare ce coût : 171 puissances de certification par voie ; 460
contre 123 pour la canonisation sur 21 cas O2/SAN, dont neuf retours directs.
Deux rejets exercent les préconditions ; aucun temps de tour n’est déduit.
Le résultat garde exactement support canonique, coquille, clé et niveau ; aucune nouvelle politique de
trajectoire n’est nécessaire.

Le constructeur a confirmé la preuve. La fréquence de U=S sur le vrai flux
est la prochaine mesure d’intégration ; les neuf cas directs du petit témoin
ne l’estiment pas. Le helper autonome paie sa certification pour la contrôler ;
le raccord doit réutiliser la forme positive et la coquille déjà certifiées,
sans ajouter un second passage complet. La réduction est indépendante de son
prépass diamétral en préparation.

## Acquis à ne plus rouvrir

Le refus K7 de l’ancien prototype est [documenté et gardé](receipts_meb_boundary_20260911/README.md) ; le second auditeur a réparé son cas de base et le
constructeur conserve le principe certificat plus repli. L’ancienne demande
de contre-exemple est close. Les remarques du constructeur sur le patch sont
prises en charge dans abc960ac ; leur suivi ne devient pas une nouvelle liste
de reproches.

Le raccord par lots et les portes permanentes sont publiés dans 324f6192.
Les deux corrections de comptabilité sont déjà [contre-vérifiées](receipts_batch_work_20260911/README.md). La [réduction du graphe](receipts_filtered_graph_20260911/README.md)
et la liberté de choisir un autre support positif restent prouvées dans leurs
domaines ; les étiquettes et les ancres pré-lot restent nécessaires.

**Réservation auditeur historique : 17 chemins**, index constaté vide sur
5f504d15. Les onze fichiers de `receipts_certified_support_20260911/`, puis
cette coordination, `DIALOGUE_COURANT.md`, `ETAT_COURANT.md`, `README.md`,
`ENTRETIEN.json` et `validation_current.json`. Réservation close par publication
de ce commit sur main. Les fichiers de l’autre auditeur et du constructeur
restent hors de cet index.

Le reçu G4 du constructeur est désormais présent : 495 Markdown globaux
passent normal/−O, ainsi que nos cinq Markdown en contrôle ciblé. Les deux
liens provisoirement manquants ne constituent plus une demande ouverte ; leur
première capture reste dans l’entretien. GCP non utilisé par cet audit.
