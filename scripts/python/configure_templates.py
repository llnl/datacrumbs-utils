from __future__ import annotations

import argparse
import os
from pathlib import Path
import sqlite3
from typing import Any


def _load_system_configuration(path: Path) -> dict[str, str]:
    with sqlite3.connect(path) as connection:
        system_configuration = dict(
            connection.execute("SELECT key, value FROM system_configuration").fetchall()
        )
        summary = dict(connection.execute("SELECT key, value FROM summary").fetchall())

    variables = {str(key): _stringify(value) for key, value in system_configuration.items()}

    install_prefix = variables.get("DATACRUMBS_INSTALL_PREFIX", "")
    if install_prefix:
        variables["DATACRUMBS_INSTALL_DIR"] = install_prefix

    install_host = variables.get("DATACRUMBS_INSTALL_HOST", "")
    if install_host:
        variables["DATACRUMBS_HOST"] = install_host

    trace_dir = summary.get("trace_log_dir")
    if isinstance(trace_dir, str) and trace_dir:
        variables["DATACRUMBS_CONFIGURED_TRACE_DIR"] = trace_dir

    configured_log_dir = variables.get("DATACRUMBS_LOG_DIR", "")
    if configured_log_dir:
        variables["DATACRUMBS_CONFIGURED_LOG_DIR"] = configured_log_dir

    scheduler = variables.get("DATACRUMBS_JOB_SCHEDULER", "")
    if scheduler:
        variables["DATACRUMBS_SCHEDULER_TYPE"] = scheduler

    job_id_var = variables.get("DATACRUMBS_JOB_ID_VAR", "")
    if job_id_var:
        variables["DATACRUMBS_SCHEDULER_JOBID_ENV_VAR"] = job_id_var

    inclusion_paths = variables.get("DATACRUMBS_INCLUSION_PATHS", "")
    if inclusion_paths:
        variables["DATACRUMBS_INCLUSION_PATH"] = inclusion_paths

    return variables


def _stringify(value: Any) -> str:
    if value is None:
        return ""
    if isinstance(value, bool):
        return "1" if value else "0"
    return str(value)


def _parse_defines(items: list[str]) -> dict[str, str]:
    defines: dict[str, str] = {}
    for item in items:
        key, sep, value = item.partition("=")
        if not sep or not key:
            raise ValueError(f"Expected KEY=VALUE for --define, got: {item}")
        defines[key] = value
    return defines


def _discover_system_configuration() -> Path:
    install_dir = os.environ.get("DATACRUMBS_INSTALL_DIR", "")
    if not install_dir:
        raise ValueError(
            "DATACRUMBS_INSTALL_DIR is not set. Use the installed datacrumbs_configure_template wrapper."
        )

    data_dir = Path(install_dir).resolve() / "share" / "datacrumbs" / "data"
    matches = sorted(data_dir.glob("system-probe-*.sqlite"))
    if not matches:
        raise ValueError(f"No system-probe-*.sqlite found under {data_dir}")
    if len(matches) > 1:
        raise ValueError(f"Multiple system-probe-*.sqlite files found under {data_dir}")
    return matches[0]


def _expand_text(text: str, settings: dict[str, str]) -> str:
    expanded = text
    for key, value in settings.items():
        expanded = expanded.replace(f"@{key}@", value)
    return expanded


def render_template(template_path: Path, system_configuration_path: Path,
                    output_path: Path, defines: dict[str, str] | None = None) -> dict[str, str]:
    settings = _load_system_configuration(system_configuration_path)
    if defines:
        settings.update(defines)

    template_text = template_path.read_text(encoding="utf-8")
    expanded = _expand_text(template_text, settings)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(expanded, encoding="utf-8")
    return settings


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Render one datacrumbs-utils YAML template using the installed datacrumbs system configuration"
    )
    parser.add_argument("template", help="Input YAML template path")
    parser.add_argument("output", help="Rendered output YAML path")
    parser.add_argument(
        "--define",
        action="append",
        default=[],
        help="Additional template variable in KEY=VALUE form; may be repeated",
    )
    args = parser.parse_args()
    system_configuration_path = _discover_system_configuration()

    render_template(
        template_path=Path(args.template).resolve(),
        system_configuration_path=system_configuration_path,
        output_path=Path(args.output).resolve(),
        defines=_parse_defines(args.define),
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
