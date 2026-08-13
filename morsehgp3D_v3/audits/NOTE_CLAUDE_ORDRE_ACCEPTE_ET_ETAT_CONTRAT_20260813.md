# Note de Claude — j'accepte votre ordre, et voici l'état exact du contrat

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé pour cette note.

## 1. J'acte votre ordre sans le contourner

Vous placez `ProjectiveWindowCounter-v0` **avant** le join factorisé
`QueryTree x PointTree`, et avant tout nouveau code shallow ou CUDA. Je m'y
tiens, alors même que ma dernière mesure désigne le join comme le seul levier
restant. C'est exactement le raccourci que vous m'avez repris quatre fois
aujourd'hui, dont deux fausses fermetures que mes propres portes ne voyaient
pas.

## 2. Ce que j'ai établi contre moi depuis votre dernière réponse

**Le profil du travail.** Sur `30 422 095` classifications de `Central-VWave`
(`uniform`, `n=8 000`, `s=2`) :

| verdict | nombre | part |
| --- | ---: | ---: |
| `ALL` — crédite | `1 707 543` | `5,6 %` |
| `CENTRAL_DEAD` — élague | `15 727 458` | `51,7 %` |
| descente pure | `12 987 094` | `42,7 %` |

**`94,4 %` du travail ne crédite rien**, et la descente repart de la racine pour
chaque rectangle alors que les nœuds créditeurs sont tous autour de `m_0`.

**Le levier local, essayé et chiffré.** `--climb` repère la feuille de `m_0`
par sa clé de Morton puis remonte, empilant à chaque ancêtre le sous-arbre
frère. Il réduit la descente pure de `30 %` — l'effet visé exactement — mais
augmente les `CENTRAL_DEAD` de `8 %`, pour un **gain net de neuf pour cent**.

La raison est instructive et je ne l'avais pas anticipée : en remontant, les
frères proches de la racine sont d'énormes sous-arbres contenant à la fois des
témoins et des non-témoins, donc `MIXED`, donc scindés. Ils ne se paient pas en
un test.

**Conclusion que j'acte :** l'optimisation locale de la descente par rectangle
ne donnera pas le facteur deux à cinq manquant. Il ne peut venir que du partage
entre rectangles. J'ai essayé le levier à ma portée avant de le dire.

## 3. L'état du contrat, sans enjolivement

`n=50 000`, mono-thread, `Central-VWave` corrigée :

| `s` | front/pt | masse q2 fermée | arbre | vague |
| ---: | ---: | ---: | ---: | ---: |
| `1` | `26,8` | `27,22 %` | `14,2` ms | `7 931` ms |
| `2` | `60,8` | `85,01 %` | `9,4` ms | `28 490` ms |

L'arbre est négligeable. Tout le temps est dans la vague : `257` M
recertifications à `s=2`, soit `111` ns chacune — quatre fois le coût
arithmétique d'un `rect_s_interval`, donc dominé par les accès mémoire.

Il faut `200` ms sur device pour un facteur `142`. Un kernel de cette forme
donne couramment `30` à `60` sur G4, donc **il manque un facteur `2` à `5` qui
doit venir de l'algorithme**. C'est une estimation d'ingénierie et je ne la
présente pas autrement.

## 4. Vos trois manques, réparés

Aucun CTest n'appelait `--vwave` : un juge exhaustif est gravé — toute fermeture
affirme dix `PointId` **distincts** satisfaisant le masque central sur tout
`A x B`, vérifié par balayage du nuage entier, sans emprunter ni l'intervalle du
score ni l'antichaîne. Référence `faux=0` sur `7 862` fermetures jugées.

Le mutant du masque global est gravé et meurt en code 4 : `2 055` fausses
fermetures sur `9 917` jugées.

Mon commentaire « sérialise une continuation » était **faux** : le code
dénombrait. Il le dit maintenant, `tronques` est imprimé, et la note précise que
seule la complétude est perdue — jamais la sûreté.

## 5. Ce que je vous demande pour tenir votre ordre

`ProjectiveWindowCounter-v0` demande `\sum_a\lvert N_q(a)\rvert` et
`\max_a\lvert N_q(a)\rvert`. Le probe cellulaire existe avec ses `432` cellules,
ses portes, ses mutants et son juge exhaustif sur `37 752` pools — mais il ne
publie **pas** de fenêtre `N_q(a)`, et je ne veux pas en inventer la définition.

1. `N_q(a)` est-il l'ensemble des `b` tels que `(a,b)` survit aux crédits
   directionnels avec `a` comme plus petit `PointId`, ou l'ensemble des sommets
   admissibles d'un support dont `a` est l'ancre ? Les deux se mesurent, mais
   ils ne se réfutent pas au même seuil.
2. Vous demandez de commencer par les `48` chambres simpliciales signées, les
   `432` cellules étant une ablation de rappel. Mon probe est bâti sur les
   `432` : dois-je en dériver les `48` par union de neuf triangles géodésiques,
   ou reconstruire la classification signée indépendamment pour que l'oracle
   reste un juge ?

## 6. Non-claims

Aucun `p95` device, aucun octet, aucun high-water, aucune tranche
`SupportKey -> BallKey -> census -> fold`. Le join factorisé n'est pas
implémenté et ne le sera pas avant votre compteur. Le contrat `50 000` reste
entièrement ouvert et G4 reste NO-GO.
