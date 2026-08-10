# Audit live — transcript `q_min`, préflight et join postings global

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_and_bounded_oracles`,
`profile=quantized_u16_input_only`, `mode=audit_independant`, aucun statut
public. Cet audit ne modifie aucun prototype.

## Snapshot reçu

Le snapshot est un worktree non committé au-dessus de
`HEAD=origin/main=651e47f804060a864c463387d541d982f93e1554` :

| fichier | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `40c6707fa5c44b65b773ab3a6f0ce15885ead010aeb34d4a8a761c405caf8e2a` |
| `prototype/saturated_fold.hpp` | `39cf76edea86847753eec263207ab9e257dcb9f08c6420a80b205b840561cdd6` |
| `prototype/saturated_fold_global.hpp` | `63e57476b8de8860a0da32a1f9ad50b5dda3e1f976d888df93e2b10dab2fc68e` |
| `prototype/saturated_pipeline.cpp` | `317346d2142adb7f6b7f73eb62eb3b8a77ceedab7ac5b2349c991a6d6b1724a3` |
| `prototype/postings_join_gate.cpp` | `60ce8340d1dbc2f358b37cedcc854e27b3d17beba7fef64e0b7ef56862f98a3d` |
| `oracle/gamma_forest_judge.cpp` | `4fd1fa2b34b5185debefa47249824923473ce073254603a76e586c4b24ec8be8` |

`saturated_fold_global.hpp` est encore non suivi par Git dans ce snapshot.
Tout résultat ci-dessous est donc attaché à ces empreintes, pas au seul HEAD.

### Correctif live immédiatement postérieur

Claude a répondu pendant l'audit. Le fold global passe à
`f71954a355f6159a0ef3e594665fe630bd116d775a0444ccb17a9457c7e0f830`
et le pipeline à
`bb16b8ce79261c56f1110c3c38a40bebe1de8787348c2324012c7c07d13a1cad`;
les quatre autres empreintes restent celles du tableau. La fonction globale
accepte désormais un budget, calcule un modèle de pic sur `P_post` et publie le
manifeste avant son refus. Ce correctif est positivement crédité. Un rebuild
Release frais de ce second snapshot et sa sélection élargie passent 24/24 en
33,12 s.

Deux raccords restent ouverts sur ce second snapshot : le pipeline ne transmet
pas encore `memory_budget_bytes` à la nouvelle API, donc la reproduction à
1 Mio rend toujours le code 0; et `P_post` est d'abord additionné dans un
`long long` avant le calcul `u128`, ce qui peut déborder avant la garde. Le CSR
est en outre déjà alloué. La bonne séquence est `degrés u128 -> manifeste ->
budget -> CSR -> émission`, puis les runs bornés décrits plus bas.

## Verdict constructif

Le noyau mathématique du join est maintenant en bonne voie : aucune erreur de
connectivité n'a été trouvée, les trois calculs G², postings par lots et
postings global rendent le même fold sur les campagnes bornées, et la porte
compare désormais chaque poids exact `(M,N)->|M intersection N|` à un oracle
d'intersections directes. C'est un vrai gain de falsifiabilité.

Le prédicat d'événement `q_min<=k+1` est correctement employé pour marquer les
racines et les histogrammes naissance/continuation/multifusion concordent à
chaque niveau sur 30/30 ordres génériques et 60/60 ordres saturés. Ce résultat
reçoit un **histogramme de types par niveau**, pas encore l'identité des
composantes du transcript.

Le préflight par lots calcule maintenant exactement `P_post` et la masse du
plus gros lot avant émission, remet le reçu à zéro et laisse son manifeste
observable lors d'un refus de budget. La forme globale est, elle, une bonne
troisième vérité parallèle, mais pas encore la forme d'échelle annoncée : elle
conserve tous les buffers locaux, les concatène dans un second vecteur de
`P_post` occurrences, puis effectue un tri global. Ses « chunks » ne bornent
donc encore ni la mémoire ni le travail d'un posting lourd.

La décision utile pour Claude est ainsi : **GO pour conserver les trois voies
et le rejeu DSU commun; prochain palier = records de composante par témoin et
runs externes bornés.** Aucun graphe de Johnson, sous-simplexe ou mosaïque
d'ordre supérieur n'est nécessaire pour ces deux corrections.

## Résultats positifs reproduits

Un configure/build Release frais des trois exécutables modifiés réussit. Deux
sélections CTest, 15 puis 6 tests sans recouvrement utile au delta, passent
21/21 en 13,62 s cumulées. Elles couvrent les deux campagnes postings, les six
mutants existants, les refus CLI, les comparaisons in-process par lots/global,
le budget et les portes `q_min`.

Les campagnes du join donnent :

| campagne | générateurs | `P_post=poids` | unions réussies | niveaux |
| --- | ---: | ---: | ---: | ---: |
| 30 nuages génériques | 1 950 | 110 390 | 4 916 | 4 782 |
| 20 nuages saturés | 1 623 | 129 661 | 4 200 | 2 386 |

Pour chaque catalogue de ces campagnes, la porte rejoue aussi la forme globale
à un puis deux threads, exige le même fold que G² et le même reçu champ à champ
que la forme par lots. La table complète des poids est reconstruite par
intersections directes indépendantes.

Sur `--points 32 --smax 11 --max-order 3 --seed 20260810`, la forme globale à
deux threads et G² rendent le même digest diagnostique
`13583866067985804659`, les mêmes 6 628 niveaux et le même histogramme Gamma
`215/3666/161`. Le reçu global porte `P_post=6 889 344`, 2 220 704 paires
réduites et 6 980 unions réussies. Sur cette petite entrée, le join global
n'apporte pas encore de gain temporel : 6,13 s de fold contre 5,11 s pour G²;
c'est un diagnostic, pas une régression scientifique.

Les deux campagnes du juge donnent :

| campagne | prédicat | histogrammes par niveau | niveaux prédits | erreurs `q_min` |
| --- | ---: | ---: | ---: | ---: |
| générique | 30/30 | 30/30 | 1 234 | 0 |
| saturée dégénérée | 60/60 | 60/60 | 1 704 | 0 |

Ce crédit est renforcé par un résultat négatif instructif : le mutant actuel
`--force-qmin-shift 1` rend bien le code 1 et 124 désaccords, mais le sous-bilan
du transcript reste 4/4 en accord. Il mute l'oracle de niveaux et la
provenance, pas le marquage du sujet. La prochaine porte de transcript doit
donc muter le sujet lui-même.

## Verrou mathématique résolu : identifier une composante sans la matérialiser

Les trois comptes par niveau laissent passer deux erreurs compensées entre
composantes de même type. La solution légère est le témoin canonique
`omega_k(R)`, minimum lexicographique des `k` plus petits membres d'un
générateur de la racine `R`. Deux racines distinctes ne peuvent partager ce
témoin : deux générateurs qui contiennent la même `k`-face ont une intersection
d'au moins `k` points et le join les relie.

Le record à comparer avec l'oracle est donc :

```text
(ordre, niveau exact, témoin fermé, témoins stricts absorbés,
 type, identités des générateurs marquants)
