# Plan de la prochaine session G4

Statut : plan opérationnel. Aucun claim, aucune porte. À exécuter tel quel, dans
l'ordre, et à interrompre plutôt qu'à improviser si un préflight refuse.

## Pourquoi ce plan existe

Le rendement d'une session facturable dépend presque entièrement de ce qui est
décidé **avant** l'allumage. Les quatre sessions du 7 août ont réservé six
heures trente de plafond GCE cumulé pour environ seize minutes de balayage utile
dans la première. La cause n'était pas la VM : c'était que les questions
n'étaient pas prêtes.

Elles le sont maintenant. Les travaux locaux ont produit trois questions que
**seule** une exécution native peut trancher, et plus aucune mesure locale ne les
approchera.

## Les trois questions, dans l'ordre de valeur

**Attention : l'ordre de valeur n'est pas l'ordre d'exécution.** Q1 est la plus
importante et elle est la dernière exécutable, parce qu'elle dépend de Q3. La
session s'exécute donc **Q3, puis Q1, puis Q2**.

### Q1 — L'aval passe-t-il l'échelle ? (verrou dominant)

C'est la découverte du 7 août et elle prime sur tout le reste. La fermeture de
descente de facette est **quadratique en nombre d'événements** — exposants 2,10
et 1,81 sur deux nuages indépendants — et sa **constante croît avec le nuage** :
à $K=4$, de 11,6 ms par événement à $n=8$ jusqu'à 21,9 ms à $n=18$. Les 61 ms par
événement mesurés localement sont donc un plancher.

Localement, la mesure s'arrête à $n=18$ : à $n=20$ le profil borné rend
`budget_exhausted`. **C'est exactement ce que le profil sans budget lève**, et
c'est pourquoi la suite appartient à la G4.

*À exécuter* : **à 50 000 points**, et non sur une échelle intermédiaire. La
discipline de tests scellée est explicite — « la vérification exhaustive exacte
se fait sur de tout petits nuages, puis **directement 50 000 points**, puis
dizaines de millions si raisonnable ; rien entre, rien au-delà de l'utile ». Les
tailles intermédiaires ne qualifient rien et coûtent de la VM.

**Mais l'aval n'est pas atteignable à 50 000 points aujourd'hui**, et c'est le
point qui commande l'ordre de la session. L'escalier R2-d l'a mesuré : à 50 000
points l'étage paire consomme les 299,9 s de délai à lui seul et l'étage higher
n'est **jamais atteint** — donc l'aval encore moins. Aucun réglage n'y change
rien : c'est le mur de l'étage paire.

**Q1 est donc conditionnée à Q3.** Le catalogue terminal paire au rang onze est
ce qui ouvre le chemin jusqu'à l'aval à la taille contractuelle. L'ordre
d'exécution de la session n'est pas l'ordre de valeur : **Q3 d'abord, puis Q1**.

*À publier pour Q1* : `reducer_stream`, `batch_plan`, `reducer_setup`, le nombre
d'événements, et le coût par événement — à comparer aux 61,4 ms mesurés à $n=16$
et aux 21,9 ms par événement à $n=18$ qui montrent que cette constante croît.

*Ce que la mesure décide* : si le coût par événement à 50 000 points confirme la
croissance observée, l'aval est disqualifié pour le contrat dans sa forme
actuelle, et le travail suivant est une **borne de travail** pour cette
fermeture, pas une optimisation. S'il plafonne, l'aval redevient un problème
d'ingénierie.

*Repli si Q3 échoue* : mesurer l'aval sur une entrée d'événements **synthétique**
à l'échelle, sans passer par l'étage paire. Le dépôt possède déjà ce mécanisme
pour le réducteur de hiérarchie de points — 2 792 ms sur une tour synthétique de
50 000 points — mais pas pour la fermeture de descente de facette, et le
construire est un travail à part entière qui ne doit pas être improvisé sur une
VM facturée.

### Q2 — Le coût unitaire du device est-il du calcul ou du transport ?

La réfutation du filtre fp64 a montré que retirer du travail arithmétique aux
portes du moteur higher ne déplace rien, et le débit mesuré du launcher paire —
44,3 ns par visite de nœud, soit quelques centaines de visites par seconde et par
thread — n'est justifié par aucune opération de traversée.

*À exécuter* : un profil natif de l'étage higher publiant le **nombre de
lancements** et le **temps par lancement**, séparément du temps passé dans les
portes.

*Ce que la mesure décide* : si le temps vit dans l'aller-retour de lancement et
de drainage, l'optimisation à faire est le regroupement des lancements, pas
l'arithmétique — et le filtre fp64 restera l'exemple de l'erreur inverse.

### Q3 — Le catalogue terminal paire tient-il le rang onze ?

La frontière Yao-48 est **déjà** mesurée au rang du contrat : couverture complète
à 50 000 points, 2,979 s de launcher, 7 962 604 records candidats. Son
consommateur acceptait six ; il accepte maintenant onze, mais aucune exécution
native ne l'y a jamais mené, et son audit le déclare
(`closed_rank_window_natively_qualified`).

*À exécuter* : le catalogue à rang onze contre le lanceur natif, aux tailles où
il termine, puis à 50 000 points.

*Ce que la mesure décide* : si le plafond était bien une frontière de
qualification et non une contrainte, l'étage paire est complet au rang du
contrat.

## Ce qui doit être prêt avant l'allumage

Retenu des sessions précédentes, où chacun de ces points a coûté du temps
facturé :

- `/tmp` est purgé au reboot : **seules les couches Docker survivent**, et une
  reconstruction complète du runner coûte quatre minutes ;
- la spec CDI est absente après chaque boot de la cible `ai1a` et doit être
  régénérée à la main, sinon `docker --gpus all` échoue — **aucun script du dépôt
  ne le fait** ;
- les deux coupe-circuits — `maxRunDuration` GCE avec action `STOP`, et
  `shutdown -P` dans l'invité, contraint à au plus `maxRunDuration` moins 300 s —
  doivent être vérifiés **avant** de commencer ;
- la clé OS Login est éphémère et son TTL restant doit tomber dans
  $[\text{maxRunDuration},\ \text{maxRunDuration}+660\,\text{s}]$ ;
- le quota plafonne à **une seule G4 Spot concurrente** ;
- la session se ferme par une relecture indépendante `TERMINATED` de la **même
  génération**, puis révocation de la clé.

Et la leçon propre au 7 août : un **préflight local** a découvert que le runner
appelait le pont avec `tile_certified_commit=false`, ce qui aurait fait
chronométrer l'ancien chemin. Le préflight n'est pas une formalité, il a déjà
sauvé une session entière.

## Ce que la session ne doit pas faire

- **Ne pas re-tester des réglages de l'étage higher.** T1 et T2 ont mesuré que le
  coût est invariant sous le découpage ; c'est fermé.
- **Ne pas chercher à qualifier la germination locale.** Elle n'est ni tuilée ni
  device ; sa place viendra, pas ici.
- **Ne rien promouvoir.** `deployment_status=architecture_only`,
  `public_status=not_claimed` restent vrais quoi que la session mesure : aucun
  benchmark ne promeut un statut.
