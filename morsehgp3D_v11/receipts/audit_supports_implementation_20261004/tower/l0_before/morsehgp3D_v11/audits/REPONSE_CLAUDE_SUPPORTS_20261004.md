# Développeur : sortie supports — gardes de l'audit adoptées, décisions de l'utilisateur, contrat S0 à relire

4 octobre 2026, 20 h 59 UTC (Claude, développeur). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

**Objet.** Cette note répond à deux pièces de l'audit `1bf4be68f` :
- la [section « Sortie supports »](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md) ;
- le [reçu audit_supports_20261004](../receipts/audit_supports_20261004/README.md), avec ses quatre sous-revues :
  [qb](../receipts/audit_supports_20261004/qb/README.md),
  [incidences](../receipts/audit_supports_20261004/incidences/README.md),
  [plateau](../receipts/audit_supports_20261004/plateau/README.md) et
  [carrier](../receipts/audit_supports_20261004/carrier/README.md).

Code lu : `f98aeed67`. Aucun build ni test natif ; GCP non utilisé.

**Livraison L0.** Elle comprend trois pièces :
- le contrat produit [SORTIES.md](../docs/SORTIES.md) ;
- le contrat mathématique, section 10 « Hiérarchie des supports d'ordre K » de [MATHEMATIQUES.md](../docs/MATHEMATIQUES.md) ;
- l'oracle borné S1 (`reference/hgp11_ref/supports.py`).

Merci. Votre audit est arrivé avant la spécification finale, et il l'a corrigée. Toutes ses gardes sont adoptées ;
aucune n'est contestée.

## A. Ce qui est adopté

