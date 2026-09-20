"""Minimal MCP stdio client: launch a built target and speak MCP to it.

Plain JSON-RPC lines over subprocess pipes, stdlib only. Lets agents
dynamically discover and exercise example MCP servers without registering
them in opencode.json. Handles both bare JSON lines and SSE `data:` frames.
"""

from __future__ import annotations

import json

from . import repo as repo_paths
from .config import ROOT


def _parse_transcript(output: str) -> dict:
    """Collect JSON-RPC responses by id from a captured transcript."""
    found: dict = {}
    for line in output.splitlines():
        message = _parse(line)
        if message is None:
            continue
        if "id" in message and ("result" in message or "error" in message):
            found[message["id"]] = message
    return found


def _parse(line: str | None):
    """Parse one transport line into a JSON-RPC object, skipping noise."""
    if not line:
        return None
    text = line.strip()
    if text.startswith("data:"):
        text = text[len("data:"):].strip()
    if not text.startswith("{"):
        return None
    try:
        message = json.loads(text)
    except json.JSONDecodeError:
        return None
    return message if isinstance(message, dict) else None


def _require(results: dict, request_id: int, output: str) -> dict:
    """Fetch one response or raise describing the transcript state."""
    message = results.get(request_id)
    if message is None:
        tail = output.strip().splitlines()[-3:] if output.strip() else ["(no output)"]
        raise TimeoutError(
            f"No response to request {request_id}. Last output:\n" + "\n".join(tail)
        )
    if "error" in message:
        raise ValueError(f"MCP error: {message['error']}")
    return message.get("result", {})


def _tool_summary(tool: dict) -> str:
    """Render one tool line: name, description, required parameters."""
    schema = tool.get("inputSchema", {})
    required = schema.get("required", []) if isinstance(schema, dict) else []
    suffix = f" (required: {', '.join(required)})" if required else ""
    return f"- {tool.get('name')}: {tool.get('description', '')}{suffix}".rstrip()


def probe(
    program: Path,
    tool_name: str | None = None,
    tool_arguments: dict | None = None,
    timeout: int = 60,
) -> str:
    """Handshake a target over stdio, list its tools, optionally call one.

    Half-duplex batching: all requests are written up front, stdin is closed,
    then the full transcript is parsed. Mirrors `echo ... | program`, which
    request/response servers (like our examples) answer before exiting on EOF.
    """
    from .process import capture  # noqa: PLC0415 - reuse the proven runner

    label = repo_paths.relative_posix(program)
    script = [
        {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "protocolVersion": "2025-11-25",
                "capabilities": {},
                "clientInfo": {"name": "libmcp-probe", "version": "1"},
            },
        },
        {"jsonrpc": "2.0", "method": "notifications/initialized", "params": {}},
        {"jsonrpc": "2.0", "id": 2, "method": "tools/list", "params": {}},
    ]
    if tool_name:
        script.append(
            {
                "jsonrpc": "2.0",
                "id": 3,
                "method": "tools/call",
                "params": {"name": tool_name, "arguments": tool_arguments or {}},
            }
        )
    blob = "".join(json.dumps(message) + "\n" for message in script)
    status, output = capture(
        [str(program)], timeout=timeout, workdir=ROOT, stdin_text=blob
    )
    if status == "launch_error":
        return f"target: {label}\nexit_code: launch_error\n{output}"
    if status == "timeout":
        partial = _parse_transcript(output)
        state = f"No response in {timeout} seconds."
        if 1 in partial:
            state = "Handshake answered but the run did not finish; "
            state += "the target likely ignores EOF and serves forever."
        return f"target: {label}\nexit_code: timeout\n{state}\nPartial:\n{output[-1500:]}".rstrip()
    results = _parse_transcript(output)
    try:
        init = _require(results, 1, output)
        listing = _require(results, 2, output)
    except (TimeoutError, ValueError) as error:
        return f"target: {label}\nexit_code: probe_error\n{error}"

    info = listing.get("serverInfo", init.get("serverInfo", {}))
    tools = listing.get("tools", [])
    out = [
        f"target: {label}",
        f"server: {info.get('name', '?')} {info.get('version', '')}".rstrip(),
        f"protocol: {init.get('protocolVersion', '?')}",
        "tools:",
        *[_tool_summary(tool) for tool in tools if isinstance(tool, dict)],
    ]
    if tool_name:
        names = {str(tool.get("name")) for tool in tools if isinstance(tool, dict)}
        if tool_name not in names:
            return "\n".join(out + [f"exit_code: unknown_tool\nAvailable: {sorted(names)}".rstrip()])
        try:
            called = _require(results, 3, output)
        except (TimeoutError, ValueError) as error:
            return "\n".join(out + [f"exit_code: call_error\n{error}"])
        out.append(f"called {tool_name}({json.dumps(tool_arguments or {})}):")
        out.append(_render_result(called))
    return "\n".join(out + [f"exit_code: {status}"]).rstrip()


def _render_result(result: dict) -> str:
    """Render a tools/call result payload as compact text."""
    content = result.get("content", [])
    parts = []
    for block in content:
        if isinstance(block, dict) and block.get("type") == "text":
            parts.append(str(block.get("text", "")))
    if parts:
        return "\n".join(parts)
    structured = result.get("structuredContent")
    if structured is not None:
        return json.dumps(structured)[:2000]
    return json.dumps(result)[:2000]
