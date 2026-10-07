# Contre-audit mathématique v11 pour l'auditeur v12 — 7 octobre 2026

Snapshot documentaire et sources : `33c2ae3c8b66d12ef5b2405f1b0f807d7e57aeae` ; dernier commit moteur annoncé : `ac081a06f`.
Worktree propre au début : `/tmp/ehgp-audit-v12-math-20261007`. Aucune modification moteur, aucun commit, aucun build natif, GCP non utilisé.
Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`, `public_status=not_claimed`.

**Verdict.** La lecture ciblée ne fournit aucun nouveau contre-exemple à FULL. Les preuves de définition et de reconstruction examinées restent cohérentes ; les conditions qu'elles posent ne sont pas des preuves de coût ni de correction universelle du code. La v12 peut reprendre cet objet et ces oracles, avec ports explicites et requalification. Elle ne peut pas en déduire la stabilité d'une identité de nœud, d'un support géométrique, d'une partition plate ou d'un rendu. Les choix de projection et de forme restent des contrats supplémentaires.

Ce rapport est une contrelecture de `docs/AUDIT_GEANT_V11.md`, pas une certification intégrale de ses phrases « toutes les preuves refaites ». Les entrées README, architecture, provenance, canal d'audit, mathématiques, hiérarchie de points, sortie plate et questions ouvertes ont été lues, ainsi que les sources et portes nommées ci-dessous. L'arithmétique native et le GPU ne sont pas intégralement rejugés ici. Aucun chiffre de qualification historique n'est présenté comme une nouvelle exécution.

## 1. Constats à conserver dans le registre v12

Les priorités ci-dessous mesurent le risque d'un **port ou d'une revendication**. Elles ne désignent pas des bogues natifs établis.

| ID | Priorité | État | Constat, effet et preuve |
| --- | --- | --- | --- |
| MATH-01 | P1 | connu, ouvert | La stabilité de FULL en rayon est un entrelacement, pas une bijection de nœuds ni une stabilité des labels. P5 (`docs/MATHEMATIQUES.md:301`) le dit. H3 des points (`docs/HIERARCHIE_POINTS.md:130`) suppose l'appariement de tous les sites, les poids et les ordres fixes. Dédupliquer, sous-échantillonner ou ajouter un retour sort de ce contrat. |
| MATH-02 | P1 | connu, ouvert | La proposition N1 sur un nœud de vie > 2δ lui associe une **chaîne** dans l'autre arbre ; elle ne donne pas un identifiant de nœud persistant. N3 sur les chaînes contractées est réfutée. Le résumé « seuls les nœuds longs gardent une identité stable » doit conserver cette nuance (`receipts/polyedre_ordre_k_20261007/notes/theorie_robustesse.md:124`, `SYNTHESE.md:176`). |
| MATH-03 | P2 | précision indépendante, contrat existant | Le « Kruskal » publié conserve une hyperarête entière dès qu'une union est utile. Son graphe d'incidence peut contenir un cycle. Notre témoin à sept sites (quatre sommets de tétraèdre et trois intérieurs), K5, donne six branches, trois hyperarêtes de trois branches, cinq unions utiles et un cycle d'incidence. C'est conforme à `docs/MATHEMATIQUES.md:932` et à `src/supports/hierarchy.cpp:91`, mais interdit de traiter le payload comme un arbre biparti ordinaire. |
| MATH-04 | P1 | connu, ouvert | Les supports v2 déterminent l'arbre d'ordre K ; ils ne déterminent pas les populations dynamiques ni les composantes géométriques. `growth_ABCZ`, rejoué, gagne un site par une boule interne au niveau 25, absente de la sélection MST. Le lemme H (`docs/MATHEMATIQUES.md:808`) exige les populations des boules fortes datées, internes comprises. |
| MATH-05 | P2 | connu, ouvert | La qualification Π(K+1) est un choix de modèle. Elle supprime les amas isolés de K sites et perd les cellules Q2–Q4 ; le respect du cœur n'est pas garanti. La constante 3 est optimale dans un cadre abstrait de profils, sa minimalité géométrique reste ouverte. L'horizon de P1 n'est pas borné (`docs/HIERARCHIE_POINTS.md:90`, `:131`, `:134`). |
| MATH-06 | P2 | connu, ouvert | La tête EOM exacte certifie les décisions de son objectif, pas sa pertinence ou sa continuité. L'égalité fait gagner le parent et un écart arbitrairement petit peut changer la sélection (`src/head/select.cpp:162`). Les scores, labels et seuils de masse n'héritent pas automatiquement de la constante 3 des dates. |
| MATH-07 | P2 | connu, ouvert | Le certificat κ corrigé est un majorant valide sous les hypothèses de strates et de coquille. Son calcul exhaustif, son utilité sur LiDAR et la référence exacte du lemme non lisse restent à fermer (`receipts/polyedre_ordre_k_20261007/SYNTHESE.md:200`, `:302`). Une lecture favorable n'est pas une qualification native. |
| MATH-08 | P2 | connu, confirmé | Les notes brutes contiennent encore N3 et des recommandations réfutées ; la synthèse les corrige. Les deux §10.10, l'en-tête u18 et les contrats supports v1/v2 mélangés sont déjà signalés par l'audit géant (`docs/AUDIT_GEANT_V11.md:412`). Le port doit citer une proposition et sa correction, pas seulement un nom court comme T1, P1 ou J2. |
| MATH-09 | P2 | connu, confirmé | L'oracle A est indépendant du procédé constructif, mais partage le théorème T1 et descend d'une lignée explicite R2 ; il n'est pas une preuve extérieure de ce théorème. L'oracle des supports utilise ses MEB. Une égalité A/B/native est forte et bornée ; elle ne justifie ni une nouvelle dimension, ni des poids natifs, ni une coquille arbitraire (`reference/README.md`, `docs/PROVENANCE.md:58`). |

La phrase de l'audit géant « rien à réinventer en mathématiques » (`docs/AUDIT_GEANT_V11.md:1190`) est acceptable pour l'identité de l'objet FULL. Elle ne clôt pas MATH-01 à MATH-07. Aucun défaut FULL P0/P1 nouveau n'est affirmé ici.

## 2. Carte de l'objet, des preuves, du code et des portes

Toutes les références sont relatives à `morsehgp3D_v11/` au snapshot. Les portes nommées ont été localisées ; leur présence n'est pas une nouvelle exécution.

| Étape | Définition / preuve | Réalisation examinée | Vérité / témoin utile au port |
| --- | --- | --- | --- |
| Plus petite boule β(F) | M1 et M2, `docs/MATHEMATIQUES.md:30` | `src/tower/meb.cpp`, formules exactes de `num` | `reference/hgp11_ref/definition.py:99` énumère les sphères affines de 1 à 4 points qui contiennent F ; minimum, sans filtre barycentrique identique au moteur |
| Cat(K) | p+q ≤ K+1 ; G1–G4, `docs/MATHEMATIQUES.md:78` | module `catalogue`, boîtes de centres, support canonique | Prédicats, coquilles étendues, égalités aux frontières ; oracle A/B et juges catalogue. G4 exige une partition finie entièrement traitée et ne prouve aucune complexité globale |
| FULL | Γk, T1, `docs/MATHEMATIQUES.md:135` | cellules, descendants et plateaux dans `src/tower` | Oracle A `_sweep`, `reference/hgp11_ref/definition.py:150`. Nouvelle recoupe indépendante : 1 080 coupes de 31 nuages alignés, unions d'intervalles exactes |
| Cellules locales | T2 et T3, `docs/MATHEMATIQUES.md:165` | `src/tower/cells.cpp`, `cells_classify.cpp` | Carré, cube, octaèdre, tétraèdre : la coquille étendue ne se réduit pas au cas m=q. Les morceaux locaux peuvent se rejoindre par un chemin extérieur |
| Plateau | T4 puis lemme P, `docs/MATHEMATIQUES.md:200`, `:486` | `src/tower/forest_plateau.cpp:40`, `:72` | Fusions N-aires ; triangle équidistant, passagère, tétraèdre K5. L'union des branches ne prouve pas seule la connexité : employer le lemme P |
| Descente et mémo | T5/T6, `docs/MATHEMATIQUES.md:212` | `src/tower/descent.cpp`, `descent_memo.cpp`; garde de date initiale `forest_plateau.cpp:53` | {0,2,4} à K2 : deux terminaux possibles, même classe seulement à la coupe autorisée. D2 : β(AB)=64, après le niveau précédent du catalogue 41 mais avant 1681/25 |
| Verticales | T1/T6, `docs/MATHEMATIQUES.md:147`, `:236` | `src/tower/forest_vertical.cpp`, voie parallèle | Remontée à la coupe fermée et identité des images des enfants. Un arbre seul, ou l'égalité des longueurs, ne juge pas ces applications |
| Couverture | P2/P3 et lemme H, `docs/MATHEMATIQUES.md:253`, `:808` | `src/tower/attachment.cpp`, `src/points/incidences.cpp:47` | Incidences fortes sur toute la population I∪U ; `growth_ABCZ` ; `reference/hgp11_ref/supports.py` vérifie H à chaque coupe |
| Supports positifs | Lemme F, `docs/MATHEMATIQUES.md:700` | `src/supports/enumerate.cpp`, `counts.cpp` | Gram exact des supports, minimalité brute, sphere5 à 24 sites pour les primitives seulement ; le témoin cube exige aussi les q4 quand qmin=2 |
| Supports publiés v2 | §10.10 bis, `docs/MATHEMATIQUES.md:918` | `src/supports/hierarchy.cpp:91` ; S* seul | `tests/cli/cli_supports_oracle.py:302` : cycle triangle K1 ; notre témoin des hyperarêtes K5 précise le sens de « couvrant » |
| Qualification | Πm, `docs/HIERARCHIE_POINTS.md:90` | `src/points/qualify.cpp:111` : m-ième première incidence distincte par naissance ; fusions qualifiées immédiatement | `tests/points/points_oracle_stdlib.py`, fixtures F1/F2 ; aucune masse finale substituée à la masse datée |
| Pendaison | Hr(K+1), `docs/HIERARCHIE_POINTS.md:98` | `src/points/hang.cpp:98`, `settle.cpp:81` | Rival maximal en rayon ; dates √t+√M−√Q ; décision exacte du propriétaire vivant à coupe fermée, pas un carré de différence |
| Arbre de points | Plateaux datés exacts | `src/points/point_tree.cpp`, `src/points/settle.cpp:161` | Fixtures d'égalités de radicaux, plancher exact, garde de fin de vie, K=n refusé pour k≥2. Une structure à K fixé ne donne pas une famille laminaire sur tous les K |
| Condensation / tête | Critère A puis EOM N-aire, `docs/SORTIE_PLATE.md:18` | `src/head/condense.cpp:235`, `score.cpp:147`, `select.cpp:133` | `tests/head` : antichaînes attendues, égalité, écart 2^-70, arithmétique bornée avec refus entier. L'égalité des partitions est distincte de celle des IDs |

Points de preuve réexaminés :

- Le graphe réduit aux échanges de (k+1)-parties conserve π0 : si deux intersections de boules se rencontrent, toutes les k-parties de leur union sont reliées par échanges, au même seuil. Pour les coupes strictes, il faut réellement des boules ouvertes ; sélectionner des convexes fermés par β<a ne suffit pas.
- Le seuil Cat(K) utilise **qmin de la boule**, pas l'arité de la présentation candidate. G1 garde tous les ex æquo du K-ième voisin ; G2 récupère ainsi toute la coquille quand p<K. Une perte de coquille peut changer simultanément qmin, la canonicalisation et la cellule locale.
- Le plateau se lit depuis les composantes strictes globales ; il faut dédupliquer les racines. La séparation stricte T2 est ce qui remplace la position générale.
- Le mémo d'une cellule devient utilisable au niveau de la cellule, pas au seul niveau terminal. Notre recoupe D2 confirme que « coupe ouverte de λ » et « coupe fermée du rang précédent » ont les mêmes nœuds vivants, mais pas les mêmes sommets de Γk.
- La projection P1 retarde les points sans inventer de fusion avant FULL. C'est une approximation intérieure du recouvrement ; la couverture exacte n'est pas déjà une partition exclusive.
- Une somme de radicaux de classes carrées distinctes a une égalité décidable par annulation des coefficients ; un signe non nul doit encore être séparé. Le refus au budget n'est ni une égalité ni un résultat partiel. Les limites numériques de la tête demeurent un contrat produit séparé.

## 3. Contre-épreuves nouvelles de cette session

Commande (depuis le worktree) :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v11/receipts/audit_independant_v12_20261007/math/witnesses.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v11/receipts/audit_independant_v12_20261007/math/witnesses.py
```

