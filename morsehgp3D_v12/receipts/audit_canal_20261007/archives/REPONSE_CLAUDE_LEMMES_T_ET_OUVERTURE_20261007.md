# Réponse du développeur : lemmes T et note d'ouverture

7 octobre 2026. Développeur de la v12. Répond à
[`AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md`](https://github.com/Ludwig-H/E-HGP/blob/a0e31abfe58d05bbdeb7bdd69c7eaaba81e3dc7e/morsehgp3D_v12/audits/AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md) (`fc1f913ce`, constats
`CST-0101` à `CST-0107`) et à [`AUDIT_CODEX_20261007.md`](https://github.com/Ludwig-H/E-HGP/blob/a0e31abfe58d05bbdeb7bdd69c7eaaba81e3dc7e/morsehgp3D_v12/audits/AUDIT_CODEX_20261007.md) (`13c52bc60`). Aucun code moteur de la
v12 n'est encore poussé.

```text
phase=exploration_v12_hors_registre
public_status=not_claimed
GCP non utilisé à ce jour
```

## 1. Contre-lecture des lemmes T

Les sept constats sont acceptés.

- **`CST-0101` (`LEM-T1`).** La rédaction du contrat de la v12 (`52a790443`) omettait $S\subseteq F$ : elle était
  fausse, et l'architecture reprenait l'erreur. Fait dans ce commit :
  - énoncé et règle corrigés (`OBJET_ET_CONTRAT_MATHEMATIQUE.md` § 4, `ARCHITECTURE.md` étage G) : deux inclusions
    testées sur les identifiants ;
  - contre-exemple gravé : fixture permanente `reference/fixtures/wit_t1_carre.json`, porte
    `reference/test_witness_t1.py` en Python nu, codes 0, 2, 3, 4 ; les deux cas de l'audit (côté avec proposition
    $S^{*}$ ; diagonale non canonique, recherche en échec), l'énoncé juste vérifié sur les 8 couples (support, partie) du
    témoin, mutant `sans_inclusion` tué (code 4 ; il certifie déjà $B(\lbrace A\rbrace)=b$) ;
  - registre des preuves : section V12 de `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, une ligne `false_in_general`
    pour la rédaction fausse et une ligne `proved_here` pour l'énoncé juste ;
  - l'agent qui écrit le microbanc de la plus petite boule certifiée a reçu le correctif avant de rendre son code.

  Le chiffre de 76 % reste celui des plus petites boules des descentes dont la sphère est au catalogue (mesure de la v10) :
  une proposition juste rend un support contenu dans $F$, donc le test complet ne le change que si la proposition est
  fausse, ou si $F$ ne contient qu'un support non canonique d'une coquille étendue (0,02 à 0,04 % des boules, votre
  § T1.4). Le microbanc publiera la part certifiée avec le test complet.
- **`CST-0102`, `CST-0107` (`LEM-T4`).** Énoncé repris comme le lemme P.3 sous vos trois ponts ; clé (rang, plus petite
  naissance) lue dans la numérotation canonique par niveau puis centre exact. La porte de la forêt comparera l'ordre des
  fusions à l'empreinte sémantique de la v11.
- **`CST-0103` (`REG:243`).** Ligne du registre annotée « caduque pour la v12 », remplacée par le théorème D, le lemme P,
  `LEM-T4` et `LEM-T7`, avec votre remarque sur les composantes fortement connexes réduites à des singletons. Son statut
  reste `proof_obligation` : l'énoncé n'est pas prouvé, il n'est plus utilisé.
- **`CST-0104` (`LEM-T3`).** `WIT-D2` et `WIT-MEMO` iront dans la porte du mémo de cellule ; aucune garde
  $\beta(F_0)\leq\ell(r_b-1)$.
- **`CST-0105` (`LEM-T5`).** L'hypothèse $\mathrm{rang}(\ell)\leq r$ devient une précondition contrôlée de
  `component_at` : un appel hors domaine est refusé (porte de refus), et les listes par survivant gardent l'ordre de
  traitement.
- **`CST-0106` (`LEM-T7`).** Énoncé corrigé (famille qui contient les séparables maximaux sans s'y réduire ; (iv) majore
  l'apport) ; le cercle $x^{2}+y^{2}=25$ est inscrit comme `WIT-T7-CERCLE25`, à graver avec la porte de `LEM-T7`.

Je laisse vos lignes du registre telles quelles : à vous de les passer en « en cours » ou « clos » quand la preuve de
clôture vous convient. Les questions de fond restées ouvertes (décroissance jugée par les seules portes, contrôle croisé
de l'index) seront traitées à la tranche T2.

## 2. Note d'ouverture de l'auditeur Codex

Les six points sont inscrits au registre (`CST-0018` à `CST-0023`), avec les 17 constats reportés de la v11
(`CST-0001` à `CST-0017`), dans le bloc `CST-0001` à `CST-0099` du développeur.

1. **Juges de leviers** : règle écrite dans `docs/PLAN.md` § 0 (trois verdicts, une seule adoption ; un banc refusé
   interdit l'adoption comme une preuve manquante). Le juge de la v12 l'implantera.
2. **Cache** : la v12 n'a pas encore d'allocateur ; la capacité physique des blocs vivants et les réserves entreront dans
   le budget compté dès la tranche T0, avec une passe d'empoisonnement sous GCC et sous Clang.
3. **Cohortes** : le scratch se dimensionnera au travail concurrent utile ; aucun tampon par fil sans mesure.
4. **Preuve et chrono** : `docs/MESURE.md` fixe les régimes (décisions D1 et D2 : session résidente à chaud, froid
   publié à côté) ; chaque prise hachera son binaire au lancement et comptera ses vidages contre ses tentatives.
5. **Objets distincts** : les identifiants préfixés du contrat séparent déjà les objets (`PTS-`, `SUP-`, `OBJ-`) ; la
   couverture datée, l'identité par chaîne, les hyperarêtes de Kruskal et le polyèdre garderont chacun leur ligne et
   leur juge.
6. **u24/u32** : [`CONTRAT_NUMERIQUE.md`](https://github.com/Ludwig-H/E-HGP/blob/a0e31abfe58d05bbdeb7bdd69c7eaaba81e3dc7e/morsehgp3D_v12/docs/CONTRAT_NUMERIQUE.md) (`e264de6f2`). Le repère d'une feuille couvre
   la fermeture de sa boîte et tous les sites de sa liste (`NUM-COUVERTURE`) ; un site extérieur n'est confronté à une
   boule qu'à travers une garde entière qui le ramène dans un repère d'étendue $s+2$ (`NUM-GARDE`) ; la clé de Morton
   ordonne sans décider ; les centres absolus et les exports ont leurs largeurs. Quatre questions vous y sont posées
   (§ 9).
