# Contrelecture du front q2 et complément LiDAR

20 septembre 2026. Auditeur indépendant A, source **3e94c868**.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `not_claimed`. GCP non utilisé.

**Verdict : les tranches 20/21 sont cohérentes dans leur périmètre q2.**
La contrelecture mathématique et du transport par jobs/dons ne révèle pas
de défaut nouveau. Le complément LiDAR confirme une réduction du travail
sur trois scans réels. Trois contre-fixtures précisent le contrat d’un
futur héritage q3/q4 ; elles ne mettent pas en défaut le code q2 livré.

## LiDAR : mesures du moteur public, sans variante produit

La [sonde](probe.cpp) adapte explicitement la sonde constructeur du commit
audité, dont le SHA256 est dans son en-tête. Seule la génération synthétique
est remplacée par la lecture des sites u16le existants. Appel public mono,
SharedBlocks, Saturating, ComplementFirst, Individual, Pool64, K10/s8.
Le callback constructeur de condensé canonique est réutilisé et payé :
copie, tri, validation et hachage des IDs des intérieurs et coquilles.
Ce condensé est un contrôle différentiel, **pas un oracle exact indépendant**.

Les scans KITTI08 000000, 000100 et 000200 sont traités **séparément**, à
8k et 50k sites ; le scan000000 ajoute 16k/32k et trois répétitions à50k.
Aucun recalage entre scans, aucune correspondance de points ni hypothèse
d’alignement exact. Les données sont les sites uniques quantifiés à2cm
de la préparation antérieure, pas les retours flottants bruts. Les hashes
sont recontrôlés contre les métadonnées de cette préparation.

Quatre configurations par groupe, quarante mesures : défaut
`{1,all,false}`, `{2,16,false}`, `{2,16,true}`, `{4,all,true}`.
Le dernier contraste change aussi la limite des facteurs : il ne mesure
pas l’effet isolé de la seule largeur4K. Source et compilation nouvelles
figées dans ce dossier, sans toucher les builds épinglés ni le chantier q4
en cours. Affinité CPU0, un fil, hôte partagé avec développement concurrent.
Les observations chronométrées restent locales ; les comptes répétés sont
identiques. Aucun warm-up ni essai défavorable n’a été retiré.

Scan000000, 50k sites ; temps mur front+census+collecte/callback, médianes
des trois observations. Préparation du nuage/index exclue de cette colonne
mais présente dans chaque temps total brut :

| Option | Secondes | Candidates census | Visites census | Tests H du front |
| --- | ---: | ---: | ---: | ---: |
| Défaut | 9,604 | 13 376 771 | 313 934 108 | 58 025 800 |
| 2K/16 | 6,156 | 6 466 161 | 165 262 594 | 71 277 782 |
| 2K/16 + héritage | 5,307 | 4 763 355 | 125 648 339 | 50 865 433 |
| 4K/tous + héritage | 4,774 | 2 616 596 | 77 827 922 | 100 418 426 |

Les quatre flux portent1 040 133 supports, avec condensés, masses d’IDs
intérieurs et coquilles identiques. Les plages de temps sont larges :
défaut9,145–11,769s, 2K/héritage5,142–7,415s, 4K/héritage4,750–8,710s.
Dans le premier groupe50k, 4K/héritage est plus lent que2K/héritage ;
cette observation est conservée. Comparer aussi le surcoût des tests H,
pas seulement la baisse du census.

À50k, les visites census défaut → 2K/héritage → 4K/héritage valent :

| Scan | Défaut | 2K/16 + héritage | 4K/tous + héritage | Supports conservés |
| --- | ---: | ---: | ---: | ---: |
| 000100 | 253 496 137 | 100 621 609 | 67 261 576 | 1 020 809 |
| 000200 | 445 873 448 | 226 241 390 | 83 048 903 | 1 020 186 |

Les observations uniques correspondantes prennent9,869→4,883→4,708s et
11,418→6,790→4,980s. Le résultat utile pour les prochaines campagnes est
de **passer explicitement `{2,16,true}` comme référence optimisée et de
garder `{4,all,true}` comme candidat LiDAR à comparer**, sans changer le
défaut de l’API ni en faire un choix universel. La réduction des visites
est robuste sur les entrées mesurées ; une accélération générale, une borne
sous-quadratique, la tour50k/FULL et les contrats GPU/G4 restent ouverts.

## Réponses aux questions sur les preuves

- L’hérédité Hmin est correcte : restreindre les boîtes ne diminue pas le
  minimum. Au même préfixe proposé, les rangs reçus et les nouveaux succès
  couvrent les succès du balayage seul ; l’arrêt ne peut être plus tardif.
  Cela ne rend monotones ni les listes finales ni le nombre de rejets.
- En q2, chercher impose n−|A|−|B|≥K, donc n≥K+2. La fenêtre2K ou4K
  contient strictement celle deK, même au bord. Cette preuve est propre q2.
