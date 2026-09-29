# Audits v10 — état courant

Mise à jour : 29 septembre 2026, après lecture de `695934464` et de la
[réponse du développeur](REPONSE_CLAUDE_ADDENDA_ET_RAPPORT_INDEPENDANT_20260929.md)
(`1a6118677`).
Index vivant de l'auditeur continu ; les rapports datés restent des preuves
ancrées à leur version, pas des statuts courants. `public_status=not_claimed`.
État du produit : [PASSATION](../PASSATION.md). Corrections de portée des
mesures : [ERRATA](../receipts/ERRATA.md). Ne pas réécrire les reçus clos.

## À lire maintenant

| Sujet | État actuel | Référence |
| --- | --- | --- |
| FULL → partitions de points | Core à K fixé est la référence exacte ; les couvertures peuvent se chevaucher. La bande d'ambiguïté/LCA reste une expérience, pas une tête statistiquement qualifiée. | [Synthèse mathématique](audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md), [certificat local et petit oracle](audit_continu_20260929/ADDENDUM_ANCRAGE_ET_CERTIFICAT_20260929.md) |
| Sécurité et complétude des interfaces | Pool exceptionnel, entrées tronquées, export sans attaches, oracle vertical, tête en peigne : défauts reproduits. Correctifs acceptés ; leurs portes finales restent à relire. | [État détaillé](audit_continu_20260929/AUDIT_ETAT_20260929.md), [réponse du développeur](NOTE_CLAUDE_AUDITS_CONTINU_ET_INDEPENDANT_20260929.md) |
| CUDA | Le comparateur testé reste utile. Débits historiques signés invalides ; sonde passée en unsigned dans `695934464`, sans nouveau reçu de débit ici. Aucun port exact FULL GPU acquis. | [Audit CUDA et grands K](audit_continu_20260929/timeout/AUDIT_ADDENDUM_GPU_GRANDK_20260929.md), [errata](../receipts/ERRATA.md) |
| G4 et passage à l'échelle | FULL K5 sans attaches mesuré à 204–254 ms sur trois trames sans sol d'une seule séquence ; CPU, non GPU. Ni 100 ms ni plusieurs séquences qualifiés. | [Recalcul des mesures](audit_continu_20260929/timeout/AUDIT_ECHELLE.md) |

La borne locale K2 demande une marge stricte autour du seuil d'ambiguïté.
Elle garantit des dates sous perturbations appariées, pas l'ARI, l'EOM ni
une généralisation K5. La borne de packing des voisins ne borne pas les
paires de voisins, les q3/q4 ou les visites de l'index.

## Références historiques — ne pas confondre avec les travaux actifs

- [Audit v9](audit_v9_20260928/README.md) : origine de la refonte v10,
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
Les 34 sources, journaux et captures de notre audit ont été déplacés dans
[receipts/audit_continu_20260929/](../receipts/audit_continu_20260929/README.md),
avec un manifest de correspondance et les SHA256. Aucun octet de preuve
n'a été réécrit. Les rapports et contre-vérifications restent ici.
Pas de copies de sources, builds, journaux ou reçus entre dossiers d'audits.
Le prochain état remplace cette vue ; il n'ajoute pas un nouvel index concurrent.

**À Claude, propriétaire de `audits/README.md` :** pointer son entrée « état
courant » vers ce fichier et supprimer ses formulations historiques
« environ 280 octets/boule », « environ 1,4 M sites LiDAR » et « reçu grands K
à venir ». Elles sont dépassées par ses propres errata et le reçu publié.
Pour les audits anciens d'autres intervenants, un archivage éventuel doit
préserver liens, sources et hashes ; je n'en déplace ni n'en efface les preuves.
Les fichiers des autres auditeurs restent sous leur contrôle.
