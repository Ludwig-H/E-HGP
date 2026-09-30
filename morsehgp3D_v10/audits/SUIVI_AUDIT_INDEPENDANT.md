# Audit indépendant v10 — état courant

Mis à jour le **30 septembre 2026**, avec le contrat massif demandé par l'utilisateur et les progrès R2 publiés par l'autre auditeur en `a6b380e9c`. Réponses du développeur relues jusqu'à `bdc0b8f08`. Audits déjà publiés en `dc4915666` et `bc07fa1db`. `public_status=not_claimed`.

**À lire en premier.** Le [rapport de référence](AUDIT_INDEPENDANT_V10_20260929.md) porte sur la base figée `6206d1d118794c9e1cabb6faeaec2aaa77d37e5b`. Il conserve les constats prouvés sur cette base ; leur fermeture est suivie ici. Une correction annoncée ou préparée dans une copie n'est pas une correction intégrée et contre-vérifiée.

## Points ouverts et fermés

| Sujet | État utile au développeur |
| --- | --- |
| Portée des chronos, séquences, mémoire et grands K | Rectifications documentaires relues en `2aacfa2e5` et `695934464`. Elles ne qualifient pas un nouveau contrat. |
| I1/I2 — lecture complète et options CLI | Défauts reproduits sur la base ; deuxième préparation en cours. La garde produit M≥K+3 existe dans la copie relue, mais le diagnostic budgété et le parseur strict annoncés restent à recevoir. |
| P1 — exceptions du pool | Première copie : quatre cas appelant/ouvrier, directs/imbriqués, passent en Release et ASan/UBSan ; quiescence, TLS et réutilisation contrôlés. R2 : réclamations CAS saturantes, neuf sondes Release aux bornes u64 passent. Wrappers relus : bad_alloc devient resource_exhausted après nettoyage ; les autres exceptions sont propagées. Pas de clôture intégrée. |
| H1–H4 — validation, racine, domaine de z, coût de la tête | Première capture conservée ; progrès R2 désormais contre-vérifiés par l'autre auditeur : domaine conjoint, produit M·λ maximal comparé exactement et racine zéro protégée, 26 fixtures valides et 25 refus. Porte H4 et mutants observés dans ses journaux, sans nouveau rejeu ici. La fermeture sur le binaire commun reste distincte. |
| E1 — décision et fusion des bancs | Admission d'un lot incomplet reproduite ; le lot C archivé est complet. Première correction du plan préparée ; scores impossibles et course signal annoncés pour r2, encore absents des unités relues. |
| G1 — centre rationnel hors enveloppe dans SiteTree | Correction de la copie constatée par l'autre auditeur. La garde hors FE_TONEAREST annoncée n'est pas encore présente ; aucun appel produit courant exposé à G1 trouvé. |
| Juges catalogue et verticales FULL | R2 : l'autre auditeur a rejoué normal/−O cinq nouveaux refus de listes/ordre, l'attache morte et le plateau ternaire binarisé, avec contrôles positifs. Sa campagne observée est terminale 33/33. Ce progrès ferme les causes dans la copie jugée, sans intégration commune qualifiée ni rejeu produit nouveau de notre part. |
| Budget mémoire et plateaux double de la tête | Dettes acceptées par le développeur, ouvertes. |
| Lecteur de la sonde CUDA | Schéma renforcé en bd8a9286f, anciens résultats tronqués refusés. Contre-audit pur Python : nombre JSON énorme provoquant OverflowError et échec de lancement simulé sans attempt.json restent à fermer. Aucune exécution CUDA ni qualification GPU. |
| FULL → hiérarchie de points | Préserver couverture et masses des points frontière avant condensation. Majorité stricte à univers/poids fixes : laminarité confirmée, mais couverture unique différée dans les cinq sites. Le nouveau contre-exemple de coquille de l'autre auditeur exige aussi une marge sur l'admission des atomes. Comparer ancrage unique puis ascendance, bande K2 et majorité ; rappel avant fusion et robustesse restent à qualifier. |
| LiDAR massif, dizaines de millions | [Audit courant](AUDIT_MASSIF_LIDAR_20260930.md) : volume de sortie et états simultanés, conversion B→u32 sans garde, milieu de dichotomie au-delà de 2³¹ niveaux, étendue u18 de 262,143 m à 1 mm. Voie proposée : boîtes de centres certifiées, segments triés et fusion exacte ; atlas, plateaux, verticales, points et reprise restent à concevoir. Aucune capacité massive qualifiée. |

