# Contre-audit de la session B : observations M3/M4 confirmées, adoption incomplète

**Les chiffres et identités de la session B sont retrouvés. La qualification
d'adoption définitive reste incomplète.** M3 présente une réduction de résolution
de 45,3 à 45,9 % à K10 sur une seule prise par cas ; M4 satisfait les seuils
chronométriques par ordre à K5 et dépasse celui de contraction à K10. Les cinq
exécutables M3/M4 ne sont pas hachés au reçu : **CST0021** s'applique aux deux mesures,
en plus du manque de réplication de résolution **CST0213**. Aucun constat n'est clos.

Pin : `e30000dec1027c5f0ade3093a94563ee412409d3`. Campagne initiale `f1ea04c40`,
addendum `abe9df451`, instantané mesuré `5bd963078161cff1de8abe17f0595ab657bdc5e7`.
Entrée : [reçu B](../../g4_t0b_20261007/README.md). Audit CPU local, sans build, GPU,
GCP, ni lecture ou copie de nuage/vidage binaire. Cadre : `exploration_v12_hors_registre`,
`cpu_reference`, `quantized_u21_input_only`, `not_claimed`.

## Recalcul M3

Les rapports ci-dessous utilisent les lignes JSONL, indépendamment de la synthèse
du pilote. « MEB » désigne la plus petite boule isolée ; « résolution » comprend
la suite de descentes et le census. Le dénominateur de cette dernière comparaison
est **la réplique v11**, comme déclaré au reçu, pas son bras v11 original.

| Cas | Parties | MEB nouvelle/référence | Résolution nouvelle/réplique v11 | Réplique v11 → v12, secondes |
| --- | ---: | ---: | ---: | ---: |
| ng00 K5 | 1 175 034 | 0,972427 | 0,933698 | 1,465923 → 1,368730 |
| ng01 K5 | 911 687 | 0,947560 | 0,925005 | 1,087422 → 1,005871 |
| ng02 K5 | 1 039 136 | 0,977246 | 0,924009 | 1,293379 → 1,195094 |
| ng00 K10 | 8 856 134 | 0,315154 | 0,542058 | 24,011372 → 13,015559 |
| ng01 K10 | 6 557 526 | 0,311227 | 0,541211 | 16,899912 → 9,146425 |
| ng02 K10 | 7 148 742 | 0,317328 | 0,546885 | 18,840890 → 10,303808 |

Le seuil de réduction de 40 % figure dans `PLAN.md` avant la mesure. Les trois
observations K10 le passent. Le processus de vidage qui mesure la résolution
n'est lancé qu'une fois par cas : une passe interne à K10, minimum de trois à K5.
Les **24 processus M3** (5 × 3 cas K5 + 3 × 3 cas K10) répètent le microbanc MEB,
pas cette résolution. Aucun intervalle interprocessus de résolution ne peut être
reconstruit. L'addendum rend correctement l'adoption M3 provisoire, en demandant
au moins cinq processus de résolution ; le rattachement des binaires reste aussi à compléter.

Les **141 lignes M3 par ordre** sont complètes, identiques aux rapports et sans
écart géométrique ; les routes sont stables entre processus. Les six cas représentent
**25 688 259 parties**, comptées une fois par cas, sans multiplier par les répétitions.
La part T1 est retrouvée : 75,19–79,97 % à K5 et 76,09–79,53 % à K10. Les témoins
et le mutant sans `S ⊆ F` sont non vides ; la variante « mère puis Welzl » conserve
l'identité dans le binaire normal et provoque un écart réel dans le mutant.

## Recalcul M4

Temps de l'ordre K, en ms. Chaque cellule est la **médiane entre processus des
minima de cinq passes internes**, conformément au code et au README du microbanc
antérieurs à la campagne. Les cinq durées individuelles ne sont pas conservées :
ce ne sont pas des médianes de passes, ni un chrono global de forêt.

| Cas | Noyau, un fil | Contraction, un fil | Contraction, 48 fils |
| --- | ---: | ---: | ---: |
| ng00 K5 | 8,78404 | 7,13654 | 1,69953 |
| ng01 K5 | 7,21365 | 5,79734 | 1,46626 |
| ng02 K5 | 9,45878 | 7,90522 | 1,77838 |
| ng00 K10 | 26,39760 | 21,73080 | 4,08831 |
| ng01 K10 | 19,65890 | 16,29630 | 3,14291 |
| ng02 K10 | 24,55410 | 21,08920 | 3,94757 |

