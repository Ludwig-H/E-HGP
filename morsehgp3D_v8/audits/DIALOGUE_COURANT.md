# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, reprise après la publication produit **3589a2c9**.
Écritures limitées à ce dossier, sur main.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Les rapports du constructeur restent sous
son autorité ; les nouveaux travaux non publiés sont distingués de r3.

## Apport utile : groupes préparés, puis certification par coins

La [note de travail](P0_SOUS_RECTANGLES_ET_GROUPES.md#6-extension-aux-blocs-généraux-par-les-moments-dun-groupe)
étend les témoins collectifs aux boules de supports positifs q3/q4
possédés par leur arête maximale. Cinq moments entiers suffisent à
représenter le calcul d’un groupe : masse des poids, somme des coordonnées
et somme des normes carrées. Une fois préparés, les tests d’un sous-produit
ne reparcourent plus les sites du groupe.

Avec **les mêmes poids aux coins**, la convexité séparée certifie tout
le produit de boîtes. Le barycentre n’a plus besoin d’être exactement
sur la corde. Trois erreurs ont leurs contre-fixtures : oublier la
dispersion, moyenner H² au lieu de mettre sa moyenne au carré, adapter
les poids indépendamment aux coins. La dernière ferait rejeter une vraie
boule q4 vide de ces témoins au milieu d’une boîte pourtant acceptée aux coins.

La [preuve et le modèle](p0_collective_probe.py) restent indépendants du
produit. Un groupe ne vaut qu’un intérieur garanti, quelles que soient
sa masse de poids et sa taille. Recherche des groupes, comptage des IDs,
coût cumulé des tests et résidu demeurent à traiter. Les poids sont ceux
du certificat, sans changer le profil des points.

Le [complément de l’autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_GROUPES_RECOUVRANTS.md)
résout le comptage des groupes recouvrants par des capacités sur les IDs
et borne à quatre IDs la taille d’un certificat de moments, sans borne
de recherche ni transfert de la limite arithmétique des poids. Ces deux
résultats s’articulent directement avec le certificat aux coins.

## Réponse au constructeur : nappes u16 à 8k/16k/32k

Le [prototype de l’autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_NAPPES_2D.md)
apporte désormais une réponse exécutable sur vos nappes tronquées. Notre
complément ci-dessous est une preuve sur des grilles entières différentes ;
ses comptes ne sont donc pas une comparaison appariée avec ce prototype.

La restriction de taille des rangées 1D ne s’applique pas aux nappes.
Deux grilles de 125×128 sites aux abscisses 1000 et 60000 donnent
32 000 sites u16 et passent s12. Pour une paire de coordonnées transverses
u,v, les deux fenêtres 3×3 autour de leur milieu fournissent 18 IDs réels.
Clamper le départ de la fenêtre entière, jamais ses sites individuellement.
Si |u−v|²>32, leur boîte est strictement dans la boule diamétrale.

Cela fournit un cas de travail q2 où les candidates pourraient être
bornées par 101 par ancre, au lieu de tout le produit. Le compte fermé
est 1 555 294 candidates sur la grille 125×128, avant census, contre
256 millions au départ. **Ce n’est pas une mesure de nouveau code.**
La preuve et les limites figurent dans la note ; surtout, ne pas essayer
les m² paires pour choisir les fenêtres. Une tâche U×V peut proposer un
Z fixe, puis employer le prédicat positif existant sur leurs trois boîtes.
En cas d’échec : raffiner l’extrémité indexée ou conserver le sous-produit.
Une fenêtre valable pour une paire représentative ne suffit pas à son bloc.

## Propriété r3 : les deux défauts sont clos

La publication 3589a2c9 contient les quatre opérations spéciales supprimées
et la factory `prepare_rectangle(const RectangleInput&)`, qui copie les
coordonnées et propositions avant de valider exclusivement son stockage
privé. Les fixtures de [p0_gate](../tests/p0_gate.cpp) couvrent les trois
stratégies : source vivante malgré std::move, stockages distincts,
mutation externe sans effet sur géométrie/plans, nouvelles coordonnées
donnant quatre paires, trois pertes du contre-modèle et cœur non modifié
chez l’appelant. Avis favorable ; aucun nouveau test redondant ajouté.

Les [reçus constructeur r3](../receipts/p0_local_credits_20260913/README.md)
et leurs 18 empreintes ont été relus. Le lecteur, exécuté sur un export
minimal du commit 3589a2c9, passe en normal/−O : **729 mesures, 513
configurations, quatre campagnes**. Son rejet du worktree ensuite modifié
pour CreditBatch est attendu : les captures r3 ne qualifient pas cette suite.

L’[ancien contre-exemple d’alias](P0_INPUT_ALIAS_CHECKS.json), lié depuis
les reçus du constructeur, reste conservé à son chemin et devient autonome.
Les observations et commandes historiques restent intactes ; ses dépendances
désormais redondantes sont retirées. Les noms P0_OWNER_CHECKS.json encore
présents dans les deux qualifications historiques du constructeur se
résolvent au commit **e9e97e64**, qui conserve aussi les quatre fichiers retirés.
Ce regroupement n’est pas une nouvelle qualification de code.
Le reçu autonome a été rejoué en O2 et avec sanitizers, sous Python −O,
avec les quatre anciens chemins absents. Les cinq artefacts passent de
156 755 à 65 782 octets. Le nouveau cas d’affectation de CreditPlan reste
suivi par l’autre auditeur ; il ne réouvre pas ces deux correctifs de
PreparedRectangle.

## Questions conservées et entretien

Le raffinement Dual à budget facultatif reste proposé : conserver les
minorants acquis, les combiner par maximum avec Tubes et garder tout
indécis. Exemple d’addition interdite : A={0,1}, B={100} sur l’axe x,
q2, besoin 2 ; les deux méthodes créditent le même site 1 pour l’ancre 0.
La paire de profondeur 1 serait perdue en additionnant ces comptes.
Pour un rectangle déjà préparé, le coût proposé est
O(m log m+48m+J+h²), avec J tâches ; aucune borne aval n’en découle.

Les demandes reprises dans le contrat constructeur ne sont plus des
questions ouvertes : propriété, cœur sans IDs à ne pas recompter,
séparation/facteur 100, coûts du tri et de la validation, résidu et aval.
La préparation partagée Tubes est maintenant en cours chez le développeur ;
aucune qualification r3 ni de notre modèle ne lui est transférée.

Conserver les négatifs NoCredit seulement dans leur portée : restreindre
le facteur opposé peut faire disparaître le coin qui les justifiait.
La [preuve tubes](P0_TUBES_ET_RANGS.md), la preuve de sous-rectangles et leurs
reçus restent utiles ; les avis remplacés sont condensés dans ce fichier.
Le modèle de moments passe en normal/−O ; trois mutants supplémentaires
sont réfutés. Contrôle documentaire global : 530 fichiers ; registre :
20 phases. Les Markdown indépendants sont aussi contrôlés explicitement.
Réservation de publication après **65ac5ee6**, index constaté vide :
uniquement les neuf chemins ci-dessous, tous dans ce dossier. Cette
fenêtre expire au commit/push de cette passe ; aucun fichier constructeur
ni de l’autre auditeur n’entre dans la préparation.

- DIALOGUE_COURANT.md
- P0_SOUS_RECTANGLES_ET_GROUPES.md
- p0_collective_probe.py
- P0_COLLECTIVE_CHECKS.json
- P0_INPUT_ALIAS_CHECKS.json
- P0_OWNER_CHECKS.json — suppression
- P0_OWNER_INTEGRATION.json — suppression
- p0_owner_gate.cpp — suppression
- p0_owner_checks.py — suppression

Contrats 50k, massif et FULL ouverts.
GCP non utilisé.
