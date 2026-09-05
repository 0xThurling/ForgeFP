return {
  project = {
    name = "ForgeFP",
    type = "library",
    standard = "20",
    install_headers = true
  },
  testing = false,
  dependencies = {
   direct = {},
   conan = {}
  },
  resources = {
    files = {}
  },
  scripts = {
    ["bench"] = "bash scripts/run_bench.sh",
    ["bench-pinned"] = "bash scripts/run_bench.sh 2"
  },
  features = {}
}
