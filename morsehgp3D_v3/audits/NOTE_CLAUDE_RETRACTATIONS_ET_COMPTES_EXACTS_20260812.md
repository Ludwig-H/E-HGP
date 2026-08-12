# Note de Claude — rétractations, rampe gelée et comptes exacts du graphe

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 1. Faute de méthode reconnue : rampe sur binaire mouvant

L'audit a raison et la faute est entière. J'ai reconstruit
`mhgp3v_centre_cell` **pendant** la campagne d'échelle. Le cas `25 000` s'est
terminé sur un inode supprimé et le cas `50 000` a démarré sur un ELF différent
de celui annoncé dans l'en-tête du reçu. Le fichier
`receipts/centre_cell_scale_20260812/scale_counters_raw.txt` est donc
**irrecevable** et je l'ai supprimé plutôt que de le laisser induire en erreur.

La campagne est relancée sur un binaire **gelé hors de l'arbre de build**, avec
l'empreinte de l'ELF revalidée **avant et après chaque cas** et un champ
`identique=oui|NON` par cas. Le nouveau reçu est
`scale_counters_frozen.txt`, contrat `CentreCellScaleReceipt-v2`.

## 2. Rétractation : la déduplication par lot n'est pas prouvée locale

Mon commit `64cf6fe` conclut que le RLE `SupportKey` n'a pas besoin d'être
global. **Je retire ce titre.** Il s'appuyait sur un seul point de laboratoire,
`terrain, n=400`, sans transcript archivé et sur un ELF depuis remplacé.

L'audit exhibe deux contre-mesures qui suffisent à interdire l'extrapolation :
sur `uniform, n=5`, le gain local à profondeur quatre vaut `1,007`; sur
`uniform, n=25`, le gain global vaut `3,960` mais les gains locaux tombent à
`2,276/1,575/1,130` aux profondeurs `1/2/3`. Il donne aussi la raison
géométrique : le plan bissecteur d'une paire diamétrale traverse un nombre
quadratique de cubes par niveau dyadique, et rien ne le retire des listes quand
`n<smax-1`.

L'énoncé défendable est donc seulement : **pour une antichaîne fixée de
sous-arbres, la déduplication locale reste exacte** — le lot qui contient la
feuille propriétaire conserve le support pertinent, les autres constatent zéro
propriétaire — mais elle ne possède **aucune borne de parcimonie**, et son gain
dépend de la famille et de la taille.

Je retire également la borne « facteur 42 ». Le tableau contient
`263 825` clés distinctes non dégénérées, donc une géométrie par `SupportKey`
ramènerait `2 220 024` occurrences à `263 826..268 632` solves : un facteur
`8,26..8,41`.

## 3. Mesure : le lift n'est pas le goulot sur ce backend

Le lift différé est implémenté et **jugé** : il passe l'accord d'identités sur
`uniform`, `terrain` et la grille gravée, y compris avec un lot minuscule de
`1 024` enregistrements qui force de nombreuses vidanges.

Sur `terrain, n=1 500`, il divise les lifts par `5,345` — `7 820 379` vers
`1 462 984` — et rend exactement les mêmes `98 752` supports. Il coûte pourtant
`13 %` de temps `user` de plus : `4,345 s` contre `3,832 s`.

La conclusion est utile et je la garde : **sur ce backend, le lift n'est pas la
dépense dominante; l'énumération et le trafic mémoire le sont.** Un tuple qui
atteint le lift coûte de l'ordre du dixième de ce que coûte l'ensemble du
parcours qui l'a produit. La variante reste donc `--deferred-lift`, désactivée
par défaut, exactement comme le filtre d'axe.

## 4. Comptes exacts du graphe de bissecteurs

Le critère de split employait le potentiel d'intervalles, majorant grossier d'un
surgraphe. C'est lui qui rendait la pente des cellules rouge : près de la racine
il explose et force à découper là où le vrai graphe est creux.

Les compteurs sont maintenant **exacts** et emploient chacun sa coupe de lane :
`E_2` sur `D_9`, `T_3` sur `D_8`, `T_4` et `Q_4` sur `D_7`. Les bitsets étant
orientés par identifiant, `popcount(N+(i) inter N+(j))` compte chaque triangle
une fois et `popcount(N+(i) inter N+(j) inter N+(k))` chaque `K4` une fois.
Comme `D_7` est la plus courte des trois listes, le compte exact des `K4` y est
bon marché et remplace tout coefficient empirique.

La borne d'incidence de l'audit, `4 Q_4 <= (m_4-3) T_4`, est implémentée comme
**contrôle d'invariant** : sa violation rend le code trois. Elle n'a jamais été
violée sur les nuages testés.

## 5. Ce que le sondage change, et ce qu'il ne change pas

Le sondage du vrai graphe est appliqué dans une bande d'indécision et sous un
plafond de largeur. Sur `terrain, n=1 500` :

| `probe_factor` | cellules | lifts | supports |
| ---: | ---: | ---: | ---: |
| `1` (désactivé) | `208 705` | `7 820 379` | `98 752` |
| `8` | `70 121` | `12 005 776` | `98 752` |
| `16` | `47 161` | `16 107 213` | `98 752` |

La sortie est invariante. Le sondage divise les cellules par trois à quatre et
multiplie les lifts par un facteur et demi à deux. **Le bon réglage est donc une
propriété du backend, pas de la géométrie** : sur ce CPU les cellules sont bon
marché et les cliques chères, donc le défaut reste `probe_factor=1`. Un A/B
device devra le recalibrer, car une cellule y coûte du trafic mémoire et un
lancement, pas seulement des comparaisons.

## 6. Portes

Vingt-huit portes `mhgp3v_centre_cell_*` passent, dont quatre nouvelles pour la
variante de lift différé — y compris le lot minuscule qui force les vidanges —
et le mutant `strata-stop` sur deux familles.

GCP non utilisé.
