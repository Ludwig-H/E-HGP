#!/usr/bin/env python3
"""Read-only archive judge: never imports or executes the historical capture."""
import hashlib
import json
import math
from pathlib import Path
import shlex
import sys

HERE = Path(__file__).resolve().parent
ORIGINAL_MANIFEST_SHA = "0d3be870027869fa879ffe309500a6d95386870467b64e80c33e2618ec707197"
SRC = "/workspaces/E-HGP/build/v10-integration-r2/src/morsehgp3D_v10"
TMP = "/tmp/mhgp10-flags-quote-review-20260930.yrK6iG2g"
EXPECTED_ORIGINAL = {
 "sources/CMakeLists.txt":"9642137f4bcf4483d80eb178dc684456388c2d7eb51ca33edce455d2f961b423",
 "sources/site_tree.cpp":"a3af773f252d2c4c0789945617b9e727bda856e2fd2e03f5e493a940e25205f9",
 "sources/tower.cpp":"b13d4d8e0f2640eb7e5596e5fb714d346f86a0abf37a4b870d2ce8d2f341aa8f",
 "sources/fp_flags_configure.cmake":"9020ab19ade0c12231fef41ba382e12032b2b4b271d7428a3e54c6d7808ebe71",
 "capture.py":"05d83e0a68b38e8ec80c51ae1a6ef6863976a2311c4ae4bc1eaf1308cbf6df8b",
 "execution.json":"d06a855da381341963e79a4f378a2593c90457664853fad8913fb73aa28db05c",
 "README.md":"2c1b9b74e91087f6c562f8590a112497fd55430c7adf4781a71c6649a530a328",
 "review_source_pins.txt":"1ea0b786cd1b1a0fd1f3f4078d22ce6396495d05cfb7be60d880504177e2e4f8",
}
EMPTY = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
STREAM_SHA = [
 ("f648cc81e42239b25b99ababf47eacf8075095245edd382fa9d94f22c6b0b03b",
  "6e0f14159e5c0a388d41d42476222dbb7cfe57d5bb390f65ac4775e3200d798c"),
 ("505ff559ba411cb9172ab7d6f8497e01c85a34ca00038d1220d186b7d670d773", EMPTY),
 ("6e335ed04380c56b3b598b01926bbd49d83767a1ded8859fbc88c81ada8f4abf", EMPTY),
 (EMPTY, EMPTY),
]
ROOT_NAMES = {"README.md", "verify.py"} | {"original/"+n for n in EXPECTED_ORIGINAL} | {"original/SHA256SUMS"}

def require(ok, message):
 if not ok:
  raise ValueError(message)

def digest(data):
 return hashlib.sha256(data).hexdigest()

def manifest(data):
 result = {}
 for line in data.decode("ascii").splitlines():
  require(len(line) > 66 and line[64:66] == "  ", "manifest format")
  sha, name = line[:64], line[66:]
  require(len(sha) == 64 and all(c in "0123456789abcdef" for c in sha), "manifest digest")
  require(name not in result and name and not name.startswith("/") and ".." not in Path(name).parts,
          "manifest name")
  result[name] = sha
 return result

def no_duplicates(pairs):
 result = {}
 for k, v in pairs:
  require(k not in result, "duplicate JSON key")
  result[k] = v
 return result