Tous les ordres ont été contrôlés : seuil du noyau 10 ms à K5 et 35 ms à K10
satisfait ; contraction parallèle ≤ 3 ms sur tous les ordres K5, dépassée sur
les trois cas K10. La contraction séquentielle ne satisfait pas ce seuil de 3 ms.
L'échec K10 annoncé est confirmé ; il n'a pas été masqué par une nouvelle règle.

Les **24 processus M4 et 165 lignes par ordre** sont complets. L'identité avec
la v11, la racine unique et l'identité de la contraction parallèle sont positives.
Les inventaires contiennent bien `FLOWER` pour chaque ordre ≥ 2, et le nombre de
naissances jugées par T6 égale celui des naissances : **14 829 064** sur les six
cas, comptés une fois. L'ordre 1 n'a pas d'ordre inférieur. La faille générique
**CST0214** n'est donc pas déclenchée par cette campagne. Les mutants sans
contraction échouent réellement en identité, pas seulement par un code arbitraire.

L'indépendance des ordres autorise un futur ordonnancement parallèle ; elle ne
mesure pas son coût. Le banc parcourt les ordres successivement. Exemple ng00 K5 :
somme des noyaux 24,15715 ms, maximum par ordre 8,78404 ms ; ng00 K10 : somme
117,8999 ms, maximum 26,3976 ms. Assimiler le futur mur au maximum reste une
projection à vérifier, avec partage des ressources et coût des verticales.

## Sources, commandes et preuves conservées

Les **79 fichiers du manifeste public** sont présents et leurs hashes concordent.
Les neuf fichiers source M3/M4 du paquet mesuré sont identiques au commit annoncé
et au pin d'audit. L'archive des sources v11 `ac081a06f` est reproduite par
`git archive` et retrouve `6f3454ad9d9ad2f6bb0b846f1aaad7c1a4c509d14cd450a99381c72b04a57885`.
Le hash de `libmhgp11.a` du rapport M3/M4 concorde avec celui de l'étape source v11.
Les six empreintes FULL correspondent aux références gelées de `MESURE.md`.
Les inventaires des **96 fichiers de vidage** concordent avec les cas et populations
des microbancs ; leurs contenus restent exclus de l'audit et du dépôt.

La petite archive locale des résultats (117 685 octets) et le paquet source
(939 057 octets) retrouvent les SHA du reçu, ainsi que les plans JSON et shell.
Les **77 fichiers publics de résultats** sont exactement ceux de l'archive après
anonymisation des répertoires personnels. Les commandes de session concordent
avec les arguments des rapports ; les stdout horodatés contiennent les six
vidages et les **48 appels de processus M3/M4** attendus. Les JSONL individuels
recollent aux rapports, jusqu'aux marqueurs de fin et codes. Aucune répétition
manquante, identité vide ou différence public/brut n'a été découverte ici.

Cette concordance ne corrige pas **CST0018**. Elle n'établit pas davantage les SHA
des exécutables absents : `pilote.py:193` hache la bibliothèque v11, `:242` et
`:251` les vidages ; aucune étape ne hache les cinq `mhgp12_*`. Ils ne sont pas
conservés dans l'archive. Les journaux de construction séparés ne sont pas publiés
non plus ; le rapport conserve les codes CMake/build. **CST0021 reste ouvert**,
et le veto « binaire non haché » du `PLAN.md` §0 antérieur empêche de transformer
ces observations en adoption définitive, même pour M4 K5. Aucun sanitizer des
microbancs M3/M4 n'est établi dans cette session Release.

## Reproduction

Depuis la racine d'un worktree contenant les fichiers du pin et l'historique Git :

```sh
python3 morsehgp3D_v12/receipts/audit_b_m5_20261007/campagne_b/check_campaign_b.py
python3 -O morsehgp3D_v12/receipts/audit_b_m5_20261007/campagne_b/check_campaign_b.py
```

Sans les petites archives locales d'origine, ajouter `--published-only` : le
résultat indique alors explicitement la contre-lecture brute non réalisée.
Le script refuse une différence du fichier consommé avec le pin ; il recontrôle
les hashes en fermeture et utilise des exceptions explicites sous normal/`-O`.
[results.normal.json](results.normal.json) contient ratios, effectifs, bornes
interprocessus observées et empreintes ; [verification.json](verification.json)
enregistre l'égalité normal/`-O`, les codes zéro et le rejeu public seul.

Suite bornée : hacher les exécutables construits et effectivement lancés, répliquer
la résolution M3, puis mesurer le mur de l'ordonnancement des ordres avant de lui
attribuer le maximum des coûts isolés. Aucun contrat FULL, multi-séquences D7 ou
temps produit ne découle de ce contre-audit.
