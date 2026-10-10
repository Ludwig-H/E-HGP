# Synthèse du canal — 10 octobre 2026

Extraits historiques repris de `589e3b752` : cellules conservées, liens relatifs relocalisés.
Le registre courant conserve identifiants, états, constats et témoins ; seuls
les suivis historiques de ses dernières cellules sont résumés. Les nouvelles
preuves A6c/B3b restent liées directement depuis le registre courant.

| Origine | Destination |
| --- | --- |
| audits/CONSTATS.md, CST-0018, pin589e3b752 | [CST-0018](#cst-0018) |
| audits/CONSTATS.md, CST-0242, pin589e3b752 | [CST-0242](#cst-0242) |

## CST-0018

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0018` | juges de leviers : un banc refusé doit interdire toute adoption, comme une prise ou une preuve manquante (auditeur Codex, point 1) | 2026-10-07 | auditeur (Codex), inscrit par le développeur | mesure | majeure | `13c52bc60`, `AUDIT_CODEX_20261007.md` | — | en cours | [Historique](../audit_canal_20261008/suivis_t1d/README.md#cst-0018), résidus cache2/C3/GAPP/L1p inchangés. [A6](../audit_reponses_20261008/a6_retrait_qualification/README.md) fermé6497 puis rejeté ; [B2 admis](../audit_reponses_20261008/session_b2_admission/README.md). [Port T1-d](../audit_reponses_20261008/t1d_admission/README.md) intégré150392, attendu36 corrigé8b9 ; [T1-d2 admis](../audit_reponses_20261008/session_t1d_admission/README.md), T1-d1 refusé. [Identité niveaux/table](../audit_reponses_20261008/t1d_identite_flux_proposition/README.md) proposée,46 injections et [CTest46 requis](../audit_reponses_20261008/t1d_identite_flux_ctest/README.md). [R1 admis](../audit_reponses_20261008/session_r1_admission/README.md). [A6b](../audit_reponses_20261008/session_a6b_admission/README.md) : refus A/A puis rejet ng00/01. [B3 juge v1](../audit_reponses_20261008/b3_identite_admission/README.md) : 35 journaux manquants acceptés dans le contre-exemple. [Correctif v2](../audit_reponses_20261008/b3_identite_proposition/README.md), 27+21 contrôles, non embarqué. [B3 G4](../audit_reponses_20261008/session_t2db3_admission/README.md) : trois rejets, A/A valide ; 10 codes G/stderr absents, admission stricte incomplète. Garde LF/port GAPP2 proposés. [A6c](../audit_reponses_20261010/a6c_pilote/README.md) : cohorte/relecture strictes,85 journaux synthétiques,8 corruptions refusées ; diagnostics/stderr à compléter. |

## CST-0242

| Identifiant | Constat | Date | Rôle | Classe | Gravité | Pin | Témoin | État | Preuve de clôture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `CST-0242` | indices concurrents A6 : appartenance à la composante finale insuffisante pour justifier le préfixe au moment de consommation ; pont de publication à expliciter | 2026-10-08 | auditeur (Codex) | concurrence | majeure (preuve avant adoption) | prototype `69dd8e902`, `forest_kernel.cpp` SHA `136d46a5`, douze corps capturés | [témoin abstrait de coupe intermédiaire, lecture source et preuve release/acquire](../audit_reponses_20261008/a6_indices_concurrence/README.md) ; aucune contre-exécution native ou C++ complète établie | clos | [Historique](../audit_canal_20261008/suivis_t1d/README.md#cst-0242). [Clôture par retrait](../audit_reponses_20261008/a6_retrait_qualification/README.md) : pont livré6497, A6 rejeté puis retiréab561, trois scopes tower=bdf. [A6b](../audit_reponses_20261008/a6b_produit/README.md) : pont favorable sous préconditions ; [portes v2](../audit_reponses_20261008/a6b_portes_v2/README.md),83 programmes structurels. [Clôture locale f2](../audit_reponses_20261008/a6b_cloture_locale/README.md) :753 Passed/1Skipped, deux mutants code ; cinq écarts hors produit natif. [Retrait après rejet](../audit_reponses_20261008/a6b_retrait_r1/README.md) rejoué contre8a0716e74 livré :163 fichiers exacts R1, CST-0241 préservé ; aucune nouvelle porte native. Priorité G=réclamé, pas achevé. [A6c aa633](../audit_reponses_20261010/a6c_math/README.md) réintroduit le pont : lecture/modèles favorables sous préconditions, aucune qualification générale C++. |
