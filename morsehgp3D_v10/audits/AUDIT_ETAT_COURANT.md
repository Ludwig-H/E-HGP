# Audits v10 — état courant

Mise à jour : 30 septembre 2026, lecture de la réponse `e9eab2754`,
contre-épreuve frontière et relecture des nouvelles copies corrigées. Les parties I
et II de la thèse ont été relues intégralement dans la tranche précédente.
Index vivant de l'auditeur continu ; les rapports datés restent des preuves
ancrées à leur version, pas des statuts courants. `public_status=not_claimed`.
État du produit : [PASSATION](../PASSATION.md). Corrections de portée des
mesures : [ERRATA](../receipts/ERRATA.md). Ne pas réécrire les reçus clos.

## À lire maintenant

| Sujet | État actuel | Référence |
| --- | --- | --- |
| FULL → partitions de points | Frontière à préserver, cible de la définition 8 ; core n'est qu'un contrôle. Majorité de masses fixes : preuve d'emboîtement, mais masses uniformes perdant deux groupes précoces sur huit petits nuages, dont quatre tétraèdres. 1/β les récupère ; 32 exports natifs, pas EOM ni tête native. Témoins propres à K contre le biais Kmax. | [Relecture, piste et contre-épreuve, sections 7–9](audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md), [certificat local](audit_continu_20260929/ADDENDUM_ANCRAGE_ET_CERTIFICAT_20260929.md) |
| Fixtures de projection | F1–F4 cohérentes. Notre Γ exact confirme 75 couples nuage/K ; campagnes développeur 10/10 closes, distinctes de notre test autonome. Aucun vote ni nouveau traitement frontière qualifié. | [Contre-audit des fixtures](audit_continu_20260929/CONTRE_AUDIT_FIXTURES_PROJECTION_20260929.md) |
| Retrouver toutes les couvertures | Lemme par composante : témoin p+q_min≤K, coquilles et intérieurs complets. 30 102 requêtes autonomes, puis 64 appels natifs K1–K4, huit géométries, nerf rationnel indépendant et listes complètes. Ne pas supprimer les fusions FULL p+q_min=K+1. Qualification native bornée, pas globale. | [Preuve, oracle et complément R2](audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md) |
| Sécurité du pool | Copie corrigée : exceptions/TLS par l'autre auditeur ; création partielle 0/1/2 fils joints ici ; consommateurs contre-vérifiés, 124+84 injections. Logs terminés ASan/TSan du développeur maintenant observés et figés. Intégration encore distincte. | [Contre-audit courant](audit_continu_20260929/pool_head/CONTRE_AUDIT_POOL_CORRIGE_20260929.md) |
| Interfaces | Pipe corrigé complet sur neuf commandes courtes. Suffixes numériques et dépassements u32 acceptés silencieusement : ouverts sur copie. | [Interfaces corrigées et nouveau défaut](audit_continu_20260929/catalogue/CONTRE_AUDIT_INTERFACES_CORRIGEES_20260929.md) |
| SiteTree et centres rationnels | G1 correctement ciblé : domaine exact, deux replis exacts. Quatre arrondis sans désaccord géométrique observé, mais harnais code 1 par trois planchers adverses non atteints. Formaliser le domaine FENV ; aucun faux élagage produit ni gain de complexité global établi. | [Contre-audit SiteTree](audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md) |
| Tête numérique | H3 API générale : λ finis mais poids faisant déborder les stabilités et fausser EOM ; fusion zéro → NaN. Nouveau singleton : zéro consommé même sous min_cluster_size. Borne positive conditionnelle, pas gestion des zéros ; garde annoncée non intégrée. | [Contre-audit numérique et bornes](audit_continu_20260929/pool_head/CONTRE_AUDIT_TETE_NUMERIQUE_20260929.md), [complément zéro](audit_continu_20260929/ADDENDUM_ZERO_ET_STATUTS_20260930.md) |
| Juges catalogue/FULL | Sur copie, S* canonique et verticale sans point sont maintenant jugés. Doublons I/U, ordre exporté et fusion ternaire binarisée au même plateau encore acceptés. Ces mutants révèlent des angles morts du juge, pas un défaut géométrique du moteur. | [Contre-audit des juges corrigés](audit_continu_20260929/catalogue/CONTRE_AUDIT_JUGES_CORRIGES_20260929.md) |
| Bancs et arrêt des calculs | Copie corrigée contre-vérifiée : arrêt/récolte au délai et signal, complétude refusée, 18 commandes courtes normal/−O. ARI impossible de 1,25 encore accepté jusque dans la décision de fixture. | [Contre-audit des bancs](audit_continu_20260929/timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md) |
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

Les preuves natives de couverture utilisent l'archive `6206d1d11` ; les
preuves de consommateurs utilisent la copie corrigée du pool. Elles ne
constituent pas ensemble une qualification d'un unique binaire intégré.

## Références historiques — ne pas confondre avec les travaux actifs

- [Archive de l'audit v9](audit_v9_20260928/README.md) : origine de la refonte v10,
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

Le point d'entrée `audits/README.md` a été actualisé par son propriétaire
et son auditeur coordonné dans le worktree ; je ne le modifie pas. Leur
`SUIVI_AUDIT_INDEPENDANT.md` et leurs nouvelles notes sont encore à publier
au HEAD distant `e9eab2754` observé avant cette publication : je n'embarque aucun
fichier d'un autre acteur. Nos captures brutes sont
dans `receipts/audit_continu_20260929/`, avec relocalisation à hashes identiques
publiée dans `c5015a570`. Les nouveaux petits tests y restent aussi. Les
rapports utiles sont conservés dans `audits/`, sans copie de journaux ou
de builds. Les fichiers des autres intervenants restent sous leur contrôle.
GCP non utilisé dans cette tranche ; aucun contrat ou statut public promu.
