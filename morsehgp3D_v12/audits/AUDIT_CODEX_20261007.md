# Audit Codex — état courant v12

8 octobre 2026, 18:53 UTC. FULLN : produit R1 `8a0716e74`, après retrait d'A6/A6b ; B3 absent.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Dernières mesures FULL : contrat multi-séquences à 100 ms non tenu.**
[Protocole](../receipts/audit_reponses_20261008/fulln_protocole/README.md) et
[contre-extraction indépendante](../receipts/audit_reponses_20261008/fulln_temps/README.md) :
38 processus, 610 passes, 392 chaudes retenues ; u21, W48, cache 8 Gio.
GPU désigne le catalogue GPU suivi de la tour CPU ; CPU désigne toute la chaîne CPU.
Médianes de toutes les passes chaudes, en ms :

| FULL | ng00, 39 885 sites | ng01, 35 551 sites | ng02, 45 845 sites |
| --- | ---: | ---: | ---: |
| **GPU K5** | **80,31** | **66,59** | **83,32** |
| **CPU K5** | **354,36** | **298,90** | **361,18** |
| GPU K10 | 495,10 | 366,14 | 422,19 |

K5 GPU : cinq processus × neuf chaudes/trame ; CPU K5 et GPU K10 : trois × quatre.
Sur **37 trames de six séquences**, cinq secondes visites par trame : médiane des médianes
**142,41 ms**, pire médiane **287,19 ms**, maximum brut **288,22 ms** ; **14/37** sous 100 ms.
Pas de CPU K10 ni de CPU sur les 37 trames ici. Lecture, masque, ouverture, validation,
FUL1 et libération hors mur ; `cpu_ns` est le temps cumulé des fils, pas une latence CPU.
La comparaison avec CPU M sans cache reste descriptive, sans gain causal attribué.

[Relecture FULLN](../receipts/audit_reponses_20261008/session_fulln_admission/README.md) concordante :
488 sources exactes, archive close, 747 CTests déclarés réussis, arrêt certifié.
Limites : codes/stderr individuels des sondes non archivés, ELF hachés avant campagne seulement.
Pas de fermeture indépendante complète ni d'adoption nouvelle déduite de ces observations.

Petits nuages MES-C : C1/C2/C3 non tenus ; CPU K5/W48, coût fixe ajusté **14,28 ms**,
pente **9,697 µs/site** ; huit refus `wide_leaf` sur deux sphères, CPU/GPU, K5/K10.
Sur les 132 petits réels, médiane par nuage CPU **23,845 ms**, GPU **7,315 ms**.

**Priorités mathématiques pour le développeur.** Sur les mêmes 36 chaudes CPU, C prend
**83,78–85,20 %** du FULL en moyenne des rapports et dépasse 100 ms à chaque passe
(minimum global **253,07 ms**). À C inchangé, accélérer G seul ne peut tenir 100 ms.
Sur les 37 trames GPU, annuler seulement la queue après G, tous les autres termes de chaque
passe fixés, laisserait **18/37** médianes sous 100 ms : trame médiane **100,75 ms**, pire
**214,64 ms**. Cette borne conditionnelle ne prédit aucun gain A6c ; réduire aussi G/C reste nécessaire.
Les médianes d'étages ne s'additionnent pas ; les sommes appariées sont dans le reçu.

[Diagnostic CPU ciblé](../receipts/audit_reponses_20261008/fulln_petits_cpu/README.md) : feuilles **49–52 % de C** sur ng00–02 ;
les annuler seules laisserait **199,70/173,29/214,27 ms** de FULL médian.
Sur 20 réels ≤150 sites, CPU4 **6,12 ms** contre CPU48 **10,00 ms** ; parcours/finition à examiner.
Comparaison descriptive : aucun seuil de fils ni gain futur acquis.
[Réduction census CPU proposée](../receipts/audit_reponses_20261008/cpu_census_reduction/README.md) : même préfixe, fautes et coquille ; modèle Python seulement, gain à mesurer.

**B3 livré `545ed987e`, non adopté.** [Prélecture mathématique](../receipts/audit_reponses_20261008/b3_supports_math/README.md) :
clés et appartenance équivalentes sur catalogues produit réussis ; 957 requêtes Python, aucun natif.
Surcoût hôte **16 octets/boule**, copie Pool et transfert GPU ; gain à mesurer.
[Portes relues](../receipts/audit_reponses_20261008/b3_portes_prelecture/README.md) : 746 passées / une sautée sur R1+B3 ;
six mutants préparés, exécutions non admises ; deux portes négatives proposées.
**[CST-0018 reproduit](../receipts/audit_reponses_20261008/b3_identite_admission/README.md)** :
trois leviers encore « adoptés » après suppression des 35 journaux d'identité, en mode strict.
[Correctif proposé](../receipts/audit_reponses_20261008/b3_identite_proposition/README.md) : 27 cas et 21 contrôles indépendants,
schéma futur v2, seuils inchangés. À intégrer avec sa porte ; aucune mesure G4 B3 admise.

**A6b [rejeté](../receipts/audit_reponses_20261008/session_a6b_admission/README.md) et
[retiré](../receipts/audit_reponses_20261008/a6b_retrait_r1/README.md)** malgré le gain sur les grandes.
[Diagnostic](../receipts/audit_reponses_20261008/a6b_etages/README.md) : suivre les fins R(K2/K3) et le mur entier ;
réclamer toutes les tranches G ne certifie pas leur achèvement.

**Massifs — record GPU : Paris sans sol, 9 111 422 sites, FULL K5 en 46,453 s à froid.**
[L2t](../receipts/audit_reponses_20261008/session_l2t_admission/README.md) : un succès, trois refus mémoire sans stade nommé ;
aucun FUL1 ni chrono chaud. Source équivalente R1 ; archive/arrêt clos, ELF initial seul.
Paris brut 14,552M et Lyon 24,017/32,413M refusent. Record [CPU L2](../receipts/audit_reponses_20261008/session_l2_admission/README.md) :
Paris brut 14,552M en 165,789 s froid. [L1t](../receipts/audit_reponses_20261008/session_b1t_admission/README.md) :
FUL1 limité à 1,6M, ELF final absent ; aucune série statistique.

**CST-0243 ouvert** : TU Wien sans sol 5,200M réussit 32,856 s à froid puis refuse la passe1.
[Diagnostic](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md) : 79,99 Gio CUDA gardés, 8,012 Gio de marge ; stade inconnu.
Instrumentation proposée sous protocole distinct. [Porte à qualifier](../receipts/audit_reponses_20261008/resident_reemploi_couverture/README.md) :
trois succès immédiats, budget figé, finition complète/front rendu ; seuil Pool non transférable à CUDA.
Aucun plafond universel en sites ni correctif de réemploi qualifié.

Sources, métadonnées et vérifications Python uniquement ; aucun moteur lancé par l'auditeur.
