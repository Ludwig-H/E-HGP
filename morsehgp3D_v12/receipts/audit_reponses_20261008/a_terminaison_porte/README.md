# CST-0241 : protocole de porte native déterministe

Proposition du 8 octobre 2026, **non implantée, non compilée, non exécutée**. Pin de lecture :
`a785b9ef33aad30a825999c02bcd5fcf13f3846b`. Le corps de `pipeline_run.cpp` est encore celui de
`86d7e39d834cae86768d49a23d83bf53454ec2a3`, sans le correctif. Le [reçu initial](../a_terminaison/README.md)
conserve le cycle SC de seize transitions, le patch `last` et le modèle borné : rien n'y est réécrit.
Ce protocole vise une propriété précise du correctif, **pas le cycle complet ni la correction FULL**.
Aucun blocage natif n'est observé ici ; aucun lien causal avec le timeout MES-M0 n'est établi.

## Corps effectivement visé

`src/tower/pipeline_run.cpp:436` annonce un participant avant le scan ; après un scan infructueux,
le retrait est ligne444 et le test de sortie ligne446. La boucle d'attente commence ligne449.
`tests/tower/pipeline_unit.cpp` confronte déjà la Session à la voie séquentielle, mais ne commande
pas cet entrelacement. Les portes de panne d'allocation ne le commandent pas davantage.

Le correctif visé mémorise `fetch_sub(1, acq_rel) == 1`, puis autorise la sortie si ce dernier
retrait est suivi d'un scan sec vide. Une réannonce ultérieure ne retire pas cette permission.
L'acteur qui se réannonce reste responsable de son propre travail et de son propre retrait.

## Trois points d'observation, uniquement dans la construction du test

Une même macro de test, vide dans le produit, aurait trois appels dans **le vrai corps** :

1. `annonce` juste après le `fetch_add` ;
2. `retrait_vide` juste après le `fetch_sub` infructueux, **après** capture de `last` dans la version corrigée ;
3. `attente` juste avant le `while`, après les deux décisions par scans secs.

Le callback reçoit l'identité du participant. Aucun crochet ne modifie `in_flight`, `epoch`,
les drapeaux de travaux ou la valeur retournée. Il utilise des sémaphores du seul harnais.
Pas de pointeur de callback ou de branche dynamique dans la compilation produit. Trois sites
sont nécessaires pour cet oracle court : avec les deux premiers seulement, l'ancien code peut
attendre sans émettre d'événement, et le juge retomberait sur un timeout probabiliste.

Le dépôt construit `mhgp12` comme archive statique (`CMakeLists.txt:264`) et les unités via
`mhgp12_add_unit` (`cmake/gates.cmake:258`). Une cible dédiée peut compiler cette unité de
traduction avec ses crochets, en s'assurant qu'une seule définition de ses fonctions publiques
est liée. Vérifier la commande et la carte de lien : ne pas tester accidentellement le corps
non instrumenté de l'archive. Le choix exact de ce raccord appartient au développeur ; aucun
montage non compilé n'est livré ici comme porte fonctionnelle.

## État valide et durée de vie

Réutiliser la préparation native de `pipeline_unit.cpp` avec **un site, K1 et un Pool de taille2**.
Créer un `SessionRun`, appeler `open_session`, `admit_session`, régler `pipeline.start`, puis
appeler `run_region` une fois seul, crochets désactivés. Garder tous les propriétaires en vie :
nuage, index, catalogue, budget, Pool et `SessionRun`. Ne pas appeler la clôture qui déplace les
sorties entre cette préparation et les deux fils.

Avant d'armer la porte, exiger : chaque étape présente est `kStepDone`, `g_next >= g_total`,
`in_flight == 0`. La région est alors à une coupe finale réellement construite ; les deux
appels suivants ne trouvent aucun travail. On teste cette coupe interne réutilisée, pas une
nouvelle opération publique FULL. Aucun `Cloud` vide supposé accepté, aucune référence nulle,
aucun `Pipeline` fabriqué avec des objets invalides, aucun producteur concurrent.

Les deux callbacks utilisent des identités distinctes0/1 et des `std::thread` du harnais.
Ils appellent directement `run_region` ; le test ne prétend donc pas qualifier le distributeur
du Pool. Leurs retours sont annoncés au harnais après le retour effectif de la fonction.

## Ordonnancement et assertion sans délai

Les crochets spéciaux ne bloquent que leur **première** visite pour le participant indiqué.
Le callback de retour et le crochet `attente` de A publient chacun un événement distinct dans
un canal synchronisé. Aucun compteur non atomique partagé sans sémaphore.

