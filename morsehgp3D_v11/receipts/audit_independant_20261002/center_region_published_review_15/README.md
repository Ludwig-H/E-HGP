# CentreRegion publié — deltas de la revue15

Lecture du commit `7f1922c7743d8682e2665a491b01d32e8f2d546c`, capturé par
Git avant lecture dans un scratch, puis transféré après autorisation de
publication. Six fichiers nouveaux sont conservés ici ; dix fichiers exacts
sont liés par SHA à la [capsule14 close](../center_region_contract_review_14/README.md).
[FILE_BINDINGS.json](FILE_BINDINGS.json) vérifie chaque liaison, sans recopier
les sources déjà conservées. Le présent reçu dépend donc de14 pour ces sources.
`SOURCE_BEFORE.json` et `SOURCE_AFTER.json` distinguent le pin publié du LIVE.
Aucun reçu clos, produit ou note active n'a été changé.

**Aucun défaut mathématique nouveau identifié.** Les fichiers numériques
`center_region.cpp/.hpp` et le DFS `leaf.cpp` sont exactement ceux relus en14.
Les preuves SAT, budgets d'entiers et nécessité géométrique des rejets restent
valables. Les contacts de fermeture, notamment hi=M, survivent ; l'appartenance
à la boîte propriétaire demi-ouverte reste séparée. `degenerate` n'est pas un
verdict générique `disjoint`, mais son rejet dans ce DFS est sûr pour les
supports q3/q4 affinement indépendants ; q2 reste exploré. L'aiguïté n'est
toujours exigée que pour l'émission q3, jamais pour ses prolongements q4.

Le test fixe ajouté dans7f,
`a=(4,7,2),b=(2,2,7),c=(6,2,2),Q=[0,2]^3`, sépare effectivement par la
seule normale k=2 : les couples `(left,right)` sont `(54,60),(55,90),(95,70)`.
Une vérification indépendante suffit même par le second plan :
`−2x+5y=25/2` impose `y=5/2+2x/5>2` dans Q. L'omission de k=2 accepte
donc à tort cette droite dans le modèle exact. Cela ne prétend pas exécuter
ou tuer le mutant natif. Six mutants numériques CentreRegion sont déclarés,
avec motifs uniques dans le code figé ; les contacts, axe, signe et domaine
sont visés. Les deux nouveaux mutants catalogue concernent le rejet de paire
avant Sphere et l'interdiction d'une face obtuse. Le premier contrôle le
travail effectué ; le second peut supprimer une boule q4 positive et relève
du juge géométrique. Leurs résultats natifs restent à qualifier sur G4.

Une précision de couverture B21 reste utile : la fixture native à axes simples
`0,(m,0,0),(0,m,0),Q=[0,1]^3` atteint `m³−m²<2^63` en21. Une reconstruction
locale autonome de la génération des seules entrées aléatoires annoncées,
sans import du produit ni lancement de son juge, ne trouve pas non plus de
produit ou somme SAT exécuté hors i64 en21 : maximum
`7127609308375271968<INT64MAX`. En24, cette reconstruction trouve57 normales
avec un intermédiaire hors i64. Ce contrôle local de génération ne remplace
pas le payload et les résultats du futur reçu G4.

Le témoin explicite proposé en14,
`0,(m,m,0),(m,0,m),Q=[0,1]^3`, atteint `2m³−2m²>INT64MAX` en21 et24.
L'ajouter comme garde ciblée ferait exercer le dépassement dès21. Le code
publié élargit déjà correctement avant multiplication : ce manque de garde
explicite n'est pas un défaut de son arithmétique. Un intermédiaire hors i64
ne démontre pas à lui seul la mort géométrique d'un mutant ; il peut servir
à une porte de budget ou sanitizer.

[check.py](check.py) vérifie uniquement ces deltas et l'exposition arithmétique
des entrées annoncées. Il ne rejoue pas le panneau864/432 de14 et n'importe
aucun oracle produit. Les modes normal et `-O` passent avec sorties identiques.
Le `center_region_model_test.py` publié importe son oracle paramétrique et
des helpers Fraction numériques : son selftest ne constitue pas une troisième
voie indépendante. Le plan G4 reste une intention d'exécution ; aucune
qualification CentreRegion, catalogue, FullDomain ou FULL n'est acquise ici,
et aucune qualification MEB antérieure ne leur est transférée.

Relecture : `python3 check.py`, `python3 -O check.py`, puis
`sha256sum -c SHA256SUMS`. Le manifeste inclut tous les fichiers du reçu,
sauf lui-même à la racine ; les dépendances de14 sont vérifiées par le lecteur.
