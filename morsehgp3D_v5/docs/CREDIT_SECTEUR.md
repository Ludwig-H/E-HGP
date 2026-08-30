# Le crédit d'extrémité paie aux secteurs, pas au fuseau

`phase=exploration_v5_hors_registre` `backend=cpu_reference`
`profile=quantized_u16_input_only` `mode=audit_independant_math_and_architecture`
`public_status=not_claimed`

Cette note énonce et démontre *où* le crédit d'extrémité $h_a(a) + h_b(b)$ peut
tuer une ancre, mesure le gain, et referme une demi-idée : l'union que j'avais
câblée au stage $W_q$ ne pouvait **pas** tuer une ancre de plus, et c'est
démontrable ; celle du stage secteur le peut, et pour une raison structurelle.

## 1. Les deux stages et le crédit

Une ancre $(a,b)$ d'un rectangle $A \times B$ meurt si l'on exhibe $h_q$ témoins
de profondeur valables pour *tous* ses porteurs. Trois sources s'additionnent
**si leurs domaines sont disjoints** :

- le **fuseau** $W_q(a,b)$, balayé sur le cover : $n$ témoins ;
- le **crédit d'extrémité** $\mathrm{base} = h_a(a) + h_b(b)$, acquis en amont
  par le grand-livre de ligne : témoins de $A \setminus \lbrace a \rbrace$
  universels sur $\mathrm{Box}(B)$, et symétriquement ;
- les **secteurs** ($K = 8$) : par secteur angulaire $k$ du porteur,
  $\mathrm{cnt}[k]$ témoins de la boule diamétrale ouverte, le verdict étant
  $\min_{k} \mathrm{cnt}[k] \geq h_q$.

## 2. Théorème (le crédit ne tue rien au stage fuseau)

**Énoncé.** Soit $n = n_{\mathrm{in}} + n_{\mathrm{out}}$ le partage des témoins
de $W_q(a,b)$ selon leur appartenance à $A \cup B$. Alors
$n_{\mathrm{out}} + \mathrm{base} \leq n$ ; la forme d'union
$\max ( n, n_{\mathrm{out}} + \mathrm{base} ) \geq h_q$ décide donc exactement
comme $n \geq h_q$.

**Preuve.** $h_a(a)$ compte des points de $A \setminus \lbrace a \rbrace$
universels sur $\mathrm{Box}(B)$, donc en particulier dans $W_q(a,b)$ : c'est un
sous-ensemble de $W_q \cap (A \setminus \lbrace a \rbrace)$, d'où
$h_a(a) \leq \lvert W_q \cap (A \setminus \lbrace a \rbrace) \rvert$, et de même
pour $h_b(b)$. Les boîtes $A$ et $B$ d'un rectangle WSPD sont disjointes, donc
ces deux majorations portent sur des parties disjointes de $n_{\mathrm{in}}$ :
$\mathrm{base} \leq n_{\mathrm{in}}$. En ajoutant $n_{\mathrm{out}}$,
$n_{\mathrm{out}} + \mathrm{base} \leq n_{\mathrm{out}} + n_{\mathrm{in}} = n$.
$\square$

**Ce que l'union y gagne quand même.** $\mathrm{base}$ est connu *avant* le
balayage, tandis que $n_{\mathrm{in}}$ ne s'accumule qu'au fur et à mesure : à
mi-balayage, $n_{\mathrm{out}} + \mathrm{base}$ peut dépasser $n$. L'union
achète donc une **sortie anticipée**, jamais une mort de plus. La mesure le
confirme à l'unité près : `ancres_w4` vaut $138\,565$ sur `terrain` $n = 8000$
avant comme après le câblage.

## 3. Pourquoi les secteurs échappent au théorème

