#!/usr/bin/env bash
# MCP stdio smoke: connect -> list -> call -> assert against example_parse_schema.
# Fails the job on the first assertion that does not hold.
set -euo pipefail

: "${INSPECTOR_VERSION:=2.7.0}"

BIN="$(find build -type f -name 'example_parse_schema*' -print 2>/dev/null | sort | head -n 1)"
if [[ -z "${BIN}" ]]; then
  echo "::error::example_parse_schema binary not found under build/"
  exit 1
fi
if [[ ! -x "${BIN}" ]]; then
  chmod +x "${BIN}"
fi
echo "Probing ${BIN}"

OUT_DIR="mcp-inspector-results"
mkdir -p "${OUT_DIR}"

mcp() {
  # shellcheck disable=SC2086
  npx --yes "@modelcontextprotocol/inspector@${INSPECTOR_VERSION}" --cli "${BIN}" \
    --connect-timeout 10000 --format json "$@"
}

# 1. Handshake: server alive and speaking MCP.
mcp --method initialize | tee "${OUT_DIR}/initialize.json" \
  | jq -e '.result.protocolVersion and .result.serverInfo and .result.capabilities' > /dev/null
echo "ok: initialize"

# 2. Tool surface: the impl registers get_weather.
mcp --method tools/list | tee "${OUT_DIR}/tools-list.json" \
  | jq -e --argjson want '["get_weather"]' \
      '[.result.tools[].name] as $have | $want - $have | length == 0' > /dev/null
echo "ok: tools/list contains get_weather"

# 3. Compliance level: tool-schema portability. Errors fail, warnings do not.
#    Exit 6 means an error-severity finding; stdout carries schemaFindings.
mcp --method tools/list --strict | tee "${OUT_DIR}/tools-list-strict.json" \
  | jq -e '[.schemaFindings[]?.findings[]? | select(.severity == "error")] | length == 0' > /dev/null
echo "ok: tools/list --strict (no error-severity findings)"

# 4. Representative call: safe, idempotent stub tool.
mcp --method tools/call --tool-name get_weather \
  --tool-args-json '{"location":"Minsk"}' | tee "${OUT_DIR}/tools-call.json" \
  | jq -e '.result.isError != true
      and ([.result.content[]? | select(.type == "text") | .text] | length > 0)' > /dev/null
echo "ok: tools/call get_weather"

# 5. Negative assertion: unknown tool must fail, not silently pass.
status=0
mcp --method tools/call --tool-name does_not_exist \
  --tool-args-json '{}' > "${OUT_DIR}/tools-call-unknown.json" 2> "${OUT_DIR}/tools-call-unknown.stderr" || status=$?
if [[ "${status}" -eq 0 ]]; then
  echo "::error::tools/call accepted an unknown tool"
  exit 1
fi
echo "ok: tools/call unknown rejected (exit ${status})"

# 6. Full surface: every list method the impl routes must answer.
mcp --method resources/list | tee "${OUT_DIR}/resources-list.json" \
  | jq -e '.result.resources | type == "array"' > /dev/null
echo "ok: resources/list"

mcp --method resources/templates/list | tee "${OUT_DIR}/resources-templates-list.json" \
  | jq -e '.result.resourceTemplates | type == "array"' > /dev/null
echo "ok: resources/templates/list"

mcp --method prompts/list | tee "${OUT_DIR}/prompts-list.json" \
  | jq -e '.result.prompts | type == "array"' > /dev/null
echo "ok: prompts/list"

mcp --method ping | tee "${OUT_DIR}/ping.json" \
  | jq -e '.result != null' > /dev/null
echo "ok: ping"

{
  echo "## MCP stdio smoke"
  echo ""
  echo "- Binary: \`${BIN}\`"
  echo "- Inspector: \`@modelcontextprotocol/inspector@${INSPECTOR_VERSION}\`"
  echo "- initialize, tools/list (+ --strict), tools/call, resources/list,"
  echo "  resources/templates/list, prompts/list, ping: all green."
} >> "${GITHUB_STEP_SUMMARY:-/dev/null}"

echo "smoke OK"