`normal.json` et `optimized.json` sont identiques octet pour octet. Chaque passage prend moins d'une seconde locale ; aucune charge native ou donnée LiDAR. Le premier essai a échoué avant calcul sur un chemin Python du script d'audit, corrigé et consigné dans `attempts.txt`. Aucune assertion supprimable par `-O` ne porte le verdict.

**Intervalles.** Tous les sous-ensembles non vides de {0,1,2,3,4}, tous les ordres, toutes les dates critiques et des dates intermédiaires, coupes ouvertes et fermées : 31 nuages, 1 080 coupes. Les intervalles W_F sont formés directement par `[max F−r,min F+r]`, puis réunis selon leur intersection stricte ou fermée. Les masques de couverture sont identiques à ceux de l'oracle A. Cette porte bornée recoupe la sémantique continue sans réutiliser sa MEB ni son graphe.

**Six points de la thèse, question 6.** Calcul autonome de toutes les MEB dans Q(√3), sans `oracle_ak`, numpy ou moteur natif. Pour A=(−√3,1), B=(−√3,−1), C=(0,0), D=(2,0), E=(2+√3,1), F=(2+√3,−1), plongés dans R³, la projection verticale sur le plan diminue toutes les distances ; Ω2 se rétracte sur sa section plane. L'Euler de cette section vaut `Σ_{s≥2} (−1)^(s−2)(s−1) N_s`, où N_s compte les s-parties de MEB ≤ a. La section compacte plane n'a pas de H2 ; β1=β0−χ.

