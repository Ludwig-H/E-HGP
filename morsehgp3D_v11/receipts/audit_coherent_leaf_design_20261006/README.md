# Feuille cohérente : réponse à la section Q

6 octobre 2026, source `ee3eabe5e7aae4d95515f63935e8f9a84adbd468`.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Conception favorable avec les invariants ci-dessous ; aucun noyau nouveau,
build, test natif ou appel GCP exécuté par les auditeurs.

**Le résultat est celui du premier événement dans l'ordre séquentiel.**
Pour le census, θ=K+1−q : retenir le premier entre le (θ+1)-ième intérieur
connu et le premier refus réellement atteint. Compter le site d'arrêt.
Pour la canonisation, retenir le premier succès **ou refus**, par arité
puis ordre lexicographique, avec les gardes internes de chaque candidat.
Le rang du cache J2 n'est pas le rang lexicographique des supports.

- [Réponse mathématique Q1/Q2](mathematics/REPORT.md) : formules, helper
  de masques et traces symboliques. 33 contrôles normal/−O identiques.
- [Contrat CUDA Q3](cuda_contract/README.md) : une seule consultation et
  comptabilisation logique de J2, participation aux collectives, listes
  ordonnées et certificats avant arithmétique spéculative. Modèle de cache
  à 32 ordres et cinq compactages, normal/−O conformes.
- [Réponse active complète](../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md#feuille-coherente--reponse-a-la-section-q-du-6-octobre).

Les traces de refus injectées testent le contrôle ; elles ne sont pas des
nuages. Dans le code actuel, G3 et le certificat global de `side` rendent
inaccessible un refus `side` réel après le dépassement θ+1. Ne pas exiger
une détection native de cette mutation sans témoin atteignable. Les
certificats sont vérifiés avant le calcul entier ; ignorer un résultat
après l'arrêt n'autorise jamais une arithmétique hors borne.

**Deux constats antérieurs restent ouverts en source.** Le
[patch restant, recalé sur ee3](remaining_fixes_ee3.patch) ajoute la frontière
de phase avant les écritures du drapeau de refus ; il adapte l'attente
`near_max` à u18 et impose u21 au mutant de publication partielle.
Les autres corrections proposées en f030 sont intégrées autrement par le
développeur : manifeste catalogue et boîte stricte. Les
[anciennes preuves](../audit_coop_wip_followup_20261006/README.md) restent
inchangées ; le patch actuel est applicable au pin ee3, sans application
dans le worktree développeur ni qualification native prétendue.

Chaque sous-dossier garde son manifeste d'origine ; le manifeste racine
ferme cette compilation et les empreintes du patch. Aucun octet LiDAR
n'est inclus. [Coop1](../audit_g4_coop1_20261006/README.md) qualifie son pin
c3df et ses données propres, pas cette nouvelle conception.
