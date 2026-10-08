# A repris : corriger la fenêtre du test d'allocations récupéré

**8 octobre 2026, 04:41:43 UTC**, prototype non commis **A/w8da**, base déclarée `8da450ab7`.
`phase=exploration_v12_hors_registre`, `public_status=not_claimed`. [Pins](pins.json) vérifiés avant/après ; aucune
mutation du scratch. Travail récupéré encore en préparation, pas régression livrée ni preuve native.

Le test récupéré `pipeline_fault.cpp` **c92e6dbe…** compte à ses lignes 129–132 un appel `attempt(...,&expected)`
et exige trois allocations levantes. Or `attempt` réalise aussi le digest avant de revenir (ligne 116) :
`full_digest` alloue un `Stream` par `make_unique` (`export_full.cpp:171`), puis `digest_of` construit une
`std::string` de 64 caractères (ligne 106 ; allocation avec la libstdc++ locale). Le compteur contient donc
**au moins les trois allocations de la tour plus celles du digest**. Le test peut échouer au contrôle « 3 », puis
injecter des indices supplémentaires avec digest nul, pour lesquels la tour réussit au lieu du refus exigé.
C'est un défaut du témoin, aucune faute du moteur n'en découle.

Le [patch proposé](compte_tour_seule.patch) calcule d'abord le témoin sans digest, fige le compte, puis produit
l'empreinte attendue dans un appel séparé. Les trois injections et la vérification de reprise restent inchangées.
Il cible uniquement les octets c92e du test sauvé dans le [reçu de reprise](../reprise_sources/README.md) ; ne pas
l'appliquer aveuglément à une réécriture ultérieure. Une version intermédiaire utilisant une injection par taille
avait été observée, mais elle n'est plus la source à cette capture.

Les corrections produit examinées sont présentes dans **cette copie A/w8da** : `open_session` sans `noexcept`
(déclaration et définition), laissant ses deux `make_unique` atteindre le `guarded` public ; `note_g_end`
(`pipeline_run.cpp:204–248`) publie le maximum des fins de calcul G, notées avant les feuilles, via les compteurs
`acq_rel`. La sonde sépare explicitement le schéma recouvert : T/M/V/R sont des sommes de fenêtres murales des
tâches, pas des murs séquentiels. `tour_ns` enveloppe l'appel, et ne doit pas être égalé à la durée interne `fin_ns`.
Ces observations statiques ne prouvent ni exécution des portes ni qualification du nouveau schéma.

Vérification **Python normal et −O**, sans compilation : application du patch sur copie temporaire du test archivé,
SHA du résultat, contrôle inverse. Rejeu :

```sh
python check.py --archive /chemin/vers/sources_t2d.tar.gz
python -O check.py --archive /chemin/vers/sources_t2d.tar.gz
```

L'enregistrement CMake évolue pendant la reprise ; le fichier correspondant est épinglé, sans attester de cohorte
exécutée. Le développeur doit vérifier ses groupes et jouer le témoin corrigé. Aucun natif, benchmark ou GCP lancé.
