# T2-d C — budgets, identité locale et limites de la simulation

Contrelecture du 8 octobre 2026, prototype `repo902`, base produit
`902041f66`, non livré à cette capture. Exploration v12 hors registre,
u21, catalogue exécuté sur CPU/Pool ; aucune exécution CUDA par cet audit.
Les sources, journaux et binaires observés sont épinglés dans `capture.json`.
Les sources temporaires sont conservées hors dépôt ; aucune coordonnée ni
donnée sous licence n'est copiée ici.

Les deux constructions Release/u21, **CUDA OFF**, se terminent avec code 0
à 04:54:21 et 04:58:14 UTC. Le conducteur conserve `$?` avant toute commande
de date. Les essais d'identité avant/après sont lus, jamais relancés : neuf
cas, dix-huit processus code 0, sept lignes comparées par cas (trois avant,
quatre après). Chaque cas conserve nombres de boules/niveaux et les trois
empreintes catalogue/niveaux/table, avec zéro écart de recherche de support.
Il s'agit de neuf entrées déclarées par le conducteur : ng00–02 K5/K10 et
trois synthétiques de 8k/16k/32k K5. Les fichiers d'entrée n'ont pas été lus
ni rehachés. Les niveaux et la table sont contrôlés séparément par la sonde.

Le pic hôte du modèle « comptes CUDA » diminue dans les neuf cas ; le pic
de son budget appareil reste égal et le transit conservé finit à 16 Mio.
Les valeurs exactes figurent dans `capture.json`. Ces mesures utilisent
des budgets neufs par voie ; la préparation du nuage est dans un autre
budget. Ce ne sont ni des pics FULL résidents, ni RSS, ni mesures de VRAM.
Les durées locales de ces processus ne décident aucun gain G4.

**La simulation ne reproduit pas les allocations des tableaux CUDA.**
`StagedExecutor` et `CudaLike` héritent de `PoolExecutor::Array`, donc de
`FrontArray` : croissance `max(n, 2*ancienne_taille)` ; `ensure` rend
l'ancien tableau avant allocation. CUDA réserve `max(n, cap+cap/2+1024)`
éléments et garde l'ancien tableau jusqu'à synchronisation et libération,
y compris avec `keep=0`. Par exemple, le premier `ensure<u32>(1)` demande
4 octets au modèle et 4096 à CUDA. Les deux régimes ne partagent donc ni
les pics, ni les seuils exacts de refus. La simulation partage le pipeline
et teste la séquence des tranches ; elle ne qualifie pas les capacités ou
les échecs du pilote CUDA. Les petits buffers de simulation ne constituent
pas non plus une mesure physique de la mémoire épinglée.

Les nouvelles portes ont une portée utile et explicite :

- `pipeline_budget` : deux nuages, trois fils, budgets séparés, succès au
  pic du modèle puis refus entiers sous plusieurs limites, compteurs dans
  les limites et réservations rendues à la destruction.
- `pipeline_budget_reuse` : après un refus hôte, le même état/exécuteur
  simulé produit le petit carré correctement.
- `finish_budget` : chaîne de 202 éléments, reprise et sorties exactes,
  succès/refus avec limites sur chacun des deux budgets.
- `device_open_budget` : vraie voie CUDA si disponible, deux nuages,
  budgets minuscules puis pic/pic−1 et libération ; chaque essai ouvre un
  contexte neuf. Sans appareil, la porte réussit seulement le contrôle
  `device_unavailable`. Elle ne prouve pas la reprise du même contexte GPU
  après refus ; un tel cas compléterait utilement la couverture.

Les 41 contrôles et répétitions budget annoncés par le rapport ne sont pas
contre-certifiés ici : aucun journal primaire correspondant n'a été trouvé
dans les emplacements consultés. Les binaires et objets existants sont
hachés sans exécution. La présence d'un objet construit et le rapport ne
remplacent pas les résultats de ces portes. Le nouveau test de formule
`stream_staging` appartient à la source courante ; aucun rejeu ne lui est
attribué dans ce reçu.

Lecture statique du corps CUDA : les réservations précèdent `cudaMalloc` ;
le transit ancien est synchronisé puis libéré avant son remplacement ; les
tranches sont consommées après événement et toute issue de `stream_staged`
en erreur draine les copies avant retour. Les sorties anticipées accroissent
la coexistence mémoire, désormais déclarée, et restent privées jusqu'au
succès. Aucun dépassement de budget ou publication partielle n'a été
démontré par cette lecture. Cela ne remplace pas les portes GPU futures.

Rejeu du reçu, sur la capture extérieure indiquée dans la passation :

```sh
python check.py --evidence /chemin/capture
python -O check.py --evidence /chemin/capture
```

Le lecteur ne lance que des lectures/hash de métadonnées et compare son
résultat au champ `result` de `capture.json`. Les deux sorties sont égales.
Il dépend de cette capture extérieure ; il n'est pas une archive autonome
des sources et journaux. La provenance source→compilation des sondes locales
n'est pas intégralement attestée : recette, configuration, binaires et
corps présents sont épinglés, sans inventer un relevé avant compilation.
L'admission du pilote G4 C est auditée séparément.
