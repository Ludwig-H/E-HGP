# Audits v10 — état courant

Mise à jour : 30 septembre 2026, ancrage persistant en flux,
exceptions des sorties et limites des nouveaux raccords. Le développeur reste
sur `408d1ffe4`, moteur inchangé dans cette tranche. Les parties I
et II de la thèse ont été relues intégralement dans la tranche précédente.
Index vivant de l'auditeur continu ; les rapports datés restent des preuves
ancrées à leur version, pas des statuts courants. `public_status=not_claimed`.
État du produit : [PASSATION](../PASSATION.md). Corrections de portée des
mesures : [ERRATA](../receipts/ERRATA.md). Ne pas réécrire les reçus clos.

**Rôle courant : audit, sur la nouvelle instruction utilisateur.** Aucun
changement du moteur dans cette reprise ; prototypes de preuve isolés,
revue des raccords et suivi du développeur seulement. Plus tôt le même
jour, l'utilisateur avait demandé la reprise du développement et une
précision supérieure à u18. Le
[suivi actif](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md)
décrit le correctif RankIndex intégré et le nouveau candidat exact par bande.
Les régressions natives de ce candidat restent u18 et bornées ; la tête
statistique, le profil large et G4 ne sont pas qualifiés. Les correctifs R2
des autres acteurs restent à intégrer séparément.
Deux relectures indépendantes valident la première brique u32 isolée
(distance u128, Morton96, 212 684 contrôles). Elle ne qualifie pas les
supports q3/q4, le catalogue, la tour ou le GPU à ce nouveau domaine.

**Nouveau verrou de tête, confirmé :** les départs de points directement
attachés ne déclenchent pas le contrôle de masse `min_cluster_size`.
Le [contre-exemple clos](../receipts/audit_continu_20260929/point_condensation_20260930/README.md)
reproduit un changement EOM sur le vrai C++, racine exclue, mcs5.
HDBSCAN réel et deux oracles exacts indépendants confirment la correction.
La réalisation 3D des arbres API précis reste hors preuve ; les scores
historiques A/C ne sont pas invalidés sans rejeu. Corriger et ajouter cette
porte avant d'interpréter de nouvelles expériences frontière/statistiques.
Le [complément géométrique séparé](../receipts/audit_continu_20260929/point_condensation_cover_r2_20260930/README.md)
matérialise l'export natif historique d'un vrai nuage à six sites K3,
recoupé contre Γ3 Fraction. La tête actuelle y surestime aussi la stabilité
à mcs6, mais ne change pas EOM ni les étiquettes. Deux nouvelles invocations
de tête normal/UBSan, aucun nouvel appel générateur : ne pas confondre ce
témoin géométrique de score avec le renversement EOM des arbres API.

**Développeur actif :** la [note Claude](NOTE_CLAUDE_REPRISE_ET_PRECISION_20260930.md)
confirme la grille u32 par paliers, u24 puis u32 ; float32 natif différé.
Sa campagne Release CPU est réellement terminée : 11/11 portes hors
oracles et 2/2 oracles, sur sources stables. Elle reste u18 et n'intègre
pas les copies R2 ; ce n'est ni un port large ni une qualification G4.

**Piste q3/q4 utile :** le [crédit quantitatif par moments de groupe](../receipts/audit_continu_20260929/group_moments_20260930/README.md)
certifie plusieurs intérieurs sans témoin individuellement universel.
Deux vrais supports 3D en donnent quatre pour q3 et trois pour q4, au-delà
des seuils tight K5. 5 793 contrôles Fraction normal/−O et deux erreurs
logiques causales ; pas de port natif ni de gain LiDAR acquis. Tester un
petit nombre de groupes préparés une fois, masques q3/q4 de supports
seulement : ne pas retirer ces sites du census ni déplacer le carré.

