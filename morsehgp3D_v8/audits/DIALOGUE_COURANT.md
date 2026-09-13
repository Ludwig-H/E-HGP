# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, reprise après e409aa46. Écritures limitées à ce dossier,
sur main. Réservation ponctuelle des quatre fichiers nommés ci-dessous,
close automatiquement au commit de cette publication. Les six rapports
d’ouverture du constructeur, son ETAT_COURANT en cours de mise à jour et
les preuves v7 restent sous leur autorité ; ils ne sont ni recopiés ni
réattribués. Ce fichier conserve les apports indépendants encore utiles.

## P0 : une troisième voie concrète à comparer

La [preuve tubes/rangs](P0_TUBES_ET_RANGS.md) fournit des minorants par
suffixes triés : préparation O(m log m), mémoire O(m), trois voies servies
par une même grille transverse. La marge géométrique rend le rang sûr ;
les intervalles indécis passent au consommateur complet. Sur deux tubes
favorables, le résidu d’un rectangle m×m vaut h(h+1)/2 pour m≥h.
Ce n’est ni une borne de sortie FULL ni une solution générale de P0.

À comparer aux stratégies Pool/DualBlocks actuellement en préparation.
La variante par dominance 3D évite le choix d’une grille mais demande
O(m log²m) temps et O(m log m) mémoire ; sa preuve est séparée, sans
exécution ni gain mesuré. Le modèle de tubes passe normal/−O : 90 essais
de voies, 95 040 tests aux coins et cinq mutants rejetés. La fixture
isotrope où il manque 12 vrais témoins reste dans la qualification.

Le nouveau contrat de séparation du header constructeur parle de distance
des boîtes et de **diamètre**. Il diffère du critère v7 sur centres/rayons.
Il implique D_centres≥16R_max à s≥8 : le lemme est donc applicable.
Les comparaisons s8/10/12 doivent conserver ce changement de critère explicite.

**Raccord à la famille de rails proposée par l’auditeur complémentaire :**
le modèle inchangé retrouve les 2 718 crédits de la formule, neuf cellules
par facteur et 2 916 paires résiduelles, sans développer les 1 846 881
paires du rectangle. La largeur par défaut suffit à séparer les rails.
Commande et périmètre sont conservés dans P0_TUBES_CHECKS.json : modèle
Python et comparaison à la formule, pas census exhaustif ni mesure C++.
Cette fixture est donc prête pour une comparaison au futur raccord tubes.

## Réemploi des recherches : distinguer deux négatifs

La même note donne un contre-exemple u16 : un bloc sans témoin universel
pour le parent devient positif pour un enfant. Réexaminer les négatifs
qui ne signifient qu’absence d’universalité. Les positifs universels sont
héritables avec IDs distincts ; les comptes seuls ne suffisent pas à
éviter le double crédit après changement de populations.

La lecture du nouveau `NoCredit` est favorable : son témoin de refus b₀
est commun à tout le bloc A×Z. Ce négatif survit à la scission d’A/Z avec
B constant ; revalider si une restriction de B fait disparaître b₀.

## Contrat de consommation de la brique en préparation

Le propriétaire immuable conservé par CreditPlan et les blocs indexant ses
permutations donnent une base cohérente. L’expansion des candidates est
explicitement payée par le consommateur.

`core_credit` est actuellement un compte sans IDs conservés. Ne pas
l’additionner à un nouveau parcours extérieur à A∪B qui pourrait retrouver
les mêmes sites. Réutiliser seulement les crédits A/B en excluant A∪B,
ou refaire le census depuis zéro ; transporter le cœur demanderait ses
identités ou une preuve de disjonction. Cette obligation concerne le futur
raccord, pas une faute démontrée du module actuel.

L’entrée possédée par rectangle convient à cette comparaison isolée.
Le raccord à la WSPD devra partager le nuage et sa validation, pour ne pas
copier toute l’entrée à chaque rectangle. Aucune qualification d’exécution
du C++ n’est transférée par cette lecture des headers.

## Entretien et périmètre

Les constats déjà présents dans PLAN_DE_REFONTE et VERROUS_ARCHITECTURE
ne sont pas réécrits en nouveaux audits. Aucun ancien dossier v7 déplacé,
aucun moteur ni reçu clos modifié. Les futurs avis seront intégrés ici
ou retirés quand le développeur les aura repris, en conservant seulement
les preuves et contre-fixtures nécessaires. Contrats 50k et massif ouverts.
GCP non utilisé.

## Périmètre de publication

```text
morsehgp3D_v8/audits/DIALOGUE_COURANT.md
morsehgp3D_v8/audits/P0_TUBES_ET_RANGS.md
morsehgp3D_v8/audits/p0_tube_probe.py
morsehgp3D_v8/audits/P0_TUBES_CHECKS.json
```
