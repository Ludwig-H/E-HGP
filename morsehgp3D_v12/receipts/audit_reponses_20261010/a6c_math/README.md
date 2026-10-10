# A6c : chaîne conditionnelle, publication et borne d'admission

10 octobre 2026, Codex. Prélecture de `aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66`,
identique pour les onze fichiers épinglés au snapshot `a6c_prelecture_20261010_1800`.
Comparaison à R1 `8a0716e7470197c95953b38d79f26b8d8f2379fc` :
[capture](capture.json), [lecteur](check.py), [résultats](results.json).
Lecture et modèles Python seulement ; aucune compilation, exécution native, scène
sous licence ou action GCP. Ni adoption, ni gain mesuré, ni qualification générale C++.

## Point à corriger : le budget ne suit pas encore la bascule

`pipeline.cpp:194–198` calcule la borne avant `admit_session`; l'admission à la
ligne 205 précède la décision `chain_engaged` aux lignes 209–215. La borne ajoute
inconditionnellement, pour K ≥ 2, **W m (sizeof(Sphere)+4)**, avec W le nombre de
fils et m le maximum de 1 et des plus grandes cohortes des ordres 2..K
(`region_bytes:38–41`). Ce supplément N reste exigé lorsque tous les ordres
suivent la branche désactivée (< 43 900 sites au seuil produit). Les nouveaux
compteurs agrandissent aussi `WorkerTotals` de cinq champs u64 ; ne pas présenter
le supplément N comme l'unique différence du budget.

Conséquence source : à usage initial identique, si A est l'ancienne borne et la
marge disponible vaut A, `MemoryBudget::admit` sans cache accepte A et refuse
A+Δ pour tout Δ positif (`core/buffer.hpp:116–120`). Avec cache, le contrôle
ajoute encore floor(bytes/8). **Aucune fixture native de ce refus n'a été jouée** ;
la portée établie est la différence de formule et son effet conditionnel sur
l'admission. « Chemin de la base » désigne ici les corps du calcul, pas un budget
ni un coût identiques à R1.

Proposition : fixer la décision avant `region_bytes`, conditionner le supplément
N aux ordres engagés, puis conserver cette décision jusqu'à la région. Les portes
qui forcent le seuil doivent le faire avant cette décision ; permettre un passage
off→on après calcul d'une borne off serait une sous-admission. Réutiliser aussi
la plus grande cohorte : `KernelBytes::body` la parcourt via `kernel_bytes`, puis
une seconde fois via `widest_cohort`, y compris chaîne inactive. Le retrait de ce
double parcours et de la borne superflue reste à mesurer. Pour la branche active,
la somme des W plus grands besoins de morceaux reste une piste de borne plus fine
déjà proposée ; aucun pic natif nouveau n'est établi ici.

## Publication, durée de vie, échecs

- G écrit ses cibles et éventuellement les feuilles avant le store release du
  drapeau de tranche (`pipeline_run.cpp:287–321`). Le noyau acquiert ce drapeau,
  n'avance que sur `kSliceDone`, traite les cellules dans l'ordre puis publie
  `kernel_slice` en release (`:328–384`). Un compteur de réclamation n'est pas
  utilisé pour autoriser la consommation d'une tranche non publiée.
- L'aide reste appelée après échec de la recherche d'un G réclamable (`:231–239`).
  Cela permet des aides pendant des calculs G déjà réclamés. Le compteur
  `hint_jobs_during_g==0` n'établit donc pas une absence de chevauchement avec G.
- Le pont CST-0242 est présent : store release de la feuille par l'aide, load
  acquire par le noyau. Pour un parent H lu par l'aide, H précède sa publication,
  qui précède la consommation L, elle-même antérieure à tout store U futur du
  noyau. H HB U interdit que H lise U dans le modèle examiné. Les reprises du
  noyau restent ordonnées par publication release de son drapeau et réservation
  CAS acq_rel. Le contrôle `target_index < processed`, reçu via l'acquire de
  `kernel_slice`, protège aussi la lecture ordinaire de `element`.
- L'annonce de l'aide, son contrôle de fermeture, son retrait, la fermeture et
  l'attente du compte nul restent seq_cst (`:338–345,451–464`). Dans leur ordre
  total, une aide admise est annoncée avant la fermeture et reste comptée
  jusqu'à sa fin ; une aide annoncée après la fermeture lit vrai et saute le
  travail. Le modèle borné de une/deux aides explore **25/161 états**, sans travail
  après libération ; enlever l'attente produit **1/16 transitions fautives**.
- CST-0241 est préservé : `last` vient du `fetch_sub(acq_rel)==1`, avant le
  dry-scan, et non d'une nouvelle lecture effaçable par une annonce concurrente.
  Sur refus G, les tranches restantes sont encore jouées ; les nouvelles tâches
  de forêt peuvent être évitées, mais la région attend les travaux déjà partis.
  Un refus du noyau ne libère pas ses tampons sous une aide active : ils restent
  possédés jusqu'au retour de la région. Un échec d'étape ne libère pas ses
  successeurs ; les refus sont fusionnés après le retour du Pool, G en premier.

Le lecteur rejoue le [fragment historique de 47 événements](../../audit_reponses_20261008/a6_prefixe_relaxed/README.md),
dont le script est épinglé : variante relâchée admise par ses relations examinées,
pont feuille release/acquire rejetant le parent futur. Ce n'est ni un vérificateur
C++ indépendant, ni une construction géométrique FULL. La terminaison générale
suppose des tâches finies, le progrès des opérations atomiques et des participants ;
le petit modèle SC de fermeture ne démontre pas la vivacité du planificateur entier.

## N et H : sorties déterministes, travail distinct

Une cohorte est écrite par le morceau contenant son début, même si sa fin dépasse
ce morceau. Les autres morceaux sautent ce préfixe de cohorte. Les intervalles
écrits sont donc disjoints et couvrent les naissances ; la clôture n'arrive qu'après
tous les morceaux. **6 138 partitions/morceaux bornés** et cinq cas de frontière
16384 sont contrôlés par le modèle ; le comparateur exact natif n'est pas exécuté.

Le calcul H par chemins d'attache donne la même profondeur que la récurrence
inverse des événements : le survivant ne perd qu'ultérieurement. **3 039 histoires
d'unions par taille à 1..5 naissances / 15 017 profondeurs** sont comparées.
La taille d'une composante double chaque fois que sa racine perd : profondeur
≤ floor(log2(nb)), donc ≤ 30 dans le domaine nb ≤ 2^31−1. Le travail des chemins
peut être O(nb log nb), contre O(nb) pour la passe inverse ; le parallélisme ne
transforme pas cette différence en gain prouvé. H lit les attaches conservées,
pas les événements susceptibles d'être libérés par R après `kHistory`.

Enfin, le seuil 43 900 sites est une politique empirique fixée avant exécution,
identique pour tous les ordres. Il ne prouve pas que le noyau K de toute trame
au-dessus du seuil termine après G. Les identités et performances de cette
bascule restent à qualifier sur les cohortes prescrites et leurs deux côtés.

```sh
python3 -B -S check.py /chemin/a6c_prelecture_20261010_1800 /chemin/depot
python3 -O -B -S check.py /chemin/a6c_prelecture_20261010_1800 /chemin/depot
```
