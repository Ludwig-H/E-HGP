# Réponse aux audits `21e617d` (banc contrebalancé) et `95061c1` (`first_batch`, ordonnancement)

Date : 19 août 2026 UTC. Cadre v4 : `phase=exploration_v4_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Les deux audits sont **exécutés intégralement**. Reçu :
`receipts/forest_20260817/ADDENDUM_BANC_APPARIE_ET_ORDONNANCEMENT_20260819.md`.

## 1. Le contre-audit du banc a raison, et je retire les chiffres

Vous avez trouvé deux fautes, et les deux sont miennes :

1. le banc exécutait toujours `tri` puis `streaming` — le second mode
   héritait de l'état laissé par le premier, dans un sens et une
   amplitude inconnus ;
2. je publiais le rapport de deux médianes marginales alors que le plan
   est **apparié**.

Votre recalcul sur mes propres données est exact : les cinq paires
donnent `0,964 ; 0,756 ; 1,047 ; 0,682 ; 0,926`, médiane appariée
`0,926`, soit `×1,08` et non `×1,19`. **Les facteurs `×1,19` et
`×1,58`, ainsi que la phrase « le gain décroît avec la taille », sont
retirés** — un erratum est en tête du reçu du 18 août.

Le banc corrigé fait ce que vous demandez : échauffement non
chronométré des deux modes, **ordre ABBA** (paire paire = tri d'abord,
paire impaire = streaming d'abord), **refus code 2** d'un
`--bench-repeat` impair ou `< 4`, **signature du résultat complet**
comparée à chaque exécution hors chronométrage, et publication des
mesures brutes — chaque exécution dans son ordre réel, les rapports
appariés, la médiane appariée, la médiane des différences
logarithmiques, le rapport de médianes en second, les victoires et
l'équilibre des positions.

Résultat, dix paires contrebalancées, n=8000, K dominant :

```text
0,8797 0,8459 0,8963 0,8615 0,9139 0,8661 0,8548 0,8741 0,8829 0,8900
mediane_appariee = 0,8769  -> x1,14      victoires 10/10
mediane_log      = -0,1313 -> x1,14      P(X>=10 | Bin(10;1/2)) = 1/1024
rapport_de_medianes = 0,8853             ordre 5/5, objet identique
```

Une seconde série (binaire antérieur au passage par référence) donne
`0,8453`, soit `×1,18`, avec 9/10 victoires. Les deux séries
s'accordent à `×1,14–×1,18` ; le rapport de médianes, lui, donne
successivement `×1,08`, `×1,32`, `×1,13` selon l'échantillon. Votre
point est démontré par les données elles-mêmes.

J'ai ajouté la **porte synthétique** que vous suggérez
(`--bench-report-gate`) : temps fictifs, aucun calcul réel ; elle
vérifie la médiane appariée, exige que le jeu SÉPARE les deux
statistiques (0,75 contre 1,1667 — sans quoi la porte ne prouverait
rien), contrôle les victoires et l'équilibre des positions, et refuse
`repeat ∈ {1,2,3,5,7}`. Elle a immédiatement attrapé une erreur
d'arithmétique de ma part dans sa propre fixture (0,875 au lieu de
0,75, médiane d'un échantillon pair) — avant tout banc.

Aucune conclusion d'échelle n'est tirée, et la passation porte
désormais la règle : toute comparaison de représentations passe par un
banc apparié contrebalancé intra-processus.

## 2. `first_batch` sort de la sémantique — exécuté

Votre renforcement est plus net que le mien : `T_b(f) ⟹ ¬S_b(f)` donne
`S_b(f) ⟹ A_b(f)`, et la réduction ne demande même pas
`birth_violations = 0`. `first_batch` est supprimé, remplacé par un bit
`seen` mis à jour **après** le macro-lot, qui n'alimente que les deux
compteurs ; les instantanés lisent `seen` après mise à jour.

Le gain n'est pas seulement de preuve, comme vous l'annonciez : un
`u32` par facette unique disparaît (**74,3 Mio touchés** à n=8000) et
avec lui une **écriture aléatoire par sondage réussi** du chemin chaud.

Portes : `intern-first-batch-last` supprimé (il testait une
redondance) ; `attach-detector-disabled` et `seen-before-check`
gravés, le premier tué par la fixture de flux incohérent, le second par
les familles géométriques régulières — la porte compare maintenant
aussi `attach_violations` et `birth_violations` entre backends.

**Un point que je signale plutôt que de le cacher** : la fixture de
flux incohérent n'exige plus l'égalité des sorties entre le backend
figé et le fold compact. Ce flux viole par construction l'hypothèse
sous laquelle ils sont prouvés égaux ; depuis la suppression de
`first_batch`, ils classent légitimement `{1,2}` différemment hors
hypothèse. La fixture exige donc ce qui a un sens — que le détecteur
parle et que les lots soient justes. Si vous préférez qu'elle exige
aussi une égalité, il faudra d'abord dire lequel des deux
comportements hors contrat fait foi.

## 3. Ordonnancement : votre budget mémoire est le bon cadre

Étape préalable faite : `build_forest` reçoit ses événements par
**référence constante** et trie une **permutation compacte d'indices**
— fin des ~400 Mo de copies cumulées à n=8000. Contrat public intact
(`batch_of_event` et `ev_fid` restent indexés par la position dans
l'ordre trié).

`M_K` est calculé avant lancement à partir des quantités que vous
listez. Les trois modes, même processus, mêmes événements, budget
2 Gio :

| mode | latence | pic RSS | réserve max | signature |
|---|---|---|---|---|
| `contiguous_reference` | 29 951 ms | 4 932 596 Kio | — | `ab003a59158fc7c8` |
| `LPT_unbounded` | 10 705 ms | 5 230 304 Kio | 4 319 406 292 | identique |
| `memory_budgeted_LPT` | 21 323 ms | 4 952 616 Kio | 2 140 153 484 | identique |

`LPT_unbounded` divise la latence par 2,80 mais réserve deux fois le
budget : il reste une **borne**, comme vous le prescrivez.
`memory_budgeted_LPT` divise par **1,40** pour **+0,4 %** de pic RSS et
respecte le plafond. Les trois signatures sont identiques.

Le défaut de production est donc passé à `memory_budgeted_LPT`, budget
déclaré (`--fold-memory-budget`, 2 Gio) et publié avec la réserve
atteinte sur une **ligne séparée** — la ligne `execution` est ancrée
`^…$` par le validateur de campagne et n'a pas été touchée. Garde
anti-blocage explicite : une tâche seule plus grosse que le budget
s'exécute quand rien d'autre ne tourne.

## 4. Ce que je n'ai pas fait

- La priorité générale est inchangée : le **scan q3 et les covers**
  restent le verrou dominant de `t_gen`, et rien de ce qui précède n'y
  touche.
- Je n'ai pas rouvert la question de l'assiette des couches convexes :
  elle demande des compteurs de charge par ancre, pas une décision.
- `ctest --test-dir build/v4` : **134 tests, tous verts** ;
  `python tools/check_docs.py` et `python tools/check_passation.py` :
  verts.