Le [complément boîtes réelles](../receipts/audit_continu_20260929/group_moments_box_r2_20260930/README.md)
ajoute un témoin cubique, quatre crédits q4 de trois sans dominance
individuelle. Mais les petites entrées 14/16 sites s'arrêtent dès la
première feuille M16 : ce n'est pas la boîte des témoins. Si l'ancre est
dans la boîte fermée, σ≥0 et le certificat ne peut rien rejeter ; sauter
ce calcul. Le recadrage interdit aussi de supposer les feuilles toujours
presque cubiques. 503 contrôles Fraction normal/−O, aucun appel générateur.
Prochain essai : vraies listes S/candidats, ancres hors S et coût total.

**Frontière, piste plus légère :** le [contre-audit des témoins redondants](../receipts/audit_continu_20260929/antichain_counterreview_20260930/README.md)
confirme un vrai changement de partition à K2/η1/8 sur quatre points,
réunion 0/3 avancée de β25 à 200/9 ; Γ2 et univers fort complets Fraction.
Les six exports antérieurs n'avançaient aucune des 434 hauteurs contrôlées.
Surtout, l'antichaîne n'a pas besoin d'être triée ou stockée : deux extrêmes
Euler donnent exactement son LCA, un balayage des incidences puis au plus
une LCA par point. 2 892 bandes et 6 227 sélections supplémentaires,
normal/−O concordants. Réductions parallélisables ; ni port natif, gain
LiDAR/EOM/ARI, borne du nombre d'incidences ou chrono G4 acquis.

**Piste robuste maintenant prioritaire :** le mémo privé `ancrage_marges`
propose Pκ, qui retarde l'attache selon la persistance des branches
concurrentes. À K fixé, sa preuve tient : déplacements appariés ≤ε,
dates `(1+2κ)ε`, hauteurs `(1+4κ)ε`, en rayon. Aucun résultat EOM/ARI
ou ajout/retrait de sites n'en découle. Pour κ≥2, cutoff exact β≤4α².
Notre [simplification en flux](../receipts/audit_continu_20260929/persistent_anchor_stream_20260930/README.md)
supprime aussi le tri de l'antichaîne et le code-barres par point :
LCA préfixe de toutes les incidences fortes, puis maximum pénalisé,
propriétaire conservé depuis toute la première cohorte. Oracles abstraits,
pas nouveau moteur. Le D réellement parcouru et les LCA restent payés ;
aucune borne sous-quadratique du générateur ni performance acquise.
La bande non saturée reste instable même avec une marge au bord :
une branche courte parasite dans la fenêtre suffit. L'optimisation exacte
de son calcul n'est donc pas une justification de sa robustesse.

L'[alternative quadratique Qκ](../receipts/audit_continu_20260929/quadratic_anchor_rule_20260930/README.md)
évite les sommes de racines, avec une preuve de stabilité conditionnelle
à l'entrelacement couvrant. Mais elle retarde davantage que Pκ au même
paramètre et demande un ordre rationnel plus large. C'est une ablation
bornée proposée, non une refonte ni une meilleure qualité démontrée.

**Deux défauts ciblés nouveaux :** le [juge de précision](../receipts/audit_continu_20260929/precision_reader_orientation_20260930/README.md)
accepte les orientations manquantes ou fausses, code0 normal/−O ;
renforcer inventaire et exigences. Le [helper OutputSet](../receipts/audit_continu_20260929/outputset_exception_20260930/README.md)
du clone de raccord fuit un descripteur après `bad_alloc` ou exception
du writer. Deux microcaptures natives normal/UBSan identiques, contrôle
sans exception et sentinelles privées ; RAII du FILE avant toute opération
qui peut lever. Ces captures ne prouvent pas de fichier utilisateur perdu.
Les sept correctifs réunis textuellement n'ont toujours pas de binaire
commun qualifié retrouvé ; tests santé sur l'ancien HEAD et deux builds
partiels ne s'additionnent pas. Le défaut de condensation reste présent.
Le [suivi au développeur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md)
distingue aussi condensation terminale/progressive et hypothèses statistiques.

