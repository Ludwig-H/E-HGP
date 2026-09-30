# Audit indépendant v10 — état courant

Mis à jour le **30 septembre 2026**, après relecture des parties I et II de la thèse. Réponses du développeur relues jusqu'à `bdc0b8f08`. Audit publié en `dc4915666`. `public_status=not_claimed`.

**À lire en premier.** Le [rapport de référence](AUDIT_INDEPENDANT_V10_20260929.md) porte sur la base figée `6206d1d118794c9e1cabb6faeaec2aaa77d37e5b`. Il conserve les constats prouvés sur cette base ; leur fermeture est suivie ici. Une correction annoncée ou préparée dans une copie n'est pas une correction intégrée et contre-vérifiée.

## Points ouverts et fermés

| Sujet | État utile au développeur |
| --- | --- |
| Portée des chronos, séquences, mémoire et grands K | Rectifications documentaires relues en `2aacfa2e5` et `695934464`. Elles ne qualifient pas un nouveau contrat. |
| I1/I2 — lecture complète et options CLI | Défauts reproduits sur la base ; deuxième préparation en cours. La garde produit M≥K+3 existe dans la copie relue, mais le diagnostic budgété et le parseur strict annoncés restent à recevoir. |
| P1 — exceptions du pool | Première copie : quatre cas appelant/ouvrier, directs/imbriqués, passent en Release et ASan/UBSan ; quiescence, TLS et réutilisation contrôlés. R2 : réclamations CAS saturantes, neuf sondes Release aux bornes u64 passent. Wrappers relus : bad_alloc devient resource_exhausted après nettoyage ; les autres exceptions sont propagées. Pas de clôture intégrée. |
| H1–H4 — validation, racine, domaine de z, coût de la tête | H1/H2/H4 ciblent les causes dans la première copie, sans clôture intégrée. H3 reste ouvert : petits niveaux positifs validés puis NaN/∞. Le domaine conjoint niveaux/poids et la porte H4 causale sont annoncés, absents des unités r2 à notre lecture. Le refus du singleton racine à zéro sous mcs est désormais accepté en bd8a9286f, à porter puis vérifier dans la tête. Aucun défaut sur les niveaux du producteur u18 déduit. |
| E1 — décision et fusion des bancs | Admission d'un lot incomplet reproduite ; le lot C archivé est complet. Première correction du plan préparée ; scores impossibles et course signal annoncés pour r2, encore absents des unités relues. |
| G1 — centre rationnel hors enveloppe dans SiteTree | Correction de la copie constatée par l'autre auditeur. La garde hors FE_TONEAREST annoncée n'est pas encore présente ; aucun appel produit courant exposé à G1 trouvé. |
| Juges catalogue et verticales FULL | Nouveau juge catalogue : mutants support non canonique et rang décalé rejetés ; doublons I/U et ordre publié encore non contrôlés dans la copie disponible. Verticales : naissances sans point, attachements et fusion simultanée à contrôler. Aucune intégration qualifiée. |
| Budget mémoire et plateaux double de la tête | Dettes acceptées par le développeur, ouvertes. |
| Lecteur de la sonde CUDA | Schéma renforcé en bd8a9286f, anciens résultats tronqués refusés. Contre-audit pur Python : nombre JSON énorme provoquant OverflowError et échec de lancement simulé sans attempt.json restent à fermer. Aucune exécution CUDA ni qualification GPU. |
| FULL → hiérarchie de points | Préserver couverture et masses des points frontière avant condensation. Majorité stricte à univers/poids fixes : laminarité confirmée, mais 1/β peut différer une couverture unique puis choisir une autre branche (cinq sites exacts). Comparer ancrage à la première couverture unique puis ascendance, bande K2 et majorité ; contrôler aussi rappel avant fusion et marges sous perturbation. |

Les réponses reçues sont [la note de prise en compte](NOTE_CLAUDE_AUDITS_CONTINU_ET_INDEPENDANT_20260929.md), [la réponse au rapport et aux addenda](REPONSE_CLAUDE_ADDENDA_ET_RAPPORT_INDEPENDANT_20260929.md), [le plan r2 et de raccord commun](REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md) [la réponse sur les zéros, le lecteur CUDA et la masse](REPONSE_CLAUDE_ZERO_ET_LECTEUR_CUDA_20260930.md) et [la prise en compte des cinq sites](REPONSE_CLAUDE_CINQ_SITES_ET_R2_20260930.md). La version publiée est `bdc0b8f08` ; les sources du moteur n'ont pas changé depuis `777406b82`. Les copies r2 relues séparément le 30 septembre ne constituent pas le binaire commun annoncé. Aucun test n'a été répété sur leurs unités inchangées. Les sources produit ne sont pas modifiées par cet audit.

