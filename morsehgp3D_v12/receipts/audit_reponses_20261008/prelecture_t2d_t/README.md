# Prélecture T2-d-A : diagnostic historique T et conditions du recouvrement

**8 octobre 2026, 03:19 UTC. Lecture seule, aucun nouveau chrono, build, moteur ou GCP.**
Pas de constat contre un prototype encore en préparation : `RAPPORT.md` de A contient seulement
l'ouverture de 03:08, base `8dc5d6b16`. `pins.json` conserve les hashes des consignes A/communes,
du rapport, du script historique et des sources lues. Les neuf sources sont identiques entre
la livraison TMVR `7398aed7d` et `039b2657e` observé. Aucun payload ni journal massif copié.

## Ce que les traces permettent d'attribuer à T

`temps6_f1`, script du 8 octobre : **un fil**, affinité CPU 0–2, hôte local partagé,
cibles v12, minimum de trois répétitions. Le commentaire « 1 et 3 fils » du script ne
remplace pas sa boucle effective `for F in 1`. La sonde minimise chaque champ indépendamment
(`tower_dumps.cpp:229–246`) ; on ne somme pas ces minima pour reconstruire une prise.
Ces durées murales locales ne sont ni des CPU·s ni la ventilation des nouveaux FULL G4/W48.

| Trace / ordre | Numérotation | Pré-passe | Noyau | Histoire |
| --- | ---: | ---: | ---: | ---: |
| ng00 K5, ordre 2 | 13,517 ms | 1,805 ms | 4,988 ms | 1,368 ms |
| ng00 K5, ordre 5 | 5,421 ms | 6,327 ms | 20,034 ms | 6,126 ms |
| ng00 K10, ordre 2 | 14,252 ms | 1,716 ms | 5,074 ms | 1,447 ms |
| ng00 K10, ordre 10 | 6,235 ms | 18,892 ms | 55,899 ms | 21,976 ms |

Le noyau du grand ordre est le coût séquentiel dominant **dans ces traces** ; l'histoire pèse
également. La numérotation de l'ordre 2 est une autre limite : 18 222 cohortes multiples,
taille maximale 24 à K5. Ses chronos incluent allocations, numérotation et copies ; ceux du
noyau incluent allocation/initialisation de l'union-find. La libération des feuilles est entre
les chronos noyau/histoire mais dans T. Les champs de pré-passe sont des sommes de durées de
morceaux, à ne pas transformer en mur à plusieurs fils.

Aujourd'hui, `run_kernels` impose trois barrières globales : numérotation par ordre, pré-passe
par morceaux, puis noyau+histoire par ordre (`forest_build.cpp:171–176`). Les mesures FULL K
ne publient que l'enveloppe T : aucune part exacte de leurs 26–38 ms ng n'est déduite ici.
A vise déjà le recouvrement et le parallélisme ; ne pas proposer une seconde refonte concurrente.
Une variante bornée à envisager si le profil du noyau le confirme : séparer le tableau `parent`
u32 des trois champs `size/last/minleaf` de `UnionCell`. `find` ne lit/modifie que `parent` ;
les unions gardent exactement leur ordre, leur règle par taille et leurs événements. Les tableaux
restent 4B+12B octets avant arrondis, contre 16B ; admission/cache et préchargements à requalifier,
aucun gain cache/temps acquis. Cette variante n'est pas la pré-passe déjà portée le 7 octobre.

## Dépendances et risque précis d'orchestration

Le Pool de Session n'accepte **qu'un `parallel_for` actif** : appel concurrent ou réentrant
refusé `pool_busy` (`sched.hpp:38–45`). Une tâche G ne peut donc appeler tels quels les étages
M/V/R actuels, qui lancent eux-mêmes ce même Pool. Le recouvrement demande une orchestration
coopérative unique ou des corps de tâches factorisés ; pas de Pool imbriqué implicite.

Le DAG à préserver est plus précis qu'une barrière sur toute la tour :

- Pour T(k), les métadonnées et la permutation des naissances doivent être prêtes ; chaque
  tranche de cibles G doit être entièrement publiée avant consommation, avec visibilité mémoire.
  Le propriétaire T consomme dans l'ordre des cellules/rangs, afin que chaque cible cellule de
  rang antérieur ait déjà son `element`. La fin d'un plateau ne peut être supposée à une frontière
  de morceau arbitraire avant sa contraction M.
- V-naissances(k) requiert M(k−1), M(k). V-fusions(k) requiert en plus les images des naissances
  de k ; il lit `lower[minleaf]` (`vertical_images.cpp:83–94`). R(k) utilise T/M(k), pas V(k).
  V et R peuvent lire les mêmes forêts immuables, avec sorties et compteurs distincts.

**Mémoire :** `MemoryBudget::admit` n'est pas une réservation. Sa promesse suppose qu'aucune
allocation d'un autre étage ne consomme le même budget pendant l'étage (`buffer.hpp:107–121`).
Les compteurs atomiques protègent la limite, pas cette promesse préalable. Pré-admettre les
coexistences G/T/M/V/R et les files bornées, puis affecter leurs tampons avant le travail
concurrent, est une solution explicite ; additionner les admissions indépendantes n'en est pas une.
Conserver les tampons publiés jusqu'à l'acquittement de leur dernier consommateur. Les erreurs
ne doivent ni laisser T attendre une tranche jamais publiée ni libérer son stockage trop tôt.
Les compteurs physiques doivent rester par tâche/propriétaire avant réduction ; aucun compteur
partagé incrémenté sans protocole, aucune double comptabilisation des tâches aidées par T.

## Nouvelle frontière de mesure à déclarer

Les gardes du lecteur K, `P+C+G+raccord+TMVR <= wall` et `T+M+V+R <= TMVR`, reposent sur
les fenêtres **séquentielles de c403**. Elles ne se transfèrent pas au pipeline. Conserver ce
lecteur pour l'archive K ; publier un schéma identifié et des fenêtres vérifiables pour A.

Mesurer une enveloppe unique de G au dernier résultat T/M/V/R disponible, raccord et
attentes compris ; si P/C restent séquentiels avant elle, vérifier `P+C+pipeline <= wall`.
Les durées actives par tâche et leurs sommes sont des diagnostics séparés, éventuellement
supérieurs à l'enveloppe. « T non recouvert » doit être défini par ses intervalles effectifs,
pas par soustraction de médianes ni de sommes de tâches. Juger l'adoption sur le mur FULL
apparié préannoncé ; conserver les CSR R dans les preuves d'identité, FUL1 ne les écrit pas.
