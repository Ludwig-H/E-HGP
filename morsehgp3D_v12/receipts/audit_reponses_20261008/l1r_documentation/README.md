# L1/L1R — corrections documentaires proposées

Contrelecture de la publication `302dc9fe675816b02c24507aa9a1e1915dfa5dae`.
Lecture Git et arithmétique JSON seules, sans moteur, GCP ou entrée LiDAR.
Les colonnes publiées sont recoupées avec les admissions indépendantes
[L1](../session_l1_admission/README.md) et
[L1R](../session_l1r_admission/README.md) : noms, K, voie, sites, passes,
CPU·ns, RSS, budgets, chronos et mémoire par étage correspondent.

Les corrections de `proposition.patch` portent seulement sur README/PLAN :

- **Huit refus**, six K5 et deux K10. Par session : 18 processus pour
  15 scènes, dix succès (neuf scènes), 18 passes complètes dont huit chaudes.
  B1/B2/B4 sont non tenus ; B3 reste non évalué.
- Les refus bruts ont seulement `open` puis `exit` avec `memory_budget`.
  **Le poste hôte/appareil n'est pas identifié**. L'appareil est une piste
  de diagnostic ; attribuer tous les refus à son budget dépasse ces bruts.
- `P+C+G+raccord+TMVR ≤ mur` et `T+M+V+R ≤ TMVR`. Le résidu TMVR de SCION
  sans sol vaut 726 354 479 ns : l'égalité écrite dans README est fausse.
  Deux cas n'ont qu'une passe froide ; leur temps reste étiqueté comme tel.
- L1R : GNU time publie **62 437 420 KiB**, soit **63 935 918 080 octets**
  ou **59,544964 Gio**. Ce maximum du processus correspond ici exactement
  au maximum du champ RSS publié par passe. « 62,4 Go » est une conversion
  incorrecte. L1 originale donne séparément 63 936 311 296 octets ; les
  deux sessions ne sont pas confondues.
- Les derniers murs L1/L1R diffèrent de 0,072 à 3,476 % relativement à L1R,
  sans gain causal établi. Les coûts T/site de scènes différentes ne
  prouvent pas une loi superlinéaire ; FULL ne ventile pas les composantes
  internes de T. L'extrapolation de capacité hôte est
  une projection indicative. La comparaison FUL1 appareil/CPU ne porte
  que sur Boreas 10 trames sans sol, contrairement aux seules comparaisons
  entre passes des deux cas Boreas 1 trame.

Les budgets sont bien séparés : 171 798 691 840 octets hôte (160 Gio) et
94 489 280 512 octets appareil (88 Gio). L1 est épinglée à `403736300`, L1R
à `ea62cd691` ; leurs fichiers `src/` sont égaux, leurs sondes diffèrent.
Les SHA des rapports Git publiés diffèrent de ceux des archives originales
notamment après remplacement des chemins ; la contrelecture compare les
champs numériques et identités, sans revendiquer l'égalité de ces fichiers.

Rejeu, depuis un clone contenant le commit et les reçus d'admission :

```sh
python check.py --repo /depot --admissions /depot/morsehgp3D_v12/receipts/audit_reponses_20261008
python -O check.py --repo /depot --admissions /depot/morsehgp3D_v12/receipts/audit_reponses_20261008
```

Les deux sorties sont égales au champ `result` de `capture.json`. Le patch
est proposé, non intégré par l'auditeur : `git apply --check`, application
sur extraction temporaire des deux fichiers et hashes résultants contrôlés.
Les preuves de fermeture et l'admission globale restent dans les reçus
séparés ; aucune campagne réelle n'est invalidée par ces corrections de texte.