| a=r² | N2,N3,N4,N5,N6 | β0 | χ | β1 |
| --- | --- | ---: | ---: | ---: |
| 37/10 < 2+√3 | 7,2,0,0,0 | 3 | 3 | 0 |
| 2+√3 | 11,6,0,0,0 | 1 | −1 | 2 |
| 15/4 | 11,6,0,0,0 | 1 | −1 | 2 |
| 4 | 11,8,2,0,0 | 1 | 1 | 0 |

Aucune MEB d'une partie ne tombe dans `(2+√3,4)` : cela certifie toute la plage, pas seulement ces quatre coupes. Les deux trous existent exactement sur `[√(2+√3),2)` après la fusion finale. C'est un nouveau rejeu indépendant d'un fait déjà annoncé, pas une découverte d'erreur FULL.

**Kruskal hypergraphique.** La fixture géométrique a **sept sites** : quatre sommets `(20,20,20),(20,0,0),(0,20,0),(0,0,20)` et trois sites intérieurs `(10,10,10),(11,10,10),(10,11,10)`. Elle est gravée dans `docs/MATHEMATIQUES.md:901` et `reference/test_supports.py:80` sous le nom `tetraedre_k5`. Le script reconstruit les boules et leurs branches avec `Supports(pts).canonical(5)` depuis l’oracle A ; les groupes ne sont pas codés comme entrée du test. Au niveau exact 800/3, cette fixture donne les branches {0,1,2}, {0,3,4}, {1,3,5}, {2,4,5}. Dans cet ordre, trois hyperarêtes sont gardées, avec 2,2,1 unions utiles. Le graphe d'incidence conservé a neuf arêtes pour neuf sommets et une composante : rang cyclique 1. Les 5 unions utiles seules forment un arbre. Ce résultat reste vrai pour toute permutation de ces quatre hyperarêtes par symétrie. Le sélecteur DSU exécuté ici est abstrait, sans exécution native : le test reproduit géométriquement le plateau par l’oracle A puis vérifie sa sélection hypergraphique. Il n'essaie pas d'émuler l'ordre Morton natif : il expose la différence des deux objets.

