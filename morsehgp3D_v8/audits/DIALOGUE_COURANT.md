# Dialogue courant de l’auditeur indépendant A v8

20 septembre 2026, main ; écritures limitées à audits/.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## Tranches 28/29 : coût réel et composition sûre

[Audit et preuves](q4_kernel_composition_20260920/README.md), produit relu
à `31b0243a` : pas de défaut identifié dans les contrats examinés.
La sonde lie les vrais ports 28/29 : 192 appels Release/Clang ASan/UBSan
sur petits oracles, puis **126 mesures LiDAR appariées**, scans séparés
de SemanticKITTI08, sans correspondance ni fusion de points.

Les 90 premiers cas ne produisent que des rejets. Le complément de neuf
arêtes productives vérifiées indépendamment donne 36 mesures à 8k/50k,
**54 présentations identiques**, profondeur et coquille contrôlées par
census global. Toutes les 20 occurrences de cible admissible sont retrouvées ;
les 16 devenues trop profondes sont rejetées. Ce n’est pas un front complet.

**Ne pas substituer systématiquement 29 à 28 :** sur le complément productif
8k, leurs lectures de témoins sont proches ; à 50k/K10, 29 réduit 856 sites
à 643 mais fait 14 219 lectures et 100 670 comparaisons de tri, contre
5 628 et 4 725 pour 28. Les petits appels 29 peuvent être plus rapides ;
les temps uniques sur hôte partagé ne qualifient pas un gain stable.

**Composition à comparer :** noyau global R, puis atlas sur R ; dans une
feuille de compte exact c<T, réduire éventuellement ses actives au seuil T−c.
Les certificats doivent être emboîtés. L’[intersection de noyaux indépendants
est fausse](q4_kernel_composition_20260920/COMPOSITION.md), avec contre-fixture
positive réelle. Le recentrage local conserve l’orientation originale par
une identité exacte ; son déterminant traduit naïf peut dépasser i128.
Aucun raccord ni gain de cette composition n’est encore mesuré.

## Réponse à la fenêtre 30 et au futur index

[Certificat confirmé](q4_kernel_composition_20260920/WINDOW_INDEX.md) :
avec H=T−c>0, conserver la fenêtre **fermée** de la H-ième sortie décroissante
à la H-ième entrée croissante. L=U doit survivre ; le seuil compte les IDs
avec leurs multiplicités. Au plus 2H−2 IDs ont leur racine strictement entre
les bornes ; cette borne ne limite pas les coquilles aux extrémités.
508 cas rationnels et deux mutants passent en normal/−O. Le port 30 en
chantier n’est pas qualifié par cet audit des ports 28/29.

Pour l’index futur, conserver toutes les frontières cycliques, leurs poids
et plateaux, puis les partager entre seeds. Tangences, pôle et point dual
de la seed découpent les projections en chaînes monotones ; les seuls voisins
de sa couche ne suffisent pas. Formules et cas c=0 sont explicités dans la
note ; aucune complexité logarithmique acquise n’est annoncée.

La [fixture q4 isolée à huit sites](q4_kernel_composition_20260920/DEGENERACIES.md)
est déjà prise en compte par le constructeur pour son port 30. Elle exige
des intersections fermées de dimensions 0/1 et une coquille entière.
Au budget local d’intérieurs nul, une seule intersection fermée suffit ;
l’énumérateur général démontré reste potentiellement combinatoire.

## Entretien

Les conseils désormais portés dans 28/29 sont retirés du dialogue.
Les [preuves antérieures](q4_local_sweeps_20260920/README.md) restent épinglées
car réutilisées ou référencées ; fichiers constructeur et autres auditeurs
préservés. P0 global, tour FULL et contrats 50k/massif restent ouverts.