**Massif, garde globale distincte :** `ExtCell::rep_first` indexe une seule
arène `ext_reps` pour tous les K, mais son cast et l'addition à l'accès
restent u32 (`tower.cpp:1290/1024`). Les gardes par ordre `sr[k]` et sur
le nombre de cellules ne bornent pas cette somme globale. Élargir décalage
et addition, ou refuser avant insertion/conversion. Réserve d'adressage
pour le futur massif, pas contre-exemple géométrique exécuté ; la preuve
indépendante protégeant forêt/CSR actuelle reste correcte.

**Port précis, contre-épreuves du même jour :** relever seulement le refus
u18 serait incorrect. Les corps géométriques donnent déjà un rayon q4
tronqué à u21 ; à u24, un rayon devient zéro sans diagnostic UBSan.
Les filtres de contacts doivent aussi changer, pas seulement leurs entiers.
Les [preuves séparées](../receipts/audit_continu_20260929/precision_port_20260930/README.md)
conservent les erreurs attendues, les contrôles positifs et un préflight
scalaire rejeté. Elles ne démontrent aucun défaut du profil u18 protégé.

**Réparation isolée après ces contre-épreuves :** le
[filtre relatif certifié](../receipts/audit_continu_20260929/relative_filter_20260930/README.md)
passe 3 600 requêtes Fraction normal/UBSan, dont 196 contacts et 40
translations bit à bit ; deux mutants code0 sont rejetés mathématiquement.
La contre-relecture du juge conserve R1/R2/R3 et leurs limites. Ce n'est
pas un nouveau nearest natif, un repli exact ni un port FULL large ;
parallélisation, croissance et G4 restent non qualifiés.
Le [lecteur renforcé](../receipts/audit_continu_20260929/relative_filter_reader_r2_20260930/README.md)
vérifie les hashes avant import et les ensembles d'empreintes obligatoires,
sans modifier la première clôture.

**Ordre des niveaux larges :** le
[comparateur entier isolé](../receipts/audit_continu_20260929/level_order_20260930/README.md)
passe 2 444 requêtes normal/UBSan et tue les deux mutants numériques.
Les 729 comparaisons géométriques sont 27² couples de niveaux Python,
pas un constructeur natif qualifié. Le port doit aussi reprendre les
seuils, les distances K-NN sur 66 bits et les exports de rangs exacts ;
la table double de points fusionne délibérément certains niveaux distincts.
Ni le tri natif large, FULL, croissance ou performance G4 ne sont acquis.

**Compléments indépendants publiés dans `782e0d2a0` :**
[Morton96](../receipts/audit_independant_20260930/grid32_followup/README.md)
conserve bien les coordonnées dans un contexte de grille ; une translation
commune peut inverser ses rangs sans changer les distances. Ne pas employer
ces rangs comme IDs persistants entre origines. Le
[croisement K2/K3](../receipts/audit_independant_20260930/cover_band_followup/README.md)
des partitions de points ne réfute ni FULL ni la laminarité à K fixé.
L'utilisateur demande actuellement une hiérarchie depuis le seul arbre
de niveau K : ce croisement n'est donc pas un blocage de cette cible.
Une combinaison future de plusieurs K devra annoncer une autre règle.
Ces petits contrôles ne qualifient pas la robustesse statistique, FULL
large, la croissance ou une nouvelle performance G4.
Réserve de rédaction dans la preuve 1D : la formule p=K−2, q_min=m=2
concerne K≥2 ; K1 est le singleton p=0, q_min=m=1. Le script K1 reste
correct. À K fixé, comparer l'antichaîne minimale des témoins avant LCA
pour éviter les retards dus à un ancêtre redondant ; cette variante change
le bras et n'a pas de qualité statistique démontrée.

## À lire maintenant