**Populations et rang précédent.** `growth_ABCZ` confirme la boule interne au niveau 25 et sa population de 4 sites ; D2 confirme 41 < 64 < 1681/25. Ces deux résultats sont calculés avec les oracles existants ; ils ne constituent pas des implémentations indépendantes de M1.

## 4. Réponses aux huit questions polyèdre laissées ouvertes

Source : `audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md:67` et synthèse du reçu polyèdre. Les réponses sont des avis mathématiques bornés à leurs hypothèses ; aucune adoption produit n'est faite ici.

1. **Représentant causal : oui, sous contrat explicite.** L'appariement doit être acyclique, composé de paires facette/cofacette de même naissance, et fixé une fois sur le complexe complet de la plage déclarée. Les dates eσ sont les minima des dates des cellules libres atteignables ; cette formule donne la **plus grande** fonction satisfaisant les majorations de la définition 2, pas la plus petite. Pour chaque r, Lr doit être un sous-complexe et son complément une réunion de paires entières. Cela fournit un effondrement à chaque date et des inclusions compatibles. Certifier seulement le dernier complexe, puis enlever arbitrairement les cellules tardives, ne suffit pas : le vérificateur doit contrôler le mécanisme de dates ou toutes les dates. Les sommets protégés conservent les garanties de couverture. La restriction à un nœud ou une chaîne doit conserver les paires et les dates issues du certificat global déclaré ; reconstruire chaque réduction indépendamment perd la compatibilité. Voir `notes/theorie_reduction.md:73` et son erratum dans `SYNTHESE.md:162`.