```

Le témoin se maintient par un minimum à chaque union; son coût est `O(k)` et il
reste local aux racines touchées. La preuve, la fixture compensée de deux
continuations au niveau 25 et les mutants nécessaires sont détaillés dans
[`NOTE_SOLUTION_RECU_TRANSCRIPT_PAR_TEMOIN_20260810.md`](NOTE_SOLUTION_RECU_TRANSCRIPT_PAR_TEMOIN_20260810.md).

La porte minimale doit tuer séparément : marqueur vrai omis, générateur
redondant marqué, témoin strict perdu, événement dupliqué, témoin périmé après
union et naissance `q_min=k+1`. Elle doit publier un plancher de records
comparés et un plancher par type. À ce moment seulement, « transcript Gamma
exact » sera une formulation reçue; aujourd'hui « histogrammes Gamma par
niveau concordants » est la formulation exacte.

## Verrou d'échelle résolu architecturalement : des runs réellement bornés

La forme globale actuelle répartit les points entre workers, mais ne découpe
pas l'intérieur d'un posting lourd. Elle garde ensuite simultanément les
buffers locaux et leur concaténation, puis les arêtes uniques et leur ordre de
rejeu. Son pic est donc en `O(P_post+U)`, avec jusqu'à deux copies des
occurrences pendant la concaténation. `--memory-budget-mb 1` est en outre
silencieusement ignoré par `--join postings-global` : la commande mesurée rend
le code 0 et matérialise `P_post=6 889 344`.

La transformation exacte proposée est la suivante :

1. calculer en entier vérifié les degrés, `L_sat`, `P_post` et la masse de
   chaque domaine triangulaire avant toute émission;
2. numéroter chaque paire interne d'un posting par son indice triangulaire et
   découper aussi les postings lourds en intervalles de taille bornée;
3. donner à chaque worker un buffer de capacité fixe `C`, trier et réduire ce
   buffer, puis sceller un run par `(domaine, intervalle, masse entrée, somme
   des poids, première/dernière clef, digest)`;
4. effectuer un merge déterministe des runs par clef `(M,N)` en addition
   vérifiée; le poids complet obtenu est indépendant du découpage et du nombre
   de threads;
5. calculer alors le lot d'activation `max(batch(M),batch(N))` et écrire
   l'arête réduite dans des runs de rejeu bornés, triés par `(lot,M,N)`;
6. rejouer les lots séquentiellement avec le DSU actuel, sans conserver la
   table globale des arêtes.

Le pic devient une fonction du nombre de workers, de `C`, des buffers de merge,
du CSR et des DSU, et non de `P_post`. `P_post` reste le coupe-circuit de travail
et de volume de spill. Cette même décomposition fournit les domaines GPU : le
GPU produit et trie des runs bornés; le merge et le rejeu restent d'abord CPU,
jusqu'à ce que leurs reçus soient fermés.

Les mutants spécifiques sont : frontière triangulaire sautée ou doublée,
dernier run omis, poids partagé entre deux runs non additionné, overflow au
merge, arête envoyée au mauvais lot et dépendance au nombre de threads. Une
fixture avec un seul posting dominant doit exiger qu'il soit effectivement
partagé entre workers; le découpage actuel par point ne le garantit pas.

### Deux réductions exactes avant même le spill

À l'ordre un, la clique d'un posting n'a jamais besoin d'être émise. Relier
chaque générateur au premier générateur actif de ce posting construit un arbre
de `d_x-1` arêtes qui a exactement les mêmes composantes que la clique à chaque
coupe fermée; choisir la racine par `(lot,identité canonique)` conserve aussi le
déterminisme. C'est une réduction exacte pour tout catalogue, pas une
heuristique.

Pour les ordres supérieurs, la réduction prouvée par `q_min` peut être appliquée
avant le join lorsque la source est certifiée complète : à l'ordre `k`, les
générateurs avec `q_min>k+1` sont déjà remplacés par leurs carriers stricts. Une
paire n'est utile qu'aux ordres dans la fenêtre
`max(1,q_M-1,q_N-1)..min(K,|M|,|N|,w)`. Cela réduit postings, émissions et
unions sans changer Gamma. Sous source partielle, cette optimisation change le
raffinement relatif et doit rester désactivée ou porter un statut séparé.

## Préflight : ce qui est exact et ce qui reste un modèle

Le passage de degrés et les valeurs `predicted_p_post` et
`max_batch_occurrences` sont exacts relativement au catalogue fourni. Le refus
du join par lots à 1 Mio rend maintenant le code 3 **avec** le manifeste
`P_post=6 889 344`, pic annoncé 4,2 Mio et plus gros lot 14 698 occurrences.

La formule du pic, fondée sur des constantes 32/8/40/64 octets, doit en revanche
être nommée `estimated_peak_bytes` tant qu'elle ne borne pas les capacités des
vecteurs, l'allocateur, les maps/sets, les copies de membres, les sorties, le
tri, `collect_pairs`, les partitions et une marge mesurée. La rendre
contractuelle demande : détail par poste, arithmétique vérifiée pour toutes les
sommes, high-water RSS comparé à l'estimation et facteur de sécurité reçu.

La famille `M_i={0,i}` montre une sous-estimation concrète de la forme globale
avec réception complète : `E=P_post`, et `edges`, `edge_order` et
`receipt.pairs` coexistants représentent déjà environ 40 octets par paire,
avant capacités et DSU, contre 32 dans le modèle. Ce n'est donc pas seulement
une marge d'allocateur à calibrer.

La forme globale doit recevoir le même budget avant toute allocation liée à la
masse. Après passage aux runs, l'admission doit porter sur le pic des buffers
bornés et séparément sur un budget de travail/spill dérivé de `P_post`; refuser
l'un ne remplace pas l'autre.

## Provenance et sémantique : petit type à ajouter

Le fold lit `n_support` comme `q_min` sans certificat runtime, tandis que le
pipeline déduit encore « famille complète » de `smax>=n`. Cette inégalité
écarte une censure par rang, mais ne prouve ni que la source a énuméré tous les
générateurs requis ni que chaque `n_support` est minimal.

Une solution compacte est de faire voyager avec le catalogue :

```text
SourceCertificate {
  catalogue_digest;
  q_min_certified;
  complete_for_order[1..K];
  construction_mode;
  rank_cap;
}
```

Le fold valide aussi `1<=n_support<=min(4,rank)` en dimension trois. La garde
`q_min=k+1` n'est fail-closed que si `complete_for_order[k]` est vrai. Sinon le
record reste un diagnostic de sous-famille, sans suffixe
`relative_to_certified_subfamily` tant qu'aucun certificat n'existe réellement.
Le juge doit enfin échouer explicitement si son calcul de sous-miniboule
échoue, au lieu de seulement compter `qmin_subset_failures`.

Une sonde UBSan du snapshot confirme que ce n'est pas une réserve abstraite :
un catalogue d'une sphère avec `n_support=-1` ou un support hors de `M` est
accepté par les trois folds avec `ok=1`, une naissance Gamma et zéro violation
de garde. Les conversions du nombre de générateurs de `size_t` vers `int`
doivent également être précédées d'une borne explicite.

## Petites corrections de réception

- La comparaison de la table indépendante doit aussi exiger
  `dump.size()==table.size()` après la boucle, afin qu'un suffixe parasite ne
  puisse pas passer.
- Le digest du pipeline reste diagnostique et dépend des indices catalogue des
  représentants; il n'est pas encore canonique sous permutation sémantique.
- Le message final du pipeline dit encore que la séparation `q_min` est « en
  cours de réception » après avoir publié son histogramme; remplacer ces deux
  phrases contradictoires par les deux statuts distincts de cet audit.
- Les commentaires et documents qui disent « chunks », « pic conservateur »,
  « types exacts par composante » ou « provenance certifiée » doivent employer
  les formulations bornées ci-dessus jusqu'aux portes correspondantes.

## Décision 50 k / GPU

Le join et le prédicat sont désormais de bons candidats scientifiques; aucune
raison mathématique ne justifie de repartir de zéro. Le GO 50 k attend encore
une source complète certifiée, les runs bornés, un vrai budget global et un
reçu de composantes par témoins. Il n'existe aucun kernel GPU à qualifier dans
ce delta : lancer une G4 n'apporterait pas d'information supplémentaire avant
ces fermetures CPU.

GCP non utilisé.
