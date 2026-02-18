#include "skill_registry.h"

#include <Arduino.h>
#include <SPIFFS.h>

#include "brain_config.h"

namespace {

const char *kSkillsDir = "/skills";

struct SkillEntry {
  String name;         // e.g. "frontend_dev"
  String description;  // first-line summary (kept in RAM)
};

SkillEntry g_skills[SKILL_MAX_COUNT];
int g_skill_count = 0;
bool g_ready = false;

// Extract description from first line of skill file
// Expected format: first non-empty line is the description
String extract_description(const String &content) {
  int start = 0;
  // Skip YAML frontmatter if present
  if (content.startsWith("---")) {
    int end = content.indexOf("---", 3);
    if (end > 0) {
      // Look for description field in frontmatter
      int desc_pos = content.indexOf("description:");
      if (desc_pos > 0 && desc_pos < end) {
        String desc = content.substring(desc_pos + 12, content.indexOf('\n', desc_pos));
        desc.trim();
        return desc;
      }
      start = end + 3;
    }
  }

  // Fallback: first non-empty, non-heading line
  while (start < (int)content.length()) {
    int nl = content.indexOf('\n', start);
    if (nl < 0) nl = content.length();
    String line = content.substring(start, nl);
    line.trim();
    if (line.length() > 0 && !line.startsWith("#") && !line.startsWith("---")) {
      if (line.length() > 80) {
        line = line.substring(0, 80);
      }
      return line;
    }
    start = nl + 1;
  }
  return "No description";
}

// Extract skill name from filename (remove .md extension)
String name_from_filename(const String &filename) {
  String name = filename;
  if (name.startsWith("/")) {
    name = name.substring(name.lastIndexOf('/') + 1);
  }
  if (name.endsWith(".md")) {
    name = name.substring(0, name.length() - 3);
  }
  name.toLowerCase();
  return name;
}

// Scan /skills/ directory and build index
void scan_skills() {
  g_skill_count = 0;

  if (!SPIFFS.exists(kSkillsDir)) {
    SPIFFS.mkdir(kSkillsDir);
    Serial.println("[skills] Created /skills/ directory");
    return;
  }

  File root = SPIFFS.open(kSkillsDir);
  if (!root || !root.isDirectory()) {
    Serial.println("[skills] /skills/ is not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file && g_skill_count < SKILL_MAX_COUNT) {
    String fname = String(file.name());
    if (fname.endsWith(".md")) {
      // Read only enough to get description (first 300 bytes)
      char buf[301];
      size_t read_len = file.readBytes(buf, 300);
      buf[read_len] = '\0';
      String header = String(buf);

      g_skills[g_skill_count].name = name_from_filename(fname);
      g_skills[g_skill_count].description = extract_description(header);
      g_skill_count++;

      Serial.printf("[skills] Indexed: %s\n", g_skills[g_skill_count - 1].name.c_str());
    }
    file = root.openNextFile();
  }

  Serial.printf("[skills] %d skill(s) indexed\n", g_skill_count);
}

// Create default skills if /skills/ is empty
void create_default_skills() {
  // Morning Briefing
  String morning_path = String(kSkillsDir) + "/morning_briefing.md";
  if (!SPIFFS.exists(morning_path.c_str())) {
    File f = SPIFFS.open(morning_path.c_str(), FILE_WRITE);
    if (f) {
      f.println("---");
      f.println("name: morning_briefing");
      f.println("description: Daily morning briefing with weather, tasks, and motivation");
      f.println("---");
      f.println();
      f.println("# Morning Briefing Skill");
      f.println();
      f.println("When activated, follow these steps:");
      f.println("1. Greet the user warmly based on time of day");
      f.println("2. Run `weather` for their location if timezone is set");
      f.println("3. Run `task_list` to show pending tasks");
      f.println("4. Generate a short motivational quote");
      f.println("5. Combine everything into a friendly morning update");
      f.println();
      f.println("Keep the response concise and energetic.");
      f.close();
    }
  }

  // Frontend Dev
  String frontend_path = String(kSkillsDir) + "/frontend_dev.md";
  if (!SPIFFS.exists(frontend_path.c_str())) {
    File f = SPIFFS.open(frontend_path.c_str(), FILE_WRITE);
    if (f) {
      f.println("---");
      f.println("name: frontend_dev");
      f.println("description: Build premium web UIs with HTML, CSS, and JavaScript");
      f.println("---");
      f.println();
      f.println("# Frontend Development Skill");
      f.println();
      f.println("When building web pages or UIs:");
      f.println("1. Always use semantic HTML5 elements");
      f.println("2. Use modern CSS: flexbox/grid, custom properties, smooth transitions");
      f.println("3. Add micro-animations and hover effects for premium feel");
      f.println("4. Use a dark theme by default with vibrant accent colors");
      f.println("5. Include Google Fonts (Inter or Outfit)");
      f.println("6. Make it responsive with media queries");
      f.println("7. Add glass-morphism effects where appropriate");
      f.println("8. Use gradient backgrounds, not flat colors");
      f.println("9. Output as a single self-contained HTML file");
      f.println("10. Include all CSS and JS inline");
      f.println();
      f.println("The result should look premium and state-of-the-art.");
      f.println("Never output plain or basic-looking HTML.");
      f.close();
    }
  }

  // Code Review
  String review_path = String(kSkillsDir) + "/code_review.md";
  if (!SPIFFS.exists(review_path.c_str())) {
    File f = SPIFFS.open(review_path.c_str(), FILE_WRITE);
    if (f) {
      f.println("---");
      f.println("name: code_review");
      f.println("description: Review code for bugs, security issues, and improvements");
      f.println("---");
      f.println();
      f.println("# Code Review Skill");
      f.println();
      f.println("When reviewing code:");
      f.println("1. Check for bugs and logic errors");
      f.println("2. Check for security vulnerabilities");
      f.println("3. Check for performance issues");
      f.println("4. Suggest cleaner patterns and naming");
      f.println("5. Rate severity: CRITICAL / WARNING / INFO");
      f.println("6. Keep feedback actionable and specific");
      f.println("7. If code is good, say so briefly");
      f.close();
    }
  }
}

}  // namespace

void skill_init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[skills] SPIFFS mount failed");
    return;
  }

  create_default_skills();
  scan_skills();
  g_ready = true;
}