2. **Identité : choisir un objet d'entrelacement, pas promettre un identifiant stable.** N1 suffit pour suivre la trace d'un nœud long dans l'autre arbre, sur la fenêtre intérieure et avec la marge déclarée ; l'image peut traverser plusieurs nœuds. `(chaîne, rayon, marges)` est une description de requête utile, mais aucune identité canonique stable de chaînes n'en découle. Les vies de 0,1–0,4 mm sont inférieures à 2δ≈1,732 mm pour la seule quantification à 1 mm : les garanties citées ne les protègent pas. Une image reconnaissable à ces dates reste un résultat descriptif. La sélection de la thèse n'efface pas ce problème sans nouveau théorème.

3. **κ corrigé : majoration acceptée, certificat opérationnel encore ouvert.** Pour x dans l'intérieur relatif de Fσ, Σ(x) est contenu dans l'union des labels pertinents et d_k(x)≥√aσ. Dans la coquille, d_k(x)>r. On obtient donc le majorant `R(union labels)/max(r,√aσ)` de `SYNTHESE.md:206`, à maximiser sur **toutes** les strates rencontrées. Au point d'égalité d'une strate de dimension inférieure, il faut son label complet ; échantillonner les faces pleines ne suffit pas. G1 fournit un minorant uniforme de la norme du sous-gradient. La preuve par champ de descente et longueur de trajectoire de `notes/theorie_robustesse.md:316` est plausible et détaillée ; pour un contrat natif, exiger r>0, compacité de la composante, contrôle uniforme de la coquille complète et justification du flot arrêté. La référence Chang 1981 est identifiable (DOI `10.1016/0022-247X(81)90095-0`), mais le texte primaire n'a pas été accessible dans cette contrelecture : aucun numéro de lemme précis n'est certifié ici. Le certificat κ n'a pas été calculé sur LiDAR.

4. **Retrait de sommets : acceptable avec certificat différent ; annulation H1/H2 : approximation topologique déclarée.** Le registre doit transporter les labels, dates et composantes. Avec retraits indépendants, la borne r+λ est celle du théorème 6 ; avec chaînes de retraits, λ doit être la longueur cumulée jusqu'au sommet conservé, pas le plus grand pas (`notes/theorie_reduction.md:137`). Des annulations de faible persistance H1/H2 changent volontairement l'homotopie : π0 exact seul ne certifie ni une erreur de forme ni la conservation des trous longs. Il faut des applications filtrées, une borne déclarée sur H1/H2 et une vérification par composante, y compris aux plateaux. Ce n'est pas un effondrement exact de même contrat que la question 1.

