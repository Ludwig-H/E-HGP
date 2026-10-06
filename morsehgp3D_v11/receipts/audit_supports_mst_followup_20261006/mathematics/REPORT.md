# Apport mathématique ciblé

## Source et constats

Le fichier WIP `tests/cli/cli_supports_oracle.py` au HEAD `9eee2ed4bcef1e960cdf2456012b84416854dc20` est capturé deux fois à octets identiques, SHA256 `839ec1a4dc383c63684b278214f42b1196fc85d442b05ae88a3707a3a6b006a9`. Ce contrôle de capture n'est pas une qualification native. Les constats prolongent ceux publiés en `be8085ec1` et `eaa2ecf63`.

1. Lignes 69–72 : la sélection conserve toute boule dont le rôle diffère d'`interne`. Sur le triangle équilatéral entier de trois sites, les trois paires sont au même niveau de fusion ; ce filtre garde trois boules alors que Kruskal en garde deux. Le juge reproduit donc le défaut de sélection.
2. Ligne 190 : la comparaison porte sur `spanning(mine)` et `spanning(want)`. Même une sélection corrigée des deux côtés effacerait une boule cyclique réellement publiée par le natif. Le patch compare `mine` sans projection à `spanning(want)` ; la preuve vérifie aussi cette propriété dans l'AST proposé.
3. Ligne 226 : `max(report['decoded'].m) == 25` est une attente fausse pour `sphere9_25`, à K1 comme à K2. La preuve exacte montre que les traces strictes sont déjà toutes connexes avant le niveau 9. La boule centrale n'est donc pas une fusion ; elle est interne et absente de la sélection. Les six points axiaux imposent cette unique sphère à toute boule ayant ces 25 sites sur sa frontière : aucune autre boule à m=25 ne peut remplacer la centrale. Le patch conserve admission et S=B, et vérifie son absence ; il n'invente pas la valeur du plus grand m restant.

## Sélection proposée

`projection.py` et le patch utilisent une DSU locale sur les enfants ouverts de chaque multifusion. Les boules sont traitées dans l'ordre natif BallIdx : niveau exact, puis S* en indices denses de Morton, rembourré par une sentinelle supérieure à tout SiteIdx. S* est choisi séparément par arité minimale puis ordre lexicographique des SiteIdx. L'ordre des centres utilisé par le document S1 n'est pas l'ordre de Kruskal : un témoin de trois sites le distingue.

Une naissance est conservée. Une boule de fusion est conservée si l'union de ses branches ouvertes effectue au moins une union réussie. Une hyperarête peut effectuer plusieurs unions ; aucune égalité B=N−1 n'est imposée. Les `prior` géométriques restent inchangés. Le helper vérifie notamment rôle, niveau, arité de S*, qmin, enfants/prior, naissance unique et connexion finale des enfants. La sortie sélectionnée retrouve ensuite l'ordre du document oracle pour la comparaison.

Il s'agit du Kruskal comprimé du constructeur HGP sur ses composantes ouvertes, avec toutes les traces d'une boule connectées. Ce reçu n'étend pas ce contrat à un graphe littéral de toutes les cellules et ne réintroduit pas Q_b, que la décision utilisateur ne demande plus dans cette sortie.

## Nouvelle preuve portable

Les exécutions normales et `-O` finales font 2336 contrôles, code 0, stderr vide et stdout identique. Quatre petits documents S1 indépendants couvrent le cycle K1, la différence ordre des centres/BallIdx, une hyperarête K2 à trois branches et une hyperarête K3 à quatre branches. Sur le cycle, projeter à nouveau la sortie native invalide la rend artificiellement égale à l'attendu : la nécessité de comparer `mine` intact est démontrée.

La preuve bornée de `sphere9_25` traite 25 sites, 290 paires strictes et les 2300 triples. Elle calcule chaque MEB avec `Fraction` par candidats de paires et sphère circonscrite exacte. Les 2048 triples stricts relient toutes les 290 paires strictes à K2 ; à K1, le graphe des paires strictes relie les 25 sites. La sphère centrale a le niveau exact 9. Aucun appel à l'oracle complet de supports/fermeture sur les 25 sites n'est fait.

`patch_check.json` conserve le contrôle d'applicabilité sur la capture isolée et le contrôle de syntaxe. Les sources de référence sont obtenues par Git au pin, avec vérification d'empreintes ; le rejeu ne dépend pas du worktree développeur.

## Essais échoués et limites

Les deux premiers essais, normal et `-O`, ont échoué avec code 1 et message final exact `RuntimeError: strict pair count`. Le harnais attendait à tort 288 paires ; le compte exact est 290 (300 paires moins 10 antipodales). `replay_initial.py`, les deux sorties et `attempts.json` conservent ces échecs avant correction du harnais. Les fichiers finaux enregistrent les deux exécutions de la source corrigée ; aucune source perdue n'est reconstituée.

Aucun exécutable, lecteur de fichier, build, test natif ou G4 n'est invoqué. L'efficacité du patch appliqué au produit et son raccord CMake restent à qualifier côté développeur. Le reçu établit les défauts du juge capturé et la correction mathématique ciblée proposée, sans prétendre clôturer cette qualification.
