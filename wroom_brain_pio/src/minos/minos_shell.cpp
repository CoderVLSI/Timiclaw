#include "minos.h"
#include <vector>

static String shell_output;

static void shell_print(const String &s) { shell_output += s; }
static void shell_println(const String &s) { shell_output += s + "\n"; }

/* Command Implementations */
static void cmd_help() {
    shell_println("\nAvailable commands:");
    shell_println("  help       - Show this help");
    shell_println("  ps         - List tasks");
    shell_println("  ls         - List SPIFFS files");
    shell_println("  cat <file> - Print file content");
    shell_println("  rm <file>  - Delete a file");
    shell_println("  df         - Show disk usage");
    shell_println("  free       - Show free RAM");
    shell_println("  uptime     - Show system uptime");
    shell_println("  sysinfo    - Show system information");
    shell_println("  reboot     - Restart ESP32");
}

static void cmd_ps() {
    shell_println("\nPID  STATE      PRI  NAME");
    shell_println("---  ---------  ---  ----");
    for (uint32_t i = 0; i < task_count; i++) {
        const char *state = "UNKNOWN";
        switch (tasks[i].state) {
            case TASK_READY:    state = "READY"; break;
            case TASK_RUNNING:  state = "RUNNING"; break;
            case TASK_SLEEPING: state = "SLEEP"; break;
            case TASK_BLOCKED:  state = "BLOCKED"; break;
        }
        char buf[64];
        snprintf(buf, sizeof(buf), "%-3d  %-9s  %-3d  %s", 
                tasks[i].id, state, tasks[i].priority, tasks[i].name);
        shell_println(buf);
    }
}

static void cmd_ls() {
    shell_println("\nListing SPIFFS:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
        shell_print(String(file.name()) + " \t");
        shell_println(String(file.size()) + " bytes");
        file = root.openNextFile();
    }
}

static void cmd_cat(const String &path) {
    if (!path.startsWith("/")) {
        cmd_cat("/" + path);
        return;
    }
    if (!SPIFFS.exists(path)) {
        shell_println("Error: File " + path + " not found");
        return;
    }
    File f = SPIFFS.open(path, "r");
    while(f.available()) {
        shell_print(String((char)f.read()));
    }
    shell_println("");
    f.close();
}

static void cmd_rm(const String &path) {
    String p = path.startsWith("/") ? path : "/" + path;
    if (SPIFFS.remove(p)) {
        shell_println("Removed " + p);
    } else {
        shell_println("Error: Could not remove " + p);
    }
}

static void cmd_df() {
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    shell_println("SPIFFS Usage:");
    shell_println("Total: " + String(total) + " bytes");
    shell_println("Used:  " + String(used) + " bytes");
    shell_println("Free:  " + String(total - used) + " bytes");
}

static void cmd_free() {
    shell_println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
}

void shell_init(void) {
    shell_println("MinOS Shell v0.2 (ESP32 Port)");
    shell_println("Type 'help' for commands");
}

void shell_run_once(const String &input, String &output) {
    shell_output = "";
    String cmd_line = input;
    cmd_line.trim();

    if (cmd_line == "help") cmd_help();
    else if (cmd_line == "ps") cmd_ps();
    else if (cmd_line == "ls") cmd_ls();
    else if (cmd_line == "df") cmd_df();
    else if (cmd_line == "free") cmd_free();
    else if (cmd_line == "uptime") {
        uint32_t sec = millis() / 1000;
        uint32_t min = sec / 60;
        uint32_t hr = min / 60;
        shell_println("Uptime: " + String(hr) + "h " + String(min % 60) + "m " + String(sec % 60) + "s");
    }
    else if (cmd_line == "sysinfo") {
        shell_println("OS: MinOS v0.2 (ESP32)");
        shell_println("CPU: Xtensa LX6 @ 240MHz");
        shell_println("Flash: " + String(ESP.getFlashChipSize() / (1024*1024)) + "MB");
        shell_println("Chip ID: " + String((uint32_t)ESP.getEfuseMac(), HEX));
    }
    else if (cmd_line == "reboot") {
        shell_println("Rebooting...");
        output = shell_output;
        delay(100);
        ESP.restart();
    }
    else if (cmd_line.startsWith("cat ")) {
        cmd_cat(cmd_line.substring(4));
    }
    else if (cmd_line.startsWith("rm ")) {
        cmd_rm(cmd_line.substring(3));
    }
    else if (cmd_line.length() > 0) {
        shell_println("MinOS: Unknown command: " + cmd_line);
    }

    output = shell_output;
}
