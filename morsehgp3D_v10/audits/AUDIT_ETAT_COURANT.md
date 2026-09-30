# Audits v10 — état courant

Mise à jour : 30 septembre 2026, clôtures R2 observées, raccord CLI et
proposition de rejet par blocs, après la réponse `e9eab2754`. Les parties I
et II de la thèse ont été relues intégralement dans la tranche précédente.
Index vivant de l'auditeur continu ; les rapports datés restent des preuves
ancrées à leur version, pas des statuts courants. `public_status=not_claimed`.
État du produit : [PASSATION](../PASSATION.md). Corrections de portée des
mesures : [ERRATA](../receipts/ERRATA.md). Ne pas réécrire les reçus clos.

**Reprise du développement au 30 septembre, après cette revue :** l'utilisateur
repasse l'auditeur continu développeur et demande une précision supérieure
à u18. Le [suivi actif](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md)
décrit le correctif RankIndex intégré et le nouveau candidat exact par bande.
Les régressions natives de ce candidat restent u18 et bornées ; la tête
statistique, le profil large et G4 ne sont pas qualifiés. Les correctifs R2
des autres acteurs restent à intégrer séparément.
Deux relectures indépendantes valident la première brique u32 isolée
(distance u128, Morton96, 212 684 contrôles). Elle ne qualifie pas les
supports q3/q4, le catalogue, la tour ou le GPU à ce nouveau domaine.

**Port précis, contre-épreuves du même jour :** relever seulement le refus
u18 serait incorrect. Les corps géométriques donnent déjà un rayon q4
tronqué à u21 ; à u24, un rayon devient zéro sans diagnostic UBSan.
Les filtres de contacts doivent aussi changer, pas seulement leurs entiers.
Les [preuves séparées](../receipts/audit_continu_20260929/precision_port_20260930/README.md)
conservent les erreurs attendues, les contrôles positifs et un préflight
scalaire rejeté. Elles ne démontrent aucun défaut du profil u18 protégé.

## À lire maintenant

