# État courant des audits v9

22 septembre 2026. Ouverture v9 `3595725a` contre-auditée sur les documents
et le moteur v8 `a74e90f2` ; compléments jusqu'à `27c26eb6` relus sur
l'objet mathématique, les pins et les reçus. Aucun code moteur v9 n'existe
encore. Cadre : `phase=exploration_v9_hors_registre`, `backend=none`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`.

Ce dossier appartient aux **auditeurs indépendants** de la v9. Le développeur
n'y écrit pas, sauf sous la forme de fichiers `REPONSE_CLAUDE_*`,
`NOTE_CLAUDE_*` ou `QUESTION_CLAUDE_*` datés. Chaque auditeur tient son propre
fichier de dialogue et consigne ses constats dans des fichiers datés
`_YYYYMMDD`, ancrés au hash court du code jugé, avec leurs sources et captures
**dans le dépôt** (jamais dans une archive jointe à une conversation). Ce
fichier `ETAT_COURANT.md` porte le verdict mutable unique, ancré au `HEAD`
audité. Le canal commun est
[`audits/COORDINATION_MORSEHGP3D_V9.md`](../../audits/COORDINATION_MORSEHGP3D_V9.md).

## Réponses indépendantes à l'ouverture

L'auditeur A a rendu un [contre-audit mathématique et moteur](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md)
et un [contre-audit des mesures et du plan](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md)
à la [question du développeur](QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md).
Le raccord q3 ← feuille **exacte** de l'atlas q4 est mathématiquement sûr
avec le même nuage, arête, cover et centre dans la cellule ; un nœud profond
à K−2 ne fournit aucun fragment q3. La borne u18 du port FULL doit changer
avant `BallKey::power` (`A<2^76`, `|B|<2^96`, `|C|<2^116` suffisent pour
les clés q2/q3/q4 produites par la v8, le majorant q3 dominant q4) ; le plafond v7 de coquille à 12
demeure un refus de domaine, pas une tour
générale. La note u18 publiée contient huit énoncés numériques faux ; les
42 inégalités simples `c·M^d<2^b` relevées dans les commentaires du code
publié sont vraies. Ces contrelectures statiques ne remplacent ni CTest,
ni sanitizer, ni TSan du futur port.

Les chiffres principaux de la synthèse d'ouverture concordent avec les
reçus, mais les facteurs ×6 à ×101 sont des **scénarios de débit**, et
`0,49 CPU·s G4/local` compare W48 à W4. La première preuve FULL doit
arriver tôt ; un reçu entier sans sol à 1 mm, ou un échec explicite borné,
peut ouvrir les expériences de réduction sans attendre six longues lignes
W1. Ces six lignes restent nécessaires avant tout claim de performance.
La cascade LiDAR publiée conserve les mêmes arêtes et ses extensions
chiffrées ne sont pas entièrement rejouables depuis Git ; les 221 rejets
collectifs q4 sont échantillonnés et ne valent pas gain du pipeline combiné.

La [synthèse constructive A](AUDIT_A_ARCHITECTURE_K_GABRIEL_20260922.md)
et ses trois notes [q3](Q3_STRUCTURE_ET_BORNES.md),
[q4](Q4_STRUCTURE_ET_BORNES.md),
[coûts et parallélisme](CONTRAT_COUTS_ET_PARALLELISATION.md) proposent une
route de **miniballes k-Gabriel locales**, sans mosaïque globale de Voronoï
ou Delaunay d'ordre K. Deux certificats exacts sont établis : compte et
frontière par signe de `Δ_a,z(c)` sur cellule fermée, et borne de rayon par
`K−2` gardes distinctes pour q4. Ils ne prouvent encore ni une génération
complète sans repli v8, ni une croissance sous-quadratique : il faut couvrir
tous les produits, centres et frontières, puis payer le front, les covers,
les clés, les intérieurs et FULL. Le catalogue par boule canonique peut
éviter le flux exhaustif de toutes les présentations, sous une preuve de
complétude et un calcul exact de `q_min`.

Le jalon temporel v9 documenté vise d'abord les trames **sans sol en u18/1 mm** ;
le float32 original demeure le défaut d'entrée fixé en v8, mais son
développement temporel v9 est suspendu. Le contrat principal antérieur sur
trame brute entière n'est pas effacé par ce jalon ; sa portée temporelle v9
reste une question ouverte du développeur. Les captations superposées seront
un diagnostic de robustesse distinct, sans hypothèse de recalage.
Aucune qualification FULL, GPU/G4 ou sous-quadratique v9 n'est acquise.
GCP non utilisé par ces contre-audits.

Le complément A sur [q4](Q4_STRUCTURE_ET_BORNES.md) distingue désormais le
rejet graine × cellule **après** atlas, déjà étudié en v8, d'un filtre avant
partition Z à instruire : il vise les milliards de tests et copies payés
pour construire l'atlas. La cellule reste fermée et tous les blocs de
graines possibles doivent être couverts. Une fixture entière à quatre
points vérifie qu'une complétion q4 peut être obtuse et l'autre aiguë alors
que q2 rejette l'arête ; le port ne doit conditionner q4 ni à q2 ni aux
deux complétions aiguës. Une [fixture q2 u18](CONTRAT_COUTS_ET_PARALLELISATION.md)
de 80 sites produit 1600 feuilles Gabriel strictes : la sortie explicite
reste une charge à mesurer même dans le domaine entier fixé. Ces deux
fixtures sont des preuves et contrôles locaux, pas des résultats LiDAR.

Le développeur a répondu par écrit aux constats A dans le [canal
commun](../../audits/COORDINATION_MORSEHGP3D_V9.md) au commit `c399808e` :
les ports critiques, le statut conditionnel de FULL, les deux fronts et
les mesures de coût total entrent au plan. Le nouveau reçu de cascade
rectangulaire reste un **filtre échantillonné**, sans durée de tour ; sa
classe de rectangles de plus de 65 536 paires est exclue du prélèvement.
Il n'autorise ni un choix de défaut ni une estimation de gain de l'appel
complet. La contrelecture des Déf. 20–22 du manuscrit et de la spécification
ne trouve pas de divergence avec l'objet FULL visé ; les applications
verticales demeurent une obligation indépendante du port horizontal v7.

Depuis `3f0d188f`, la reprise u18 et sa ligne sans sol 1 mm sont
**versionnées**, mais cette ligne est toujours un seul flux q3/q4 K5/W8,
sans gain mesuré de `saturate_deep`. Les deux captures R2 de qualification
restent `failed` : le lecteur confond les exclusions JUnit
`status="disabled"` avec des enfants `<skipped>` absents du vrai CTest.
Le [suivi A](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md) donne le correctif
et demande un pin distinct `3f0d188f` dans l'héritage v9 pour les gardes
de domaine publiées. Le reçu montre aussi 60,5 % d'arêtes q4 sans feuille
vivante **après** atlas ; la [note q4](Q4_STRUCTURE_ET_BORNES.md) sépare
ce diagnostic du gain encore inconnu d'un filtre pré-atlas, en tenant
compte de la réutilisation q3.

La [contrelecture B des grandes coquilles](PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
propose un quotient local exact par chambres en `O(u²)` régions au lieu de
la table `2^u` de la v7 ; elle ne borne pas le catalogue global. Le
[complément numérique A](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md) factorise
les normales des plans du test `q_min=3` : i128 suffit pour les normales
u18, tandis qu'un déterminant direct de coplanarité demande i192 sous les
majorants publiés. Ce calcul ne prétend pas borner l'arrangement complet.
L'[oracle entier local A](check_qmin_planes_u18_20260922.py) passe quatre
fixtures et 755 sous-coquilles exactes sans antipodes, y compris un centre
demi-entier ; le prochain port doit encore comparer au `ShellTable` gelé
et qualifier l'arrangement complet.

## Objets d'audit à suivre

Demande du développeur sortant, en deux lots :
[QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md](QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md).

1. L'audit d'ouverture lui-même : [synthèse](../docs/AUDIT_V8_SYNTHESE.md),
   [plan](../docs/PLAN_V9.md), [héritage](../docs/HERITAGE_V7_V8.md). Écrit
   par le développeur sortant, il a une première contrelecture A ; les
   portes et décisions de contrat encore ouvertes demeurent à résoudre.
2. Les six changements moteur v8 des 21–22 septembre ont une première
   lecture **statique** indépendante, pas un rejeu complet : `748ec082`
   (atlas en i64 à Q = 2^20), `02987f18`
   (fragments d'atlas), `0948d2d0` (rejet des graines q3 par l'atlas),
   `5224ff4e` (file de plages), `5fdda963` (chronos par worker), `a74e90f2`
   (moteur 18 bits, bornes réécrites avec M = 262 143). La tranche
   `3f0d188f` est publiée mais ses reçus R2 de qualification restent en
   échec ; ses corrections exigent un pin distinct et une qualification v9.
   Les fixtures ciblées, mutants, sanitizers et TSan restent à porter ou à
   exécuter avant héritage.
3. Chaque port v9 : épinglage, requalification, fixtures d'égalité, mutants.
4. La première tour v9 de bout en bout : objet (contre le juge T2 et les
   fixtures E5, A–E, quatre points), sorties, chronomètre.

Verdict public : `not_claimed`. Aucun contrat acquis.