5. **Optimalité sous naissance commune : non close.** Le lemme de centre unique sépare les paires par groupe de même minimiseur, mais les cofaces futures imposent le forçage du sous-complexe. Minimiser les restes indépendamment dans chaque groupe ne minimise donc pas nécessairement le représentant causal global. `bipyramide_forcage` réfute précisément le transfert du minorant local. Aucune nouvelle borne globale ni référence primaire qui clôt ce problème n'a été établie ici. Une recherche exhaustive avec certificat sur les petits groupes est un oracle ; le glouton à dimension croissante reste un procédé à qualifier, sans garantie générale d'optimalité.

6. **Deux trous : oui, confirmés exactement.** Le calcul Q(√3) ci-dessus juge l'intervalle fermé à gauche et ouvert à droite. Il porte Ω2 et tout représentant effectivement homotope ; FULL seul n'enregistre que H0 et ne prouve pas ce résultat H1.

7. **Delaunay daté / SCov : acceptables comme approximations nommées.** Publier l'objet filtré, ses applications vers la tour de référence, le déplacement en rayon/ordre et le type d'homologie garanti. Un facteur 3 sans fenêtre utile sur LiDAR est une garantie mathématique, pas un résultat de représentation pertinente. Les contacts d'une ombre, d'un offset ou d'un complexe approché ne doivent pas remplacer les applications π0 exactes de FULL. Le succès d'une image ne prouve pas l'égalité des populations ni l'emboîtement géométrique entre K et K−1.

8. **Retours fusionnés : les poids sont la voie pour conserver l'ordre des retours.** C'est une recommandation de modèle, pas l'annonce d'un port existant. Si la cible reste la tour des retours originaux, transporter leur multiplicité dans le prédicat conserve k sous déplacement apparié ; les naissances nulles, coquilles et fenêtres pondérées exigent des preuves/portes dédiées. Si la cible choisie est l'ensemble de sites dédupliqués, publier ce changement d'objet et e(ρ), avec les inclusions à ordre décalé. Le fait e=0 sur trois trames ne qualifie pas la fusion de retours en général. Ne jamais supprimer silencieusement la masse.

