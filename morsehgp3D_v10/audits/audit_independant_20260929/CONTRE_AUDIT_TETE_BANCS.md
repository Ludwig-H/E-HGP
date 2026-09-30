# Contre-audit courant : correctifs de la tête et du banc

Mise à jour du 30 septembre 2026, checkout `e9eab2754`. `public_status=not_claimed`. Référence :
[TETE_BANCS_PREUVES.md](TETE_BANCS_PREUVES.md) et
[réponse de raccord du développeur](../REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md), §1–3.

**État : second tour en chantier, H3 et domaine des métriques encore ouverts dans les copies disponibles ;
qualification commune en attente.** Les extractions `/tmp/mhgp10-r2/tete/src/morsehgp3D_v10` et
`/tmp/mhgp10-r2/bancs/src/morsehgp3D_v10` contiennent encore les unités du premier tour. Leurs
[empreintes de lecture](../../receipts/audit_independant_20260930/head_bench/sources_read_20260930.json)
sont stables avant/après ; les sources intégrées restent antérieures. Aucun snapshot commun recevant tous les
correctifs et qualifié n'a été identifié à cette lecture. La garde H3 conjointe et le domaine des métriques sont
annoncés, mais pas encore présents dans ces unités. Les défauts déjà capturés ne sont pas rejoués sur du code identique.

| Fichier de la copie | SHA-256 |
| --- | --- |
| tête corrigée : `src/points/dendrogram.cpp` | `2eadd747ff6804003e971bd2cb0d492beb7ee5fbd697d3975fb9822f966ba71f` |
| tête corrigée : `src/head/head.cpp` | `dc34aed57e9248a58858515f731654540d38d9ed919877d81f7ca68476bc6dd5` |
| tête corrigée : `src/head/head.hpp` | `8d9e29b1992d8d27cb9c542dadca4bd08631b8de128a8bd62a6fb46f8cad474b` |
| banc corrigé : `bench/synthetic/decide.py` | `bab58df89c398479cb2a59791a7d23370937cfc51f5064037a76a160d5da7dd3` |
| banc corrigé : `bench/g4/merge_sessions.py` | `2526cc3d2f4dee8347e052698dbfeccba050aee30e92934ff7dd5a7ec7ae7ca5` |
| banc corrigé : `bench/scaling/scale_run.py` | `f8d5e891a06b0549dd1de19aaad76e32cbe32e19f69b8dab3bd34cfd4ff5f6e2` |

La lecture du premier tour montre les contrôles de bornes et la réciprocité parent/CSR pour H1, le seuil
d'appartenance de la racine pour H2 et les propagations parent→enfant pour H4. Ces changements ciblent les causes.
Le validateur de paramètres et le lecteur de configurations traitent aussi les anciens z invalides. Le second tour
doit ajouter la garde numérique conjointe et une porte de coût causale. Aucun build du développeur, capture active
ou VM n'est modifié ; aucune graine de test, campagne ou nouveau build n'est exécuté dans cette reprise.

Le [contre-audit des bancs](../audit_continu_20260929/timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md) a exécuté
18 commandes courtes, normal et −O : arrêt/récolte et rejet de lots incomplets réussissent sur la copie du premier
tour. Il prouve aussi qu'un ARI de 1,25 est admis et publié. Ces résultats sont relus, pas présentés comme de nouvelles
exécutions de cet audit. L'absence de contrôle des bornes des scores est encore visible dans `finite_scores()`.

## H3 — résultat non fini malgré les deux préconditions satisfaites

**P2, reproduit sur la copie corrigée.** `head.hpp:14–15` publie comme préconditions `validate(d).ok()` et
`validate(p).ok()`. Le domaine z est désormais `0 < z <= 16` ; sa justification à `head.hpp:28–30` suppose les
niveaux dans `[1/4, 2^64]`. Toutefois, `dendrogram.cpp:17–21` admet tout niveau double fini positif ou nul.
Cette hypothèse de la preuve de finitude n'est donc pas portée par l'interface générale de la tête.

