#!/usr/bin/env bash
# v11_worker.sh -- worker invite d'une session G4 v11 (backend=reference_cpu, public_status=not_claimed).
#
# Port explicite de gcp-migration/v10_worker.sh : memes etapes, memes garanties. Differences : la
# lignee (morsehgp3D_v11, mhgp11*, schemas ehgp.v11.*) ; la source du paquet (--source a la place de
# --commit) ; le plan (PLAN_PYTHON_PINNED : aucun controle pip par defaut ; PLAN_DEFAULT_BUILD : la
# construction Release peut etre laissee aux commandes) ; les faits de la VM (env/vm_facts.txt) ; la
# variable MHGP11_DATA_DIR exportee vers les commandes.
#
# Lance DETACHE (setsid nohup) par gcp-migration/v11_session.py sur la VM, depuis le paquet de la
# session : archive `git archive` d'un commit pousse (--source commit:SHA), ou instantane de l'arbre
# de travail (--source worktree_snapshot:SHA de HEAD ; essai de developpement, jamais un recu) :
#
#   bash v11_worker.sh --src DIR --data DIR --plan FICHIER --work DIR --deadline-epoch N \
#       --source NATURE:SHA --plan-sha256 SHA --package-sha256 SHA --generation HORODATAGE
#
# Etapes : releve de l'environnement (nproc, lscpu, charge, versions, chemins des outils, faits de
# la VM : compilateurs, bibliotheques de sanitizers, drapeaux du processeur),
# verification du plan, du paquet et des donnees (sha256), Python epingle SEULEMENT si le plan le
# demande (python_packages = pinned ; sinon les commandes Python tournent avec le Python nu de la VM)
# (pip --user dans PYTHONUSERBASE=$WORK/pyuser, jamais ~/.local ; --only-binary --no-cache-dir,
# versions exactes, sklearn.cluster.HDBSCAN et
# hdbscan.validity exiges, pip freeze releve), construction Release de morsehgp3D_v11 (-j nproc)
# sauf si le plan dit default_build = false (les commandes construisent alors elles-memes),
# provenance (sha256 des binaires mhgp11*, compilateur de CMakeCache), puis commandes du plan
# DANS L'ORDRE, cwd = dossier de build.
#
# Chaque etape tourne dans son PROPRE groupe de processus (setsid) :
#   /usr/bin/time -v -o time.txt  timeout --foreground --kill-after=10s N  commande...
# GNU time mesure donc la commande (time.txt et RSS renseignes meme sur depassement du delai) ;
# le temps mural est pris a la sortie de la commande, puis tout residu du groupe recoit TERM, puis
# KILL, et la fermeture du groupe est certifiee (sinon l'etape n'est pas « ok »).
# stdout/stderr de chaque etape sont tronques (tete + queue, marqueur) au-dela de min(64 Mio,
# plafond/4). Un ctest n'est « ok » que si sa sortie dit « 100% tests passed, 0 tests failed out
# of N », N >= 1, SANS « The following tests did not run » (tests desactives ou sautes).
#
# SIGTERM (session a l'echeance ou abandon) : groupe courant tue, commandes suivantes sautees,
# emballage quand meme. A la fin : plafond de taille des resultats (seuls les GROS fichiers de
# {out} partent dans overflow/ sur la VM, listes dans overflow.txt ; jamais les petits), statut
# calcule APRES le plafond (tout debordement ou troncature => « overflow », jamais « completed »),
# results/MANIFEST.sha256, results.tar.gz, SHA256SUMS, puis worker.exit (publie en dernier,
# atomiquement ; une sortie inattendue le publie avec le code 5). Aucune installation systeme,
# aucun sudo, aucune commande cloud. Codes : 0 conforme, 1 echec ou debordement, 2 usage invalide,
# 3 emballage impossible, 5 sortie inattendue.

set -uo pipefail
umask 077
export LC_ALL=C

readonly TIME_BIN=/usr/bin/time
readonly PACK_RESERVE_SECONDS=120      # identique a WORKER_PACK_RESERVE de v11_session.py
readonly MIN_STEP_SECONDS=5
readonly KILL_GRACE_SECONDS=10
readonly PIP_TIMEOUT_SECONDS=600       # identique a PIP_TIMEOUT de v11_session.py
readonly MAX_RESULTS_BYTES=1073741824  # identique a MAX_RESULTS_BYTES de v11_session.py
readonly STREAM_CAP_MAX=67108864
readonly PIP_PINS=(numpy==2.2.6 scipy==1.15.3 scikit-learn==1.7.2 hdbscan==0.8.44)
readonly PLAN_MAGIC='# ehgp.v11.worker_plan.v1'
readonly RESULT_SCHEMA='ehgp.v11.worker_result.v1'

usage_error() {
  printf '[v11_worker] %s\n' "$*" >&2
  exit 2
}