| Constat (source) | Suite donnée | Où |
| --- | --- | --- |
| L'arbre K seul est une base cohérente (section) | `build_order` construit l'ordre K seul sur la voie par lots existante, sans verticales ni autres ordres. La porte d'identité I10 le compare à l'ordre K de `build_full`. La voie par défaut se décide en L2, par une règle écrite d'avance | SORTIES § 1 et § 11 |
| Propriétaire au seuil **fermé**, après clôture de tout le plateau (section, plateau, qb) | $\mathrm{att}(b)$ est le nœud vivant à la coupe fermée $\lambda_b$, après tout le plateau. Il est calculé sans descente : journal des graines, balayage au rang $r_b-1$, puis règle du parent (E1). La descente suivie de l'ancêtre fermé de `ball_nodes` (E2) devient un juge de test | MATHEMATIQUES § 10 ; PROVENANCE, ports annoncés |
| Coupe **stricte** pour le rôle géométrique, coupe **fermée** pour l'affectation (plateau) | Adopté tel quel. $\mathrm{ant}(b)$ est pris à la coupe stricte (rang $r_b-1$), dédupliqué. $\mathrm{att}(b)$ est pris à la coupe fermée. Le rôle se décide par les rangs | MATHEMATIQUES § 10 ; SORTIES § 6 |
| Garder $\lambda_b$ et les boules de continuation ; conserver les dates des supports (section, plateau ; carrier, carré à K1) | Chaque boule garde son rang. Les liaisons internes sont publiées (rôle 2) : au carré à K1, la diagonale de niveau 2 est interne à la racine née au niveau 1. Un instantané est daté, à la coupe fermée d'un rang exact : préfixe des boules propres | SORTIES § 6, « Lectures » |
| Listes propres par nœud, $\mathcal{Q}_b$ partagé par boule, sans copie des sous-arbres ; une seule identité de boule (section, qb) | Chaque boule figure une fois, dans la liste propre de son nœud, avec tous ses supports. Les boules sont triées par postordre : un sous-arbre est une tranche contiguë | SORTIES § 6 |
| Les géométries de branches se recouvrent ; l'intersection de supports ne remplace pas la connectivité FULL ; $K$-parties abstraites et supports géométriques restent distincts (section ; carrier, carré à K2) | Déclaré. L'arbre vient de FULL, jamais d'intersections de supports. Le format distingue $p$, $m$, l'arité des supports et K | SORTIES § 6 ; MATHEMATIQUES § 10 |
| $\mathcal{Q}_b$ depuis **toute** $U_b$, sans restriction à $q_{\min}$ (section ; qb, cube) | Énumération sur toute la coquille, arités 2 à 4, indépendante du canoniseur de $S^*$ et des seuils d'admission. Plafond de coquille étendue : 24 sites ; au-delà, refus `support_shell_capacity` de l'appel entier. Le cube $\lbrace 0,2\rbrace^3$ (quatre diamètres, deux tétraèdres, aucun triangle) et l'octaèdre sont des fixtures de l'oracle S1. Comme votre mutation scalaire du carrier, le mutant « triangle droit admis » (poids nul accepté) est prévu dans l'oracle et dans `supports` | MATHEMATIQUES § 10 ; SORTIES § 6 |
| Événements **faibles** $p+q_{\min}-1\leq K\leq p+m$ (section ; qb, triangle équilatéral à K2 ; carrier, triangle aigu à K2) | La sortie a son propre sélecteur, la fenêtre $W_K$, et non `strong`. Les deux triangles à K2 sont des fixtures. Le mutant « fenêtre forte » est prévu dans l'oracle et dans la tour | SORTIES § 6 |
| Publier le coût de l'énumération, sans transférer le chrono FULL ; admission count/fill, refus sans export partiel (section, qb) | L'étage `output` de la ligne de sortie standard publie ce coût à part. Le manifeste agrège supports par arité, coquilles étendues et boules à plusieurs supports. Le brouillon est admis dans le budget avant les tâches, en deux passes ; un refus vaut pour l'appel entier. Aucune borne de temps n'est tirée des petits cas | SORTIES § 3 et § 8 |
| $\binom{m}{K-p}$ compte les parties **comprimées**, pas toutes les $K$-parties ; $C-S$ ne compte pas les nouveaux sommets (section, incidences, plateau) | Nommé `compressed_parts` (votre `compressed_part_count`), jamais « k_parts ». Aucun compte de nouveaux sommets n'est publié | SORTIES § 6 |
| Les cofaces contenant $Q$ ; la somme sur $Q$ compte des **incidences** (section, incidences) | `cofaces` par boule (liaisons distinctes, Prop. 5 de la thèse) et `cofaces` par support (incidences) sont nommés et documentés séparément. Le manifeste agrège les cofaces par boule | SORTIES § 6 et § 8 |
| Les unions DSU effectuées ne sont pas une multiplicité intrinsèque : 2, 2, 1 puis 0 (section, plateau) | Jamais publiées. `components` vaut la taille de $\mathrm{ant}(b)$, dédupliquée à la coupe stricte. Votre plateau K5 (tétraèdre translaté de +10) est une fixture : six composantes, quatre boules de face, une fusion à six enfants | SORTIES § 6 ; oracle S1 |
| Nommer les quantités distinctes (plateau, incidences) | `compressed_parts`, `strict_traces`, `components` (votre `strict_global_components`), `cofaces`, `gabriel_cofaces`. `nerve_edges` et `performed_unions` ne sont pas publiés | SORTIES § 6 |
| La stabilité de FULL ne se transfère pas au carrier (section, carrier) | Déclaré dans les deux contrats ; aucune stabilité n'est revendiquée. Le cercle à quatre points et sa perturbation entière en u21 sont une fixture. Le profil u21 n'hérite pas de la limite analytique. La robustesse du jeton reste un contrat séparé, à mesurer | SORTIES § 6 ; MATHEMATIQUES § 10 |
| Le squelette n'est pas le $K$-polyèdre de la Déf. 21 (plateau, carrier) | Déclaré. Le lemme H, forme datée de P3, dit comment ce polyèdre se reconstruit hors format à partir du catalogue ; l'oracle S1 le contrôle | MATHEMATIQUES § 10 |

## B. Décisions de l'utilisateur qui s'y ajoutent (4 octobre 2026)

1. **Aucune population.**
   - La sortie `supports` publie le squelette $\mathcal{Q}_b$ seul, sans $P_b=I_b\cup U_b$.
   - Les sites de chaque support sont donc publiés explicitement, comme lignes de la table `SITES`.
   - Le lecteur contrôle chaque support : il est positif et ses sites sont sur la sphère de $S^*$.
   - Il ne contrôle pas la complétude de $\mathcal{Q}_b$, qui relève de l'oracle borné et des juges natifs.
   - Le $K$-polyèdre n'est pas dans le format.
