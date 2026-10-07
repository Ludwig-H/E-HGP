# Réponse du développeur : outils de données, cache, juges de M2, M4, M5 et M6 (CST-0018, CST-0216 à CST-0218, CST-0220, CST-0222, CST-0223)

7 octobre 2026. Constats de l'auditeur Codex lus aux pins `4147c5460`, `e30000dec` et `07ee13ef6`
([registre](../../audits/CONSTATS.md)). Correctif fait par un agent du développeur sur `df9140b5d`, relu, fusionné à la
main sur `main` (une ligne du tableau de `microbancs/README.md`), portes rejouées par le développeur. Détail fichier par
fichier : [`outils_juges_CHANGEMENTS.md`](outils_juges_CHANGEMENTS.md) ; journal de l'agent :
[`outils_juges_RAPPORT.md`](outils_juges_RAPPORT.md). Les états du registre restent à l'auditeur. GCP non utilisé.

| Constat | Correction | Porte (rejouée par le développeur) |
| --- | --- | --- |
| `CST-0216` pilote de rejeu | `bench/data/replay_all.sh` cherche ses outils à côté de lui ; étape `outils` (rejeu à blanc : empreintes de `SHA256SUMS.txt`, aucun outil non épinglé) jouée avant toute autre ; `ROOT` obligatoire | `bench/donnees_test.py` : 37 cas, 0 écart |
| `CST-0217` vérificateur | `verify_inputs.py` admet le manifeste avant toute lecture (schéma, cas non vides, empreintes de 64 hexadécimaux, comptes positifs) | idem |
| `CST-0218` découpes | carré horizontal fermé de rayon $r_N$ (distance de Tchebychev du $N$-ième site) : une colonne est gardée entière ou pas du tout ; `count` réel et `target_sites` publiés ; **les 69 découpes publiées sont à rejouer** (`DONNEES.md`, en-tête et § 3) | idem |
| `CST-0220` cache | `data_cache.py` : liens durs comptés, plancher limité à ce qui rend des blocs, décision avant toute éviction | `bench/data_cache_test.py` : 8 cas, 0 écart |
| `CST-0222` M5 émission | nombre de tâches en `u64` par un seul `tasks_of`, garde de capacité avant toute réservation | `host/traversal_identity.cpp --unit` (mutant tué) |
| `CST-0223` lecteur M5 | lecteur strict : tests G1 hors bornes, somme débordante, racine ou boîte hors enveloppe exacte, feuille qui aurait dû être coupée, refusés | `host/format_selftest.cpp` : 21 cas dont les 4 références de l'auditeur |
| `CST-0018` juges | M5 sur le modèle de M2 (relecture `--rejudge`) ; résidus du reçu `audit_cd_corrections` : M2 (preuves Compute Sanitizer validées comme une prise, échauffement concordant, formes cohérentes), M4 (un mutant n'est tué que par une sortie complète rattachée à son cas), M6 (`mes_m6_ok` seulement sur preuves, `--rejuger`) | `test_juge_m2.py` 40 cas, `test_pilote.py` 38 cas, `test_juge_m5.py` 30 cas, `test_juge_m6.py` 44 cas : 0 écart ; relectures des sessions A–D : verdicts retrouvés |

**Mutants des juges** (`microbancs/outils/mutants_juges.py`, rejoué par le développeur sous `python3 -S -O` avec les
reçus des sessions A à D) : 7 témoins non mutés conformes, **66 mutants, 66 tués** (44 nouveaux). Un premier passage
du développeur a laissé le lot « tour » vivant faute des binaires mutants dans sa construction locale (témoin
`TEMOIN_EN_ECHEC`, comme prévu par l'outil) ; avec toutes les cibles construites, ce lot a son témoin conforme et ses
14 mutants tués.

**Ce qui reste** : rejouer les données (`replay_all.sh crops verify bundles`) et remplacer les tables du § 3 de
`DONNEES.md` ; rejouer sur G4 les pilotes durcis de M2, M3/M4, M5 et M6 avant toute nouvelle adoption (les sessions A
à D ont été relues, pas rejouées) ; deux observations non traitées, notées au journal de l'agent (`prepare_outputs` et
la couverture partielle de M2).
