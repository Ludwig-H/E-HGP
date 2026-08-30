# Réponse de Claude — V155 : le mur n'est pas q3, c'est q4 — 53,6 % du temps et une pente de 1,98

- **Ancrage :** pin `f930ae78`. Extraction de la ligne `temps_ms` des reçus
  `masses_q3_seed{3}_20260829` — cible **produit** `mhgp5`, cinq tailles, quatre
  cohortes, `statut=complete`. Aucun run neuf : ces temps étaient déjà gravés et
  personne ne les avait décomposés.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Les parts du mur, à $n = 32\,000$

| cohorte | mur | descente q3 | **lane q3** | descente q4 | **lane q4** | census | fold |
|---|---:|---:|---:|---:|---:|---:|---:|
| `terrain` | 218 s | 6,7 % | **9,4 %** | 7,7 % | **53,6 %** | 5,3 % | 8,4 % |
| `scanline_single_pass` | 304 s | 5,0 % | **14,1 %** | 5,5 % | **50,6 %** | 6,4 % | 8,2 % |
| `eight_clusters` | 969 s | 8,3 % | **12,8 %** | 9,5 % | **24,9 %** | 14,9 % | 17,3 % |
| `uniform` | 655 s | 14,5 % | **4,9 %** | 17,8 % | **10,3 %** | 14,0 % | 19,7 % |

## Les pentes, $2\,000 \to 32\,000$

| cohorte | descente q3 | **lane q3** | descente q4 | **lane q4** | census | fold | **total** |
|---|---:|---:|---:|---:|---:|---:|---:|
| `terrain` | 1,19 | 1,54 | 1,21 | **1,98** | 1,11 | 1,18 | **1,44** |
| `scanline` | 1,29 | 1,64 | 1,30 | **1,81** | 1,31 | 1,26 | **1,52** |
| `eight_clusters` | 1,32 | 1,37 | 1,36 | 1,35 | 1,32 | 1,33 | 1,32 |
| `uniform` | 1,19 | 1,11 | 1,25 | 1,12 | 1,16 | 1,20 | 1,18 |

## Ce que cela dit

1. **La lane q4 est le mur** : $53{,}6\,\%$ du temps sur `terrain` et $50{,}6\,\%$
   sur `scanline` à $n=32\,000$, avec des pentes de $1{,}98$ et $1{,}81$ — donc
   essentiellement **quadratique** sur `terrain`.
2. **La lane q3 est minoritaire** : $4{,}9$ à $14{,}1\,\%$ du mur. Sa pente
   ($1{,}54$ sur `terrain`) est nettement inférieure à celle de ses seeds
   ($1{,}75$), parce que les postes par ancre et par rectangle, eux, sont
   quasi linéaires.
3. **Les descentes WSPD sont saines** : $1{,}19$ à $1{,}36$ partout, et leur part
   décroît avec $n$ sur les cohortes pathologiques.
4. **`census` et `fold` sont linéaires** ($1{,}11$ à $1{,}33$) et ne sont pas un
   problème d'échelle.
5. `uniform` n'a **aucune** pathologie : toutes les pentes entre $1{,}11$ et
   $1{,}25$.

## Ce que j'en retire pour moi-même

J'ai passé la session sur q3, sur consigne explicite de l'utilisateur, et le
travail a produit des résultats réels — la cascade intégrée, les trois classes
disjointes, l'union qui domine la forme résiduelle, la correction du dénominateur
($100\,\%$ des morts exigent $h_a+h_b$), et deux pistes de coût ouvertes puis
fermées. Mais **aucun de ces gains ne pouvait déplacer le mur**, parce que la
lane q3 ne pèse que $9{,}4\,\%$ sur `terrain`. Il fallait le dire, et le voici
chiffré.

Le fait n'est pas neuf en substance — j'avais mesuré en début de session que q4
portait $57$ à $83\,\%$ du temps — mais il s'était perdu dans la conversation, et
il n'était pas adossé à des reçus. Il l'est maintenant, aux cinq tailles.

## Question

- **V155.** La lane q4 croît en $n^{1{,}98}$ sur `terrain` et $n^{1{,}81}$ sur
  `scanline`, contre $n^{1{,}12}$ sur `uniform`. Les mêmes trois crédits
  ($h_{\mathrm{coeur}}$, $h_a$, $h_b$) y sont appliqués, avec
  $\kappa_4 = \sin 15^\circ$ au lieu de $\kappa_3 = 1/(2\sqrt{3})$ et un citron
  plus étroit ($125{,}26$ degrés au lieu de $120$). Avant que je n'y porte le
  même travail : la cascade que vous venez de spécifier vaut-elle telle quelle
  pour q4, ou son terminal axial et sa complétion changent-ils la structure des
  domaines disjoints ?
