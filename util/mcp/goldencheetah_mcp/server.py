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
from pathlib import Path
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

    def create_planned_activity(self, athlete: str, payload: dict[str, Any]) -> dict[str, Any]:
        return self._request_json("POST", self._athlete_path(athlete, "ai", "plan"), payload=payload)

    def simulate(self, athlete: str, payload: dict[str, Any]) -> dict[str, Any]:
        return self._request_json("POST", self._athlete_path(athlete, "ai", "simulate"), payload=payload)

    def banister(self, athlete: str, payload: dict[str, Any]) -> dict[str, Any]:
        return self._request_json("POST", self._athlete_path(athlete, "ai", "banister"), payload=payload)

    def list_activities(
        self,
        athlete: str,
        *,
        since: str = "",
        before: str = "",
        metrics: str = "",
        metadata: str = "",
    ) -> str:
        query: dict[str, str] = {}
        if since.strip():
            query["since"] = since.strip()
        if before.strip():
            query["before"] = before.strip()
        if metrics.strip():
            query["metrics"] = metrics.strip()
        if metadata.strip():
            query["metadata"] = metadata.strip()
        return self._request_text("GET", self._athlete_path(athlete), query=query or None)

    def get_activity(self, athlete: str, filename: str, *, fmt: str = "json") -> str:
        query: dict[str, str] | None = None
        if fmt.strip() and fmt.strip() != "json":
            query = {"format": fmt.strip()}
        return self._request_text("GET", self._athlete_path(athlete, "activity", filename), query=query)

    def get_meanmax(
        self,
        athlete: str,
        filename: str = "bests",
        *,
        series: str = "watts",
        since: str = "",
        before: str = "",
    ) -> str:
        query: dict[str, str] = {}
        if series.strip() and series.strip() != "watts":
            query["series"] = series.strip()
        if since.strip():
            query["since"] = since.strip()
        if before.strip():
            query["before"] = before.strip()
        return self._request_text("GET", self._athlete_path(athlete, "meanmax", filename), query=query or None)

    def get_zones(self, athlete: str, *, zone_type: str = "power") -> str:
        query: dict[str, str] | None = None
        if zone_type.strip() and zone_type.strip() != "power":
            query = {"for": zone_type.strip()}
        return self._request_text("GET", self._athlete_path(athlete, "zones"), query=query)

    def get_measures(
        self,
        athlete: str,
        group: str = "",
        *,
        since: str = "",
        before: str = "",
    ) -> str:
        if group.strip():
            path = self._athlete_path(athlete, "measures", group.strip())
        else:
            path = self._athlete_path(athlete, "measures")
        query: dict[str, str] = {}
        if since.strip():
            query["since"] = since.strip()
        if before.strip():
            query["before"] = before.strip()
        return self._request_text("GET", path, query=query or None)

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
            "or gc_create_planned_activity after explicit user confirmation."
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

    @mcp.tool(
        name="gc_create_planned_activity",
        description=(
            "Create a planned activity in GoldenCheetah that references a saved workout file. "
            "Only use this after the user has explicitly approved the write."
        ),
        annotations=ToolAnnotations(
            title="Create Planned Activity",
            readOnlyHint=False,
            destructiveHint=False,
            idempotentHint=False,
            openWorldHint=False,
        ),
    )
    def gc_create_planned_activity(
        athlete: str,
        workout_path: str,
        date: str,
        confirm: bool,
        time: str = "06:00:00",
        sport: str = "Bike",
        title: str = "",
        description: str = "",
    ) -> dict[str, Any]:
        if not confirm:
            raise ValueError("gc_create_planned_activity requires confirm=true after explicit user approval.")

        payload = _remove_empty_strings(
            {
                "workoutPath": workout_path,
                "date": date,
                "time": time,
                "sport": sport,
                "title": title,
                "description": description,
            }
        )
        return client.create_planned_activity(athlete=athlete, payload=payload)

    # ------------------------------------------------------------------
    # AI simulation tools (Phase 1+2)
    # ------------------------------------------------------------------

    @mcp.tool(
        name="gc_simulate_workout",
        description=(
            "Simulate all 7 workout types forward and rank them by training goal. "
            "Uses PMC-based forward projection with physiological safety constraints. "
            "Goals: 'build' (maximize fitness), 'maintain' (steady state), "
            "'taper' (reduce fatigue pre-race), 'recover' (maximize freshness). "
            "Returns ranked candidates with projected CTL/ATL/TSB and constraint violations."
        ),
        annotations=ToolAnnotations(
            title="Simulate Workout Selection",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_simulate_workout(
        athlete: str,
        goal: str = "build",
        duration_min: int = 60,
        date: str = "",
    ) -> dict[str, Any]:
        payload: dict[str, Any] = {
            "goal": goal,
            "durationMin": duration_min,
        }
        if date.strip():
            payload["date"] = date.strip()
        return client.simulate(athlete=athlete, payload=payload)

    @mcp.tool(
        name="gc_banister_compare",
        description=(
            "Compare training plan templates using the Banister impulse-response model. "
            "Extracts the athlete's fitted model parameters (k1, k2, τ1, τ2) and projects "
            "predicted performance for each plan template over the specified number of days. "
            "Templates: '3-1' (3 build + 1 recovery week), '2-1', 'progressive', 'polarized', 'flat'. "
            "Requires weeklyTSS (target weekly training stress) and days (planning horizon)."
        ),
        annotations=ToolAnnotations(
            title="Banister Plan Comparison",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_banister_compare(
        athlete: str,
        weekly_tss: float,
        days: int = 28,
        date: str = "",
        metric: str = "coggan_tss",
    ) -> dict[str, Any]:
        payload: dict[str, Any] = {
            "weeklyTSS": weekly_tss,
            "days": days,
            "metric": metric,
        }
        if date.strip():
            payload["date"] = date.strip()
        return client.banister(athlete=athlete, payload=payload)

    # ------------------------------------------------------------------
    # New read tools
    # ------------------------------------------------------------------

    @mcp.tool(
        name="gc_list_activities",
        description=(
            "List activities for an athlete with optional metric/metadata columns and date filtering. "
            "Returns CSV text. Use 'metrics' to request specific ride metrics (comma-separated, or 'NONE'), "
            "'metadata' for metadata fields (comma-separated, 'ALL', or 'NONE'), "
            "and 'since'/'before' for date range filtering (yyyy/MM/dd)."
        ),
        annotations=ToolAnnotations(
            title="List Activities",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_list_activities(
        athlete: str,
        since: str = "",
        before: str = "",
        metrics: str = "",
        metadata: str = "",
    ) -> dict[str, Any]:
        csv_text = client.list_activities(
            athlete,
            since=since,
            before=before,
            metrics=metrics,
            metadata=metadata,
        )
        return {"csv": csv_text}

    @mcp.tool(
        name="gc_get_activity",
        description=(
            "Get detailed data for a single activity by filename. "
            "Supported formats: json (default), csv, tcx, pwx."
        ),
        annotations=ToolAnnotations(
            title="Get Activity",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_get_activity(
        athlete: str,
        filename: str,
        format: str = "json",
    ) -> dict[str, Any]:
        raw = client.get_activity(athlete, filename, fmt=format)
        if format.strip() == "json" or not format.strip():
            try:
                return json.loads(raw)
            except json.JSONDecodeError:
                return {"raw": raw}
        return {"raw": raw}

    @mcp.tool(
        name="gc_get_meanmax",
        description=(
            "Get mean-max (best power/HR/etc.) data. Use filename='bests' for all-time bests "
            "or a specific activity filename. Supports series: watts (default), hr, cad, speed, "
            "vam, IsoPower, xPower, nm. Supports 'since'/'before' date filtering for bests."
        ),
        annotations=ToolAnnotations(
            title="Mean Max Data",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_get_meanmax(
        athlete: str,
        filename: str = "bests",
        series: str = "watts",
        since: str = "",
        before: str = "",
    ) -> dict[str, Any]:
        csv_text = client.get_meanmax(
            athlete,
            filename=filename,
            series=series,
            since=since,
            before=before,
        )
        return {"csv": csv_text}

    @mcp.tool(
        name="gc_get_zones",
        description=(
            "Get training zones for an athlete. Supported zone types: "
            "power (default), hr, pace, swimpace."
        ),
        annotations=ToolAnnotations(
            title="Get Zones",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_get_zones(
        athlete: str,
        zone_type: str = "power",
    ) -> dict[str, Any]:
        csv_text = client.get_zones(athlete, zone_type=zone_type)
        return {"csv": csv_text}

    @mcp.tool(
        name="gc_get_measures",
        description=(
            "Get body measures or HRV data for an athlete. "
            "Omit 'group' to list available measure groups. "
            "Common groups: Body, Hrv. Supports 'since'/'before' date filtering."
        ),
        annotations=ToolAnnotations(
            title="Get Measures",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_get_measures(
        athlete: str,
        group: str = "",
        since: str = "",
        before: str = "",
    ) -> dict[str, Any]:
        csv_text = client.get_measures(
            athlete,
            group=group,
            since=since,
            before=before,
        )
        return {"csv": csv_text}

    # ------------------------------------------------------------------
    # File-based write tools (calendar / workout management)
    # ------------------------------------------------------------------

    @mcp.tool(
        name="gc_list_planned_activities",
        description=(
            "List all planned activities on the calendar for an athlete. "
            "Returns the JSON content of each planned activity file."
        ),
        annotations=ToolAnnotations(
            title="List Planned Activities",
            readOnlyHint=True,
            destructiveHint=False,
            idempotentHint=True,
            openWorldHint=False,
        ),
    )
    def gc_list_planned_activities(athlete: str) -> dict[str, Any]:
        planned_dir = _resolve_planned_dir(athlete)
        if not planned_dir.is_dir():
            return {"activities": []}
        activities: list[dict[str, Any]] = []
        for f in sorted(planned_dir.glob("*.json")):
            try:
                data = json.loads(f.read_text(encoding="utf-8"))
                data["_filepath"] = str(f)
                activities.append(data)
            except (json.JSONDecodeError, OSError):
                continue
        return {"activities": activities}

    @mcp.tool(
        name="gc_delete_planned_activity",
        description=(
            "Remove a planned activity from the calendar by its filepath. "
            "Only use this after the user has explicitly approved the deletion."
        ),
        annotations=ToolAnnotations(
            title="Delete Planned Activity",
            readOnlyHint=False,
            destructiveHint=True,
            idempotentHint=False,
            openWorldHint=False,
        ),
    )
    def gc_delete_planned_activity(
        athlete: str,
        filepath: str,
        confirm: bool,
    ) -> dict[str, Any]:
        if not confirm:
            raise ValueError("gc_delete_planned_activity requires confirm=true after explicit user approval.")
        path = Path(filepath)
        _validate_gc_path(path)
        if not path.exists():
            raise ValueError(f"Planned activity file not found: {filepath}")
        if not path.suffix == ".json":
            raise ValueError("Expected a .json planned activity file.")
        path.unlink()
        return {"deleted": True, "filepath": filepath}

    @mcp.tool(
        name="gc_update_planned_activity",
        description=(
            "Update a planned activity on the calendar. Provide the filepath of the existing "
            "planned activity and any fields to change (date, time, workout_path, sport, title, description). "
            "Only use this after the user has explicitly approved the change."
        ),
        annotations=ToolAnnotations(
            title="Update Planned Activity",
            readOnlyHint=False,
            destructiveHint=False,
            idempotentHint=False,
            openWorldHint=False,
        ),
    )
    def gc_update_planned_activity(
        athlete: str,
        filepath: str,
        confirm: bool,
        date: str = "",
        time: str = "",
        workout_path: str = "",
        sport: str = "",
        title: str = "",
        description: str = "",
    ) -> dict[str, Any]:
        if not confirm:
            raise ValueError("gc_update_planned_activity requires confirm=true after explicit user approval.")
        path = Path(filepath)
        _validate_gc_path(path)
        if not path.exists():
            raise ValueError(f"Planned activity file not found: {filepath}")

        data = json.loads(path.read_text(encoding="utf-8"))

        if date.strip():
            data["date"] = date.strip()
        if time.strip():
            data["time"] = time.strip()
        if workout_path.strip():
            data["workoutPath"] = workout_path.strip()
        if sport.strip():
            data["sport"] = sport.strip()
        if title.strip():
            data["title"] = title.strip()
        if description.strip():
            data["description"] = description.strip()

        path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
        return {"updated": True, "filepath": filepath, "activity": data}

    @mcp.tool(
        name="gc_delete_workout",
        description=(
            "Remove a saved workout (.erg) file from the workout library. "
            "Only use this after the user has explicitly approved the deletion."
        ),
        annotations=ToolAnnotations(
            title="Delete Workout",
            readOnlyHint=False,
            destructiveHint=True,
            idempotentHint=False,
            openWorldHint=False,
        ),
    )
    def gc_delete_workout(
        athlete: str,
        filepath: str,
        confirm: bool,
    ) -> dict[str, Any]:
        if not confirm:
            raise ValueError("gc_delete_workout requires confirm=true after explicit user approval.")
        path = Path(filepath)
        _validate_gc_path(path)
        if not path.exists():
            raise ValueError(f"Workout file not found: {filepath}")
        if path.suffix not in (".erg", ".mrc", ".zwo"):
            raise ValueError("Expected a workout file (.erg, .mrc, or .zwo).")
        path.unlink()
        return {"deleted": True, "filepath": filepath}

    return mcp


def _resolve_planned_dir(athlete: str) -> Path:
    gc_home = Path.home() / ".goldencheetah" / athlete / "planned"
    return gc_home


def _validate_gc_path(path: Path) -> None:
    gc_home = Path.home() / ".goldencheetah"
    try:
        path.resolve().relative_to(gc_home.resolve())
    except ValueError:
        raise ValueError(
            f"Path {path} is not inside the GoldenCheetah data directory ({gc_home})."
        )


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
