# Audit constructif du fold hybride `3147bb0`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=hybrid_differential_and_mutation_bounded`, `mode=audit_read_only`,
`public_status=not_claimed`.

Snapshot audité : `HEAD=origin/main=17b70cf003ddfa7d6b2603b1799d9df279ad4148`,
avec le prototype hybride committé
`saturated_fold_hybrid.hpp=3147bb0564d419ef048100073e724bad41ab712aeab1411e6c6507583bb67b06`,
la gate `6b044b041a60c19621eb6893de8e0d1f6266f2e71a124d172547c7406c0e8c5e`,
le pipeline `11de165c2332297362193a3b52e3382aeba1d4ecb0e624902c586aa6150257da`
et CMake `845c183e6a8d9eb406cf900cf867894c40d16f24769e8f5aac3253a0c65fc3c2`.

## Verdict utile

Le verrou combinatoire est pratiquement résolu : sur un catalogue géométrique
interne cohérent et complet, le fast path `principal-support`, la réduction des
générateurs redondants et le fallback par intersections rendent le même fold et
les mêmes records Gamma que `G2`. Le fallback live respecte les quatre
invariants difficiles : lot entièrement stagé, intersections en identifiants de
générateurs, projection vers les racines seulement après intersection et
pruning local réévalué après chaque union.

Le prototype n'est toutefois pas encore une API produit fail-closed. Il
recalcule un simple booléen `principal` depuis un triplet brut
`(points, point_count, Catalogue)`, sans certificat de source, sans sidecar lié
au catalogue final, sans provenance `q_min` et sans budget mémoire. Le bon
correctif n'est pas un nouveau théorème : c'est une frontière de confiance
typée, construite après la canonisation du catalogue.

Le statut exact est donc : **math locale et différentiel positifs; contrat
produit, admission mémoire et provenance non reçus**.

## Résultats positifs reproductibles

- Le correctif du cas `q=k+1` est décisif. Lorsque la taille cible de `T` vaut
  zéro, l'ancienne boucle ajoutait tout de même un point, construisait des
  carriers de taille `k+1`, puis les sautait silencieusement. Sur le premier
  nuage générique, cela produisait 17 fausses naissances. Le test de taille
  avant insertion rétablit la continuation exacte.
- La fixture minimale observée est `seed=20260810`, `k=1`, `q=2`, support
  `{6,7}`, saturé `{4,6,7}`. Elle exerce aussi `q=k` à l'ordre suivant et doit
  devenir une porte nommée, indépendante des campagnes aléatoires.
- Les cinq CTests hybrides live passent : deux mutants, refus du mutant inconnu,
  comparaison pipeline et refus du mode déclaré partiel.
- Les campagnes donnent 30/30 nuages génériques et 20/20 saturés, puis une
  campagne renforcée donne 100/100 génériques et 50/50 saturés à `K=6`, sans
  désaccord avec `G2`.
- La cosphère de réfutation exerce réellement les deux chemins : 96 traitements
  principaux, 286 fallback, 87 certificats positifs, 11 négatifs, 2 500 nœuds
  de trie et 278 feuilles. Les mutants `force-principal` et `raw-ball-key`
  meurent par le différentiel.
- Une permutation externe du catalogue conserve partitions et records. Les
  compteurs de travail changent cependant (`trie 508 -> 524`, `leaves 87 -> 88`,
  `postings 9253 -> 9551`), car l'ordre de traitement pilote le pruning.
- `17b70cf` a corrigé la chute dans la branche postings et supprimé la fausse
  ligne `identites=VIOLEES`. La chaîne conditionnelle reste toutefois coupée en
  deux : le mode hybride tombe ensuite dans le `else` final et imprime encore
  une fausse ligne `forme de vérité O(G^2)` à zéro.

Ces résultats sont de vraies preuves de mise en œuvre bornées. Ils ne prouvent
ni la complétude de la source à l'exécution ni une borne 50 k.

Les sondes hostiles confirment cette limite : l'API live accepte encore un
`point_count` supérieur à la taille réelle de la vue, une source vide sous
prétention complète, une boule dupliquée, `den=0`, un support qui ne porte pas
géométriquement la boule et une valeur `n_support` cohérente seulement en forme.
Une source tronquée n'est refusée que lorsqu'un lookup manque. Ces acceptations
ne réfutent pas le chemin nominal; elles prouvent que son type d'entrée brut ne
peut pas porter le statut produit revendiqué.

