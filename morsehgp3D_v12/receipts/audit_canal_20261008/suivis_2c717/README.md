# Suivis Codex déplacés depuis le registre 2c717

8 octobre 2026. Archive de suivi au commit `2c717b124950abd8e6e11b2a631d8f4e4d31943c`, sans changement de constat,
d’état ou de qualification. Les quatre cellules ci-dessous reproduisent leurs suivis à cette date ; seuls les
chemins relatifs des liens sont adaptés. Le registre courant reste l’autorité des états et des suites récentes.
Aucune donnée brute, ancien code ou identité de compte n’est recopié.

[Correspondance source → ancre](mapping.json) ; [synthèse proposée](proposition.json),
[patch limité au registre](proposition.patch). Les neuf premières colonnes restent identiques.
Le patch vise uniquement ce pin : les nouveaux suivis C/C2/A ajoutés ensuite doivent être conservés lors du raccord.
Le diff a zéro ligne de contexte (`git apply --unidiff-zero` sur la copie épinglée) ; ne pas l'appliquer
aveuglément sur un registre ultérieur. Après raccord, le responsable rejoue `tools/check_constats.py`.

Économie proposée : **2 977 octets**, canal **65 140 → 62 163 octets** au pin source. Les **47 liens des
suivis** sont conservés ; dates, auteurs, pins, témoins et états restent identiques. Les répétitions dans ce
reçu et le patch servent la revue du déplacement, sans ajouter une seconde autorité aux états.

Le [rejeu](check.py) vérifie la conservation textuelle des suivis et de tous leurs liens, la portée du diff,
les métadonnées inchangées et le gain de taille. Il ne rejoue aucune preuve métier et ne clôt aucun constat.

```sh
python check.py --repo /workspaces/E-HGP
python -O check.py --repo /workspaces/E-HGP
```

## CST-0018

Source : `morsehgp3D_v12/audits/CONSTATS.md:26` au pin ci-dessus. Date : 2026-10-07.
Auteur : auditeur (Codex), inscrit par le développeur. État : **en cours**.
Pin du constat : `13c52bc60`, `AUDIT_CODEX_20261007.md`.
Témoin conservé : —.