bool skill_list(String &list_out, String &error_out) {
  if (!g_ready) {
    error_out = "Skills not initialized";
    return false;
  }

  if (g_skill_count == 0) {
    list_out = "🧩 No skills installed.\nUse: skill_add <name> <description>: <instructions>";
    return true;
  }

  list_out = "🧩 Agent Skills (" + String(g_skill_count) + "):\n\n";
  for (int i = 0; i < g_skill_count; i++) {
    list_out += "• **" + g_skills[i].name + "** — " + g_skills[i].description + "\n";
  }
  list_out += "\nUse: skill_show <name> for details";

  return true;
}

String skill_match(const String &query) {
  if (!g_ready || g_skill_count == 0) {
    return "";
  }

  String q = query;
  q.toLowerCase();

  // Check for explicit "use <skill> skill" pattern
  for (int i = 0; i < g_skill_count; i++) {
    String name_lc = g_skills[i].name;
    name_lc.toLowerCase();

    // Explicit: "use frontend_dev skill" or "use frontend skill"
    if (q.indexOf("use " + name_lc) >= 0 || q.indexOf("skill " + name_lc) >= 0) {
      return g_skills[i].name;
    }

    // Partial name match: "use frontend" matches "frontend_dev"
    // Split name by underscore and check first part
    int us = name_lc.indexOf('_');
    if (us > 0) {
      String prefix = name_lc.substring(0, us);
      if (q.indexOf("use " + prefix) >= 0 || q.indexOf(prefix + " skill") >= 0) {
        return g_skills[i].name;
      }
    }
  }

  // Keyword matching against descriptions
  for (int i = 0; i < g_skill_count; i++) {
    String desc = g_skills[i].description;
    desc.toLowerCase();

    // Count keyword overlap between query and description
    int matches = 0;
    int words = 0;

    // Tokenize description into keywords (skip short words)
    int start = 0;
    while (start < (int)desc.length()) {
      int end = start;
      while (end < (int)desc.length() && desc[end] != ' ' && desc[end] != ',') {
        end++;
      }
      String word = desc.substring(start, end);
      word.trim();
      if (word.length() > 3) {
        words++;
        if (q.indexOf(word) >= 0) {
          matches++;
        }
      }
      start = end + 1;
    }

    // If 2+ keyword matches, consider it a hit
    if (matches >= 2) {
      return g_skills[i].name;
    }
  }

  return "";
}

