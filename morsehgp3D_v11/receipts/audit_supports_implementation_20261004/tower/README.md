# S3 : journal, rattachement fermé et capacités

Lecture indépendante de l'acteur `build/v11-impl-s3`, HEAD
`f98aeed67d4030dd78e11d5faf7d8556c4d17aaf`. Les fichiers S3 étaient **WIP**,
capturés avant lecture : aucune qualification native héritée. L0 D.4 est
capturé séparément. Les métadonnées donnent les empreintes avant/après et
l'état Git de chaque acteur ; aucun fichier d'acteur n'a été modifié.

## Garde matérielle : nombre de traces vers u32

La copie [attachment.cpp](sources_before/morsehgp3D_v11/src/tower/attachment.cpp)
ligne 119 publie `static_cast<u32>(end - begin)` sans garde de capacité.
Le journal réserve des comptes `u64`. `build_order` n'impose pas une
coquille de 24 sites ; ce plafond appartient à la future sortie supports,
pas à FULL.

Témoin exact : les 150 sites entiers de `x²+y²+z²=225`, translatés de +15
sur les trois axes, restent distincts dans u21. La boule centrale a
`p=0`, `m=150`, `qmin=2` (antipodes) et appartient mathématiquement à Cat8.
Ses 69 sites de demi-espace `x>15` fournissent `C(69,8)=8 361 453 672`
traces strictes : leur enveloppe convexe exclut le centre, donc T2 donne
un rayon strictement inférieur. Ce **minorant** dépasse `UINT32_MAX`.

Si les allocations et le parcours sont admis, ce champ perd donc de
l'information. Ce reçu ne reproduit pas cette construction colossale :
un budget insuffisant ou l'allocateur peuvent la refuser auparavant.
Le cast est défini en C++, ce n'est pas une preuve d'UB ni de sortie
FULL géométriquement erronée. Correctif minimal : garde
`end - begin > UINT32_MAX -> tower_capacity` avant le cast ; alternativement
élargir ce champ. Ne pas plafonner la coquille de FULL à 24 pour ce motif.

## Points difficiles effectivement respectés par le WIP

- [forest_plateau.cpp](sources_before/morsehgp3D_v11/src/tower/forest_plateau.cpp)
  lignes 54–60 garde `initial_level < λ_b`, puis journalise la naissance
  rendue, jamais une racine DSU ou un sommet déjà remonté.
- `attachment.cpp` lignes 91–103 ne demande que `seed.birth < λ_b` ;
  le balayage est avancé au rang précédent, puis le parent de même rang
  donne le propriétaire **après tout le plateau**. Il ne teste pas la
  fausse borne `initial_level <= niveau précédent`.
- Les quatre cellules du plateau K5 précédemment prouvé ont toutes le
  même propriétaire final, chacune trois branches antérieures, 12 branches
  publiées au total et une fusion à six enfants. Leurs unions exécutées
  peuvent valoir `2,2,1,0`, sans changer leurs rôles. Le modèle teste les
  24 ordres de traitement. Les compteurs recomptés sont distincts du coût
  réel des descentes et des requêtes du balayage.
- L'attachement d'une continuation utilise le nœud vivant ; aucune
  naissance artificielle n'est créée. `build_order` appelle `finish()`.
  L'option `births_only` n'est pas une option de ce WIP ; elle relève de
  futures sorties, auxquelles cet attachement ne doit pas être appliqué.

Le cas D2 donne `41 < β(F)=64 < λ_b=1681/25` avec la graine ZW née à 1.
Cette graine se remonte au nœud antérieur 5 ; les trois branches fermées
aboutissent au nœud 6. La justification utilise T5 au niveau initial et
la constance de H₀ entre événements du catalogue, **pas** l'existence de
F à la coupe 41. Aucun défaut d'attachement n'est démontré ici.

## Réponse D.4 : le juge E2 aux événements faibles

Pour les boules positives, une naissance est forte. Une boule faible,
`K=p+qmin−1`, n'est jamais une naissance : les traces comprimées
`I∪A`, `|A|=qmin−1`, sont toutes strictes par minimalité de qmin et T2.
Cela ne s'étend pas à une K-partie quelconque de `P=I∪U`.

Sur la ligne `(0,0,0),(1,0,0),(2,0,0)`, K2, la boule AC de niveau 1 est
faible (`p=1,qmin=2`). AB et BC ont niveau 1/4 ; AC a niveau 1.
Le sélecteur historique « tout I puis les sites U nécessaires » reste
strict. Le [juge WIP](sources_before/morsehgp3D_v11/tests/tower/order_tree_support.hpp)
le conserve pour `first=true`. Pour `first=false`, une partie peut être
non stricte : sa garde `initial<=λ_b` et son `ancestor_closed` sont
corrects. Ne pas remplacer cette garde par `<`, ni journaliser ce témoin
comme une trace stricte. Les naissances de sites à K1 sont un cas séparé.

## Contrôles et limites

Commandes portables, dans ce dossier :

```sh
python3 -B -S check_contract.py
python3 -B -O -S check_contract.py
sha256sum -c SHA256SUMS
```

Les deux exécutions passent **462 gardes**, avec JSON identiques. Le modèle
est autonome stdlib/Fraction : aucun import, test ou exécutable produit.
Il teste un transport sur une forêt exacte donnée, une multifusion,
la ligne E2 et le minorant de capacité ; il n'est pas un nouveau constructeur
de FULL. Les gardes géométriques Gram/Γ de D2 (77) et du plateau (139)
restent dans leurs reçus clos, cités par `PREVIOUS_CLOSURES.json` et jamais
réécrits. L'identité native avec `build_full`, les injections mémoire,
les modes parallèles et les mutants S3 restent à qualifier sur G4.
