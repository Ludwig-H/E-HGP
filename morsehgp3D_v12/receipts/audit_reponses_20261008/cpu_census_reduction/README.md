# Proposition : fusion de la réduction du census CPU

8 octobre 2026. **Proposition non implantée, sans gain mesuré.** Neuf sources identiques à FULLN
`8a0716e74`, au produit B3 `545ed987e` et au HEAD observé `0e16aaa3a` sont épinglées dans
`capture.json`. Aucun moteur exécuté ni compilé ; aucun accès GCP ou aux données géométriques.

Le rapport public FULLN épinglé donne sur ng00, CPU/K5/W48, 354,36 ms de mur et 298,60 ms pour C,
dont 154,82 ms de comptage des feuilles et 23,49 ms d'émission. Ce sont les valeurs publiées,
pas une nouvelle admission des chronos. Elles motivent un microbanc du census ; elles ne prouvent
pas que les votes dominent les feuilles. FULLN ne donne pas les compteurs `judged`, `census_tests`
ou `leaves_rewritten` permettant cette attribution. B3 vise surtout la recherche des supports de G.

Dans `leaf_census.hpp:134–166`, l'hôte cesse les prédicats après saturation, mais parcourt encore
N cases pour initialiser trois tableaux booléens puis les trois `ballot()` de `simt.hpp:226`.
Le compilateur peut déjà réduire une partie de ce travail : aucun coût machine n'est déduit du texte.
Proposition à microbancher : garder **la classification géométrique commune**, mais fournir une
réduction hôte qui construit directement les masques intérieur I, coquille U et faute, dans l'ordre
des rangs. Le warp GPU produit garde ses votes. Une éventuelle spécialisation `!__CUDA_ARCH__`
cible l'hôte ; `!MHGP12_SIMT_WARP` couvrirait aussi le mutant CUDA sériel, portée plus large.

## Contrat exact de la réduction

Raccord requis : N vaut 32 ou 256, `0<m≤N`, générateurs distincts dans `[0,m)`, q∈{2,3,4},
`1≤K≤12`, `q≤K+1`, donc **θ=K+1−q≥0** sans soustraction non signée invalide. Les catégories
proviennent de la priorité actuelle : générateur ⇒ U ; sinon dominance intérieure ⇒ I ; sinon
dominance extérieure ⇒ O ; sinon prédicat ⇒ I/U/O ou faute F. Les générateurs sont des contacts
certifiés ; le modèle abstrait n'établit pas cette propriété géométrique.

La réduction visite les rangs croissants jusqu'au **premier (θ+1)-ième intérieur inclus**, ou jusqu'à
m. Elle continue après une faute, jusqu'à cette même borne : le préfixe de classifications est donc
exactement celui du code actuel. En fin de réduction :

- Si une faute a été vue, `kLeafInvariant` gagne, **zéro ajouté à `census_tests`**, même si un
  intérieur ultérieur a saturé le compte. `judged` a déjà été incrémenté, une fois, avant la réduction.
- Sinon, si le rang t a saturé, rejet et ajout de **t+1** à `census_tests`.
- Sinon, ajout de **m** ; I et U sont complets et passent à l'aval inchangé. Aucun arrêt à θ :
  le dernier contact de la coquille doit encore être collecté.

Une faute au-delà de t reste masquée comme aujourd'hui : jusqu'à `m−t−1` catégories fautives du
suffixe peuvent être ignorées. Examiner ce suffixe « par sécurité » changerait le refus observé.
À l'inverse, retourner immédiatement après une faute modifierait le préfixe évalué ; la proposition
ne le fait pas. Le modèle suppose une classification déterministe, lisant des données immuables,
sans effet observable autre que sa catégorie locale. **Il ne suppose pas l'ordre commutatif** :
inverser les rangs peut changer la visibilité des fautes et le compteur logique. Le code des
prédicats épinglé ne montre que des calculs locaux et le paramètre de sortie ; le modèle ne prouve
ni leur arithmétique ni leur compilation.

## Enveloppe conservée, hors du modèle de votes

Le test global `inside & outside` reste **avant la première voie** et gagne même si le conflit
concerne un rang derrière le futur seuil. Il n'est pas remplacé par une observation du préfixe.
`wide_leaf` relève du parcours : le niveau est refusé avant l'émission et la consommation de ses
feuilles (`traversal_driver.hpp:173–175`). Cette réduction ne change pas cet ordre et ne traite pas
ce refus. Les niveaux antérieurs peuvent déjà avoir été consommés : aucune priorité globale
entre leurs fautes et le `wide_leaf` d'un niveau ultérieur n'est revendiquée.

La canonicalisation, la vérification de S*, le test `p+qmin≤K+1`, puis la limite de coquille 64
restent dans leur ordre actuel **après** la réduction. Un masque U de 65 contacts doit pouvoir
sortir du helper : une canonicalisation ou un test d'admission peut encore rejeter avant
`shell_capacity`. Aucun plafond de coquille ni calcul de qmin n'entre dans le helper.

N32 est un mot de 32 bits ; N256 est quatre mots de 64 bits (`Bits<4>`, hôte seulement). Les bits
`[m,N)` restent nuls. Le modèle construit ces mots explicitement et les compare à des ensembles
de rangs. Il ne prouve ni l'absence d'UB C++ ni l'équivalence des fautes CPU/GPU : le GPU évalue
toutes ses voies, contrairement au préfixe hôte. Les catégories abstraites excèdent les entrées
géométriquement réalisables ; ce domaine plus large vérifie seulement la transformation des votes.

## Contrôles et prochaine décision

`check.py` compare l'ancienne boucle hôte, la réduction proposée et une spécification indépendante
du préfixe : tous les mots de catégories I/O/U/F jusqu'à six rangs, tous leurs seuils distincts, N32
et N256 ; compléments jusqu'au rang 255 et les 33 couples K/q admissibles. Huit mutants Python
ciblent seuil, faute, arrêt prématuré, largeur, padding et permutation. Ce sont des mutants du
**modèle**, pas du moteur. Le cas U×65 vérifie seulement que la réduction ne décide pas le plafond.
Le mutant `return_at_first_fault` viole le contrat renforcé du même préfixe évalué ; sous la
précondition de pureté, son statut extérieur faute/delta0/aucune émission peut rester identique.
Le tuer ici n'établit donc pas qu'une autre optimisation avec cet arrêt serait incorrecte.

```sh
python -B check.py DEPOT_GIT
python -B -O check.py DEPOT_GIT
```

Avant toute adoption : microbanc déclaré, mêmes feuilles/candidats et résultats exacts, histogramme
des préfixes et taux de rejet ; puis portes natives de masques/faute/coquille et comparaison FULL
CPU entière avec cache/répétitions fixés. Préserver les compteurs logiques ; mesurer séparément les
opérations physiques évitées. Ni la géométrie, ni FULL, ni un gain ne sont qualifiés par ce reçu.