SRC="" DATA="" PLAN="" WORK="" DEADLINE="" SOURCE="" PLAN_SHA="" PACKAGE_SHA="" GENERATION=""
while (($# > 0)); do
  (($# >= 2)) || usage_error "valeur manquante apres $1"
  case "$1" in
    --src) SRC="$2" ;;
    --data) DATA="$2" ;;
    --plan) PLAN="$2" ;;
    --work) WORK="$2" ;;
    --deadline-epoch) DEADLINE="$2" ;;
    --source) SOURCE="$2" ;;
    --plan-sha256) PLAN_SHA="$2" ;;
    --package-sha256) PACKAGE_SHA="$2" ;;
    --generation) GENERATION="$2" ;;
    *) usage_error "option inconnue : $1" ;;
  esac
  shift 2
done
[[ -n "${SRC}" && -n "${DATA}" && -n "${PLAN}" && -n "${WORK}" && -n "${DEADLINE}" && -n "${SOURCE}" && \
   -n "${PLAN_SHA}" && -n "${PACKAGE_SHA}" && -n "${GENERATION}" ]] || usage_error "arguments obligatoires manquants"
[[ "${SOURCE}" =~ ^(commit|worktree_snapshot):[0-9a-f]{40,64}$ ]] || usage_error "source invalide : ${SOURCE}"
[[ "${DEADLINE}" =~ ^[0-9]{9,11}$ ]] || usage_error "echeance invalide : ${DEADLINE}"
[[ -d "${WORK}" && ! -L "${WORK}" && ! -e "${WORK}/results" ]] || \
  usage_error "repertoire de travail absent ou deja utilise : ${WORK}"

RESULTS="${WORK}/results"
BUILD="${WORK}/build"
mkdir "${RESULTS}" || exit 2
EXIT_WRITTEN=0
publish_exit() {
  printf '%s\n' "$1" > "${WORK}/worker.exit.partial" && mv "${WORK}/worker.exit.partial" "${WORK}/worker.exit"
  EXIT_WRITTEN=1
}
on_exit() {
  if ((EXIT_WRITTEN == 0)); then
    publish_exit 5
  fi
}
trap on_exit EXIT
mkdir "${RESULTS}/env" "${RESULTS}/setup" "${RESULTS}/build" "${RESULTS}/cmd" "${RESULTS}/provenance" || exit 2
printf '%s\n' "$$" > "${WORK}/worker.pid.partial" && mv "${WORK}/worker.pid.partial" "${WORK}/worker.pid"
readonly STARTED_EPOCH="$(date +%s)"

FATAL=""
INTERRUPTED=0
CHILD=""
STATUS_DATA="pending"
STATUS_PIP="not_requested"
STATUS_BUILD="pending"
PYTHON_VERSIONS=""
COMMANDS_OK=0
PLAN_COUNT=0
NPROC=1
RESULTS_CAP=${MAX_RESULTS_BYTES}
STREAM_CAP=${STREAM_CAP_MAX}
MIN_EVICT=1048576
OVERFLOW_FILES=0
OVERFLOW_UNRESOLVED=0
TRUNCATED_STREAMS=0
STEP_RC=""
STEP_STATUS=""
: > "${RESULTS}/truncated.txt"

note() {
  printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" >> "${RESULTS}/events.txt"
}

on_signal() {
  INTERRUPTED=1
  if [[ -n "${CHILD}" ]]; then
    kill -TERM -- "-${CHILD}" 2>/dev/null || kill -TERM "${CHILD}" 2>/dev/null || true
  fi
}
trap on_signal TERM INT HUP

remaining_seconds() {
  local now
  now="$(date +%s)"
  printf '%s' "$((DEADLINE - PACK_RESERVE_SECONDS - now))"
}

# Ferme le groupe $1 : residu -> SIGTERM, grace KILL_GRACE_SECONDS, puis KILL ; renseigne
# GROUP_RESIDUAL (1 si des membres survivaient a l'etape) et GROUP_CLOSED (fermeture certifiee).
close_group() {
  local group="$1" i=0
  GROUP_RESIDUAL=0
  if kill -0 -- "-${group}" 2>/dev/null; then
    GROUP_RESIDUAL=1
    kill -TERM -- "-${group}" 2>/dev/null || true
  fi
  while kill -0 -- "-${group}" 2>/dev/null && ((i < KILL_GRACE_SECONDS * 5)); do
    sleep 0.2
    i=$((i + 1))
  done
  if kill -0 -- "-${group}" 2>/dev/null; then
    GROUP_RESIDUAL=1
    kill -KILL -- "-${group}" 2>/dev/null || true
    sleep 0.2
  fi
  if kill -0 -- "-${group}" 2>/dev/null; then
    GROUP_CLOSED=0
  else
    GROUP_CLOSED=1
  fi
}

