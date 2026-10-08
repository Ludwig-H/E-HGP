# T1-d : compléter l'identité des prises sous budget

**Proposition pour les futures prises, hors produit et sans campagne native.**
Base `c31beaf2200d1a7f8eed09b0c1c7c0f6af3b74f3`, avec application préalable
obligatoire du [port d'admission `port_c31.patch`](../t1d_admission/port_c31.patch)
publié en `a6e3634fc`. `capture.json` épingle la base, ce prérequis et les
postimages ; `check.py` applique les deux patches dans une copie temporaire.
Le patch incrémental ne s'applique pas seul au c31 brut. Aucun reçu historique,
seuil statistique, résultat T1-d1/T1-d2 ou état de constat n'est modifié.

## Ce que prouve chaque empreinte

`catalogue_probe.cpp:18–20` précise que MHGP12DP ne contient ni les niveaux
ni la table. Les prises `spec_free/spec_budget` ne demandaient que cette
empreinte et les compteurs de tranches. L'identité CPU/appareil séparée
possède déjà des `digest_complet`, mais les succès sous budget n'y étaient
pas raccordés. FUL1 sans ce budget n'atteste pas à lui seul les sorties de
ces prises contraintes.

Le producteur existant suffit : `full_digest_json` hashe tous les mots
numérateur/dénominateur des niveaux publiés, puis les réponses de
`find_support(S*)` pour **chaque support canonique émis**. `table_ecarts`
compte les réponses différentes de l'identifiant attendu. Il s'agit des
représentations exactes des niveaux et des requêtes positives sur la table,
**pas d'une empreinte de son stockage interne ni d'une qualification de
toutes les requêtes négatives**. MHGP12DP demeure nécessaire en complément.

## Correctif proposé

Les trois fichiers Python du patch demandent `--digest --digest-complet`
pour les prises libres et sous budget, et ajoutent les gardes suivantes :

1. La prise libre est reliée au `digest_complet` CPU déjà acquis par l'étape
   d'identité. Le juge existant continue de vérifier CPU/appareil/F2.
2. Chaque succès libre ou contraint porte un complément ; niveaux et table
   sont identiques à la référence, avec `table_ecarts == 0`.
3. Toutes les passes réussies précédant un refus sont vérifiées **avant**
   d'accepter le refus `resource_exhausted/memory_budget`. Un refus dès la
   première passe reste admissible comme refus, sans inventer d'empreinte.
4. Si C réussit mais que sa preuve complémentaire ne s'écrit pas, le juge
   refuse la preuve incomplète. Ce cas n'est ni une identité acquise ni une
   erreur géométrique démontrée. Une empreinte bien formée mais différente
   entraîne un rejet ; un champ mal typé ou absent entraîne un refus.

La sonde arrête `wall_ns` immédiatement après `build_catalogue*`, avant ces
empreintes (`catalogue_probe.cpp:249–277`). Elles restent dans les prises
d'identité/flux ; la commande de la campagne chronométrée ne change pas.
Le lecteur strict garde les clés, types, séquences, métadonnées, comptes,
mémoire et cohortes du port préalable. `RULE['flux']` annonce explicitement
le nouveau contrôle ; tous les autres champs de la règle restent identiques,
dont coût 1,01, A/A ±1,5 %, tours, bootstrap et graine.

## Preuves Python bornées

Les **36 injections préexistantes** passent après le prérequis. La nouvelle
porte permanente `g4_catalogue_t1d_selftest.py` en contient **46**, normales
et `-O` identiques : mutations isolées des niveaux/table/écarts, corruption
du préfixe avant refus, preuve absente, champ booléen, et même erreur dans
la prise libre et tous ses budgets malgré une référence CPU correcte.
Les positifs conservent notamment le refus initial et le préfixe complet
exact suivi d'un refus. La porte vérifie aussi les options commandées.

Six mutations causales du juge réadmettent chacune son contre-exemple tout
en conservant le nominal : retirer la comparaison des niveaux, celle de la
table, le compte d'écarts, la complétude, le contrôle des préfixes refusés,
ou remplacer la référence CPU par la prise libre elle-même. Aucune sonde
n'est invoquée ; les nombres synthétiques ne sont pas des chronos.

`results.json` conserve également une projection vers l'ancien protocole :
les lignes complémentaires et l'option sont retirées du même scénario.
C'est une démonstration d'information absente, **pas une prétendue mauvaise
sortie native historiquement admise**. Le cas de l'écriture de preuve qui
refuse après C reste séparé ; on ne lui invente pas de journal ancien équivalent.

Relecture indépendante de `/root/latest_perf_e` : aucun faux refus repéré
dans les accès aux références, le préfixe vide ou l'échec de preuve après C ;
lecture statique seulement, aucune exécution native.

```sh
python -B -S check.py /workspaces/E-HGP
```

Ce lecteur reconstitue les sources Git, applique et vérifie le prérequis puis
le patch, contrôle les hashes et les options/règles, rejoue les auto-tests
et six mutations en Python normal/`-O`, puis compare à `results.json`.
Il ne lit aucune coordonnée, ne construit rien et ne contacte aucun contrôleur.
Les archives T1-d déjà closes sans ces preuves restent à leur portée initiale.
