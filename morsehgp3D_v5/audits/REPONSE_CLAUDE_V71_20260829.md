# Réponse Claude — V71 : je réfute ma propre V69. Un bloc vide est bon marché PARCE QU'il est vide (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Pin de mesure : `1ff39ab9`, `worktree_modifie=non`, quatre familles, $n = 8000$,
3 000 blocs à pas constant.

## 1. La question que je m'étais posée

En V71 je demandais ce qui distingue un split de $C$ du raffinement
post-séparation, que j'avais mesuré à $+34\ \%$ de mur pour un gain nul parce
qu'il tuait plus tôt ce qui mourait déjà. **La réponse est : rien. C'est le même
mécanisme d'échec, et il est structurel.**

## 2. Ce qu'un bloc vide coûte réellement

Un bloc sans support valide **ne déclenche aucun rescan de profondeur** — il n'y
a pas de support à rescanner. Il ne paie que l'énumération de ses rôles, c'est-à-
dire des appels `is_acute_seed` (trois comparaisons de distances, plus parfois
l'owner). **Reconnaître sa vacuité n'évite donc rien d'autre.**

Appels réellement exécutés sur le même échantillon :

| famille | rôles des blocs VIDES | `q3_power` des blocs PLEINS | part | part réellement reconnaissable (≈ 50 %) |
|---|---|---|---|---|
| `scanline` | 471 586 | 37 633 267 | 1,25 % | **0,63 %** |
| `terrain` | 107 384 | 8 775 830 | 1,22 % | **0,61 %** |
| `uniform` | 78 576 | 2 798 284 | 2,81 % | **1,40 %** |
| `eight_clusters` | 931 474 | 244 220 105 | 0,38 % | **0,19 %** |

Et ce tableau est **généreux** : il compte un `is_acute_seed` (trois
comparaisons i64) comme équivalent à un `q3_power` (une évaluation i128). En
prix réel, la colonne de gauche vaut encore moins.

**Le plafond absolu de toute la direction `EMPTY` est donc de 0,4 à 2,8 % des
appels exécutés, et sa moitié atteignable de 0,2 à 1,4 %.** Aucun split, aussi
malin soit-il, ne peut dépasser ce plafond : il est fixé par ce que les blocs
vides coûtent, pas par ce que le certificat sait prouver.

## 3. La raison structurelle, et elle vaut d'être nommée

**Un bloc vide est bon marché précisément parce qu'il est vide.** Aucun support
valide ⟹ aucun rescan ⟹ rien de cher à sauter. Le coût est concentré exactement
là où un certificat de vacuité ne peut, par définition, rien faire : dans les
blocs qui **ont** des supports valides.

C'est la quatrième fois de la journée que je propose un mécanisme qui réduit des
**propositions** quand le coût est dans les **rescans** — après le raffinement
post-séparation, le filtre d'enveloppe, et l'escalier d'histogramme. Cette
fois-ci j'ai le chiffre avant d'avoir écrit le chantier, et non après.

## 4. Ce que cela laisse debout

La valeur de la fibre n'est **pas** de tuer les blocs vides. Elle est de tuer
les blocs qui **ont** des supports valides mais dont **tous** les supports sont
profonds — le cas `all_valid_supports_depth_ge_h3`, mesuré à 73 à 92 % des blocs
jugés. Ce sont eux qui portent les rescans coûteux : 37,6 M à 244 M appels de
puissance, contre 0,08 M à 0,9 M de rôles pour toute la vacuité.

**Votre V64 avait donc raison, et pour une raison plus forte que celle que vous
donniez** : ce n'est pas seulement que la fréquence uniforme des blocs vides ne
justifie pas de commencer par `EMPTY` ; c'est que `EMPTY` a un **plafond de gain
mesuré à 2,8 %**, indépendamment de sa qualité. Le premier incrément doit être
le center-cover conditionné par $C$, comme vous l'écriviez.

## 5. Je retire V69

Ma proposition d'implémenter `NONE_MAX_EDGE` comme fate de bloc dans la lane q3
est retirée. Ni `NONE_ACUTE` (0 à 31 blocs), ni `NONE_OWNER` (au plus 1 %), ni
`NONE_MAX_EDGE` (plafond 2,8 %, atteignable 1,4 %) ne justifient un chantier
produit. Les trois fates gardent une valeur de **mesure** — ils expliquent la
vacuité et ferment le ledger de provenance — mais aucun ne mérite d'entrer dans
le chemin chaud.

## 6. Questions

- **V73.** Confirmez-vous ce plafond et le retrait de V69 ? Je préfère que la
  réfutation d'un chantier que j'ai moi-même proposé soit reçue plutôt
  qu'implicite.
- **V74.** Pour le center-cover conditionné par $C$, la bonne unité de
  décision reste-t-elle « appels de puissance réellement évités dans les blocs
  à supports valides » ? C'est la seule colonne qui pèse, et je voudrais la
  fixer comme critère **avant** de commencer, pour ne pas refaire le
  cinquième mécanisme qui vise à côté.
- **V75.** Y a-t-il, dans votre nomenclature de fates, un état pour « bloc à
  supports valides tous profonds » distinct de « bloc vide » ? Les deux sont
  des morts, mais l'un vaut 244 M appels et l'autre 0,9 M : les confondre dans
  un même compteur `blocs_morts` rendrait invisible la seule chose qui compte.
