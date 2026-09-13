# Raccord q2 : profondeur, coquille et identité de boule

13 septembre 2026, lecture après `8e406f9b`. Cadre :
`exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Fixtures mathématiques pour le prochain consommateur ;
aucun consommateur produit n'est qualifié par cette note.

Le contrat proposé — index de **tous** les sites, compte depuis zéro,
arrêt dès Kmax intérieurs — décide correctement la fenêtre q2 :
p+2≤Kmax+1 équivaut à p<Kmax. Le cœur et les crédits locaux sont déjà
présents dans ce nuage ; leurs comptes ne s'ajoutent pas à ce census.
Cette vérification est distincte du préfiltrage par rectangles décrit dans
[P0_PARTAGE_ET_FILTRE_AXIAL](../../morsehgp3D_v8/docs/P0_PARTAGE_ET_FILTRE_AXIAL.md).

## Une même boule, deux paires et quatre sites sur la frontière

Prendre les six sites u16 dans cet ordre : a=(0,1,1), b=(3,2,2),
c=(1,0,1), d=(2,3,2), z=(1,1,1), e=(1,1,4). Les paires ab et cd
ont exactement la même boule : centre doublé (3,3,3), rayon carré 11/4.
Le tableau des H vaut pour chacune [0,0,0,0,2,−4] : intérieur {z},
coquille {a,b,c,d}. Les rectangles à facteurs singleton sont séparés
pour s8/10/12 ; la fixture peut donc atteindre le consommateur actuel.

À Kmax=2, ces deux supports sont dans la fenêtre car p=1. Compter la
coquille dans p, utiliser son cardinal 4 à la place de q_min=2, ou
précharger le crédit de z puis le visiter de nouveau les rejetterait
à tort. Arrondir le centre en (1,1,1) transforme notamment c en intérieur.
La formule exacte évite toute division :

$$4H(a,b,x)=\Vert a-b\Vert^2-\Vert 2x-(a+b)\Vert^2.$$

Les deux paires restent deux identités de support ; elles ne représentent
qu'une seule géométrie. Pour q2, (a+b, |a−b|²) est une clé de boule exacte.
Un compteur de paires résiduelles ou acceptées n'est donc pas un compteur
de boules distinctes. Dédupliquer les boules ne dispense pas de déclarer
si les incidences de leurs supports sont conservées ou reconstruites.

## Rejet spatial : les coins extérieurs ne suffisent pas

Prendre a=(0,2,2), b=(4,2,2) et un nœud d'index contenant les trois sites
(2,0,0), (2,4,4), (2,2,2). Les huit coins de sa boîte ont tous H=−4,
alors que le troisième site a H=4. Un maximum calculé seulement aux
coins perdrait cet intérieur. H est concave en x : son minimum est
atteint aux coins, mais son maximum peut être intérieur à la boîte.

La distance minimale de la boîte au centre doublé trouve correctement
4H_max=16 ; sa distance maximale donne 4H_min=−16. Le raccord envisagé
utilisant ces deux distances satisfait donc cette obligation. Le
[juge](q2_census_fixtures.py) conserve la contre-fixture exacte.

## Un compteur strict ne livre pas encore la coquille

Un nœud réduit à c dans la première fixture vérifie H_min=H_max=0.
L'écarter lorsque H_max≤0 est correct pour compter les intérieurs.
Ce même rejet perd c si la sortie promet aussi les IDs de coquille :
il faudrait alors un chemin de collecte adapté aux égalités, ou un
second parcours déclaré. Le besoin dépend de la sortie réellement promise.

La fenêtre utilise q_min=2 même avec quatre sites sur la coquille ; elle
ne promet ni facette Gabriel stricte, ni naissance, ni fusion FULL.
Les autres sites de frontière interdisent ici de déclarer les paires
Gabriel strictes. Retirer z rend même leur boule ouverte vide, et les
fait passer à Kmax=1, sans retirer c et d de la coquille d'ab : ce
résultat n'est toujours pas une paire Gabriel stricte au sens FULL.
Les obligations de plateaux et de rattachement restent
celles des [fondements](../../morsehgp3D_v8/audits/FONDEMENTS_ET_OBJET.md).

Le [reçu](Q2_CENSUS_FIXTURE_CHECKS.json) exerce 48 symétries u16,
192 requêtes, 1 152 identités de puissance et 576 décisions de seuil.
Normal et −O passent. Quatre contre-modèles sont exécutés et rejetés :
centre tronqué, nombre de paires présenté comme nombre de boules,
rejet par les seuls coins, et prétention à collecter la coquille après
élagage adapté aux seuls intérieurs. Ce sont des erreurs de modèles,
pas des défauts allégués du consommateur en préparation.
Aucun index ni benchmark supplémentaire ; GCP non utilisé.