Sur un nuage valide `n=6/G=33`, la sonde directe rend :

| mutation d'entrée | résultat live |
| --- | --- |
| entrée valide | `ok=1`, principaux 44, fallback 3, attaches 85 |
| `point_count=pts.size()+1` | accepté avec les mêmes comptes |
| catalogue vide sous prétention complète | `ok=1`, compteurs nuls |
| boule dupliquée | `ok=1`, principaux 45, attaches 86 |
| support sérialisé incohérent | accepté avec les mêmes comptes |
| `den=0` | `ok=1`, principaux 15, fallback 45, attaches 58 |
| support géométriquement faux | `ok=1`, principaux 43, fallback 4, attaches 83 |
| dernier générateur retiré | refus seulement au premier lookup manquant |

## Solution 1 — déplacer la confiance dans un sidecar validé

La factory minimale est post-catalogue :

```text
make_validated_hybrid_sidecar(points, catalogue_final, source_receipt)
    -> Result<ValidatedHybridSidecar, Refusal>
```

Elle travaille après le tri canonique final. Elle couvre donc automatiquement
le chemin singleton, n'a aucun tableau parallèle à permuter et construit
l'index de boules avec les handles définitifs. Le fold reçoit ensuite un seul
`ValidatedHybridSidecar`, jamais `points`, `point_count` et `Catalogue`
séparément.

Le type validé possède globalement :

- une vue immuable ou une propriété partagée des points et du catalogue final;
- leurs digests canoniques, le profil d'entrée et la version du contrat;
- `source_complete_for_order[k]`, provenant du reçu de source et jamais de
  `smax>=n`;
- un index de boule injectif construit seulement après validation complète.

Pour chaque handle, il possède :

- `q_min`, `q_min_certified` et le support strict validé;
- l'état tri-valué `unknown`, `principal_certified` ou
  `non_principal_certified`;
- pour chaque `u`, le support positif `V_u` de quatre points au plus, indexé par
  le `PointId u`; ou un support alternatif obligatoire pour certifier l'état
  négatif;
- le digest des membres et la clé exacte de la boule.

La factory vérifie notamment `miniball(M)=B`, le support strict, la couverture
et la saturation de `M`, les bornes des identifiants, `den>0`, les rangs et les
digests. Sur le prototype borné, la saturation peut être rescannée. Le
`CertifiedIndex` exact déjà utilisé par `order_k_flats` peut aussi énumérer la
boule fermée puis laisser `sphere_side` décider, ce qui évite un balayage
`O(nG)`. À l'échelle, la preuve doit provenir du même index ou d'un reçu de
source lié au même digest; un simple hash ne remplace pas la complétude.

La décision devient alors mécanique :

```text
si source_complete[k] et q_min_certified et q>k+1 : une attache stricte
sinon si source_complete[k] et principal_certified et q<=k+1 : q attaches
sinon : fallback exact relatif au catalogue
```

Le fold relatif peut donc fonctionner sur une source partielle; seuls le fast
path et l'autorité du transcript sont désactivés. Le statut public doit séparer
`exact_relative_to_catalogue` de `authoritative_gamma`.

## Solution 2 — rendre chaque lookup auto-vérifiant

Le bucket actuel par centre rationnel réduit, suivi de la comparaison exacte du
niveau, identifie correctement une boule valide. Il doit être nommé
`CenterKey`, car le rayon n'est pas dans la clé de hash. À la construction,
deux handles représentant la même boule doivent soit être le même générateur
canonique, soit provoquer un refus; deux saturés différents pour une même boule
sont une faute de source.

Chaque lookup rapide doit ensuite vérifier :

1. unicité du handle exact;
2. inclusion de la face demandée dans les membres du carrier;
3. niveau du carrier strictement inférieur au niveau courant;
4. cohérence des digests et de l'ordre d'activation.

Un lookup absent sous source certifiée complète refuse le lot. Sans certificat
de complétude, il ne prouve rien et le dispatcher choisit le fallback. Une
miniboule supprimée invalide ou plus grande que `B` est une contradiction et
refuse; l'égalité avec `B` ne vaut `non_principal_certified` qu'avec son support
alternatif vérifié.

## Solution 3 — un fallback simple avant le trie optimisé

