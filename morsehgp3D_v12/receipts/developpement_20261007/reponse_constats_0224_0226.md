# Réponse du développeur aux constats CST-0224 (résidu), CST-0225 et CST-0226 (auditeur Codex)

7 octobre 2026. Constats lus au pin `e5642bfa8` ([registre](../../audits/CONSTATS.md),
[rapport de l'auditeur](../audit_u32_20261007/README.md)). Tous sont acceptés ; les états du registre restent à
l'auditeur. GCP non utilisé.

| Constat | Correction | Preuve |
| --- | --- | --- |
| `CST-0225` lecteur `MHGP12DP` | `microbancs/mes_m3_m4_tour/common/format.hpp` : invariant `at <= taille` ; l'en-tête de section, les données et le remplissage sont chacun comparés à la place restante, sans addition qui déborde ; le produit `elem_bytes × count` est contrôlé avant d'être formé | porte `mhgp12_format_test` (`tests/format_reader_test.cpp`, cible du microbanc) : neuf fichiers synthétiques, dont le témoin de 88 octets annonçant $2^{62}-1$ éléments, le produit débordant, les données, le remplissage et l'en-tête de section tronqués, les octets en trop et la section manquante ; nouveau lecteur : 2 admis, 7 refus, code 0 ; **lecteur d'origine : le témoin de 88 octets est admis, code 1** ; deux vrais vidages relus (ng00 K5 et K10 : 6 097 121 et 45 383 538 incidences) |
| `CST-0224` résidu fichier/dossier | `microbancs/outils/recu_session.py` : la sélection et les chemins expurgés sont calculés **avant** toute écriture ; refus (code 3, rien d'écrit) si deux chemins sont égaux ou si un fichier deviendrait le dossier ancêtre d'un autre ; toute erreur d'écriture ensuite retire les fichiers créés (jamais de reçu partiel sans manifeste) | porte `microbancs/outils/test_recu_session.py` : 11 appels du vrai `main`, adresse fictive ; conforme, nom expurgé, collisions plate et imbriquée, **fichier devenant dossier**, destinations absente, vide et occupée ; code 0 sous `python3 -S` et `-O` ; **outil d'origine : deux exceptions `FileExistsError` sur le cas fichier/dossier, code 1** |
| `CST-0226` table des ports et provenance | `docs/PROVENANCE.md` § 3.1 : ligne du contrat numérique dans le socle (`6a38f7e4b`, développement et non port), profil 32 admis, 18 et 33 refusés ; le paragraphe qui annonçait u32 refusé est remplacé ; `docs/PORTS.md` dit, depuis `35b217dad`, qu'il ne décrit que le port | relecture des deux documents |
| précision des centres (note de l'auditeur, `numerique/README.md`) | `CONTRAT_NUMERIQUE.md` § 4 et commentaire public de `compare_centers` (`src/num/geometry.hpp`) : « parties entières sur 64 bits » n'est vrai que pour une boule certifiée ; le code les calcule en `i128` (`Big` au palier large), ce qui couvre aussi la candidate générique q3 de l'auditeur | aucun changement de code ; commentaire et contrat seulement |
| complément `CST-0113` (`PointId`) | la phrase du contrat T1 qui annonçait des `PointId` dans le format version 1 est corrigée par le correctif du lecteur de transition, intégré à part : le format v1 porte les positions (suffisantes au différentiel géométrique), pas les identifiants | commit du lecteur de transition |

Le script de l'auditeur `audit_u32_20261007/publication/check.py` est épinglé à la source de son pin et refuse donc,
comme prévu, toute version corrigée : la porte ci-dessus en reprend les cas sur le vrai point d'entrée.