La preuve du § 2 repose sur $\mathrm{base} \leq n_{\mathrm{in}}$, c'est-à-dire
sur le fait que les témoins du crédit sont **déjà comptés** par le balayage. Au
stage secteur, $\mathrm{cnt}[k]$ ne compte que les témoins *du secteur $k$*,
alors que le crédit est **universel sur tous les secteurs** — un point de
$W_q(a,b)$ est témoin de tout seed de l'ancre, quel que soit le porteur. Un
témoin du crédit peut donc être absent de $\mathrm{cnt}[k]$, et la majoration
tombe. Le verdict devient

$\min_{k} \max ( \mathrm{cnt}[k],\ \mathrm{cnt\_hors}[k] + \mathrm{base} ) \geq h_q,$

où $\mathrm{cnt\_hors}[k]$ ne retient que les témoins de secteur pris hors de
$A \cup B$, disjoints du crédit par localisation. La forme domine les deux
prises séparément.

**Le maximum se prend secteur par secteur, avant le minimum.** La forme globale
$\max ( \min_{k} \mathrm{cnt}[k],\ \min_{k} ( \mathrm{cnt\_hors}[k] + \mathrm{base} ) )$
est strictement plus faible : elle rate les ancres dont certains secteurs sont
fermés par le compte pur et les autres par le crédit. L'audit du 30 août a
relevé que mon premier code calculait cette forme globale alors que la note
annonçait la forme par secteur ; il a fourni la configuration qui les sépare,
gravée en `tests/sector_credit_crossed_fixture.cpp` :

```text
a = (0,1000,1000)  b = (2000,1000,1000)  D² = 4 000 000  h = 2
e = (10,990,990)   i = (10,910,910)      o = (10,1020,1020)
A = {a,e,i}  B = {b}  base = 1  cover = {e,i,o}
cnt      = [1,2,2,2,2,1,1,1]
cnt_hors = [1,0,0,0,0,1,1,1]
```

Chaque secteur atteint $h = 2$ par l'une des deux branches, jamais la même ; les
deux minima globaux valent $1$. La forme par secteur tue, la forme globale non.
Mutant `sector-credit-global`, porte `mhgp5_sector_credit_mutant_global`.

## 4. Mesure (`s = 8`, graine 3, lane q4)

Ancres tuées par le stage secteur, avant et après le câblage du crédit :

| famille | $n$ | secteurs avant | secteurs après | gain du stage | gain total des morts |
|---|---:|---:|---:|---:|---:|
| `terrain` | 8000 | 10 798 | 11 623 | +7,6 % | +0,53 % |
| `terrain` | 32000 | 51 997 | 57 385 | +10,4 % | +0,28 % |
| `uniform` | 8000 | 241 185 | 243 613 | +1,0 % | +0,41 % |

Le gain sur les morts totales est **modeste** : le stage secteur ne portait déjà
que $9{,}5$ % à $69$ % des morts au-delà de $W_4$, et le crédit n'en améliore
qu'une fraction.

**Ce que ces chiffres n'établissent pas.** Ils ont été pris avant la correction
de la forme (§ 3) et avant le raccord des routes par lots : ils ne sont donc
pas attribuables au contrat exact, et sont à reprendre. Surtout, ils ne
suffisent pas à dire « gratuit » : le code ajoute une classification et jusqu'à
huit compteurs par site, et une petite population d'ancres peut concentrer une
grande part des balayages longs. Ce qui est acquis est plus étroit et plus
solide : le crédit est **sûr** (disjonction démontrée) et **monotone** (il ne
retire jamais une mort). La mesure qui trancherait le coût est appariée par
`AnchorKey` — morts nouvelles dues au crédit, somme de `q4_core_site_tests`
évitée, essais évités, temps exclusif et pic mémoire — et elle n'est pas faite.

## 5. Ce que cette note ne dit pas

Elle ne change **pas** l'exposant de la lane q4 : les morts d'ancre gagnées sont
de l'ordre du demi-pour-cent, très loin du facteur qu'il faudrait pour déplacer
une pente. Le mur de q4 reste le balayage du cœur par seed
(`docs/Q4_MUR_UNITE.md`), et aucune mesure ici ne le concerne.
