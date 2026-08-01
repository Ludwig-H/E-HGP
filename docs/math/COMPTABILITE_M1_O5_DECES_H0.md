# Comptabilité M.1/O5 des décès $H_0$ dans un lot de niveau égal

> [!IMPORTANT]
> **Statut scientifique local : `proved_here`. Statut logiciel : `implemented_and_freshly_certified`.** La preuve ci-dessous ferme uniquement l'obligation combinatoire O5, relativement à un lot et à ses cibles pré-lot fournis. Elle ne ferme ni le contrat M.1 global, ni la complétude topologique des événements, bras, carriers ou incidences silencieuses, ni la naturalité, la verticalité, la scalabilité ou un statut public exact.

| propriété | valeur |
|---|---|
| phase | 15, sans ouverture ni fermeture de phase |
| backend | `reference_cpu` |
| profile | `hgp_reduced` |
| mode | `bounded_n14_conditional_direct_equal_level_m1_o5_death_accounting` |
| porte d'entrée | satisfaite relativement à la façade directe, au journal d'événements, aux graines de bras et au journal de forêt v4 fraîchement rejouables |
| portée | combinatoire locale d'un lot de niveau exact, conditionnelle aux objets fournis |
| théorème O5 | `proved_here` |
| implémentation produit | `implemented_and_freshly_certified` |
| falsificateur combinatoire permanent | 9 cas, 4 tests `PASS` en 0,001 s |
| validation intégrée | CTest Release `1/1 PASS` en 0,05 s, 0,06 s au total; runner v6 et gate v4 `PASS` en 19,736 s |
| déploiement | `architecture_only` |
| statut public | `not_claimed` |
| porte suivante | non satisfaite |

Le [contrat candidat M.1](../contracts/M1_RECONSTRUCTION.md) reste globalement `proof_obligation`. La présente note isole la partie de O5 qui ne dépend que de la combinatoire d'un lot simultané. Elle n'affirme pas que ce lot est topologiquement complet ni qu'il représente le passage du sous-niveau strict au sous-niveau fermé; ces questions restent notamment dans O1–O4.

## 1. Données et convention de lot

On fixe un ordre $k$, un niveau carré exact $a$ et l'état strictement pré-lot. Soit $R_a$ l'ensemble fini de ses racines. Le lot contient un ensemble fini $E_a$ d'événements d'indice un. Pour chaque $e\in E_a$, le shell indexe l'ensemble non vide $U_e$ de ses bras. Une occurrence de bras peut désigner une racine de $R_a$ ou un carrier latent canonique du lot; aucune occurrence latente n'est supprimée avant la contraction.

Soit $A_a$ l'ensemble des terminaux canoniques du lot. Deux occurrences d'un même terminal latent canonique désignent le même élément de $A_a$. Une occurrence résolue $x\in A_a$ porte une racine $\rho(x)\in R_a$; une occurrence latente vérifie $\rho(x)=\bot$. Pour chaque événement $e$, l'image de ses bras forme une hyperarête $H_e\subseteq A_a$, avec multiplicité d'occurrence conservée dans le compte $\lvert U_e\rvert$. Les répétitions d'un terminal dans une hyperarête ne peuvent qu'abaisser son coût effectif de contraction et ne compromettent donc pas la borne démontrée plus bas.

On définit $\sim_a$ comme la plus petite relation d'équivalence sur $A_a$ qui contient les deux familles de générateurs suivantes :

1. deux terminaux résolus portant la même racine pré-lot sont équivalents;
2. tous les terminaux d'une même hyperarête $H_e$ sont équivalents, y compris les terminaux latents.

Ainsi, la contraction est simultanée : elle dépend de la clôture transitive du lot entier et non d'un ordre de parcours des événements. On note $\mathcal{C}_a=A_a/{\sim_a}$ l'ensemble de ses classes. Pour $C\in\mathcal{C}_a$, on pose

$$R(C)=\left\lbrace r\in R_a:\text{il existe }x\in C\text{ tel que }\rho(x)=r\right\rbrace,\qquad q_C=\lvert R(C)\rvert.$$

Les racines sont comptées distinctement : plusieurs bras, plusieurs événements ou plusieurs occurrences portant la même racine ne contribuent qu'une fois à $q_C$. On note aussi

$$R_{\mathrm{touch}}=\bigcup_{C\in\mathcal{C}_a}R(C),\qquad N_{+}=\left\lvert\left\lbrace C\in\mathcal{C}_a:q_C>0\right\rbrace\right\rvert.$$

## 2. Théorème local O5

> **Théorème O5 — comptabilité des décès dans un lot.** Sous les seules conventions de la section 1, le nombre de décès $H_0$ imposés par la contraction simultanée des racines pré-lot touchées est

$$D_a=\sum_{C\in\mathcal{C}_a}\max(q_C-1,0)=\lvert R_{\mathrm{touch}}\rvert-N_{+}.$$

