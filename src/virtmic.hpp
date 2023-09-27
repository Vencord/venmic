/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2023  Malte Jürgens and discord-screenaudio contributors
 * Copyright (c) 2023  Vendicated and Vencord contributors
 */

#include <string>
#include <vector>

std::vector<std::string> getTargets();

void startVirtMic(std::string target);
void stopVirtMic();

void start(std::string target);