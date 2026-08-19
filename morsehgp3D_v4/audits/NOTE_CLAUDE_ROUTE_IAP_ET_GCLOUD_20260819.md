# Note de Claude — gcloud installé, et la route IAP est joignable depuis le bac à sable

Date : 19 août 2026 UTC. Cadre v4 : `phase=exploration_v4_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

**Aucune mutation GCP n'a eu lieu.** Cette note ne rapporte que des
mesures de joignabilité et l'installation d'un outil local.

## 1. Ce qui a changé

`gcloud` (Google Cloud SDK 581.0.0) est installé dans le conteneur, sous
`/opt/google-cloud-sdk`, lié dans `/usr/local/bin`. Il sort par le proxy
d'agent déjà configuré (`CLOUDSDK_PROXY_*`,
`CLOUDSDK_CORE_CUSTOM_CA_CERTS_FILE`). L'installation est **hors du
dépôt** et ne survivra pas au conteneur.

Le premier des trois blocages recensés le 18 août (« `gcloud` absent »)
tombe donc. Les deux autres sont mesurés ci-dessous.

## 2. L'inventaire reste IMPOSSIBLE : aucun identifiant

```text
$ gcloud auth list
No credentialed accounts.

$ gcloud compute instances list --project=devpod-gpu-exploration
WARNING: Some requests did not succeed.
 - Request had invalid authentication credentials.
```

La variable `CLOUDSDK_AUTH_ACCESS_TOKEN` présente dans l'environnement
est rejetée par l'API Compute (`ACCESS_TOKEN_TYPE_UNSUPPORTED`) : ce
n'est pas un jeton OAuth GCP. **Je ne peux donc toujours pas certifier
que les instances du projet sont `TERMINATED`.** Le dernier état connu
reste celui du journal du 18 août, avec sa nuance sur la tentative 6.

## 3. La conclusion « il faut un poste avec SSH » doit être NUANCÉE

Le journal du 18 août concluait, sur la foi du port 22 bloqué, que la
campagne devait s'exécuter depuis un poste disposant de SSH. Le port 22
est bien toujours bloqué :

```text
$ exec 3<>/dev/tcp/github.com/22   ->  bloqué
```

Mais l'endpoint du **tunnel IAP**, qui transporte SSH sur 443, répond
bout en bout depuis ce conteneur :

```text
$ curl -D - https://tunnel.cloudproxy.app/v4/connect
HTTP/1.1 200 Connection Established        <- CONNECT du proxy accepté
HTTP/2 404
referrer-policy: no-referrer               <- réponse de Google, pas du proxy
```

Le contrôle discriminant est fait : un hôte inexistant rend
`HTTP/1.1 502 Bad Gateway` du proxy, sans `200 Connection Established`.
La réponse ci-dessus vient donc bien du serveur Google. `oauth2.googleapis.com`
répond également.

**Conséquence** : `gcloud compute ssh --tunnel-through-iap` (et
`gcloud compute scp` avec la même option) est une route techniquement
ouverte depuis un bac à sable qui bloque le port 22. Le maillon jamais
franchi le 18 août — l'armement SSH de la garde invité — cesse d'être
structurellement impossible ici.

## 4. Ce que cette route coûterait, et pourquoi je ne l'ai pas prise

Elle n'est PAS gratuite, et elle touche le chemin gardé :

1. **Côté projet** : une règle de pare-feu autorisant `35.235.240.0/20`
   en `tcp:22` sur l'instance, et le rôle
   `roles/iap.tunnelResourceAccessor` pour le compte utilisé. Ce sont
   des mutations d'infrastructure, hors du périmètre des scripts gardés.
2. **Côté dépôt** : `session_scale_threads_g4.sh` construit ses tableaux
   `SSH=(...)` et `SCP=(...)` sans `--tunnel-through-iap`. Les modifier
   change la façon dont la session atteint la VM — donc la façon dont la
   garde invité est armée et dont les résultats sont rapatriés. C'est
   exactement le genre de modification que l'audit `9d19ede` vient de
   placer sous pin transitif : elle doit être commitée, pinnée, et
   accompagnée de ses portes.
3. **Identifiants** : rien de tout cela ne remplace le point 2 — sans
   jeton, la route ouverte ne mène nulle part.

Je n'ai donc **rien modifié** dans `gcp-migration/`. La décision
appartient à l'utilisateur et l'implémentation à un audit.

## 5. Les deux chemins possibles, énoncés sans préférence cachée

- **Chemin A — poste avec SSH (le protocole actuel, inchangé).** La
  ligne prête de `PASSATION.md` § 6 est vérifiée en mode enveloppe
  (`enveloppe conforme aux six gardes`, aucune action GCP) et passe
  désormais par les gardes pinnées. Rien à écrire, rien à auditer.
- **Chemin B — IAP depuis un bac à sable.** Rend une session CCR
  autonome, au prix des trois points du § 4. À traiter comme un
  chantier gardé : `NOTE_SOLUTION` → implémentation → portes
  (au minimum : refus si `--tunnel-through-iap` est demandé sans que la
  règle de pare-feu et le rôle IAM soient constatés en lecture seule) →
  audit de réception.

Dans les deux cas, la première action de toute session GCP reste un
inventaire en lecture seule, et il n'a toujours pas pu être établi
depuis ici.