La [sonde de quatre points](../../receipts/audit_independant_20260929/contre_tete_preintegration/positive_levels.cpp)
emploie deux feuilles de masse deux fusionnées en une racine, niveaux `{1e-300, 2e-300}`, `mcs=2`, `z=16`,
`allow_single=true`. Les deux validations rendent `ok/none`. `cluster` retourne normalement, sans statut de refus,
avec deux naissances infinies, deux stabilités NaN, une stabilité infinie et quatre λ de sortie infinies ; les quatre
labels valent zéro. Les niveaux sont strictement positifs : le résultat ne dépend pas d'une politique des dates nulles.
Compilation Release de deux unités seulement, sortie 0 ; aucune campagne ni extrapolation de performance.
Les 31 sources ont été fermées avant/après dans le
[reçu historique de cette sonde](../../receipts/audit_independant_20260929/contre_tete_preintegration/positive_levels.json).

Le producteur u18 ne fournit pas ces niveaux minuscules ; la sonde vise le contrat public `PointDendrogram`→tête,
pertinent pour une future hiérarchie de points dans un autre profil ou une autre unité. Pour fermer H3, relier
explicitement la validation à l'échelle réelle des niveaux et à z, puis refuser avant condensation si le domaine
annoncé n'assure pas le résultat fini. Une validation conjointe `(d,p)` ou une restriction documentée et contrôlée
du domaine du dendrogramme convient ; une borne sur z seule ne suffit pas. La régression devrait vérifier que cette
fixture est soit refusée explicitement, soit calculée dans un domaine fini déclaré.

Le [contre-audit numérique distinct](../audit_continu_20260929/pool_head/CONTRE_AUDIT_TETE_NUMERIQUE_20260929.md)
complète ce point : niveaux `1e-300` et `9e-300`, z=2 et poids u32 maximal donnent des λ finis mais des stabilités
infinies, puis un choix EOM contraire à l'inégalité analytique ; une fusion zéro donne NaN. La masse u64 reste sûre.
La preuve positive entière publiée dans cette note exclut le débordement pondéré sous ses conditions de niveaux,
unités et masses ; elle ne ferme pas l'API générale ou tous ses zéros. Contrôler uniquement la finitude de λ ne suffit pas.

## Critères courts pour recevoir le second tour

La réponse propose une validation conjointe `(d,p)` avant condensation, des λ positifs normaux et
`masse totale × λ maximal < 2^1000`. Les fixtures suivantes sont les critères d'entrée du contre-rejeu ; les refus
doivent être explicites avant le calcul, et le contrat API doit exiger cette validation conjointe.

| Cas manuel | Résultat attendu du domaine annoncé |
| --- | --- |
| Ancienne sonde positive `1e-300,2e-300`, z16 | Refus numérique avant condensation |
| `1e-300,9e-300`, z2, poids 1 puis u32 maximal | Premier cas fini avec deux feuilles ; second refusé par la borne pondérée |
| `1/4,3/4`, poids u32 maximal, z1 puis z2 | Acceptés et finis : racine à z1, deux feuilles à z2 |
| Fusion au niveau zéro | Refus avant création de clusters nés à λ infini |
| Deux feuilles zéro de masse 1, fusion positive, mcs2 | Acceptées si le domaine conserve ce régime K1 : les points sortent à la fusion positive |
| Racine singleton zéro, poids 1, mcs2 | Refus aussi : aucune fusion positive ne protège la lecture directe du λ zéro |

La condition « nœud né à zéro de masse ≥ mcs refusé » ne couvre pas à elle seule le dernier cas. La protection
vient du λ effectivement consommé : une petite feuille abandonnée à une fusion positive et une racine singleton
légère ont des parcours différents. Les unités courantes ne portent encore aucune garde conjointe.

