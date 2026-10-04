# Sorties paramétrées : preuves et décisions — 4 octobre 2026

La dernière décision utilisateur est **Q_b seul**, **K-parties reliées**, puis **supports, points, plat**.
La réponse primaire du 4 octobre à 20:25:29.306 UTC est conservée avec la question, toutes les options et les
empreintes des événements dans [USER_DECISION_20261004T202529.json](USER_DECISION_20261004T202529.json).
Le plan révisé de 19:46 proposant `POP=P_b` est donc dépassé sur ce point. Il reste une proposition historique,
pas une autorisation de publier ces populations. La sortie Q_b seule conserve la colonne `SITES.point_id` de
la spécification initiale.

L'option choisie dit exactement : « Les K-parties (sommets de Γ_K) reliées par la boule : K+1 à chaque jonction
ordinaire. » Elle ne dit pas « les K-parties contenant Q ». Ces deux comptes diffèrent : dans le triangle aigu
à K=2, la boule relie trois paires, tandis qu'aucune paire ne contient son support q3 entier. Définir le compteur
par boule avant de le présenter avec chacun de ses supports ; conserver à part les incidences/cofaces par Q.

Un constat matériel de contrat reste à corriger avant les formats : [le révisé](sources/CRITIQUE_ET_PLAN_REVISE.md)
lignes 157–158 et 332 annonce des octets identiques sous réétiquetage, mais stocke les PointId originaux
ligne 300. [La spécification initiale](sources/SPECIFICATION_FINALE.md), lignes 427–429, distingue correctement
la colonne d'ID. Deux autres exceptions doivent rester explicites : les étiquettes plates sont dans l'ordre
d'entrée (819–820) et le manifeste conserve les hashes des fichiers d'entrée (832).

Le [modèle autonome](check_identity_and_reader.py) montre, sur trois positions distinctes admises en u21 :

- permutation des lignes : SITES canonique identique, mais hashes des entrées et vecteur plat dans l'ordre
  d'entrée différents ;
- réétiquetage injectif : géométrie identique, colonne d'ID et hashes différents ;
- sous une injection arbitraire, le plus petit ID d'un cluster n'est pas nécessairement l'image de son ancien
  plus petit ID. Comparer les partitions après inversion de la permutation et mise en correspondance des IDs.

Correction minimale : identité binaire en W sur **la même entrée et la même demande** ; invariance géométrique
après permutation/réétiquetage, avec exceptions explicites pour les ID, les labels dans l'ordre d'entrée et la
provenance des octets bruts. Ce témoin vérifie des clauses de format proposées, pas un export natif exécuté.

La revue du juge SHA de S4 est favorable. Le `main` et la classe `Gate` exacts du pin
`f98aeed67d4030dd78e11d5faf7d8556c4d17aaf` sont extraits par AST et jouent sur un transport fictif :
1 242 requêtes, plancher 1 244 ; réponse correcte acceptée, sortie vide, manquante, incorrecte, sans LF final,
refus, signal et délai tous rejetés. **Aucun processus ni appel natif n'est lancé.** Cela ne qualifie pas
l'implémentation C++ du SHA.

Les acteurs réels sont distincts du worktree Claude resté `57dd21be1`, propre :

| Tranche | Source de l'acteur propre capturée | Portée actuelle |
| --- | --- | --- |
| S2 | `257aabb9291f42eb38bc80561c49fb0b07115845`, `build/v11-impl-s2` | parapluie public tower, rapports locaux ; aucune qualification G4 transférée |
| S4 | `f98aeed67d4030dd78e11d5faf7d8556c4d17aaf`, `build/v11-impl-s4` | module io ; rapports locaux, nouvelle matrice G4 encore nécessaire |

Les ports natifs `points`, `head` et la façade ne sont pas livrés par S2/S4. Les tests locaux annoncés dans les
rapports restent des déclarations d'auteurs ; ils n'ont pas été rejoués ici. Les remarques déjà relevées dans
`verif_s4.md` ne sont pas présentées comme des découvertes nouvelles. Les performances relèvent de la revue
parente et ne sont ni recalculées ni qualifiées dans cette capsule.

Rejeu portable depuis ce dossier, sans paquet :

```sh
python3 -B -S check_identity_and_reader.py
python3 -B -O -S check_identity_and_reader.py
```

**79 gardes explicites**, sorties conservées identiques, stderr vides. Aucun build, fit ou GCP.
Les 26 sources copiées sont vérifiées avant/après ; une actualisation finale peut constater un changement
du chantier vivant sans modifier les copies. `LEDGER.json` inventorie les payloads, et `SHA256SUMS` les
inventorie tous, ledger inclus ; seul le SHA256SUMS de racine est exclu de son propre inventaire.
