# GoldenCheetah MCP Server

This package exposes GoldenCheetah's local AI workout endpoints as MCP tools.
It is designed to run on the same machine as GoldenCheetah and only talks to
the loopback HTTP API at `127.0.0.1` by default.

## What it exposes

- `gc_status`
- `gc_list_athletes`
- `gc_get_athlete_snapshot`
- `gc_create_workout_draft`
- `gc_save_workout`
- `gc_create_planned_activity`

The write path is intentionally split:

1. Generate a draft with `gc_create_workout_draft`
2. Review it in the client
3. Persist it with `gc_save_workout(confirm=true, ...)`
4. Optionally schedule it with `gc_create_planned_activity(confirm=true, ...)`

The write tools reject calls unless `confirm=true`.

## GoldenCheetah prerequisites

GoldenCheetah must have its local API enabled.

- In the GUI: enable `Enable API Web Services`
- Default local API address: `http://127.0.0.1:12021`
- Current AI endpoints require the athlete profile to already be open in GoldenCheetah

The default HTTP server config is in
`src/Resources/webservice/httpserver.ini` and binds to `127.0.0.1`.

## Install

```bash
cd /path/to/GoldenCheetah/util/mcp
python3 -m venv .venv
. .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -e .
```

## Run

Default stdio transport:

```bash
cd /path/to/GoldenCheetah/util/mcp
. .venv/bin/activate
goldencheetah-mcp --transport stdio
```

Optional streamable HTTP transport:

```bash
cd /path/to/GoldenCheetah/util/mcp
. .venv/bin/activate
goldencheetah-mcp --transport streamable-http --host 127.0.0.1 --port 8765
```

## Configuration

These can be passed as flags or environment variables:

- `--api-base-url` or `GC_API_BASE_URL`
- `--timeout` or `GC_API_TIMEOUT_SEC`
- `--host` or `GC_MCP_HOST`
- `--port` or `GC_MCP_PORT`
- `--transport` or `GC_MCP_TRANSPORT`

Examples:

```bash
GC_API_BASE_URL=http://127.0.0.1:12021 goldencheetah-mcp
GC_API_TIMEOUT_SEC=20 goldencheetah-mcp
```

## Tool behavior

### `gc_status`

Checks whether the local GoldenCheetah API is reachable and returns the athlete
count it can currently see.

### `gc_list_athletes`

Returns the athlete rows from GoldenCheetah's root CSV API.

### `gc_get_athlete_snapshot`

Calls `GET /<athlete>/ai/snapshot`.

### `gc_create_workout_draft`

Calls `POST /<athlete>/ai/draft`.

Supported workout types:

- `recovery`
- `endurance`
- `sweetspot`
- `threshold`
- `vo2max`
- `anaerobic`
- `mixed`

### `gc_save_workout`

Calls `POST /<athlete>/ai/save`.

This write tool requires `confirm=true`.

### `gc_create_planned_activity`

Calls `POST /<athlete>/ai/plan`.

This creates a planned activity that points at a saved workout file and also
requires `confirm=true`.

## Smoke test

With GoldenCheetah running and an athlete already open:

1. Run `goldencheetah-mcp --transport stdio`
2. Connect any MCP-capable client to that stdio command
3. Call `gc_list_athletes`
4. Call `gc_get_athlete_snapshot`
5. Call `gc_create_workout_draft`
6. Review the returned draft
7. Call `gc_save_workout` with the returned draft and `confirm=true`
8. Call `gc_create_planned_activity` with the saved `filepath` and `confirm=true`

## Tests

The test suite uses a mock GoldenCheetah HTTP server plus an in-process MCP
client:

```bash
cd /path/to/GoldenCheetah
. .venv-mcp/bin/activate
python -m unittest discover -s util/mcp/tests -t util/mcp
```
