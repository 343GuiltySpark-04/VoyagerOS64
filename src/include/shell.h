#pragma once

#ifndef _SHELL_H
#define _SHELL_H

#define VSH_CMD_BUFFER_SIZE 100
#define VSH_VERSION "0.0.1"

void vsh_loop();

char vsh_readline();

#endif