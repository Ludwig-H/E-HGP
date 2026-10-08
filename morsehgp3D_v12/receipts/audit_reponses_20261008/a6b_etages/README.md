# A6b2 : queue raccourcie, goulot déplacé aux petits ordres

8 octobre 2026, Codex. Source A6b `f2c106d93`, témoin R1 `47feedc96`.
Contre-analyse des postes de la deuxième campagne A6b ; l'admission des sources,
cohortes, statistiques contractuelles et arrêts appartient au reçu de session.
Ici : sources Git et JSON de mesure seulement, sans moteur, compilation, GCP,
contrôleur, coordonnées ni identifiants de points. Aucune adoption proposée
contre le verdict de la campagne.

## Décomposition sans additionner des médianes

Le lecteur relit **63 processus / 1 206 FULL**, y compris le témoin A/A.
Le diagnostic apparié retient **261 paires avant/après** : 45 par trame ng
(5 processus × 9 passes chaudes), puis 126 grandes
(6 processus × 21 secondes visites de trames). Il garde tous ces couples,
sans retirer d'observation. Le froid est exclu exactement comme dans le pilote.

Pour chaque passe :

`mur = P + C + ouverture_tour + G_régional + queue + reste`,

avec `G_régional = etapes_ns.G − g_ns.ouverture` et
`reste = mur − P − C − G − queue`. Les ouvertures de l'appareil et lectures
d'entrée restent hors FULL. La somme est vérifiée en entiers pour chaque
passe puis pour les différences appariées. Le tableau donne les **moyennes
arithmétiques de ces différences**, après moins avant, en millisecondes :

| Cohorte | Δ mur | Δ P | Δ C | Δ ouverture | Δ G régional | Δ queue | Δ reste |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | +0,367074 | +0,102530 | +0,115196 | +0,215037 | +0,883471 | −0,949731 | +0,000572 |
| ng01 | +0,447039 | +0,127124 | +0,027040 | +0,176943 | +0,610208 | −0,496349 | +0,002073 |
| ng02 | −3,436301 | +0,084648 | +0,107373 | +0,296005 | +1,515694 | −5,440473 | +0,000452 |
| grandes | −25,687734 | +0,161716 | −0,184893 | +0,600037 | +4,392601 | −30,657679 | +0,000485 |

Les écarts d'arrondi du tableau n'affectent pas l'identité entière dans
`results.json`. Ce tableau explique les postes ; **il ne remplace pas** les
médianes par processus, rapports géométriques, IC et veto A/A du protocole.
Il confirme une hausse avant la queue sur les petites trames, mais **pas
uniquement dans G** : P, C et ouverture bougent aussi. Une mesure temporelle
globale ne localise pas à elle seule la cause dans N, H ou les aides.

Les sommes de fenêtres des tâches G varient de −5,191 / −1,240 / +8,901 ms-fils
sur ng00/01/02, alors que la durée régionale de G augmente partout. Le quotient
`Σ fenêtres G / Σ G_régional` passe de 43,19 à 42,06 sur ng00, de 42,96 à
42,02 sur ng01, et de 43,58 à 41,41 sur les grandes. Il décrit la concurrence
moyenne des fenêtres G dans cet intervalle ; **ce n'est pas une utilisation CPU**,
car une tâche préemptée compte son attente. Les temps CPU FULL moyens augmentent
de 32,249 / 29,497 / 39,716 ms-CPU sur ng00/01/02 et de 14,529 ms-CPU sur les
grandes. Le parallélisme et la position des tâches changent sans que ces nombres
isolent un coût algorithmique précis.

## Ce qui finit désormais en dernier

Par passe, on compare les cinq dates publiées par ordre : dernier calcul de G,
fin du noyau, M, V, R. Il n'y a aucune égalité au maximum dans ce lot.

| Grandes, 126 prises par bras | Avant | Après |
| --- | ---: | ---: |
| Dernière fin publiée R(K5) | 126 | 0 |
| Dernière fin publiée R(K2) | 0 | 64 |
| Dernière fin publiée R(K3) | 0 | 59 |
| Autres dernières fins | 0 | 3 : R(K1), V(K2), V(K3) |
| Noyau K5 terminé après le dernier calcul G global | 126 | 0 |

Le retard positif moyen du noyau K5 sur la fin globale de G passe de
**26,857 ms à zéro** ; la queue entière moyenne de **43,867 à 13,209 ms**.
Le noyau K5 termine plus tôt de 44,259 ms en moyenne depuis l'ouverture de la
tour. Cela ne signifie ni que toute la queue restante est du noyau ni que
R(K2/3) consomme 13 ms : ce sont des dates de fin, incluant dépendances et
attente. Dans les 45 prises ng00 de chaque bras, le noyau K5 était **déjà** fini
avant G ; le gain sur cette chaîne pouvait donc être masqué dès le témoin.

Borne sur les **dates publiées** : en gelant les autres dates, avancer une
date `a` peut diminuer leur maximum d'au plus
`max(0, a − max(autres dates))`. Après A6b, cette borne vaut zéro pour R(K5)
sur les 126 grandes prises. Ce n'est une borne sur le gain FULL que sous
l'hypothèse supplémentaire `FULL = coût fixé + max(dates) + suffixe fixe`.
Le suffixe doit inclure les épilogues non publiés des tâches, les retraits,
la sortie de région et la clôture, pas seulement les contrôles finaux.

Cette hypothèse n'est pas prouvée par les journaux. Contre-exemple abstrait,
non observé dans la campagne : R5 publie sa date à 10 puis a encore 90 unités
d'épilogue ; un autre ordre publie sa fin à 20 et termine. La région finit à
100. Avancer seulement le préfixe de R5 de 10 unités avance sa date à 0 et sa
fin réelle à 90 ; l'autre date et le coût de clôture sont inchangés, mais le
mur gagne 10 malgré une borne nulle sur le maximum des dates. Le placement
des horodatages dans `complete_step`, avant le retour complet du travail,
impose de distinguer ces deux quantités.