> Pour les événements d'indice un, la multiplicité locale satisfait $\Delta_1(e)=\binom{\lvert U_e\rvert-1}{1}=\lvert U_e\rvert-1$, et le total du lot vérifie

$$D_a\leq\sum_{e\in E_a}\bigl(\lvert U_e\rvert-1\bigr)=\sum_{e\in E_a}\Delta_1(e).$$

> Le nombre $D_a$ est canonique au niveau du lot. En général, il n'existe aucune attribution canonique de ses unités aux événements individuels.

## 3. Preuve

### 3.1 Contribution d'une classe

Avant le lot, les $q_C$ racines distinctes de $R(C)$ représentent $q_C$ composantes préexistantes distinctes. Après contraction de la classe $C$, elles appartiennent à une seule composante si $q_C>0$. La diminution du nombre de composantes préexistantes est donc $q_C-1$ dans ce cas. Si $q_C=0$, la classe ne contient que des carriers latents et ne tue aucune composante pré-lot. Sa contribution est donc exactement $\max(q_C-1,0)$.

La relation « porter la même racine pré-lot » est déjà un générateur de $\sim_a$. Une racine touchée ne peut donc appartenir à deux classes distinctes. Réciproquement, chaque racine touchée appartient à une classe. Les ensembles non vides $R(C)$ forment ainsi une partition de $R_{\mathrm{touch}}$, d'où

$$\sum_{C\in\mathcal{C}_a}q_C=\lvert R_{\mathrm{touch}}\rvert.$$

En sommant seulement sur les classes telles que $q_C>0$, on obtient

$$\sum_{C\in\mathcal{C}_a}\max(q_C-1,0)=\sum_{C:q_C>0}(q_C-1)=\lvert R_{\mathrm{touch}}\rvert-N_{+}.$$

### 3.2 Borne par les multiplicités locales

Pour chaque événement $e$, on remplace conceptuellement son hyperarête par une étoile reliant un terminal pivot à chacune de ses autres occurrences. Cette étoile contient au plus $\lvert U_e\rvert-1$ arêtes ordinaires et engendre la même identification que l'hyperarête; une répétition de terminal ne fait qu'ajouter une arête inutile. Les étoiles de tous les événements, ajoutées aux identifications gratuites des occurrences portant une même racine pré-lot, engendrent donc $\sim_a$.

Dans une classe finale $C$ contenant $q_C>0$ racines distinctes, tout graphe témoin qui les relie contient au moins $q_C-1$ arêtes issues des étoiles. Des carriers latents peuvent être des sommets intermédiaires, mais ils ne réduisent pas ce minimum et ne créent aucun décès supplémentaire. En sommant sur les classes disjointes, chaque arête d'étoile est comptée au plus une fois, donc

$$D_a=\sum_{C\in\mathcal{C}_a}\max(q_C-1,0)\leq\sum_{e\in E_a}\bigl(\lvert U_e\rvert-1\bigr).$$

Enfin, tout événement pertinent pour $H_0$ a l'indice $\mu=1$. La formule locale de Reani–Bobrowski donne alors $\Delta_1(e)=\binom{\lvert U_e\rvert-1}{1}=\lvert U_e\rvert-1$, ce qui prouve la seconde égalité annoncée. La preuve de O5 est complète dans cette portée combinatoire conditionnelle.

## 4. Pourquoi aucun décès n'est attribué canoniquement à un événement

Considérons trois racines pré-lot distinctes $r_1,r_2,r_3$ et trois hyperarêtes de taille deux reliant respectivement les paires $(r_1,r_2)$, $(r_2,r_3)$ et $(r_1,r_3)$. Le lot possède une seule classe avec $q_C=3$, donc $D_a=2$, tandis que la somme des trois multiplicités locales vaut $3$. Dans tout rejeu séquentiel, les deux premières hyperarêtes d'un arbre couvrant peuvent recevoir un décès et la troisième zéro; permuter l'ordre change l'événement crédité de zéro sans changer le lot.

Le même défaut apparaît déjà avec deux événements identiques reliant deux racines : $D_a=1$, mais l'un ou l'autre événement peut être crédité selon l'ordre. Une convention de pivot ou de tri peut produire une attribution déterministe de journalisation; elle n'est pas une donnée topologique canonique. Seuls $D_a$, les classes $C$, les ensembles $R(C)$ et les nombres $q_C$ sont invariants dans la portée présente.

## 5. Conséquence architecturale bornée

Le calcul de O5 requiert seulement les racines pré-lot touchées, les terminaux latents canoniques et les hyperarêtes du lot courant. Une structure locale d'union–recherche peut former $\sim_a$, dédupliquer les racines par classe et calculer $D_a$ sans matérialiser de catalogue global de cellules, cofaces ou incidences, aucune composante Gamma globale et aucune mosaïque de Delaunay d'ordre supérieur. Sa mémoire intermédiaire est proportionnelle aux terminaux et incidences du lot accepté, non à une mosaïque exhaustive.

