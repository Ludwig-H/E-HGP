# Préflight statique — profil G4 échelle v6

Date : 2 septembre 2026. Coupe source : `8afd1057`. Profil examiné :
`gcp-migration/profils/g4_echelle_v1.env`, encore non versionné au moment de
la lecture.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Audit strictement statique : aucun appel GCP et aucune mutation externe par
l'auditeur. Une session gardée lancée par Claude reste sa cible ; elle n'est
ni adoptée ni arrêtée ici.

## Verdict utile

La sécurité de session est correctement dessinée : `g4-standard-48`, SPOT,
`instanceTerminationAction=STOP`, arrêt invité, durée GCE de 28 800 s et
arrêt ciblé vérifié. Les 465 minutes invitées satisfont exactement la borne du
lifecycle. Aucun P0 de coupe-circuit n'est trouvé.

Le profil n'est en revanche **pas exécutable par le canon courant**. Il doit
rester en NO START jusqu'au raccord local ci-dessous ; ce refus n'invite ni à
contourner le lifecycle ni à prolonger une session déjà ouverte.

## P1 — raccorder réellement le profil et l'axe K

1. `g4_echelle_v1.env` manque aux listes `PROTOCOL_FILES` de
   `v6_campaign_pin.sh` et `v6_session_lifecycle.sh`, ainsi qu'à leurs portes.
   Il sera absent de `pinned/`, puis `CAMPAIGN_PROFILE=g4_echelle_v1` sera
   refusé comme profil canonique inconnu.
2. Le profil encode `FRONTIER_SPECS` comme `famille:n:smax`, mais
   `v6_campaign_remote.sh` n'accepte que `famille:n` et impose
   `--smax=11`. Le validateur reconstruit lui aussi `--smax=11` : les tokens
   `:6` sont donc refusés avant tout run. Une correction partielle de la regex
   risquerait ensuite de lire `n=6` ou de faire collisionner les points K5/K10.
   Porter `smax` de bout en bout dans le plan, l'argv, le `.status`, le résumé
   et le validateur ; suffixer les artefacts hors K10, par exemple `_s6`.
3. Choisir et graver le layout dans le plan : `classic` pour une baseline ou
   `csr` pour mesurer le palier KeyCSR. Depuis `8afd1057`, le laisser implicite
   rendrait un futur reçu architecturalement ambigu.

Le profil affirme qu'un timeout est une donnée, tandis que le validateur
classe tout code 124 comme sortie non attribuée. Soit le superviseur écrit un
marqueur causal vérifiable, soit 124 reste une observation censurée et ne
décide pas la frontière.

## Portée scientifique à corriger sans alourdir la session

- Le bras terrain K5 ne contient que 1 M et 2 M : il estime une sécante
  1 M→2 M, pas la pente annoncée 200k→2 M.
- Le plan pilote mesure `uniform/100k` et `terrain/200k`, mais aucun témoin
  50k sous le même commit et le même binaire. Il peut publier les coûts à ces
  points ; parler de croissance « depuis 50k » demanderait des témoins appariés
  50k, éventuellement dans un petit préfixe commun.
- Un `std::bad_alloc` générique n'identifie pas « l'étage qui déborde » : les
  RSS par étage ne sont imprimés qu'après succès. Sans capture typée par étage,
  la conclusion honnête est la frontière de complétion et le dernier profil
  réussi.

## Budget et mémoire

L'estimateur du lifecycle calcule 13 800 s de frontières, 6 500 s de pilotes,
2 700 s de build/portes et 140 s d'overhead, soit 23 140 s pour une fenêtre
utile de 23 995 s. La somme des plafonds atteint 23 700 s : il ne reste que
855 s de marge nominale et 295 s hors surcoûts. Cela contredit le commentaire
du profil (`14 600 s` de frontières et environ `4 900 s` de pilotes). Déclarer
un préfixe obligatoire et un suffixe optionnel, séparer Q1/Q2 ou réduire
`FRONTIER_SPECS` évite de promettre un reçu complet sur une troncature
prévisible.

`FRONTIER_ULIMIT_KB=183500800` vaut exactement 175 Gio. Le reçu antérieur de
cette G4 grave 185 463 908 Kio, soit environ 176,872 Gio : 1,872 Gio n'est que
l'écart numérique entre `MemTotal` et la limite d'espace virtuel, pas une
réserve physique garantie. Le protocole grave déjà `MemTotal`, `MemAvailable`
et la charge ; il faut les valider et les rapporter. Garantir davantage de
marge face à l'OOM killer demande une limite plus basse ou une réserve mémoire
explicite, car `RLIMIT_AS` ne borne pas à lui seul le RSS du système.

## Fermeture minimale

1. Versionner le profil dans tous les inventaires normatifs et leurs mutants.
2. Porter `smax` et le layout choisi dans toute la chaîne, avec noms sans
   collision et validation exacte.
3. Aligner le statut des timeouts, les claims de portée et le commentaire de
   budget.
4. Rejouer les selftests campagne/lifecycle/revalidation, puis seulement
   demander un GO distinct pour une nouvelle tentative gardée.

Aucun résultat G4 ni GO produit n'est créé par ce préflight.
