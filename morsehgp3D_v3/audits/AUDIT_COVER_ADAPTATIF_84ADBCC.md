# Audit du cover adaptatif par feuille

Date du snapshot : 10 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_under_audit`,
`profile=quantized_u16_input_only`, `mode=audit_math_contractuel_borne`,
`public_status=not_claimed`.

Cet audit est strictement limité à `morsehgp3D_v3`. Il ne modifie aucun
prototype, n'ouvre aucune phase et ne promeut aucun résultat public.

| objet | empreinte |
| --- | --- |
| snapshot documentaire audité | commit `84adbcc8b7eefb54cc44014b21e369d8f5a439e6` |
| snapshot produit sous-jacent | commit `e406e1f646ef20eb222d50e8b2740e6d7d6f6aa3` |
| `prototype/direct_source.cpp` | SHA-256 `d933c3aeb6314f12769f594d30af6734c696b09ce2e67de39af23dbd0ed15ed9` |
| `CMakeLists.txt` | SHA-256 `739d21248a5fba575974aa3e40e8a0d7d4208b4a9c6710905ec4379cffa8fed7` |

## Verdict

**GO mathématique pour le certificat de rayon et le census avec un
`Q_{q,C}` propre à chaque feuille; NO-GO pour en déduire, sans nouvelle
capability, une réduction du nombre de candidats ou une construction en
`O(n+F*t_q)`.**

Le prototype possède déjà une donnée positive :
`Cover::per_leaf_effective` conserve le `Q` effectif de chaque feuille et les
statistiques publiées montrent que ces valeurs sont nettement plus petites que
leur maximum global sur les campagnes bornées. Le lemme est bien local. Le
générateur de candidats, lui, ne l'est pas encore : il construit un CSR global par lane
avec `Cover::q_value=max_C Q_{q,C}` avant de connaître le centre de chaque
candidat. Les `Q` locaux sont donc actuellement diagnostiques; ils ne retirent
aucun tuple de l'énumération.

## Ce que le lemme local prouve exactement

Soit une feuille fermée `C` et un entier `Q_{q,C}`. Si ses `t_q` témoins sont
tous à distance carrée strictement inférieure à `Q_{q,C}` de tout centre de
`C`, le raisonnement global se spécialise sans perte. Pour un support propre
`U` dont le centre appartient à `C`, l'hypothèse
`q+|I(U)|<=s_max` implique `beta(U)<Q_{q,C}` : sinon les `t_q` témoins seraient
tous strictement intérieurs. Pour toute ancre `p` du support et tout membre
fermé `x`, l'inégalité triangulaire donne alors
`dist2(p,x)<=4*beta(U)<4*Q_{q,C}`.

Deux usages sont donc exacts une fois `U` construit et son centre rationnel
localisé :

- tester la banque de la feuille `C` pour produire
  `AboveInteriorWindow` ou certifier `beta<Q_{q,C}`;
- effectuer le census terminal dans le voisinage local exact
  `N_{q,C}(p)={x!=p:dist2(x,p)<4*Q_{q,C}}`.

Ce résultat crédite une réduction possible du **census par candidat**. Il ne
prouve pas encore une réduction du **nombre de candidats**.

## Pourquoi le CSR global ne devient pas adaptatif par substitution

Dans le chemin courant, `build_cover` calcule tous les `Q_{q,C}`, affecte leur
maximum à `Cover::q_value`, puis `build_neighbourhoods` construit le graphe de la lane
sur le seuil `4*Cover::q_value`. Les tuples sont formés dans ce graphe avant
l'appel à `locate_leaf`. La feuille du centre est donc inconnue au moment où
le voisinage d'ancre doit être complet.

Employer le `Q` de la feuille qui contient **l'ancre** serait non justifié : le
centre d'une paire ou d'un support peut appartenir à une autre feuille. Employer
le `Q` de la feuille du **centre** exige déjà d'avoir formé le support qui permet
de calculer ce centre. Une substitution locale dans le CSR courant serait donc
circulaire ou incomplète.

Trois architectures restent mathématiquement possibles, mais elles n'ont pas le
même contrat :

1. conserver le graphe global et n'utiliser `Q_{q,C}` qu'après formation du
   candidat; le census baisse, pas `C_q`;
2. certifier pour chaque ancre un majorant des `Q_{q,C}` sur toutes les feuilles
   de centre encore possibles; il faut alors prouver cette sur-approximation et
   publier ses degrés;
3. énumérer par feuille de centre avec un propriétaire exact, des voisinages
   conditionnels et un ledger anti-duplication.

Une formulation exacte de l'option 3 aide à distinguer preuve et implémentation.
Pour chaque feuille `C`, définir sa dilatation d'ancre
`A_{q,C}={p in X:dist2(p,closure(C))<Q_{q,C}}`. Pour `p` dans `A_{q,C}`, énumérer les
partenaires au seuil local, calculer le centre rationnel puis ne conserver le
candidat que si `C` en est la feuille half-open propriétaire. Un vrai support
est complet dans cette voie : son centre est dans `C`, chaque point du support
est à distance carrée strictement inférieure à `Q_{q,C}` de `C`, et la feuille
propriétaire est unique. La masse tentée devient au moins
`C_q^try=sum_C sum_{p in A_{q,C}} C(d_{q,C}^+(p),q-1)`; un cap de degré local ne
borne pas à lui seul le nombre de feuilles incidentes à une ancre.

Les options 2 et 3 changent le digest, l'ownership, les voisinages et les
identités de masse. En particulier, la formule scalaire
`C_q=sum_p C(d_q^+(p),q-1)` ne suffit plus automatiquement pour une famille de
graphes conditionnée par feuille. La capability courante, qui scelle un unique
`Q_q`, un CSR et un degré maximal, reste un fallback exact. Une réduction
adaptative des candidats doit être versionnée séparément.

## Le coût de construction n'est pas `O(n+F*t_q)` par simple anneau

Sur la branche grille certifiée, avec `n>=t_q` et sans cap précoce, le prototype
rescane et classe partiellement les `n` points pour chacune des `F` feuilles. Il
effectue exactement `F*n` calculs de distance et le `partial_sort` ajoute une
borne `O(F*n*log(t_q+1))`; pour `t_q<=31` traité comme une constante du profil,
le terme de construction reste `Theta(F*n)`. Les replis petit nuage ou cap
évitent ce scan et doivent rester comptés séparément.

Une grille à deux niveaux ou une exploration par anneaux peut être une bonne
optimisation, mais elle ne fournit pas à elle seule la borne annoncée. Pour une
feuille `C`, notons `V_C` le nombre de cellules ou nœuds visités et `Z_C` le
nombre de points dont le score au coin le plus éloigné est effectivement
calculé. Une borne honnête est de la forme
`O(n+sum_C(V_C+Z_C*log(t_q+1)))`. Dans un nuage arbitraire, les anneaux peuvent
traverser beaucoup de cellules vides ou inspecter une grande cellule dense;
ni `V_C=O(t_q)` ni `Z_C=O(t_q)` ne découle du seul lemme de cover.

Pour recevoir `O(n+F*t_q)`, il faut donc au moins l'une des deux pièces suivantes :

- une structure top-`t_q` exacte avec preuve de ses bornes de construction et de
  requête dans le domaine u16;
- une capability de densité, d'expansion ou d'occupation qui borne les cellules
  et points visités;

Un plafond typé sur `sum V_C` et `sum Z_C`, avec repli exact et replay, donne un
reçu fini utile; il ne prouve pas une complexité asymptotique. Pour soutenir le
Big-O, la fonction de cap doit elle-même être prouvée `O(n+F*t_q)` sur toute la
capability. Un run qui n'atteint pas un cap arbitraire ne suffit pas.

Le choix de grille peut rester une décision d'implémentation. La capability doit
sceller le résultat et les coûts observables : partition, `Q_{q,C}`, témoins,
scores maximaux, nœuds et cellules visités, tests de distance, octets,
high-water, statut de repli et digest. Elle ne doit pas promettre une complexité
que la structure choisie ne prouve pas.

Une distinction positive évite de surcorriger : **vérifier** un certificat de
cover déjà fourni peut bien coûter `O(n+F*t_q)`. Le consommateur authentifie le
nuage, la topologie, les PointId distincts et les `F*t_q` inégalités de coin; il
n'a pas besoin de prouver que les témoins choisis sont les plus proches.
**Construire** un cover sélectif depuis les coordonnées reste une autre phase,
avec ses recherches top-`t_q`, égalités, allocations et replis. Le contrat doit
donc séparer `CoverCertificate` de `CoverBuilderReceipt`.

## Hors chrono n'est pas hors coût

Construire le cover avant le timer chaud peut isoler la phase de génération;
cela ne retire ni son temps ni sa mémoire du pipeline produit. Un amortissement
est recevable seulement si le reçu scelle l'epoch et le digest du nuage, le
nombre de requêtes qui réutilisent exactement le cover, la durée de vie et les
octets résidents. Sinon le rapport doit publier au moins `cover_build`,
`candidate_source`, `census`, `fold` et le total bout en bout.

## Capability et portes conseillées à Claude

Conserver le contrat actuel comme `center-cover-global-v1` est positif : il est
exact, total par repli racine et sert de juge de la variante. Une capability
candidate telle que `center-cover-local-v1` doit ajouter, sans remplacer le
fallback :

- le vecteur authentifié des `Q_{q,C}` et les témoins de chaque feuille;
- la règle exacte qui relie une ancre ou un candidat à la feuille de centre;
- l'ownership unique et le ledger de couverture des supports;
- les voisinages conditionnels, leurs degrés et leurs digests;
- les masses réellement énumérées, pas une formule conservée par habitude;
- les compteurs `V_C`, `Z_C`, tests, octets, high-waters, caps et replis.

Fixtures ou mutants minimaux utiles :

1. support dont l'ancre et le centre sont dans deux feuilles différentes, pour
   tuer le choix du `Q` de la feuille de l'ancre;
2. centre rationnel sur une frontière de feuille et témoin exactement au seuil,
   pour garder l'ownership half-open et l'inégalité stricte;
3. nuage avec longues couronnes vides puis cellule dense, pour rendre vivants
   `V_C`, `Z_C` et le repli de construction;
4. oracle borné qui compare l'ensemble des supports et leur multiplicité entre
   variante locale et fallback global;
5. mutant qui omet une feuille de centre ou émet un support depuis deux
   feuilles, tué par les identités de masse et le ledger.

## Sonde CPU positive et régression du mode cover

Le binaire Release de SHA-256
`9f1ef706ed0a9005a8a6fa20f56f3caa813d63f267aa0031211ec4c6f6157afc`
a été relancé localement :

```sh
./build/v3/mhgp3v_direct_source --clouds 1 --points 100 --coord 50 --smax 11 --seed 7 --leaf 8 --judge 0 --cover-only 1
```

Le positif attendu est reproduit : `F=343`, les trois lanes effectuent
exactement `102900=3*343*100` tests de cover; pour `q=4`, les `Q` locaux valent
231 / 508 / 1455, tandis que le CSR global a degré moyen et maximal 99 et
`C_4=3921225=C(100,4)`. Les seuils locaux n'ont donc retiré aucun candidat du
graphe global, exactement comme le prédit l'inspection.

La même commande révèle une régression indépendante, retour 3. Le mode cover
saute volontairement l'énumération, puis la garde finale exige quand même
`totals[q].candidates==totals[q].bound_c`; elle compare 0 à 4950 dès `q=2`.
Avant cet échec, stdout imprime en outre le faux libellé `catalogue seul`,
`reference=0.000` et un rapport inexploitable. Le juge est bien absent de ce
mode, mais le disclaimer `MODE COVER` n'est jamais atteint. Le seul CTest cover
demande la combinaison invalide `--cover-only 1 --judge 1` et reçoit son rejet;
aucune porte positive n'exerce le mode autonome. Claude doit soit exiger cette
identité seulement dans les modes qui énumèrent, soit comparer en mode cover la
masse analytique à un compteur qui a bien la même sémantique, puis ajouter une
porte positive avec libellé et planchers propres.

### Réponse live de reprise au-dessus de `f37341d`

Le premier delta non committé applique la première option et ferme le défaut
fonctionnel : la même famille de commandes retourne désormais 0, décide 2/2
nuages, compte 205 800 tests et atteint le disclaimer cover; les trois portes
nouvelles passent. L'audit live conserve séparément les lacunes de résistance
aux mutations des lanes, planchers et libellés :
[`AUDIT_LIVE_REPRISE_COVER_F37341D.md`](AUDIT_LIVE_REPRISE_COVER_F37341D.md).

## Réponse courte aux questions du README

1. Le **lemme** `center-cover` est déjà local par feuille. La capability et le
   générateur **de candidats** publiés sont globaux. Une simple optimisation du
   census peut réutiliser le contrat; toute baisse revendiquée de `C_q` exige une
   variante versionnée et ses propres masses.
2. La construction ne doit pas être déclarée `O(n+F*t_q)` tant qu'une requête
   top-`t_q` exacte ou des bornes sur `V_C/Z_C` ne sont pas prouvées. Elle peut
   être chronométrée séparément et réutilisée sous digest; elle reste incluse
   dans le coût et la mémoire bout en bout.

GCP non utilisé.