| Sujet | État actuel | Référence |
| --- | --- | --- |
| FULL → partitions de points | Frontière de la définition 8, core contrôle seulement. Majorité fixe laminaire, mais uniforme retarde deux groupes ; 1/β échoue sous contact coquille/intérieur, fusion FULL inchangée. Chaque point a une feuille couvrante à K2 ; contre-exemples sans feuille à K3/K5, deux exports natifs. Durée à explorer en conservant les continuations, pas nouveau choix produit. | [Relecture et contre-épreuves, sections 7–11](audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md), [certificat local](audit_continu_20260929/ADDENDUM_ANCRAGE_ET_CERTIFICAT_20260929.md) |
| Fixtures de projection | F1–F4 cohérentes. Notre Γ exact confirme 75 couples nuage/K ; campagnes développeur 10/10 closes, distinctes de notre test autonome. Aucun vote ni nouveau traitement frontière qualifié. | [Contre-audit des fixtures](audit_continu_20260929/CONTRE_AUDIT_FIXTURES_PROJECTION_20260929.md) |
| Retrouver toutes les couvertures | Lemme par composante : témoin p+q_min≤K, coquilles et intérieurs complets. 30 102 requêtes autonomes, puis 64 appels natifs K1–K4, huit géométries, nerf rationnel indépendant et listes complètes. Ne pas supprimer les fusions FULL p+q_min=K+1. Qualification native bornée, pas globale. | [Preuve, oracle et complément R2](audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md) |
| Sécurité du pool | R2 : CAS saturant et série sans overflow. Deux différentiels clos 24/24, oracles 2/2 ; comparateurs limités à des préfixes SHA96/64bits. Timeout d'une sonde à barrière distinct d'un deadlock démontré. Copies non intégrées. | [Complément R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [premiers correctifs](audit_continu_20260929/pool_head/CONTRE_AUDIT_POOL_CORRIGE_20260929.md) |
| Interfaces | Parseur strict sur copie R2. Nouvelle CLI tête : refus numériques propagés, mais mêmes destinations étiquettes/arbre → texte écrasant les labels, code0. Quatre sondes closes, raccord avec écritures vérifiées encore en chantier. | [Collision et raccord R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [premiers correctifs](audit_continu_20260929/catalogue/CONTRE_AUDIT_INTERFACES_CORRIGEES_20260929.md) |
| SiteTree et centres rationnels | Filtre limité à FE_TONEAREST du thread appelant ; quatre modes rejoués code0. Nouveaux différentiels 7/7 et 18/18, CTest11/11 clos. Huit mutants survivent, dont le contournement du chemin dirigé : réserve du juge, pas défaut actuel démontré. FTZ/DAZ et FULL restent hors portée. | [Complément SiteTree R2](audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md), [nouvelles clôtures](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md) |
| Tête numérique | R2 protège racine zéro et M·λ_max ; notre porte native antérieure passe. Quatre nouvelles sondes confirment Outcome dans la CLI tête et un refus tardif avant écriture dans la même hiérarchie. Ancienne tête dans la copie CLI stricte ; union non qualifiée. | [Contrôle R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [défauts et borne d'origine](audit_continu_20260929/pool_head/CONTRE_AUDIT_TETE_NUMERIQUE_20260929.md) |
| Juges catalogue/FULL | Petits juges R2 renforcés contre-vérifiés. Nouveau lecteur structurel des grands dumps : ordre K entier manquant ou coordonnées d'attaches inconnues acceptés sur fixtures ; contrôles linéaires à ajouter. Aucun dump LiDAR réellement fautif observé. | [Compléments R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [angles morts d'origine](audit_continu_20260929/catalogue/CONTRE_AUDIT_JUGES_CORRIGES_20260929.md) |
| Bancs et arrêt des calculs | R2 refuse maintenant ARI1,25. 44 nouveaux appels courts : alpha=2/NaN et en-tête ari_s dupliqué admis ; config absente/inconnue finit en KeyError. Schéma à valider, A/C historiques non réfutés. 120 vrais signaux POSIX locaux observés distingués des simulations. | [Complément des bancs R2](audit_continu_20260929/timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md) |
| Prototypes CPU | J3 réduit le CPU de t_boxes ×1,37–1,55, mêmes comptes ; mutant survivant équivalent par parité. 1 060 cas conclusifs et dix délais observés, TSan frontière v3b terminé. Gain p1c CPU total FULL K5 seulement 2,5 % sur le lot local ; variantes non combinées sur G4. | [Contre-audit CPU, périmètres et preuves](audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md) |
| Aval ordre/tête | Contre-audit nouvelle copie : validation parallèle CSR hors bornes sur objet public forgé, alors que série refuse. Temps mur local ordre+assemblage réduits, sans preuve GPU/FULL 100 ms. Gate nouvelle copie 9/9 réellement close, défaut CSR toujours reproductible ; refus et interruptions séparés des cas conclusifs. | [Contre-audit ordre/tête](audit_continu_20260929/performance/CONTRE_AUDIT_ORDRE_TETE_CORRIGE_20260929.md) |
| CUDA | Unsigned accepté ; contrôle hôte UBSan propre. Statuts et durées corrigés dans 779dd38a9, mais lecteur d'enveloppe seulement : vingt entrées et huit simulations en précisent les limites. Débits historiques signés invalides, aucun nouveau reçu GPU ni port FULL GPU qualifié. | [Sonde corrigée](audit_continu_20260929/timeout/CONTRE_AUDIT_SONDE_CORRIGEE.md), [statuts et échecs](audit_continu_20260929/ADDENDUM_ZERO_ET_STATUTS_20260930.md), [errata](../receipts/ERRATA.md) |
| G4 et passage à l'échelle | FULL K5 sans attaches mesuré à 204–254 ms sur trois trames sans sol d'une seule séquence ; CPU, non GPU. Ni 100 ms ni plusieurs séquences qualifiés. | [Recalcul des mesures](audit_continu_20260929/timeout/AUDIT_ECHELLE.md) |
| Plusieurs dizaines de millions | RankIndex corrigé dans le produit : milieu par différence et `lo*64` élargi avant clamp, 36 047 contrôles virtuels et deux mutants tués. Cela ne qualifie pas la capacité massive, les autres conversions ou la nouvelle précision. Certificat Q×Z encore à implémenter. | [Développement et portée](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md), [audit massif indépendant](AUDIT_MASSIF_LIDAR_20260930.md) |
| Précision au-delà de u18 | Morton96/distance u128 isolés acquis ; q3/q4/niveaux/propriétaire et filtres encore à porter. Contre-exemples natifs et scalaires clos hors domaine actuel. Préparateur décimal exact disponible ; consommateur v10 des pas, origines et IDs manquant. | [Audit du port précis](../receipts/audit_continu_20260929/precision_port_20260930/README.md), [ordre de développement](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md) |

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

Observation de conception, vers 05:24 UTC : la porte frontière en chantier
reprend nos entrées internes K3/K5 en G7 ; G8 vérifie le dédoublonnage de
deux boules couvrantes d'une même composante. Les sources ont évolué pendant
la lecture : aucune exécution ni qualification nouvelle de cette porte
par notre audit. Les bras actuels n'utilisent pas de poids de durée ; le
cas de continuation devient une garde à ajouter si cette piste est retenue.
Le contact inverseβ reste un contre-test, pas une preuve de robustesse.

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