# Tronque un flux au-dela de STREAM_CAP : tete + marqueur + queue ; consigne dans truncated.txt.
truncate_stream() {
  local file="$1" size half
  size="$(stat -c %s "${file}" 2>/dev/null || printf '0')"
  if ((size > STREAM_CAP)); then
    half=$((STREAM_CAP / 2))
    if { head -c "${half}" "${file}"; printf '\n[... v11_worker : %s octets tronques ...]\n' "$((size - 2 * half))"
         tail -c "${half}" "${file}"; } > "${file}.cut" && mv "${file}.cut" "${file}"; then
      TRUNCATED_STREAMS=$((TRUNCATED_STREAMS + 1))
      printf '%s\t%s\n' "${size}" "${file#"${RESULTS}"/}" >> "${RESULTS}/truncated.txt"
      STREAM_TRUNCATED=1
    else
      rm -f "${file}.cut"
      : > "${file}"
      TRUNCATED_STREAMS=$((TRUNCATED_STREAMS + 1))
      printf '%s\t%s\tvide\n' "${size}" "${file#"${RESULTS}"/}" >> "${RESULTS}/truncated.txt"
      STREAM_TRUNCATED=1
    fi
  fi
}

# run_step DIR TIMEOUT CWD ARGV... : etape dans son propre groupe, bornee par l'echeance.
# Renseigne STEP_RC et STEP_STATUS (ok, failed, timeout, deadline_cut, interrupted, skipped_deadline,
# group_not_closed).
run_step() {
  local dir="$1" requested="$2" cwd="$3"
  shift 3
  local remaining effective capped=0 started ended closed_at rc rc2 load_before load_after wall rss time_state
  mkdir -p "${dir}"
  printf '%q ' "$@" > "${dir}/argv.txt"
  printf '\n' >> "${dir}/argv.txt"
  remaining="$(remaining_seconds)"
  if ((INTERRUPTED == 1)); then
    STEP_RC=-1 STEP_STATUS="interrupted"
    printf 'status=%s\nrequested_timeout_seconds=%s\n' "${STEP_STATUS}" "${requested}" > "${dir}/meta.txt"
    return 0
  fi
  if ((remaining < MIN_STEP_SECONDS)); then
    STEP_RC=-1 STEP_STATUS="skipped_deadline"
    printf 'status=%s\nrequested_timeout_seconds=%s\n' "${STEP_STATUS}" "${requested}" > "${dir}/meta.txt"
    return 0
  fi
  effective="${requested}"
  if ((remaining < effective)); then
    effective="${remaining}"
    capped=1
  fi
  load_before="$(cat /proc/loadavg)"
  started="$(date +%s.%N)"
  (cd "${cwd}" && exec setsid -w "${TIME_BIN}" -v -o "${dir}/time.txt" \
      timeout --foreground --kill-after="${KILL_GRACE_SECONDS}s" "${effective}s" "$@") \
    > "${dir}/stdout" 2> "${dir}/stderr" < /dev/null &
  CHILD=$!
  if ((INTERRUPTED == 1)); then
    kill -TERM -- "-${CHILD}" 2>/dev/null || kill -TERM "${CHILD}" 2>/dev/null || true
  fi
  while :; do
    wait "${CHILD}"
    rc=$?
    if kill -0 "${CHILD}" 2>/dev/null; then
      continue
    fi
    wait "${CHILD}" 2>/dev/null
    rc2=$?
    if ((rc2 != 127)); then
      rc=${rc2}
    fi
    break
  done
  ended="$(date +%s.%N)"
  close_group "${CHILD}"
  CHILD=""
  closed_at="$(date +%s.%N)"
  load_after="$(cat /proc/loadavg)"
  wall="$(awk -v a="${started}" -v b="${ended}" 'BEGIN { printf "%.3f", b - a }')"
  rss="$(sed -n 's/^[[:space:]]*Maximum resident set size (kbytes): \([0-9][0-9]*\)$/\1/p' \
    "${dir}/time.txt" 2>/dev/null | head -n 1)"
  if [[ -n "${rss}" ]]; then
    time_state="present"
  else
    time_state="absent"
    rss="absent"
  fi
  STREAM_TRUNCATED=0
  truncate_stream "${dir}/stdout"
  truncate_stream "${dir}/stderr"
  STEP_RC=${rc}
  if ((INTERRUPTED == 1)); then
    STEP_STATUS="interrupted"
  elif ((rc == 0)); then
    STEP_STATUS="ok"
  elif ((rc == 124 || rc == 137)) && awk -v w="${wall}" -v e="${effective}" 'BEGIN { exit !(w >= e) }'; then
    if ((capped == 1)); then STEP_STATUS="deadline_cut"; else STEP_STATUS="timeout"; fi
  else
    STEP_STATUS="failed"
  fi
  if [[ "${STEP_STATUS}" == "ok" && "${GROUP_CLOSED}" != "1" ]]; then
    STEP_STATUS="group_not_closed"
  fi
  {
    printf 'status=%s\n' "${STEP_STATUS}"
    printf 'exit_code=%s\n' "${rc}"
    printf 'requested_timeout_seconds=%s\n' "${requested}"
    printf 'effective_timeout_seconds=%s\n' "${effective}"
    printf 'started_epoch=%s\n' "${started}"
    printf 'ended_epoch=%s\n' "${ended}"
    printf 'group_closed_epoch=%s\n' "${closed_at}"
    printf 'wall_seconds=%s\n' "${wall}"
    printf 'max_rss_kb=%s\n' "${rss}"
    printf 'time_txt=%s\n' "${time_state}"
    printf 'group_closed=%s\n' "${GROUP_CLOSED}"
    printf 'residual_group_killed=%s\n' "${GROUP_RESIDUAL}"
    printf 'streams_truncated=%s\n' "${STREAM_TRUNCATED}"
    printf 'loadavg_before=%s\n' "${load_before}"
    printf 'loadavg_after=%s\n' "${load_after}"
  } > "${dir}/meta.txt"
  note "etape ${dir#"${RESULTS}"/} : ${STEP_STATUS} (code ${rc}, ${wall} s)"
}

