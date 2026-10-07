# Contre-audit des premiers contrats v12

7 octobre 2026. Codex, auditeur demandé par l'utilisateur. Contrat numérique jugé : `e264de6f2`, SHA-256
`40baf861e474c89e3155513cdbfa3f7a61896412a85948029c776bd1ad03af0e`.
Lecture initiale de l'ouverture `13c52bc60`, puis des notes indépendantes `fc1f913ce` et `2a7a5f346`, et de la réponse
du développeur publiée sur `a0e31abfe`. Le moteur v11 de référence reste `ac081a06f`.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only` pour les appels natifs ; `public_status=not_claimed`.
Les calculs Python examinent aussi les limites u32 : ils ne qualifient pas un moteur u32.

**Avis.** Le repère local, la garde d'une boule critique certifiée et la séparation des clés de localité et des
ordres publiés constituent une base utilisable. Les ports des certificats, de l'identité des sites et des capacités
du parcours demandent les corrections ci-dessous avant adoption. Les témoins montrent des erreurs dans des règles
proposées ou dans un port direct hypothétique ; ils n'établissent pas un défaut FULL du moteur v11 gelé.

## 1. Six nouveaux constats

Les identifiants sont enregistrés dans [`audits/CONSTATS.md`](../../audits/CONSTATS.md). Les constats déjà inscrits par
l'autre auditeur restent les siens ; leurs preuves sont complétées au § 2, sans créer de doublons.

| Constat | Preuve concrète | Correction attendue avant le port concerné |
| --- | --- | --- |
| **CST-0201 — certificats dépendants du domaine** | `power_certificate.hpp` et `orientation_certificate.hpp` lisent `kCoordBits`. Un triangle aigu d'étendue 20 bits passe le certificat transposé à `s=20` ; une requête admise par `NUM-GARDE` fait déborder le premier produit i128, alors que la différence finale tient | Certificat lié au domaine de toutes les requêtes, p. ex. `s+2` pour le côté d'un site gardé ; conservation de l'ancre et des coefficients couverts ; repli avant tout produit non garanti |
| **CST-0202 — Morton sert aussi d'identité** | Dans `cloud.cpp`, l'égalité de clé suffit à compter/fusionner les sites. Avec `(0,0,0)`, `(1,0,0)`, `(2^32−1,0,0)`, les deux premiers sites distincts ont la même clé tronquée | Identité/dédoublonnage sur XYZ exacts, séparés de la clé de localité ; porte avec collisions de clé et vrais doublons entrelacés par PointId |
| **CST-0204 — la fermeture d'une boîte peut exiger 33 bits** | Sites u32 valides aux extrémités : `hi=max+1=2^32`, donc `NUM-REPERE` de la fermeture vaut `s=33` | Bornes élargies ; domaine des budgets explicite par objet ; cas d'étendue nulle et extrémités de boîtes qualifiés |
| **CST-0205 — 38 niveaux ne suffisent pas** | Catalogue v11 Release u21 : 24 sites → profondeur 60 ; 48 sites, K5, feuille 24 → profondeur 63 | Reprendre la preuve de potentiel : profondeur ≤ `3B`, donc 63/72/96 aux profils 21/24/32, plus la racine pour une capacité en niveaux ; budget distinct du nombre de nœuds |
| **CST-0207 — le test de coût a changé de grandeur** | Un moteur élargi à 120 ms contre u21 à 100 ms peut prendre 120 ms aussi après translation : le nouveau test conclut 0 %, le coût du profil est 20 % | Conserver le test D6 entre profils sur la même entrée ; mesurer séparément la sensibilité à la translation dans le même binaire |
| **CST-0208 — réservoir absent des budgets** | À `s=30`, boîte `[0,1]^3`, site `(2^30−1)^3` : `Σ(2x−lo−hi)^2 = 3(2^31−3)^2 > 2^63−1` avant le prédicat G1 | Budget propre du réservoir `2s+4`, i64 garanti jusqu'à `s=29`, ou calcul i128 ; ne pas appliquer le seuil du seul G1 à tout l'étage |

Pour CST-0201, avec `M=2^20`, `h=M−1`, support `((0,0,0),(h,h,0),(h,0,h))` et requête
`q=(3M−1,3M−1,3M−1)`, le dénominateur vaut `D=6h^4`. Le premier produit `D|q|²` vaut
`215333976975021689807285368499032031250`, supérieur à `2^127−1`. Il est donc indispensable de vérifier les
**intermédiaires**, même lorsque le résultat final est représentable.

Détails, calculs exacts et réponses aux quatre questions numériques : [rapport numérique](numerique/REPORT.md).
Les deux cas de profondeur sont exécutés sur le natif v11 ; [requêtes, réponses et provenance](repere_et_profondeur/README.md).
Le contraste des ratios est un contre-modèle du protocole, pas une mesure de vitesse : [rapport mesure](mesure/NOTE_MESURE.md).

## 2. Compléments aux constats de l'autre auditeur

- **CST-0101, T1** : accord indépendant sur la nécessité de `S⊆F⊆P_b`. La réponse publiée sur `a0e31abfe`
  corrige le contrat et grave le carré. La porte Python a été relue et rejouée : 5 faits, 8 couples du lemme,
  mutant sans inclusion tué avec code 4, mauvais usage refusé avec code 2 ; mêmes résultats sous `-O`.
  Le [reçu de relecture](relecture_reponse_t1.json) porte sur cette porte, pas sur un futur moteur.
- **CST-0102 à 0107, T3–T7** : contre-lecture indépendante concordante ; les petits modèles exacts vérifient les
  plateaux, les historiques et les coquilles. `O(m³)` désigne les prédicats géométriques de la famille de T7,
  pas tout le coût de construction du quotient. [Rapport et témoins mathématiques](mathematiques/README.md).
- **CST-0108/0109, garde** : le pavé est sûr pour une boule dont le centre est certifié dans l'enveloppe convexe
  du support. Il ne convient pas à une simple sphère passant par un support obtus. Une boîte qui dépasse le pavé
  peut contenir la boule : seule la disjonction permet le rejet immédiat. Une intersection utilisée pour minorer
  la puissance n'autorise pas à compter tous les points du nœud comme intérieurs. Le centre entier le plus proche
  de `LEM-LATTICE` se prépare en relatif ; la projection rationnelle seule n'en remplace pas le contrat.
- **CST-0111, portée du repère** : `s+2` suffit pour les écarts entre un site gardé et le support. L'union de tout
  le pavé peut exiger `s+3` : ne pas lui attribuer un certificat unique minimum-sur-tous-les-points de `s+2` sans
  une autre preuve. Le témoin `{0,3}` et les requêtes `−7,11` est dans `repere_et_profondeur/check.py`.
- **CST-0113, translation et empreintes** : un FULL synthétique à un site et sa translation sont acceptés par le
  lecteur v11, mais leurs empreintes diffèrent : le lecteur hache XYZ et les centres absolus. Contrôle positif :
  même entrée en u21 et u24, même empreinte sémantique. L'égalité de la tour se vérifie après transport de la
  translation, remappage exact des sites et application du nouvel ordre canonique. Le lecteur v11 inchangé ne
  constitue pas ce nouveau juge.

Ce dernier témoin complète **CST-0113**, déjà ouvert pendant notre travail. Les états des lignes de l'autre
auditeur ne sont pas modifiés par cette contribution.

**Révision `8865e32c1` arrivée avant dépôt.** La nouvelle version précise la garde des boules certifiées, les boîtes
partielles, le minimum entier en relatif, les requêtes initiales à centre entier et la comparaison à translation
près. Ces corrections documentaires ont été prises en compte ; les six nouveaux constats du § 1 restent ouverts.
Les nouveaux budgets mixtes `6s+11` et `7s+14` sont valides, intermédiaires compris, sous leurs hypothèses. La
comparaison des centres par parties entières puis fractions est correcte avec division euclidienne, comparaison
**axe par axe** et produits fractionnaires séparés. [Contre-lecture des trois questions révisées](numerique/ADDENDUM_8865.md).

## 3. Ordre de mise en œuvre recommandé au développeur

1. Définir les types des repères, des boîtes, des sphères seulement construites et des boules certifiées. Attacher
   chaque certificat à ses coefficients, son ancre et son domaine de requêtes ; distinguer budgets site et boîte.
2. Séparer l'identité XYZ de la clé Morton avant le port de `Cloud`. Garder le tri géométrique canonique séparé
   des indices internes et du PointId utilisé pour départager une clé de localité.
3. Graver dans `reference/` puis dans les portes natives les cas ci-dessus ; un témoin dans `receipts/` est une
   preuve d'audit, pas une dépendance à importer dans CTest. Prévoir le passage GPU pour les primitives concernées.
4. Dimensionner la profondeur selon la preuve, puis mesurer les étendues réelles et le coût des voies locales.
   Fixer séparément les comparaisons D6 entre profils et l'invariance par translation.

Ce séquencement laisse avancer les outils, l'oracle et les modules indépendants. Il ne remplace ni les portes de
chaque tranche, ni la qualification sur G4, ni le contrat de 100 ms.

## 4. Validation et limites

Les reçus conservent les scripts, résultats, entrées synthétiques, empreintes sources et binaires. Python normal et
`-O` sont comparés ; les témoins ne dépendent pas d'assertions supprimées par l'optimiseur. Les seules exécutions
natives ajoutées sont les deux petits catalogues v11. Leurs durées locales ne sont pas interprétées comme mesures
de performance. Aucun scan, aucune coordonnée KITTI, aucun compte GCP et aucune copie d'arbre de sources ne sont ajoutés.

`tools/check_constats.py` contrôle la structure du registre, les identifiants uniques et l'absence de clôture sans
preuve déclarée. Il ne certifie pas la validité d'une preuve et ne ferme aucun constat. Les états de blocage de D14
restent explicitement admis. Les résultats de ses cas négatifs sont dans `verification_registre.json`.

Les 644 fichiers sources du build v11 réutilisé ont été revérifiés sans changement. Le manifeste du présent dossier
ferme les artefacts de cet audit. **GCP non utilisé. Aucun moteur v12, profil u32, chemin GPU ni objectif de temps
nouvellement qualifiés.**

Dernier état lu avant dépôt : `d3fc3ff09`. Il conserve les six points nouveaux ouverts. Le nouvel addendum Claude
`f6f65a0d8` précise que la même borne d'orientation `7s+14` vaut aussi pour trois sites gardés quelconques :
`3 × 50 × 96 = 14 400 < 2^14`. Notre addendum sur `8865e32c1` validait la preuve plus restreinte aux coquilles ;
cette extension est correcte. La borne de profondeur 38 et la confusion des ratios D6 demeurent. Les derniers
microbancs et contrats d'échelle ajoutés en parallèle ne sont pas qualifiés par ce reçu ; ils restent à auditer.
