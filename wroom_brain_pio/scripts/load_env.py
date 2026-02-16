from pathlib import Path
from typing import Dict

from SCons.Script import Exit, Import

Import("env")


def parse_env(path: Path) -> Dict[str, str]:
    data: Dict[str, str] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if not key:
            continue
        if len(value) >= 2 and value[0] == value[-1] and value[0] in {'"', "'"}:
            value = value[1:-1]
        data[key] = value
    return data


def cpp_quoted(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


project_dir = Path(env.subst("$PROJECT_DIR"))
env_file = project_dir / ".env"

if not env_file.exists():
    print("[env] Missing .env. Copy .env.example to .env and fill your values.")
    Exit(1)

parsed = parse_env(env_file)

required_keys = [
    "WIFI_SSID",
    "WIFI_PASS",
    "TELEGRAM_BOT_TOKEN",
    "TELEGRAM_ALLOWED_CHAT_ID",
]

missing = [k for k in required_keys if not parsed.get(k)]
if missing:
    print(f"[env] Missing required keys in .env: {', '.join(missing)}")
    Exit(1)

poll_ms = parsed.get("TELEGRAM_POLL_MS", "3000")
status_enabled = parsed.get("AUTONOMOUS_STATUS_ENABLED", "0")
status_ms = parsed.get("AUTONOMOUS_STATUS_MS", "30000")
llm_timeout_ms = parsed.get("LLM_TIMEOUT_MS", "25000")
memory_max_chars = parsed.get("MEMORY_MAX_CHARS", "1800")
heartbeat_enabled = parsed.get("HEARTBEAT_ENABLED", "1")
heartbeat_interval_ms = parsed.get("HEARTBEAT_INTERVAL_MS", "600000")
soul_max_chars = parsed.get("SOUL_MAX_CHARS", "1400")
heartbeat_max_chars = parsed.get("HEARTBEAT_MAX_CHARS", "1400")
reminder_msg_max_chars = parsed.get("REMINDER_MSG_MAX_CHARS", "220")
reminder_grace_minutes = parsed.get("REMINDER_GRACE_MINUTES", "10")
tasks_max_chars = parsed.get("TASKS_MAX_CHARS", "2600")
timezone_tz = parsed.get("TIMEZONE_TZ", "UTC0")
ntp_server_1 = parsed.get("NTP_SERVER_1", "pool.ntp.org")
ntp_server_2 = parsed.get("NTP_SERVER_2", "time.nist.gov")
web_search_api_key = parsed.get("WEB_SEARCH_API_KEY", "")
web_search_provider = parsed.get("WEB_SEARCH_PROVIDER", "auto")
web_search_base_url = parsed.get("WEB_SEARCH_BASE_URL", "https://api.search.brave.com")
web_search_timeout_ms = parsed.get("WEB_SEARCH_TIMEOUT_MS", "12000")
web_search_results_max = parsed.get("WEB_SEARCH_RESULTS_MAX", "3")
web_job_endpoint_url = parsed.get("WEB_JOB_ENDPOINT_URL", "")
web_job_api_key = parsed.get("WEB_JOB_API_KEY", "")
web_job_timeout_ms = parsed.get("WEB_JOB_TIMEOUT_MS", "20000")

image_provider = parsed.get("IMAGE_PROVIDER", "none")
image_api_key = parsed.get("IMAGE_API_KEY", "")

resend_api_key = parsed.get("RESEND_API_KEY", "")
email_provider = parsed.get("EMAIL_PROVIDER", "resend")
sendgrid_api_key = parsed.get("SENDGRID_API_KEY", "")
email_from = parsed.get("EMAIL_FROM", parsed.get("RESEND_FROM_EMAIL", "onboarding@resend.dev"))

if (
    not poll_ms.isdigit()
    or not status_enabled.isdigit()
    or not status_ms.isdigit()
    or not llm_timeout_ms.isdigit()
    or not memory_max_chars.isdigit()
    or not heartbeat_enabled.isdigit()
    or not heartbeat_interval_ms.isdigit()
    or not soul_max_chars.isdigit()
    or not heartbeat_max_chars.isdigit()
    or not reminder_msg_max_chars.isdigit()
    or not reminder_grace_minutes.isdigit()
    or not tasks_max_chars.isdigit()
    or not web_search_timeout_ms.isdigit()
    or not web_search_results_max.isdigit()
    or not web_job_timeout_ms.isdigit()
):
    print(
        "[env] TELEGRAM_POLL_MS, AUTONOMOUS_STATUS_ENABLED, AUTONOMOUS_STATUS_MS, "
        "LLM_TIMEOUT_MS, MEMORY_MAX_CHARS, HEARTBEAT_ENABLED, HEARTBEAT_INTERVAL_MS, "
        "SOUL_MAX_CHARS, HEARTBEAT_MAX_CHARS, REMINDER_MSG_MAX_CHARS, and "
        "REMINDER_GRACE_MINUTES, TASKS_MAX_CHARS, WEB_SEARCH_TIMEOUT_MS, "
        "WEB_SEARCH_RESULTS_MAX, WEB_JOB_TIMEOUT_MS must be integers."
    )
    Exit(1)

llm_provider = parsed.get("LLM_PROVIDER", "none")
llm_api_key = parsed.get("LLM_API_KEY", "")
llm_model = parsed.get("LLM_MODEL", "")
llm_openai_base = parsed.get("LLM_OPENAI_BASE_URL", "https://api.openai.com")
llm_anthropic_base = parsed.get("LLM_ANTHROPIC_BASE_URL", "https://api.anthropic.com")
llm_gemini_base = parsed.get(
    "LLM_GEMINI_BASE_URL", "https://generativelanguage.googleapis.com"
)
llm_glm_base = parsed.get("LLM_GLM_BASE_URL", "https://open.bigmodel.cn")

generated_header = project_dir / "include" / "brain_secrets.generated.h"
generated_header.write_text(
    "\n".join(
        [
            "#ifndef BRAIN_SECRETS_GENERATED_H",
            "#define BRAIN_SECRETS_GENERATED_H",
            "",
            f"#define WIFI_SSID {cpp_quoted(parsed['WIFI_SSID'])}",
            f"#define WIFI_PASS {cpp_quoted(parsed['WIFI_PASS'])}",
            f"#define TELEGRAM_BOT_TOKEN {cpp_quoted(parsed['TELEGRAM_BOT_TOKEN'])}",
            f"#define TELEGRAM_ALLOWED_CHAT_ID {cpp_quoted(parsed['TELEGRAM_ALLOWED_CHAT_ID'])}",
            f"#define TELEGRAM_POLL_MS {poll_ms}",
            f"#define AUTONOMOUS_STATUS_ENABLED {status_enabled}",
            f"#define AUTONOMOUS_STATUS_MS {status_ms}",
            f"#define LLM_PROVIDER {cpp_quoted(llm_provider)}",
            f"#define LLM_API_KEY {cpp_quoted(llm_api_key)}",
            f"#define LLM_MODEL {cpp_quoted(llm_model)}",
            f"#define LLM_OPENAI_BASE_URL {cpp_quoted(llm_openai_base)}",
            f"#define LLM_ANTHROPIC_BASE_URL {cpp_quoted(llm_anthropic_base)}",
            f"#define LLM_GEMINI_BASE_URL {cpp_quoted(llm_gemini_base)}",
            f"#define LLM_GLM_BASE_URL {cpp_quoted(llm_glm_base)}",
            f"#define LLM_TIMEOUT_MS {llm_timeout_ms}",
            f"#define MEMORY_MAX_CHARS {memory_max_chars}",
            f"#define HEARTBEAT_ENABLED {heartbeat_enabled}",
            f"#define HEARTBEAT_INTERVAL_MS {heartbeat_interval_ms}",
            f"#define SOUL_MAX_CHARS {soul_max_chars}",
            f"#define HEARTBEAT_MAX_CHARS {heartbeat_max_chars}",
            f"#define REMINDER_MSG_MAX_CHARS {reminder_msg_max_chars}",
            f"#define REMINDER_GRACE_MINUTES {reminder_grace_minutes}",
            f"#define TASKS_MAX_CHARS {tasks_max_chars}",
            f"#define TIMEZONE_TZ {cpp_quoted(timezone_tz)}",
            f"#define NTP_SERVER_1 {cpp_quoted(ntp_server_1)}",
            f"#define NTP_SERVER_2 {cpp_quoted(ntp_server_2)}",
            f"#define WEB_SEARCH_API_KEY {cpp_quoted(web_search_api_key)}",
            f"#define WEB_SEARCH_PROVIDER {cpp_quoted(web_search_provider)}",
            f"#define WEB_SEARCH_BASE_URL {cpp_quoted(web_search_base_url)}",
            f"#define WEB_SEARCH_TIMEOUT_MS {web_search_timeout_ms}",
            f"#define WEB_SEARCH_RESULTS_MAX {web_search_results_max}",
            f"#define WEB_JOB_ENDPOINT_URL {cpp_quoted(web_job_endpoint_url)}",
            f"#define WEB_JOB_API_KEY {cpp_quoted(web_job_api_key)}",
            f"#define WEB_JOB_TIMEOUT_MS {web_job_timeout_ms}",
            f"#define IMAGE_PROVIDER {cpp_quoted(image_provider)}",
            f"#define IMAGE_API_KEY {cpp_quoted(image_api_key)}",
            f"#define EMAIL_PROVIDER {cpp_quoted(email_provider)}",
            f"#define SENDGRID_API_KEY {cpp_quoted(sendgrid_api_key)}",
            f"#define EMAIL_FROM {cpp_quoted(email_from)}",
            f"#define RESEND_API_KEY {cpp_quoted(resend_api_key)}",
            "",
            "#endif",
            "",
        ]
    ),
    encoding="utf-8",
)

print("[env] Generated include/brain_secrets.generated.h from .env.")
