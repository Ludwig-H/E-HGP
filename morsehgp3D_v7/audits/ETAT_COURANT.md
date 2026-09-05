# État courant v7

Actualisé le 5 septembre 2026, sources examinées au HEAD
`e6d33698e62ebecf74dff01c16d7de17149d7a4e`.
Autorité : [synthèse indépendante](AUDIT_INDEPENDANT_20260904.md) et
[validation courante](receipts_20260905/validation_current.json).

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Porte exploratoire satisfaite : demande v7 explicite, lecture intégrale
des parties I et II du manuscrit et réserves déclarées. Les hashes des
sources font foi ; les reçus antérieurs conservent leurs octets et auteurs.

**La MEB différée et la topologie de l'index sont justifiées sous leurs
préconditions.** Leurs juges indépendants passent : 431 appels MEB et
6 176 puissances rationnelles ; 237 212 nuages d'index sur chacun des
deux binaires, avec sept mutants structurels. Les exécutions UBSan ne
produisent aucun diagnostic. Les 40 appels de la porte d'arrondi
conservent les objets et exercent effectivement les replis entiers.

Les reçus constructeur D sont contre-vérifiés : 323 noms CTest distincts,
sans échec ni skip, 140 sources et 37 binaires conformes. Les portes
arithmétiques, dont la branche Boost réellement compilée, ne restent
plus des travaux « à porter ». La nouvelle construction indépendante
Release passe ses 323 portes CPU, sans échec ni skip ; sources et
37 binaires stables. Construction 247,62 s, CTest 607,43 s sur hôte partagé.
Son reçu reste distinct de ceux du constructeur.

**Ce résultat complet porte sur D.** Le constructeur a commencé E après
la clôture : quatre fichiers produit du worktree portent le prétest q2.
Le manifeste conserve le snapshot D et son contrôleur renvoie donc 1 sur
ce delta local ; il ne faut ni actualiser ces pins mécaniquement ni
transférer les 323 passes à E. Les fichiers produit restent hors du commit
de l'auditeur.
L'[addendum q2](ADDENDUM_MEB_Q2_E_20260905.md) confirme néanmoins
la preuve locale et les mêmes 431 appels sur E, avec le nouveau mutant
détecté. Il ne remplace pas les portes intégrées E.

La composition horizontale et S1 restent conditionnelles au raccord
complet des primitives et du domaine d'exécution. Le nouvel audit
décharge l'index et raccorde les parcours WSPD/cover ; le grand-livre
arithmétique restant des témoins du front garde ses obligations. Les plateaux pertinents sont encore refusés.
L'archive annonce `vertical_maps=none` et ne fournit pas les scores de
vote. Le mode par défaut reste `verified_events_only` ; la complétion reste
`normalized_horizontal_h0_candidate`. `--require-exact` refuse.

Ni le seuil 50k, ni le domaine massif, ni la reprise moteur ne sont
qualifiés par les essais présents. Le registre officiel est inchangé.
Travail sur `main`, écritures limitées à `audits/`. GCP non utilisé ;
aucune mesure GPU attribuée à cet audit.