Pour H4, le peigne doit passer avec les mêmes labels, puis une véritable remontée quadratique doit échouer sur
une observable externe. La porte disponible lit encore `ancestor_steps` calculé par le code sous test ; sa borne
ne suffit pas à tuer une remontée qui laisse ce compteur inchangé. Le watchdog avec marge annoncée d'au moins
un facteur 50 doit être capturé sur le binaire correct et le mutant, avec leur source fermée ; il reste à recevoir.

Pour le banc, une petite table dev complète doit être acceptée, et chaque mutation isolée refusée sans décision :
ligne manquante, doublon, scène ou méthode hors plan, métadonnées ou épingle divergentes, score non fini ou ARI 1,25
sur une ligne non refusée. Conserver le témoin `refused=1` avec NaN (score substitué par zéro) et les scores négatifs
valides. Une fusion partielle peut préparer la suite, pas produire la décision confirmatoire. Le rejeu normal/−O et
la simulation déterministe de la course de lancement doivent ensuite porter sur les empreintes finales communes.

## Ce qui doit fermer les constats

- **H1 :** refus du rang de point hors `level`, du parent hors domaine, d'une arête absente ou dupliquée, des niveaux
  NaN/infinis/négatifs, avant toute lecture hors bornes. Vérifier également la frontière publique des consommateurs,
  et pas seulement la CLI.
- **H2 :** la petite fixture K1 `(0..7,50,100)` retrouve huit points dans l'amas et deux points de bruit lorsque la
  racine est sélectionnée. Conserver aussi les cas racine exclue et plusieurs amas ; les campagnes A/C n'utilisaient
  pas l'option fautive.
- **H3 :** z NaN, infini ou non positif refusé pour les arguments simples et les fichiers de configurations ; les
  paramètres et échelles valides gardent leur sémantique. La politique de valeurs extrêmes doit être explicite.
- **H4 :** l'étiquetage et la suppression des descendants sélectionnés ne remontent plus chaque chaîne séparément ;
  le peigne reste valide et donne les mêmes labels. Le coût doit être linéaire en points et clusters publiés.
- **E1 :** une décision exige exactement le manifeste × méthodes, sans doublon ni couple supplémentaire, avec
  métadonnées et valeurs finies contrôlées. Une fusion partielle reste autorisée pour préparer la session suivante,
  mais ne permet jamais la décision confirmatoire. Vérifier aussi reprise et normal/−O.

## Lien avec la hiérarchie laminaire recherchée

La sûreté du dendrogramme et la conservation de sa masse sont des préalables au nouveau contrat. Le correctif
`allow_single` ferme une règle d'appartenance, sans établir la robustesse de la projection. La restriction `core`
et l'affectation exclusive `cover` restent deux modèles différents ; une amélioration d'ARI après EOM ou
remplissage ne prouve pas la stabilité des hauteurs de fusion. La coalescence des dates en double est acceptée
comme une politique à publier avec les rangs exacts et les compteurs, conformément à la réponse du développeur.

Le [bilan multi-K](../tete_multik_20260929/README.md) ne trouve pas de gain de 0,02 pour les trois têtes essayées ;
les gains hors échantillon vont de +0,001 à +0,005 et `mixq` reproduit +0,0046. Il ne démontre pas qu'une meilleure
projection laminaire ou de plus grands K sont impossibles. La correction des défauts de tête ne doit ni être
annoncée comme un tel gain, ni fermer cette question de modèle.

Pour les hauteurs de fusion demandées au §5 de la réponse, je recommande deux panneaux figés avant perturbation :
un tirage uniforme des paires, contrôle global, et un panneau stratifié par distance, diagnostic des voisins,
distances intermédiaires et lointaines. Publier chaque strate ; une moyenne globale du panneau stratifié demande
des poids de population déclarés. Garder les mêmes identifiants de paires et strates après perturbation, sans les
redéfinir par la vérité, la sélection EOM ou les résultats observés. Les ambiguïtés d'affectation, points différés et
erreurs de fusion se mesurent avant le score de la sélection.
