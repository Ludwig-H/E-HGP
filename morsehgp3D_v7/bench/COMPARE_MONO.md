# Banc historique Gabriel B/C : campagnes appariées et séparations WSPD

Ce banc conserve le différentiel historique de la CLI ; il ne qualifie
pas FULL. La [sonde FULL v5](../docs/CONTRAT_SONDE_FULL_MEB.md) mesure
séparément le proposeur MEB à P explicite. La [campagne eager/lazy](../docs/RESULTATS_MONO_FULL_LAZY_20260905.md)
garde ses sources et mesures historiques ; ses reçus ne deviennent pas
ceux du nouveau raccord.

Cadre : `exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

`compare_v6_v7.py` conserve ses paramètres par défaut : K1–10, s=8, huit workers demandés, deux étages en vol, sans jointure immédiate. Les options supplémentaires sont explicites :

- `--kmax 10|5` sélectionne respectivement `smax=11|6`, le nombre exact de cardinalités et la chaîne des dix ou cinq digests.
- `--serial-stages` impose `threads=1`, `fold-inflight=1`, `fold-join=1`. Le parseur vérifie ces paramètres observables. Ce sont des **étages sérialisés demandés**, pas une preuve qu'aucun thread auxiliaire n'a été créé. Cette preuve relève des portes du binaire consommé.
- `--separations 8 10 12` étend la matrice, sans confondre s (séparation WSPD) et smax. Les objets sont comparés dans chaque paire puis entre séparations, seulement après succès des deux membres d'une paire. Les compteurs de travail internes peuvent changer.
- `--reference-version v6|v7`, par défaut v6, désigne la version réelle du payload de référence. Les rôles `reference` et `candidate` restent distincts, notamment pour B/C qui émettent tous deux un payload v7. Les fichiers utilisent alors ces rôles pour éviter toute collision.

Exemple B/C, à lancer seulement quand sources et binaires sont gelés :

```bash
python3 morsehgp3D_v7/bench/compare_v6_v7.py --reference build/v7_mono_baseline/mhgp7_B --candidate build/v7/mhgp7 --reference-version v7 --output morsehgp3D_v7/receipts/mono_bc_example --sizes 8000 --families uniform --seeds 3 --serial-stages --kmax 10 --separations 8 10 12 --timeout 600
```

La borne est par processus ; cet exemple contient six processus successifs. Aucune limite ne transforme une censure en succès. Les horodatages de stabilité et hashes ne prouvent pas que les binaires ont été construits depuis les sources courantes : rattacher chaque SHA binaire à son reçu de construction, notamment B conservé alors que les sources sont passées à C. Le runner conserve cet avertissement dans `source_binary_binding`.

Le temps externe comprend lancement, génération et digest ; il n'est pas un temps chaud entrée déjà en RAM. Les seuils de 1 seconde puis 100 ms demandent leur protocole de mesure propre et l'exactitude du même objet. Ce banc ne promeut ni performance ni exactitude. Les refus, divergences, erreurs de format et censures restent dans les reçus.

Porte locale, avec fixtures synthétiques explicitement non scientifiques :

```bash
python3 -B morsehgp3D_v7/tests/compare_campaign_gate.py
python3 -B -O morsehgp3D_v7/tests/compare_campaign_gate.py
```

GCP non utilisé par cette extension du banc.
