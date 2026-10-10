# Réponse au développeur : réduire le travail de G

10 octobre 2026. Réponse aux [quatre questions](../developpement_20261010/QUESTION_CLAUDE_g_travail.md)
du commit `5f3318baa`. Lecture du produit A6c `aa6338ee8` ; B3b est une
composition mesurée séparément. Propositions d’algorithme à déclarer avant
microbanc G4, aucune modification du moteur ni gain nouveau revendiqué ici.

## Décision immédiate issue de B3b

Les [300 processus et 2 100 passes chaudes relus](b3b_stats/README.md) soutiennent
**les clés seules** : cinq IC strictement sous 1, gain de 1,44 à 2,85 % selon
la trame. Le lot passe également, mais ajouter le balayage aux clés n’apporte
aucun bénéfice supplémentaire établi ; le balayage seul échoue sur deux trames.
L’[admission](session_b3b_admission/README.md) garde les réserves sur codes G,
stderr et clôture des exécutables. Ce banc ne remplace pas MES-FULL sur 37 trames. Depuis `2aaed1847`, les clés
seules sont adoptées : [138 fichiers produit vérifiés](b3k_produit/README.md)
identiques au bras mesuré.

## Q1 — Réduire les représentants à résoudre

La piste démontrable porte sur les **composantes locales du graphe de traces
séparables**, déjà calculées pour classifier les coquilles étendues. Un seul
représentant canonique peut servir chaque composante à la coupe ouverte de la
cellule. La [preuve et les témoins](g_travail_math/README.md) précisent la portée,
les égalités et les limites de réutilisation. Ce n’est pas un nouveau retrait
universel sur les coquilles régulières ; les effectifs LiDAR restent à mesurer.

## Q2 — Partager entre ordres

L’inclusion des régions couvertes donne une application de composantes de
l’ordre k+1 vers k. **Elle n’est pas injective** : une même image verticale ne
justifie pas d’identifier deux cibles à l’ordre supérieur. Le témoin exact et
les conditions de partage géométrique figurent dans la même
[réponse mathématique](g_travail_math/README.md). Une cible doit rester attachée
à son ordre et à sa coupe ; la dépendance entre ordres coûte aussi en latence.

## Q3 — Première sonde sans matérialiser la partie

`passes.cpp` prépare **déjà** la somme des empreintes de I et les empreintes de
U une fois par cellule. Cette optimisation n’est donc pas à refaire. La piste
restante est une vue triée de **I ∪ A**, où A est déterminée par le masque du
représentant : conserver masque/cellule/empreinte dans la file, comparer cette
vue aux lignes de la table, et ne remplir `Part` qu’au premier échec de sonde.

La [preuve par équivalence des comparaisons et le modèle borné](g_premiere_sonde/README.md)
couvrent collisions, absences et refus des rangs égaux ou supérieurs. La cible
ne se déduit pas de I/U seuls : dans un triangle aigu, trois parties de deux
sites de la même coquille correspondent à trois naissances distinctes.
Une empreinte sert à chercher, jamais à certifier l’égalité de populations.

Le chemin de succès doit garder le contrôle strict du rang avant sortie,
les compteurs et le contrat de la file de prélecture G-L7. Il faut donc un
chemin de succès commun au résolveur ; appeler simplement `resolve_part` avec
un `Part` vide détournerait son contrat. Le chemin d’échec reçoit la même
partie triée qu’aujourd’hui. La vue retire des écritures temporaires, mais
continue à lire et comparer les IDs : elle peut perdre par branches ou accès
répétés en cas de collisions. Aucun facteur de gain n’en découle. Juger coût
total FULL et distribution des collisions, pas seulement le nombre de traces.

## Q4 — Maximum et trames de plus de 60 000 sites

Le texte tranche déjà : [DECISIONS D7](../../docs/DECISIONS.md) porte sur
« plusieurs séquences, toutes tailles », avec jugement à la médiane **et au
maximum**. [MESURE §2](../../docs/MESURE.md) désigne les 37 trames de
33 179 à 99 099 sites comme base des décisions de vitesse. « Environ 60 000 »
décrit le régime principal ; ce n’est pas une borne d’exclusion. **Le maximum
de 241,89 ms reste dans le contrat**, qui n’est pas tenu.

On peut publier des classes de taille annoncées à l’avance pour diagnostiquer
le coût ; aucune ne remplace le jeu complet. Le rapport 99 099/60 000 ne suffit
pas à corriger le temps par une règle de trois : géométrie, représentants,
collisions, census et recouvrement changent aussi. Une borne générale en n
ne découle ni du profil d’une trame ni de deux temps voisins. Modifier le
périmètre serait une nouvelle décision explicite, pas une lecture du contrat
actuel. La prochaine priorité reste donc de réduire G, puis C, sur ce jeu.

## Complément après lecture du profil B3b

Le [profil relu](b3b_profil/README.md) porte sur le **lot complet**, pas sur
les clés seules adoptées. Il confirme 75,23–77,12 % de succès à la première
sonde. Sur 02/001606, traces 23,71 %, sondes 14,98 %, LEM-T1 20,93 % des cycles
sommés. Ces parts ne prédisent pas directement une baisse de latence FULL.
Deux propositions plus petites peuvent précéder la vue virtuelle :

- [Retour immédiat à l’égalité exacte dans `find`](g_population_egalite/README.md).
  L’ordre strict des fiches rend la réponse unique. La suite des comparaisons
  devient un préfixe de celle d’aujourd’hui, collisions comprises ; 53 560
  requêtes de modèle. Patch isolé, gardes/rangs/API inchangés.
  **Avant intégration :** réancrer les mutants existants
  `recherche_sans_egalite_des_sites` et `recherche_premiere_place_du_seau` :
  leurs deux motifs textuels disparaissent avec l’ancien corps de `find`.
  Garder IDs, portes et plancher73 ; ne pas traiter leur absence comme un succès.
- [LEM-T1 : rechercher seulement F\S](t1_support_population/README.md).
  Après authentification de S⊆F et réponse exacte S=S*(b), le Catalogue
  garantit déjà S⊆U_b. Le helper peut omettre ces sites, aux deux appels,
  en utilisant le support certifié à l’appel post-certificat. 24 300 cas,
  mêmes décisions et premier rejet. Les sites hors support restent contrôlés.

Les deux patches sont **proposés, non intégrés et non mesurés**. Ils touchent
des unités distinctes et demandent des bras séparés pour attribuer les gains.
Le préchargement de populations actuellement essayé dans `passes.cpp` peut
être un autre bras ; l’associer d’emblée masquerait leurs effets respectifs.
La réduction locale Q1 reste conditionnée au volume réel de représentants
des coquilles étendues : leur faible nombre de cellules ne borne pas leur coût.

## Mémoire B3-K, revue complémentaire

[Aucune omission comptable démontrée](b3k_memoire/README.md) pour les clés,
sur les voies complètes et découpées. Les compter à nouveau dans la tour
ferait doublon. Le reçu propose seulement de libérer plus tôt un curseur
devenu inutile : bénéfice local conditionnel sans cache, aucune promesse
de baisse de RSS ni de pic avec cache. Cette piste secondaire ne remplace
ni la réduction de G ni la qualification mémoire CST-0244/0245 en attente.
