# Reprise de revue et réponse à la section R — 6 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Les signalements sont reproduits sur la source publiée ddb8d4ea9. La publication 0cc9cbec4 et la section R du développeur sont prises en compte avant clôture. Les propositions sont des patches à relire et intégrer, sans édition du worktree développeur.

| Sujet | Preuve et proposition | État |
| --- | --- | --- |
| Population comparative faussée par l'arrondi | [Population](population/README.md) : 39 contrôles Python, code réel extrait par AST, seuil strict conservé | À corriger ; effet réel non mesuré |
| Entrée dans un bloc de points déjà fusionné | [Chronologie](chronology/README.md) : 36 lectures exactes, trois profils, garde Python/C++ et porte native proposée | À corriger ; aucun défaut de factory démontré |
| Suffisance pour T_K assimilée à Kruskal | [Contrat](contract/README.md) : correction du §10.10 et de SORTIES, preuve du triangle déjà publiée | À corriger avec le sélecteur proposé antérieurement |
| Références API u18 et mutant SPv2 | [Raccords](gates/README.md) : comparaison aux références historiques ; clôture du mutant en 0cc9cbec4 | Références u18 ouvertes ; mutant corrigé en source |

La réponse directe à R1/R2 est dans la [note mathématique active](../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md#réponse-à-la-section-r--sélection-de-kruskal-et-contrôles-sans-q_b). Le triangle exige deux boules, et non la première seulement. La sélection réduite peut être contrôlée sans réénumérer Q_b, en gardant les portes `all` et en ajoutant le différentiel indépendant de sélection.

`verification.json` consigne les quatre paires de rejeux normal/−O faites par l'auditeur principal, avec sorties identiques. Huit patches utiles s'appliquent ensemble sur une copie temporaire de 0cc9cbec4, dont les propositions MST/lecteur déjà publiées. L'essai antérieur d'application du patch du mutant échoue parce que le développeur l'avait déjà corrigé ; cet essai est conservé et ce patch historique est exclu de l'application finale.

Chaque sous-dossier est fermé par ses empreintes ; la somme racine couvre aussi ces manifestes. Aucun test natif, compilation, benchmark ni action GCP. Les gardes C++ et leur porte restent à qualifier sur G4. Les archives et sorties réelles antérieures ne sont pas réécrites.
