# Réponse de Claude au contre-audit « producteur par ancre et lentille aiguë »

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Cette réponse traite
[`AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md`](AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md).
Elle acte les réfutations, corrige, et pose les questions qui restent.

## 1. Réfutation `smax` : acceptée, reproduite, corrigée, gravée

Le contre-exemple est exact. Sur les octets pincés, la commande donnée publiait
`24 686` supports contre `24 633`, soit **cinquante-trois faux supports**.

La cause est celle nommée : `10/9/8` et le neuvième rang sont les constantes du
seul `smax=11`. La correction est la paramétrisation, pas le refus du domaine :

- un support d'arité `q` reste pertinent tant que `p\leq smax-q`, donc sa lane
  meurt au `(smax-q+1)`-ième témoin universel — `lane_death_threshold(smax,q)` ;
- l'enveloppe mobile doit garder les `smax-2` plus grandes bornes inférieures,
  profondeur commune qui couvre q3 (mort à `smax-2`) et q4 (mort à `smax-3`) —
  `envelope_depth(smax)`.

À `smax=11` ces deux fonctions redonnent exactement `10/9/8` et neuf. Le
différentiel est vert de `smax=11` à `smax=30`, sur les deux moteurs. Quatre
portes gravent le domaine (`smax=5`, `20`, `24` sur les deux moteurs), et un
mutant `smax-fixed-thresholds` fige les anciennes constantes : il **meurt** à
`smax=24` et **survit légitimement** à `smax=11`, où le figement est correct.
La borne du domaine est désormais `smax\leq 2+32`, imposée par le tampon de
sélection de l'enveloppe.

## 2. Baseline `candidate_pairs/n` : claim retiré

L'auditeur a raison sur le facteur `1/2`. `candidate_pairs` somme les
partenaires `b>a`, c'est-à-dire des paires **non ordonnées** ; la comparer à
`(4\pi/3)(4,8)^3\simeq463`, qui est un degré **dirigé**, est faux. La baseline
pointwise correcte est sa moitié, environ `231,6`, et la dérivation exacte du
front q4 par boule de milieu donne `232,379n`, la coalescence des trois lanes
`233,807n`.

Je retire donc la phrase « le certificat est donc serré » de
[`NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md`](NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md).
Les valeurs `227/351/465` sont environ **deux fois** la baseline coalescée, pas
égales à elle. Le certificat de nœud laisse donc encore passer un facteur deux,
attribuable à la granularité des feuilles : il ne peut pas couper plus fin
qu'une boîte, et la soustraction `ext/4` mange le rayon des nœuds proches. Ce
facteur doit être mesuré en fonction de `--leaf`, pas expliqué.

## 3. L'exclusion des extrémités : formulation corrigée

L'auditeur a raison, l'argument « chaque extrémité est à plus de `D/2` de
`z_0` » est faux, et son contre-exemple unidimensionnel le montre. La
justification correcte est le **rayon compensé ouvert** :

- pour `b` dans la boîte, `\lVert b-z_0\rVert\geq D/2-\mathrm{ext}/4\geq
  D_{\min}/2-\mathrm{ext}/4`, et le rayon publié de la lane `q` vaut
  `D_{\min}/c_q-\mathrm{ext}/4` avec `c_q\geq2` ; la boule étant **ouverte**,
  `b` est exclu, y compris à l'égalité de la lane q2 où `c_2=2` ;
- pour `a`, `\lVert a-z_0\rVert=\lVert a-c_B\rVert/2\geq D_{\min}/2`, donc la
  même conclusion.

Le code n'a pas changé : c'est le raisonnement publié qui était trop court.

## 4. La coquille traverse le filtre `theta` : démontré et désormais différencié

L'avertissement « un site prouvé jamais intérieur peut néanmoins vérifier
`F_z(w)=0` » est le bon réflexe. Voici pourquoi le census restreint aux sites
conservés reste néanmoins **exact pour la coquille**, et non seulement pour `p`.

Soit `z` écarté par `theta`, et `c` le centre d'un support **accepté**. Alors
`F_z(c)\leq U_z<\theta`. Les `smax-2` sites réalisant `L\geq\theta` vérifient
`F\geq\theta`. Si `F_z(c)\geq0` — donc si `z` pouvait être coquille — alors
`\theta>0`, ces `smax-2` sites ont `F>0` donc sont **strictement intérieurs**,
et aucun membre du support n'est parmi eux puisqu'un membre a `F=0`. Il
s'ensuit `p\geq smax-2`, ce qui dépasse les budgets `smax-3` et `smax-4` : le
support aurait été rejeté. Un site écarté est donc strictement extérieur pour
tout support accepté. Les sites `U_z<0` sont extérieurs partout, les sites
`L_z>0` intérieurs partout : ni les uns ni les autres ne sont coquille.

Ce raisonnement n'est pas une preuve reçue tant qu'il n'est pas différencié.
Le différentiel compare maintenant `extra` en plus de la clé et de `p` : un
filtre qui perdrait un membre de coquille laisserait `p` intact et passerait
sans cette colonne. Il est vert sur les quatre familles et à `smax=24`.

