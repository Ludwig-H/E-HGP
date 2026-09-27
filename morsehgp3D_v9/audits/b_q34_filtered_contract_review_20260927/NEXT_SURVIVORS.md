# Étape suivante possible : survivants S résidents et remise en ordre GPU

27 septembre 2026. Proposition algorithmique indépendante ; aucun code,
benchmark, lancement GCP ni modification du prototype en cours. Le
[contrat rectangles filtrés](README.md) reste inchangé. Cette proposition
est à examiner **après** la qualification de cette première couture.

## Décision proposée

Conserver les survivants des vagues sur GPU, les trier par leur ordinal
original u64, puis ne télécharger que leurs rangs spatiaux et leur masque
final. Cela laisse les prédicats, les bandes, E et S inchangés. Le tri
porte sur **S seulement**, jamais sur P ou E. La sortie triée pourrait
ensuite être consommée directement par S3 au moyen d'un propriétaire
résident ; ce second raccord nécessite sa propre interface exacte.

La [première capture CUDA](../../receipts/q34_cuda_g4_20260927/r1/SUMMARY.json)
mesure 39,171 ms d'ordre/conversion CPU et 22,568 ms de téléchargement/
allocation hôte des survivants. Ce sont des postes de **l'ancien** runner,
pas des gains accessibles par soustraction : ils seront remplacés par
accumulation, copies device, tri, validation, transferts et allocations
différents. Les 160,847 ms non attribuées de son intervalle CUDA restent
non attribuées ; cette proposition ne les élimine pas par hypothèse.

## Piège prioritaire : ne pas recréer E avec des chunks « S »

Allouer un chunk de capacité Q pour chaque vague non vide puis le garder
jusqu'à la fin peut consommer Θ(E), malgré seulement S éléments utiles.
Contre-exemple : E/Q vagues, chacune ne conservant qu'une seule paire.
On aurait S=E/Q mais une capacité Q·S=E.

Deux stratégies correctes possibles :

- Après le petit retour du nombre `live_i` déjà effectué par le runner,
  allouer exactement `live_i` entrées et y copier les survivantes de cette
  vague. La somme des capacités utiles vaut S, mais les allocations et
  leurs métadonnées peuvent être nombreuses.
- Remplir des pages de sortie **à travers les frontières de vagues** :
  toutes les pages précédentes sont pleines, seule la dernière peut être
  incomplète. Avec une taille Qs indépendante, la capacité est au plus
  S+Qs, pas nombre_de_vagues×Qs. Une croissance géométrique d'un buffer
  contigu est aussi possible, à condition de compter les copies et la
  coexistence des anciennes/nouvelles allocations.

La réutilisation du buffer temporaire d'une vague doit attendre que ses
survivantes aient été copiées dans leur stockage possédé. Des streams
concurrents exigent des événements/dependencies explicites. Une réserve
pleine ou une allocation impossible ne permet jamais d'abandonner le
reste de E ou de publier une sortie partielle.

## Ordre exact et propriété

Chaque survivante garde le triplet indivisible :

```text
(ordinal original p : u64, rang_a : u32, rang_b : u32, masque final)
```

L'ordinal est celui du rectangle initial, avec les raw_base comprenant
les rectangles fermés. Trier seulement selon les bits nécessaires à P
**filtré** serait faux : les ordinals peuvent dépasser P. Trier tous les
64 bits non signés est la convention simple. Un raccourcissement éventuel
doit être prouvé depuis la borne brute des ordinals, jamais depuis E/S.

Le payload a/b/masque doit suivre sa clé. Les clés admises sont uniques :
un doublon est un défaut à refuser, pas à dédupliquer. Avec cette unicité,
une stabilité entre clés égales n'est pas nécessaire à la preuve de
l'ordre final ; le parcours et le contrôle d'unicité le restent.

Après tri et jointure, le propriétaire `DeviceSurvivors` devrait retenir
index/K/filiation de la décision rectangle, S, ordre déclaré, données et
compteurs clos. Le contrôle adjacent des clés et les erreurs CUDA sont
jugés avant publication. S3 reçoit ce propriétaire, pas des pointeurs
extérieurs accompagnés de masques supposés fiables. Son ordinal d'arête
j doit rester la **position dans S trié**, car les décisions et records
ultérieurs sont indexés par cet ordre.

L'adaptateur natif doit toujours restituer les R masques rectangle,
P logique et les rejets par voie définis dans le contrat. Les transferts
ordinals peuvent disparaître après qualification de leur tri/contrôle
device, mais pas les contrôles différentiels sur les petites fixtures.
La segmentation, les coordonnées et la géométrie ne changent pas.

## Mémoire et transport : compter toutes les représentations

Le `Edge` actuel occupe 24 octets dans le build qualifié ; le natif n'a
besoin que de deux rangs u32 et du masque. Une représentation de transport
de 12 ou 16 octets est possible en principe, avec layout/padding
explicitement définis et testés. Des tableaux séparés demanderaient
9 octets logiques par survivante mais imposeraient leur propre conversion.
Ne pas supposer qu'un `memcpy` entre deux structs de même taille est un
contrat de layout suffisant.

Pour S=2 043 612, les payloads bruts représenteraient respectivement :

| format par survivante | octets de payload |
| --- | ---: |
| 24 octets actuels, ordinal compris | 49 046 688 |
| 16 octets sans ordinal | 32 697 792 |
| 12 octets sans ordinal | 24 523 344 |

Ce tableau exclut les petits retours de compteurs, les capacités et tout
scratch. Il ne prédit pas le rapport des temps D2H : l'ancien chrono
contient aussi les allocations/croissances hôtes.