`ExactDirectMorseM1O5DeathAccounting` implémente ce contrat : il préflight toutes les capacités avant ses arènes persistantes et transitoires, rejoue fraîchement les quatre autorités sources, forme l'union–recherche locale, compare sa partition aux groupes atomiques de la forêt et reconstruit les deux identités ainsi que la borne. Son vérificateur refait récursivement le calcul sans laisser le reçu observé choisir le rejeu. Les identifiants process-local de carrier, racine et nœud sont détruits avec le scratch; le reçu conserve seulement digests, niveaux, supports, comptes et audits canoniques.

L'oracle exhaustif borné à $n\leq14$ reste une autorité relative de falsification, jamais l'architecture produit. La porte d'entrée est satisfaite par la façade directe, le journal d'événements, les graines de bras et le journal de forêt v4 fraîchement rejouables, uniquement relativement à la façade fournie. L'autorité cible $K=2\to1$ est un jalon orthogonal acquis séparément; O5 ne la consomme pas.

## 6. Validation logicielle et fixtures permanentes

La matrice de falsification indépendante couvre :

- une classe latente pure avec $q_C=0$ et zéro décès;
- une classe contenant plusieurs occurrences de la même racine avec $q_C=1$ et zéro décès;
- une fusion triple et une fusion quadruple avec égalité dans la borne;
- une chaîne de deux hyperarêtes reliant deux racines via un carrier latent partagé;
- plusieurs événements incidents dont une hyperarête devient redondante;
- deux événements identiques, avec invariance sous permutation des événements et de leurs bras.

La fixture permanente [`morse_m1_o5_death_accounting.json`](../../tests/fixtures/regressions/morse_m1_o5_death_accounting.json) contient neuf falsificateurs combinatoires : CE-M1-01 à CE-M1-04, une chaîne de carriers latents, une pré-union de plusieurs carriers portant la même racine, une classe purement latente et un événement dupliqué. Le checker indépendant [`test_morse_m1_o5_death_accounting.py`](../../tests/oracle/test_morse_m1_o5_death_accounting.py) ferme les deux identités de $D_a$, la borne par les multiplicités locales, le refus de la somme événementielle naïve et l'invariance par permutation; ses quatre tests passent en 0,001 s. Ces cas sont des falsificateurs permanents de la comptabilité combinatoire, pas des contradictions topologiques à M.1.

Le CTest intégré construit une chaîne E5 réelle, y observe des groupes $q_C=0$, $q_C=1$ et $q_C\geq2$, vérifie les deux identités et la borne par lot, puis reconstruit récursivement le reçu. Il passe avec le budget exactement observé, échoue atomiquement avec un cap inférieur d'une unité pour les groupes, comparaisons, passages de bindings et scratch, rejette une forêt dont $q_C$ est muté, rejette un total de décès muté, falsifie les certificats d'échec portant schéma, enum, fait positif, compteur ou digest étranger et refuse $n=15$ avant tout rejeu source. Sous GCC Release strict, il passe `1/1` en 0,05 s, 0,06 s pour le CTest total.

Le mode runner `bounded_m1_o5_death_accounting_qualification` publie le schéma v6 et termine avant Gamma, l'autorité $K=2\to1$ et le suffixe vertical. Son checker de gate v4, garde O5 $n=15$ distinct inclus, passe en 19,736 s. Le reçu manuel `uniform_latin`, $n=4$, $K=3$ contient 6 événements, 6 groupes, 12 lots, 7 racines touchées, 4 groupes positifs, 3 décès globaux et 6 unités de capacité locale. Il prend 23,040 ms internes, dont 3,949 ms de construction et 6,373 ms de vérification, soit 10,322 ms pour O5; le mur externe vaut 0,025 s. Ses compteurs corrigés publient 24 scans de bindings, 18 de selles, 36 de lots, 36 comparaisons exactes et un pic de scratch logique de 25 entrées.

## 7. Non-revendications et porte suivante

La preuve ne démontre pas :

- que tous les événements, bras, carriers latents ou incidences silencieuses ont été fournis;
- que les hyperarêtes fournies réalisent le passage topologique exact du niveau strict au niveau fermé;
- O1–O4 ou O6–O9, donc le contrat M.1 global;
- une attribution canonique d'un décès à chaque événement;
- la naturalité horizontale ou verticale, la complétude des applications verticales ou la commutation de leurs carrés;
- une hiérarchie produit massive, un SLO, une capacité à 50 k ou plusieurs dizaines de millions de points;
- `forest_semantics=exact`, `public_status=exact` ou toute promotion publique.

La porte suivante, explicitement non satisfaite, est la certification de fidélité des carriers et des incidences silencieuses, suivie de la complétude bidirectionnelle des groupes Gamma et des checkpoints silencieux. Toute contradiction minimale deviendra une fixture permanente et mettra à jour le registre des preuves avant optimisation ou extension de portée.
