#!/bin/bash

# Cores para facilitar a leitura
VERDE='\033[0;32m'
AMARELO='\033[1;33m'
AZUL='\033[0;34m'
VERMELHO='\033[0;31m'
NC='\033[0m' # Sem cor

msg() {
    printf '%b\n' "$1"
}

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    else
        sudo "$@"
    fi
}

# Descobre o usuário alvo mesmo se o script for executado via "sudo sh fix.sh"
TARGET_USER="${SUDO_USER:-$(id -un)}"
TARGET_HOME="$(getent passwd "$TARGET_USER" 2>/dev/null | cut -d: -f6)"
if [ -z "$TARGET_HOME" ] || [ ! -d "$TARGET_HOME" ]; then
    TARGET_HOME="$HOME"
fi

run_as_target() {
    if [ "$(id -u)" -eq 0 ] && [ "$TARGET_USER" != "root" ]; then
        sudo -u "$TARGET_USER" env \
            "DISPLAY=${DISPLAY:-}" \
            "WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-}" \
            "XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/run/user/$(id -u "$TARGET_USER")}" \
            "XDG_CONFIG_HOME=${XDG_CONFIG_HOME:-$TARGET_HOME/.config}" \
            "XDG_STATE_HOME=${XDG_STATE_HOME:-$TARGET_HOME/.local/state}" \
            "$@"
    else
        "$@"
    fi
}

clear 2>/dev/null || true
msg "${AZUL}======================================================${NC}"
msg "${AZUL}    CORREÇÃO DE TECLADO PARA ZACOS 9 (DEBIAN 13)      ${NC}"
msg "${AZUL}                 Wayland + XWayland                   ${NC}"
msg "${AZUL}======================================================${NC}"
msg ""
msg "Este script configura o teclado corretamente no ZacOS 9 (Wayland):"
msg " - Habilita a variante ABNT2 na interface visual (Painel Teclado);"
msg " - Atualiza /etc/default/keyboard (modelo pc105);"
msg " - Ajusta o compositor zacos9-wm (~/.config/zacos9/desktop.conf);"
msg " - Sincroniza a camada XWayland (apps X11, Office, Games, etc.)."
msg ""
msg "${AMARELO}Qual layout você deseja utilizar como padrão?${NC}"
msg "1) Português do Brasil (ABNT2) - Layout br, variante abnt2, pc105 (com 'Ç' e acentos)"
msg "2) Estados Unidos Internacional (US Intl) - Layout us, variante intl, pc105"
msg "3) Sair"
msg ""
printf "Digite o número da opção desejada: "
read -r opcao

case "$opcao" in
    1)
        MODELO="pc105"
        LAYOUT="br"
        VARIANTE="abnt2"
        NOME="Português do Brasil (ABNT2)"
        ;;
    2)
        MODELO="pc105"
        LAYOUT="us"
        VARIANTE="intl"
        NOME="US Internacional"
        ;;
    3)
        msg "Saindo..."
        exit 0
        ;;
    *)
        msg "${VERMELHO}Opção inválida.${NC}"
        exit 1
        ;;
esac

LAYOUT_COMPLETO="${LAYOUT}(${VARIANTE})"

msg ""
msg "${VERDE}--> Configurando para: $NOME [$LAYOUT_COMPLETO / $MODELO] (usuário: $TARGET_USER)${NC}"
msg ""

# 1. Garantir que a variante abnt2 exista nos arquivos XML do XKB (Debian 13 / xkb-data)
#    O zacos9-controlpanel e o zacos9-keytags leem /usr/share/X11/xkb/rules/evdev.xml (ou base.xml).
#    Sem o bloco <variant> abnt2 dentro de <name>br</name>, o modelo ABNT2 nao aparece na interface visual.
msg "${AMARELO}[1/4] Habilitando variante ABNT2 na interface visual do ZacOS 9 (requer senha)...${NC}"

run_root python3 - <<'PYEOF'
import os
import re
import xml.etree.ElementTree as ET

XML_FILES = [
    "/usr/share/X11/xkb/rules/evdev.xml",
    "/usr/share/X11/xkb/rules/base.xml",
]

VARIANT_BLOCK = (
    "        <variant>\n"
    "          <configItem>\n"
    "            <name>abnt2</name>\n"
    "            <description>Portuguese (Brazil, ABNT2)</description>\n"
    "          </configItem>\n"
    "        </variant>\n"
)

