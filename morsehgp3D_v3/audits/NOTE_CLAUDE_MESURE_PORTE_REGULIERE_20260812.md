# Note de Claude — la porte régulière, mesurée sur les familles cibles

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note répond par la **mesure** à la question Q1 de
[`QUESTIONS_CLAUDE_ROUTE_SPARSE_20260812.md`](QUESTIONS_CLAUDE_ROUTE_SPARSE_20260812.md) :
la porte régulière de la section 2 de
[`AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md`](AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md)
tient-elle sur le régime LiDAR ? Elle ne prononce aucune admission.

## 1. Ce qui a été mesuré, et comment

La porte exige notamment un **support minimal `U(Q)` unique et essentiel** et
aucun label extérieur exactement sur la frontière. La violation la plus directe
est donc : il existe un point hors du support, exactement sur la sphère de la
miniboule. Le prédicat est exact et entier dans les trois arités :

- q2 : `(z-x) . (z-y) = 0` ;
- q3 : `D ||w||^2 = 2 (Na (w.u) + Nb (w.v))` avec `w = z-a` ;
- q4 : le déterminant InSphere développé vaut zéro.

Le sujet est `prototype/certified_locality_probe.cpp`, `--mode=arity`, dont la
génération locale est vérifiée **égale au juge exhaustif des trois arités**
(`q2/q3/q4 = 681/884/202` à `n = 70`, `K = 4`).

## 2. Résultat

Fraction des enregistrements dont un point hors support est exactement sur la
sphère — donc dont le support minimal n'est **pas unique** — à `n = 1 500`,
`K = 10`, fenêtre de support 48, fenêtre de voisinage complète :

| famille | q2 | q3 | q4 | ensemble |
| --- | ---: | ---: | ---: | ---: |
| `terrain` | 13,712 % | 0,190 % | 0,815 % | **4,172 %** |
| `scanline_single_pass` | 32,677 % | 2,876 % | 5,092 % | **11,478 %** |
| `scanline_overlap_multiecho` | 39,129 % | 1,548 % | 6,875 % | **11,390 %** |

## 3. Lecture

La porte régulière **échoue sur une fraction non négligeable du régime cible**,
et cette fraction n'est pas un artefact : les familles sont construites avec un
sol plat, un jitter entier `{0,1,2}`, des pas de balayage entiers `2` et `8` et
des multi-échos verticaux quantifiés. Sur une grille u16, les angles droits et
les configurations cosphériques y sont structurels, pas accidentels.

Trois observations qui orientent la suite :

1. **La dégénérescence est très inégale entre arités.** Elle est massive en q2
   (13 % à 39 %) et faible en q3 (0,19 % à 2,88 %), qui porte pourtant les deux
   tiers du volume. Une politique qui refuserait la coface entière paierait donc
   surtout sur la lane la moins volumineuse.
2. **Ce n'est pas un support « faux », c'est un support non maximal.** Pour une
   paire `(x,y)` avec un `z` exactement sur la sphère diamétrale, la boule
   diamétrale reste bien la miniboule de `{x,y}` : le support propre positif
   `{x,y}` est valide. Ce qui n'est pas unique, c'est le support **maximal** de
   la même boule. C'est exactement la distinction `q_min` / `q_cert` de la
   proposition.
3. **La conséquence est donc une question d'identité de boule, pas de
   validité.** Plusieurs supports propres positifs distincts désignent la même
   boule, donc plusieurs cofaces directes distinctes ont la même miniboule. Mes
   comptes de sortie dénombrent des **supports**, pas des **boules** : ils
   majorent le nombre de boules de la fraction ci-dessus.

## 4. Ce que cela demande à la route sparse

Je ne propose pas de solution ici ; je fixe la question que la réparation doit
trancher.

Une clé canonique de **boule** — et non de support — semble nécessaire avant la
déduplication des bras stricts et avant le fold : sinon deux bras issus de deux
supports de la même boule seraient comptés deux fois, ou pire, rattachés à deux
composantes distinctes au même niveau exact.

La question Q1.3 de ma note reste donc ouverte et devient prioritaire :
existe-t-il une variante de l'étoile silencieuse exacte lorsque le support
minimal est **multiple**, par exemple en choisissant `u_0` canonique dans
l'union des supports minimaux plutôt que dans un support unique ?

## 5. Portée de cette mesure

Elle porte sur `n = 1 500`, une graine, quatre familles, `K = 10`. Elle ne dit
rien de la fraction à 50 k, ni des familles réelles, ni d'une politique de
quotient. Les secondes de la campagne ne sont pas publiées : ce ne sont pas des
mesures de performance.

GCP non utilisé pour cette note.
