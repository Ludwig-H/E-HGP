# État de l'audit v8

13 septembre 2026. Audit constructeur, avec contrelectures parallèles.
Ce dossier n'est pas l'auditeur indépendant propriétaire des audits v7.

La porte d'entrée documentaire est ouverte : demande explicite de refonte,
cadre et périmètre déclarés dans [README](../README.md), base v7 publiée
`dc57ffd5`, état local sale distingué. Les règles du dépôt restent en vigueur.
La lecture intégrale antérieure des parties I/II est consignée dans
[la lecture v7](../../morsehgp3D_v7/docs/LECTURE_ET_CONTRATS.md) ; les
définitions et preuves utiles sont réexaminées pour cet audit.

Trois contrelectures ont traité séparément WSPD/supports/témoins,
contrats et mesures, architecture/CPU/GPU/tests, puis confronté leurs
rapports. La synthèse et les fondements du constructeur ont aussi été
contrelus. Les précisions de régularité, voies actives, compteurs et
maturité ont été intégrées. Aucun avis indépendant futur n'est anticipé.

La [synthèse](../docs/AUDIT_V7_SYNTHESE.md) et le
[périmètre vérifiable](PERIMETRE_ET_PREUVES.md) constituent la livraison
d'audit. Les états « prouvé sous hypothèses », « testé borné », « mesuré »,
« proposé » et « manquant » sont distingués. L'audit couvre la chaîne et
ses contrats, sans prétendre relire chaque ligne des 26 777 fichiers ni
relancer les suites C++. Six lecteurs et deux recalculs exacts sont clos.

Verdict public inchangé : `not_claimed`. Contrats 50k et massif ouverts.
La CLI reste F, les sondes FULL et les prototypes privés sont distingués.
Aucun moteur v8, aucun usage GCP. Les contrôles documentaires et d'index
de livraison sont conservés dans les reçus v8, sans valeur de preuve moteur.