for path in XML_FILES:
    if not os.path.isfile(path):
        continue

    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # Verifica se 'abnt2' já está registrado dentro do layout 'br'
    tree = ET.fromstring(content)
    already_present = False
    for layout in tree.findall(".//layout"):
        if layout.findtext("./configItem/name") == "br":
            for v in layout.findall("./variantList/variant"):
                if v.findtext("./configItem/name") == "abnt2":
                    already_present = True
                    break
            break

    if already_present:
        print(f"OK: variante 'abnt2' já disponível em {path}")
        continue

    # Insere a variante abnt2 no início da <variantList> do layout <name>br</name>
    pattern = re.compile(
        r"(<layout>\s*<configItem>\s*<name>br</name>.*?</configItem>\s*<variantList>\s*\n)",
        re.DOTALL,
    )
    new_content, count = pattern.subn(r"\1" + VARIANT_BLOCK, content, count=1)
    if count != 1:
        raise RuntimeError(f"Não foi possível localizar a variantList de 'br' em {path}")

    # Valida o XML resultante antes de gravar
    ET.fromstring(new_content)

    backup = path + ".bak"
    if not os.path.exists(backup):
        with open(backup, "w", encoding="utf-8") as bf:
            bf.write(content)

    tmp_path = path + ".tmp"
    with open(tmp_path, "w", encoding="utf-8") as tf:
        tf.write(new_content)
    os.replace(tmp_path, path)
    print(f"Atualizado: variante 'abnt2' adicionada em {path}")
PYEOF

if [ $? -eq 0 ]; then
    msg "${VERDE}Sucesso: Interface visual preparada para reconhecer ABNT2.${NC}"
else
    msg "${VERMELHO}Aviso: Falha ao atualizar regras XML do XKB.${NC}"
fi

# 2. Configurar o sistema base (/etc/default/keyboard)
#    Tenta localectl primeiro; se systemd-localed retornar Access denied (comum no Debian 13 com
#    ReadOnlyPaths/polkit sob sudo), grava /etc/default/keyboard diretamente.
msg ""
msg "${AMARELO}[2/4] Aplicando configuração no sistema base (/etc/default/keyboard)...${NC}"

if run_root localectl set-x11-keymap "$LAYOUT" "$MODELO" "$VARIANTE" 2>/dev/null; then
    msg "${VERDE}Sucesso: Configuração do sistema atualizada via localectl.${NC}"
else
    run_root python3 - "$MODELO" "$LAYOUT" "$VARIANTE" <<'PYEOF'
import os
import sys

modelo, layout, variante = sys.argv[1:4]
kbd_path = "/etc/default/keyboard"

updates = {
    "XKBMODEL": f'"{modelo}"',
    "XKBLAYOUT": f'"{layout}"',
    "XKBVARIANT": f'"{variante}"',
    "XKBOPTIONS": '""',
}

lines = []
if os.path.isfile(kbd_path):
    with open(kbd_path, "r", encoding="utf-8") as f:
        lines = f.read().splitlines()

seen = set()
new_lines = []
for line in lines:
    stripped = line.strip()
    matched = False
    for key, val in updates.items():
        if stripped.startswith(key + "="):
            new_lines.append(f"{key}={val}")
            seen.add(key)
            matched = True
            break
    if not matched:
        new_lines.append(line)

for key, val in updates.items():
    if key not in seen:
        new_lines.append(f"{key}={val}")

if not any(l.strip().startswith("BACKSPACE=") for l in new_lines):
    new_lines.append('BACKSPACE="guess"')

tmp_path = kbd_path + ".tmp"
with open(tmp_path, "w", encoding="utf-8") as f:
    f.write("\n".join(new_lines) + "\n")
os.replace(tmp_path, kbd_path)
PYEOF

    if [ $? -eq 0 ]; then
        msg "${VERDE}Sucesso: /etc/default/keyboard atualizado diretamente (localectl bloqueado pelo polkit/systemd-localed).${NC}"
    else
        msg "${VERMELHO}Erro: Falha ao atualizar /etc/default/keyboard.${NC}"
    fi
fi

# 3. Configurar o compositor Wayland do ZacOS 9 (~/.config/zacos9/desktop.conf)
#    O zacos9-wm monitora esta pasta via inotify e aplica 'keyboard-layouts' imediatamente,
#    substituindo qualquer configuracao antiga como br(nodeadkeys) que desativa os acentos.
msg ""
msg "${AMARELO}[3/4] Aplicando layout no compositor Wayland do ZacOS 9...${NC}"

CONF_DIR="$TARGET_HOME/.config/zacos9"
CONF_FILE="$CONF_DIR/desktop.conf"
STATE_LAYOUT_FILE="$TARGET_HOME/.local/state/zacos9/keyboard-layout"