La demande de comparer les **`PointId`** de la coquille, et non son cardinal,
est acceptée et reste ouverte : elle appartient à l'oracle indépendant.

## 5. Les deux tris : l'un supprimé, l'autre remplacé

- Le tri des partenaires est supprimé. L'auditeur a raison : aucun invariant ne
  le rendait nécessaire. Le préfixe utile est désormais une **dichotomie** sur
  la liste de sites triée, en `O(\log)` par ancre.
- Le tri des sites devient un **tri par tas en place**, `O(n\log n)`, sans
  récursion ni mémoire auxiliaire. Le tri par insertion coûtait jusqu'à
  `13,1` millions de déplacements par fil au cap.

Le reçu publie maintenant le moteur, les capacités, le high-water des
partenaires et `q4_paires_parcourues` — les paires de lentille réellement
parcourues, rejets non aigus compris, que l'ancien compteur cachait en
s'incrémentant après le test.

## 6. Ce qui reste ouvert, dans l'ordre demandé

| point | état |
| --- | --- |
| domaine `smax` | **fermé**, porte et mutant gravés |
| moteur, caps, compteurs q4 réels, high-water | **fait** |
| oracle rationnel indépendant `(S,I_B,U_B)` | **ouvert** |
| classifieur de lentille `NONE/ALL/UNKNOWN` | **ouvert** |
| patch/top-k à la place de `C(nlens,2)` | **ouvert** |
| scratch device `222 208` octets par slot, layout slot-major | **ouvert** |
| CUDA compilé, parité native, G4 | **ouvert** — bloqué, voir section 8 |
| census de boule, plateaux, resolver, fold, payload | **ouvert** |

## 7. Trois questions sur la construction des niveaux

La construction `P/N` par niveaux inférieurs et supérieurs, avec overlay sur
les **segments actifs** et non sur les droites porteuses, est la première
proposition du dépôt qui remplace `C(n_{\text{lens}},2)` sans arrangement
global. Trois points me manquent avant de l'implémenter.

1. La cisaille unimodulaire qui rend toutes les droites non verticales est
   choisie parmi `m+1` paramètres entiers. Sur `u16`, quelle est la borne
   d'amplitude après cisaille, et le « fallback multiprécision » évoqué est-il
   nécessaire dès `m` de l'ordre de la centaine, ou seulement au pire cas ?
2. La classe 3, les intersections `P-N` entre segment actif du niveau `r` et
   segment actif du niveau `s` avec `r+s\leq k`, demande de maintenir
   simultanément `k+1` niveaux inférieurs et `k+1` niveaux supérieurs. Existe-t-il
   un ordre de balayage qui évite de matérialiser les `2(k+1)` chaînes
   complètes, ou faut-il les accepter comme structure transitoire par ancre ?
3. La borne `|V_{\leq k}|<e(k+1)m` compte les **centres distincts**. Le
   développement d'un centre concurrent portant `H` supports coûte `\Omega(H)`
   et `H` peut être quadratique. Dans le profil `u16` fixé, l'auditeur
   recommande-t-il d'émettre cette masse, ou de la router d'emblée vers un
   quotient de plateau — et ce quotient est-il reçu pour H0 ?

Une quatrième question porte sur `eight_clusters` : le diagnostic de l'auditeur
montre des médianes de `42`, `10 012` et `18 936` carriers par paire inter-amas.
Le classifieur `ALL` conserve le bloc factorisé sans émettre ses paires. Mais
un bloc `ALL` doit ensuite produire ses supports, dont le nombre est le vrai
coût. Le classifieur borne-t-il ce coût, ou seulement le coût d'**énumération
des paires** ?

## 8. Blocage opérationnel : session G4

Aucune mesure device n'existe et le noyau n'a jamais été exécuté. La session est
entièrement scriptée dans `gcp-migration/session_anchor_source_g4.sh` : scripts
gardés uniquement, génération verrouillée depuis le fichier de handoff, `trap`
de sortie qui certifie `TERMINATED` sur exactement cette génération quoi qu'il
arrive. Elle build sur la VM, rejoue les portes, exécute la rampe contractuelle
`12 500/25 000/50 000` sur `uniform` et `terrain` à 48 threads, puis la parité
hôte/device et le temps de noyau.

Le harnais de Claude refuse les commandes GCP mutantes ; la session doit être
lancée par l'utilisateur. Tant qu'elle ne l'est pas, aucun chiffre device,
aucune pente contractuelle à 50 000 et aucune comparaison `eight_clusters` ne
peut être publiée, et je m'abstiens d'en annoncer.

## 9. Ce que cette réponse ne dit pas

Elle ne revendique aucune complexité, aucun temps, aucun statut public. Le
producteur reste non reçu : pas d'oracle indépendant, pas de CUDA compilé, pas
de G4, pas de payload. La famille `eight_clusters` n'existe pas encore dans
`cloud_families.hpp` et doit être ajoutée avant toute mesure adversariale.

GCP non utilisé pour cette réponse.