capture() {
  local name="$1"
  shift
  timeout 60s "$@" > "${RESULTS}/env/${name}.txt" 2>&1 || true
}

# Remplace les jetons litteraux {src} {build} {data} {out} ; rien d'autre n'est interprete.
substitute() {
  local s="$1"
  local t_src='{src}' t_build='{build}' t_data='{data}' t_out='{out}'
  s="${s//"${t_src}"/"${SRC}"}"
  s="${s//"${t_build}"/"${BUILD}"}"
  s="${s//"${t_data}"/"${DATA}"}"
  s="${s//"${t_out}"/"$2"}"
  printf '%s' "${s}"
}

# Controle Python : versions exactes et API exigees ; imprime les versions reellement importees.
check_python() {
  timeout 120s python3 - <<'PY'
# EHGP_V11_PYTHON_CHECK
import importlib.metadata as metadata
import sys
pins = {'numpy': '2.2.6', 'scipy': '1.15.3', 'scikit-learn': '1.7.2', 'hdbscan': '0.8.44'}
import numpy, scipy, sklearn, hdbscan  # noqa: E401,F401
from sklearn.cluster import HDBSCAN  # noqa: F401
import hdbscan.validity  # noqa: F401
seen = {name: metadata.version(name) for name in pins}
print(','.join('%s=%s' % item for item in sorted(seen.items())))
sys.exit(0 if seen == pins else 1)
PY
}

results_bytes() {
  du -sb "${RESULTS}" 2>/dev/null | cut -f 1
}

# Plafond : les plus GROS fichiers de {out} (>= MIN_EVICT) restent sur la VM (overflow/), jamais les
# petits ; arret si un deplacement echoue ou si la taille ne baisse plus (jamais de boucle infinie).
enforce_results_cap() {
  local total previous size path
  : > "${RESULTS}/overflow.txt"
  total="$(results_bytes)"
  while ((total > RESULTS_CAP)); do
    size="" path=""
    while IFS= read -r -d '' entry; do
      size="${entry%%/*}"
      path="${entry#*/}"
      break
    done < <(find "${RESULTS}/cmd" -path '*/files/*' -type f -size "+$((MIN_EVICT - 1))c" -printf '%s/%P\0' | \
               sort -z -t / -k 1,1nr)
    [[ -n "${path}" ]] || break
    mkdir -p "$(dirname "${WORK}/overflow/${path}")" || break
    printf '%s\t%s\t%q\n' "$(sha256sum "${RESULTS}/cmd/${path}" | cut -d ' ' -f 1)" "${size}" "cmd/${path}" \
      >> "${RESULTS}/overflow.txt"
    mv "${RESULTS}/cmd/${path}" "${WORK}/overflow/${path}" || break
    OVERFLOW_FILES=$((OVERFLOW_FILES + 1))
    previous=${total}
    total="$(results_bytes)"
    ((total < previous)) || break
  done
  if ((total > RESULTS_CAP)); then
    OVERFLOW_UNRESOLVED=1
  fi
  RESULTS_TOTAL=${total}
}