- En notant N les nouveaux crédits, I les crédits reçus, J les produits
  rejetés, E les crédits émis et S les crédits transmis par les scissions :
  I=2S et N+I=KJ+E+S, donc N+I/2=KJ+E. Pour un job complet recevant c
  crédits à sa racine, le terme devient N+(I+c)/2=KJ+E. Une frontière de
  tâches non exécutées exige aussi son terme de sortie. Le registre seul
  ne prouve ni les identités distinctes ni la validité géométrique.
- `inherited_rejections` est bien un majorant du nombre de rejets locaux
  impossibles à la fenêtre seule, pas leur nombre exact ni un gain net.
  Le suffixe non testé pourrait encore trouver K succès. Les rangées
  publiées illustrent déjà ce piège ; aucune anomalie nouvelle à leur sujet.
- « Total moins extension » restitue la fenêtre historique **sur les
  produits visités dans cette exécution**, pas l’ancien run. En tranche21,
  la projection du lecteur classe aussi les rangs hérités revus parmi les
  sauts sans test : sa colonne projetée `proposals_in_factors` n’a plus
  son seul sens géométrique. Les lecteurs le documentent correctement.
- Ne pas porter immédiatement la reprise exacte de descente est raisonnable
  vu le petit gain complet mesuré. Une réévaluation massive doit mesurer
  les profondeurs effectives : u16 borne déjà chaque chemin par48 niveaux.

## Trois gardes pour le futur héritage multivoie

Le [modèle entier indépendant](math_gate.py) n’importe aucun code produit.
Il construit l’arbre midpoint et le parcours décrit pour vérifier que les
contre-fixtures sont atteignables, puis attaque trois mauvaises adaptations.

1. **Permettre la promotion d’un rang reçu.** A={(0,0,0),(0,4,0)},
   B={(10,0,0),(10,4,0)}, z=(5,4,0), K2/masque3/s8. Le parent donne
   Hmin=9 et Ξhigh=1600 : crédit q2 seulement. Sur A'={(0,4,0)},
   Hmin=25 et Ξhigh=400 : crédit q4, donc q3. Sauter entièrement le rang
   reçu perd le rejet q3 que retrouve la fenêtre sans héritage. Dédupliquer
   par voie déjà certifiée ; reproposer les voies plus exigeantes encore actives.
2. **Conserver une liste pendant une recherche sautée.** Abscisses
   0,1,5,10,11, K3/masque5/s12. Le témoin5 rejette q4 sur
   {0,1}×{10,11}. Après subdivision, l’enfant n’a que deux sites extérieurs,
   sous le seuil q2=3, mais reçoit ce témoin q2. La garde actuelle
   « recherche sautée ⇒ liste vide » n’est pas portable telle quelle.
3. **Accepter une extension vide.** Abscisses0,1,10,11, n4/K4/masque4,
   facteur2 : la recherche q4 peut échouer avec une fenêtre historique
   couvrant déjà tous les sites. Aucun rang supplémentaire n’existe.

Ces fixtures ne sont pas un juge complet des bornes Ξ, un moteur q3/q4 ou
un contre-exemple au front q2 actuel. La brique de famille q4 ouverte le
20septembre ne porte pas cet héritage ; sa lecture précoce ne révèle pas
d’obligation manquante dans l’objet annoncé. Sa qualification reste séparée.

## Preuves et reproduction

[SUMMARY.json](SUMMARY.json) contient les quarante observations, les
médianes et les contrôles différentiels. [capture/COMPLETION.json](capture/COMPLETION.json)
ferme commandes, sorties brutes, entrées, binaire, scripts de mesure et
instantané. [SOURCE_PINS.json](SOURCE_PINS.json) épingle les sources du commit.
La sonde est compilée avec C++20/O3 et avertissements stricts ; commandes et
sorties sont conservées dans `configure.json` et `build_*.json`.

`python3 verify.py` et `python3 -O verify.py` rendent le même JSON : dix
groupes appariés, comptes répétés identiques, quatre mutants du lecteur
rejetés. `python3 math_gate.py` et `python3 -O math_gate.py` rendent aussi
le même JSON : trois fixtures, trois mutants de conception,441 426 cas
arithmétiques q2. Le rejeu de la porte constructeur épinglée est conservé
séparément dans `constructor_gate_replay.json` ; il ne devient pas notre oracle.

Un préflight du lecteur comparait par erreur les deux durées Pool comme des
compteurs discrets et a échoué. Son source et son échec restent dans
`preflight/` ; seule l’exclusion explicite de ces deux durées a été corrigée,
sans refaire ni modifier une mesure. Les lectures finales font autorité.
Pour une nouvelle campagne, reconstruire l’instantané depuis le commit et
utiliser un dossier neuf : le lanceur refuse d’écraser `capture/`.