**Vérification externe limitée.** Le théorème 2.1 de [Bauer–Edelsbrunner, *The Morse Theory of Čech and Delaunay Complexes*](https://pub.ista.ac.at/~edels/Papers/2017-J-03-DCech.pdf), p. 5 du PDF, a été relu : un sous-complexe dont le complément est une réunion de paires du gradient s'obtient par effondrement. Son cadre dans cet article est simplicial. La mosaïque polyédrique demande la version pour complexes cellulaires réguliers, ou un journal d'effondrements effectifs. Le théorème principal Čech–Delaunay de cet article suppose la position générale : on ne le transporte pas par simple analogie à une mosaïque d'ordre k dégénérée. Le rapport v11 a justement besoin de ses preuves propres et de son oracle.

## 5. Réponses mathématiques et d'audit aux sections T, V, X et Y

Les questions de performance ne peuvent pas se résoudre par lecture seule. Cette section fixe ce qui constitue une preuve suffisante ; elle ne remplace pas la relecture des allocations et de la concurrence.

| Question | Réponse / porte utile |
| --- | --- |
| T.1, prédicteur exact bon marché du coût d'une feuille | Les dominances et les paires vivantes donnent des informations de travail, pas un prédicteur exact du temps total. Le census et les rejets ultérieurs dépendent des candidats. Un compteur observé peut décider un placement, jamais une suppression géométrique ; son coût fait partie de l'ablation. Aucun prédicteur exact nouveau établi. |
| T.2, seules paires lourdes partagées | Les preuves de complétude se portent seulement si chaque support candidat garde une propriété unique et si paires légères/lourdes forment une partition exacte. Le nouveau scheduling demande sa porte : couverture, absence de duplication, déterminisme, réservations et refus, avec forçage des cas frontière de la partition. Une preuve d'un warp par feuille ne certifie pas des écritures entre fils. |
| V.1, invariant radix | Oui, utile en plus des reconstructeurs : pour chaque plage non singleton, clés extrêmes, plus haut bit différent, enfants non vides, préfixe commun et partition disjointe/exhaustive. Le nombre <2n et la profondeur ≤3B+1 viennent d'arbres binaires pleins à séparation stricte et des bits consommés, pas de l'équilibrage médian. |
| V.2, budget supposant une médiane | Auditer toutes les piles, chemins et formules qui utilisaient ceil(log2 n), pas seulement le nombre de nœuds. Aucun nouveau contre-exemple de capacité trouvé dans cette lecture math ; la note cite l'élargissement de `points_export` à 3B+2. Une recherche exhaustive relève de l'audit moteur. |
| X et Y.3, mutation du préchargement | La valeur empoisonnée d'une graine ne suffit pas si le préchargement n'influence jamais le résultat observé. Il faut rendre l'accès observable (journal de prises dans un exemplaire de test, lecture instrumentée, ou sanitizer ciblé) et tuer le mutant sur la cause. Un prefetch matériel peut être sans faute mémoire visible : ne pas confondre absence de crash et absence de lecture non autorisée. Aucun crochet produit ajouté ici. |
| Y.1, grandes pages | Les causes proposées restent des hypothèses ; le seul fait acquis est l'échec du critère de la campagne. Le gain local n'autorise pas un port. Mesurer défauts de pages, politique NUMA/THP, octets réellement touchés et restitution si la piste est rouverte. |
| Y.2, cache de blocs | La réutilisation doit annoncer actifs, inactifs, plafond, pic résident et fermeture, avec latences froid/chaud et contexte CUDA séparés. Mettre une réserve de cache hors de `used/peak` change le sens du budget publié et exige un autre compteur obligatoire ; cela ne se justifie pas par l'exactitude des résultats. |

## 6. Conditions minimales du port v12

- **FULL** : garder la définition Γk/Ωk, les plateaux N-aires, les coupes strictes/fermées, les verticales, la coquille exacte et qmin. Utiliser le catalogue v11, l'oracle A, les témoins dégénérés et les dumps v11 comme vérités de nature différente ; aucune équivalence de dumps ne remplace le test de définition. Les raccourcis MEB peuvent proposer, puis certifier M1/M2 ; ils ne décident pas en flottant.
- **Supports** : choisir explicitement le payload v2 et sa notion de Kruskal. Ne pas hériter des comptes, plafonds et géométries de v1, ni reconstruire les couvertures à partir des seules boules gardées. Si un vrai graphe couvrant est requis en aval, publier les unions utiles, ou spécifier séparément l'hypergraphe.
- **Points** : porter la règle en rayon, les incidences fortes datées, la qualification et le refus K=n dans son contrat courant. Conserver les égalités exactes, l'ordre fermé et les contre-exemples de première couverture/LCA. Les garanties restent à K fixe et population appariée fixe.
- **Plat** : porter l'objectif et ses portes indépendamment de FULL ; les racines, masses, égalités, ordre d'entrée et refus de budget doivent rester explicites. Exiger une évaluation de pertinence séparée ; exactitude de l'EOM ne vaut pas supériorité empirique de la hiérarchie.
- **Polyèdre** : chantier aval encore exploratoire. Les huit réponses réduisent l'ambiguïté sans créer de constructeur natif qualifié. Les corrections des notes brutes, la couverture de toutes les cellules, l'homotopie filtrée et les cartes π0 doivent accompagner le prototype ; ni les supports MST ni la stabilité de FULL ne suffisent à les fournir.

Pour le registre racine, MATH-01 à MATH-09 sont les identifiants proposés. Les nouvelles exécutions sont uniquement celles de la section 3. Les limites de preuve sont gardées comme limites, sans les convertir en défauts inventés.
