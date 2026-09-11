# Journal incrémental : stabilité des coupes et économie nette

11 septembre 2026, contrelecture du plan publié par `ad7ffd28`.
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [plan constructeur](../../docs/PLAN_JOURNAL_INCREMENTAL.md) couvre déjà
l'essentiel : propriétaire des populations, lot atomique, assembleur privé,
refus définitif et publication globale. Ce complément précise une conséquence
testable de ces règles et chiffre l'économie **nette** de stockage logique.
Il n'ajoute pas de verrou mathématique. Aucun assembleur incrémental, modèle,
oracle nouveau, binaire ou mesure moteur n'est livré par ce paquet.

## 1. Un append préserve les anciennes coupes

Soit un préfixe valide, puis l'ajout réussi d'un lot complet de niveau r.
Le domaine est fixe et les populations déjà référencées restent identiques.
Les arènes `nodes`, `parents` et `contributions` prolongent leur préfixe.
**`successors` n'est pas append-only** : chaque ancien parent consommé par une
multifusion reçoit une seule fois un successeur nouveau, de niveau r. Un
successeur déjà renseigné ne change jamais, puisque son nœud n'est plus une
racine vivante. Une continuation conserve son identité et ne reçoit que des
contributions datées r. L'imbrication des ensembles de points dans une même
racine, ou leur recouvrement entre racines, ne change pas ces règles.

Pour toute coupe t<r, ouverte ou fermée, ainsi que pour la coupe ouverte r,
les nouveaux nœuds et contributions sont inactifs. Le lecteur s'arrête avant
chaque nouveau successeur de niveau r. Les racines des anciens segments et
leurs couvertures sont donc exactement celles du préfixe précédent ; les
nouveaux identifiants sont absents. La coupe fermée r peut changer. Cette
preuve par un append donne par induction la stabilité après toute extension
valide à des niveaux ultérieurs. Elle protège aussi les verticales consultées
à une ancienne coupe : une fusion inférieure future ne déplace pas
rétrospectivement l'image. Elle ne promet pas la constance des images aux
coupes qui admettent cette fusion future.

L'égalité avec l'encodeur actuel se prouve séparément par induction sur les
lots : même état `live` et même `prior_count` à l'entrée, même validation
complète des parents avant toute restauration de continuation, puis mêmes
IDs, offsets, parents, dates et écritures de successeurs. La capacité des
vecteurs ne fait pas partie de cette égalité de contenu. Après un refus,
l'assembleur est abandonné ; cette preuve ne permet ni de publier ni de
reprendre son préfixe partiellement modifié.

**Gate proposée, non exécutée ici.** Reprendre les lots `structural_mixed`
du [corpus indépendant](../receipts_coverage_cpp_20260910/corpus.py) :
naissances, continuation et fusion ternaire au même niveau, recouvrements,
contributions répétées, puis fusion future. Construire chaque préfixe valide
avec la façade actuelle sur la même banque finale, et comparer la vue interne
de test de l'assembleur après chaque append. Contrôler les anciennes coupes
ouvertes/fermées, l'ouvert r contre le préfixe précédent, et le fermé r contre
le préfixe augmenté. Vérifier aussi que seuls les parents nouvellement
fusionnés changent de successeur, vers un nouveau nœud de niveau r. Rejouer
les coupes fermées anciennes après la fusion future. Aucun préfixe public
n'est nécessaire à cette gate ; ses vues restent propres au test.