finish() {
  local code status
  cat /proc/loadavg > "${RESULTS}/env/loadavg_end.txt" 2>&1
  RESULTS_TOTAL=0
  enforce_results_cap
  if [[ -z "${FATAL}" && ( "${STATUS_BUILD}" == "ok" || "${STATUS_BUILD}" == "not_requested" ) && \
        "${STATUS_PIP}" != "failed" && ${INTERRUPTED} -eq 0 && ${COMMANDS_OK} -eq ${PLAN_COUNT} ]]; then
    if ((OVERFLOW_FILES == 0 && TRUNCATED_STREAMS == 0 && OVERFLOW_UNRESOLVED == 0)); then
      status="completed"
      code=0
    else
      status="overflow"
      code=1
    fi
  else
    status="failed"
    code=1
  fi
  {
    printf 'schema=%s\n' "${RESULT_SCHEMA}"
    printf 'status=%s\n' "${status}"
    printf 'fatal=%s\n' "${FATAL}"
    printf 'data=%s\n' "${STATUS_DATA}"
    printf 'pip=%s\n' "${STATUS_PIP}"
    printf 'python_versions=%s\n' "${PYTHON_VERSIONS}"
    printf 'python_user_base=%s\n' "${PYTHONUSERBASE:-}"
    printf 'build=%s\n' "${STATUS_BUILD}"
    printf 'commands_total=%s\n' "${PLAN_COUNT}"
    printf 'commands_ok=%s\n' "${COMMANDS_OK}"
    printf 'interrupted=%s\n' "${INTERRUPTED}"
    printf 'results_cap_bytes=%s\n' "${RESULTS_CAP}"
    printf 'results_bytes=%s\n' "${RESULTS_TOTAL}"
    printf 'overflow_files=%s\n' "${OVERFLOW_FILES}"
    printf 'truncated_streams=%s\n' "${TRUNCATED_STREAMS}"
    printf 'overflow_unresolved=%s\n' "${OVERFLOW_UNRESOLVED}"
    printf 'nproc=%s\n' "${NPROC}"
    printf 'source=%s\n' "${SOURCE}"
    printf 'plan_sha256=%s\n' "${PLAN_SHA}"
    printf 'package_sha256=%s\n' "${PACKAGE_SHA}"
    printf 'generation=%s\n' "${GENERATION}"
    printf 'deadline_epoch=%s\n' "${DEADLINE}"
    printf 'started_epoch=%s\n' "${STARTED_EPOCH}"
    printf 'ended_epoch=%s\n' "$(date +%s)"
  } > "${RESULTS}/worker.txt"
  # Le manifeste partiel s'ecrit HORS de results/ ; seul ./MANIFEST.sha256 (racine) est exclu.
  if ! (cd "${RESULTS}" && find . -type f ! -path ./MANIFEST.sha256 -print0 | sort -z | \
          xargs -0 -r sha256sum) > "${WORK}/MANIFEST.sha256.partial" || \
     ! mv "${WORK}/MANIFEST.sha256.partial" "${RESULTS}/MANIFEST.sha256" || \
     ! tar -czf "${WORK}/results.tar.gz.partial" -C "${WORK}" results || \
     ! mv "${WORK}/results.tar.gz.partial" "${WORK}/results.tar.gz" || \
     ! (cd "${WORK}" && sha256sum results.tar.gz > SHA256SUMS.partial && mv SHA256SUMS.partial SHA256SUMS); then
    code=3
  fi
  publish_exit "${code}"
  exit "${code}"
}

note "worker demarre (pid $$), echeance ${DEADLINE}, generation ${GENERATION}"

# 1. Environnement mesure (jamais declare) ; PATH systeme inchange, chemins des outils releves.
capture nproc nproc
capture lscpu lscpu
capture loadavg_start cat /proc/loadavg
capture uptime_start uptime
capture meminfo cat /proc/meminfo
capture free free -b
capture df df -Pk "${WORK}"
capture uname uname -a
capture os_release cat /etc/os-release
capture user id
capture gxx g++ --version
capture clangxx clang++ --version
capture cmake cmake --version
capture python3 python3 --version
capture pip python3 -m pip --version
capture time_version "${TIME_BIN}" --version
capture tool_paths bash -c 'for t in cmake ctest g++ c++ python3 tar; do printf "%s=%s\n" "$t" "$(command -v "$t")"; done'
NPROC="$(nproc 2>/dev/null || printf '1')"
[[ "${NPROC}" =~ ^[1-9][0-9]*$ ]] || NPROC=1

# Faits de la VM : env/vm_facts.txt, une ligne cle=valeur par fait. Ce sont des RELEVES (le controleur
# les recopie dans le recu), jamais des conditions de conformite.
# first_line OUTIL ARGS... : premiere ligne de la sortie, ou « absent » (outil manquant, echec, sortie vide).
first_line() {
  local text
  if text="$(timeout 60s "$@" 2>/dev/null)" && [[ -n "${text}" ]]; then
    printf '%s' "${text%%$'\n'*}"
  else
    printf 'absent'
  fi
}