Une recette possible consiste à aplatir les pages vers des clés u64 et
un payload 12 octets, libérer les pages, puis trier avec deux jeux de
clés/payload. Ces deux jeux représentent alors 40S octets, **en plus du
scratch réel du tri**. L'aplatissement peut connaître un autre pic avec
pages source et destination simultanées. Ce sont des comptes illustratifs
de cette recette, pas la promesse d'allocation d'une API particulière.

Les buffers de la vague Q, l'index résident, les rectangles/bandes,
le résultat trié, le scratch du tri, et éventuellement les buffers S3
coexistent selon le calendrier choisi. Les libérations doivent être
placées dans ce calendrier et dans le chrono complet. « O(S) » ne signifie
ni un seul buffer S ni mémoire négligeable à plusieurs millions de sites.

S et les offsets globaux restent u64. L'interface, le type de compte et
le scratch d'un éventuel radix-sort CUB doivent être vérifiés dans les
headers de **la version effectivement compilée** ; la limite `int`
imposée aujourd'hui par l'appel de scan d'une vague Q ne prouve aucune
limite universelle de l'API de tri. Au-delà des capacités supportées,
il faut un chemin exact documenté (runs triés puis fusion, par exemple)
ou un refus explicite, jamais une troncature réussie.

## Comparaison avec l'alternative ordered_rows déjà qualifiée

Les [listes B ordonnées](../b_q34_ordered_rows_20260927/README.md) changent
la représentation **avant S2** : A reste en ordre original ; une liste B
ordonnée et masquée est partagée par toutes les ancres d'une même classe
de crédits. Avec des vagues parcourues dans cet ordre et un compactage
stable, S sort déjà en ordre natif, sans tri global ni clés conservées
pour les remettre en ordre.

| Option | Nouveau travail | Coût évité | Qualification existante |
| --- | --- | --- | --- |
| Bandes actuelles + tri device de S | Accumulation et tri S, copies/scratch | Tri CPU, transfert des ordinals | Bandes/décodeur/vagues qualifiés ; ce tri résident ne l'est pas |
| Listes B ordonnées + compactage stable | W scans de crédits, listes T, préfixes par ancre/tâches | Tout tri global de S | Représentation et ordre petits cas qualifiés ; ni S2 GPU ni tour |

Pour les listes, `W=sum(C_A·|B|)` et `T=sum(t_c)`, alors que
`E_plans=sum(m_c·t_c)`. Les classes stockées sont occupées : T≤E_plans ;
T≤W≤K(K−1)F_B, donc une constante au plus 20 à K5 et 90 à K10. Mais T peut
être égal à E_plans : ne pas vendre cette représentation comme toujours
strictement plus petite que le résidu. Les fallbacks ne sont pas inclus
dans W/T et gardent leur émission directe ordonnée.

Les [mesures historiques de représentation](../../receipts/q34_ordered_rows_20260927/README.md)
sur ng00 K5/s8 donnent W=4 079 626, T=1 968 618 pour E total=9 122 704.
Le sidecar prend 33,648 Mo de capacité, en plus de l'ancien plan encore
présent. Les 71,985 ms locaux ne mesurent que l'ajout des lignes après
préparation des crédits, sans S2 : **aucune comparaison numérique directe
avec les 39 ms de tri G4** n'est valable. Un constructeur collectif direct,
ses préfixes par ancre et son décodeur GPU restent à développer.

Recommandation : conserver la couture rectangle résident actuellement en
qualification, puis essayer d'abord bandes + S résident/tri GPU comme
changement étroit du chemin de sortie. Ordered_rows reste l'alternative
structurelle si le coût complet du tri/transport domine après ce raccord,
ou si ses listes/préfixes se construisent plus économiquement que les
permutations/bandes. Les deux solutions ne réduisent ni E, ni S, ni le
travail géométrique aval par elles-mêmes.

## Test utile, sans nouvelle campagne de variantes q2

Sur les mêmes décisions et la même représentation, comparer la sortie
ancienne et la nouvelle **champ par champ**, ainsi que P/E/S et les
visites ponctuelles. Inclure des ordinals au-delà de 2^32, une majorité de
vagues vides, une seule survivante par vague, pages traversées par une
vague, ordres de bandes inversés, masques 2/4/6, et mutations désolidarisant
clé/payload ou laissant un doublon. Les dépassements u64 restent testés
par formules sans prétendre parcourir des milliards de sorties.

Mesurer ensuite dans un même processus et avec ordre apparié le coût
complet de sortie : accumulation, allocation, copies D2D, tri, contrôle,
D2H, conversion native et destruction des temporaires. Publier les
capacités/pics et conserver le coût froid. Une comparaison à préparation
réutilisée est seulement une comparaison de cette étape ; elle ne devient
pas un nouveau temps d'adaptateur ni de tour FULL.

Pour une décision de port moteur, le comparateur de performance doit
rester le **filtre GPU natif**, apparié dans le même processus et avec
les mêmes options/entrées. La référence CPU du harnais reste l'oracle de
correction. Battre le prototype qui préparait les rectangles en CPU
pendant environ 9,9 s ne prouve pas que l'on bat les quelque 101 ms du
filtre GPU historique. Ces 101 ms eux-mêmes ne sont qu'une référence
historique tant qu'une comparaison GPU/GPU appariée n'est pas réalisée.

Cette piste n'est pas un résultat acquis, ne ferme pas S3/S4 et ne
qualifie pas le contrat de tour explicite 100 ms.