bool skill_load(const String &name, String &content_out, String &error_out) {
  if (!g_ready) {
    error_out = "Skills not initialized";
    return false;
  }

  String path = String(kSkillsDir) + "/" + name + ".md";
  if (!SPIFFS.exists(path.c_str())) {
    error_out = "Skill '" + name + "' not found";
    return false;
  }

  File f = SPIFFS.open(path.c_str(), FILE_READ);
  if (!f) {
    error_out = "Failed to read skill file";
    return false;
  }

  size_t file_size = f.size();
  if (file_size > SKILL_MAX_FILE_SIZE) {
    // Read only up to max
    char *buf = (char *)malloc(SKILL_MAX_FILE_SIZE + 1);
    if (!buf) {
      f.close();
      error_out = "Out of memory";
      return false;
    }
    size_t read = f.readBytes(buf, SKILL_MAX_FILE_SIZE);
    buf[read] = '\0';
    content_out = String(buf);
    free(buf);
  } else {
    content_out = f.readString();
  }

  f.close();
  Serial.printf("[skills] Loaded skill: %s (%d bytes)\n", name.c_str(), content_out.length());
  return true;
}

bool skill_show(const String &name, String &content_out, String &error_out) {
  return skill_load(name, content_out, error_out);
}

bool skill_add(const String &name, const String &description,
               const String &instructions, String &error_out) {
  if (!g_ready) {
    error_out = "Skills not initialized";
    return false;
  }

  if (g_skill_count >= SKILL_MAX_COUNT) {
    error_out = "Max skills reached (" + String(SKILL_MAX_COUNT) + ")";
    return false;
  }

  String clean_name = name;
  clean_name.toLowerCase();
  clean_name.trim();
  // Replace spaces with underscores
  clean_name.replace(" ", "_");

  String path = String(kSkillsDir) + "/" + clean_name + ".md";

  File f = SPIFFS.open(path.c_str(), FILE_WRITE);
  if (!f) {
    error_out = "Failed to create skill file";
    return false;
  }

  f.println("---");
  f.println("name: " + clean_name);
  f.println("description: " + description);
  f.println("---");
  f.println();
  f.println("# " + clean_name + " Skill");
  f.println();
  f.print(instructions);
  f.println();
  f.close();

  // Re-index
  scan_skills();

  Serial.printf("[skills] Added skill: %s\n", clean_name.c_str());
  return true;
}

bool skill_remove(const String &name, String &error_out) {
  if (!g_ready) {
    error_out = "Skills not initialized";
    return false;
  }

  String clean_name = name;
  clean_name.toLowerCase();
  clean_name.trim();

  String path = String(kSkillsDir) + "/" + clean_name + ".md";
  if (!SPIFFS.exists(path.c_str())) {
    error_out = "Skill '" + clean_name + "' not found";
    return false;
  }

  if (!SPIFFS.remove(path.c_str())) {
    error_out = "Failed to delete skill file";
    return false;
  }

  // Re-index
  scan_skills();

  Serial.printf("[skills] Removed skill: %s\n", clean_name.c_str());
  return true;
}

String skill_get_descriptions_for_react() {
  if (!g_ready || g_skill_count == 0) {
    return "";
  }

  String out;
  out.reserve(g_skill_count * 60);
  for (int i = 0; i < g_skill_count; i++) {
    out += "  - use_skill " + g_skills[i].name + ": " + g_skills[i].description + "\n";
  }
  return out;
}