[Historique qualifié M5/G1/M6/CUDA/D6/Gc](../../audit_canal_20261008/suivis/README.md#cst-0018). [D6](../../audit_reponses_20261008/d6_livraison_stricte/README.md) : CLI/collecte C ouverts. [FULL commun](../../audit_reponses_20261008/lecteur_full_commun/README.md) : types/statuts/mémoire ouverts ; patch partiel. [MES-B corrigé](../../audit_reponses_20261008/mes_b_memoire/README.md). [K admise](../../audit_reponses_20261008/session_k_full/README.md), contrat non tenu. [A902](../../audit_reponses_20261008/t2d_a_schema902/README.md) : queue/mémoire ouverts ; [A bcd](../../audit_reponses_20261008/t2da_integration/README.md), route/isolation/cohorte permissives, patch ; [comparaison](../../audit_reponses_20261008/t2d_a_comparaison/README.md) même binaire proposée ; [fenêtre feuilles](../../audit_reponses_20261008/t2d_a_fenetre_patch/README.md) à corriger, mur FULL non réfuté. [C suivi](../../audit_reponses_20261008/t2d_c_suivi/README.md) : huit gardes ajoutées, refus/cohorte ouverts sur ce pin ; [intégration `02b735d6b`](../../audit_reponses_20261008/t2d_c_integration_math/README.md) mathématiquement relue ; [G4 admise](../../audit_reponses_20261008/session_t2dc_admission/README.md), lot/quatre leviers adoptés, FULL K5 153,86/122,66/155,64 ms ; [sources/arrêt](../../audit_reponses_20261008/session_t2dc_provenance/README.md), [pic commun précisé](../../audit_reponses_20261008/session_t2dc_documentation/README.md) ; [cohorte du juge/tableau close en 5f5c](../../audit_reponses_20261008/t2dc_cohorte_livree/README.md), mutants en échec encore permissifs. Source après à harmoniser avec avant avant inclusion du nouveau pool. [B corrigé](../../audit_reponses_20261008/t2d_b_admission_reprise/README.md) : mur G/device, veto A/A et parité corrigés ; quatre blocs FULL informatifs ouverts ; [qualification](../../audit_reponses_20261008/t2d_b_traces/README.md), [39 mutants](../../audit_reponses_20261008/t2d_b_mutants/README.md). [L1](../../audit_reponses_20261008/session_l1_admission/README.md)/[L1r](../../audit_reponses_20261008/session_l1r_admission/README.md) admises séparément, B1/B2/B4 non tenus, B3 non évalué ; [arrêts](../../audit_reponses_20261008/session_l1_recuperation/README.md), [errata](../../audit_reponses_20261008/l1r_documentation/README.md). [L2 admise](../../audit_reponses_20261008/session_l2_admission/README.md), deux succès CPU froids/trois refus, B1–B4 non évalués ; [arrêt](../../audit_reponses_20261008/session_l2_provenance/README.md), [errata](../../audit_reponses_20261008/session_l2_documentation/README.md). [MES-C24](../../audit_reponses_20261008/mes_c_correction_24dec/README.md) : cohorte/verdict corrigés, 11 mutants ; [campagne83 admise](../../audit_reponses_20261008/session_c_admission/README.md), 4 009 passes/2 666 chaudes, C1–C3 non tenus, GPU K10 interrompu, verdict refusé ; [arrêt](../../audit_reponses_20261008/session_mes_c_provenance/README.md). Aucun temps réel faux ni gain causal L1/L1r démontré.

## CST-0207

Source : `morsehgp3D_v12/audits/CONSTATS.md:52` au pin ci-dessus. Date : 2026-10-07.
Auteur : auditeur (Codex). État : **en cours**.
Pin du constat : `8865e32c1`, contrat § 8 ; `DECISIONS.md` D6.
Témoin conservé : [contre-modèles des deux ratios](../../audit_contrats_20261007/mesure/NOTE_MESURE.md).

Comparaison des binaires séparée de la translation (`0dbf69347`) ; [socle](../../audit_socle_microbancs_20261007/session/REPORT.md). Pilote `9b2747eff` [audité](../../audit_d6_20261007/README.md), référence u21 parfois absente ; [corrigé `e37fd8935`](../../audit_reponses_20261008/d6_livraison_stricte/README.md), quatre plans refusés/K3 relu. [Préparation à intégrer au choix D6](../../audit_d6_preparation_20261007/README.md). [K contre-rejouée](../../audit_reponses_20261008/d6_session_k/README.md) : 126 sorties/630 passes, ratios reconstruits. u24/u32 CPU +0–1 % sur mêmes coordonnées ; homothétie ×2048 <2²⁹, pas «32 bits pleins» ni précision physique nouvelle. [Erratum accepté `8da450ab7`](../../g4_fullk_20261008/ERRATUM_20261008.md) ; seuil D6 produit toujours ouvert.

## CST-0233

Source : `morsehgp3D_v12/audits/CONSTATS.md:76` au pin ci-dessus. Date : 2026-10-07.
Auteur : auditeur (Codex). État : **en cours**.
Pin du constat : `58d384721`, `assemble.cpp`, `table.cpp` ; session F2.
Témoin conservé : [prises brutes](../../audit_performance_20261007/mesures/README.md), [scan proposé : preuve, 3 537 confrontations et quatre mutants](../../audit_performance_20261007/assemblage/README.md).

Cause historique corrigée : finition parallèle depuis `8ba7d7287` ; [K CPU](../../audit_reponses_20261008/cpu_popcount/README.md), C 274–329 ms dont finition 59–72 ms. Gain isolé Gc non acquis. [Réemploi S*](../../audit_reponses_20261008/catalogue_radix_reuse/README.md) : patch/modèle, retrait demandé 40n+1056t+1056 octets, pas un pic ; qualification native requise. [C budgets](../../audit_reponses_20261008/t2d_c_budgets/README.md) : neuf paires locales ; CUDA/reprise après refus ouverts. [Rangs C→T/K1](../../audit_reponses_20261008/t_naissances_reutilisees/README.md) : export 4N à mesurer. [Modèle K](../../audit_reponses_20261008/session_k_recouvrement_modele/README.md) : recouvrement parfait à coûts inchangés, 31/37 maxima >100 ms ; aucune borne sur A. [Après C, trois FULL](../../audit_reponses_20261008/session_t2dc_recouvrement_modele/README.md) : scénario 96,05/78,67/103,48 ms, pas de nouveau FULL 37 trames. [Historique](../../audit_canal_20261008/suivis_9c9c/README.md#cst-0233).

## CST-0234

Source : `morsehgp3D_v12/audits/CONSTATS.md:77` au pin ci-dessus. Date : 2026-10-07.
Auteur : auditeur (Codex). État : **en cours**.
Pin du constat : `58d384721`, `leaf_common.hpp`, `leaf_census.hpp`.
Témoin conservé : [sept feuilles instrumentées ; variante d'arrêt CPU, émissions mot à mot et compteurs conservés](../../audit_performance_20261007/feuilles/README.md) ; aucun gain temps acquis.

B livré ; qualification locale et limites dans [historique](../../audit_canal_20261008/suivis_9c9c/README.md#cst-0234), gain isolé non acquis. [Objet CPU](../../audit_reponses_20261008/cpu_popcount/README.md) : 496 appels logiciels ; [wrappers SWAR](../../audit_reponses_20261008/cpu_popcount_asm/README.md), cinq→zéro, sans gain produit qualifié. [Patches symétrie/Q2](../../audit_reponses_20261008/cpu_live/README.md) : 17 628 cas/dix mutants de modèle, warp conservé ; natif ouvert. [MES-C appariée](../../audit_reponses_20261008/session_c_diagnostic/README.md) : C porte l’écart CPU/GPU ; 41/41 petits réels ralentis à W48/W4, coût des fils aussi dans G/TMVR. [Pool `5b3362bbd`](../../audit_reponses_20261008/pool_equipes/README.md) : synchronisation/modèle favorables, gain G4 non acquis.
