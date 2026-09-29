# Audits v10 — état courant

Mise à jour : 29 septembre 2026, après lecture de `12aa92110`, relecture
intégrale des parties I et II de la thèse et contre-audits des copies corrigées.
Index vivant de l'auditeur continu ; les rapports datés restent des preuves
ancrées à leur version, pas des statuts courants. `public_status=not_claimed`.
État du produit : [PASSATION](../PASSATION.md). Corrections de portée des
mesures : [ERRATA](../receipts/ERRATA.md). Ne pas réécrire les reçus clos.

## À lire maintenant

| Sujet | État actuel | Référence |
| --- | --- | --- |
| FULL → partitions de points | Frontière à préserver, cible de la définition 8 ; core n'est qu'un contrôle. Le vote plat ne garantit pas toutes les coupes. Nouvelle piste : majorité de masses fixes, preuve d'emboîtement et 180 cas abstraits. Témoins propres à K pour éviter un biais Kmax démontré sur six sites. Ni tête native ni qualité statistique acquises. | [Relecture et piste, sections 7–8](audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md), [certificat local](audit_continu_20260929/ADDENDUM_ANCRAGE_ET_CERTIFICAT_20260929.md) |
| Retrouver toutes les couvertures | Lemme par composante : témoin p+q_min≤K, coquilles et intérieurs complets. 30 102 requêtes autonomes, puis 64 appels natifs K1–K4, huit géométries, nerf rationnel indépendant et listes complètes. Ne pas supprimer les fusions FULL p+q_min=K+1. Qualification native bornée, pas globale. | [Preuve, oracle et complément R2](audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md) |
| Sécurité du pool | Copie corrigée : exceptions/TLS par l'autre auditeur ; création partielle 0/1/2 fils joints ici ; consommateurs contre-vérifiés, 124+84 injections. Logs terminés ASan/TSan du développeur maintenant observés et figés. Intégration encore distincte. | [Contre-audit courant](audit_continu_20260929/pool_head/CONTRE_AUDIT_POOL_CORRIGE_20260929.md) |
| Interfaces et tête numérique | Pipe corrigé complet sur neuf commandes courtes. Nouveau défaut : suffixes numériques et dépassements u32 acceptés silencieusement. H3 tête reste distinct, reproduit par l'autre auditeur. | [Interfaces corrigées et nouveau défaut](audit_continu_20260929/catalogue/CONTRE_AUDIT_INTERFACES_CORRIGEES_20260929.md), [état historique détaillé](audit_continu_20260929/AUDIT_ETAT_20260929.md) |
| CUDA | Correction unsigned acceptée ; 96 récurrences et 4096 comparateurs hôtes exacts, UBSan propre. Débits historiques signés toujours invalides ; aucun nouveau reçu GPU de la sonde corrigée, aucun port exact FULL GPU acquis. | [Sonde corrigée](audit_continu_20260929/timeout/CONTRE_AUDIT_SONDE_CORRIGEE.md), [audit historique](audit_continu_20260929/timeout/AUDIT_ADDENDUM_GPU_GRANDK_20260929.md), [errata](../receipts/ERRATA.md) |
| G4 et passage à l'échelle | FULL K5 sans attaches mesuré à 204–254 ms sur trois trames sans sol d'une seule séquence ; CPU, non GPU. Ni 100 ms ni plusieurs séquences qualifiés. | [Recalcul des mesures](audit_continu_20260929/timeout/AUDIT_ECHELLE.md) |

La borne locale K2 demande une marge stricte autour du seuil d'ambiguïté.
Elle garantit des dates sous perturbations appariées, pas l'ARI, l'EOM ni
une généralisation K5. Différer des points jusqu'à la fusion peut perdre
le rappel frontière recherché : mesurer leur récupération avant connexion
parasite, pas seulement les hauteurs. La borne de packing des voisins ne
borne pas les paires de voisins, les q3/q4 ou les visites de l'index.

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
au HEAD distant `12139ed6f` observé avant cette publication : je n'embarque aucun
fichier d'un autre acteur. Nos captures brutes sont
dans `receipts/audit_continu_20260929/`, avec relocalisation à hashes identiques
publiée dans `c5015a570`. Les nouveaux petits tests y restent aussi. Les
rapports utiles sont conservés dans `audits/`, sans copie de journaux ou
de builds. Les fichiers des autres intervenants restent sous leur contrôle.
GCP non utilisé dans cette tranche ; aucun contrat ou statut public promu.
