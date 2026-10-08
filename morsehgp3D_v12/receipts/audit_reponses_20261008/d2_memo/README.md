# CST-0104 : portes natives D2/MEMO désormais acquises

La mention « porte native du mémo non acquise » est périmée pour **l’étage G
livré**. Les témoins `witness_d2` et `witness_memo`, leur adaptateur et
`resolve_part` sont présents depuis `99fa83246` (7 octobre, 21:32:15 UTC),
identiques aux sources du snapshot `d2f39fe82` joué en session I. Le manifeste
de cette session identifie aussi le harnais et les inscriptions CTest.
Son journal primaire atteste les deux portes **Passed**, 355 et 356, parmi
665 réussites. Ce reçu ne relance aucun test natif ou GCP.

Les corps dépassent une simple identité de nom :

- **D2**, plancher 15 : construction réelle nuage/index/catalogue/G ; appel
  direct du résolveur sur AB sous la jonction ABC ; naissance ZW exigée,
  route certificat+census saturé, un saut et deux contrôles ; même cible
  retrouvée dans la trace AB de la sortie complète de G.
- **MEMO**, plancher 12 : pour Kmax 2 et 3, toutes les cibles produites doivent
  précéder strictement leur jonction ; la résolution de `{0,6}` sous la
  jonction de niveau 4 doit refuser `tower_invariant`, ordre 2. Les planchers
  sont ceux du harnais, pas des nombres de contrôles extraits d’une sortie
  détaillée absente de ce journal CTest.

`unit_support.hpp` construit les objets du produit et appelle sa fonction
`resolve_part`. Celle-ci initialise `Previous` au rang de la jonction, vérifie
la première plus petite boule et chaque descente **avant** un saut ; le
succès direct de la table de naissances passe aussi par ce contrôle. Le test
n’est donc pas une imitation du résolveur. L’arrêt à la première cellule de
fenêtre conserve son genre dans la cible.

Les petits calculs **Fraction indépendants** retrouvent les prémisses :
D2 a `β(AB)=64`, `β(ZW)=1`, niveau de jonction `1681/25`, prédécesseur
positif de Cat₂ égal à `41`. AB a p=2 et qmin=2, donc est hors de Cat₂.
Ainsi `64 < 1681/25` autorise la trace alors que la garde supplémentaire
`64 ≤ 41` la rejetterait à tort. Pour MEMO, `β({0,6})=9 > 4 > 1` : une
cible terminale née au niveau 1 ne rend pas la partie initiale valide au
niveau 4. L’identité d’un objet cible ne transporte pas sa date d’usage.
Le modèle de disques critiques utilisé pour retrouver 41 est exhaustif sur
les paires/triples de ces cinq sites plans, avec barycentres stricts ; il ne
prétend pas reconstruire la tour native.

**Limite de clôture.** L’obligation de graver les deux portes natives est
satisfaite. Le [reçu T2 antérieur](../../audit_t2_20261007/mathematiques/README.md)
demandait aussi un mutant « date terminale » : cette preuve causale native
reste absente. Les sept mutants historiques et les
[18 du prototype Gc rebasé](../../audit_reponses_20261007/gc_mutants_finaux/README.md)
visent d’autres règles ; aucun ne sélectionne D2/MEMO comme juge et aucun
ne remplace la garde initiale par une date terminale. Ils ne doivent pas
être invoqués pour clore cette sous-obligation. Le lot Gc est un prototype
séparé, pas une nouvelle livraison G. Les rapports primaires sont hachés
et peuvent être revérifiés avec `--scratch` ; aucune trace de CHECK précise
n’est inventée à partir du seul verdict de campagne.

Mise à jour proposée : **porte native D2/MEMO acquise**, avec la portée
ci-dessus ; conserver explicitement le mutant de date comme restant si
CST-0104 englobe cette obligation historique. Les futurs lecteurs T/M/V/R
et attaches à coupe fermée doivent encore prouver leurs propres dates.
Ni cette porte ni l’identité des cibles G ne leur transfèrent de qualification.

[Résultats](results.json), [pins](pins.json). Rejeux Python normal/−O :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/d2_memo/check.py --check
python3 -O -B -S morsehgp3D_v12/receipts/audit_reponses_20261008/d2_memo/check.py --check
```

`--repo-root` sélectionne le dépôt contenant les objets Git ; `--scratch`
permet en plus de vérifier les trois petits rapports/manifestes externes
épinglés, sans exécuter leurs campagnes. Sources et journaux volumineux sont
réutilisés depuis Git. Aucun fichier du registre ou du produit n’est modifié.
