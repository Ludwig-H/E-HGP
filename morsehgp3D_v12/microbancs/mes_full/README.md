# Microbanc MES-FULL : la tour FULL de la v12 en Session résidente sur G4

8 octobre 2026. Mesure du **contrat** de la v12 (FULL K1..5 en mémoire, verticales comprises, 100 ms sur G4 sur les
trames SemanticKITTI sans sol, plusieurs séquences, médiane et maximum), sur la frontière de mur proposée par
l'auditeur Codex ([`frontiere_full_proposee`](../../receipts/audit_reponses_20261008/frontiere_full_proposee/README.md))
et implantée par la sonde [`bench/full_probe.cpp`](../../bench/full_probe.cpp).

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R) ; bras CPU identifié
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

[`pilote_full.py`](pilote_full.py) (bibliothèque standard, Python 3.10 nu) construit la sonde au profil 21 avec
`MHGP12_ENABLE_CUDA=ON`, puis joue : ng00–02 à K5 sur l'appareil (5 processus × 10 passes, ordre tournant), à K10
(3 × 5), un bras CPU à K5 (3 × 5), et une Session qui enchaîne les 37 trames `v12set` (six séquences, 33 179 à
99 099 sites ; 5 processus, deux tours, le second fait foi). Toutes les passes portent l'empreinte FUL1 (hors du mur) :
une empreinte par trame et par K, identique sur toutes les passes, tous les processus et les deux voies, sinon refus.

**Verdict du contrat, écrit d'avance** : « tenu » si, sur ng00–02 et sur les 37 trames, la médiane et le maximum (sur
les trames, des maximums des médianes par processus) sont au plus 100 ms ; « non tenu » sinon ; « refusé » si un
contrôle manque. Aucune règle d'adoption : c'est la mesure du contrat, publiée telle quelle avec les étages P, C
(transferts compris), G, T, M, V, R.

Mode `--essai --sonde <binaire>` : logique jouée en local sur la voie CPU (minima relâchés, deux trames `v12set`),
verdict « essai », jamais publié comme mesure. Essai du 8 octobre : aucun refus, empreintes identiques.