| Sujet | État actuel | Référence |
| --- | --- | --- |
| FULL → partitions de points | À K fixé, Pκ est la piste prioritaire de stabilité, calculable en flux sans antichaîne. Frontières et entrées internes K3/K5 conservées, core contrôle. Majorités par marches et bandes restent des bras limités ; pas de victoire EOM/ARI ni choix produit qualifié. | [Calcul en flux](../receipts/audit_continu_20260929/persistent_anchor_stream_20260930/README.md), [preuves frontière antérieures](audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md), [suivi au développeur](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md) |
| Fixtures de projection | F1–F4 cohérentes. Notre Γ exact confirme 75 couples nuage/K ; campagnes développeur 10/10 closes, distinctes de notre test autonome. Aucun vote ni nouveau traitement frontière qualifié. | [Contre-audit des fixtures](audit_continu_20260929/CONTRE_AUDIT_FIXTURES_PROJECTION_20260929.md) |
| Retrouver toutes les couvertures | Lemme par composante : témoin p+q_min≤K, coquilles et intérieurs complets. 30 102 requêtes autonomes, puis 64 appels natifs K1–K4, huit géométries, nerf rationnel indépendant et listes complètes. Ne pas supprimer les fusions FULL p+q_min=K+1. Qualification native bornée, pas globale. | [Preuve, oracle et complément R2](audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md) |
| Sécurité du pool | R2 : CAS saturant et série sans overflow. Deux différentiels clos 24/24, oracles 2/2 ; comparateurs limités à des préfixes SHA96/64bits. Timeout d'une sonde à barrière distinct d'un deadlock démontré. Copies non intégrées. | [Complément R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [premiers correctifs](audit_continu_20260929/pool_head/CONTRE_AUDIT_POOL_CORRIGE_20260929.md) |
| Interfaces | Parseur strict sur copie R2. Nouvelle CLI tête : refus numériques propagés, mais mêmes destinations étiquettes/arbre → texte écrasant les labels, code0. Quatre sondes closes, raccord avec écritures vérifiées encore en chantier. | [Collision et raccord R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [premiers correctifs](audit_continu_20260929/catalogue/CONTRE_AUDIT_INTERFACES_CORRIGEES_20260929.md) |
| SiteTree et centres rationnels | Filtre limité à FE_TONEAREST. Après les huit mutants survivants historiques, nouvelle contre-porte privée : chemins réellement observés, 34 mutants tués, ASan/TSan passent isolément. Aucun transfert à un binaire commun, FENV global ou FULL. | [Complément SiteTree R2](audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md), [suivi du raccord](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md) |
| Tête numérique | R2 protège racine zéro et M·λ_max ; notre porte native antérieure passe. Quatre nouvelles sondes confirment Outcome dans la CLI tête et un refus tardif avant écriture dans la même hiérarchie. Ancienne tête dans la copie CLI stricte ; union non qualifiée. | [Contrôle R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [défauts et borne d'origine](audit_continu_20260929/pool_head/CONTRE_AUDIT_TETE_NUMERIQUE_20260929.md) |
| Condensation des points | Défaut de seuil des départs différés : renversement EOM sur API valide mcs5/racine exclue ; sklearn sur ultramétrique équivalente confirme. Export natif historique de six sites K3 recoupé : stabilité erronée mcs6, sans changement d'étiquettes. Aucun nouvel appel générateur ni impact ARI acquis. | [API et référence](../receipts/audit_continu_20260929/point_condensation_20260930/README.md), [témoin géométrique](../receipts/audit_continu_20260929/point_condensation_cover_r2_20260930/README.md) |
| Rejets q3/q4 par groupe | Somme affine de puissances → plusieurs intérieurs certifiés ; dominance individuelle vide sur deux fixtures 3D. Test division-free strict, contacts gardés, groupes recouvrants non additifs. Sélection/coût total/croissance encore à mesurer. | [Preuve et essai borné proposé](../receipts/audit_continu_20260929/group_moments_20260930/README.md) |
| Juges catalogue/FULL | Petits juges R2 renforcés contre-vérifiés. Nouveau lecteur structurel des grands dumps : ordre K entier manquant ou coordonnées d'attaches inconnues acceptés sur fixtures ; contrôles linéaires à ajouter. Aucun dump LiDAR réellement fautif observé. | [Compléments R2](audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [angles morts d'origine](audit_continu_20260929/catalogue/CONTRE_AUDIT_JUGES_CORRIGES_20260929.md) |
| Bancs et arrêt des calculs | R2 refuse maintenant ARI1,25. 44 nouveaux appels courts : alpha=2/NaN et en-tête ari_s dupliqué admis ; config absente/inconnue finit en KeyError. Schéma à valider, A/C historiques non réfutés. 120 vrais signaux POSIX locaux observés distingués des simulations. | [Complément des bancs R2](audit_continu_20260929/timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md) |
| Prototypes CPU | J3 réduit le CPU de t_boxes ×1,37–1,55, mêmes comptes ; mutant survivant équivalent par parité. 1 060 cas conclusifs et dix délais observés, TSan frontière v3b terminé. Gain p1c CPU total FULL K5 seulement 2,5 % sur le lot local ; variantes non combinées sur G4. | [Contre-audit CPU, périmètres et preuves](audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md) |
| Aval ordre/tête | Contre-audit nouvelle copie : validation parallèle CSR hors bornes sur objet public forgé, alors que série refuse. Temps mur local ordre+assemblage réduits, sans preuve GPU/FULL 100 ms. Gate nouvelle copie 9/9 réellement close, défaut CSR toujours reproductible ; refus et interruptions séparés des cas conclusifs. | [Contre-audit ordre/tête](audit_continu_20260929/performance/CONTRE_AUDIT_ORDRE_TETE_CORRIGE_20260929.md) |
| CUDA | Unsigned accepté ; contrôle hôte UBSan propre. Statuts et durées corrigés dans 779dd38a9, mais lecteur d'enveloppe seulement : vingt entrées et huit simulations en précisent les limites. Débits historiques signés invalides, aucun nouveau reçu GPU ni port FULL GPU qualifié. | [Sonde corrigée](audit_continu_20260929/timeout/CONTRE_AUDIT_SONDE_CORRIGEE.md), [statuts et échecs](audit_continu_20260929/ADDENDUM_ZERO_ET_STATUTS_20260930.md), [errata](../receipts/ERRATA.md) |
| G4 et passage à l'échelle | FULL K5 sans attaches mesuré à 204–254 ms sur trois trames sans sol d'une seule séquence ; CPU, non GPU. Ni 100 ms ni plusieurs séquences qualifiés. | [Recalcul des mesures](audit_continu_20260929/timeout/AUDIT_ECHELLE.md) |
| Plusieurs dizaines de millions | RankIndex corrigé dans le produit : milieu par différence et `lo*64` élargi avant clamp, 36 047 contrôles virtuels et deux mutants tués. Cela ne qualifie pas la capacité massive, les autres conversions ou la nouvelle précision. Certificat Q×Z encore à implémenter. | [Développement et portée](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md), [audit massif indépendant](AUDIT_MASSIF_LIDAR_20260930.md) |
| Précision au-delà de u18 | Morton96/distance u128, filtre relatif et comparateur 266/200 bits éprouvés isolément. Constructeurs, seuils K-NN 66 bits, nearest, propriétaire et FULL encore à porter ; distinguer rangs exacts et table double. Préparateur décimal exact disponible ; consommateur v10 des pas, origines et IDs manquant. Aucune croissance ou performance G4 large héritée. | [Ordre exact](../receipts/audit_continu_20260929/level_order_20260930/README.md), [filtre et limites](../receipts/audit_continu_20260929/relative_filter_20260930/README.md), [ordre de développement](../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md) |

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