Le [plan, section transaction](../../docs/PLAN_JOURNAL_INCREMENTAL.md#transaction-et-refus-à-préserver)
et le [contrat v2](../../docs/CONTRAT_COUVERTURES_DATEES.md#lectures-et-atomicité)
imposent déjà cette sémantique. Le lemme distingue explicitement les écritures
permises dans `successors` d'un faux invariant d'immuabilité de ses octets.

## 2. Ce qui disparaît, et ce qui réside plus tôt

Les chiffres suivants viennent uniquement du triplet **historique régulier**
8k/16k/32k de [full_ball_runs_20260910](../../receipts/full_ball_runs_20260910/README.md).
Ils ne décrivent ni le nouveau G4 ni une exécution incrémentale. L'ABI vient
de la [sonde indépendante épinglée](../receipts_tower_cost_review_20260910/layout_capture/receipt.json),
déjà exécutée le 10 septembre : Batch80, Action48, Ref16, FullNode64,
contribution datée80 et identifiant u64 de 8 octets.

Noter N les nœuds, P les références de parents finales, C les contributions,
T les continuations publiées, A=N+T les actions et B les lots publiés. Les
parents présents dans les actions sont P+T. Les volumes logiques sont :

- Brouillons : `D = 80B + 48A + 8(P+T) + 16C`.
- Quatre arènes finales : `F = 72N + 8P + 80C`.
- Écart à la même frontière : `D−F = 80B − 24N − 64C + 56T`.

Sous `extra_records=0`, les connexions régulières ont une contribution vide ;
leurs continuations ne sont pas journalisées. Ainsi T=0, A=N et C est le
nombre de naissances. Le lot initial K1 groupe n actions. Chacun des autres
lots contient au moins une action : `B ≤ N−n+1`. Un lot groupé de m boules
produit au plus m actions et économise donc au plus m−1 en-têtes de batch.
Si L=`lot_dsu_slots` et G=`grouped_lots`, on obtient
`N−n−(L−G)+1 ≤ B ≤ N−n+1`. Les lots inertes omis ne compromettent pas la
borne inférieure : leur économie a seulement été majorée trop largement.

| n | Intervalle de B | Brouillons moins arènes finales, octets logiques |
| ---: | ---: | ---: |
| 8 000 | 3 967 999 à 3 968 473 | **68 107 248 à 68 145 168** |
| 16 000 | 8 291 348 à 8 294 400 | **142 168 536 à 142 412 696** |
| 32 000 | 17 114 695 à 17 134 976 | **292 786 504 à 294 408 984** |

Cette comparaison porte sur la fin du dernier ordre, avant copie de la banque.
La comparaison concerne les journaux seuls. Banque, verticales, histoires,
catalogue partagé et index sont exclus ; `live`, scratchs, surcapacités,
allocateur et coexistence lors des réallocations sont également exclus.
Les 2 193 190 400 octets d'en-têtes minimum à 32k ne sont donc pas le gain
net : le format final remplace aussi ces brouillons et réside plus tôt.
L'intervalle ci-dessus n'est **ni une baisse RSS mesurée ni une borne du gain
de pic**. Le plan signale déjà ce compromis ; le présent calcul le chiffre.
Il ne se transpose pas au cas non régulier en imposant T=0.

## 3. Reproduction et autorité

[verify.py](verify.py) épingle puis appelle le lecteur historique
`receipts_tower_cost_review_20260910/verify.py::compute()`. Celui-ci revalide
les manifestes constructeur, captures, archives sources et preuve ABI déjà
scellés. Aucun header actif n'est recompilé ni consommé comme une nouvelle
autorité. Le calculateur vérifie les deux expressions algébriques de l'écart
et produit [review.json](review.json). Ses dépendances immuables sont dans
[input_pins.json](input_pins.json) ; les octets de la note constructeur lus
à `ad7ffd28` y sont déclarés séparément comme contexte de contrelecture.

```bash
python3 -B morsehgp3D_v7/audits/receipts_incremental_review_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_incremental_review_20260911/verify.py
```

Le succès du lecteur porte sur l'intégrité des entrées et le recalcul des
volumes ; il n'exécute pas la gate métamorphique proposée. Le plan couvre
déjà la majeure partie des conditions de correction. Une implémentation
devra encore qualifier ses écritures et sa publication effective.
GCP non utilisé.
