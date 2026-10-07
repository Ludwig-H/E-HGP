# Contre-lecture du lecteur de transition catalogue

7 octobre 2026 ; pin `274592a30f6961cb7702125dcd2f031ff22b7df2` (lecteur livré par `66e4976d8`).
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Les coordonnées u32 ci-dessous sont des témoins locaux synthétiques, pas une qualification du régime produit.

## Acquis bornés

[`check.py`](check.py) construit ses propres catalogues par énumération des parties de cardinal 2 à 4,
résolution de systèmes de Gram avec `Fraction`, poids barycentriques strictement positifs, identité par centre
et rayon carré, puis populations exhaustives. Il n'importe ni le lecteur, ni `hgp12_ref`, ni le générateur
du développeur pour ces attendus. Les six géométries discriminantes sont reprises explicitement, puis quatre
nuages déterministes à six sites sont ajoutés. Chaque nuage est présenté sans translation, près de la borne
u21, étiré jusqu'à la borne u32, puis translaté près de cette borne. Le candidat inverse ses indices de sites.

Les **40 catalogues, 660 boules** (`q=2 : 412`, `q=3 : 204`, `q=4 : 44`) passent par le **vrai CLI** du lecteur.
Cela contrôle exactement les identités rationnelles, l'admission, les populations et les deux conventions,
y compris les changements de support sur les coquilles étendues. Trois mutations causales supplémentaires
(une boule absente, une boule dupliquée, un intérieur omis côté candidat) rendent respectivement les seules
catégories `absente`, `doublon`, `interieur`, code 1. Un compteur de section exorbitant est refusé, code 2 :
le lecteur Python n'hérite pas du débordement d'addition du lecteur C++ `CST-0225`.

La suite officielle bornée repasse également : 49 cas, dont 3 appels CLI, et les 25 mutants (23 tués par chacun
de leurs cas, 2 gardes équivalentes survivantes). Les mutations sont de vraies copies textuellement modifiées
du lecteur ; leur jugement appelle `judge_files` dans le processus. Ce ne sont pas 23 fautes géométriques :
les mutants de validation structurelle changent aussi un refus en code 1 ou 3, conformément à leur déclaration.
Les gardes de rang croisé et de cardinal sont redondantes **pour le verdict** : bijection avec niveaux denses
exacts, puis égalité des coquilles avec `q_min` contrôlé. Les compteurs détaillés d'écarts ne sont pas déclarés
identiques sous ces mutants.

**47 paires indépendantes au vrai CLI et 26 appels aux petites portes officielles par régime**, normal et `-O`.
Les résultats normal et `-O` sont identiques octet pour octet : une seule copie est conservée dans
[`normal.json`](normal.json), les deux empreintes et commandes dans [`verification.json`](verification.json).
L'optimisation est propagée explicitement au CLI et par `PYTHONOPTIMIZE` aux enfants des portes.
Sources comparées au pin avant exécution et hachées de nouveau à la clôture. Aucun C++, jeu LiDAR, GCP, test
lourd ou prétention FULL / 100 ms dans cette capture.

## CST-0227 — identité de trame décodée avec perte

Dans `reference/transition_catalogue.py:264`, `decode('ascii', 'replace')` convertit deux en-têtes différents
`audit\\xff` et `audit\\xfe` en la même chaîne `audit�`. Les deux petits catalogues valides correspondants
(728 octets chacun, neuf boules) rendent **code 0**, `transition_catalogue_conforme` ; le rapport garde cette
identité fusionnée. Contrôle positif : les trames ASCII `audit` et `autre` sont refusées, code 2.

C'est une lacune mineure du contrôle d'identité annoncé (« trames égales »), pas un défaut géométrique ni une
preuve de corruption d'un export v11 valide. Refuser les octets non ASCII avec `Refus`, ou comparer les octets
de la trame avant un décodage destiné à l'affichage, évite cette collision.

## Limites et suite de CST-0113

Le lecteur de **catalogue** existe et sa qualification synthétique bénéficie maintenant d'une contre-lecture
indépendante. Garder `CST-0113` en cours pour les sorties de tour `supports` / `cover` : Kruskal, sélection
canonique, relation de couverture et choix prescrit ne sont pas jugés par ce lecteur. Aucun catalogue natif v12
n'est qualifié par cette seule livraison. Les auto-différentiels LiDAR relatés dans `reference/README.md` ne sont
pas rejoués ici et ne remplacent pas un différentiel v12 / v11.

La complétude dépend toujours de la référence : supprimer **le même intérieur dans les deux vidages** rend
code 0 sur un témoin contrôlé. C'est exactement la limite annoncée, et non un nouveau constat. Pour la même
raison, cette porte ne certifie pas à elle seule l'exactitude d'un vidage v11 inconnu. La permutation des sites
est volontairement tolérée ; l'égalité de l'ordre de Morton requis pour les feuilles et leurs compteurs doit
rester une porte distincte de l'identité géométrique du catalogue.

## Rejouer

Depuis la racine du dépôt au pin :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_t2_20261007/transition/check.py > /tmp/transition-normal.json
python3 -B -S -O morsehgp3D_v12/receipts/audit_t2_20261007/transition/check.py > /tmp/transition-optimized.json
```

Les vidages synthétiques sont temporaires et détruits. Le manifeste ferme les fichiers textuels du reçu.
