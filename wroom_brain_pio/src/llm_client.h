#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <Arduino.h>

bool llm_generate_plan(const String &task, String &plan_out, String &error_out);
bool llm_generate_reply(const String &message, String &reply_out, String &error_out);
bool llm_generate_heartbeat(const String &heartbeat_doc, String &reply_out, String &error_out);
bool llm_route_tool_command(const String &message, String &command_out, String &error_out);

#endif
