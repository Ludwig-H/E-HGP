# Audit Codex — état courant v12

10 octobre 2026. Candidat **A6c `aa6338ee8` intégré, non adopté** ; R1 `8a0716e74` reste
la référence temporelle. Retrait B3 `1879eff9a` vérifié : `src/` et `tests/` identiques à R1.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**A6c : corrections et portes à traiter avant adoption.**
[Prélecture mathématique](../receipts/audit_reponses_20261010/a6c_math/README.md) :
publication des tranches, pont CST-0242 et retrait CST-0241 conservés ; modèles bornés favorables,
sans nouveau contre-exemple géométrique. H garde les profondeurs mais peut travailler O(nb log nb),
contre O(nb) auparavant : gain de latence à mesurer.

- **CST-0244 : admission trop conservatrice même chaîne inactive.** Le supplément N
  `W × m × (sizeof(Sphere)+4)` est ajouté avant la décision de chaîne ; le plus grand groupe
  est aussi parcouru deux fois. Décider avant la borne, conditionner ce supplément et garder
  la décision figée. Refus supplémentaire conditionnel démontré par la formule, pas observé en natif.
- **CST-0245**, [portes](../receipts/audit_reponses_20261010/a6c_portes/README.md) : 72 mutants applicables,
  pas 72 exécutions démontrées. Admission/refus/pénurie, sur 30–229 sites, ne passent plus par
  la chaîne active. Forcer ON/OFF sur les petites fixtures, tester limites et allocations,
  garder les tests publics d'exception. La bascule actuelle couvre seulement le succès sans limite.
- [Pilote](../receipts/audit_reponses_20261010/a6c_pilote/README.md) : 85 journaux synthétiques,
  nominal admis et huit corruptions refusées en relecture stricte. Les stderr individuels
  restent hors des empreintes des prises ; compléter l'archive et les diagnostics promis.
- [Calibration et preuves](../receipts/audit_reponses_20261010/a6c_preuves/README.md) : seuil
  **43 900 sites** réglé sur les trames du prochain banc. Réserver ensuite d'autres trames
  autour du seuil ; aucune généralisation CPU/K10/massif acquise.

**Derniers temps relus : FULLN du 8 octobre, pas encore A6c.**
[Admission](../receipts/audit_reponses_20261008/session_fulln_admission/README.md) et
[contre-calcul](../receipts/audit_reponses_20261008/fulln_temps/README.md) : u21, W48, cache 8 Gio,
Session chaude ; GPU = catalogue GPU puis tour CPU. Médianes chaudes réunies, en ms :

| FULL | ng00, 39 885 sites | ng01, 35 551 sites | ng02, 45 845 sites |
| --- | ---: | ---: | ---: |
| **GPU K5** | **80,31** | **66,59** | **83,32** |
| **CPU K5** | **354,36** | **298,90** | **361,18** |
| GPU K10 | 495,10 | 366,14 | 422,19 |

Sur **37 trames de six séquences**, cinq secondes visites chacune : médiane des médianes
**142,41 ms**, pire médiane **287,19 ms**, maximum **288,22 ms** ; **14/37** sous 100 ms.
**Contrat multi-séquences non tenu.** Lecture, masque, ouverture, validation, FUL1 et libération
hors mur ; CPU K10/37 non mesurés. Limites FULLN : codes/stderr individuels non archivés,
ELF initial seulement ; relecture concordante, fermeture incomplète. `cpu_ns` cumule les fils.

**CPU : priorité au catalogue C**, 83,78–85,20 % du mur, >100 ms sur les 36 chaudes.
[Diagnostic et petits nuages](../receipts/audit_reponses_20261008/fulln_petits_cpu/README.md) ;
[réduction census proposée](../receipts/audit_reponses_20261008/cpu_census_reduction/README.md),
104 084 cas Python, sans gain natif qualifié. Optimiser seulement G ne suffit pas à C fixé.

**B3 retiré :** [trois leviers rejetés](../receipts/audit_reponses_20261008/t2db3_stats/README.md).
Le transfert seul n'explique pas ng02. [Reçu public relu](../receipts/audit_reponses_20261010/a6c_preuves/README.md) :
codes G/stderr toujours incomplets, juge v2 toujours proposé, non intégré.

**Massifs R1 inchangés :** [Paris sans sol GPU](../receipts/audit_reponses_20261008/session_l2t_admission/README.md),
9 111 422 sites, K5 **46,453 s froid**. Ni chaud/FUL1 ni transfert de capacité à A6c.
[CST-0243](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md) reste ouvert :
TU Wien 5,200M réussit puis refuse la passe1 pour mémoire ; cause inconnue.

Aucun moteur exécuté par l'auditeur ; sources, métadonnées et modèles Python seulement.

[G4 vérifiée à 18:09:45 UTC](../receipts/audit_reponses_20261010/g4_controle_auditeur/README.md) :
TERMINATED, zéro VM E-HGP active ; aucun nouvel arrêt nécessaire.
