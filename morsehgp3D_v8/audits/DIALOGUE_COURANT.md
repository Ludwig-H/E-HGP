# Dialogue courant de l’auditeur indépendant A v8

21 septembre2026, chantier33 après462c29a1, main. Écritures dans audits/.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## Retour33 et réponse sur l’ordre des témoins

**Brouillon33 relu sans défaut identifié**, aux hashes conservés dans le
[nouvel audit](q34_prefix_order_20260921/README.md) : bornes affines,
normalisation4H/16Xi, exclusions locales, admissions mixtes et réductions
des registres cohérentes. Relecture statique, sans qualification globale33.
La sonde indépendante emploie une copie explicite de la primitive affine.

**Oui à un ordre figé proche du milieu**, en gardant le pivot du rectangle
original chez tous les descendants. Deux navigateurs sont démontrés :

- Cercle [r,n), puis[0,r). Départ au plus haut nœud commençant en r ;
  au retour, raffiner toute boîte traversant r AVANT sa borne. Aucun
  nouveau sous-index ni boîte partielle, un compte/curseur/phase par voie.
- DFS orienté vers le pivot sur son chemin seulement, global ailleurs.
  Au plus48 références de frères préparées une fois et partagées ; état
  mutable constant par voie. Intercepter escape(frère) avant de suivre
  le curseur : il peut désigner un sous-arbre déjà consommé.

Le modèle passe960 exécutions et7 056 parcours structurels, normal/−O.
Deux mutants de navigation sont détectés ; le préflight où un mutant
n’était pas exercé reste archivé. Le split A/B avant feuille Z ambiguë,
les deux comptes distincts et le census repartant de zéro restent requis.

**Mesure d’appui LiDAR :18 cas,3 800 requêtes**, scans0/100/200 séparés,
n8k/16k/32k, K5/10. Même compte saturé pour les quatre ordres, oracle sur
70,56M sites cumulés. Sur les2 344 requêtes saturées de rectangles multiples,
visites par rapport à proche-du-milieu : global×2,094, chemin-pivot×1,078,
cercle×0,945. Le chemin-pivot reste entre×1,017 et×1,142 sur les18cas.
Mais il paie des tests de rang, le cercle ses découpes, et les deux leur
préparation parentale. Ce sont des requêtes échantillonnées par voie,
sans héritage A/B ni aval q4 : **aucun gain de temps global déduit**.
Un essai du port avec contexte parental proche du pivot est justifié ;
conserver le DFS global comme référence. Détails/coûts/reçus dans l’audit.

## Réponse q4 : joindre blocs de seeds et cellules

Piste sûre. La primitive existe déjà : `Q4LocalGeometry::node_bounds`
borne le produit boîte X × cellule C. Elle réduit aux quatre coins de C,
puis aux extrema quadratiques séparables en X ; les seuls coins de X
ne suffisent pas. Un signe strict exclut une incidence X×C, et X entier
seulement si aucune cellule admissible ne reste. Zéro et cellules fermées
restent actifs ; le domaine Positive conserve toutes les complétions de
la lentille, pas seulement les seeds de X. Aucun transfert vers q3.

Relayer directement vers le fragment trouvé : relancer l’entrée actuelle
à la racine pour chaque incidence répéterait le travail et risquerait des
émissions multiples. Garder acuité/propriété/canonicité et ownership des
frontières. Compter couples visités, constructions réelles de familles,
incidences, scans actifs, tris et stockage partagé : pas de borne globale
acquise par ce parcours. [Réponse détaillée, §6](q34_prefix_order_20260921/README.md).

## Entretien et preuves acquises

Les précisions31/32 et le premier invariant de préfixe, désormais lus par
le constructeur, quittent le dialogue actif. Leurs preuves, fixtures et
298 appels C++ restent dans l’[audit clos462c29a1](q34_global_contract_20260921/README.md),
sans transfert vers33. Fichiers constructeur et B préservés ; leurs
campagnes restent distinctes. Aucune réservation d’index.
