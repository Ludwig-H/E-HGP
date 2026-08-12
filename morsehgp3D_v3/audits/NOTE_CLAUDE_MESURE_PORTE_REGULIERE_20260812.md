# Note corrigée — extra-shells observées sur les familles cibles

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note apporte un diagnostic borné à la question Q1 de
[`QUESTIONS_CLAUDE_ROUTE_SPARSE_20260812.md`](QUESTIONS_CLAUDE_ROUTE_SPARSE_20260812.md) :
la porte régulière de la section 2 de
[`AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md`](AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md)
tient-elle sur le régime LiDAR ? Elle corrige aussi l'interprétation initiale
du compteur : il détecte une **extra-shell par rapport au support choisi**, pas
la non-unicité du support minimal. Elle ne prononce aucune admission.

## 1. Ce qui a été mesuré, et comment

La porte exige notamment un **support minimal `U(Q)` unique et essentiel** et
aucun label extérieur exactement sur la frontière. Le compteur implémenté teste
seulement le second fait : il existe un point hors du support choisi, exactement
sur la sphère de sa miniboule. Le prédicat est exact et entier dans les trois
arités :

- q2 : `(z-x) . (z-y) = 0` ;
- q3 : `D ||w||^2 = 2 (Na (w.u) + Nb (w.v))` avec `w = z-a` ;
- q4 : le déterminant InSphere développé vaut zéro.

Le sujet est `prototype/certified_locality_probe.cpp`, `--mode=arity`. À
`n=70`, le juge retrouve les mêmes **cardinalités** `q2/q3/q4 = 681/884/202`
pour `K=4`. Il partage les constructeurs et prédicats de sphère du générateur
et ne compare alors ni intérieurs, ni shells, ni `BallKey`; ce vert borné ne
reçoit donc pas les fractions à `n=1 500`.

## 2. Résultat

Fraction des enregistrements émis dont un point hors support est exactement sur
la sphère, à `n = 1 500`, `K = 10`, fenêtre de support 48 et fenêtre de
voisinage complète :

| famille | q2 | q3 | q4 | ensemble |
| --- | ---: | ---: | ---: | ---: |
| `terrain` | 13,712 % | 0,190 % | 0,815 % | **4,172 %** |
| `scanline_single_pass` | 32,677 % | 2,876 % | 5,092 % | **11,478 %** |
| `scanline_overlap_multiecho` | 39,129 % | 1,548 % | 6,875 % | **11,390 %** |

La campagne d'origine ne conserve ni CLI et seed complets, ni empreintes du
sujet et du binaire, ni journal brut. À `n=1 500`, le juge exhaustif borné à
400 points est absent et la fenêtre q3/q4 de 48 n'est pas certifiée saturée.
Ces nombres restent donc des observations diagnostiques non reçues, pas des
fractions exactes des familles.

## 3. Lecture

Ces observations falsifient l'hypothèse « aucune extra-shell parmi les records
émis » sur une fraction non négligeable du régime cible. Elles ne mesurent ni la
fraction de boules distinctes, ni celle des cofaces directes, ni celle des
supports minimaux multiples. Les familles sont construites avec un sol plat, un
jitter entier `{0,1,2}`, des pas de balayage entiers `2` et `8` et des
multi-échos verticaux quantifiés : les égalités observées sont cohérentes avec
ce régime quantifié, mais une graine à `n=1 500` ne fournit aucune loi à 50 k.

Trois observations qui orientent la suite :

1. **L'extra-shell observée est très inégale entre arités.** Elle est fréquente
   en q2 (13 % à 39 %) et plus rare dans les q3/q4 émis. Ce sont des fractions
   de records de support, pas des coûts de repli ni des fractions de boules.
2. **Le support choisi reste valide.** Pour une paire `(x,y)` avec un `z`
   exactement sur la sphère diamétrale, `{x,y}` peut rester l'unique support
   minimal positif de cette boule. Le point `z` prouve alors seulement
   `E\U != empty`, où `E` est le shell fermé global. L'extra-shell n'implique
   donc ni un « support maximal » distinct, ni plusieurs supports minimaux.
3. **L'identité de boule reste nécessaire mais ne suffit pas.** Deux supports
   peuvent désigner la même boule et une seule boule dégénérée peut porter un
   plateau combinatoire. Les comptes publiés dénombrent des supports proposés,
   pas des `BallKey`, des saturés ou des cofaces directes reçues.

## 4. Ce que cela demande à la route sparse

Je ne propose pas de solution ici ; je fixe la question que la réparation doit
trancher.

Une clé canonique de **boule** — et non de support — est nécessaire avant la
déduplication sémantique et le fold. Elle doit être suivie d'un census fermé qui
sépare les intérieurs stricts `I`, le shell complet `E` et les supports minimaux
positifs réellement exhibés. Grouper seulement par `BallKey` ne contracte pas
encore le plateau et ne prouve aucune composante stricte.

La réponse à Q1.3 est négative pour le pivot proposé : choisir `u_0` dans
l'union de plusieurs supports ne garantit pas une baisse stricte, car un autre
support peut rester entier après la suppression. Une extra-shell ou un support
multiple de rang pertinent exige un quotient atomique dédié ou un refus fermé.
Au-dessus de la fenêtre utile, le théorème 4.2 d'inertie saturée permet en
revanche une tombstone H0 sans prétendre que la boule est régulière.

## 5. Portée de cette mesure

Elle porte sur `n = 1 500`, une graine, les trois familles structurées listées,
`K = 10`, et sur les supports effectivement émis avec une fenêtre q3/q4 de 48.
La liste de points inspectée pour le shell de chacun de ces records était
globale; c'est l'univers des supports q3/q4 proposés par la fenêtre 48 qui n'est
pas certifié complet à cette taille. La note ne dit rien de la fraction à 50 k
ou de familles réelles et ne valide aucune politique de quotient. Les secondes
de la campagne ne sont pas publiées : ce ne sont pas des mesures de performance.

GCP non utilisé pour cette note.