# sanitizer_probe NOM DRAPEAUX : compile (g++) puis execute un programme d'une ligne avec
# -fsanitize=DRAPEAUX, dans $WORK/probe ; sorties dans env/sanitizer_NOM.txt. Imprime :
#   ok             cinq executions natives reussies (sans fichier core : ulimit -c 0) ;
#   ok_setarch     une execution native a echoue, l'execution sous `setarch -R` reussit : avec une forte
#                  randomisation de l'espace d'adressage, un binaire ThreadSanitizer echoue ALEATOIREMENT au
#                  demarrage (mesure dans le codespace : 6 reussites sur 40 en natif, 40 sur 40 sous setarch) ;
#   run_failed, compile_failed, no_compiler.
# Delais : 60 s pour la compilation, 20 s par execution.
sanitizer_probe() {
  local name="$1" flags="$2" dir="${WORK}/probe" log="${RESULTS}/env/sanitizer_$1.txt" attempt
  if ! command -v g++ > /dev/null 2>&1; then
    printf 'no_compiler'
    return 0
  fi
  mkdir -p "${dir}"
  printf 'int main() { return 0; }\n' > "${dir}/${name}.cpp"
  if ! timeout 60s g++ -std=c++20 -O1 "-fsanitize=${flags}" -o "${dir}/${name}" "${dir}/${name}.cpp" \
       > "${log}" 2>&1 < /dev/null; then
    printf 'compile_failed'
    return 0
  fi
  for attempt in 1 2 3 4 5; do
    if ! (ulimit -c 0; exec timeout 20s "${dir}/${name}") >> "${log}" 2>&1 < /dev/null; then
      if timeout 20s setarch "$(uname -m)" -R "${dir}/${name}" >> "${log}" 2>&1 < /dev/null; then
        printf 'ok_setarch'
      else
        printf 'run_failed'
      fi
      return 0
    fi
  done
  printf 'ok'
}

# Drapeaux du processeur : premiere ligne « flags » de /proc/cpuinfo, entouree d'espaces (mot entier).
CPU_FLAGS=" $(sed -n '/^flags[[:space:]]*:/{s/^flags[[:space:]]*:[[:space:]]*//p;q}' /proc/cpuinfo 2>/dev/null) "
{
  printf 'gxx=%s\n' "$(first_line g++ --version)"
  printf 'clangxx=%s\n' "$(first_line clang++ --version)"
  printf 'cmake=%s\n' "$(first_line cmake --version)"
  printf 'ctest=%s\n' "$(first_line ctest --version)"
  printf 'python3=%s\n' "$(first_line python3 --version)"
  printf 'nproc=%s\n' "${NPROC}"
  printf 'mem_total_kb=%s\n' "$(sed -n 's/^MemTotal:[[:space:]]*\([0-9][0-9]*\) kB$/\1/p' /proc/meminfo 2>/dev/null)"
  printf 'sanitizer_address_undefined=%s\n' "$(sanitizer_probe address_undefined address,undefined)"
  printf 'sanitizer_thread=%s\n' "$(sanitizer_probe thread thread)"
  for flag in avx2 avx512f avx512dq avx512vl bmi2; do
    if [[ "${CPU_FLAGS}" == *" ${flag} "* ]]; then
      printf 'cpu_%s=1\n' "${flag}"
    else
      printf 'cpu_%s=0\n' "${flag}"
    fi
  done
} > "${RESULTS}/env/vm_facts.txt"
rm -rf "${WORK}/probe"   # rien ne reste sur le disque persistant de la VM

# 2. Entrees : paquet et plan epingles, GNU time, echeance.
if [[ ! -f "${SRC}/morsehgp3D_v11/CMakeLists.txt" ]]; then
  FATAL="paquet sans morsehgp3D_v11/CMakeLists.txt"
elif [[ "$(sha256sum "${WORK}/package.tar.gz" 2>/dev/null | cut -d ' ' -f 1)" != "${PACKAGE_SHA}" ]]; then
  FATAL="sha256 du paquet different de l'epingle"
elif [[ ! -f "${PLAN}" || "$(sha256sum "${PLAN}" | cut -d ' ' -f 1)" != "${PLAN_SHA}" || \
        "$(head -n 1 "${PLAN}")" != "${PLAN_MAGIC}" ]]; then
  FATAL="plan rendu absent, d'un autre schema ou different de l'epingle"
elif [[ ! -x "${TIME_BIN}" ]]; then
  FATAL="GNU time absent (${TIME_BIN}) ; aucune installation n'est tentee"
elif (($(remaining_seconds) < MIN_STEP_SECONDS)); then
  FATAL="echeance deja atteinte au demarrage du worker"
