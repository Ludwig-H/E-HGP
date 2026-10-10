# CST-0244 : proposition isolée pour l'admission mémoire d'A6c

**Proposition, non intégrée et non qualifiée nativement.** Base `aa6338ee8` ; le
patch s'applique aussi à B3b `81b0883d1`, avec les mêmes postimages. Il modifie
uniquement `pipeline.cpp`, ses commentaires dans `pipeline.hpp`, et le helper
`play` des portes de leviers. Aucun changement de seuil, de sortie géométrique,
de planification ni de suppression du double scan des cohortes n'est proposé.
Seul le supplément N est corrigé : les compteurs ajoutés par A6c et ce double
parcours restent. Aucune équivalence générale de mémoire ou de temps avec R1
n'en découle.

La décision `p.chain[]` et le masque `diag.chain_orders` sont fixés par
`open_session`, avant le calcul de `region_bytes`. Le supplément de numérotation
par morceaux ne concerne que les ordres engagés d'indice `i >= 1` :

```text
supplément N = W × max({0} ∪ {widest[i] : i >= 1 et chain[i]}) × (sizeof(Sphere) + 4)
```

Un fil exécute au plus une tâche de numérotation à la fois ; ses deux tampons
locaux sont au plus ceux de la plus grande cohorte concernée. L'ordre 1 trie les
sites sans tampon de sphères. Les ordres non engagés gardent leur numérotation
séquentielle, dont les sphères sont déjà comprises dans `t_bytes` par
`kernel_bytes`. Tous les autres termes de la borne restent identiques. Quand
tous les ordres sont engagés, le supplément reste celui de la base ; lorsqu'aucun
ordre 2..K n'est engagé, il devient nul. Ce raisonnement porte sur ce supplément,
pas sur une nouvelle preuve de toute la borne ou de l'arithmétique u64.

Au début d'`admit_session`, un changement **effectif** de décision depuis
l'ouverture renvoie `Reason::tower_invariant`, avant `budget.admit`, les espaces
des fils et les tables. Modifier numériquement `chain_sites` tout en conservant
la même décision reste permis. Le helper de test fixe désormais le seuil avant
`open_session`. L'objet interne doit ensuite rester immuable pendant la région ;
ce patch n'ajoute pas un protocole autorisant sa modification concurrente.
Le `Stopwatch` reste tout au début d'`admit_session` : sur succès, la durée de la
garde demeure comprise dans `diag.open_ns`.

Vérification Python normale et `-O` :

- application par `git apply --check`, puis application dans deux copies
  temporaires : A6c et B3b ; aucun fichier produit modifié ;
- motifs uniques des **72 / 74 mutants** conservés, substitutions multiples
  rejouées dans l'ordre ; cela ne prouve pas leur mort après compilation ;
- **27 990 cas de borne**, dont OFF, ON, K1 et masques mixtes artificiels ;
  **2 544 allocations simultanées** modélisées ;
- **525 transitions** : 216 bascules effectives refusées sans appel d'admission,
  309 décisions inchangées admises ; quatre témoins distinguent les variantes
  numériques erronées (supplément inconditionnel, ordre 1 inclus, maximum pris
  sur un ordre inactif, oubli du nombre de fils).

Avant intégration, le développeur devra rejouer les portes natives de budget avec
seuil fixé avant ouverture : OFF, ON, K1, limite `usage + admis`, et bascules
tardives dans les deux sens. Le refus précoce doit préserver usage et pic du
budget ; une modification de seuil sans changement de décision doit passer.
Rejouer aussi les portes de leviers et les mutants : `chaine_jamais_engagee`
garde son ancre, mais pourrait désormais être détecté dès la garde d'admission.
Les portes ON de pénurie et d'injection d'échec de **CST-0245 restent séparées**.
Aucun gain temporel, résultat FULL, fermeture de constat ou adoption ne découle
de cette proposition.

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261010/a6c_admission_proposition/check.py /workspaces/E-HGP
```

`results.json` contient les pins des sources et des postimages, le hash du patch,
les comptes de contrôles et les contre-exemples du modèle. Aucun moteur,
compilateur, test natif ou appel cloud n'est lancé par ce lecteur.