Le noyau le plus robuste pour recevoir le fallback ne doit même pas matérialiser
les `k`-faces. Pour un nouveau générateur `M`, balayer les postings de chacun de
ses points et maintenir un compteur saturant par `GeneratorId`. Un candidat
`N` devient voisin exactement lorsque son compteur atteint `k`, puisque ce
compteur vaut `|M intersection N|`. La projection DSU se fait seulement à ce
moment; l'union porte le vrai handle `N`, et les `k` premiers points communs ou
une intersection courte fournissent le témoin d'arête.

Des tableaux `epoch`, `count` et `reached` réutilisables donnent une mémoire
`O(G)` sans remise à zéro globale. Tous les générateurs du lot sont ajoutés aux
postings avant le premier balayage. Ce noyau ferme naturellement les quatre
erreurs structurelles documentées et fournit une vérité exécutable simple pour
le trie.

Son coût est `sum_{M fallback} sum_{x in M}|P_x|` : il reste pessimiste en masse
postings, mais ne la paie que pour le sous-ensemble fallback. C'est le bridge
exact conseillé. Le trie demand-driven reste une optimisation optionnelle,
différenciée contre ce noyau sur chaque lot.

Si le trie est conservé, stocker dans les postings des identifiants d'activation
canoniques les rend déjà triés. Il ne faut plus copier puis trier le posting
entier à chaque nœud. À une feuille, une table
`root -> smallest_real_incident` conserve le carrier véritable; unir au seul
représentant DSU préserve la partition mais détruit le certificat d'arête.

## Corrections locales avant toute mesure

1. Refuser explicitement `--join hybrid --memory-budget-mb N` pour tout `N>0`
   tant qu'aucun modèle ou allocateur plafonné n'existe. Le live accepte encore
   1 MiB et ignore silencieusement le paramètre.
2. Former une seule chaîne d'affichage
   `hybrid / faceowner / postings / G2`; le live publie encore une fausse ligne
   G2 après le reçu hybride.
3. Remplacer le `continue` sur une taille de carrier inattendue par un refus
   interne; le bug `q=k+1` a précisément montré qu'un tel silence masque une
   perte d'attache.
4. Valider `point_count`, la taille réelle de la vue, `count<=INT_MAX`, les
   rangs, `den>0` et toutes les conversions avant allocation. La négation de
   `INT128_MIN` dans le PGCD doit également être évitée ou rendue impossible par
   le type validé.
5. Publier des identités indépendantes par chemin et par `(k,q)`. L'identité
   live `attaches==unions_attempted` compare deux incréments adjacents et ne
   reçoit aucune exhaustivité.

## Portes minimales restantes

- Graver la fixture `q=k+1` ci-dessus et un mutant qui réintroduit le point de
  trop dans `T`.
- Extraire le fallback dans un noyau combinatoire et tuer séparément : projection
  par racine avant intersection, `seen` global au lot, postings stricts sans
  staging et union à profondeur inférieure à `k`.
- Exiger sur la cosphère `principal>0`, `fallback>0`, certificats positifs et
  négatifs, nœuds et feuilles strictement positifs; le différentiel actuel jette
  le reçu hybride et permettrait à un chemin jamais exercé de rester vert.
- Ajouter une permutation hybride sémantique. Si les compteurs doivent être
  canoniques, ordonner les générateurs du lot par `(niveau,membres,BallKey)`;
  sinon les étiqueter `relative_to_catalogue_order` et ne pas les comparer.
- Muter le sidecar : oubli d'un `u`, `<` remplacé par `<=`, couverture de
  `M privé de u` omise, support externe, certificat permuté, digest obsolète,
  faux `q_min`, faux bit de complétude, doublon de boule et singleton décalé.
- Une allocation injectée en échec doit produire un refus propre et un reçu
  remis à zéro; aucun benchmark 50 k ou GPU ne précède cette porte.

## Conclusion

Claude n'a plus besoin de chercher une nouvelle réduction mathématique. La
séquence la plus courte est : deux corrections CLI locales, sidecar validé
post-catalogue, fallback compteur comme vérité de production, puis trie optimisé
seulement s'il apporte un gain mesuré. Cette route conserve l'invariant v3 : ni
mosaïque d'ordre supérieur, ni graphe de Johnson, ni table globale de faces.

GCP non utilisé.