fi
PLAN_PYTHON_PINNED="" PLAN_DEFAULT_BUILD="" PLAN_BUILD_TIMEOUT="" PLAN_RESULTS_CAP=""
PLAN_BUILD_TARGETS=()
if [[ -z "${FATAL}" ]]; then
  # shellcheck disable=SC1090
  source "${PLAN}"
  cp "${PLAN}" "${RESULTS}/plan.sh"
  if ! [[ "${PLAN_COUNT}" =~ ^[0-9]{1,3}$ && "${PLAN_BUILD_TIMEOUT}" =~ ^[1-9][0-9]*$ && \
          "${PLAN_PYTHON_PINNED}" =~ ^[01]$ && "${PLAN_DEFAULT_BUILD}" =~ ^[01]$ && \
          "${PLAN_RESULTS_CAP}" =~ ^[1-9][0-9]{6,9}$ ]] || \
     ((PLAN_RESULTS_CAP > MAX_RESULTS_BYTES)); then
    FATAL="plan rendu incoherent"
  else
    for ((i = 0; i < PLAN_COUNT; i++)); do
      name_var="PLAN_NAME_${i}" timeout_var="PLAN_TIMEOUT_${i}"
      if ! [[ "${!name_var-}" =~ ^[a-z0-9][a-z0-9_.-]{0,63}$ && "${!timeout_var-}" =~ ^[1-9][0-9]*$ ]] || \
         ! declare -p "PLAN_ARGV_${i}" > /dev/null 2>&1; then
        FATAL="entree ${i} du plan rendu incoherente"
        break
      fi
    done
  fi
  if [[ -z "${FATAL}" ]]; then
    RESULTS_CAP=${PLAN_RESULTS_CAP}
    STREAM_CAP=$((RESULTS_CAP / 4 < STREAM_CAP_MAX ? RESULTS_CAP / 4 : STREAM_CAP_MAX))
    MIN_EVICT=$((RESULTS_CAP / 16 < 1048576 ? RESULTS_CAP / 16 : 1048576))
  else
    PLAN_COUNT=0
  fi
fi
[[ -z "${FATAL}" ]] || { note "refus : ${FATAL}"; finish; }

# 3. Donnees : seconde verification des sha256, ici, apres le transfert.
if [[ ! -f "${DATA}/SHA256SUMS" ]]; then
  FATAL="SHA256SUMS absent des donnees"
elif [[ ! -s "${DATA}/SHA256SUMS" ]]; then
  if (($(find "${DATA}" -mindepth 1 -maxdepth 1 | wc -l) == 1)); then
    STATUS_DATA="empty_ok"
  else
    FATAL="fichiers de donnees non declares"
  fi
else
  if (cd "${DATA}" && sha256sum -c --strict SHA256SUMS) > "${RESULTS}/data_check.txt" 2>&1 && \
     (($(find "${DATA}" -mindepth 1 -maxdepth 1 | wc -l) == $(wc -l < "${DATA}/SHA256SUMS") + 1)); then
    STATUS_DATA="ok"
  else
    FATAL="verification sha256 des donnees en echec"
  fi
fi
[[ -z "${FATAL}" ]] || { STATUS_DATA="failed"; note "refus : ${FATAL}"; finish; }

export V11_SRC="${SRC}" V11_BUILD="${BUILD}" V11_DATA="${DATA}"
export V11_SOURCE_PIN="${SOURCE}" V11_PACKAGE_SHA256="${PACKAGE_SHA}" V11_GENERATION="${GENERATION}"
# Dossier des donnees de la session ({data}) : les portes « lidar » de la v11 y cherchent leurs trames.
export MHGP11_DATA_DIR="${DATA}"
export PYTHONPATH="${SRC}/morsehgp3D_v11/python${PYTHONPATH:+:${PYTHONPATH}}"
export JOBLIB_TEMP_FOLDER="${WORK}/joblib"
mkdir -p "${JOBLIB_TEMP_FOLDER}"
# Paquets Python PROPRES A LA SESSION : pip --user installe sous PYTHONUSERBASE et Python n'ajoute
# alors a sys.path que ce site utilisateur ; ~/.local, qu'une installation interrompue d'une session
# precedente a pu laisser incoherent sur le disque persistant, n'est plus jamais lu.
export PYTHONUSERBASE="${WORK}/pyuser"
mkdir -p "${PYTHONUSERBASE}"

# 4. Python epingle, SEULEMENT si le plan le demande (python_packages = pinned) : paquets utilisateur de la
# session, roues seulement. Par defaut (none) aucun controle pip : la v10 rendait failed_remote des que pip
# manquait sur la VM, meme quand toutes les commandes avaient reussi avec le Python nu.
if ((PLAN_PYTHON_PINNED == 1)); then
  if PYTHON_VERSIONS="$(check_python 2> "${RESULTS}/setup/check_before.stderr")"; then
    STATUS_PIP="already_present"
  elif ! timeout 60s python3 -m pip --version > "${RESULTS}/setup/pip_version.txt" 2>&1; then
    STATUS_PIP="failed"
    note "python3 -m pip absent : aucune installation systeme n'est tentee"
  else
    run_step "${RESULTS}/setup/pip" "${PIP_TIMEOUT_SECONDS}" "${WORK}" \
      python3 -m pip install --user --quiet --disable-pip-version-check --no-cache-dir --only-binary=:all: \
      "${PIP_PINS[@]}"
    if [[ "${STEP_STATUS}" == "ok" ]] && PYTHON_VERSIONS="$(check_python 2> "${RESULTS}/setup/check_after.stderr")"; then
      STATUS_PIP="installed"
    else
      STATUS_PIP="failed"
      note "pip en echec : les commandes Python echoueront, les autres s'executent"
    fi
  fi
  printf '%s\n' "${PYTHON_VERSIONS}" > "${RESULTS}/env/python_packages.txt"
  capture pip_freeze python3 -m pip freeze --all
