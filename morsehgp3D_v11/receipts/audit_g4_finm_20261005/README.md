# Relecture individuelle de finm close — 5 octobre 2026

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
La campagne ci-dessous s’exécute explicitement en u18.

Session `v11.20261005.claudefinm`, source publiée
`38b76701b9b0198fc1c37afe16e1480e638e513c`. DONE=3,
`failed_remote`, worker=1 ; arrêt ciblé certifié. Tous les SHA et l’égalité
exacte des 635 fichiers utiles livrés sont vérifiés dans le résumé.

Les 39 portes CTest se terminent : 37 PASS, 2 Failed, aucune manquante.
Ce nombre de portes n’est pas un nombre de mutants. Aucun rapport
`mutants_<module>.json` n’est conservé dans l’archive. La lecture utilise les
lignes individuelles de `LastTest.log`, regroupées par campagne et vérifiées
contre les IDs des manifestes du paquet épinglé.

| État individuel | Nombre |
|---|---:|
| Déclarés dans les 13 manifestes | 485 |
| Effectivement jugés | 462 |
| TUE par code | 455 |
| TUE par ligne absente | 4 |
| Rejets de construction attendus | 2 |
| SURVIT | 1 |
| INVALIDE, signal, délai | 0 |
| API non jugés après témoin rouge | 23 |

Les deux rejets de construction concernent les mutants core
`refus_construit_une_valeur` et `identifiants_confondus`, déclarés comme
preuves de construction dans leur manifeste. Ils restent séparés des mises
à mort à l’exécution et des compilations invalides. Les campagnes head
(10 mutants), points (6) et num (57) jugent et tuent effectivement tous leurs
mutants. Les IDs et catégories des 462 jugements sont conservés dans le résumé.

API : le témoin sans mutation échoue sur
`mhgp11_api_supports_route_scale8000` avec `ligne_absente`. Sa sortie métier
est conforme, avec les comptes/journal attendus, mais les préfixes SHA u18
réels `09a1101bc512394e` / `e10e6c8d117429e3` diffèrent des préfixes u21 gravés
`9b77614618bbdfc9` / `d61003785c2ff158`. Le lanceur annonce explicitement
`TEMOIN ROUGE module=api : aucun mutant juge`. Aucun des 23 mutants API
n’est classé comme tué ou survivant par cette campagne.

CLI : 27 mutants TUE par code et un verdict explicite SURVIT pour
`sp_masque_16379`, dont `mhgp11_cli_supports_oracle` passe sur la copie mutée.
La [relecture des chemins de code](cli_mutant/README.md) établit que cette
porte ne passe plus par la fonction mutée depuis L2b. Elle propose de
conserver la mutation et de la raccorder à `mhgp11_cli_points`, qui
l’atteint encore. Ce raccord reste à qualifier par le harnais natif ;
aucun contre-exemple géométrique n’est établi par le survivant actuel.

La qualification globale reste ouverte. Pour les cas courts ASan/TSan,
`build/v11-persist/qual_sorties/chaine_fin.sh` prévoit explicitement B après
les onze lots S : ordre `a2 m s b l p10 p9 mesure`, source 38b76701b et
`data_complet`. `plan_b.json` sélectionne `gcc_asan_ubsan,gcc_tsan` ; son
préflight `dryfin_b.json` est validé au même pin. Cette modalité prévue
n’est pas un résultat exécuté ni une nouvelle demande de campagne.

Rejeu, Python normal ou −O :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

`--session` et `--repo` permettent de déplacer les dépendances locales.
Le script vérifie fermeture, hashes, sources utiles, inventaires CTest et
records individuels, puis compare le résultat au résumé figé. Aucun build,
test natif, réseau ou accès cloud. Dépendances : archives locales de finm et
commit Git ; cette capsule n’est pas un reçu autonome.

Aucun journal brut, sortie binaire native, identité de compte ou octet LiDAR
copié. Seules les métadonnées de qualification et leurs empreintes sont publiées.
