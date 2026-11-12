#!/bin/bash

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt
get_controls

GAMEDIR=/$directory/ports/paciencia
BINARY=paciencia.${DEVICE_ARCH}

cd $GAMEDIR

> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
# gptokeyb2 emula o mouse; impede que telas touch do SDL2 briguem com ele
export SDL_TOUCH_MOUSE_EVENTS=0

# O nome do binario passa de 15 caracteres (limite do kernel para comm), entao o gptokeyb2
# recebe so o prefixo "paciencia" para o kill switch (Start+Select) conseguir achar o processo.
$GPTOKEYB2 "paciencia" -c "$GAMEDIR/paciencia.ini" &

pm_platform_helper "$GAMEDIR/$BINARY"

./$BINARY

pm_finish