fi

# 5. Construction Release (aucun drapeau ajoute : -Werror du CMake v11 reste en vigueur), sauf si le plan
# dit default_build = false : {build} est alors un dossier vide et les commandes construisent elles-memes.
if ((PLAN_DEFAULT_BUILD == 1)); then
  run_step "${RESULTS}/build/configure" "${PLAN_BUILD_TIMEOUT}" "${WORK}" \
    cmake -S "${SRC}/morsehgp3D_v11" -B "${BUILD}" -DCMAKE_BUILD_TYPE=Release
  if [[ "${STEP_STATUS}" == "ok" ]]; then
    build_argv=(cmake --build "${BUILD}" -j "${NPROC}")
    if ((${#PLAN_BUILD_TARGETS[@]} > 0)); then
      build_argv+=(--target "${PLAN_BUILD_TARGETS[@]}")
    fi
    run_step "${RESULTS}/build/build" "${PLAN_BUILD_TIMEOUT}" "${WORK}" "${build_argv[@]}"
  fi
  if [[ "${STEP_STATUS}" == "ok" ]]; then
    STATUS_BUILD="ok"
  else
    STATUS_BUILD="failed"
  fi
  (cd "${BUILD}" 2>/dev/null && find . -maxdepth 1 -type f -name 'mhgp11*' -perm -u+x -print0 | sort -z | \
     xargs -0 -r sha256sum) > "${RESULTS}/provenance/binaries.sha256" 2>&1 || true
  grep -E '^CMAKE_(CXX_COMPILER|BUILD_TYPE|CXX_FLAGS)' "${BUILD}/CMakeCache.txt" \
    > "${RESULTS}/provenance/cmakecache.txt" 2>&1 || true
  grep -hE 'set\(CMAKE_CXX_COMPILER(_ID|_VERSION)? ' "${BUILD}"/CMakeFiles/*/CMakeCXXCompiler.cmake \
    > "${RESULTS}/provenance/compiler.txt" 2>&1 || true
elif mkdir -p "${BUILD}"; then
  STATUS_BUILD="not_requested"
  note "construction par defaut non demandee (default_build = false) : {build} est vide"
else
  STATUS_BUILD="failed"
  note "dossier {build} impossible a creer"
fi

# 6. Commandes du plan, dans l'ordre, cwd = dossier de build.
printf 'index\tname\tstatus\texit_code\twall_seconds\ttimeout_seconds\tmax_rss_kb\n' > "${RESULTS}/commands.tsv"
for ((i = 0; i < PLAN_COUNT; i++)); do
  eval "name=\${PLAN_NAME_${i}}; timeout_s=\${PLAN_TIMEOUT_${i}}; argv=(\"\${PLAN_ARGV_${i}[@]}\")"
  dir="${RESULTS}/cmd/$(printf '%03d' "${i}")_${name}"
  out="${dir}/files"
  mkdir -p "${out}"
  if [[ "${STATUS_BUILD}" != "ok" && "${STATUS_BUILD}" != "not_requested" ]]; then
    STEP_RC=-1
    STEP_STATUS="skipped_build"
    printf 'status=%s\n' "${STEP_STATUS}" > "${dir}/meta.txt"
  else
    args=()
    for a in "${argv[@]}"; do
      args+=("$(substitute "${a}" "${out}")")
    done
    export V11_OUT="${out}"
    run_step "${dir}" "${timeout_s}" "${BUILD}" "${args[@]}"
    # ctest rend 0 sans aucun test, ou avec des tests desactives/sautes : « ok » exige au moins un test
    # passe et aucun test non execute.
    if [[ "${argv[0]}" == "ctest" && "${STEP_STATUS}" == "ok" ]] && \
       { ! grep -Eq '(^|[[:space:]])100% tests passed, 0 tests failed out of [1-9][0-9]*' "${dir}/stdout" || \
         grep -q 'The following tests did not run' "${dir}/stdout"; }; then
      STEP_STATUS="vacuous"
      sed -i 's/^status=ok$/status=vacuous/' "${dir}/meta.txt"
      note "etape ${dir#"${RESULTS}"/} : reclassee vacuous (aucun test passe, ou tests desactives/sautes)"
    fi
  fi
  if [[ "${STEP_STATUS}" == "ok" ]]; then
    COMMANDS_OK=$((COMMANDS_OK + 1))
  fi
  wall="$(sed -n 's/^wall_seconds=//p' "${dir}/meta.txt")"
  rss="$(sed -n 's/^max_rss_kb=//p' "${dir}/meta.txt")"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "${i}" "${name}" "${STEP_STATUS}" "${STEP_RC}" \
    "${wall}" "${timeout_s}" "${rss}" >> "${RESULTS}/commands.tsv"
done

finish