| Étape du harnais | État assuré |
|---|---|
| Lancer A ; attendre son `retrait_vide`, qui le suspend | A a décrémenté1→0 ; aucune autre annonce |
| Lancer B ; attendre son `annonce`, qui le suspend | B a incrémenté0→1 ; il n'a pas encore scanné |
| Libérer A de `retrait_vide` | A reprend tandis que B reste compté |
| Attendre `retour_A` **ou** `attente_A` | Corrigé : retour. Ancien : attente, annoncée avant d'entrer dans le `while` |
| Si `attente_A`, le garder suspendu ; dans tous les cas libérer B et attendre `retour_B` | B fait son scan vide, retire1→0 et sort ; A n'est pas compté |
| Si `attente_A`, libérer ce crochet et attendre `retour_A` | A voit désormais0 dans l'attente, recommence seul, retire1→0 et sort |
| Joindre A et B ; contrôler leurs issues et `in_flight==0` | Aucun fil ni emprunt laissé actif |
| `CHECK(attente_A == 0 && retours_A == 1 && retours_B == 1)` | Ancien code : échec explicite et fini ; correctif : réussite |

Dans le cas ancien, le second passage d'A dans les crochets est transparent. Le harnais ne
retourne pas prématurément du produit et ne tue aucun fil. La garde CTest externe sert seulement
à contenir un défaut inattendu du test ou une autre régression ; **elle n'est pas l'oracle**.
Tout stockage et toute synchronisation du harnais sont préparés avant les fils. Aucun
`CHECK`, exception volontaire ou retour anticipé ne précède leur libération et leurs joins.
Si la création de B échoue, libérer A de son retrait (le compteur est encore0), attendre son
retour et le joindre avant de signaler cet échec de préparation ; ne pas attendre `annonce_B`.
La réversion du seul correctif `last` doit produire cette assertion précise, pas un crash,
un échec de préparation ni un timeout. Les sorties d'échec doivent conserver ce motif causal.

Les sémaphores imposent un entrelacement déjà permis en SC. Ils ajoutent des relations
happens-before, mais ne publient aucun travail : le compteur1 de B est garanti visible au
test de sortie d'A. Ils ne changent pas la valeur0 enregistrée au retrait antérieur d'A.
Cette porte est donc un témoin de la décision native après réannonce ; le cycle infini reste
prouvé séparément par le reçu initial. Elle ne qualifie pas tous les entrelacements faibles.

## Terminaison de la queue vide pour N participants

Hypothèses : nombre fini N de participants, aucun nouvel appel ajouté, équité faible,
opérations atomiques et scans locaux terminants ; après stabilisation, plus de travail ni
publication, `epoch` constant et scans claim/sec vides. Dans le raisonnement C++, cette
dernière propriété exige aussi la visibilité des états finaux : les RMW `in_flight` en
`acq_rel` transmettent les publications faites avant les retraits. Le petit modèle SC ne
prouve pas à lui seul cette relation mémoire ni le progrès du matériel.

Supposons qu'aucun participant ne sorte plus. S'il n'y avait plus de passage du compteur
à zéro, chaque participant déjà entre un ancien contrôle et sa réannonce pourrait encore
s'annoncer au plus une fois ; après un retrait non dernier, il attendrait ce zéro puisque
`epoch` ne change plus. Ces annonces en suspens sont en nombre fini. Les participants
annoncés terminent leurs scans et leurs retraits sous l'hypothèse de progrès : le compteur
doit donc atteindre zéro, contradiction.

À ce passage à zéro, le dernier retrait conserve `last=true`. Son scan sec est vide :
son retour reste autorisé même si d'autres participants se réannoncent avant ce scan.
L'équité faible assure qu'il achève ce nombre fini d'opérations. Il y a donc un participant
vivant de moins. L'induction sur N donne la terminaison de tous les appels. Cette preuve
porte sur la **queue vide stabilisée** ; elle n'est ni une borne de latence, ni une propriété
wait-free, ni une preuve de progression du DAG tant qu'il produit encore des travaux.

## Relecture et limites

Lecture seule des sources ; aucun moteur, compilateur, test natif, TSan ou GCP exécuté.
Le lecteur Python vérifie uniquement les pins Git et les sites du protocole :

```sh
python check.py /workspaces/E-HGP
python -O check.py /workspaces/E-HGP
```

La contrelecture indépendante de l'auditeur E confirme le nettoyage et la portée des
sémaphores en lecture statique seulement. La porte native reste à intégrer, compiler et
faire échouer causalement sur l'ancien retrait avant toute revendication de qualification.