def main():
 require(len(sys.argv) == 1, "usage: python3 [-O] verify.py")
 # Inventory and every byte hash BEFORE parsing any archived metadata, let alone executing code.
 all_paths = list(HERE.rglob("*"))
 require(not any(p.is_symlink() for p in all_paths), "symlink")
 found = {p.relative_to(HERE).as_posix() for p in all_paths if p.is_file()}
 require(found == ROOT_NAMES | {"SHA256SUMS"}, "exact archive inventory")
 root_manifest = manifest((HERE/"SHA256SUMS").read_bytes())
 require(set(root_manifest) == ROOT_NAMES, "exact root manifest inventory")
 blobs = {n:(HERE/n).read_bytes() for n in ROOT_NAMES}
 for n, sha in root_manifest.items():
  require(digest(blobs[n]) == sha, "root hash: "+n)
 require(digest(blobs["original/SHA256SUMS"]) == ORIGINAL_MANIFEST_SHA, "original manifest pin")
 require(manifest(blobs["original/SHA256SUMS"]) == EXPECTED_ORIGINAL, "exact original manifest")
 for n, sha in EXPECTED_ORIGINAL.items():
  require(digest(blobs["original/"+n]) == sha, "original file pin: "+n)
 j = json.loads(blobs["original/execution.json"], object_pairs_hook=no_duplicates)
 require(set(j) == {"schema","scope","calls","cache_lines","site_compile_command","site_command_tokens",
                    "selected_math_macros","source_before","source_after","source_stable","EXPECTED_GAP"},
         "exact execution fields")
 require(j["schema"] == "mhgp10_quote_configure_micro_v1", "schema")
 require(j["scope"] == "One quoted flag fixture and its unquoted control. Two configurations and two preprocessor calls; no engine object compiled or native geometric execution.", "scope")
 base = ["cmake","-S",SRC,"-DCMAKE_CXX_COMPILER=/usr/bin/clang++","-DBUILD_TESTING=OFF"]
 expected = [
  ("unquoted_refused", base+["-B",TMP+"/control","-DCMAKE_CXX_FLAGS=-freciprocal-math"], 1),
  ("quoted_configure", base+["-B",TMP+"/quoted",'-DCMAKE_CXX_FLAGS="-freciprocal-math"'], 0),
  ("clang_reciprocal_macros", ["/usr/bin/clang++","-freciprocal-math","-dM","-E","-x","c++","/dev/null"], 0),
  ("site_tree_preprocessor", ["/usr/bin/clang++","-freciprocal-math","-std=c++20","-I"+SRC+"/src","-E","-o","/dev/null",SRC+"/src/cloud/site_tree.cpp"], 0),
 ]
 require(type(j["calls"]) is list and len(j["calls"]) == 4, "four calls")
 for i, (call, (name, argv, rc)) in enumerate(zip(j["calls"], expected)):
  require(type(call) is dict and set(call) == {"name","argv","returncode","seconds","stdout","stderr"},
          "exact call fields")
  require(call["name"] == name and call["argv"] == argv, "command binding")
  require(type(call["returncode"]) is int and call["returncode"] == rc, "return code")
  require(type(call["seconds"]) is float and math.isfinite(call["seconds"]) and 0 < call["seconds"] < 10,
          "duration metadata")
  for stream, sha in zip(("stdout","stderr"), STREAM_SHA[i]):
   require(type(call[stream]) is str and digest(call[stream].encode("utf-8")) == sha, "exact stream pin")
 require("mhgp10_fp_flags_interdits" in j["calls"][0]["stderr"], "unquoted refusal token")
 require(j["cache_lines"] == ["CMAKE_CXX_COMPILER:STRING=/usr/bin/clang++",
                             'CMAKE_CXX_FLAGS:STRING="-freciprocal-math"'], "actual quoted cache")
 command = "/usr/bin/clang++  -I"+SRC+'/src "-freciprocal-math" -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -ffp-contract=off -o CMakeFiles/mhgp10_core.dir/src/cloud/site_tree.cpp.o -c '+SRC+"/src/cloud/site_tree.cpp"
 require(j["site_compile_command"] == command, "exact compiler command")
 tokens = shlex.split(command)
 require(j["site_command_tokens"] == tokens and tokens.count("-freciprocal-math") == 1,
         "compiler consumes flag token")
 macros = j["calls"][2]["stdout"].splitlines()
 selected = [s for s in macros if any(t in s for t in
             ("FAST_MATH","ASSOCIATIVE_MATH","RECIPROCAL_MATH","FINITE_MATH","FLT_EVAL"))]
 require(j["selected_math_macros"] == selected == ["#define __FINITE_MATH_ONLY__ 0"], "macro selection")
 names = {line.split()[1] for line in macros if line.startswith("#define ")}
 require(not names & {"__FAST_MATH__","__ASSOCIATIVE_MATH__","__RECIPROCAL_MATH__"}, "guard macros absent")
 require('#define __clang_version__ "18.1.3 (1ubuntu1)"' in macros, "Clang version")
 source_map = {SRC+"/CMakeLists.txt":"sources/CMakeLists.txt",
               SRC+"/src/cloud/site_tree.cpp":"sources/site_tree.cpp",
               SRC+"/src/tower/tower.cpp":"sources/tower.cpp",
               SRC+"/tests/regression/fp_flags_configure.cmake":"sources/fp_flags_configure.cmake"}
 pinned_sources = {path:EXPECTED_ORIGINAL[n] for path, n in source_map.items()}
 require(j["source_before"] == j["source_after"] == pinned_sources, "exact four source pins")
 require(type(j["source_stable"]) is bool and j["source_stable"] is True, "source stability")
 require(type(j["EXPECTED_GAP"]) is bool and j["EXPECTED_GAP"] is True, "recorded result")
 cmake = blobs["original/sources/CMakeLists.txt"].decode()
 require('if(" ${${var}} " MATCHES "[ \\t](${mhgp10_fp_flags_forbidden})[ \\t]")' in cmake,
         "historical raw-token guard")
 for n in ("site_tree.cpp","tower.cpp"):
  text = blobs["original/sources/"+n].decode()
  require("#if defined(__FAST_MATH__) || defined(__ASSOCIATIVE_MATH__) || defined(__RECIPROCAL_MATH__)" in text,
          "historical source guards")
 # No subprocess, import of original/capture.py, external source or build is accessed.
 for n, sha in root_manifest.items():
  require(digest((HERE/n).read_bytes()) == sha, "hash after read: "+n)
 print("ARCHIVE_PASS historic_calls=4 configures=2 preprocessors=2 fresh_subprocesses=0 native_engine_executions=0")

if __name__ == "__main__":
 try:
  main()
 except (OSError, ValueError, TypeError, KeyError, json.JSONDecodeError) as exc:
  print("ARCHIVE_REFUSED: "+str(exc), file=sys.stderr)
  raise SystemExit(1)
