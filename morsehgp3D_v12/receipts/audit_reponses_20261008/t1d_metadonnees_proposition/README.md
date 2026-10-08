# T1-d adopté : proposition de comptage des descripteurs

Contrôle du commit `caf9585e4a89ba101ba86d2d46e77cb340be15bf`, 8 octobre 2026. Les sept sources de `capture.json`
sont identiques à celles de `5f8e777cf` : les métadonnées non comptées et le détail appareil incomplet signalés dans
[t1d_produit](../t1d_produit/README.md) restent présents. L'adoption de T1-d2 n'est pas annulée par cette observation ;
aucun résultat réel n'est relu ici. Proposition exclusivement textuelle : aucun moteur, compilation, test natif,
GPU ou donnée sous licence. Produit et travaux non commis laissés intacts.

`plan.patch` remplace le vecteur de `Bin` par un Buffer à capacité croissante et un compteur logique séparé.
L'allocation du nouveau stockage précède la copie, tandis que l'ancien reste compté ; après échange, l'ancien
est rendu avant l'ajout. Tous les accès conservent `[0,count)` : aucune lecture de capacité non initialisée.
Une allocation refusée conserve le plan précédent et rend `memory_budget`. L'ordre des tranches ne change pas.
Le patch ne ferme QUE ce premier conteneur ; ce n'est pas un correctif complet des trois métadonnées.

Les deux autres conteneurs portent des types non triviaux (`Buffer<LevelWords>` et `Chunk`). `Buffer<T>` les refuse
explicitement : remplacer leur `std::vector` par `Buffer<T>` ne constitue pas une solution compilable.
[DESCRIPTEURS.md](DESCRIPTEURS.md) donne le raccord précis : un stockage compté d'objets, taille S connue pour les
niveaux, capacité croissante pour les lots, destruction des objets AVANT retour du stockage au cache. Ce raccord
reste du pseudocode à traduire et à qualifier ; aucune classe générique non éprouvée n'est ajoutée au produit.

Pour S entrées, la capacité géométrique du plan est inférieure à 2S ; pendant une croissance, ancienne et nouvelle
capacités totalisent moins de 3S entrées (S≥2), donc moins de `120*S` octets logiques de Bin. Une entrée isolée
demande 40 octets. Les arrondis physiques éventuels du cache ne sont PAS assimilés à cette borne logique : Buffer
réserve leur taille physique effective et respecte la limite même pendant la coexistence. Un nouveau refus dû à
ces métadonnées est attendu : les anciens pics ne les mesuraient pas. Les allocations des histogrammes, comptes
de répartition et contenus étaient déjà comptées ; elles restent à additionner, pas remplacées par cette borne.

Le correctif des durées peut rester local et indépendant : chronométrer `materialize_levels` (allocation,
conversion et rendu des mots compris) dans `slices_close`, transporter ce temps dans FinishStats et l'ajouter à
la publication appareil UNE fois. Chronométrer également le rassemblement hôte avant les envois, en retranchant
seulement les transferts effectivement mesurés si la fenêtre les englobe. Le résidu englobant CPU les comprend
déjà : ne pas lui ajouter ces temps une seconde fois. Aucun changement du mur C ni des comptes logiques.

Validation textuelle : les préimages du patch sont uniques dans les objets Git et sa postimage est hachée dans la
capture. **Pas de compilation, pas d'exécution du code proposé.** Avant intégration, le développeur doit éprouver
les refus à chaque croissance/descripteur, rendu sous cache et sans cache, réemploi du contexte, et les identités
des catalogues à 1, 2 et plusieurs tranches. Cela porte sur le stockage et ses refus, pas sur une nouvelle politique
géométrique. Aucun gain de temps ni pic réel après correction n'est annoncé.

Révision AVANT première publication, demandée en contrelecture : conservation de l'initialiseur membre `{}` dans
`Buffer<Bin> slices{};`, comme pour l'ancien vecteur. `Planner` reste initialisé avec ses cinq premiers membres ;
on évite ainsi une initialisation membre omise sous les avertissements stricts. Ancienne empreinte de manifeste,
ancien patch et ancienne postimage sont tracés dans `capture.json` ; aucun reçu publié n'est réécrit. Contrôle
textuel `git apply --check` réussi, sans compilation. Le complément sur les empreintes des niveaux et de la table
est traité séparément par l'auditeur mathématique.
