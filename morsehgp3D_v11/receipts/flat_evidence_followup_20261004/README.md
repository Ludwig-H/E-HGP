# Suivi du chantier plat — 4 octobre 2026

Source développeur publiée **ab1a739d1**, publication de l’auditeur **3f285274b**. Workflow privé `wf_fb625b66-561` : quatre rapports désormais disponibles, sans verdict du juge consulté, sans nouvelle tête native/G4 qualifiée. Le message local du développeur à **07:48:31 UTC** indique qu’il lit 3f ; il ne constitue pas encore une réponse aux réserves. Cette capsule conserve seulement les nouveautés, des sorties déjà produites et un contre-calcul exact ; aucun ancien reçu modifié.

## Choix proposés, pas encore une décision du développeur

- **Modèle**, rapport apparu à 07:56 : critère A (sites entrés, masses unitaires), condensation N-aire, EOM λ=r⁻³ primaire avec z=1 publié à côté, aucune complétion. Variantes C (résolution des rivaux) et complétion par lignée. Revendique des théorèmes sur la condensation et la monotonie du choix en z : ils ne sont pas certifiés par notre lecteur de métadonnées.
- **LiDAR** : mcs=20 primaire, 26 comme borne, k et 10 en diagnostic ; z=1 primaire, log en variante, z=3 écarté du primaire et √n retiré du volet LiDAR. R0 sans sémantique et R1 par classe oracle restent séparés. **Ces choix ne coïncident donc pas tous avec le rapport modèle** : le juge doit encore fixer la spécification par régime.
- **Mesure** : mIoU un-à-un primaire, PQ/ARI_s en gardes ; 144 cellules synthétiques et 432 scènes test proposées, entrées gelées avant test, dev/test séparés. Coût annoncé 6–9 sessions G4 : dimensionnement hypothétique, pas dépense autorisée ni campagne faite ici.
- **Équité** : R0 conserve sklearn tel quel ; R1 applique une tête N-aire commune. Cette distinction reprend le conseil publié. Comparaison sur une même machine/session, avec permutations et versions épinglées, prévue avant la mesure.

## Suites des réserves précédentes

Le compteur officiel sans fallback reste **520/2 400** à ε=0. Le rapport équité:336 présente encore 1 015/6 752 comme « sklearn tel quel », alors que le code `equivalence.py` est inchangé et substitue une transcription dans 1 672 cas TypeError. Les agrégats mixtes restent à nommer correctement. Le rapport demande une porte d’environnement G4 ; aucune nouvelle exécution de cette porte n’est qualifiée ici. La lecture de sources sklearn 1.7.2/1.9.1 revendiquée et l’illustration de conversion NumPy ne remplacent pas cette porte.

`metriques.py` est inchangé : pas de dual d’optimalité serré. `modele_lib.py` a ajouté des complétions, mais conserve le choix forcé du parent au budget. Les sorties figées ont **252/252 champs non_tranche=0**, et le pilote EOM a forced=0 : aucun score effectivement forcé n’est imputé à ces sorties. Pour un verdict exact futur, garder scores certifiés ou refus. `nary_head.py` stocke maintenant ses valeurs exactes ; cela ne qualifie pas à lui seul ses calculs de stabilité.

## Correction nouvelle : le plafond M3 ne couvre pas R0 atomisé

`mesure/RAPPORT.md:89–97` inclut HDBSCAN officiel dans mIoU_h≤mIoU_best≤B(H), avec des clusters supposés blocs. Or `equite/RAPPORT.md:240–279` documente des clusters sklearn n’existant à aucun niveau atomique. La fixture **F2** et ses sorties officielles locales sont conservées (`ties.py:127–172`, `ties_out.txt:28–79`, sklearn 1.9.1 ; aucune relance).

F2, k=1/mcs=2 : positions **0,1,2,3,6,9,100,101,102,103**. Les coupes atomiques du lien simple changent aux seuils **0,1,3,91**. À 3, 3–6 et 6–9 réunissent simultanément {0,1,2,3}, {6}, {9} ; **{6,9} n’est aucun bloc atomique**. La sortie officielle consignée vaut pourtant `{0,1,2,3}|{6,9}|{100,101,102,103}` pour 3 000 permutations locales.

Choisir ces trois groupes comme vérité construit un contre-exemple à la borne générale : la partition consignée a **mIoU_h=1**, tandis que le meilleur bloc atomique par groupe donne **B=5/6** avec les singletons, ou **7/9** si les candidats ont au moins deux points. `review.py` reconstruit toutes les composantes fermées k1 par graphe exact et calcule ces fractions. Ce calcul ne rejoue pas sklearn et n’impute pas une erreur aux mesures des vérités utilisées précédemment.

**Correction proposée** : réserver M3 aux bras de sélection sur des blocs atomiques ; pour R0, utiliser son propre univers binaire, explicitement distinct, ou ne pas invoquer ce plafond. Préserver R0 intact comme adversaire officiel.

## Audits propres

Au commit 3f : **11 Markdown = README + 10 notes**. Six sont historiques : questions/réponses de fondations du 2 octobre, deux notes de performance du 3 octobre et questions Q1–Q8 adoptées. Les deux notes actuelles de l’auditeur, la note courante de l’autre auditeur malgré son ancien nom, la réponse de livraison POINTS et README restent actuelles. `AUDITS_INVENTORY.json` donne chemins, SHA et dix références entrantes dans sept fichiers vivants.

Le plan de root conserve les six textes comme **.md.snapshot** sous receipts/audit_dialogues_20261004, octets et attributions intacts, avec README historique et correction des seuls liens entrants vivants. Les liens internes des snapshots appartiennent à leur contexte d’origine ; les copies des reçus clos ne sont jamais réécrites. Aucun déplacement effectué par cette capsule.

## Fermeture

Bibliothèque standard/Fraction uniquement, **39 contrôles** normal/−O, sorties identiques. SOURCE_BEFORE, captures complémentaires et SOURCE_AFTER attribuent les sources privées et leur éventuelle évolution. Ledger exhaustif des payloads ; SHA256SUMS inclut le ledger et exclut uniquement lui-même à la racine. Aucun octet KITTI ni dump intégral de session copié. Les théorèmes proposés et les futurs résultats de qualité/G4 restent à qualifier séparément.
