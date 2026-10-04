# Audit de conception : arbre K et supports positifs

4 octobre 2026, sources [épinglées](SOURCE.json) ee2b/57dd. Les rapports du
workflow `wf_a7dbdf1a-21c` sont des captures de lecture WIP, prises avant
la spécification finale. Aucune nouvelle sortie native n'est qualifiée ici.

La sortie demandée — arbre K, toutes les boules critiques de sa fenêtre,
tous les supports positifs minimaux par inclusion — est cohérente avec
la forêt existante. Un atome (boule,Q) a un propriétaire unique au seuil
fermé de la boule, après clôture de tout le plateau. Les listes propres
partitionnent ces atomes ; leurs unions de sous-arbres sont emboîtées.
Les géométries de branches peuvent se recouvrir : ce ne sont pas des
partitions de points, ni une reconstruction de ΓK par intersection.
Les boules de continuation et leurs niveaux doivent rester disponibles.

Quatre sous-revues fermées apportent des gardes utiles au futur port :

- [Famille Q_b et événements faibles](qb/README.md) : cube avec quatre
  diamètres et deux tétraèdres stricts malgré qmin2 ; triangle équilatéral
  K2 avec fusion q3 à8/3, exclue par le prédicat strong de l'export points.
- [Parties, cofaces et incidences](incidences/README.md) : C(m,K-p)
  compte les parties comprimées contenant I, pas toutes les K-parties
  fermées. La formule par support compte les cofaces contenant Q ; leur
  somme compte des incidences, qui peuvent partager une même coface.
- [Plateau exact K5](plateau/README.md) : chaque boule de face a trois
  traces comprimées mais six K-parties et trois nouveaux sommets. Les
  unions effectuées par cellule sont2,2,1,0 suivant sa place ; leur rôle
  opérationnel ne définit pas une multiplicité intrinsèque des liaisons.
- [Carrier et stabilité](carrier/README.md) : Q_b est canonique comme
  famille géométrique, mais sa réalisation peut être discontinue près
  d'une dégénérescence. Cela ne contredit pas la stabilité de FULL ; les
  garanties de robustesse du tokenizer doivent nommer leur représentation.

Le libellé `kparts=C(m,K-p)` et «nouveaux sommets=C(m,K-p)-S» du rapport
forêt WIP sont à corriger avant reprise dans le format. Le produit actuel
n'est pas accusé de cette confusion. Les choix de coupe et d'attribution
fermée sont déjà prévus favorablement dans ce rapport ; les gardes servent
à qualifier leur future mise en œuvre.

`check.py` vérifie les inventaires complets et rejoue quatre modèles
stdlib/Fraction, normal et -O : **1365 gardes**, sorties conservées
identiques, une mutation scalaire à poids nuls rejetée causalement.

```sh
python3 -B -S check.py
python3 -B -O -S check.py
```

Périmètre : modèles exacts sur petits ensembles, lecture de sources et
propositions. Aucun C++/CUDA, build, fit, archive de points, exécution GCP,
mesure temporelle, qualification statistique ou garantie native nouvelle.
Les recettes de transfert ne remplacent pas les futurs différentiels
contre FULL et les portes d'échelle de l'export demandé.

Le [premier échec de collecte](attempts/README.md) est conservé : ajout du
stdout entre deux rejeux sans refermer l’inventaire. Le collecteur est
corrigé ; la clôture finale rejoue les deux modes sans écriture interne.
