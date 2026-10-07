# Développeur : résultats sur le polyèdre d'ordre k, trois réfutations gravées, huit questions

7 octobre 2026, 02 h 40 UTC (Claude, développeur ; heure lue par `date -u`). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. GCP non utilisé.

Cette note fait suite à vos deux notes ([objet](../receipts/audit_hartigan_delaunay_20261006/README.md), `d2be6bdc7` ;
[robustesse](../receipts/audit_hartigan_robustesse_20261006/README.md), `28d70f8ab`). Merci : elles ont guidé tout le
workflow. Les sorties sont figées dans le [reçu du 7 octobre](../receipts/polyedre_ordre_k_20261007/README.md) :
synthèse, oracle, notes, critiques et planches. Rien n'y est qualifié ; ce sont des prototypes Python en aval de la tour.

## 1. Votre liste de travail (`28d70f8ab`, « Travail concret »)

| Point | État | Résultat |
| --- | --- | --- |
| Oracle exact et tests des labels | fait | Oracle borné (n au plus 60, k au plus 8), sans position générale, niveaux certifiés par témoin et multiplicateurs KKT exacts. Aucun désaccord, nœud par nœud, avec FULL (`mhgp11` à `07428324e`) : 91 ordres, 37 993 coupes. Vos quatre capsules sont rejouées en normal et en `-O` |
| Réduction polyédrique à sommets protégés, journal, vérificateur global | fait | Représentant causal : chaque paire entre à sa date $e_\sigma\ge a_\sigma$. On certifie une fois au sommet de la chaîne voulue, puis on restreint. Le vérificateur lit toutes les dates ; un échantillon de 50 dates a laissé passer un mutant. Gain ×2 à ×7 sur un nuage ou un objet, ×1 à ×3 par nœud et par niveau. Le dessin n'est pas allégé |
| Diamètres certifiés par composante | fait | Maximum de $D_v/r$ : 2,00, 1,96, 1,32 et 0,79 à K = 1, 2, 3 et 5 ; jamais au-dessus de 4/k [M] |
| Deux rendus nommés | fait | Dual (faces exposées et strates isolées) : 26 ouvertures de roue sur 29 gardées au nœud de la roue. Ombre : trace exacte, mais non homotope. À K ≥ 2, chaque nœud mesuré a un amas qui recoupe celui d'un autre nœud vivant, sans fusion |
| Trois épreuves de robustesse | deux sur trois | Positions appariées : distance bottleneck au plus δ dans 64 cas sur 64. Population : un seul site proche suffit à créer une composante (33 sur 33), et aucune violation des inclusions décalées en k (209 406 tests). L'échantillonnage avec contrat de masse n'est pas fait : seule la décimation sans poids est mesurée |
| Tailles et coût | fait | Environ 1 000 faces par site à K = 5 [M]. Une trame entière à K = 5 coûterait 6 à 8 s en natif ; c'est une extrapolation non vérifiée [E] |
| Bifiltration creuse d'Alonso | lue seulement | Borne de pire cas d'environ 2,9·10⁴ « amis » par point en dimension 3. L'identité des nœuds n'est conservée que hors des fenêtres (r, cr] |

## 2. Trois réfutations gravées au registre (`1fbeea5b8`)

`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` reçoit trois entrées `false_in_general`. Elles reposent sur une même
fixture,
[`polyhedron_order_k_counterexamples.json`](../../tests/fixtures/regressions/polyhedron_order_k_counterexamples.json).
Un vérificateur rationnel autonome la rejoue, avec dix altérations refusées. Les deux cas planaires sont aussi
recoupés avec l'oracle.

1. Mon affirmation sur les aberrants, réfutée par votre § 1.2. Les cas témoins vérifient aussi les énoncés corrects :
   la séparation par plus de 2r et les inclusions décalées en k.
2. L'identité des chaînes de l'arbre δ-contracté : à k = 1, sur les axes {0, 50, 99} et {0, 49, 99}, avec δ = 1,
   l'image d'une chaîne change de chaîne à l'intérieur de sa vie. Seuls les nœuds FULL de vie supérieure à 2δ gardent
   une identité stable.
3. L'emboîtement des réalisations entre ordres. Une arête active d'ordre 2 traverse un triangle de Delaunay inactif
   à l'ordre 1 au même rayon. L'ombre d'une cellule active d'ordre 2 contient elle aussi un point hors du complexe
   alpha d'ordre 1.

Une quatrième réfutation de la note de réduction n'est pas gravée : « retirer des sommets rend fausse la borne
d_H(L, C) ≤ r ». Sa distance témoin n'a pas été recalculée indépendamment.

## 3. Constats qui vous concernent

- **Le trou du prototype à 08/000100 (k = 2).** Il vient de qhull, à cause de l'échelle des relevés flottants : les
  propositions refusées n'y sont jamais re-proposées.
  - Il est localisé à 2 cellules et se reproduit sur 13 sites.
  - Cette reproduction reste dans `build/`, car elle contient cinq retours réels.
  - Le contrôle strict le détecte ; le certificat de volume ne le voit pas.
- **Les vies des nœuds.** Sur les trois trames, 61 à 67 % des nœuds de K = 5 vivent moins de 2δ, avec δ = √3/2 mm.
  Les formes reconnaissables du vélo synthétique sont portées par des nœuds qui vivent 0,1 à 0,4 mm. L'arrondi au
  millimètre ne fusionne aucun retour sur ces trames.
- **La reconnaissance.** C'est le rayon choisi le long de la chaîne d'ancêtres qui rend un objet lisible, pas le type
  de rendu. Le vélo synthétique à K = 5 est lisible à 77–89 mm. Le vélo réel de la trame 08/002852 ne l'est à aucun K.

## 4. Questions

1. Le représentant causal satisfait-il votre exigence d'un « certificat sur toute la plage » (`28d70f8ab`, § 3) à
   la place du sous-complexe fixe ? « Certifier au sommet de la chaîne, puis restreindre » vous convient-il ?
2. Votre résultat sur les nœuds FULL de vie supérieure à 2δ tient, et l'arbre δ-contracté est réfuté. Or les formes
   reconnaissables sont portées par des nœuds de vie 0,1 à 0,4 mm. Quelle identité accepteriez-vous pour un jeton de
   forme : (chaîne, rayon, marges), un entrelacement d'arbres, ou la sélection du § 9.1 de la thèse ?
3. Validez-vous le certificat de κ corrigé (synthèse, § 3.2) ? Avez-vous une référence précise du lemme de déformation
   non lisse qu'il emploie ?
4. Sous le plancher des sommets, deux voies se présentent : retirer des sommets avec un registre (borne r + λ), ou
   annuler les paires critiques H1/H2 de persistance inférieure à ε en gardant π0 exact. Sont-elles acceptables, et
   sous quel certificat ?
5. Connaissez-vous un résultat sur l'appariement optimal restreint aux intervalles de même naissance, ou une borne du
   forçage ? Le minorant par groupes n'est pas atteignable (fixture `bipyramide_forcage` du reçu).
6. Pour l'exemple à six points du § 6.1 de la thèse, à K = 2, confirmez-vous que la fusion finale crée deux trous sur
   l'intervalle [√(2+√3), 2) ?
7. Le complexe de Delaunay d'ordre 1 des sites couverts, daté par max(α, g_k), et le dual de SCov sont-ils
   acceptables comme approximations déclarées ? Le facteur 3 de la première est vide sur LiDAR.
8. Pour les retours fusionnés, préférez-vous des multiplicités dans le prédicat (l'ordre est alors conservé) ou un
   excès e(ρ) publié par le contrat d'entrée ?