run_as_target mkdir -p "$CONF_DIR"

run_as_target python3 - "$CONF_FILE" "$LAYOUT_COMPLETO" <<'PYEOF'
import os
import sys

conf_path = sys.argv[1]
layout_val = sys.argv[2]

lines = []
if os.path.isfile(conf_path):
    with open(conf_path, "r", encoding="utf-8") as f:
        lines = f.read().splitlines()

new_lines = []
in_general = False
has_general = False
written = False

for line in lines:
    stripped = line.strip()
    if stripped.startswith("[") and stripped.endswith("]"):
        if in_general and not written:
            new_lines.append(f"keyboard-layouts={layout_val}")
            written = True
        in_general = (stripped == "[General]")
        if in_general:
            has_general = True
        new_lines.append(line)
    elif in_general and stripped.startswith("keyboard-layouts="):
        if not written:
            new_lines.append(f"keyboard-layouts={layout_val}")
            written = True
    else:
        new_lines.append(line)

if not has_general:
    if new_lines and new_lines[-1] != "":
        new_lines.append("")
    new_lines.append("[General]")
    new_lines.append(f"keyboard-layouts={layout_val}")
elif not written:
    new_lines.append(f"keyboard-layouts={layout_val}")

tmp_path = conf_path + ".tmp"
with open(tmp_path, "w", encoding="utf-8") as f:
    f.write("\n".join(new_lines) + "\n")
os.replace(tmp_path, conf_path)
PYEOF

if [ $? -eq 0 ]; then
    # Limpa o estado de layout ativo anterior para forçar o uso do novo layout padrão
    run_as_target rm -f "$STATE_LAYOUT_FILE"
    msg "${VERDE}Sucesso: $CONF_FILE atualizado com keyboard-layouts=$LAYOUT_COMPLETO.${NC}"
else
    msg "${VERMELHO}Erro: Falha ao atualizar $CONF_FILE.${NC}"
fi

# Evita que o im-config injete GTK_IM_MODULE=fcitx / QT_IM_MODULE=fcitx em sessões não japonesas
# (no ZacOS 9, o fcitx5 só é iniciado automaticamente quando LANG começa com 'ja')
if command -v im-config >/dev/null 2>&1; then
    case "${LANG:-}" in
        ja*) ;;
        *)
            if [ ! -f "$TARGET_HOME/.xinputrc" ]; then
                run_as_target im-config -n none >/dev/null 2>&1 && \
                    msg "${VERDE}Sucesso: im-config ajustado para 'none' (evita conflito com fcitx inativo).${NC}"
            fi
            ;;
    esac
fi

# 4. Sincronizar XWayland e atualizar o autostart
msg ""
msg "${AMARELO}[4/4] Sincronizando XWayland e atualizando inicialização automática...${NC}"

if command -v setxkbmap >/dev/null 2>&1 && [ -n "${DISPLAY:-}" ]; then
    if run_as_target setxkbmap -model "$MODELO" -layout "$LAYOUT" -variant "$VARIANTE" 2>/dev/null; then
        msg "${VERDE}Sucesso: XWayland ($DISPLAY) atualizado para $MODELO / $LAYOUT($VARIANTE).${NC}"
    fi
fi

DIR_AUTOSTART="$TARGET_HOME/.config/autostart"
FILE_DESKTOP="$DIR_AUTOSTART/force_keyboard_layout.desktop"

run_as_target mkdir -p "$DIR_AUTOSTART"
run_as_target tee "$FILE_DESKTOP" >/dev/null <<EOF
[Desktop Entry]
Type=Application
Name=Forcar Layout Teclado XWayland
Exec=setxkbmap -model $MODELO -layout $LAYOUT -variant $VARIANTE
StartupNotify=false
OnlyShowIn=ZacOS9;
EOF

run_as_target chmod +x "$FILE_DESKTOP"

msg "${VERDE}Sucesso: Entrada de inicialização atualizada em $FILE_DESKTOP${NC}"
msg ""
msg "${AZUL}======================================================${NC}"
msg "${VERDE}TUDO PRONTO!${NC}"
msg "O compositor Wayland do ZacOS 9 já deve aplicar o novo layout"
msg "imediatamente (inclusive na interface visual em Teclado)."
msg "Se algum aplicativo ainda não reconhecer os acentos, faça"
msg "${AMARELO}Logout${NC} e entre novamente na sessão."
msg "${AZUL}======================================================${NC}"