Les réponses reçues sont [la note de prise en compte](NOTE_CLAUDE_AUDITS_CONTINU_ET_INDEPENDANT_20260929.md), [la réponse au rapport et aux addenda](REPONSE_CLAUDE_ADDENDA_ET_RAPPORT_INDEPENDANT_20260929.md), [le plan r2 et de raccord commun](REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md), [la réponse sur les zéros, le lecteur CUDA et la masse](REPONSE_CLAUDE_ZERO_ET_LECTEUR_CUDA_20260930.md) et [la prise en compte des cinq sites](REPONSE_CLAUDE_CINQ_SITES_ET_R2_20260930.md). La dernière réponse est `bdc0b8f08` ; les sources du moteur n'ont pas changé depuis `777406b82`. Les copies r2 relues séparément le 30 septembre ne constituent pas le binaire commun annoncé. Aucun test n'a été répété sur leurs unités inchangées. Les sources produit ne sont pas modifiées par cet audit.

Le [contre-audit R2 de l'autre auditeur](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), publié en `a6b380e9c`, actualise les états tête et juges ci-dessus. Il montre aussi une collision de chemins de sorties CLI qui écrase les étiquettes tout en rendant `ok`. Les anciennes captures de nos copies restent attribuées à leur heure et leurs hashes ; elles ne doivent pas faire rouvrir les causes depuis réparées dans une autre copie. Le raccord doit recevoir ensemble la tête, les refus et les écritures contrôlées.

Le [reçu de contre-vérification du pool](../receipts/audit_independant_20260929/contre_pool_preintegration/receipt.json) fige la première copie et ses binaires. Hash de `pool.cpp` : `db59f6c763dff698e92537340e8bed1cbb18344f343ac763c97a6189900698cf`, stable avant/après. Ce lot ne qualifie ni TSan, ni l'échec de création partielle de threads, ni les conversions des consommateurs publics. Le [reçu r2](../receipts/audit_independant_20260930/pool/source_status_20260930.json) ferme seulement la nouvelle arithmétique des tranches en Release ; les autres mécanismes sont inchangés et leurs preuves restent distinctes.

**Points frontière.** Le développeur a ajouté les cinq sites à la porte de conception et la variante « couverture unique, sinon majorité fixe » aux bras préenregistrés, en bdc0b8f08 ; aucune nouvelle campagne ni qualification statistique n’en découle. La définition 8, le théorème 2 et le §9.1 de la thèse rendent la couverture discrète et les masses d'incidence centrales. Retirer ou différer une observation avant condensation peut faire perdre une branche avant la fusion parasite, même si les hauteurs restent stables. [La note courante](audit_independant_20260929/ANCRAGE_AMBIGUITES.md) rassemble les références, le raccord possible des incidences au catalogue natif et les critères de comparaison. [La trace de lecture](../receipts/audit_independant_20260929/lecture_these/receipt.json) identifie le manuscrit et les pages relues.

## Dossier de lecture

- [Rapport de référence](AUDIT_INDEPENDANT_V10_20260929.md) : verdict complet, défauts et périmètres.
- [LiDAR de plusieurs dizaines de millions](AUDIT_MASSIF_LIDAR_20260930.md) : obstacles de représentation, dimensionnement conditionnel et voie exacte par segments ; [captures dédiées](../receipts/audit_independant_20260930/massif/).
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