Changer le travail de K5 peut aussi libérer des ressources et déplacer les
autres dates. La suite doit donc suivre K1–3, les retours effectifs des tâches
si nécessaire, et le mur FULL ; le déplacement des dernières dates constitue
un signal de diagnostic, **pas une preuve que K5 ne peut plus améliorer FULL**.

Les trois exemples détaillés du reçu sont `kitti_ng_02_001606`,
`kitti_ng_00_001896` et `kitti_ng_08_002119`. Ils sont nommés sans les qualifier
de trame médiane ou maximale ; ces qualificatifs dépendent de la statistique
et du bras, traités dans l'admission de session.

## Réclamation de G et achèvement de G sont deux événements

Dans `find_g`, `g_next` est incrémenté **avant** `run_g`. Si deux participants
réclament les deux dernières tranches, `g_next == g_total` alors que leurs
calculs peuvent être encore en vol. Un troisième participant peut prendre
une aide. Le compteur `hints_during_g` teste seulement
`g_next < g_total` à l'entrée du travail d'aide : son zéro ne démontre donc
pas l'absence d'aides pendant ces calculs. De nouvelles étapes peuvent aussi
devenir prêtes après le balayage de `find_step`.

Les JSON FULL de cette campagne **n'émettent ni** `hint_jobs_during_g`,
`hint_jobs`, `hinted_reps`, **ni les durées séparées de N et H**. `T` regroupe
naissances, feuilles, noyau et historique. Les reprises/arrêts du noyau sont
publiés, mais ne comptent pas les aides et ne localisent pas le temps N/H.
La campagne ne permet pas de mesurer rétrospectivement la concurrence des
aides avec les dernières tranches G. Le texte « aucune aide ne commence
pendant G » est plus fort que le mécanisme et que la preuve disponible.

Un signal plus fort existe déjà :
`g_end_ns.load(std::memory_order_acquire) != 0`. Dans le pin examiné,
chaque corps de calcul G termine puis participe à `g_computed.fetch_add(acq_rel)`.
Ces RMW forment une chaîne d'acquisition/libération ; le dernier publie
`g_end_ns` avec `release`. Une lecture `acquire` de sa valeur positive observe
donc la fin de **tous les corps de calcul G**. Les maxima écrits avant les
comptages sont également publiés. Le `Pipeline` est recréé pour chaque appel
et initialise ce champ à zéro : aucun signal chaud d'une passe précédente.

Cette valeur positive **ne signifie pas** que les prépasses de feuilles,
drapeaux de tranches, notifications et autres épilogues de `run_g` soient
terminés. Elle ne certifie pas non plus le succès : `note_g_end` précède
l'enregistrement d'un éventuel refus, puis les contrôles globaux viennent
après la région. Les gardes des feuilles et la fermeture du noyau restent
nécessaires. Ajouter cette condition avant `find_hint` serait donc un levier
d'ordonnancement à qualifier, pas une nouvelle barrière de validité globale.

## Expérience utile, sans gain préjugé

Pour une variante déclarée avant mesure : ne changer d'abord que l'éligibilité
des aides par ce signal, conserver les autres dépendances et compter les
démarrages **avant publication de la fin G** séparément des aides utiles et
des aides sans effet. Une lecture zéro est seulement « fin non encore
publiée », même si les calculs viennent de finir ; nommer le compteur ainsi.
Les prépasses et les étapes N/H gardent leur politique actuelle : cette
variante n'éliminerait pas toute concurrence avec G.

Comparer ensuite mur FULL, G régional, queue, fins K1–5 et temps CPU, sous
la même cohorte et les mêmes seuils écrits d'avance. Si les aides utiles
avant publication étaient déjà nulles, cette garde ne teste pas la cause
supposée de la perte et ne promet aucun gain. Instrumenter N/H séparément
peut alors distinguer la préparation nécessaire du travail facultatif.
Toute instrumentation nouvelle doit avoir son schéma et ses portes ; aucun
compteur manquant ne peut être inventé dans les prises existantes.

Enfin, le bras témoin est R1 : A6b y ajoute l'ensemble N/H/aides et leurs
raccords. Cette comparaison mesure cet ensemble. Elle **n'isole pas** le seul
réordonnancement par rapport à l'ancien A6, mesuré dans une autre campagne.

```sh
python3 -B -S check.py --repo DEPOT --results DOSSIER_T2DA6B_ARCHIVE
python3 -B -O -S check.py --repo DEPOT --results DOSSIER_T2DA6B_ARCHIVE
```

Le primaire est le rapport original archivé de **422 160 octets**, SHA
`23d92706dc9b66cca094537f3b14db5bc71c51bf8e9d3ebecffdc2ad9f967959`.
Le rapport public expurgé a **422 088 octets**, SHA
`f9497277b132145f148b08ece67ac8b2af922e5f30f17034e17aeb1536ae74fc`.
Le lecteur épingle les deux et prouve leur égalité structurée après retrait
des seuls trois champs `construction.binaires.{avant,avant_bis,apres}.chemin` ;
aucune valeur de chemin privé n'est copiée dans le reçu. Les journaux et
`results.json` sont identiques entre les deux captures. Le rapport public
reste dans le dépôt ; `--results` doit viser l'extraction originale admise.

Rejeux normal/−O identiques. Les pins des sept sources et des 126 fichiers de
journaux/stderr sont dans `capture.json` ; les sommes entières, dernières fins,
retards par ordre et médianes de processus sont dans `results.json`.
