# T2d_G : prélecture du protocole avant modification du moteur

8 octobre 2026. Aide à la construction du chantier B, **pas un défaut présumé de
son futur pilote**. À la première lecture, la copie B/base `8dc5d6b16` et B/repo
ont les mêmes corps num/index/tower et portes associées. Aucun moteur, compilateur
ou GCP exécuté par cet audit. Sources publiées épinglées à `039b2657e` dans
`capture.json`. Aucun nouveau constat ni changement d'état ; rattachement aux
suivis [CST-0018 et CST-0201](../../../audits/CONSTATS.md).

## Réemploi du pilote T2-c

Le pilote publié `microbancs/mes_t2c_g/pilote_t2c.py`, SHA `06f10189…`, juge
correctement son propre protocole historique. Son réemploi doit adapter :

- **Les deux schémas déclarés.** `schema_bras` attribue encore le format antérieur
  à Gc aux noms `avant`/`avant_bis`. Celui-ci contient sept diagnostics scalaires
  et `order_ns[K]`. Le format Gc contient treize scalaires et quatre tableaux :
  `order_ns[K]`, `pass_ns[K]`, `table_ns[K−1]`, `join_ns[K−1]`. La base T2d
  et la future variante B utilisent toutes deux Gc : ne pas choisir leur format
  d'après leur seul rôle avant/après. Le contrôle de champs fermé refuserait
  sinon une sortie Gc saine du bras avant.
- **Les bras et comparaisons.** `avant`, `avant_bis`, `sans_gl7`, `apres`, `gl5`
  et leurs substitutions décrivent T2-c ; ils ne séparent pas les leviers B.
  Repartir de la base actuelle pour chaque levier, puis déclarer le lot combiné.
- **L'effectif et la statistique.** Le minimum T2-c reste dix processus et six
  passes ; les consignes T2d demandent dix tours de dix passes. Fixer ces minima,
  ordre équilibré, appariement et intervalle avant la campagne. Les unités du
  rééchantillonnage sont les tours/processus, jamais les passes chaudes supposées
  indépendantes. Un bras manquant refuse sa comparaison ; les exclusions doivent
  porter sur les paires complètes, sans changer la cohorte après observation.

Les contrôles JSON, sorties finales, types stricts, identité des fichiers/binaries
et refus d'une preuve manquante restent utiles. Le lecteur G ne publie ordres et
digest qu'à la dernière passe : ne pas lui attribuer une preuve géométrique à
chaque passe. Les bruts FULL doivent passer une admission stricte ; le code 0 du
pilote FULL livré ne suffit pas ([contre-lecteur K](../mes_full_contrelecture/README.md)).

## Équivalence à demander selon le levier

| Levier déclaré | Ce qui reste identique | Ce qui peut changer |
| --- | --- | --- |
| Pavé resserré seul | Signes, parcours/saturation, cibles, résolution, travail logique, FUL1 et registre R | Rejets sans calcul et voies physiques `GuardLedger`/`LaneCount` |
| Promotion arithmétique prouvée, séparée | Même géométrie et même résolution | Distribution des voies et évaluations exactes |
| Index exact de supports, politique inchangée | Réponses de recherche, cibles et objets | Recherche, préparation, mémoire, compteurs physiques déclarés |
| Proposition modifiant explicitement la politique des sauts | Objet FULL canonique et registre R à vérifier | Cibles TARG/TMSK et travail de résolution ; SHA de résolution éventuellement différent |

Pour la dernière ligne, conserver l'ancienne obligation d'égalité brute du SHA G
serait trop fort : comparer FUL1 avec les portes sémantiques prévues. R n'est pas
encodé dans FUL1 ; garder sa preuve propre. Le changement de politique doit être
isolé de la garde et des accès exacts, pas masqué comme optimisation de cache.

## Garde et budgets : conditions de qualification

La [preuve NUM-GARDE et ses témoins](../garde_census/README.md) s'appliquent à
`CertifiedBall` seulement. Première tranche proposée : pavé `(m−M,m+2M)`, avec
budgets `6s+11`, sélection de voies `s+2` et choix dot64/dot128 inchangés. Toute
promotion est un autre bras : `s+1` est déjà sûr dans l'ancien pavé ; `s` devient
sûr dans le nouveau, sous les préconditions et l'ordre d'opérations prouvés.
Les deux mutations de domaine doivent donc être reclassées comme politique dans
ces contextes ; conserver un vrai témoin de repli large, sans réinterpréter leurs
anciens meurtres comme preuves nouvelles de débordement.

Pour un index supplémentaire construit côté G, compter stockage et staging avec
le catalogue encore vivant, y compris les capacités réutilisées. La construction
par trame demeure dans G et FULL ; réutiliser une capacité ne rend pas gratuite
une table dépendant du nuage. Le census seul et les parts du profil instrumenté
ne décident pas du gain : juger G non instrumenté et publier le mur FULL, avec
preuves de source/construction et maxima. Aucune amélioration de complexité ou
latence n'est déduite de la réduction du volume du pavé.

`check.py --repo DEPOT` vérifie les cinq sources épinglées et exerce uniquement
les deux schémas de diagnostics sur des dictionnaires synthétiques (aucun faux
chrono de moteur). Il rend le même résultat sous Python normal et `-O`.
