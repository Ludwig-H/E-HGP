# Oracle local du transport des intérieurs q4

Source : `check.py`, SHA-256
`e8871cdc7a1ea98f9e8adb5797036bb18d4eae626677e339de9799eda674f0d7`.
Exécuté le 26 septembre 2026, Python normal et `-O`, code 0 dans les deux
cas. Aucun moteur, GPU ou fichier de données externe utilisé.
La sortie JSON commune est conservée dans `result.json` ; elle ne contient
ni durée ni affirmation de performance.

Commandes depuis la racine :

```sh
python3 -B morsehgp3D_v9/audits/b_q4_payload_math_20260926/check.py
python3 -B -O morsehgp3D_v9/audits/b_q4_payload_math_20260926/check.py
```

Résultat commun : 126 fixtures, 805 groupes de racines, 257 groupes avec
contacts multiples, 250 groupes d'extrémité, 7 664 IDs intérieurs
comparés ; groupes superficiels K3..K10 = 2/5/7/8/104/183/246/311 ;
29 transitions profond→superficiel effectivement rencontrées. Sept
contre-épreuves mathématiques locales, pas des mutations compilées du
moteur. Le script emploie des refus explicites, pas `assert`.

L'identité est testée aussi pour les groupes non positifs : elle est
affine et reste vraie ; seuls les groupes passant les validations du
producteur pourront émettre. Le calcul Python direct des frontières
n'est pas un prototype performant de scans parallèles. Aucune borne de
croissance globale ni mesure LiDAR/G4/FULL n'en découle.

Voir [l'analyse et le plan de port](../AUDIT_B_TRANSPORT_INTERIEURS_Q4_20260926.md).
