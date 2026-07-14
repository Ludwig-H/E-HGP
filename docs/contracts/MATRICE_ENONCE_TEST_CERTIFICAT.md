# Matrice énoncé–test–champ du certificat

Cette matrice est normative pour les tests T0 des cinq fixtures contractuelles. Une assertion scientifique n'est permise que si le résultat observable et les champs de complétude indiqués sont présents. Les chemins JSON seront alignés sur le schéma `MorseHGP3DResult` v1; un champ de certificat ne remplace jamais la comparaison de la sortie attendue.

| identifiant | énoncé observable | fixture ou test | sortie comparée | champ du certificat requis |
|---|---|---|---|---|
| `C-K-EFF` | $K_{\mathrm{eff}}=\min(K_{\max},n)$ et chaque forêt a un ordre valide | les cinq fixtures; cas terminal E2 | nombre et ordre des forêts | `run_certificate.k_eff` |
| `C-SMAX` | le catalogue utile s'arrête à $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$ | E1 et E2 | rang maximal des événements | `run_certificate.s_max` et `run_certificate.catalog_complete_by_rank` |
| `C-LEVEL` | tout niveau public est un rayon carré rationnel canonique | les cinq fixtures | niveaux d'événements, lots et nœuds | `run_certificate.exact_predicate_counts` et niveaux `ExactLevel` de la sortie |
| `C-K1` | à l'ordre un, les deux profils coïncident avec le single linkage de l'EMST au poids divisé par quatre | E1 | coupes `0/1/9` et arbre `B0,B1,B2,M01,M012` | `run_certificate.batches_complete_by_order` à l'offset `0` et `run_certificate.catalog_complete_by_rank` |
| `C-ISOLATED` | une vraie naissance isolée de cardinal $k$ est présente dans `full_pi0` au rang fermé $k$ | E2 | nœud `B01` de $T_2$ au niveau $1$ | `run_certificate.attachments_complete_by_order` à l'offset `1` et profil obtenu |
| `C-REDUCED-SCOPE` | pour $k\geq2$, `hgp_reduced` omet une facette qui reste isolée | E2, attente négative du profil réduit | $T_2$ réduit vide | `run_certificate.effective_profile` et `run_certificate.catalog_complete_by_rank` |
| `C-RANK-USES` | un événement de rang $s$ est un minimum à l'ordre $s$ et une selle à l'ordre $s-1$ lorsque ces ordres existent | E2 | usages naissance ordre 2 et selle ordre 1 du même événement | `run_certificate.catalog_complete_by_rank` et usages du catalogue critique |
| `C-ARMS` | à l'indice un, les bras stricts sont exactement $S\setminus\lbrace u\rbrace$ pour $u\in U$ | E3 et E4 | deux puis trois bras canoniques | `run_certificate.attachments_complete_by_order` à l'offset `1` |
| `C-ALL-FACETS` | le lot active toutes les facettes de $S$, y compris celles qui ne sont pas des bras stricts | E3 | ajout simultané de `02` au delta de couverture | `run_certificate.batches_complete_by_order` à l'offset `1` et journal de couverture |
| `C-BINARY` | deux racines globales distinctes donnent une fusion binaire qui tue une classe de $H_0$ | E3 | `M012` avec deux enfants antérieurs | `run_certificate.attachments_complete_by_order` à l'offset `1` |
| `C-MULTI` | trois bras distincts peuvent donner une multifusion ternaire unique et tuer deux classes de $H_0$ | E4 | `M012` avec trois enfants, sans nœud binaire auxiliaire | complétudes d'attaches et de lots à l'offset `1` |
| `C-MORSE-MULT` | la multiplicité locale d'indice un vaut $\binom{\lvert U\rvert-1}{1}=\lvert U\rvert-1$ et borne le nombre de classes tuées | E3 et E4 | couples `(1,1)` et `(2,2)` pour multiplicité et classes tuées | `run_certificate.attachments_complete_by_order` à l'offset `1` et événement critique |
| `C-EQUAL-BATCH` | tous les événements de même niveau exact sont contractés simultanément | E3 à l'ordre 1 | un lot commun, composante d'hypergraphe déterministe | `run_certificate.batches_complete_by_order` |
| `C-COVERAGE` | une activation sans nœud persistant reste rejouable sans réajouter un point déjà couvert | E3 | delta avec `added_facet_point_ids=[[0,2]]` et `added_point_ids=[]` | `run_certificate.batches_complete_by_order` à l'offset `1` et journal de couverture |
| `C-OVERLAP` | à $k\geq2$, les unions de points forment une couverture et peuvent se recouvrir | E5 | deux racines distinctes couvrant `012` et `234` | complétude des lots à l'offset `1` et forêt d'ordre 2 |
| `C-NO-POINT-UNION` | partager un identifiant d'observation ne fusionne pas deux composantes de facettes | E5 | deux racines malgré l'intersection `2` | `run_certificate.catalog_complete_by_rank` et complétude des lots à l'offset `1` |
| `C-VERTICAL` | chaque composante source a une cible verticale unique au même niveau fermé | E2 et E5 | `B01` vers `M01`; deux sources vers une même cible dans E5 | `run_certificate.vertical_maps_complete` |
| `C-NATURALITY` | la montée en ordre puis en échelle commute avec la montée en échelle puis en ordre | E2, E3 et E5 | carrés aux niveaux critiques consécutifs | `run_certificate.vertical_maps_complete` et complétude des lots adjacents |
| `C-EXACT-GATE` | `public_status=exact` exige le contrat et la base de preuve autorisés, puis catalogue, enfants canoniques, incidences, lots, attaches et verticalité complets sur le profil annoncé | validation négative dérivée des cinq fixtures | mutation de chaque preuve ou booléen vers une valeur insuffisante doit interdire `exact` | `run_certificate.reconstruction_contract_id`, `run_certificate.proof_basis` et tous ses champs de complétude |
| `C-M1` | l'accord des exemples `full_pi0` ne ferme pas l'obligation de preuve M.1 | E2, E3 et E4 | métadonnée de statut scientifique, distincte du résultat attendu | `run_certificate.reconstruction_contract_id`, `run_certificate.proof_basis` et `run_certificate.public_status` |

## Règles de validation négative

À partir de chaque fixture valide, les tests T0 construisent au minimum les mutations suivantes :

1. inverser le numérateur et le dénominateur d'un niveau sans préserver sa valeur doit échouer au différentiel;
2. utiliser un dénominateur nul ou négatif doit être refusé par le schéma;
3. ajouter un champ inconnu à un objet critique doit être refusé;
4. transformer la multifusion E4 en une chaîne binaire doit échouer, même si la partition finale au-dessus de $5$ est identique;
5. réunir les deux composantes E5 parce qu'elles partagent `0` doit échouer;
6. annoncer `exact` avec une fermeture requise fausse ou manquante doit être refusé;
7. annoncer `hgp_reduced` tout en conservant la naissance terminale isolée E2 doit échouer au test de sémantique;
8. annoncer `full_pi0` tout en omettant cette naissance doit échouer au test de sémantique.

Le schéma valide la forme. Les tests de contrat valident les relations entre champs et les objets mathématiques; aucune des deux couches ne remplace l'autre.
