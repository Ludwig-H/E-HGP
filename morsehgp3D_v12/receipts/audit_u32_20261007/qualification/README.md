# Provenance et portée des annonces du socle u32

Lecture indépendante épinglée à `c3de9d73d8999f2f1e31a0f592b829efc6e7a4da`.
Le contrôle lit Git et les manifestes JSON ; il ne construit aucun programme et ne
rejoue aucune campagne native. GCP et données réelles non utilisés.

## Résultats mécaniques

- **CST-0115 : correction historique confirmée.** À `a0091e2b7`, la table des
  ports classe à tort `reference/tests.cmake` comme renommage seul. À
  `95247cf4b`, ses ajouts sont explicitement déclarés : les 209 empreintes des
  sources v11 sont conformes, et les 139 fichiers déclarés copies ou renommages
  sont conformes à leur classe. Le fichier des portes de référence est identique
  entre cette correction et le pin courant. Cela permet de clore ce constat précis.
- **La table doit maintenant dater ses états.** Au pin courant, les 209 empreintes
  d'origine restent conformes ; 30 fichiers déclarés copies ou renommages ont
  toutefois changé avec le socle numérique. Leur liste figure dans `result.json`.
  Le reçu du développeur décrit cette nouvelle tranche, mais `PORTS.md` et
  `PROVENANCE.md` n'y renvoient pas encore comme état courant ; ce dernier indique
  toujours que u32 est refusé. Il faut soit marquer explicitement la table comme
  état historique T0 et relier la tranche suivante, soit actualiser ses classes.
  Ce décalage ultérieur ne réouvre pas le défaut spécifique de `reference/tests.cmake`.
- Les **191 blobs** du socle retenus par la déclaration de raccord sont identiques
  entre `26b53648c` et `3d6c92c1f`. Le commit numérique `6a38f7e4b` touche **82 fichiers**
  de cette tranche élargie à README/architecture : 3 637 lignes ajoutées,
  729 supprimées. Cela confirme la base d'intégration, pas son comportement.
- Les manifestes contiennent `core` 84, `num` 76, `index` 18, `cloud` 17, `io` 22,
  `sched` 8 mutants, avec les mêmes planchers. Un manifeste décrit des essais à
  exécuter ; ses effectifs ne prouvent ni leur exécution ni leur résultat.

## Ce que les reçus ne permettent pas de certifier ici

Les rapports du développeur annoncent les portes rapides aux profils 21/24/32,
172 portes ASan/UBSan aux profils 21 et 32 et des campagnes de mutants. Les journaux
et builds historiques correspondants n'ont pas été récupérés lors de cette reprise.
Ils ne sont donc ni relus ni rejoués par ce reçu ; aucune fermeture des constats
numériques ne repose ici sur ces seuls effectifs déclarés.

Le rapport situe explicitement ces campagnes dans le codespace, hors de la règle
des tests lourds sur G4, et les appelle contrôles locaux d'appoint. Elles ne
qualifient pas la matrice G4 exigée par `CST-0002`, ni D6 (surcoût du profil), ni le
contrat FULL/100 ms. Les rapports et leurs empreintes sont conservés comme annonces
du développeur, sans transfert de qualification.

**CST-0024 reste ouvert.** Le reçu d'intégration rapporte une expiration à 300 s,
puis un échec, puis treize passes du différentiel v10. La cause n'est pas établie ;
les passes ultérieures ne démontrent ni une cause liée à la charge ni une correction.
Aucun nouvel essai de ce différentiel n'a été lancé par cet audit.

## Reproduction

Depuis la racine du dépôt, exécuter `python3 -B` puis `python3 -B -O` sur
`morsehgp3D_v12/receipts/audit_u32_20261007/qualification/check.py` ; comparer les
deux sorties à `result.json`. La lecture exige les objets Git historiques cités.
Les sources locales ancrées sont comparées à leur contenu Git puis hachées avant et
après. `verification.json` enregistre l'identité des captures normale et optimisée ;
`SHA256SUMS` ferme les fichiers de ce reçu. Les contrôles utilisent des exceptions,
pas des assertions effacées par `-O`.
