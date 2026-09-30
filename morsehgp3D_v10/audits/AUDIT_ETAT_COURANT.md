# Audits v10 — état courant

Mise à jour : 30 septembre 2026, complément R2 et preuve des entrées
frontière internes K3/K5, après la réponse `e9eab2754`. Les parties I
et II de la thèse ont été relues intégralement dans la tranche précédente.
Index vivant de l'auditeur continu ; les rapports datés restent des preuves
ancrées à leur version, pas des statuts courants. `public_status=not_claimed`.
État du produit : [PASSATION](../PASSATION.md). Corrections de portée des
mesures : [ERRATA](../receipts/ERRATA.md). Ne pas réécrire les reçus clos.

## À lire maintenant

| Sujet | État actuel | Référence |
| --- | --- | --- |
| FULL → partitions de points | Frontière de la définition 8, core contrôle seulement. Majorité fixe laminaire, mais uniforme retarde deux groupes ; 1/β échoue sous contact coquille/intérieur, fusion FULL inchangée. Chaque point a une feuille couvrante à K2 ; contre-exemples sans feuille à K3/K5, deux exports natifs. Durée à explorer en conservant les continuations, pas nouveau choix produit. | [Relecture et contre-épreuves, sections 7–11](audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md), [certificat local](audit_continu_20260929/ADDENDUM_ANCRAGE_ET_CERTIFICAT_20260929.md) |
| Fixtures de projection | F1–F4 cohérentes. Notre Γ exact confirme 75 couples nuage/K ; campagnes développeur 10/10 closes, distinctes de notre test autonome. Aucun vote ni nouveau traitement frontière qualifié. | [Contre-audit des fixtures](audit_continu_20260929/CONTRE_AUDIT_FIXTURES_PROJECTION_20260929.md) |
| Retrouver toutes les couvertures | Lemme par composante : témoin p+q_min≤K, coquilles et intérieurs complets. 30 102 requêtes autonomes, puis 64 appels natifs K1–K4, huit géométries, nerf rationnel indépendant et listes complètes. Ne pas supprimer les fusions FULL p+q_min=K+1. Qualification native bornée, pas globale. | [Preuve, oracle et complément R2](audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md) |
| Sécurité du pool | R2 : réclamations CAS saturantes et série sans overflow, injections alignées. Logs clos ASan11/TSan10/sondes27 code0 observés. Campagne globale et différentiels non clos par cet audit ; intégration distincte. | [Complément R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [premiers correctifs](audit_continu_20260929/pool_head/CONTRE_AUDIT_POOL_CORRIGE_20260929.md) |
| Interfaces | Parseur strict sur copie R2. Nouveau défaut natif : mêmes destinations étiquettes/arbre → texte écrasant les labels, code0/status ok. Préserver contrôle des écritures et propagation des nouveaux refus de tête lors du raccord. | [Collision et raccord R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [premiers correctifs](audit_continu_20260929/catalogue/CONTRE_AUDIT_INTERFACES_CORRIGEES_20260929.md) |
| SiteTree et centres rationnels | R2 : filtre réservé à FE_TONEAREST du thread appelant, modes dirigés en repli exact. Porte quatre modes rejouée code0, 5 969 requêtes/mode. Ancien harnais code1 historique. FTZ/DAZ et autres filtres FULL non qualifiés ; représentation et emprunt Cloud restent des préconditions. | [Complément SiteTree R2](audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md) |
| Tête numérique | R2 : garde conjointe, racine zéro protégée même sous mcs, comparaison exacte de M·λ_max. Porte native rejouée ici code0 : 26/26 domaines, 25/25 invalides sans sortie. Refus à propager dans la CLI commune ; pas encore intégré. | [Contrôle R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [défauts et borne d'origine](audit_continu_20260929/pool_head/CONTRE_AUDIT_TETE_NUMERIQUE_20260929.md) |
| Juges catalogue/FULL | Petits juges R2 renforcés contre-vérifiés. Nouveau lecteur structurel des grands dumps : ordre K entier manquant ou coordonnées d'attaches inconnues acceptés sur fixtures ; contrôles linéaires à ajouter. Aucun dump LiDAR réellement fautif observé. | [Compléments R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [angles morts d'origine](audit_continu_20260929/catalogue/CONTRE_AUDIT_JUGES_CORRIGES_20260929.md) |
| Bancs et arrêt des calculs | R2 refuse maintenant ARI1,25. 44 nouveaux appels courts : alpha=2/NaN et en-tête ari_s dupliqué admis ; config absente/inconnue finit en KeyError. Schéma à valider, A/C historiques non réfutés. 120 vrais signaux POSIX locaux observés distingués des simulations. | [Complément des bancs R2](audit_continu_20260929/timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md) |
| Prototypes CPU | J3 réduit le CPU de t_boxes ×1,37–1,55, mêmes comptes ; mutant survivant équivalent par parité. 1 060 cas conclusifs et dix délais observés, TSan frontière v3b terminé. Gain p1c CPU total FULL K5 seulement 2,5 % sur le lot local ; variantes non combinées sur G4. | [Contre-audit CPU, périmètres et preuves](audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md) |
| Aval ordre/tête | Contre-audit nouvelle copie : validation parallèle CSR hors bornes sur objet public forgé, alors que série refuse. Temps mur local ordre+assemblage réduits, sans preuve GPU/FULL 100 ms. Gate nouvelle copie 9/9 réellement close, défaut CSR toujours reproductible ; refus et interruptions séparés des cas conclusifs. | [Contre-audit ordre/tête](audit_continu_20260929/performance/CONTRE_AUDIT_ORDRE_TETE_CORRIGE_20260929.md) |
| CUDA | Unsigned accepté ; contrôle hôte UBSan propre. Statuts et durées corrigés dans 779dd38a9, mais lecteur d'enveloppe seulement : vingt entrées et huit simulations en précisent les limites. Débits historiques signés invalides, aucun nouveau reçu GPU ni port FULL GPU qualifié. | [Sonde corrigée](audit_continu_20260929/timeout/CONTRE_AUDIT_SONDE_CORRIGEE.md), [statuts et échecs](audit_continu_20260929/ADDENDUM_ZERO_ET_STATUTS_20260930.md), [errata](../receipts/ERRATA.md) |
| G4 et passage à l'échelle | FULL K5 sans attaches mesuré à 204–254 ms sur trois trames sans sol d'une seule séquence ; CPU, non GPU. Ni 100 ms ni plusieurs séquences qualifiés. | [Recalcul des mesures](audit_continu_20260929/timeout/AUDIT_ECHELLE.md) |

La borne locale K2 demande une marge stricte autour du seuil d'ambiguïté.
Elle garantit des dates sous perturbations appariées, pas l'ARI, l'EOM ni
une généralisation K5. Différer des points jusqu'à la fusion peut perdre
le rappel frontière recherché : mesurer leur récupération avant connexion
parasite, pas seulement les hauteurs. La borne de packing des voisins ne
borne pas les paires de voisins, les q3/q4 ou les visites de l'index.

Les [trois verrous transmis au développeur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md)
priorisent le choix de masse frontière, la validation CSR et une qualification
sur un binaire réellement intégré. La réponse `e9eab2754` retient ces choix,
mais leur raccord reste à auditer. Ajouter le diagnostic de masse fractionnaire
du chapitre 9 avant condensation. Les petits contre-exemples ne sont pas
une autorisation d'élargir les optimisations sans signal utile.

Le [nouveau complément R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md)
transmet la collision de sortie, le raccord des refus et les gardes de
schéma peu coûteuses. La section 10 de
la note mathématique interdit de considérer `1/β` comme solution robuste
déjà acquise. L'attache réellement unique avec marge et le diagnostic de
durée couverte restent des bras limités, pas des choix produit qualifiés.
La section 11 impose de traiter les entrées frontière internes à K3/K5
et de conserver la masse des continuations ; elle prouve à K2 au moins une
incidence de feuille par point, pas toutes ses composantes couvrantes.
Le succès SiteTree R2 est clos seulement
à son périmètre, pas comme contrat FENV de toute la tour.

Les preuves natives de couverture utilisent l'archive `6206d1d11` ; les
preuves de consommateurs utilisent la copie corrigée du pool. Elles ne
constituent pas ensemble une qualification d'un unique binaire intégré.

## Références historiques — ne pas confondre avec les travaux actifs

- [Archive de l'audit v9](../receipts/audit_v9_20260928/README.md) : origine de la refonte v10,
  avec sources et contre-vérifications. Ne qualifie pas automatiquement v10.
- [Audit hiérarchie k-NN](audit_hierarchie_knn_20260929/AUDIT_HIERARCHIE_KNN_20260929.md) :
  étude antérieure de l'objet et de la tête, à lire avec les objections ci-dessus.
- [Trois pistes multi-K](tete_multik_20260929/README.md) : expériences et juge,
  pas trois implémentations recommandées ni une solution à la laminarité.
- Les essais interrompus, mutants et échecs des sous-dossiers de preuves
  sont conservés pour la traçabilité ; ils ne sont pas des qualifications vivantes.

## Rangement et coordination

Une seule vue courante : ce fichier. Une réponse cite le constat et indique
son état — ouvert, corrigé à relire, ou clos — sans reproduire son rapport.
Pas de copies de sources, builds, journaux ou reçus entre dossiers d'audits.
Le prochain état remplace cette vue ; il n'ajoute pas un nouvel index concurrent.

Le point d'entrée `audits/README.md` a été actualisé par son propriétaire ;
je ne le modifie pas. L'audit indépendant et l'archive v9 ont été publiés
dans `dc4915666`, puis la navigation dans `bd8a9286f`. Les travaux encore
locaux des autres acteurs sont préservés et exclus de notre publication.
Nos captures brutes sont
dans `receipts/audit_continu_20260929/`, avec relocalisation à hashes identiques
publiée dans `c5015a570`. Les nouveaux petits tests y restent aussi. Les
rapports utiles sont conservés dans `audits/`, sans copie de journaux ou
de builds. Les fichiers des autres intervenants restent sous leur contrôle.
GCP non utilisé dans cette tranche ; aucun contrat ou statut public promu.
