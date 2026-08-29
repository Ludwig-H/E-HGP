# Réponse Claude — V74 : le plafond du center-cover est 95 à 99 % du travail résiduel, deux ordres de grandeur au-dessus de tout le reste (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Pin de mesure : `1ff39ab9`, `worktree_modifie=non`, quatre familles, $n = 8000$,
3 000 blocs à pas constant.

## 1. La méthode, avant le chiffre

Mesurer le **plafond** d'une direction avant de la construire vient de me faire
retirer deux chantiers en une journée (`EMPTY` en V71, l'escalier en V57). Je
l'applique donc au center-cover **avant** d'écrire une ligne.

Les appels de puissance des blocs à supports valides se répartissent en trois
seaux **disjoints** :

1. **déjà tués par $W_3$** — la production les capte, un certificat de bloc n'y
   gagnerait rien ;
2. **tous profonds mais NON tués par $W_3$** — exactement le gain marginal
   qu'un center-cover conditionné par $C$ pourrait viser ;
3. **inhérent** — un support survit, donc du travail que rien ne peut éviter.

## 2. Le plafond

| famille | déjà tués par $W_3$ | **gain marginal** | inhérent |
|---|---|---|---|
| `scanline` | 28 670 848 (76,2 %) | **8 811 644 (23,4 %)** | 150 775 (0,4 %) |
| `terrain` | 6 385 839 (72,8 %) | **2 331 573 (26,6 %)** | 58 418 (0,7 %) |
| `uniform` | 1 471 068 (52,6 %) | **1 266 952 (45,3 %)** | 60 264 (2,2 %) |
| `eight_clusters` | 164 104 748 (67,2 %) | **79 162 964 (32,4 %)** | 952 393 (0,4 %) |

Et la lecture qui décide. En production, une ancre tuée par $W_3$ **ne scanne
jamais** : le seau 1 n'est pas payé. Le travail que la production paie
réellement dans ces blocs est donc seau 2 + seau 3, et sa part évitable est :

| famille | résiduel réellement payé | dont évitable en principe | part |
|---|---|---|---|
| `scanline` | 8 962 419 | 8 811 644 | **98,3 %** |
| `terrain` | 2 389 991 | 2 331 573 | **97,6 %** |
| `uniform` | 1 327 216 | 1 266 952 | **95,5 %** |
| `eight_clusters` | 80 115 357 | 79 162 964 | **98,8 %** |

**De 95,5 % à 98,8 % du travail de profondeur que la production paie encore se
trouve dans des blocs dont TOUS les supports sont profonds** — donc tuables en
principe par un certificat de bloc. Le travail vraiment inhérent, celui qu'aucun
certificat ne peut éviter parce qu'un support survit, ne pèse que 0,4 à 2,2 %.

## 3. Pourquoi ce chiffre change la nature de la discussion

Comparaison des plafonds mesurés aujourd'hui, tous en appels réellement
exécutés, tous sur le même échantillon :

| direction | plafond |
|---|---|
| raffinement post-séparation | 0 % (mesuré : compteurs aval bit-identiques) |
| filtre d'enveloppe | rendement 0,38 à 1,44 en tests, pas de gain d'exposant |
| escalier d'histogramme | attaque une boucle en $n^{1{,}22}$ sur `terrain` |
| fates `EMPTY` de bloc | **0,4 à 2,8 %**, moitié atteignable |
| **center-cover conditionné par $C$** | **95,5 à 98,8 %** |

Deux ordres de grandeur séparent la dernière ligne des précédentes. C'est la
première direction de la journée dont le plafond justifie un chantier, et cela
confirme votre V64 par la mesure et non par l'autorité.

## 4. Le critère de décision que je propose de figer maintenant

Pour ne pas produire un cinquième mécanisme qui vise à côté, je propose de
**fixer le critère avant de commencer** :

> unité = **appels de puissance réellement évités dans les blocs à supports
> valides non tués par $W_3$**, comptés par étage, avec sorties anticipées, et
> le coût du certificateur dans un compteur séparé ; jamais converti en temps.

Un center-cover conditionné par $C$ sera jugé sur cette seule colonne. S'il
capture $X$ des 8,8 M / 2,3 M / 1,3 M / 79,2 M appels marginaux, c'est $X$ qui
compte — ni le nombre de blocs classés, ni la part de vacuité reconnue, ni un
majorant statique.

## 5. Ce que le plafond ne dit pas

- **C'est un plafond, pas un gain.** Il suppose un certificat parfait ; le
  vôtre perdra les couplages (trois médiatrices séparées, coplanarité ignorée
  en q3), donc capturera strictement moins.
- **Le coût du certificat n'est pas dedans.** Un certificat qui capturerait
  90 % du plafond en coûtant plus que ce qu'il évite serait une perte.
- **Une taille, un échantillon, pas de reçu**, et la brute force n'est pas le
  chemin de production : histogramme, secteurs et grille ont déjà retiré une
  part que cette sonde ne modélise pas. Le plafond est donc **relatif**, pas
  absolu.

## 6. Questions

- **V76.** Acceptez-vous ce plafond et ce critère comme cadre de réception du
  center-cover conditionné par $C$, avant que j'écrive quoi que ce soit ?
- **V77.** Le seau « inhérent » vaut 0,4 à 2,2 %. Cela signifie que presque
  tous les blocs à supports valides sont **entièrement** profonds, et que les
  blocs mixtes — un support survit, les autres meurent — sont rares. Est-ce que
  cela simplifie votre construction par patches, qui doit précisément gérer le
  cas où des patches différents meurent par des témoins différents ?
- **V78.** `uniform` se distingue nettement : seau 1 à 52,6 % contre 67 à 76 %
  ailleurs, donc gain marginal presque double. Est-ce un signal que le
  center-cover conditionné par $C$ rapporterait le plus là où $W_3$ est le plus
  faible, ou un artefact de la petite masse de `uniform` dans cet échantillon ?
