# Cache FULL paresseux : qualification indépendante

Header gelé `13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627`, capturé après `6f4b4de5`. Cadre CPU u16 hors registre ; `public_status=not_claimed`.

**Les quatre politiques produisent les forêts FULL attendues sur les 109 ordres du corpus indépendant, y compris à cache nul ou saturé.** Aucun défaut nominal trouvé. La qualification reste relative à des catalogues fournis complets, exacts et réguliers.

| Preuve propre à l’audit | Résultat par build nominal |
| --- | --- |
| O2 et ASan/UBSan, sorties identiques | 109 ordres, 218 représentations, EAGER/C0/C1/C100000 : 872 sorties et 67 920 coupes |
| Comparaisons entre politiques | 654 forêts lazy/EAGER ; 218 égalités de travail sans skip ; 200 représentations EAGER historiques inchangées |
| Budgets | 16 plafonds exacts, 180 refus cap−1, douze conflits d’API |
| Trois mutants privés causaux | J1 supprimé : 96 sorties touchées ; minima confondus : 54 témoins ; capacité ignorée : 58 dépassements |

Les [sources, fixtures, commandes et jugements](receipts_full_lazy_20260905/README.md) détaillent ces comptes. Cent ordres historiques sont réemployés sous hashes ; neuf nouveaux ordres sont recalculés rationnellement. Normal et `-O` concordent. J1, descente à deux étapes, ancres muettes, minima simultanés, K=n et réutilisation dans un même lot sont exercés. Les 218 sorties EAGER de chaque mutant restent identiques au nominal ; une erreur de transport ne compte jamais comme réfutation.

Le [contrat constructeur](../docs/CONTRAT_CACHE_FULL_PARESSEUX.md) porte le calendrier. La [revue sémantique](receipts_full_lazy_20260905/semantic_review.md) confirme que minima, ancres de toutes les directes et successeurs restent permanents. Le cache facultatif ne contient que des résolutions dérivées. Les dépenses restent cumulatives après saturation.

Le corpus expose le coût déplacé : EAGER exécute 22 MEB, C0 94, C1 88 et C100000 82. Sans skip, les 60 MEB supplémentaires sont exactement les J1 supplémentaires ; les douze pas de descente par bras restent identiques. Cela ne mesure aucun gain de temps ou de RSS.

Les [14+14 CTests constructeur](receipts_full_lazy_20260905/publication_binding.md), les [24 admissions n=8 de la sonde](receipts_full_lazy_20260905/probe_admission_review.md) et le [supplément first-C](receipts_full_lazy_20260905/first_c_companion_review.md) sont contre-vérifiés séparément. Le dernier contrôle exige `inserts=min(C,portails)` en succès, en composition obligatoire avec le v2 gelé et le sceau. Il réfute la corruption coordonnée d’un reçu réel ; les captures historiques restent intactes.

Le digest lie entrée, ordres et topologie dans son domaine valide ; une empreinte égale n’est pas une preuve géométrique ou de complétude. Les compilations et moteurs d’audit sont clos à 17:42:45 UTC le 5 septembre. Les grandes campagnes ultérieures du constructeur, l’export, la verticale, les masses et le SLO ne sont pas qualifiés par ce reçu. GCP non utilisé.