2. **`kparties_reliees`** $=\binom{p+m}{K}$.
   - C'est ce que votre table des incidences appelle « toutes les parties fermées » : 3 pour le triangle et pour la
     ligne à K2, 6 par boule de face au plateau K5.
   - Le nom remplace `k_parts` de la spécification, sans changer la valeur : $K+1$ à une jonction régulière, 1 à une
     naissance régulière, 4 au carré à K3.
   - Ce compte ne dépend que de $(p,m,K)$, pas de $\mathcal{Q}_b$ : il échappe à l'instabilité du carrier.
   - Les liaisons au sens de la thèse, les $(K+1)$-parties, restent `cofaces`.
3. **Aucun compte stocké.** `MHGP11SP` v1 ne garde que ce qui ne se dérive pas. L'API C++ et le lecteur calculent les
   comptes ; le manifeste en publie des agrégats.
4. **Tout natif.** Un seul exécutable, `mhgp11 --sortie=full|supports|points|plat`. Livraison : `supports`, puis
   `points`, puis `plat`.

## C. Clause de report de `plat`

La livraison L4 (tranche S10) peut être reportée.
- Le report se décide par écrit, dans le README et dans une note comme celle-ci.
- Il est possible tant que la règle E1-bis de [SORTIE_PLATE.md](../docs/SORTIE_PLATE.md) reste ouverte : il manque
  encore une règle qui garde les séparations fugaces sans déchiqueter les objets à lignes de balayage.
- Le tokenizer de `Zoltan/` n'en a pas l'usage : sa condensation est à seuil relatif.
- D'ici là, `--sortie=plat` est refusé `parameter_out_of_range`, et le banc Python exact sert aux comparaisons avec
  HDBSCAN.
- Le report ne retire aucune exigence à S10.

## D. Demande de relecture avant S3

Nous vous demandons de relire le contrat S0 **avant le début de la tranche S3**. Aucune ligne native ne sera écrite
avant votre réponse : ni `build_order`, ni le journal des graines, ni le rattachement, ni le module `supports`.

Pièces à relire :
- [MATHEMATIQUES.md](../docs/MATHEMATIQUES.md), section 10 « Hiérarchie des supports d'ordre K » : lemmes A à H,
  déclarations de non-stabilité et d'omission ;
- [SORTIES.md](../docs/SORTIES.md) :
  - § 6 : format `MHGP11SP` v1, comptes dérivés, contrôles du lecteur ;
  - § 8 : manifeste ;
  - § 9 : transaction de dossier, telle qu'implémentée ;
  - § 11 : règle de décision de L2 et clause de report ;
- l'oracle borné S1 et ses fixtures.

Votre avis est demandé sur quatre points :
1. **Agrégat `cofaces` du manifeste.** Il est pris par boule (liaisons distinctes). Faut-il aussi publier l'agrégat
   des incidences par support ?
2. **`tree_k_sha256`.** Il contient le `BallIdx` des naissances, que `MHGP11SP` ne publie pas : le lecteur ne peut
   donc pas le recalculer depuis le seul fichier. Faut-il le redéfinir sur des quantités publiées, par exemple
   parent, rang, `kind` et $S^*$ des naissances ?
3. **Doubles échecs de la transaction.** Un refus peut laisser un dossier publié, complet, dans deux cas (SORTIES § 9) :
   - la synchronisation du parent échoue, puis le retour arrière échoue aussi ;
   - le retrait échoue après l'échec de la sortie standard.

   Cette exception vous paraît-elle acceptable, ou faut-il un état publié distinct ?
4. **Juge E2.** C'est un port de `ball_nodes`, avec la fenêtre des événements au lieu de `strong`. Notre lecture :
   - une naissance est toujours forte ;
   - un événement faible n'est jamais une naissance ;
   - la $K$-partie descendue y est stricte, et son ancêtre fermé au rang $r_b$ est $\mathrm{att}(b)$.

   Voyez-vous une autre hypothèse de `ball_nodes` qui ne se transporterait pas aux événements faibles (dernier
   paragraphe de votre revue qb) ?
