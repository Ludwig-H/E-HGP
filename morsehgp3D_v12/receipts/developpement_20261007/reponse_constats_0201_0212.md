# Réponse du développeur aux constats CST-0201 à CST-0212 (auditeur Codex)

7 octobre 2026. Constats lus au pin `995c0f424` ([registre](../../audits/CONSTATS.md),
[contre-audit numérique](../audit_contrats_20261007/numerique/REPORT.md),
[repère et profondeur](../audit_contrats_20261007/repere_et_profondeur/README.md),
[mesure](../audit_contrats_20261007/mesure/NOTE_MESURE.md), [échelle](../audit_suivi_20261007/echelle/README.md),
[session](../audit_suivi_20261007/session/REPORT.md)). Tous sont acceptés. Les corrections de ce commit portent sur
les contrats et sur le microbanc M6 ; aucune ne qualifie un moteur, qui n'existe pas encore. Les états du registre
restent à l'auditeur.

| Constat | Correction | Reste |
| --- | --- | --- |
| `CST-0201` certificats | `CONTRAT_NUMERIQUE.md` § 3 : la phrase « ils ne lisent ni $B$ ni $s$ » était fausse ; un certificat porte l'exposant $t$ de son domaine, $t=s+2$ pour le recensement gardé ; types reconstruits, aucune conversion rétrécissante avant un contrôle | porte du témoin q3 à $s=20$, mutant « certificat du support seul » |
| `CST-0202` identité des sites | § 4 : clé de Morton **exacte** (`u64` si $B_{\mathrm{eff}}\leq 21$, `u128` sinon), identité des sites comme dans la v11 ; la clé tronquée est abandonnée ; coupe de l'index sur coordonnées normalisées | portes d'identité et de doublons intercalés, refus D8 |
| `CST-0204` boîtes à 33 bits | § 2 : bornes de boîtes en 64 bits, paliers jusqu'à $s=33$, étendue nulle $s=0$ | témoin $(0,0,0)$, $(2^{32}-1,0,0)$ |
| `CST-0205` profondeur | `ARCHITECTURE.md` § 4.1 : la borne de 38 niveaux était fausse ; profondeur au plus $3B$ (63, 72, 96), nombre de nœuds budgété à part | témoins de profondeur 60 et 63 dans la porte du parcours |
| `CST-0207` mesure de D6 | § 8 : binaire u21 contre candidats u24 et u32, mêmes trames, régime D1–D3, règle écrite avant les prises ; la translation devient un banc distinct ; `ARCHITECTURE.md` § 1 aligné sur D6 | règle statistique du banc |
| `CST-0208` réservoir | § 3 : ligne $2s+4$, natif `i64` jusqu'à $s=29$ ; `ARCHITECTURE.md` § 4.1 ne cite plus $2B+5$ | témoin à $s=30$ |
| `CST-0209` médiane de M6 | quantiles interpolés ; auto-test (1..10 donne 5,5) joué avant tout appel CUDA | campagne G4 |
| `CST-0210` premier usage de M6 | chaque mesure répétée sépare `<nom>_first` des répétitions chaudes ; préparation du graphe mesurée à part | campagne G4 |
| `CST-0211` pré-vol | `ARCHITECTURE.md` § 4.6 : prévision, admission certifiée (comptage exact par lot ou borne combinatoire démontrée), réservation contrôlée avec refus transactionnel | code du catalogue en flux |
| `CST-0212` domaine des indices | `ARCHITECTURE.md` § 4.6 : domaine exact par espace d'indices, sentinelle exclue, bit de genre déclaré, décalages 64 bits dans les prototypes aussi ; l'agent du microbanc de forêt a reçu le constat | porte à la borne dans le microbanc |

Remarques sur les rapports :

- Garde (`numerique/REPORT.md` § 3) : la révision `8865e32c1` disait déjà « ne touche pas le pavé » pour les boîtes ;
  ce commit ajoute la preuve par requête (pas de repère commun à toutes les requêtes, pavé entier en $s+3$ bits), le
  point entier saturé à la boîte, la boîte partielle raffinée et jamais rejetée, la candidate non certifiée du
  témoin $(419,0,0)$, et sépare contact à la sphère et contact au pavé dans les portes.
- Empreinte (`mesure/NOTE_MESURE.md`, complément à `CST-0113`) : l'empreinte de la v11 juge la conformité sur les
  mêmes coordonnées absolues, jamais l'invariance par translation, qui a sa porte propre (§ 4 et § 7 du contrat).
- `MESURE.md` § 3.1 : mode `868347` à K10, `868347:400` à K5.

GCP non utilisé. `public_status=not_claimed`.
