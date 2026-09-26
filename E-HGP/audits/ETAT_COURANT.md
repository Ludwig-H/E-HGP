# État courant de E-HGP, vu par l'audit

> **Verdict mutable unique**, ancré au commit audité. Ce fichier est réécrit à
> chaque audit ; les rapports datés, eux, sont immuables.
>
> Ancré à : `f44a8db0372189dba2ede3a55bbce669aa74baab` (26 septembre 2026).
> Cadre : `phase=exploration_ehgp_hors_registre`, `public_status=not_claimed`.
> GCP non utilisé.

## Verdict

**Le calcul tient, la comptabilité et les portes ne tiennent pas.** Sept constats
bloquants, dix-huit majeurs. Aucun ne remet en cause les deux résultats de fond
du chantier — l'obstruction de taille et la valeur de la régularisation du noyau —
qui ont tous deux résisté à des attaques indépendantes. Ce qui est cassé est la
garde : plusieurs phrases publiées revendiquent davantage que ce que les portes
garantissent.

Rapports : [`AUDIT_OUVERTURE_20260926.md`](AUDIT_OUVERTURE_20260926.md),
[`FIXTURES_AUDIT_20260926.md`](FIXTURES_AUDIT_20260926.md),
[`AUDIT_VERIFICATIONS_DIRECTES_20260926.md`](AUDIT_VERIFICATIONS_DIRECTES_20260926.md).

## Ce sur quoi on peut bâtir

| acquis | statut après audit |
| --- | --- |
| boule englobante minimale exacte, dimension quelconque | **tient** (5 913 sous-ensembles, certificat indépendant, 0 désaccord ; refuse au lieu de mentir quand sous-provisionnée) |
| naissances topologiques et obstruction de taille | **tient** (deux réécritures indépendantes, 48 valeurs sur 48 pour l'une, 2 732 naissances pour l'autre, 0 écart) |
| tour d'ordre 1 égale la liaison simple | **tient** (juge extérieur scipy, 28 accords, 0 désaccord, $d$ de 1 à 60) |
| encadrement du noyau rampe, décalage optimal | **tient** (6 480 cas rationnels jusqu'en $d=200$) |
| fait de segment | **tient** |
| majorant à témoins égal à la projection exacte | **tient et renforcé** (27 024 paires, 0 violation) |
| sûreté du certificat de séparation | **tient** (0 faux certificat par tous les chemins d'attaque) |
| 142 portes sous `-O`, zéro `assert`, cinq codes de sortie | **tient** |

## Ce qui est refusé en l'état

| refus | motif |
| --- | --- |
| « zéro faux positif » du catalogue critique | **B1, B5, M18** : faux positif sur la fixture gravée du chantier ; drapeau de point fixe jamais lu ; aucun exécutable ne mesure la propriété |
| « la condition de point fixe est exactement celle de la spécification » | **B2** : `conv` n'est pas `relint conv` |
| « la porte confronte chaque certificat ; une violation est un échec » | **B3** : un mutant non sûr survit, 41 668 certificats faux non vus |
| « toutes les paires certifiées à l'ordre 1 en $d=20$ », statut « démontré » | **B4** : 9/21 sur une graine ; un compte est mesuré, jamais démontré |
| « reçus immuables, ancrés au commit » | **B6** : immuables oui, ancrés non ; le seul pin nomme un commit sans `E-HGP/` |
| « chaque porte échoue si elle n'a pas effectivement comparé » | **B7** : quatre vacuités prouvées par exécution |
| « le port flottant est un minorant certifié » | **M8** : viole la borne au-delà de l'échelle $10^7$ et son auto-certification ment |
| forme close spectrale « résolue et sans hypothèse » | **M5** : réfutée en général ; l'algèbre tient, l'identification au vrai log-rapport demande une hypothèse non énoncée |

## Prochaine porte d'entrée pour le développeur

Dans cet ordre, parce que chaque item conditionne le suivant :

1. la criticité de la descente (**B1, B2, B5, M18**) — c'est la brique dont tout
   le reste du moteur dépend ;
2. la porte du certificat de séparation (**B3**), avec `F-AUD-4` gravée ;
3. les quantificateurs et les mots de statut (**B4, M9, M10**) ;
4. l'ancrage des reçus (**B6**) ;
5. les planchers et les codes de sortie (**B7, M13, M15**) ;
6. les hypothèses manquantes de la théorie (**M4, M5, M6, M16**) et le seuil
   d'échelle du port flottant (**M8**).

Aucune de ces corrections ne demande de revenir sur l'objet mathématique.
