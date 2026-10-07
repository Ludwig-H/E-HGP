# M5 : lecteur strict et admission de capacité

Pin `1f7642e105aebd76632c58c63fdfd5b5c0824779`. Corrections livrées par `1b40c0411`.
Contrôles synthétiques CPU Release ; aucun GPU, GCP, nuage réel, sanitizer ou parcours massif.

**CST-0222 clos dans le microbanc partagé.** `tasks_of` calcule le plafond en u64,
sans addition susceptible de déborder. Le scan, l'émission et la racine utilisent
cette fonction. Le vrai `EmitKernel` et `child_fields` concordent avec le calcul
entier indépendant sur **520 petits enregistrements**, dont tout l'intervalle
`0xfffffe00..0xffffffff` et les bornes de paquets. Les sept comptes du témoin
historique sont inclus. Le helper est aussi exact à `UINT64_MAX` ; cette borne
ne prétend pas être une population admise par le microbanc.

La garde du vrai `Driver::run` est exercée par **six totaux injectés** : petit
contrôle, limites parents/tâches et leurs dépassements séparés ou conjoints.
Un exécuteur factice intercepte la première réservation suivant les totaux.
Les trois dépassements rendent `kStatusCapacity`, livre et sorties vides,
**avant toute réservation de tampons par l’exécuteur, Scatter ou Emit**. Les trois autres valeurs
atteignent la réservation, immédiatement interrompue par le témoin. Ces totaux
éprouvent le domaine de la garde ; aucun arbre géant censé les produire n'est
fabriqué. Le diagnostic `stats.push_back` peut encore allouer avant la garde ;
cette porte ne qualifie ni toutes les allocations ni le budget Session.

**CST-0223 clos pour les invariants déclarés du lecteur.** L'ancien témoin
[`format_probe.cpp`](../../audit_b_m5_20261007/juge_format/format_probe.cpp)
est recompilé contre les en-têtes corrigés ; seules les quatre admissions
attendues deviennent des refus. Les **18 résultats** sont conformes : les quatre
anciens défauts sont refusés, les six corruptions classiques et la taille géante
annoncée restent refusées, les six contrôles du comparateur restent conformes.
La porte livrée `format_selftest.cpp` passe également ses **21 cas**, dont bornes
des tests G1, enveloppes exactes, feuille non terminale et addition à la borne u64.
Le témoin de somme débordante est arrêté dès le compte impossible d'un nœud ;
la fonction d'addition est donc testée séparément, sans milliards de nœuds.

Un lecteur de format ne recalcule pas tout G1. La clôture concerne les refus
constatés et les invariants documentés, sans transformer une référence lue en
preuve géométrique autonome. Le juge d'adoption reste suivi en `CST-0018`.
Le code de parcours partagé devra encore être qualifié sur l'appareil et dans
le catalogue produit ; `CST-0211` sur le budget global reste ouvert.

Rejeu :

```sh
python3 -B morsehgp3D_v12/receipts/audit_juges_emst_20261007/m5/check.py
python3 -B -O morsehgp3D_v12/receipts/audit_juges_emst_20261007/m5/check.py
```

Trois petites unités compilées par mode, supprimées ensuite. Dix-huit sources
sont comparées au pin et hachées avant/après ; les neuf dépendances locales
effectivement compilées (témoin compris, hors TU historique transformée) sont
contrôlées par `-MMD`. La version du compilateur et les empreintes des binaires temporaires et
du témoin historique transformé sont publiées ; aucun binaire ni copie de
source produit conservé. Normal et `-O` donnent le même résultat octet pour octet.
