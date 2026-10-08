# Audit Codex — état courant v12

8 octobre 2026, 19:15 UTC. R1 `8a0716e74` reste la référence adoptée ; B3 `545ed987e` échoue au banc.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Autorité : [registre](CONSTATS.md).

**Campagne d’état FULLN : contrat multi-séquences à 100 ms non tenu.**
[Protocole](../receipts/audit_reponses_20261008/fulln_protocole/README.md),
[relecture](../receipts/audit_reponses_20261008/session_fulln_admission/README.md) et
[temps indépendants](../receipts/audit_reponses_20261008/fulln_temps/README.md) :
38 processus, 610 passes, 392 chaudes ; R1 sans A6b/B3, u21, W48, cache 8 Gio.
GPU = catalogue GPU puis tour CPU ; CPU = chaîne entière CPU. Médianes chaudes réunies, en ms :

| FULL | ng00, 39 885 sites | ng01, 35 551 sites | ng02, 45 845 sites |
| --- | ---: | ---: | ---: |
| **GPU K5** | **80,31** | **66,59** | **83,32** |
| **CPU K5** | **354,36** | **298,90** | **361,18** |
| GPU K10 | 495,10 | 366,14 | 422,19 |

K5 GPU : cinq processus × neuf chaudes ; CPU K5/GPU K10 : trois × quatre par trame.
Sur **37 trames de six séquences**, cinq secondes visites chacune : médiane des médianes
**142,41 ms**, pire médiane **287,19 ms**, maximum brut **288,22 ms** ; **14/37** sous 100 ms.
Lecture, masque, ouverture, validation, FUL1 et libération hors mur ; CPU K10/37 non mesurés.
488 sources exactes, archive close, 747 CTests déclarés réussis, arrêt certifié. Limites : codes/stderr
individuels non archivés, ELF hachés avant campagne seulement ; relecture concordante, fermeture incomplète.
Comparaison CPU M sans cache non causale ; `cpu_ns` cumule les fils.

Petits MES-C : C1/C2/C3 non tenus ; CPU K5/W48, coût fixe ajusté **14,28 ms**, pente **9,697 µs/site**.
Huit refus `wide_leaf` sur deux sphères, CPU/GPU, K5/K10. Sur 132 réels, médiane par nuage
CPU **23,845 ms**, GPU **7,315 ms**.

**Priorités CPU et FULL.** C représente **83,78–85,20 %** du CPU et dépasse 100 ms sur les 36 chaudes
(minimum **253,07 ms**) : à C fixé, G seul ne peut suffire. Annuler seulement la queue GPU,
reste de chaque passe fixé, laisserait **18/37** médianes sous 100 ms ; pire **214,64 ms**.
Ces bornes conditionnelles ne prédisent aucun gain A6c ; ne pas additionner les médianes d’étages.
[Diagnostic CPU](../receipts/audit_reponses_20261008/fulln_petits_cpu/README.md) : feuilles **49–52 % de C** sur ng00–02 ;
les annuler seules laisserait **199,70/173,29/214,27 ms** de FULL médian.
Sur 20 réels ≤150 sites, CPU4 **6,12 ms**, CPU48 **10,00 ms** ; parcours/finition à examiner,
sans seuil de fils acquis. [Réduction census proposée](../receipts/audit_reponses_20261008/cpu_census_reduction/README.md) :
préfixe, fautes et coquille conservés dans 104 084 cas Python ; aucun natif ni gain qualifié.

**B3 : trois leviers en échec temporel, A/A valide.**
[Session relue](../receipts/audit_reponses_20261008/session_t2db3_admission/README.md) : 387 journaux, identités concordantes ;
10 codes G et stderr individuels absents. 747 CTests et deux campagnes mutantes CPU réussis,
rapports mutants individuels absents ; admission stricte d’identité incomplète.
[Contre-calcul indépendant](../receipts/audit_reponses_20261008/t2db3_stats/README.md) :
300 processus × huit passes, 2 100 chaudes, sans empreinte dans les prises ; règle sur rapports
appariés des médianes de processus, distincte de FULLN. Bornes hautes bloquantes : clés ng02 **1,012667** ;
lot ng01/ng02 **1,046561/1,017223** ; balayage ng00/ng01/02-001606 **1,000937/1,000225/1,001474**.
Aucun tour retiré ; arrondir une borne à 1,000 ne la rend pas inférieure à 1.
Le contraste clés/transfert échoue aussi sur ng02 : G **−2,446 ms**, queue **+3,140 ms**, FULL **+0,748 ms**
en moyennes appariées. Avancer la fin G peut allonger la queue sans accroître le travail forêt.
Prochaine piste : fins et ordonnancement sur ng02 ; aucun gain FULL déduit d’un lookup plus rapide.

[Preuve des clés/populations](../receipts/audit_reponses_20261008/b3_supports_math/README.md),
[compléments de portes](../receipts/audit_reponses_20261008/b3_portes_prelecture/README.md) et
[clarification mémoire](../receipts/audit_reponses_20261008/b3_memoire_voies/README.md) : **16 octets/boule hôte** ;
D2H supplémentaire en finition complète, reconstruction hôte en tranches, y compris dans le bras « transfert ».
Les capacités massives R1 ne se transfèrent pas à B3 ; nombre de boules absent des journaux L1t/L2t.
[CST-0018](../receipts/audit_reponses_20261008/b3_identite_admission/README.md) reste ouvert : juge original embarqué,
Contre-exemple synthétique : 35 journaux d’identité supprimés encore acceptés par sa garde. [Correctif v2 proposé](../receipts/audit_reponses_20261008/b3_identite_proposition/README.md),
27 cas + 21 contrôles indépendants ; intégration et porte restantes. B3 non adopté, retrait du produit à contrôler.

**Massifs R1 : record GPU Paris sans sol, 9 111 422 sites, K5 en 46,453 s froid.**
[L2t](../receipts/audit_reponses_20261008/session_l2t_admission/README.md) : un succès, trois refus mémoire sans stade ;
ni FUL1 ni chrono chaud, ELF initial seul. Record [CPU L2](../receipts/audit_reponses_20261008/session_l2_admission/README.md) :
Paris brut 14,552M en 165,789 s froid. Aucun plafond universel en sites.
**CST-0243 ouvert** : TU Wien sans sol 5,200M réussit 32,856 s puis refuse la passe1.
[Diagnostic](../receipts/audit_reponses_20261008/b1t_tuwien_memoire/README.md) : 79,99 Gio CUDA gardés, 8,012 Gio de marge,
stade inconnu. [Porte de réemploi proposée](../receipts/audit_reponses_20261008/resident_reemploi_couverture/README.md), sans correctif qualifié.

[Contrôle GCP demandé à l’auditeur](../receipts/audit_reponses_20261008/g4_controle_auditeur/README.md), **19:11:32 UTC** :
VM déjà TERMINATED, garde ciblée exécutée, zéro VM E-HGP active ; aucun nouvel ordre d’arrêt nécessaire.
Aucun moteur exécuté par l’auditeur ; sources, métadonnées et modèles Python seulement pour les qualifications.
