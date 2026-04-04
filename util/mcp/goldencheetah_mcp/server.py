from __future__ import annotations

import argparse
import csv
import io
import json
import os
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from typing import Any

from mcp.server.fastmcp import FastMCP
from mcp.types import ToolAnnotations


DEFAULT_API_BASE_URL = "http://127.0.0.1:12021"
DEFAULT_TIMEOUT_SEC = 10.0
DEFAULT_MCP_HOST = "127.0.0.1"
DEFAULT_MCP_PORT = 8765


class GoldenCheetahApiError(RuntimeError):
    """Raised when the local GoldenCheetah HTTP API rejects a request."""

    def __init__(self, status: int, message: str, body: str = "") -> None:
        super().__init__(message)
        self.status = status
        self.message = message
        self.body = body

    def to_dict(self) -> dict[str, Any]:
        return {
            "status": self.status,
            "error": self.message,
            "body": self.body,
        }


@dataclass(frozen=True)
class GoldenCheetahApiConfig:
    base_url: str = DEFAULT_API_BASE_URL
    timeout_sec: float = DEFAULT_TIMEOUT_SEC
    user_agent: str = "goldencheetah-mcp/0.1"


class GoldenCheetahApiClient:
    def __init__(self, config: GoldenCheetahApiConfig) -> None:
        self._config = config

    @property
    def base_url(self) -> str:
        return self._config.base_url

    def list_athletes(self) -> list[dict[str, str]]:
        payload = self._request_text("GET", "/")
        reader = csv.DictReader(io.StringIO(payload), skipinitialspace=True)
        athletes: list[dict[str, str]] = []
        for row in reader:
            cleaned = {key: (value or "").strip() for key, value in row.items() if key}
            if cleaned:
                athletes.append(cleaned)
        return athletes

    def snapshot(self, athlete: str, date: str = "") -> dict[str, Any]:
        return self._request_json("GET", self._athlete_path(athlete, "ai", "snapshot"), query=self._date_query(date))

    def create_draft(self, athlete: str, payload: dict[str, Any]) -> dict[str, Any]:
        return self._request_json("POST", self._athlete_path(athlete, "ai", "draft"), payload=payload)

    def save_draft(self, athlete: str, payload: dict[str, Any]) -> dict[str, Any]:
        return self._request_json("POST", self._athlete_path(athlete, "ai", "save"), payload=payload)

    def _athlete_path(self, athlete: str, *segments: str) -> str:
        encoded = [urllib.parse.quote(part, safe="") for part in (athlete, *segments)]
        return "/" + "/".join(encoded)

    def _date_query(self, date: str) -> dict[str, str] | None:
        cleaned = date.strip()
        if not cleaned:
            return None
        return {"date": cleaned}

    def _request_text(
        self,
        method: str,
        path: str,
        *,
        query: dict[str, str] | None = None,
        payload: dict[str, Any] | None = None,
    ) -> str:
        url = self._build_url(path, query)
        data: bytes | None = None
        headers = {
            "Accept": "application/json, text/plain, text/csv",
            "User-Agent": self._config.user_agent,
        }
        if payload is not None:
            data = json.dumps(payload).encode("utf-8")
            headers["Content-Type"] = "application/json"

        request = urllib.request.Request(url, data=data, headers=headers, method=method)
        try:
            with urllib.request.urlopen(request, timeout=self._config.timeout_sec) as response:
                return response.read().decode("utf-8")
        except urllib.error.HTTPError as exc:
            body = exc.read().decode("utf-8", errors="replace")
            raise GoldenCheetahApiError(exc.code, self._extract_error_message(body), body) from exc
        except urllib.error.URLError as exc:
            raise GoldenCheetahApiError(503, f"Could not reach GoldenCheetah at {self.base_url}: {exc.reason}") from exc

    def _request_json(
        self,
        method: str,
        path: str,
        *,
        query: dict[str, str] | None = None,
        payload: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        text = self._request_text(method, path, query=query, payload=payload)
        try:
            data = json.loads(text)
        except json.JSONDecodeError as exc:
            raise GoldenCheetahApiError(502, "GoldenCheetah returned invalid JSON", text) from exc
        if not isinstance(data, dict):
            raise GoldenCheetahApiError(502, "GoldenCheetah returned an unexpected JSON payload", text)
        return data

    def _build_url(self, path: str, query: dict[str, str] | None) -> str:
        url = self.base_url.rstrip("/") + path
        if query:
            url += "?" + urllib.parse.urlencode(query)
        return url

    @staticmethod
    def _extract_error_message(body: str) -> str:
        try:
            data = json.loads(body)
        except json.JSONDecodeError:
            return body.strip() or "GoldenCheetah request failed"
        if isinstance(data, dict):
            message = data.get("error")
            if isinstance(message, str) and message.strip():
                return message.strip()
        return body.strip() or "GoldenCheetah request failed"


def build_server(
    *,
    api_base_url: str = DEFAULT_API_BASE_URL,
    timeout_sec: float = DEFAULT_TIMEOUT_SEC,
    host: str = DEFAULT_MCP_HOST,
    port: int = DEFAULT_MCP_PORT,
) -> FastMCP:
    client = GoldenCheetahApiClient(
        GoldenCheetahApiConfig(
            base_url=api_base_url.rstrip("/"),
            timeout_sec=timeout_sec,
        )
    )

    mcp = FastMCP(
        "GoldenCheetah MCP",
        instructions=(
            "This server bridges to a local GoldenCheetah instance over loopback HTTP. "
            "Use gc_create_workout_draft to prepare workouts and only call gc_save_workout "
            "after explicit user confirmation."
        ),
        host=host,
        port=port,
        streamable_http_path="/mcp",
        json_response=True,
    )

    @mcp.tool(
        name="gc_status",
        description="Check whether the local GoldenCheetah API is reachable over loopback HTTP.",
        annotations=ToolAnnotations(
            title="GoldenCheetah Status",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_status() -> dict[str, Any]:
        athletes = client.list_athletes()
        return {
            "ok": True,
            "apiBaseUrl": client.base_url,
            "athleteCount": len(athletes),
            "athletes": athletes,
        }

    @mcp.tool(
        name="gc_list_athletes",
        description="List athlete profiles visible from GoldenCheetah's local API.",
        annotations=ToolAnnotations(
            title="List Athletes",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_list_athletes() -> dict[str, Any]:
        return {
            "athletes": client.list_athletes(),
            "apiBaseUrl": client.base_url,
        }

    @mcp.tool(
        name="gc_get_athlete_snapshot",
        description=(
            "Get the current AI workout snapshot for an athlete from GoldenCheetah. "
            "The athlete must already be open in the desktop app."
        ),
        annotations=ToolAnnotations(
            title="Athlete Snapshot",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_get_athlete_snapshot(athlete: str, date: str = "") -> dict[str, Any]:
        return client.snapshot(athlete=athlete, date=date)

    @mcp.tool(
        name="gc_create_workout_draft",
        description=(
            "Create a GoldenCheetah workout draft without saving it. "
            "Supported types: recovery, endurance, sweetspot, threshold, vo2max, anaerobic, mixed."
        ),
        annotations=ToolAnnotations(
            title="Create Workout Draft",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_create_workout_draft(
        athlete: str,
        workout_type: str,
        duration_min: int = 60,
        date: str = "",
        display_name: str = "",
        description: str = "",
        generator_id: str = "goldencheetah-mcp",
        model_id: str = "",
        model_version: str = "",
    ) -> dict[str, Any]:
        payload = {
            "workoutType": workout_type,
            "durationMin": duration_min,
            "date": date,
            "displayName": display_name,
            "description": description,
            "generatorId": generator_id,
            "modelId": model_id,
            "modelVersion": model_version,
        }
        return client.create_draft(athlete=athlete, payload=_remove_empty_strings(payload))

    @mcp.tool(
        name="gc_save_workout",
        description=(
            "Persist a previously reviewed WorkoutDraft into GoldenCheetah's workout library. "
            "Only use this after the user has explicitly approved the write."
        ),
        annotations=ToolAnnotations(
            title="Save Workout",
            readOnlyHint=False,
            destructiveHint=False,
            idempotentHint=False,
            openWorldHint=False,
        ),
    )
    def gc_save_workout(
        athlete: str,
        draft: dict[str, Any],
        confirm: bool,
        date: str = "",
    ) -> dict[str, Any]:
        if not confirm:
            raise ValueError("gc_save_workout requires confirm=true after explicit user approval.")
        if not isinstance(draft, dict):
            raise ValueError("draft must be a WorkoutDraft object.")

        payload: dict[str, Any] = {"draft": draft}
        if date.strip():
            payload["date"] = date.strip()
        return client.save_draft(athlete=athlete, payload=payload)

    return mcp


def _remove_empty_strings(payload: dict[str, Any]) -> dict[str, Any]:
    return {
        key: value
        for key, value in payload.items()
        if not isinstance(value, str) or value.strip()
    }


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="GoldenCheetah local MCP bridge")
    parser.add_argument(
        "--transport",
        choices=("stdio", "streamable-http", "sse"),
        default=os.environ.get("GC_MCP_TRANSPORT", "stdio"),
        help="MCP transport to expose",
    )
    parser.add_argument(
        "--api-base-url",
        default=os.environ.get("GC_API_BASE_URL", DEFAULT_API_BASE_URL),
        help="GoldenCheetah loopback API base URL",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=float(os.environ.get("GC_API_TIMEOUT_SEC", DEFAULT_TIMEOUT_SEC)),
        help="HTTP timeout in seconds for GoldenCheetah API calls",
    )
    parser.add_argument(
        "--host",
        default=os.environ.get("GC_MCP_HOST", DEFAULT_MCP_HOST),
        help="Host for MCP HTTP transports",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=int(os.environ.get("GC_MCP_PORT", str(DEFAULT_MCP_PORT))),
        help="Port for MCP HTTP transports",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    server = build_server(
        api_base_url=args.api_base_url,
        timeout_sec=args.timeout,
        host=args.host,
        port=args.port,
    )
    server.run(transport=args.transport)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
