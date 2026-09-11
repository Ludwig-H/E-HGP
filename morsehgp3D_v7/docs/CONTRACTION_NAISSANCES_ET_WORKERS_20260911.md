# Contraction directe des pivots et workers géométriques

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Ce jalon reste privé, sans remplacement du moteur actif ni nouveau claim GPU.

## Ce qui est retiré, ce qui doit rester

Le [raccord ordonné précédent](../receipts/ordered_streaming_20260911/README.md)
évitait les retris des arêtes entre hubs, mais construisait encore leur
forêt, sa projection, puis une compaction native. La variante dense retire
ces trois objets intermédiaires. Elle conserve les mêmes boules, masques
compacts, contributions datées et forêts FULL. Les marques de blocs ne
disparaissent pas avec les hubs : elles sont nécessaires aux consultations
historiques et aux verticales.

Pour chaque ordre K, préallouer le domaine des L naissances, même futures
ou définitivement isolées. Le label φ d'un bloc désigne une **identité
dense de naissance**, jamais une racine courante de l'union-find.
Traiter les occurrences dans leur ordre original :

- La première occurrence d'un bloc non natif copie φ de sa terminale.
  Cette contraction de pivot ne soumet aucune arête à l'union-find.
- Chaque autre occurrence soumet exactement une union entre deux labels
  natifs, y compris si ces labels sont égaux. Une union acceptée conserve
  les identifiants des naissances, la date émettrice et l'ordinal original.
- Après **tout K**, convertir les labels denses en BlockId lors de
  l'émission des marques. Un bloc suivant ne doit jamais lire un mélange
  de labels denses et d'identifiants publics.

L'invariant de préfixe est la connexité des hubs modulo les fibres φ déjà
attachées. Chaque pivot attache un hub jusque-là isolé ; il ne fusionne pas
deux composantes natives. Les autres occurrences prennent donc les mêmes
décisions que Kruskal dans l'ordre source. Les dates des deux naissances
restent strictement antérieures à la date de l'arête émise. L'auditeur
fournit une [preuve conditionnelle et un modèle indépendant](../audits/receipts_birth_stream_20260911/README.md).

Pour R occurrences dont P pivots, le travail de réduction devient une
initialisation sur L sommets, P affectations et R−P tentatives d'union.
Cela ne borne ni le census, ni R, ni la taille de sortie en fonction du
seul nombre de points. En particulier, ce n'est pas une preuve universelle
de complexité sous-quadratique pour une sortie FULL explicitée.

## Qualification dense fermée

Le [paquet mono et pool](../receipts/birth_streaming_20260911/README.md)
sépare la nouvelle source compilée du brouillon non compilé contenu dans
le paquet parent. O2 et ASan/UBSan/LSan passent chacun :

- 114 census, 456 essais de fenêtres, 253 224 terminales directement
  comparées ; fenêtres 1/7/31/4096, réindexages, s8/10/12 et plateaux ;
- 30 619 873 contrôles, φ et marques, mêmes arêtes natives datées,
  forêts, contributions et verticales ; les grands différentiels n32
  restent distincts des petits cas passant l'oracle indépendant ;
- 16 refus/mutants causaux. Trois modifient réellement le producteur :
  remplacement de φ par une racine courante, oubli d'un pivot entre
  fenêtres, conversion trop précoce en BlockId ;
- 744 occurrences à labels natifs égaux et 50 046 frontières de fenêtre
  internes à un hub, cumulées sur les essais : ces chemins sont exercés.

La mutation de conversion précoce est détectée lors d'une lecture dans
le même hub ; ne pas lui attribuer une fixture spécifique de réemploi par
un hub ultérieur. La géométrie payée est comparée à la voie ordonnée, pas
déduite seulement de la forêt finale. La validation commune du catalogue
est toujours chronométrée et exécutée ; son partage n'est pas encore intégré.

## Parallélisme effectif des groupes géométriques

L'unité distribuée est une **facette complète distincte dans une fenêtre**,
pas un support MEB seul. Le premier ordinal du groupe fournit le seuil
minimal sous l'ordre exact déjà établi. Les semis complets répondent sans
worker ; chaque groupe non semé est confié à un worker avec scratch et
compteurs privés. Géométrie, index et semis sont lus sans mutation.

```text
fenêtre : clés complètes → groupes distincts → semis ou tâches indépendantes
                                                        ↓
                          barrière : tous les résultats sont disponibles
                                                        ↓
                 dispersion contrôlée → consommation dans l'ordre source
                                                        ↓
                            labels φ stables → union-find sur naissances
```

Une équipe CPU est créée pour tout l'appel, pas par fenêtre ni par K.
Avec un worker, le calcul reste sur le thread appelant, sans thread ajouté.
Avec quatre workers, quatre threads sont créés une fois. La géométrie est
répartie dynamiquement par petits segments ; la réduction ordonnée reste
séquentielle. Les horizontales K et la reconstruction des multifusions ne
sont pas encore parallélisées dans cette variante.

