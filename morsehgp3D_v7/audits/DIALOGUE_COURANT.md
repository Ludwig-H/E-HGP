# Dialogue actif avec le constructeur

11 septembre 2026, après **34db4b3e**. Les premiers raccords dense et CPU
parallèle sont publiés et contre-vérifiés. La priorité reste la tour entière ;
la [coordination](COORDINATION_AUDITEURS.md) répartit les écritures.

## Réutiliser les composantes fermées déjà présentes dans les histoires

Dans le calendrier scellé **1cf438cb**, `reconstruct` calcule déjà toutes les
marques par un sweep à admission fermée. Pour le rôle `(K,B)`, soit v_B le
segment de naissance de φ(K,B), λ_B son admission et m_B sa marque liée :

$$m_B.\mathrm{segment}=A_K(v_B,\lambda_B,\mathrm{ferme}).$$

C’est exactement la réponse que `graph_full::build` **bad5051f** recalcule
avec `contribution_answers`. Le sweep traite tous les nœuds de date ≤λ_B
avant d’écrire la marque ; le tri final par MarkId conserve ce résultat.
Une fois l’histoire et les marques liées au même certificat immuable,
consommer la marque supprime cette seconde consultation.

Le raccord doit garder trois propriétés :

- Lier MarkId au bloc, représentant à φ, admission au rang de B et segment
  à cette même histoire. Une marque forgée peut satisfaire ces identités et
  avoir un mauvais segment : la garantie de fermeture doit provenir de la
  reconstruction certifiée, ou être revérifiée pour une entrée extérieure.
- Émettre la contribution à **λ_B**, même si le segment est né plus tôt.
  La réutilisation ne doit pas avancer l’activation d’une continuation.
- Conserver l’ordre `atlas.program(K)` et y chercher les marques. Leur ordre
  MarkId diffère de l’ordre `(rang, BallKey)` ; le tri stable final par
  `(rang, cible)` ne répare pas un changement d’ordre interne. Un autre
  parcours devra transporter explicitement l’ordinal source certifié.

Les singletons K1 gardent leur canal séparé. Une recherche binaire de marque
reste un coût ; une table de liaison directe demande des octets à compter.
Un objet préparé par la fabrique d’histoires peut partager ces liaisons sans
revalider chaque marque à chaque usage. Une vue const seule ne suffit pas
à garantir l’immuabilité de son propriétaire.

Les [captures CPU4 scellées](../receipts/parallel_birth_streaming_20260911/README.md)
quantifient le travail visé :

| n, s8, K1..10 | HLD contributives remplaçables | HLD inférieures et de naturalité conservées |
| ---: | ---: | ---: |
| 8 000 | 2 396 646 | 7 920 937 |
| 16 000 | 5 010 402 | 16 556 793 |
| 32 000 | 10 348 964 | 34 205 965 |

La [dérivation reproductible](ENTRETIEN.json) vérifie aussi
`contribution_queries + n == contributions`. Le nouveau chemin doit compter
ses réemplois de marques et les consultations HLD effectivement exécutées.
Les anciens compteurs light/binary sont agrégés : ils ne donnent pas le
nombre de pas ni le temps que ce delta retirera. À 32k, les 50,145 s
d’histoires et 91,704 s d’export sont les postes mesurés avant ce raccord.

Ces mêmes marques donnent **tous les u_B** de la [preuve d’export historique](receipts_historical_export_20260911/README.md),
§2–3. Son futur raccord peut éviter ses A consultations supplémentaires,
sans retirer groupages, minima de groupe/lot, premières utilisations ni
permutations. Conserver les rôles silencieux jusqu’à ces minima. Le témoin
scellé, qui payait ces consultations, reste inchangé. Un futur reconstructeur
parallèle ne fournissant pas encore les marques devra d’abord les calculer.

## Préparer les chaînes une fois, partager les index adjacents

L’export construit Chains(K), puis reconstruit le même index comme
lower_chains au tour suivant. Pour m ordres, cela fait **2m−1 constructions**.
Conserver les index précédent/courant suffit à revenir à m : **19→10** pour
K1..10. Libérer K−1 après sa dernière consultation, puis faire de K le
précédent avant de préparer K+1 ; au plus deux index restent simultanément.
Le travail évité est une préparation de chacune des histoires 1..m−1.

Conserver leur validation structurelle, même si les contributions n’utilisent
plus HLD. Les adresses des histoires restent stables et leurs tableaux
immuables jusqu’à la dernière requête. La borne des trois tableaux d’index
est `3*sizeof(Id)*(N[K] + N[K-1])` ; scratch de construction et histoires sont
séparés. Ce n’est pas une mesure RSS.

Une marque inférieure fermée à λ_B peut servir d’ancre ; elle ne remplace
pas sa normalisation à la date d’un nœud supérieur. Deux nœuds ayant le même
descendant natif peuvent avoir des images différentes si une fusion
inférieure intervient entre leurs dates. Les contrôles de naturalité
conservent donc leur coupe propre.

## Qualifications closes et prochain contrôle ciblé

Les lecteurs [dense/pool](../receipts/birth_streaming_20260911/README.md)
et [parallèle](../receipts/parallel_birth_streaming_20260911/README.md)
passent normal/−O ; dense et géométrie parallèle ont leurs propres O2/SAN.
Les premières gates et le triplet ne sont plus demandés. TSan reste le refus
préalable documenté. Aucune nouvelle exécution C++ par notre contrelecture.

Pour le delta proposé, comparer les contributions physiques, leurs dates et
les verticales sur les fixtures existantes ; cibler continuation tardive,
ordre différent des marques et du programme, rôle silencieux et fusion
inférieure entre deux dates. Préparation commune, semis et gardes de rang
restent les deltas déjà acceptés ; leurs preuves détaillées sont conservées
dans le [paquet de préparation](receipts_prepared_catalogue_20260911/README.md)
et l’[entretien de 19049adf](ENTRETIEN.json).

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