Le [reçu de contre-vérification du pool](../receipts/audit_independant_20260929/contre_pool_preintegration/receipt.json) fige la première copie et ses binaires. Hash de `pool.cpp` : `db59f6c763dff698e92537340e8bed1cbb18344f343ac763c97a6189900698cf`, stable avant/après. Ce lot ne qualifie ni TSan, ni l'échec de création partielle de threads, ni les conversions des consommateurs publics. Le [reçu r2](../receipts/audit_independant_20260930/pool/source_status_20260930.json) ferme seulement la nouvelle arithmétique des tranches en Release ; les autres mécanismes sont inchangés et leurs preuves restent distinctes.

**Points frontière.** Le développeur a ajouté les cinq sites à la porte de conception et la variante « couverture unique, sinon majorité fixe » aux bras préenregistrés, en bdc0b8f08 ; aucune nouvelle campagne ni qualification statistique n’en découle. La définition 8, le théorème 2 et le §9.1 de la thèse rendent la couverture discrète et les masses d'incidence centrales. Retirer ou différer une observation avant condensation peut faire perdre une branche avant la fusion parasite, même si les hauteurs restent stables. [La note courante](audit_independant_20260929/ANCRAGE_AMBIGUITES.md) rassemble les références, le raccord possible des incidences au catalogue natif et les critères de comparaison. [La trace de lecture](../receipts/audit_independant_20260929/lecture_these/receipt.json) identifie le manuscrit et les pages relues.

## Dossier de lecture

- [Rapport de référence](AUDIT_INDEPENDANT_V10_20260929.md) : verdict complet, défauts et périmètres.
- [Géométrie et catalogue](audit_independant_20260929/GEOMETRIE_CATALOGUE.md), [FULL et points](audit_independant_20260929/TOUR_ET_POINTS.md), [tête et bancs](audit_independant_20260929/TETE_BANCS_PREUVES.md) : détails des constats de la base.
- [Sources statistiques vérifiées](audit_independant_20260929/tower_statistical_sources.md) : hypothèses et limites des garanties possibles.
- [Points frontière et ancrage des ambiguïtés](audit_independant_20260929/ANCRAGE_AMBIGUITES.md) : relecture de la thèse, incidences et masses, définition et marges du bras K2, panel de paires.
- [Contre-audit géométrie](audit_independant_20260929/CONTRE_AUDIT_GEOMETRIE.md) et [contre-audit tête/bancs](audit_independant_20260929/CONTRE_AUDIT_TETE_BANCS.md) : notes mises à jour en place pour chaque domaine.
- [Captures et sondes](../receipts/audit_independant_20260929/) : preuves conservées, y compris échecs et essais interrompus.
- [Captures du 30 septembre](../receipts/audit_independant_20260930/) : état des copies r2 et contre-exemple rationnel de majorité ; les captures antérieures restent inchangées.

## Entretien

Ce fichier est mis à jour en place. Chaque domaine reçoit au plus une note courante de contre-audit ; les anciennes conclusions restent attribuées à leur base. Les journaux, données, sondes et notes intermédiaires remplacées sont rangés dans `receipts/audit_independant_20260929/`, avec [manifest de déplacement](../receipts/audit_independant_20260929/RELOCALISATION.json) et empreintes. Les liens des rapports ont été adaptés ; aucune preuve n'a été supprimée.

Les fichiers de l'autre auditeur et les réponses de Claude gardent leur auteur. Leurs états courants sont des sources de coordination, pas des résultats de nos propres sondes.

L'[audit historique v9](../receipts/audit_v9_20260928/) et ses 425 fichiers ont également rejoint `receipts/`. [Le manifeste](../receipts/RELOCALISATION_AUDIT_V9_20260929.json) vérifie la conservation de tous les contenus ; seuls les trois liens de navigation ont changé.

La relocalisation et nos rapports sont publiés en `dc4915666`. Le développeur a commité ses liens de README/passation en `bd8a9286f` : la remise de navigation décrite dans le [reçu de publication](../receipts/audit_independant_20260930/VERIFICATION_PUBLICATION.json) est donc terminée. Ce reçu conserve les résultats du contrôle documentaire à sa base ; ses trois liens historiques manquants sont un constat distinct.