Le raccord ne conserve qu'une fenêtre en cours. Cinq vecteurs nommés
portent requêtes, permutation, terminales, débuts de groupes et indices
des tâches non semées. Le tableau de débuts inclut sa sentinelle.
Leurs capacités ne sont ni le RSS ni la somme de toute la mémoire :
scratch par worker, catalogue, masques, sortie et runtime des threads
restent séparés. Les statistiques de sortie comptent aussi leur nouveau
vecteur de tâches par worker.

La gate autonome du pool passe O2 et ASan/UBSan/LSan : 2 500 contrôles,
10 refus, trois créations partielles, 23 lots positifs, huit lots vides,
exceptions et cinq réutilisations après refus. Le refus concurrent/récursif
a lieu avant admission. Une exception de tâche attend la quiescence de
tous les workers avant propagation ; les écritures déjà faites ne sont
pas annulées. Aucune sortie Streaming partielle n'est publiée.

ThreadSanitizer refuse cet environnement avant le test avec
`unexpected memory mapping`, code 66. Ce reçu échoué est conservé ;
les autres sanitizers ne le remplacent pas comme détecteur de races.

La gate du raccord exerce aussi un refus après vraie géométrie K≥2 :
quatre workers ont chacun achevé au moins une résolution avant l'exception
injectée. Les threads sont joints avant le catch. Un nouveau producteur
est confronté aux labels/certificats/travail de la voie ordonnée ; ce n'est
ni une reprise de l'objet échoué, ni un nouvel export FULL post-échec.
Le nombre total de tâches payées avant ce refus n'est pas récupéré.

## Suite et limites des mesures

Les microcomparaisons mono n200/400/800 donnent les mêmes tours physiques.
À n800, s8/10/12 conservent le même digest et les mêmes comptes géométriques.
Le chronométrage est exploratoire sur hôte partagé ; ces microtemps ne
choisissent pas un s optimal et ne qualifient pas une accélération stable.
Les mesures de taille supérieure doivent distinguer réduction dense seule,
workers géométriques et threads amont, avec la même largeur de fenêtre.

Le premier grand run dense mono est clos à n8000/s8/K1..10/W65536 :
186,354254893 s pour la tour entière, 58,532551438 s pour atlas/géométrie/
réduction ; RSS 2 752 852 KiB. Les 10 456 312 occurrences donnent
3 113 381 pivots sans union et 7 342 931 tentatives natives. Même digest,
4 359 540 MEB et 3 976 472 nœuds que le témoin ordonné parent, qui
mesurait 207,866762405 s. La variation est favorable, pas une statistique
de speedup ; le début de génération chevauche la fin de la gate SAN.
Le détail et les sources de ce run sont dans le paquet mono.

Les 5 518 027 sommets de l'ancienne voie désignent le domaine des DSU
**sur hubs**, cumulé sur K ; la nouvelle réduction a 2 404 646 naissances.
Le compact natif ancien touchait en plus ces naissances et est supprimé
séparément. Ne pas prendre le seul domaine hubs pour le total des DSU
anciennement exécutés, ni une somme sur K pour un pic mémoire.

Le [paquet parallèle](../receipts/parallel_birth_streaming_20260911/README.md)
ferme la paire 1/1/1 → 1/4/1 à n8k : 187,214 → 164,703 s pour la
tour, 58,649 → 34,976 s pour atlas/géométrie/réduction, même travail.
Avec 4/4/4 (amont/géométrie/consultations), le triplet devient :

| n | Tour complète (s) | MEB | Nœuds FULL | RSS (KiB) |
| ---: | ---: | ---: | ---: | ---: |
| 8 000 | 92,963 | 4 359 540 | 3 976 472 | 2 874 428 |
| 16 000 | 215,381 | 9 364 101 | 8 310 399 | 5 803 964 |
| 32 000 | 489,601 | 19 784 213 | 17 166 975 | 11 608 968 |

Les occurrences croissent par facteurs 2,099 puis 2,071 et les MEB par
2,148 puis 2,113. Le temps augmente davantage et la mémoire reste élevée ; le
triplet uniforme ne prouve aucune borne tous régimes. À 32k, validation
commune 44,207 s, atlas/géométrie/réduction 171,384 s, histoires 50,145 s,
export 91,704 s : la parallélisation des MEB seules ne peut pas suffire.

Le [partage de préparation proposé par l'auditeur](../audits/receipts_prepared_catalogue_20260911/README.md)
reste le delta suivant pour retirer les tris et métadonnées reconstruits.
Il ne permet pas de supprimer la validation géométrique ou de considérer
une vue const comme preuve d'absence d'alias mutable. Le GPU doit lier ses
semis une fois par propriétaire/K, et non comparer tout S à chaque fenêtre.
Le contexte GPU mutable actuel ne peut pas être partagé entre workers.

Aucun nouveau temps 50k, contrat 1 s/100 ms ou nuage de dizaines de millions
sur G4 n'est acquis ici. GCP non utilisé pour ce jalon.
