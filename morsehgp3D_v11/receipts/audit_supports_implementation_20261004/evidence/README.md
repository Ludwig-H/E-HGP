# L0/S0 : proposition D.2 et portée des comptes

Lecture privée, sans exécution native, compilation, fit, workflow ou GCP. Base Git `f98aeed67d4030dd78e11d5faf7d8556c4d17aaf` ; les documents et l'oracle L0 sont des copies WIP, figées avant lecture. Aucun livrable ni profil numérique n'est qualifié ici.

La copie de [SORTIES.md](sources/SORTIES.md) suit la décision utilisateur **Q_b seul**, sans population stockée, avec tous les supports minimaux sur la coquille complète et refus au-delà de 24 sites. Ses §6 et §10 explicitent les limites de complétude du lecteur, les supports orphelins et l'invariance : réétiqueter PointId change la colonne d'IDs ; permuter les lignes change le hash brut d'entrée du manifeste. La correspondance des feuilles K1 est lexicographique XYZ, distincte de l'ordre Morton des sites, et le singleton K1 ne demande aucune boule positive.

D.2 : `tree_k_sha256` du §8 utilise encore `BallIdx`, absent de SP. Proposition **V2**, distincte du SHA de l'artefact : parents, rangs, kind, enfants croissants ; identité d'une naissance par site K1 ou S* de sa boule, plus contexte géométrique canonique XYZ sans PointId. Le FullDomain conserve S* via catalogue/birth_key ([forest_build.cpp](sources/f98aeed67__morsehgp3D_v11__src__tower__forest_build.cpp), lignes41–49 et102–108 ; [catalogue.hpp](sources/f98aeed67__morsehgp3D_v11__src__catalogue__catalogue.hpp),45–49). SP donne les mêmes champs par parent, postordre et sommes de ball_count/support_count/arity (§6:257–263). Pour un kind1, la première boule propre est celle de naissance : le contrat de l'[oracle figé](sources/l0__morsehgp3D_v11__reference__hgp11_ref__supports.py),346–358 et475–481, impose une naissance unique, les fusions sur des nœuds à enfants, et les internes après le niveau du nœud. Ces obligations seront à requalifier côté natif.

**Caveat FULL** : [full_probe.cpp](sources/f98aeed67__morsehgp3D_v11__bench__full_probe.cpp),40–71, écrit dans MHGP11FUL1 les centres de naissance, pas S*. Le moteur FullDomain peut produire cette signature ; un lecteur du seul ancien FUL1 ne la recalcule pas directement. Une régénération indépendante du census est possible mais dépasse un simple parcours du fichier. Aucun PGCD natif ni changement du dump FUL1 n'est imposé par la proposition.

L'encodage illustratif du modèle est entièrement fixé : `MHGP11TK` (8octets), puis u64LE(version=2,B,K,n,N), puis SHA-256 binaire de `MHGP11GX`+u64LE(B,n)+triplets XYZ u32LE en ordre SiteIdx. Par nœud canonique viennent parent/rank u32LE, kind/arity de naissance u8, ses SiteIdx u32LE, child_count u32LE et enfants croissants u32LE ; pas de padding. Pour une fusion, l'arité vaut0. B sépare explicitement les profils ; XYZ sont des coordonnées entières de grille, pas une signature d'équivalence physique entre origines/pas différents. Le SHA sert d'empreinte structurée, pas de certificat de correction géométrique ni de preuve d'absence de collision.

D.1 : `kparties_reliees.sum` (§8:386) doit se lire **incidences (b,F)**. Pour X={0,1,2}, K2, les populations des boules AB, BC, AC sont AB, BC, ABC. Les comptes1+1+3 valent5 ; il n'existe que3 K-parties distinctes, AB et BC étant comptées deux fois. À l'inverse, les cofaces par boule ont une boule minimale unique ; leur somme compte les événements distincts de W_K, sous la complétude du contrat. L'invariance de kparties sous changement de Q à p,m fixés ne constitue pas une stabilité sous toute perturbation.

[check_d2_signature.py](check_d2_signature.py) : **36 gardes**, sorties identiques sous Python normal et −O, stderr vide et codes0. Trois fixtures : singleton K1, paire K1 avec lex≠Morton, ligne3 K2. Le modèle compare deux lectures de la proposition (SP publié / FullDomain avec catalogue), vérifie permutations/IDs/namespaces et l'arithmétique des agrégats ; il ne joue aucun moteur. Les contrôles AST des trois sources S1 relèvent seulement les attentes205 nuages/935 ordres, planchers et7 mutants, sans importer ni exécuter la suite. Les 12sources inchangées et4drifts WIP sont consignés dans [SOURCE_AFTER.json](SOURCE_AFTER.json), jamais substitués aux copies initiales.

Rejeu autonome :

```sh
python3 -B -S check_d2_signature.py
python3 -B -O -S check_d2_signature.py
```

[RUNS.json](RUNS.json) conserve les commandes et SHA des sorties ; [BILAN.json](BILAN.json) précise les limites. L'inventaire racine exclut uniquement le fichier racine SHA256SUMS ; LEDGER inventorie les payloads sans lui-même ni cet inventaire racine.
