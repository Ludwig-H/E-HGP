# Suivi des correctifs du socle — capture du 2 octobre 2026 à 08:30 UTC

**Les causes des anciens défauts sont supprimées dans le code relu ; leur requalification native/G4 reste à fournir.** Un nouveau défaut de reconnaissance du signal est identifié dans l'aide `mhgp11_expect_abnormal_stop`, sans exécution locale.

Périmètre : `suivi_verrous/snapshot/morsehgp3D_v11`, base `986f75799e8b8112080ce61bc06200721e69391c`, capture **08:30:04.919611 UTC** ; 19 fichiers relus et rapprochés de `snapshot.json`. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u18_input_only / not_claimed`. Aucun build, compilateur, CMake, CTest ou campagne locale lancé ; GCP non utilisé. Aucune arrivée live ultérieure incorporée.

## Fermeture au niveau du code

| Sujet | Constat sur la capture | Preuves d'exécution encore attendues |
|---|---|---|
| `Result<T>` / `guarded` | `src/core/status.hpp:118-168` remplace le T systématiquement construit par `optional<T>` ; le constructeur de refus est `noexcept` et n'engage aucun T. Le succès exige un déplacement sans exception. `value`/`take` vérifient la présence ; une erreur d'usage termine explicitement. L'ancien second `bad_alloc` pendant la fabrication du refus est donc supprimé. | `mhgp11_core_fault_refusal` : contrôle positif allocant, aucune tentative d'allocation sous pénurie, refus au milieu d'un calcul (`tests/core/alloc_fault.cpp:91-139`). Portes de mauvaise utilisation et refus du déplacement levant (`tests/core/tests.cmake:9-17,47-48`). Présence lue, non exécutées ici. |
| `StageTimer` | Remplacé par `Stopwatch`, qui ne conserve ni nom ni registre ; destruction triviale vérifiée (`ledger.hpp:55-69`). L'étage écrit explicitement sa durée à un endroit où `guarded` peut convertir l'allocation. Le risque de référence pendante et d'allocation dans le destructeur disparaît avec cette interface. | `ledger_test.cpp:91-102` teste monotonie et écriture explicite ; `alloc_fault.cpp:142-200` teste les mises à jour/fusions sous pénurie. Non exécutés ici. |
| Témoin sauté / faux TUE sans juge | `run_mutants.py:253-257,279-323` lit `GateRun`, exige `passe` pour le témoin, `echec` avec cause reconnue pour TUE. `mhgp11_gate.py:148-173` exige exactement une entrée JUnit du bon nom ; un saut n'est plus un succès, un rapport absent ne tue plus. Les causes lancement impossible et absence de verdict ne sont pas reconnues comme des morts. | Les contre-tests avec vrais CMake/CTest sont ajoutés à `test_run_mutants.py:303-365` ; ils sont raccordés par `tests/support/tests.cmake:134-136`. La simulation très courte ci-dessous confirme le classement du lecteur, pas l'intégration réelle. |
| Registre / saut | `gates.cmake:286-330` interdit désormais tout test de sous-dossier et les propriétés qui changent le verdict. `run_expect.cmake:90-102` retient stdout/stderr, retire le jeton de saut enfant et refuse son usurpation ; le précontrôle est seul autorisé à sauter. | Fixtures sous-dossier/propriétés ajoutées (`gate_fixture/CMakeLists.txt:50-63`) et portes enregistrées (`tests/support/tests.cmake:138-165`) ; porte de jeton usurpé aux lignes 63-69. Non exécutées ici. |
| `--list` construction | `run_mutants.py:404-408` affiche `construction:<jeton>` pour les mutants sans porte. | **Petit contrôle effectué** sur le manifeste core figé : code 0, 75 lignes, entrées de construction présentes, normal/−O identiques. Ce sont 75 entrées listées, pas 75 mutants exécutés ou tués. |

Le remplacement de `StageTimer` par `Stopwatch` est plus simple que les premières pistes de handle évoquées dans la réponse du développeur et ferme bien la cause identifiée. La table de provenance de la capture reste « À remplir par tranche » ; cette relecture ne transforme ni l'ajout de portes ni les comptes R2 historiques en qualification v11.

## Nouveau point P2 : la porte « arrêt anormal » accepte une ligne imitée

`cmake/gates.cmake:161-181` imbrique `run_expect` : la commande intérieure attend 0 ; la porte extérieure attend son code 1 et la présence de `run_expect_verdict arret_anormal`. Or `run_expect.cmake:121-126` recherche **une ligne quelconque**, pas le verdict final.

Trace déduite directement du code :

1. Une sonde imprime `run_expect_verdict arret_anormal` puis termine **normalement avec le code 3**.
2. Le wrapper intérieur reproduit cette ligne, écrit ensuite son propre verdict `code` et termine avec le code 1.
3. Le wrapper extérieur observe le code attendu 1 et trouve la ligne imitée dans sa sortie : il rend 0. Aucun signal n'est nécessaire.

Cela contredit la propriété « seul un signal la satisfait » (`tests/support/tests.cmake:76`). Les probes core actuelles n'écrivent pas cette ligne ; je ne conclus donc pas que leurs résultats soient faux. **Ce contre-exemple est établi par lecture des deux wrappers, sans rejeu CMake/CTest**, conformément à la consigne de tests sur G4.

Correction ciblée : consommer une issue réservée au wrapper ou exiger son **dernier verdict** pour cette aide, sans modifier le sens général de `EXPECT_LINE` quand une ligne quelconque est justement voulue. Ajouter sur G4 un témoin qui imprime la ligne imitée puis sort 3 (et un lancement impossible qui produit un log ressemblant). Un signal réel doit rester le contrôle positif. La fixture minimale `fake_abnormal_stop.py` et la commande exacte de rejeu sont fournies dans `REPLAY_ON_G4.md`, **non exécutées ici**. Statut : démonstration par lecture, reproduction G4 attendue.

## Annexe secondaire : cohérence du lecteur structuré, vérifiée en Python seulement

`mhgp11_gate.py:169-172` recoupe le code CTest pour `status=run`, mais pas pour `status=fail`, et ne recoupe pas un verdict d'échec explicite avec `run` :

| Entrée **simulée** | Classement actuel |
|---|---|
| JUnit `fail`, code CTest 0, verdict `code` | `echec`, cause reconnue `code` : les conditions du TUE sont satisfaites |
| JUnit `run`, code CTest 0, verdict `code` | `passe` |

Ces entrées sont contradictoires. Elles doivent devenir une issue invalide si le contrat veut refuser toute incohérence du résultat structuré. **Aucune occurrence de ces entrées avec CTest réel n'est démontrée ici**, aucun faux `mutants_ok` réel nouveau n'est revendiqué ; cela ne rouvre pas les anciens témoins désormais réparés.

La petite sonde `parse_probe.py` importe les fichiers figés, substitue explicitement le seul lancement CTest par des sorties simulées et exerce huit cas de lecture : succès, saut, rapport absent, échec reconnu, lancement impossible, absence de verdict, deux contradictions. Elle appelle ensuite le vrai `run(... --list)` en mémoire, sans construction ni commande enfant. `normal.json` et `optimized.json` concordent ; deux exécutions de moins de 0,1 seconde chacune. Reproduction : `python -B parse_probe.py`, puis `python -B -O parse_probe.py`.

## Statut à transmettre au développeur

Les réparations précédentes sont **présentes au code avec leurs portes**, pas encore requalifiées par cette revue. Attendre les reçus de la matrice G4 du développeur et les empreintes exactes des sources exécutées ; ne pas reprendre les anciens comptes de portes. Le prochain ajout précis au harnais est le témoin de faux signal ci-dessus. Aucune note d'audit ni source produit modifiée par cette relecture.
